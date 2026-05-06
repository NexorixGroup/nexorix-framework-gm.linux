/*
 * Nexorix Discord — embedded in Lua.dll
 * Gateway WebSocket (para bot aparecer online) + REST para envio de mensagens.
 *
 * Fluxo:
 *   1. onLoad: lê config.json
 *   2. onReady: inicia worker thread
 *   3. Worker: conecta ao Gateway WSS, envia IDENTIFY, mantém heartbeat
 *      → bot aparece ONLINE no Discord
 *   4. Eventos MESSAGE_CREATE → in_queue_ → Lua no tick()
 *   5. Lua chama sendMessage() → REST POST via httplib
 */

#include "discord_main.hpp"
#include <sdk.hpp>

// REST (httplib já tem CPPHTTPLIB_OPENSSL_SUPPORT via CMake define)
#include "../../../../lib/cpp-httplib/httplib.h"

#include <nlohmann/json.hpp>
#include <chrono>
#include <sstream>
#include <fstream>
#include <cstring>
#include <cstdio>

// OpenSSL para WebSocket raw
#include <openssl/ssl.h>
#include <openssl/err.h>

#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32.lib")
   typedef SOCKET sock_t;
#  define SOCK_INVALID INVALID_SOCKET
#  define sock_close   closesocket
#else
#  include <sys/socket.h>
#  include <netdb.h>
#  include <unistd.h>
   typedef int sock_t;
#  define SOCK_INVALID (-1)
#  define sock_close   close
#endif

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// WebSocket frame helpers (RFC 6455)
// ---------------------------------------------------------------------------

// Envia um frame WebSocket text (opcode 0x1) com máscara de cliente
static bool ws_send_text(SSL* ssl, const std::string& payload)
{
    std::vector<uint8_t> frame;
    size_t len = payload.size();

    frame.push_back(0x81); // FIN + opcode TEXT

    uint8_t mask_key[4] = { 0x37, 0xfa, 0x21, 0x3d };

    if (len <= 125) {
        frame.push_back((uint8_t)(0x80 | len));
    } else if (len <= 65535) {
        frame.push_back(0x80 | 126);
        frame.push_back((uint8_t)(len >> 8));
        frame.push_back((uint8_t)(len & 0xFF));
    } else {
        frame.push_back(0x80 | 127);
        for (int i = 7; i >= 0; --i)
            frame.push_back((uint8_t)((len >> (8*i)) & 0xFF));
    }

    // Mask key
    frame.insert(frame.end(), mask_key, mask_key + 4);

    // Masked payload
    for (size_t i = 0; i < len; ++i)
        frame.push_back((uint8_t)(payload[i] ^ mask_key[i % 4]));

    int written = SSL_write(ssl, frame.data(), (int)frame.size());
    return written == (int)frame.size();
}

// Lê um frame WebSocket completo (sem máscara — servidor não mascara)
// Retorna o payload text ou "" em caso de erro/ping/close
static std::string ws_read_frame(SSL* ssl)
{
    uint8_t header[2];
    if (SSL_read(ssl, header, 2) != 2) return "";

    // bool fin    = (header[0] & 0x80) != 0;
    uint8_t opcode = header[0] & 0x0F;
    bool masked    = (header[1] & 0x80) != 0;
    uint64_t len   = header[1] & 0x7F;

    if (len == 126) {
        uint8_t ext[2];
        if (SSL_read(ssl, ext, 2) != 2) return "";
        len = ((uint64_t)ext[0] << 8) | ext[1];
    } else if (len == 127) {
        uint8_t ext[8];
        if (SSL_read(ssl, ext, 8) != 8) return "";
        len = 0;
        for (int i = 0; i < 8; ++i) len = (len << 8) | ext[i];
    }

    uint8_t mask[4] = {};
    if (masked) {
        if (SSL_read(ssl, mask, 4) != 4) return "";
    }

    if (len == 0) return "";
    if (len > 10 * 1024 * 1024) return ""; // sanity: max 10MB

    std::vector<uint8_t> buf(len);
    size_t received = 0;
    while (received < len) {
        int r = SSL_read(ssl, buf.data() + received, (int)(len - received));
        if (r <= 0) return "";
        received += r;
    }

    if (masked) {
        for (size_t i = 0; i < len; ++i)
            buf[i] ^= mask[i % 4];
    }

    // Opcode 0x9 = Ping → responde Pong (0xA)
    if (opcode == 0x9) {
        uint8_t pong[2] = { 0x8A, 0x00 };
        SSL_write(ssl, pong, 2);
        return "";
    }

    // Opcode 0x8 = Close
    if (opcode == 0x8) return "\x00CLOSE";

    // Opcode 0x1 = Text, 0x0 = Continuation
    if (opcode == 0x1 || opcode == 0x0)
        return std::string(buf.begin(), buf.end());

    return "";
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void DiscordComponent::onLoad(ICore* core)
{
    core_ = core;

    // O IConfig nao conhece as chaves "discord.*" pois nao estao nos Defaults.
    // Lemos o config.json diretamente com nlohmann/json.
    try {
        std::ifstream f("config.json");
        if (!f.is_open()) {
            core_->printLn("[Discord] config.json nao encontrado.");
            return;
        }
        auto cfg = nlohmann::json::parse(f);

        auto& disc = cfg.at("discord");

        std::string enabled_str = disc.value("enabled", "false");
        if (enabled_str == "false" || enabled_str == "0") {
            core_->printLn("[Discord] Disabled via config.");
            return;
        }

        token_         = disc.value("token",      "");
        prefix_        = disc.value("prefix",     "!");
        log_channel_id_= disc.value("channel_id", "");

        if (token_.empty()) {
            core_->printLn("[Discord] Token vazio. Componente inativo.");
            return;
        }
        if (log_channel_id_.empty()) {
            core_->printLn("[Discord] channel_id vazio. Verifique config.json.");
        }

    } catch (const std::exception& e) {
        core_->printLn("[Discord] Erro ao ler config.json: %s", e.what());
        return;
    }

    enabled_ = true;
    core_->printLn("[Discord] Component loaded. Prefix='%s' Channel='%s'",
        prefix_.c_str(), log_channel_id_.c_str());
}

void DiscordComponent::onInit(IComponentList*) {}

void DiscordComponent::onReady()
{
    if (!enabled_) return;
    running_ = true;
    worker_ = std::thread(&DiscordComponent::workerLoop, this);
    core_->printLn("[Discord] Worker thread started.");
}

DiscordComponent::~DiscordComponent()
{
    running_ = false;
    if (worker_.joinable())
        worker_.join();
    if (L_) {
        if (on_ready_ref_   != LUA_NOREF) luaL_unref(L_, LUA_REGISTRYINDEX, on_ready_ref_);
        if (on_message_ref_ != LUA_NOREF) luaL_unref(L_, LUA_REGISTRYINDEX, on_message_ref_);
        for (auto& [name, ref] : command_refs_)
            luaL_unref(L_, LUA_REGISTRYINDEX, ref);
    }
}

// ---------------------------------------------------------------------------
// Lua API
// ---------------------------------------------------------------------------

void DiscordComponent::sendMessage(const std::string& channel_id, const std::string& content)
{
    if (!enabled_) return;
    std::lock_guard<std::mutex> lock(out_mutex_);
    out_queue_.push({ channel_id, content, "" });
}

void DiscordComponent::sendEmbed(const std::string& channel_id, const std::string& embed_json)
{
    if (!enabled_) return;
    std::lock_guard<std::mutex> lock(out_mutex_);
    out_queue_.push({ channel_id, "", embed_json });
}

void DiscordComponent::setStatus(const std::string& text)
{
    if (!enabled_) return;
    std::lock_guard<std::mutex> lock(status_mutex_);
    pending_status_ = text;
    status_dirty_   = true;
}

void DiscordComponent::setOnReadyRef(lua_State* L, int ref)
{
    L_ = L;
    if (on_ready_ref_ != LUA_NOREF) luaL_unref(L, LUA_REGISTRYINDEX, on_ready_ref_);
    on_ready_ref_ = ref;
}

void DiscordComponent::setOnMessageRef(lua_State* L, int ref)
{
    L_ = L;
    if (on_message_ref_ != LUA_NOREF) luaL_unref(L, LUA_REGISTRYINDEX, on_message_ref_);
    on_message_ref_ = ref;
}

void DiscordComponent::registerCommand(const std::string& name, lua_State* L, int ref)
{
    L_ = L;
    auto it = command_refs_.find(name);
    if (it != command_refs_.end()) luaL_unref(L, LUA_REGISTRYINDEX, it->second);
    command_refs_[name] = ref;
}

// ---------------------------------------------------------------------------
// tick() — main thread
// ---------------------------------------------------------------------------

void DiscordComponent::tick(lua_State* L)
{
    if (!enabled_) return;
    L_ = L;

    std::queue<DiscordInEvent> local;
    {
        std::lock_guard<std::mutex> lock(in_mutex_);
        std::swap(local, in_queue_);
    }
    while (!local.empty()) {
        DiscordInEvent ev = std::move(local.front());
        local.pop();
        if (ev.type == DiscordEventType::Ready)   dispatchReady(L);
        else                                       dispatchMessage(L, ev);
    }
}

// ---------------------------------------------------------------------------
// Dispatch helpers (main thread)
// ---------------------------------------------------------------------------

void DiscordComponent::dispatchReady(lua_State* L)
{
    lua_getglobal(L, "NX");
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "_dispatch");
        if (lua_isfunction(L, -1)) {
            lua_pushstring(L, "OnDiscordReady");
            lua_pcall(L, 1, 0, 0);
        } else lua_pop(L, 1);
    }
    lua_pop(L, 1);

    if (on_ready_ref_ != LUA_NOREF) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, on_ready_ref_);
        if (lua_isfunction(L, -1)) {
            if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
                core_->printLn("[Discord] Error in onReady: %s", lua_tostring(L, -1));
                lua_pop(L, 1);
            }
        } else lua_pop(L, 1);
    }
}

void DiscordComponent::dispatchMessage(lua_State* L, const DiscordInEvent& ev)
{
    lua_getglobal(L, "NX");
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "_dispatch");
        if (lua_isfunction(L, -1)) {
            lua_pushstring(L, "OnDiscordMessage");
            lua_pushstring(L, ev.author.c_str());
            lua_pushstring(L, ev.channel.c_str());
            lua_pushstring(L, ev.content.c_str());
            lua_pcall(L, 4, 0, 0);
        } else lua_pop(L, 1);
    }
    lua_pop(L, 1);

    if (on_message_ref_ != LUA_NOREF) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, on_message_ref_);
        if (lua_isfunction(L, -1)) {
            lua_pushstring(L, ev.author.c_str());
            lua_pushstring(L, ev.content.c_str());
            if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
                core_->printLn("[Discord] Error in onMessage: %s", lua_tostring(L, -1));
                lua_pop(L, 1);
            }
        } else lua_pop(L, 1);
    }

    if (!prefix_.empty() && ev.content.rfind(prefix_, 0) == 0) {
        std::string body = ev.content.substr(prefix_.size());
        std::istringstream iss(body);
        std::string cmd;
        iss >> cmd;
        for (char& c : cmd) c = (char)tolower((unsigned char)c);

        auto it = command_refs_.find(cmd);
        if (it != command_refs_.end()) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, it->second);
            if (lua_isfunction(L, -1)) {
                lua_pushstring(L, ev.author.c_str());
                lua_newtable(L);
                std::string arg; int idx = 1;
                while (iss >> arg) {
                    lua_pushstring(L, arg.c_str());
                    lua_rawseti(L, -2, idx++);
                }
                if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
                    core_->printLn("[Discord] Error in command '%s': %s",
                        cmd.c_str(), lua_tostring(L, -1));
                    lua_pop(L, 1);
                }
            } else lua_pop(L, 1);
        }
    }
}

// ---------------------------------------------------------------------------
// REST helpers
// ---------------------------------------------------------------------------

static const char* DISCORD_HOST = "discord.com";
static const int   DISCORD_PORT = 443;

bool DiscordComponent::httpGet(const std::string& path, std::string& out_body)
{
    httplib::SSLClient cli(DISCORD_HOST, DISCORD_PORT);
    cli.set_connection_timeout(10);
    cli.set_read_timeout(10);
    cli.enable_server_certificate_verification(false);
    httplib::Headers hdrs = {
        { "Authorization", "Bot " + token_ },
        { "User-Agent",    "NexorixBot/1.0" },
    };
    auto res = cli.Get(path, hdrs);
    if (!res || res->status < 200 || res->status >= 300) return false;
    out_body = res->body;
    return true;
}

bool DiscordComponent::httpPost(const std::string& path, const std::string& body, std::string& out_body)
{
    httplib::SSLClient cli(DISCORD_HOST, DISCORD_PORT);
    cli.set_connection_timeout(10);
    cli.set_read_timeout(10);
    cli.enable_server_certificate_verification(false);
    httplib::Headers hdrs = {
        { "Authorization", "Bot " + token_ },
        { "User-Agent",    "NexorixBot/1.0" },
    };
    auto res = cli.Post(path, hdrs, body, "application/json");
    if (!res || res->status < 200 || res->status >= 300) return false;
    out_body = res->body;
    return true;
}

bool DiscordComponent::httpPatch(const std::string& path, const std::string& body)
{
    httplib::SSLClient cli(DISCORD_HOST, DISCORD_PORT);
    cli.set_connection_timeout(10);
    cli.set_read_timeout(10);
    cli.enable_server_certificate_verification(false);
    httplib::Headers hdrs = {
        { "Authorization", "Bot " + token_ },
        { "User-Agent",    "NexorixBot/1.0" },
    };
    auto res = cli.Patch(path, hdrs, body, "application/json");
    return res && res->status >= 200 && res->status < 300;
}

// ---------------------------------------------------------------------------
// REST: envio de mensagens pendentes
// ---------------------------------------------------------------------------

void DiscordComponent::flushOutbound()
{
    std::queue<DiscordOutMsg> local;
    {
        std::lock_guard<std::mutex> lock(out_mutex_);
        std::swap(local, out_queue_);
    }
    while (!local.empty()) {
        DiscordOutMsg msg = std::move(local.front());
        local.pop();

        std::string payload;
        if (!msg.embed_json.empty()) {
            // Embed: {"embeds": [{ ...embed fields... }]}
            payload = "{\"embeds\":[" + msg.embed_json + "]}";
        } else {
            // Mensagem simples
            if (msg.content.size() > 2000) msg.content = msg.content.substr(0, 1997) + "...";
            // Escape content for JSON
            std::string esc;
            for (char c : msg.content) {
                if      (c == '"')  esc += "\\\"";
                else if (c == '\\') esc += "\\\\";
                else if (c == '\n') esc += "\\n";
                else if (c == '\r') esc += "\\r";
                else                esc += c;
            }
            payload = "{\"content\":\"" + esc + "\"}";
        }

        std::string path = "/api/v10/channels/" + msg.channel_id + "/messages";
        std::string resp;
        httpPost(path, payload, resp);
    }
}

// ---------------------------------------------------------------------------
// Gateway WebSocket — conecta, identifica, mantém heartbeat, recebe eventos
// ---------------------------------------------------------------------------

// Obtém a URL do Gateway via REST
std::string DiscordComponent::fetchGatewayUrl()
{
    std::string body;
    if (!httpGet("/api/v10/gateway/bot", body)) return "";
    try {
        auto j = json::parse(body);
        return j.value("url", "");
    } catch (...) { return ""; }
}

// Conecta ao Gateway WSS e retorna SSL* como void* (ou nullptr em falha)
void* DiscordComponent::gatewayConnect(const std::string& gateway_url)
{
    // Extrai host (remove "wss://")
    std::string host = gateway_url;
    if (host.rfind("wss://", 0) == 0) host = host.substr(6);
    if (host.rfind("ws://",  0) == 0) host = host.substr(5);
    // Remove path se houver
    auto slash = host.find('/');
    if (slash != std::string::npos) host = host.substr(0, slash);

#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
#endif

    // Resolve DNS
    struct addrinfo hints = {}, *res = nullptr;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host.c_str(), "443", &hints, &res) != 0) return nullptr;

    sock_t sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == SOCK_INVALID) { freeaddrinfo(res); return nullptr; }

    if (connect(sock, res->ai_addr, (int)res->ai_addrlen) != 0) {
        sock_close(sock); freeaddrinfo(res); return nullptr;
    }
    freeaddrinfo(res);

    // TLS
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
    SSL* ssl = SSL_new(ctx);
    SSL_CTX_free(ctx);
    SSL_set_fd(ssl, (int)sock);
    SSL_set_tlsext_host_name(ssl, host.c_str());

    if (SSL_connect(ssl) != 1) {
        SSL_free(ssl); sock_close(sock); return nullptr;
    }

    // WebSocket Upgrade
    // Gera uma chave base64 simples (não precisa ser criptograficamente segura)
    const char* ws_key = "dGhlIHNhbXBsZSBub25jZQ=="; // chave fixa válida para handshake
    std::string upgrade =
        "GET /?v=10&encoding=json HTTP/1.1\r\n"
        "Host: " + host + "\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: " + std::string(ws_key) + "\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n";

    SSL_write(ssl, upgrade.c_str(), (int)upgrade.size());

    // Lê resposta HTTP do upgrade (até \r\n\r\n)
    std::string resp;
    char ch;
    while (resp.size() < 4096) {
        if (SSL_read(ssl, &ch, 1) != 1) break;
        resp += ch;
        if (resp.size() >= 4 && resp.substr(resp.size()-4) == "\r\n\r\n") break;
    }

    if (resp.find("101") == std::string::npos) {
        SSL_free(ssl); sock_close(sock); return nullptr;
    }

    return ssl; // conexão WebSocket estabelecida
}

// Loop principal do Gateway
void DiscordComponent::gatewayLoop(void* ssl_ptr)
{
    SSL* ssl = static_cast<SSL*>(ssl_ptr);
    int heartbeat_interval = 41250; // ms — será atualizado pelo Hello
    auto last_heartbeat    = std::chrono::steady_clock::now();
    int  sequence          = -1;
    bool identified        = false;

    // Configura timeout de leitura não-bloqueante via SSL_CTX
    // Usamos select() para não bloquear indefinidamente
#ifdef _WIN32
    SOCKET sock = (SOCKET)SSL_get_fd(ssl);
#else
    int sock = SSL_get_fd(ssl);
#endif

    while (running_) {
        // Verifica se há dados disponíveis (timeout 100ms)
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sock, &fds);
        struct timeval tv = { 0, 100000 }; // 100ms
        int sel = select((int)sock + 1, &fds, nullptr, nullptr, &tv);

        if (sel > 0) {
            std::string frame = ws_read_frame(ssl);

            if (frame == "\x00CLOSE" || frame.empty()) {
                // Conexão fechada — sai para reconectar
                break;
            }

            try {
                auto j = json::parse(frame);
                int op = j.value("op", -1);

                // Atualiza sequence
                if (!j["s"].is_null()) sequence = j["s"].get<int>();

                // Op 10: Hello — recebe heartbeat_interval e identifica
                if (op == 10) {
                    heartbeat_interval = j["d"]["heartbeat_interval"].get<int>();

                    // Envia heartbeat imediato
                    json hb = { {"op", 1}, {"d", sequence >= 0 ? json(sequence) : json(nullptr)} };
                    ws_send_text(ssl, hb.dump());
                    last_heartbeat = std::chrono::steady_clock::now();

                    // Envia IDENTIFY
                    json identify = {
                        {"op", 2},
                        {"d", {
                            {"token",   token_},
                            {"intents", (1 << 9) | (1 << 15)}, // GUILD_MESSAGES + MESSAGE_CONTENT
                            {"properties", {
                                {"os",      "windows"},
                                {"browser", "nexorix"},
                                {"device",  "nexorix"}
                            }},
                            {"presence", {
                                {"status",     "online"},
                                {"afk",        false},
                                {"activities", json::array()}
                            }}
                        }}
                    };
                    ws_send_text(ssl, identify.dump());
                    identified = true;
                }

                // Op 0: Dispatch — eventos do servidor
                else if (op == 0) {
                    std::string t = j.value("t", "");

                    // READY — bot está online
                    if (t == "READY") {
                        core_->printLn("[Discord] Bot online! (Gateway READY)");
                        std::lock_guard<std::mutex> lock(in_mutex_);
                        in_queue_.push({ DiscordEventType::Ready, "", "", "" });
                    }

                    // MESSAGE_CREATE — nova mensagem
                    else if (t == "MESSAGE_CREATE") {
                        auto& d = j["d"];

                        // Ignora mensagens de bots
                        if (d.value("author", json::object()).value("bot", false)) continue;

                        std::string content = d.value("content", "");
                        std::string channel = d.value("channel_id", "");
                        auto& author = d["author"];
                        std::string username = author.value("username", "unknown");
                        std::string discrim  = author.value("discriminator", "0000");
                        std::string author_str = (discrim == "0") ? username : username + "#" + discrim;

                        std::lock_guard<std::mutex> lock(in_mutex_);
                        in_queue_.push({ DiscordEventType::Message, author_str, channel, content });
                    }
                }

                // Op 11: Heartbeat ACK — ok
                // Op 7: Reconnect — sai para reconectar
                else if (op == 7) break;

                // Op 9: Invalid Session
                else if (op == 9) {
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    break;
                }

            } catch (...) {}
        }

        // Heartbeat periódico
        if (identified) {
            auto now     = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_heartbeat).count();
            if (elapsed >= heartbeat_interval) {
                json hb = { {"op", 1}, {"d", sequence >= 0 ? json(sequence) : json(nullptr)} };
                ws_send_text(ssl, hb.dump());
                last_heartbeat = now;
            }
        }

        // Flush mensagens pendentes (REST)
        flushOutbound();

        // Status update
        if (identified) {
            std::string status_text;
            {
                std::lock_guard<std::mutex> lock(status_mutex_);
                if (status_dirty_) {
                    status_text   = pending_status_;
                    status_dirty_ = false;
                }
            }
            if (!status_text.empty()) {
                // Atualiza presença via Gateway Op 3
                json presence = {
                    {"op", 3},
                    {"d", {
                        {"since",      nullptr},
                        {"status",     "online"},
                        {"afk",        false},
                        {"activities", json::array({
                            {{"name", status_text}, {"type", 0}}
                        })}
                    }}
                };
                ws_send_text(ssl, presence.dump());
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Worker thread principal — conecta e reconecta automaticamente
// ---------------------------------------------------------------------------

void DiscordComponent::workerLoop()
{
    SSL_library_init();
    SSL_load_error_strings();

    while (running_) {
        core_->printLn("[Discord] Connecting to Gateway...");

        // Obtém URL do Gateway
        std::string gw_url = fetchGatewayUrl();
        if (gw_url.empty()) {
            core_->printLn("[Discord] Failed to get Gateway URL. Retrying in 10s...");
            for (int i = 0; i < 100 && running_; ++i)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // Conecta via WebSocket
        void* ssl = gatewayConnect(gw_url);
        if (!ssl) {
            core_->printLn("[Discord] Gateway connection failed. Retrying in 5s...");
            for (int i = 0; i < 50 && running_; ++i)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        core_->printLn("[Discord] Gateway connected.");

        // Loop de eventos
        gatewayLoop(ssl);

        // Limpa conexão
#ifdef _WIN32
        SOCKET sock = (SOCKET)SSL_get_fd(static_cast<SSL*>(ssl));
#else
        int sock = SSL_get_fd(static_cast<SSL*>(ssl));
#endif
        SSL_shutdown(static_cast<SSL*>(ssl));
        SSL_free(static_cast<SSL*>(ssl));
        sock_close(sock);

        if (!running_) break;

        // Reconecta após 5s
        core_->printLn("[Discord] Gateway disconnected. Reconnecting in 5s...");
        for (int i = 0; i < 50 && running_; ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// Note: DiscordComponent is embedded inside Lua.dll — no COMPONENT_ENTRY_POINT.
