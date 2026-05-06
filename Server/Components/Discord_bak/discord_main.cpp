/*
 * Nexorix Discord Component
 *
 * Integra um bot Discord ao servidor via REST polling (HTTPS).
 * Roda em thread separada, sem bloquear o loop principal.
 *
 * Fluxo:
 *   1. onLoad: le config.json (token, prefix, channel_id)
 *   2. onReady: inicia worker thread
 *   3. Worker: GET /channels/{id}/messages?after={last_id} a cada 1s
 *   4. Mensagens novas -> in_queue_ -> despachadas para Lua no tick()
 *   5. Lua chama sendMessage() -> out_queue_ -> worker envia POST
 */

#include "discord_main.hpp"

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "../../../lib/cpp-httplib/httplib.h"

#include <nlohmann/json.hpp>
#include <chrono>
#include <sstream>

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// IComponent lifecycle
// ---------------------------------------------------------------------------

SemanticVersion DiscordComponent::componentVersion() const
{
    return SemanticVersion(1, 0, 0, 0);
}

void DiscordComponent::onLoad(ICore* core)
{
    core_ = core;

    // Read config from server config
    IConfig& cfg = core_->getConfig();

    // discord.enabled
    auto enabled_val = cfg.getString("discord.enabled");
    if (enabled_val == StringView("false") || enabled_val == StringView("0"))
    {
        core_->printLn("[Discord] Disabled via config (discord.enabled=false).");
        return;
    }

    // discord.token — never logged
    StringView token_sv = cfg.getString("discord.token");
    if (token_sv.empty())
    {
        core_->printLn("[Discord] No token configured (discord.token). Component inactive.");
        return;
    }
    token_ = std::string(token_sv.data(), token_sv.size());

    // discord.prefix (default "!")
    StringView prefix_sv = cfg.getString("discord.prefix");
    prefix_ = prefix_sv.empty() ? "!" : std::string(prefix_sv.data(), prefix_sv.size());

    // discord.channel_id
    StringView ch_sv = cfg.getString("discord.channel_id");
    log_channel_id_ = std::string(ch_sv.data(), ch_sv.size());

    enabled_ = true;
    core_->printLn("[Discord] Component loaded. Prefix='%s'", prefix_.c_str());
}

void DiscordComponent::onInit(IComponentList* /*components*/)
{
    // Nothing to query from other components at init time
}

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

    // Release Lua refs if L_ is still valid
    if (L_)
    {
        if (on_ready_ref_   != LUA_NOREF) luaL_unref(L_, LUA_REGISTRYINDEX, on_ready_ref_);
        if (on_message_ref_ != LUA_NOREF) luaL_unref(L_, LUA_REGISTRYINDEX, on_message_ref_);
        for (auto& [name, ref] : command_refs_)
            luaL_unref(L_, LUA_REGISTRYINDEX, ref);
    }
}

// ---------------------------------------------------------------------------
// Lua API — called from lua_discord.cpp (main thread)
// ---------------------------------------------------------------------------

void DiscordComponent::sendMessage(const std::string& channel_id, const std::string& content)
{
    if (!enabled_) return;
    std::lock_guard<std::mutex> lock(out_mutex_);
    out_queue_.push({ channel_id, content });
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
    if (on_ready_ref_ != LUA_NOREF)
        luaL_unref(L, LUA_REGISTRYINDEX, on_ready_ref_);
    on_ready_ref_ = ref;
}

void DiscordComponent::setOnMessageRef(lua_State* L, int ref)
{
    L_ = L;
    if (on_message_ref_ != LUA_NOREF)
        luaL_unref(L, LUA_REGISTRYINDEX, on_message_ref_);
    on_message_ref_ = ref;
}

void DiscordComponent::registerCommand(const std::string& name, lua_State* L, int ref)
{
    L_ = L;
    auto it = command_refs_.find(name);
    if (it != command_refs_.end())
        luaL_unref(L, LUA_REGISTRYINDEX, it->second);
    command_refs_[name] = ref;
}

// ---------------------------------------------------------------------------
// tick() — called on main thread every server tick (from Lua component)
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

    while (!local.empty())
    {
        DiscordInEvent ev = std::move(local.front());
        local.pop();

        switch (ev.type)
        {
        case DiscordEventType::Ready:
            dispatchReady(L);
            break;
        case DiscordEventType::Message:
            dispatchMessage(L, ev);
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// Dispatch helpers (main thread)
// ---------------------------------------------------------------------------

void DiscordComponent::dispatchReady(lua_State* L)
{
    // Call NX.Hook dispatcher for "OnDiscordReady" if hooked
    lua_getglobal(L, "NX");
    if (lua_istable(L, -1))
    {
        lua_getfield(L, -1, "_dispatch");
        if (lua_isfunction(L, -1))
        {
            lua_pushstring(L, "OnDiscordReady");
            lua_pcall(L, 1, 0, 0);
        }
        else lua_pop(L, 1);
    }
    lua_pop(L, 1);

    // Also call direct onReady callback if registered
    if (on_ready_ref_ != LUA_NOREF)
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, on_ready_ref_);
        if (lua_isfunction(L, -1))
        {
            if (lua_pcall(L, 0, 0, 0) != LUA_OK)
            {
                core_->printLn("[Discord] Error in onReady: %s", lua_tostring(L, -1));
                lua_pop(L, 1);
            }
        }
        else lua_pop(L, 1);
    }
}

void DiscordComponent::dispatchMessage(lua_State* L, const DiscordInEvent& ev)
{
    // 1. NX.Hook dispatcher for "OnDiscordMessage"
    lua_getglobal(L, "NX");
    if (lua_istable(L, -1))
    {
        lua_getfield(L, -1, "_dispatch");
        if (lua_isfunction(L, -1))
        {
            lua_pushstring(L, "OnDiscordMessage");
            lua_pushstring(L, ev.author.c_str());
            lua_pushstring(L, ev.channel.c_str());
            lua_pushstring(L, ev.content.c_str());
            lua_pcall(L, 4, 0, 0);
        }
        else lua_pop(L, 1);
    }
    lua_pop(L, 1);

    // 2. Direct onMessage callback
    if (on_message_ref_ != LUA_NOREF)
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, on_message_ref_);
        if (lua_isfunction(L, -1))
        {
            lua_pushstring(L, ev.author.c_str());
            lua_pushstring(L, ev.content.c_str());
            if (lua_pcall(L, 2, 0, 0) != LUA_OK)
            {
                core_->printLn("[Discord] Error in onMessage: %s", lua_tostring(L, -1));
                lua_pop(L, 1);
            }
        }
        else lua_pop(L, 1);
    }

    // 3. Command dispatch
    if (!prefix_.empty() && ev.content.rfind(prefix_, 0) == 0)
    {
        std::string body = ev.content.substr(prefix_.size());
        // Split into cmd + args
        std::istringstream iss(body);
        std::string cmd;
        iss >> cmd;

        // Lowercase command name
        for (char& c : cmd) c = (char)tolower((unsigned char)c);

        auto it = command_refs_.find(cmd);
        if (it != command_refs_.end())
        {
            lua_rawgeti(L, LUA_REGISTRYINDEX, it->second);
            if (lua_isfunction(L, -1))
            {
                // arg1: author (string)
                lua_pushstring(L, ev.author.c_str());

                // arg2: args (table of strings)
                lua_newtable(L);
                std::string arg;
                int idx = 1;
                while (iss >> arg)
                {
                    lua_pushstring(L, arg.c_str());
                    lua_rawseti(L, -2, idx++);
                }

                if (lua_pcall(L, 2, 0, 0) != LUA_OK)
                {
                    core_->printLn("[Discord] Error in command '%s': %s",
                        cmd.c_str(), lua_tostring(L, -1));
                    lua_pop(L, 1);
                }
            }
            else lua_pop(L, 1);
        }
    }
}

// ---------------------------------------------------------------------------
// Worker thread
// ---------------------------------------------------------------------------

void DiscordComponent::workerLoop()
{
    // Fetch the last message ID so we don't replay old messages on startup
    fetchInitialLastId();

    // Signal ready to main thread
    {
        std::lock_guard<std::mutex> lock(in_mutex_);
        in_queue_.push({ DiscordEventType::Ready, "", "", "" });
    }

    while (running_)
    {
        flushOutbound();
        updateStatus();
        pollMessages();

        // Poll every 1 second
        for (int i = 0; i < 10 && running_; ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// ---------------------------------------------------------------------------
// HTTP helpers (worker thread)
// ---------------------------------------------------------------------------

static const char* DISCORD_HOST = "discord.com";
static const int   DISCORD_PORT = 443;

bool DiscordComponent::httpGet(const std::string& path, std::string& out_body)
{
    httplib::SSLClient cli(DISCORD_HOST, DISCORD_PORT);
    cli.set_connection_timeout(10);
    cli.set_read_timeout(10);
    cli.enable_server_certificate_verification(false); // simplify for embedded use

    httplib::Headers headers = {
        { "Authorization", "Bot " + token_ },
        { "User-Agent",    "NexorixBot/1.0" },
    };

    auto res = cli.Get(path, headers);
    if (!res || res->status < 200 || res->status >= 300)
        return false;

    out_body = res->body;
    return true;
}

bool DiscordComponent::httpPost(const std::string& path, const std::string& json_body, std::string& out_body)
{
    httplib::SSLClient cli(DISCORD_HOST, DISCORD_PORT);
    cli.set_connection_timeout(10);
    cli.set_read_timeout(10);
    cli.enable_server_certificate_verification(false);

    httplib::Headers headers = {
        { "Authorization", "Bot " + token_ },
        { "User-Agent",    "NexorixBot/1.0" },
    };

    auto res = cli.Post(path, headers, json_body, "application/json");
    if (!res || res->status < 200 || res->status >= 300)
        return false;

    out_body = res->body;
    return true;
}

bool DiscordComponent::httpPatch(const std::string& path, const std::string& json_body)
{
    httplib::SSLClient cli(DISCORD_HOST, DISCORD_PORT);
    cli.set_connection_timeout(10);
    cli.set_read_timeout(10);
    cli.enable_server_certificate_verification(false);

    httplib::Headers headers = {
        { "Authorization", "Bot " + token_ },
        { "User-Agent",    "NexorixBot/1.0" },
    };

    auto res = cli.Patch(path, headers, json_body, "application/json");
    return res && res->status >= 200 && res->status < 300;
}

// ---------------------------------------------------------------------------
// Discord operations (worker thread)
// ---------------------------------------------------------------------------

void DiscordComponent::fetchInitialLastId()
{
    if (log_channel_id_.empty()) return;

    std::string path = "/api/v10/channels/" + log_channel_id_ + "/messages?limit=1";
    std::string body;
    if (!httpGet(path, body)) return;

    try
    {
        auto arr = json::parse(body);
        if (arr.is_array() && !arr.empty())
            last_message_id_ = arr[0]["id"].get<std::string>();
    }
    catch (...) {}
}

void DiscordComponent::pollMessages()
{
    if (log_channel_id_.empty()) return;

    std::string path = "/api/v10/channels/" + log_channel_id_ + "/messages?limit=10";
    if (!last_message_id_.empty())
        path += "&after=" + last_message_id_;

    std::string body;
    if (!httpGet(path, body)) return;

    try
    {
        auto arr = json::parse(body);
        if (!arr.is_array() || arr.empty()) return;

        // Discord returns newest-first; reverse to process oldest-first
        std::vector<json> msgs(arr.begin(), arr.end());
        std::reverse(msgs.begin(), msgs.end());

        for (auto& msg : msgs)
        {
            // Skip bot messages
            if (msg.value("author", json::object()).value("bot", false))
                continue;

            std::string id      = msg["id"].get<std::string>();
            std::string content = msg.value("content", "");
            std::string channel = msg.value("channel_id", log_channel_id_);

            // Build author string
            auto& author = msg["author"];
            std::string username = author.value("username", "unknown");
            std::string discrim  = author.value("discriminator", "0000");
            std::string author_str = (discrim == "0") ? username : username + "#" + discrim;

            // Update last seen ID
            last_message_id_ = id;

            // Enqueue inbound event
            std::lock_guard<std::mutex> lock(in_mutex_);
            in_queue_.push({ DiscordEventType::Message, author_str, channel, content });
        }
    }
    catch (...) {}
}

void DiscordComponent::flushOutbound()
{
    std::queue<DiscordOutMsg> local;
    {
        std::lock_guard<std::mutex> lock(out_mutex_);
        std::swap(local, out_queue_);
    }

    while (!local.empty())
    {
        DiscordOutMsg msg = std::move(local.front());
        local.pop();

        // Truncate to Discord's 2000 char limit
        if (msg.content.size() > 2000)
            msg.content = msg.content.substr(0, 1997) + "...";

        json payload = { { "content", msg.content } };
        std::string path = "/api/v10/channels/" + msg.channel_id + "/messages";
        std::string resp;
        httpPost(path, payload.dump(), resp);
    }
}

void DiscordComponent::updateStatus()
{
    std::string status_text;
    {
        std::lock_guard<std::mutex> lock(status_mutex_);
        if (!status_dirty_) return;
        status_text   = pending_status_;
        status_dirty_ = false;
    }

    // PATCH /api/v10/gateway/bot doesn't set presence directly.
    // Use the Update Presence endpoint via the gateway — but since we're
    // REST-only, we update the bot's "about me" via PATCH /users/@me
    // (only works for OAuth2 bots with the correct scope).
    // For simplicity, we log the status change server-side.
    if (core_)
        core_->printLn("[Discord] Status set to: %s", status_text.c_str());
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

COMPONENT_ENTRY_POINT()
{
    return new DiscordComponent();
}
