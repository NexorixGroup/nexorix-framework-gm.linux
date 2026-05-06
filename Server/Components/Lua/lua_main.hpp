#pragma once

#include <sdk.hpp>
#include <Server/Components/Vehicles/vehicles.hpp>
#include <Server/Components/Dialogs/dialogs.hpp>
#include <Server/Components/Pickups/pickups.hpp>
#include <Server/Components/Objects/objects.hpp>
#include <Server/Components/TextLabels/textlabels.hpp>
#include <Server/Components/TextDraws/textdraws.hpp>
#include <Server/Components/Classes/classes.hpp>
#include <Server/Components/Checkpoints/checkpoints.hpp>
#include <Server/Components/GangZones/gangzones.hpp>
#include <Server/Components/Menus/menus.hpp>
#include <ghc/filesystem.hpp>

// Discord — compiled into the Lua component (same DLL)
#include "Discord/discord_main.hpp"
#include "Discord/lua_discord.hpp"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include "lua_scripts.hpp"

/// The Lua component — loads and runs Lua scripts as a game mode
class LuaComponent final
    : public IComponent
    , public CoreEventHandler
    , public PlayerConnectEventHandler
    , public PlayerSpawnEventHandler
    , public PlayerTextEventHandler
    , public PlayerDamageEventHandler
    , public PlayerUpdateEventHandler
    , public PlayerDialogEventHandler
{
public:
    // ---- IComponent ----
    StringView componentName() const override { return "Lua"; }
    SemanticVersion componentVersion() const override;
    UID getUID() override { return UID(0x4C75614E65786F72ULL); }

    void onLoad(ICore* core) override;
    void onInit(IComponentList* components) override;
    void onReady() override {}
    void onFree(IComponent* component) override {}
    void free() override { delete this; }
    void reset() override {}

    // ---- CoreEventHandler ----
    void onTick(Microseconds elapsed, TimePoint now) override;

    // ---- PlayerConnectEventHandler ----
    void onPlayerConnect(IPlayer& player) override;
    void onPlayerDisconnect(IPlayer& player, PeerDisconnectReason reason) override;

    // ---- PlayerSpawnEventHandler ----
    void onPlayerSpawn(IPlayer& player) override;

    // ---- PlayerTextEventHandler ----
    bool onPlayerText(IPlayer& player, StringView message) override;
    bool onPlayerCommandText(IPlayer& player, StringView message) override;

    // ---- PlayerDamageEventHandler ----
    void onPlayerDeath(IPlayer& player, IPlayer* killer, int reason) override;

    // ---- PlayerUpdateEventHandler ----
    bool onPlayerUpdate(IPlayer& player, TimePoint now) override;

    // ---- PlayerDialogEventHandler ----
    void onDialogResponse(IPlayer& player, int dialogId, DialogResponse response, int listItem, StringView inputText) override;

    // Accessors used by lua_api
    ICore*                getCore()        { return core_; }
    IVehiclesComponent*   getVehicles()    { return vehicles_; }
    IDialogsComponent*    getDialogs()     { return dialogs_; }
    IPickupsComponent*    getPickups()     { return pickups_; }
    IObjectsComponent*    getObjects()     { return objects_; }
    ITextLabelsComponent* getTextLabels()  { return textLabels_; }
    ITextDrawsComponent*  getTextDraws()   { return textDraws_; }
    IClassesComponent*    getClasses()     { return classes_; }
    ICheckpointsComponent* getCheckpoints(){ return checkpoints_; }
    IGangZonesComponent*  getGangZones()   { return gangZones_; }
    IMenusComponent*      getMenus()       { return menus_; }
    lua_State* getLuaState() { return L_; }

    // Discord — embedded in the Lua DLL
    DiscordComponent* getDiscord() { return &discord_; }

    ~LuaComponent();

private:
    ICore*                 core_         = nullptr;
    IVehiclesComponent*    vehicles_     = nullptr;
    IDialogsComponent*     dialogs_      = nullptr;
    IPickupsComponent*     pickups_      = nullptr;
    IObjectsComponent*     objects_      = nullptr;
    ITextLabelsComponent*  textLabels_   = nullptr;
    ITextDrawsComponent*   textDraws_    = nullptr;
    IClassesComponent*     classes_      = nullptr;
    ICheckpointsComponent* checkpoints_  = nullptr;
    IGangZonesComponent*   gangZones_    = nullptr;
    IMenusComponent*       menus_        = nullptr;
    DiscordComponent       discord_;      // embedded, not a separate DLL
    lua_State* L_ = nullptr;
    LuaScriptManager scripts_;

    void initLuaState();
    void registerAPI();
    void callGameModeInit();
    void callGameModeExit();
};
