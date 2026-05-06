#pragma once

#include <sdk.hpp>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <functional>
#include <unordered_map>
#include <vector>

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

// ---------------------------------------------------------------------------
// Outbound message queued from Lua -> sent by worker thread
// ---------------------------------------------------------------------------
struct DiscordOutMsg {
    std::string channel_id;
    std::string content;
};

// ---------------------------------------------------------------------------
// Inbound event queued by worker thread -> dispatched to Lua on main thread
// ---------------------------------------------------------------------------
enum class DiscordEventType {
    Ready,
    Message,
};

struct DiscordInEvent {
    DiscordEventType type;
    std::string author;
    std::string channel;
    std::string content;
};

// ---------------------------------------------------------------------------
// DiscordComponent UID — used by queryComponent<> from Lua component
// ---------------------------------------------------------------------------
static const UID DiscordComponent_UID = UID(0x4E784469736300ULL);

// ---------------------------------------------------------------------------
// DiscordComponent
// ---------------------------------------------------------------------------
class DiscordComponent final : public IComponent
{
public:
    // IComponent
    PROVIDE_UID(DiscordComponent_UID)
    StringView componentName() const override { return "Discord"; }

    void onLoad(ICore* core) override;
    void onInit(IComponentList* components) override;
    void onReady() override;
    void onFree(IComponent* component) override {}
    void free() override { delete this; }
    void reset() override {}

    ~DiscordComponent();

    // Called by Lua component on every tick to flush inbound event queue
    void tick(lua_State* L);

    // Lua API surface
    void sendMessage(const std::string& channel_id, const std::string& content);
    void setStatus(const std::string& text);
    const std::string& getChannelId() const { return log_channel_id_; }

    // Lua callback registration
    void setOnReadyRef(lua_State* L, int ref);
    void setOnMessageRef(lua_State* L, int ref);
    void registerCommand(const std::string& name, lua_State* L, int ref);

private:
    ICore* core_ = nullptr;
    lua_State* L_ = nullptr;

    std::string token_;
    std::string prefix_;
    std::string log_channel_id_;
    bool        enabled_ = false;

    std::thread       worker_;
    std::atomic<bool> running_{ false };

    std::mutex              out_mutex_;
    std::queue<DiscordOutMsg> out_queue_;

    std::mutex               in_mutex_;
    std::queue<DiscordInEvent> in_queue_;

    std::mutex      status_mutex_;
    std::string     pending_status_;
    bool            status_dirty_ = false;

    int on_ready_ref_   = LUA_NOREF;
    int on_message_ref_ = LUA_NOREF;
    std::unordered_map<std::string, int> command_refs_;

    std::string last_message_id_;

    void workerLoop();
    bool httpGet(const std::string& path, std::string& out_body);
    bool httpPost(const std::string& path, const std::string& json_body, std::string& out_body);
    bool httpPatch(const std::string& path, const std::string& json_body);
    void pollMessages();
    void flushOutbound();
    void updateStatus();
    void fetchInitialLastId();
    void dispatchReady(lua_State* L);
    void dispatchMessage(lua_State* L, const DiscordInEvent& ev);
};
