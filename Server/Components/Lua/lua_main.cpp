/*
 * Nexorix Lua Component
 * Provides Lua 5.4 scripting support for the Nexorix open.mp server.
 */

#include "lua_main.hpp"
#include "lua_api.hpp"
#include "lua_db.hpp"
#include <sdk.hpp>
// Forward declaration from lua_api.cpp
void lua_tick_timers(lua_State* L, int elapsedMs);

// ---- IComponent lifecycle ----

SemanticVersion LuaComponent::componentVersion() const
{
    return SemanticVersion(OMP_VERSION_MAJOR, OMP_VERSION_MINOR, OMP_VERSION_PATCH, BUILD_NUMBER);
}

void LuaComponent::onLoad(ICore* core)
{
    core_ = core;

    // Check scripting_mode in config — skip if set to "pawn"
    IConfig& cfg = core_->getConfig();
    StringView mode = cfg.getString("scripting_mode");
    if (mode == StringView("pawn"))
    {
        core_->printLn("[Lua] scripting_mode=pawn: Lua component disabled.");
        return;
    }
    if (mode == StringView("both"))
    {
        core_->printLn("[Lua] scripting_mode=both: Running Lua alongside Pawn. Pawn has priority on shared callbacks.");
    }

    // Register event handlers
    core_->getEventDispatcher().addEventHandler(this);
    core_->getPlayers().getPlayerConnectDispatcher().addEventHandler(this);
    core_->getPlayers().getPlayerSpawnDispatcher().addEventHandler(this);
    core_->getPlayers().getPlayerTextDispatcher().addEventHandler(this);
    core_->getPlayers().getPlayerDamageDispatcher().addEventHandler(this);
    core_->getPlayers().getPlayerUpdateDispatcher().addEventHandler(this);

    // Initialise Lua VM
    initLuaState();

    core_->printLn("[Lua] Component loaded (Lua 5.4)");
}

void LuaComponent::onInit(IComponentList* components)
{
    // Grab optional vehicle component
    vehicles_ = components->queryComponent<IVehiclesComponent>();

    // Grab dialogs component
    dialogs_ = components->queryComponent<IDialogsComponent>();
    if (dialogs_)
        dialogs_->getEventDispatcher().addEventHandler(this);

    // Grab additional components
    pickups_     = components->queryComponent<IPickupsComponent>();
    objects_     = components->queryComponent<IObjectsComponent>();
    textLabels_  = components->queryComponent<ITextLabelsComponent>();
    textDraws_   = components->queryComponent<ITextDrawsComponent>();
    classes_     = components->queryComponent<IClassesComponent>();
    checkpoints_ = components->queryComponent<ICheckpointsComponent>();
    gangZones_   = components->queryComponent<IGangZonesComponent>();
    menus_       = components->queryComponent<IMenusComponent>();

    // Init embedded Discord component
    discord_.onLoad(core_);
    discord_.onInit(components);

    // Re-register Discord Lua API now that channel_id is loaded
    // (first registration in registerAPI() had empty channel_id)
    lua_register_discord(L_, &discord_);

    // Determine script directory relative to the executable
    ghc::filesystem::path scriptDir = ghc::filesystem::absolute("lua");
    if (!ghc::filesystem::exists(scriptDir))
    {
        ghc::filesystem::create_directory(scriptDir);
    }

    // Load all scripts
    scripts_.loadAll(L_, scriptDir.string());

    // Fire OnGameModeInit
    callGameModeInit();

    // Start Discord worker after scripts are loaded
    discord_.onReady();
}

LuaComponent::~LuaComponent()
{
    callGameModeExit();

    if (core_)
    {
        core_->getEventDispatcher().removeEventHandler(this);
        core_->getPlayers().getPlayerConnectDispatcher().removeEventHandler(this);
        core_->getPlayers().getPlayerSpawnDispatcher().removeEventHandler(this);
        core_->getPlayers().getPlayerTextDispatcher().removeEventHandler(this);
        core_->getPlayers().getPlayerDamageDispatcher().removeEventHandler(this);
        core_->getPlayers().getPlayerUpdateDispatcher().removeEventHandler(this);
    }

    if (dialogs_)
        dialogs_->getEventDispatcher().removeEventHandler(this);

    if (L_)
    {
        lua_close(L_);
        L_ = nullptr;
    }
}

// ---- Private helpers ----

void LuaComponent::initLuaState()
{
    L_ = luaL_newstate();
    if (!L_)
    {
        core_->printLn("[Lua] FATAL: failed to create Lua state");
        return;
    }

    // Open standard libraries (math, string, table, etc.)
    luaL_openlibs(L_);

    // Register all nx_* API functions
    registerAPI();
}

void LuaComponent::registerAPI()
{
    lua_register_api(L_, this);
    lua_register_db(L_);
    // Discord API registered later in onInit() after discord_.onLoad() reads the config
}

void LuaComponent::callGameModeInit()
{
    lua_getglobal(L_, "OnGameModeInit");
    if (lua_isfunction(L_, -1))
    {
        if (lua_pcall(L_, 0, 0, 0) != LUA_OK)
        {
            core_->printLn("[Lua] Error in OnGameModeInit: %s", lua_tostring(L_, -1));
            lua_pop(L_, 1);
        }
    }
    else
    {
        lua_pop(L_, 1);
    }
}

void LuaComponent::callGameModeExit()
{
    if (!L_) return;
    lua_getglobal(L_, "OnGameModeExit");
    if (lua_isfunction(L_, -1))
    {
        if (lua_pcall(L_, 0, 0, 0) != LUA_OK)
        {
            if (core_)
                core_->printLn("[Lua] Error in OnGameModeExit: %s", lua_tostring(L_, -1));
            lua_pop(L_, 1);
        }
    }
    else
    {
        lua_pop(L_, 1);
    }
}

// ---- CoreEventHandler ----

void LuaComponent::onTick(Microseconds elapsed, TimePoint /*now*/)
{
    // Advance Lua-side timers
    int elapsedMs = static_cast<int>(duration_cast<Milliseconds>(elapsed).count());
    lua_tick_timers(L_, elapsedMs);

    // Flush Discord inbound event queue into Lua (thread-safe)
    discord_.tick(L_);

    lua_getglobal(L_, "OnTick");
    if (!lua_isfunction(L_, -1))
    {
        lua_pop(L_, 1);
        return;
    }
    // Pass elapsed microseconds as integer
    lua_pushinteger(L_, static_cast<lua_Integer>(elapsed.count()));
    if (lua_pcall(L_, 1, 0, 0) != LUA_OK)
    {
        core_->printLn("[Lua] Error in OnTick: %s", lua_tostring(L_, -1));
        lua_pop(L_, 1);
    }
}

// ---- PlayerConnectEventHandler ----

void LuaComponent::onPlayerConnect(IPlayer& player)
{
    lua_getglobal(L_, "OnPlayerConnect");
    if (!lua_isfunction(L_, -1)) { lua_pop(L_, 1); return; }
    lua_pushinteger(L_, player.getID());
    if (lua_pcall(L_, 1, 0, 0) != LUA_OK)
    {
        core_->printLn("[Lua] Error in OnPlayerConnect: %s", lua_tostring(L_, -1));
        lua_pop(L_, 1);
    }
}

void LuaComponent::onPlayerDisconnect(IPlayer& player, PeerDisconnectReason reason)
{
    lua_getglobal(L_, "OnPlayerDisconnect");
    if (!lua_isfunction(L_, -1)) { lua_pop(L_, 1); return; }
    lua_pushinteger(L_, player.getID());
    lua_pushinteger(L_, static_cast<int>(reason));
    if (lua_pcall(L_, 2, 0, 0) != LUA_OK)
    {
        core_->printLn("[Lua] Error in OnPlayerDisconnect: %s", lua_tostring(L_, -1));
        lua_pop(L_, 1);
    }
}

// ---- PlayerSpawnEventHandler ----

void LuaComponent::onPlayerSpawn(IPlayer& player)
{
    lua_getglobal(L_, "OnPlayerSpawn");
    if (!lua_isfunction(L_, -1)) { lua_pop(L_, 1); return; }
    lua_pushinteger(L_, player.getID());
    if (lua_pcall(L_, 1, 0, 0) != LUA_OK)
    {
        core_->printLn("[Lua] Error in OnPlayerSpawn: %s", lua_tostring(L_, -1));
        lua_pop(L_, 1);
    }
}

// ---- PlayerTextEventHandler ----

bool LuaComponent::onPlayerText(IPlayer& player, StringView message)
{
    lua_getglobal(L_, "OnPlayerText");
    if (!lua_isfunction(L_, -1)) { lua_pop(L_, 1); return true; }
    lua_pushinteger(L_, player.getID());
    lua_pushlstring(L_, message.data(), message.size());
    if (lua_pcall(L_, 2, 1, 0) != LUA_OK)
    {
        core_->printLn("[Lua] Error in OnPlayerText: %s", lua_tostring(L_, -1));
        lua_pop(L_, 1);
        return true;
    }
    bool result = lua_toboolean(L_, -1) != 0;
    lua_pop(L_, 1);
    return result;
}

bool LuaComponent::onPlayerCommandText(IPlayer& player, StringView message)
{
    lua_getglobal(L_, "OnPlayerCommand");
    if (!lua_isfunction(L_, -1)) { lua_pop(L_, 1); return false; }
    lua_pushinteger(L_, player.getID());
    lua_pushlstring(L_, message.data(), message.size());
    if (lua_pcall(L_, 2, 1, 0) != LUA_OK)
    {
        core_->printLn("[Lua] Error in OnPlayerCommand: %s", lua_tostring(L_, -1));
        lua_pop(L_, 1);
        return false;
    }
    bool result = lua_toboolean(L_, -1) != 0;
    lua_pop(L_, 1);
    return result;
}

// ---- PlayerDamageEventHandler ----

void LuaComponent::onPlayerDeath(IPlayer& player, IPlayer* killer, int reason)
{
    lua_getglobal(L_, "OnPlayerDeath");
    if (!lua_isfunction(L_, -1)) { lua_pop(L_, 1); return; }
    lua_pushinteger(L_, player.getID());
    lua_pushinteger(L_, killer ? killer->getID() : 0xFFFF);
    lua_pushinteger(L_, reason);
    if (lua_pcall(L_, 3, 0, 0) != LUA_OK)
    {
        core_->printLn("[Lua] Error in OnPlayerDeath: %s", lua_tostring(L_, -1));
        lua_pop(L_, 1);
    }
}

// ---- PlayerUpdateEventHandler ----

bool LuaComponent::onPlayerUpdate(IPlayer& player, TimePoint /*now*/)
{
    lua_getglobal(L_, "OnPlayerUpdate");
    if (!lua_isfunction(L_, -1)) { lua_pop(L_, 1); return true; }
    lua_pushinteger(L_, player.getID());
    if (lua_pcall(L_, 1, 1, 0) != LUA_OK)
    {
        core_->printLn("[Lua] Error in OnPlayerUpdate: %s", lua_tostring(L_, -1));
        lua_pop(L_, 1);
        return true;
    }
    bool result = lua_toboolean(L_, -1) != 0;
    lua_pop(L_, 1);
    return result;
}

// ---- PlayerDialogEventHandler ----

void LuaComponent::onDialogResponse(IPlayer& player, int dialogId, DialogResponse response, int listItem, StringView inputText)
{
    lua_getglobal(L_, "OnDialogResponse");
    if (!lua_isfunction(L_, -1)) { lua_pop(L_, 1); return; }
    lua_pushinteger(L_, player.getID());
    lua_pushinteger(L_, dialogId);
    lua_pushinteger(L_, static_cast<int>(response));
    lua_pushinteger(L_, listItem);
    lua_pushlstring(L_, inputText.data(), inputText.size());
    if (lua_pcall(L_, 5, 0, 0) != LUA_OK)
    {
        core_->printLn("[Lua] Error in OnDialogResponse: %s", lua_tostring(L_, -1));
        lua_pop(L_, 1);
    }
}

// ---- Entry point ----

COMPONENT_ENTRY_POINT()
{
    return new LuaComponent();
}
