/*
 * Nexorix Lua Component - API Registration
 * All nx_* functions exposed to Lua scripts.
 */

#include "lua_api.hpp"
#include "lua_main.hpp"
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
#include <cmath>
#include <string>
#include <ghc/filesystem.hpp>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static LuaComponent* getComponent(lua_State* L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, "__nx_component");
    LuaComponent* comp = static_cast<LuaComponent*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return comp;
}

static IPlayer* getPlayer(ICore* core, int id)
{
    return core->getPlayers().get(id);
}

static Colour colourFromInt(lua_Integer v)
{
    uint32_t u = static_cast<uint32_t>(v);
    return Colour((u >> 24) & 0xFF, (u >> 16) & 0xFF, (u >> 8) & 0xFF, u & 0xFF);
}

static lua_Integer colourToInt(Colour c)
{
    return (lua_Integer)(((uint32_t)c.r << 24) | ((uint32_t)c.g << 16) | ((uint32_t)c.b << 8) | c.a);
}

// ===========================================================================
// PLAYER — Basic
// ===========================================================================

static int nx_SendClientMessage(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    lua_Integer col = luaL_checkinteger(L, 2);
    const char* msg = luaL_checkstring(L, 3);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->sendClientMessage(colourFromInt(col), StringView(msg));
    return 0;
}

static int nx_SendClientMessageToAll(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    lua_Integer col = luaL_checkinteger(L, 1);
    const char* msg = luaL_checkstring(L, 2);
    comp->getCore()->getPlayers().sendClientMessageToAll(colourFromInt(col), StringView(msg));
    return 0;
}

static int nx_GetPlayerName(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) { StringView n = p->getName(); lua_pushlstring(L, n.data(), n.size()); }
    else lua_pushstring(L, "");
    return 1;
}

static int nx_SetPlayerPos(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);
    float z = (float)luaL_checknumber(L, 4);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->setPosition(Vector3(x, y, z));
    return 0;
}

static int nx_GetPlayerPos(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) { Vector3 v = p->getPosition(); lua_pushnumber(L,v.x); lua_pushnumber(L,v.y); lua_pushnumber(L,v.z); return 3; }
    lua_pushnumber(L,0); lua_pushnumber(L,0); lua_pushnumber(L,0); return 3;
}

static int nx_SetPlayerFacingAngle(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    float angle = (float)luaL_checknumber(L, 2);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->setRotation(GTAQuat(Vector3(0.f, 0.f, angle)));
    return 0;
}

static int nx_GetPlayerFacingAngle(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) { Vector3 rot = p->getRotation().ToEuler(); lua_pushnumber(L, rot.z); return 1; }
    lua_pushnumber(L, 0); return 1;
}

static int nx_SetPlayerHealth(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    float hp = (float)luaL_checknumber(L, 2);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->setHealth(hp);
    return 0;
}

static int nx_GetPlayerHealth(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    lua_pushnumber(L, p ? (double)p->getHealth() : 0.0);
    return 1;
}

static int nx_SetPlayerArmour(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    float arm = (float)luaL_checknumber(L, 2);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->setArmour(arm);
    return 0;
}

static int nx_GetPlayerArmour(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    lua_pushnumber(L, p ? (double)p->getArmour() : 0.0);
    return 1;
}

static int nx_GivePlayerMoney(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    int amount = (int)luaL_checkinteger(L, 2);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->giveMoney(amount);
    return 0;
}

static int nx_GetPlayerMoney(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    lua_pushinteger(L, p ? p->getMoney() : 0);
    return 1;
}

static int nx_ResetPlayerMoney(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->resetMoney();
    return 0;
}

static int nx_SetPlayerScore(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    int score = (int)luaL_checkinteger(L, 2);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->setScore(score);
    return 0;
}

static int nx_GetPlayerScore(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    lua_pushinteger(L, p ? p->getScore() : 0);
    return 1;
}

static int nx_SetPlayerSkin(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    int skin = (int)luaL_checkinteger(L, 2);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->setSkin(skin);
    return 0;
}

static int nx_GetPlayerSkin(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    lua_pushinteger(L, p ? p->getSkin() : 0);
    return 1;
}

static int nx_SetPlayerColor(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    lua_Integer col = luaL_checkinteger(L, 2);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->setColour(colourFromInt(col));
    return 0;
}

static int nx_GetPlayerColor(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    lua_pushinteger(L, p ? colourToInt(p->getColour()) : 0);
    return 1;
}

static int nx_SetPlayerTeam(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    int team = (int)luaL_checkinteger(L, 2);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->setTeam(team);
    return 0;
}

static int nx_GetPlayerTeam(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    lua_pushinteger(L, p ? p->getTeam() : 255);
    return 1;
}

static int nx_GetPlayerState(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    lua_pushinteger(L, p ? (int)p->getState() : 0);
    return 1;
}

static int nx_SetPlayerVirtualWorld(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    int vw = (int)luaL_checkinteger(L, 2);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->setVirtualWorld(vw);
    return 0;
}

static int nx_GetPlayerVirtualWorld(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    lua_pushinteger(L, p ? p->getVirtualWorld() : 0);
    return 1;
}

static int nx_SetPlayerInterior(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    int interior = (int)luaL_checkinteger(L, 2);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->setInterior((unsigned)interior);
    return 0;
}

static int nx_GetPlayerInterior(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    lua_pushinteger(L, p ? (int)p->getInterior() : 0);
    return 1;
}

static int nx_SpawnPlayer(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->spawn();
    return 0;
}

static int nx_KickPlayer(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->kick();
    return 0;
}

static int nx_BanPlayer(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->ban();
    return 0;
}

static int nx_IsPlayerConnected(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, getPlayer(comp->getCore(), pid) != nullptr ? 1 : 0);
    return 1;
}

static int nx_GetPlayerPing(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    lua_pushinteger(L, p ? (lua_Integer)p->getPing() : 0);
    return 1;
}

static int nx_GetPlayerIP(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) {
        const PeerNetworkData& nd = p->getNetworkData();
        const PeerAddress& addr = nd.networkID.address;
        char buf[64] = {};
        if (!addr.ipv6) {
            uint32_t ip = addr.v4;
            snprintf(buf, sizeof(buf), "%u.%u.%u.%u",
                (ip>>24)&0xFF,(ip>>16)&0xFF,(ip>>8)&0xFF,ip&0xFF);
        } else {
            snprintf(buf, sizeof(buf), "%x:%x:%x:%x:%x:%x:%x:%x",
                addr.v6.segments[0],addr.v6.segments[1],addr.v6.segments[2],addr.v6.segments[3],
                addr.v6.segments[4],addr.v6.segments[5],addr.v6.segments[6],addr.v6.segments[7]);
        }
        lua_pushstring(L, buf);
    } else lua_pushstring(L, "0.0.0.0");
    return 1;
}

// ===========================================================================
// PLAYER — Weapons, Animation, Camera, Misc
// ===========================================================================

static int nx_GivePlayerWeapon(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    int weapon = (int)luaL_checkinteger(L, 2);
    int ammo = (int)luaL_checkinteger(L, 3);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->giveWeapon(WeaponSlotData((uint8_t)weapon, (uint32_t)ammo));
    return 0;
}

static int nx_ResetPlayerWeapons(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->resetWeapons();
    return 0;
}

static int nx_GetPlayerWeapon(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    lua_pushinteger(L, p ? (lua_Integer)p->getArmedWeapon() : 0);
    return 1;
}

static int nx_GetPlayerAmmo(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    lua_pushinteger(L, p ? (lua_Integer)p->getArmedWeaponAmmo() : 0);
    return 1;
}

static int nx_SetPlayerArmedWeapon(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    int weapon = (int)luaL_checkinteger(L, 2);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->setArmedWeapon((uint32_t)weapon);
    return 0;
}

// nx_ApplyAnimation(playerid, animlib, animname, delta, loop, lockx, locky, freeze, time, sync)
static int nx_ApplyAnimation(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    const char* lib  = luaL_checkstring(L, 2);
    const char* name = luaL_checkstring(L, 3);
    float delta  = (float)luaL_optnumber(L, 4, 4.1);
    bool loop    = lua_toboolean(L, 5) != 0;
    bool lockX   = lua_toboolean(L, 6) != 0;
    bool lockY   = lua_toboolean(L, 7) != 0;
    bool freeze  = lua_toboolean(L, 8) != 0;
    int time     = (int)luaL_optinteger(L, 9, 0);
    int sync     = (int)luaL_optinteger(L, 10, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) {
        AnimationData anim;
        anim.lib   = StringView(lib);
        anim.name  = StringView(name);
        anim.delta = delta;
        anim.loop  = loop;
        anim.lockX = lockX;
        anim.lockY = lockY;
        anim.freeze = freeze;
        anim.time  = (uint32_t)time;
        p->applyAnimation(anim, (PlayerAnimationSyncType)sync);
    }
    return 0;
}

static int nx_ClearAnimations(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    int sync = (int)luaL_optinteger(L, 2, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->clearAnimations((PlayerAnimationSyncType)sync);
    return 0;
}

static int nx_SetPlayerSpecialAction(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    int action = (int)luaL_checkinteger(L, 2);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->setAction((PlayerSpecialAction)action);
    return 0;
}

static int nx_GetPlayerSpecialAction(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    lua_pushinteger(L, p ? (int)p->getAction() : 0);
    return 1;
}

static int nx_SetPlayerFightingStyle(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    int style = (int)luaL_checkinteger(L, 2);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->setFightingStyle((PlayerFightingStyle)style);
    return 0;
}

static int nx_SetPlayerVelocity(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);
    float z = (float)luaL_checknumber(L, 4);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->setVelocity(Vector3(x, y, z));
    return 0;
}

static int nx_GetPlayerVelocity(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) { Vector3 v = p->getVelocity(); lua_pushnumber(L,v.x); lua_pushnumber(L,v.y); lua_pushnumber(L,v.z); return 3; }
    lua_pushnumber(L,0); lua_pushnumber(L,0); lua_pushnumber(L,0); return 3;
}

static int nx_SetPlayerGravity(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    float g = (float)luaL_checknumber(L, 2);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->setGravity(g);
    return 0;
}

static int nx_SetPlayerWeather(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    int weather = (int)luaL_checkinteger(L, 2);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->setWeather(weather);
    return 0;
}

// nx_SetPlayerTime(playerid, hour, minute)
static int nx_SetPlayerTime(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    int hour = (int)luaL_checkinteger(L, 2);
    int min  = (int)luaL_checkinteger(L, 3);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->setTime(Hours(hour), Minutes(min));
    return 0;
}

static int nx_TogglePlayerClock(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    bool enable = lua_toboolean(L, 2) != 0;
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->useClock(enable);
    return 0;
}

static int nx_SetPlayerWantedLevel(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    int level = (int)luaL_checkinteger(L, 2);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->setWantedLevel((unsigned)level);
    return 0;
}

static int nx_GetPlayerWantedLevel(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    lua_pushinteger(L, p ? (int)p->getWantedLevel() : 0);
    return 1;
}

// nx_SetPlayerChatBubble(playerid, text, color, drawdist, expiretime_ms)
static int nx_SetPlayerChatBubble(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    const char* text = luaL_checkstring(L, 2);
    lua_Integer col  = luaL_checkinteger(L, 3);
    float dist       = (float)luaL_optnumber(L, 4, 100.0);
    int expire       = (int)luaL_optinteger(L, 5, 10000);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->setChatBubble(StringView(text), colourFromInt(col), dist, Milliseconds(expire));
    return 0;
}

// nx_GameTextForPlayer(playerid, text, time_ms, style)
static int nx_GameTextForPlayer(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    const char* text = luaL_checkstring(L, 2);
    int time  = (int)luaL_checkinteger(L, 3);
    int style = (int)luaL_checkinteger(L, 4);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->sendGameText(StringView(text), Milliseconds(time), style);
    return 0;
}

// nx_SetPlayerMapIcon(playerid, iconid, x, y, z, type, color, style)
static int nx_SetPlayerMapIcon(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid    = (int)luaL_checkinteger(L, 1);
    int iconid = (int)luaL_checkinteger(L, 2);
    float x    = (float)luaL_checknumber(L, 3);
    float y    = (float)luaL_checknumber(L, 4);
    float z    = (float)luaL_checknumber(L, 5);
    int type   = (int)luaL_checkinteger(L, 6);
    lua_Integer col = luaL_checkinteger(L, 7);
    int style  = (int)luaL_optinteger(L, 8, 0);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->setMapIcon(iconid, Vector3(x,y,z), type, colourFromInt(col), (MapIconStyle)style);
    return 0;
}

static int nx_RemovePlayerMapIcon(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid    = (int)luaL_checkinteger(L, 1);
    int iconid = (int)luaL_checkinteger(L, 2);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->unsetMapIcon(iconid);
    return 0;
}

static int nx_TogglePlayerControllable(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    bool enable = lua_toboolean(L, 2) != 0;
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->setControllable(enable);
    return 0;
}

static int nx_SetPlayerCameraPos(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);
    float z = (float)luaL_checknumber(L, 4);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->setCameraPosition(Vector3(x,y,z));
    return 0;
}

static int nx_SetCameraBehindPlayer(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->setCameraBehind();
    return 0;
}

static int nx_SetPlayerCameraLookAt(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);
    float z = (float)luaL_checknumber(L, 4);
    int cut = (int)luaL_optinteger(L, 5, 2);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->setCameraLookAt(Vector3(x,y,z), cut);
    return 0;
}

static int nx_PlaySoundForPlayer(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid   = (int)luaL_checkinteger(L, 1);
    int sound = (int)luaL_checkinteger(L, 2);
    float x   = (float)luaL_optnumber(L, 3, 0.0);
    float y   = (float)luaL_optnumber(L, 4, 0.0);
    float z   = (float)luaL_optnumber(L, 5, 0.0);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->playSound((uint32_t)sound, Vector3(x,y,z));
    return 0;
}

static int nx_PlayAudioStreamForPlayer(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    const char* url = luaL_checkstring(L, 2);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->playAudio(StringView(url));
    return 0;
}

static int nx_StopAudioStreamForPlayer(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->stopAudio();
    return 0;
}

static int nx_GetPlayerKeys(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) {
        const PlayerKeyData& kd = p->getKeyData();
        lua_pushinteger(L, (lua_Integer)kd.keys);
        lua_pushinteger(L, (lua_Integer)kd.upDown);
        lua_pushinteger(L, (lua_Integer)kd.leftRight);
        return 3;
    }
    lua_pushinteger(L,0); lua_pushinteger(L,0); lua_pushinteger(L,0); return 3;
}

static int nx_PutPlayerInVehicle(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid  = (int)luaL_checkinteger(L, 1);
    int vid  = (int)luaL_checkinteger(L, 2);
    int seat = (int)luaL_optinteger(L, 3, 0);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    IVehiclesComponent* vehicles = comp->getVehicles();
    if (p && vehicles) {
        IVehicle* v = vehicles->get(vid);
        if (v) v->putPlayer(*p, seat);
    }
    return 0;
}

static int nx_RemovePlayerFromVehicle(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->removeFromVehicle(false);
    return 0;
}

static int nx_ForceClassSelection(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->forceClassSelection();
    return 0;
}

static int nx_SetPlayerDrunkLevel(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    int level = (int)luaL_checkinteger(L, 2);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) p->setDrunkLevel(level);
    return 0;
}

static int nx_GetPlayerDrunkLevel(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    lua_pushinteger(L, p ? p->getDrunkLevel() : 0);
    return 1;
}

static int nx_IsPlayerNPC(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    lua_pushboolean(L, p ? (p->isBot() ? 1 : 0) : 0);
    return 1;
}

// ===========================================================================
// VEHICLE
// ===========================================================================

static int nx_CreateVehicle(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    IVehiclesComponent* vehicles = comp->getVehicles();
    if (!vehicles) { lua_pushinteger(L, -1); return 1; }
    int   model   = (int)luaL_checkinteger(L, 1);
    float x       = (float)luaL_checknumber(L, 2);
    float y       = (float)luaL_checknumber(L, 3);
    float z       = (float)luaL_checknumber(L, 4);
    float angle   = (float)luaL_checknumber(L, 5);
    int   color1  = (int)luaL_checkinteger(L, 6);
    int   color2  = (int)luaL_checkinteger(L, 7);
    int   respawn = (int)luaL_optinteger(L, 8, -1);
    VehicleSpawnData sd;
    sd.modelID      = model;
    sd.position     = Vector3(x, y, z);
    sd.zRotation    = angle;
    sd.colour1      = color1;
    sd.colour2      = color2;
    sd.respawnDelay = Seconds(respawn);
    sd.siren        = false;
    sd.interior     = 0;
    IVehicle* v = vehicles->create(sd);
    lua_pushinteger(L, v ? v->getID() : -1);
    return 1;
}

static int nx_DestroyVehicle(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    IVehiclesComponent* vehicles = comp->getVehicles();
    int vid = (int)luaL_checkinteger(L, 1);
    if (vehicles) { IVehicle* v = vehicles->get(vid); if (v) vehicles->release(vid); }
    return 0;
}

static int nx_SetVehiclePos(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    IVehiclesComponent* vehicles = comp->getVehicles();
    int vid = (int)luaL_checkinteger(L, 1);
    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);
    float z = (float)luaL_checknumber(L, 4);
    if (vehicles) { IVehicle* v = vehicles->get(vid); if (v) v->setPosition(Vector3(x,y,z)); }
    return 0;
}

static int nx_GetVehiclePos(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    IVehiclesComponent* vehicles = comp->getVehicles();
    int vid = (int)luaL_checkinteger(L, 1);
    if (vehicles) {
        IVehicle* v = vehicles->get(vid);
        if (v) { Vector3 p = v->getPosition(); lua_pushnumber(L,p.x); lua_pushnumber(L,p.y); lua_pushnumber(L,p.z); return 3; }
    }
    lua_pushnumber(L,0); lua_pushnumber(L,0); lua_pushnumber(L,0); return 3;
}

static int nx_SetVehicleZAngle(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    IVehiclesComponent* vehicles = comp->getVehicles();
    int vid = (int)luaL_checkinteger(L, 1);
    float angle = (float)luaL_checknumber(L, 2);
    if (vehicles) { IVehicle* v = vehicles->get(vid); if (v) v->setZAngle(angle); }
    return 0;
}

static int nx_GetVehicleZAngle(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    IVehiclesComponent* vehicles = comp->getVehicles();
    int vid = (int)luaL_checkinteger(L, 1);
    if (vehicles) { IVehicle* v = vehicles->get(vid); if (v) { lua_pushnumber(L, v->getZAngle()); return 1; } }
    lua_pushnumber(L, 0); return 1;
}

static int nx_SetVehicleHealth(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    IVehiclesComponent* vehicles = comp->getVehicles();
    int vid = (int)luaL_checkinteger(L, 1);
    float hp = (float)luaL_checknumber(L, 2);
    if (vehicles) { IVehicle* v = vehicles->get(vid); if (v) v->setHealth(hp); }
    return 0;
}

static int nx_GetVehicleHealth(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    IVehiclesComponent* vehicles = comp->getVehicles();
    int vid = (int)luaL_checkinteger(L, 1);
    if (vehicles) { IVehicle* v = vehicles->get(vid); if (v) { lua_pushnumber(L, v->getHealth()); return 1; } }
    lua_pushnumber(L, 0); return 1;
}

static int nx_RepairVehicle(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    IVehiclesComponent* vehicles = comp->getVehicles();
    int vid = (int)luaL_checkinteger(L, 1);
    if (vehicles) { IVehicle* v = vehicles->get(vid); if (v) v->repair(); }
    return 0;
}

static int nx_SetVehicleVelocity(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    IVehiclesComponent* vehicles = comp->getVehicles();
    int vid = (int)luaL_checkinteger(L, 1);
    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);
    float z = (float)luaL_checknumber(L, 4);
    if (vehicles) { IVehicle* v = vehicles->get(vid); if (v) v->setVelocity(Vector3(x,y,z)); }
    return 0;
}

static int nx_GetVehicleVelocity(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    IVehiclesComponent* vehicles = comp->getVehicles();
    int vid = (int)luaL_checkinteger(L, 1);
    if (vehicles) {
        IVehicle* v = vehicles->get(vid);
        if (v) { Vector3 vel = v->getVelocity(); lua_pushnumber(L,vel.x); lua_pushnumber(L,vel.y); lua_pushnumber(L,vel.z); return 3; }
    }
    lua_pushnumber(L,0); lua_pushnumber(L,0); lua_pushnumber(L,0); return 3;
}

static int nx_GetPlayerVehicleID(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) {
        IPlayerVehicleData* vd = queryExtension<IPlayerVehicleData>(p);
        if (vd) { IVehicle* v = vd->getVehicle(); if (v) { lua_pushinteger(L, v->getID()); return 1; } }
    }
    lua_pushinteger(L, -1); return 1;
}

static int nx_IsPlayerInVehicle(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    bool in = false;
    if (p) { PlayerState s = p->getState(); in = (s == PlayerState_Driver || s == PlayerState_Passenger); }
    lua_pushboolean(L, in ? 1 : 0); return 1;
}

static int nx_IsPlayerInAnyVehicle(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    bool in = false;
    if (p) { PlayerState s = p->getState(); in = (s == PlayerState_Driver || s == PlayerState_Passenger); }
    lua_pushboolean(L, in ? 1 : 0); return 1;
}

// nx_ChangeVehicleColor(vehicleid, color1, color2)
static int nx_ChangeVehicleColor(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    IVehiclesComponent* vehicles = comp->getVehicles();
    int vid    = (int)luaL_checkinteger(L, 1);
    int color1 = (int)luaL_checkinteger(L, 2);
    int color2 = (int)luaL_checkinteger(L, 3);
    if (vehicles) { IVehicle* v = vehicles->get(vid); if (v) v->setColour(color1, color2); }
    return 0;
}

// nx_SetVehicleNumberPlate(vehicleid, plate)
static int nx_SetVehicleNumberPlate(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    IVehiclesComponent* vehicles = comp->getVehicles();
    int vid = (int)luaL_checkinteger(L, 1);
    const char* plate = luaL_checkstring(L, 2);
    if (vehicles) { IVehicle* v = vehicles->get(vid); if (v) v->setPlate(StringView(plate)); }
    return 0;
}

// ===========================================================================
// PICKUP
// ===========================================================================

// nx_CreatePickup(model, type, x, y, z, vworld) -> pickupid
static int nx_CreatePickup(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    IPickupsComponent* pickups = comp->getPickups();
    if (!pickups) { lua_pushinteger(L, -1); return 1; }
    int   model  = (int)luaL_checkinteger(L, 1);
    int   type   = (int)luaL_checkinteger(L, 2);
    float x      = (float)luaL_checknumber(L, 3);
    float y      = (float)luaL_checknumber(L, 4);
    float z      = (float)luaL_checknumber(L, 5);
    int   vworld = (int)luaL_optinteger(L, 6, 0);
    IPickup* pk = pickups->create(model, (PickupType)type, Vector3(x,y,z), (uint32_t)vworld, false);
    lua_pushinteger(L, pk ? pk->getID() : -1);
    return 1;
}

// nx_DestroyPickup(pickupid)
static int nx_DestroyPickup(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    IPickupsComponent* pickups = comp->getPickups();
    int pid = (int)luaL_checkinteger(L, 1);
    if (pickups) { IPickup* pk = pickups->get(pid); if (pk) pickups->release(pid); }
    return 0;
}

// ===========================================================================
// OBJECT
// ===========================================================================

// nx_CreateObject(model, x, y, z, rx, ry, rz, drawdist) -> objectid
static int nx_CreateObject(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    IObjectsComponent* objects = comp->getObjects();
    if (!objects) { lua_pushinteger(L, -1); return 1; }
    int   model = (int)luaL_checkinteger(L, 1);
    float x     = (float)luaL_checknumber(L, 2);
    float y     = (float)luaL_checknumber(L, 3);
    float z     = (float)luaL_checknumber(L, 4);
    float rx    = (float)luaL_optnumber(L, 5, 0.0);
    float ry    = (float)luaL_optnumber(L, 6, 0.0);
    float rz    = (float)luaL_optnumber(L, 7, 0.0);
    float dd    = (float)luaL_optnumber(L, 8, 0.0);
    IObject* obj = objects->create(model, Vector3(x,y,z), Vector3(rx,ry,rz), dd);
    lua_pushinteger(L, obj ? obj->getID() : -1);
    return 1;
}

// nx_DestroyObject(objectid)
static int nx_DestroyObject(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    IObjectsComponent* objects = comp->getObjects();
    int oid = (int)luaL_checkinteger(L, 1);
    if (objects) { IObject* obj = objects->get(oid); if (obj) objects->release(oid); }
    return 0;
}

// nx_SetObjectPos(objectid, x, y, z)
static int nx_SetObjectPos(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    IObjectsComponent* objects = comp->getObjects();
    int oid = (int)luaL_checkinteger(L, 1);
    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);
    float z = (float)luaL_checknumber(L, 4);
    if (objects) { IObject* obj = objects->get(oid); if (obj) obj->setPosition(Vector3(x,y,z)); }
    return 0;
}

// nx_GetObjectPos(objectid) -> x, y, z
static int nx_GetObjectPos(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    IObjectsComponent* objects = comp->getObjects();
    int oid = (int)luaL_checkinteger(L, 1);
    if (objects) {
        IObject* obj = objects->get(oid);
        if (obj) { Vector3 p = obj->getPosition(); lua_pushnumber(L,p.x); lua_pushnumber(L,p.y); lua_pushnumber(L,p.z); return 3; }
    }
    lua_pushnumber(L,0); lua_pushnumber(L,0); lua_pushnumber(L,0); return 3;
}

// nx_SetObjectRot(objectid, rx, ry, rz)
static int nx_SetObjectRot(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    IObjectsComponent* objects = comp->getObjects();
    int oid = (int)luaL_checkinteger(L, 1);
    float rx = (float)luaL_checknumber(L, 2);
    float ry = (float)luaL_checknumber(L, 3);
    float rz = (float)luaL_checknumber(L, 4);
    if (objects) { IObject* obj = objects->get(oid); if (obj) obj->setRotation(GTAQuat(Vector3(rx,ry,rz))); }
    return 0;
}

// nx_MoveObject(objectid, x, y, z, speed, rx, ry, rz)
static int nx_MoveObject(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    IObjectsComponent* objects = comp->getObjects();
    int oid = (int)luaL_checkinteger(L, 1);
    float x     = (float)luaL_checknumber(L, 2);
    float y     = (float)luaL_checknumber(L, 3);
    float z     = (float)luaL_checknumber(L, 4);
    float speed = (float)luaL_checknumber(L, 5);
    float rx    = (float)luaL_optnumber(L, 6, -1000.0);
    float ry    = (float)luaL_optnumber(L, 7, -1000.0);
    float rz    = (float)luaL_optnumber(L, 8, -1000.0);
    if (objects) {
        IObject* obj = objects->get(oid);
        if (obj) {
            ObjectMoveData md;
            md.targetPos = Vector3(x,y,z);
            md.targetRot = Vector3(rx,ry,rz);
            md.speed     = speed;
            obj->move(md);
        }
    }
    return 0;
}

// nx_StopObject(objectid)
static int nx_StopObject(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    IObjectsComponent* objects = comp->getObjects();
    int oid = (int)luaL_checkinteger(L, 1);
    if (objects) { IObject* obj = objects->get(oid); if (obj) obj->stop(); }
    return 0;
}

// ===========================================================================
// 3D TEXT LABEL
// ===========================================================================

// nx_Create3DTextLabel(text, color, x, y, z, drawdist, vworld, los) -> labelid
static int nx_Create3DTextLabel(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    ITextLabelsComponent* tl = comp->getTextLabels();
    if (!tl) { lua_pushinteger(L, -1); return 1; }
    const char* text = luaL_checkstring(L, 1);
    lua_Integer col  = luaL_checkinteger(L, 2);
    float x          = (float)luaL_checknumber(L, 3);
    float y          = (float)luaL_checknumber(L, 4);
    float z          = (float)luaL_checknumber(L, 5);
    float dd         = (float)luaL_optnumber(L, 6, 40.0);
    int   vw         = (int)luaL_optinteger(L, 7, 0);
    bool  los        = lua_toboolean(L, 8) != 0;
    ITextLabel* label = tl->create(StringView(text), colourFromInt(col), Vector3(x,y,z), dd, vw, los);
    lua_pushinteger(L, label ? label->getID() : -1);
    return 1;
}

// nx_Delete3DTextLabel(labelid)
static int nx_Delete3DTextLabel(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    ITextLabelsComponent* tl = comp->getTextLabels();
    int lid = (int)luaL_checkinteger(L, 1);
    if (tl) { ITextLabel* label = tl->get(lid); if (label) tl->release(lid); }
    return 0;
}

// nx_Update3DTextLabelText(labelid, color, text)
static int nx_Update3DTextLabelText(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    ITextLabelsComponent* tl = comp->getTextLabels();
    int lid = (int)luaL_checkinteger(L, 1);
    lua_Integer col  = luaL_checkinteger(L, 2);
    const char* text = luaL_checkstring(L, 3);
    if (tl) { ITextLabel* label = tl->get(lid); if (label) label->setColourAndText(colourFromInt(col), StringView(text)); }
    return 0;
}

// ===========================================================================
// TEXTDRAW
// ===========================================================================

// nx_TextDrawCreate(x, y, text) -> textdrawid
static int nx_TextDrawCreate(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    ITextDrawsComponent* td = comp->getTextDraws();
    if (!td) { lua_pushinteger(L, -1); return 1; }
    float x = (float)luaL_checknumber(L, 1);
    float y = (float)luaL_checknumber(L, 2);
    const char* text = luaL_checkstring(L, 3);
    ITextDraw* draw = td->create(Vector2(x,y), StringView(text));
    lua_pushinteger(L, draw ? draw->getID() : -1);
    return 1;
}

// nx_TextDrawDestroy(textdrawid)
static int nx_TextDrawDestroy(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    ITextDrawsComponent* td = comp->getTextDraws();
    int did = (int)luaL_checkinteger(L, 1);
    if (td) { ITextDraw* draw = td->get(did); if (draw) td->release(did); }
    return 0;
}

// nx_TextDrawShowForPlayer(playerid, textdrawid)
static int nx_TextDrawShowForPlayer(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    int did = (int)luaL_checkinteger(L, 2);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    ITextDrawsComponent* td = comp->getTextDraws();
    if (p && td) { ITextDraw* draw = td->get(did); if (draw) draw->showForPlayer(*p); }
    return 0;
}

// nx_TextDrawHideForPlayer(playerid, textdrawid)
static int nx_TextDrawHideForPlayer(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    int did = (int)luaL_checkinteger(L, 2);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    ITextDrawsComponent* td = comp->getTextDraws();
    if (p && td) { ITextDraw* draw = td->get(did); if (draw) draw->hideForPlayer(*p); }
    return 0;
}

// nx_TextDrawSetString(textdrawid, text)
static int nx_TextDrawSetString(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    ITextDrawsComponent* td = comp->getTextDraws();
    int did = (int)luaL_checkinteger(L, 1);
    const char* text = luaL_checkstring(L, 2);
    if (td) { ITextDraw* draw = td->get(did); if (draw) draw->setText(StringView(text)); }
    return 0;
}

// nx_TextDrawColor(textdrawid, color)
static int nx_TextDrawColor(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    ITextDrawsComponent* td = comp->getTextDraws();
    int did = (int)luaL_checkinteger(L, 1);
    lua_Integer col = luaL_checkinteger(L, 2);
    if (td) { ITextDraw* draw = td->get(did); if (draw) draw->setColour(colourFromInt(col)); }
    return 0;
}

// nx_TextDrawBoxColor(textdrawid, color)
static int nx_TextDrawBoxColor(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    ITextDrawsComponent* td = comp->getTextDraws();
    int did = (int)luaL_checkinteger(L, 1);
    lua_Integer col = luaL_checkinteger(L, 2);
    if (td) { ITextDraw* draw = td->get(did); if (draw) draw->setBoxColour(colourFromInt(col)); }
    return 0;
}

// nx_TextDrawUseBox(textdrawid, use)
static int nx_TextDrawUseBox(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    ITextDrawsComponent* td = comp->getTextDraws();
    int did = (int)luaL_checkinteger(L, 1);
    bool use = lua_toboolean(L, 2) != 0;
    if (td) { ITextDraw* draw = td->get(did); if (draw) draw->useBox(use); }
    return 0;
}

// nx_TextDrawLetterSize(textdrawid, x, y)
static int nx_TextDrawLetterSize(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    ITextDrawsComponent* td = comp->getTextDraws();
    int did = (int)luaL_checkinteger(L, 1);
    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);
    if (td) { ITextDraw* draw = td->get(did); if (draw) draw->setLetterSize(Vector2(x,y)); }
    return 0;
}

// nx_TextDrawTextSize(textdrawid, x, y)
static int nx_TextDrawTextSize(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    ITextDrawsComponent* td = comp->getTextDraws();
    int did = (int)luaL_checkinteger(L, 1);
    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);
    if (td) { ITextDraw* draw = td->get(did); if (draw) draw->setTextSize(Vector2(x,y)); }
    return 0;
}

// nx_TextDrawAlignment(textdrawid, alignment)  1=left 2=center 3=right
static int nx_TextDrawAlignment(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    ITextDrawsComponent* td = comp->getTextDraws();
    int did = (int)luaL_checkinteger(L, 1);
    int align = (int)luaL_checkinteger(L, 2);
    if (td) { ITextDraw* draw = td->get(did); if (draw) draw->setAlignment((TextDrawAlignmentTypes)align); }
    return 0;
}

// nx_TextDrawFont(textdrawid, font)
static int nx_TextDrawFont(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    ITextDrawsComponent* td = comp->getTextDraws();
    int did  = (int)luaL_checkinteger(L, 1);
    int font = (int)luaL_checkinteger(L, 2);
    if (td) { ITextDraw* draw = td->get(did); if (draw) draw->setStyle((TextDrawStyle)font); }
    return 0;
}

// nx_TextDrawSetShadow(textdrawid, size)
static int nx_TextDrawSetShadow(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    ITextDrawsComponent* td = comp->getTextDraws();
    int did  = (int)luaL_checkinteger(L, 1);
    int size = (int)luaL_checkinteger(L, 2);
    if (td) { ITextDraw* draw = td->get(did); if (draw) draw->setShadow(size); }
    return 0;
}

// nx_TextDrawSetOutline(textdrawid, size)
static int nx_TextDrawSetOutline(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    ITextDrawsComponent* td = comp->getTextDraws();
    int did  = (int)luaL_checkinteger(L, 1);
    int size = (int)luaL_checkinteger(L, 2);
    if (td) { ITextDraw* draw = td->get(did); if (draw) draw->setOutline(size); }
    return 0;
}

// nx_TextDrawSetProportional(textdrawid, set)
static int nx_TextDrawSetProportional(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    ITextDrawsComponent* td = comp->getTextDraws();
    int did = (int)luaL_checkinteger(L, 1);
    bool set = lua_toboolean(L, 2) != 0;
    if (td) { ITextDraw* draw = td->get(did); if (draw) draw->setProportional(set); }
    return 0;
}

// nx_TextDrawSetSelectable(textdrawid, set)
static int nx_TextDrawSetSelectable(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    ITextDrawsComponent* td = comp->getTextDraws();
    int did = (int)luaL_checkinteger(L, 1);
    bool set = lua_toboolean(L, 2) != 0;
    if (td) { ITextDraw* draw = td->get(did); if (draw) draw->setSelectable(set); }
    return 0;
}

// nx_TextDrawShowForAll(textdrawid)
static int nx_TextDrawShowForAll(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    ITextDrawsComponent* td = comp->getTextDraws();
    int did = (int)luaL_checkinteger(L, 1);
    if (td) {
        ITextDraw* draw = td->get(did);
        if (draw) {
            for (IPlayer* p : comp->getCore()->getPlayers().entries()) {
                draw->showForPlayer(*p);
            }
        }
    }
    return 0;
}

// nx_TextDrawHideForAll(textdrawid)
static int nx_TextDrawHideForAll(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    ITextDrawsComponent* td = comp->getTextDraws();
    int did = (int)luaL_checkinteger(L, 1);
    if (td) {
        ITextDraw* draw = td->get(did);
        if (draw) {
            for (IPlayer* p : comp->getCore()->getPlayers().entries()) {
                draw->hideForPlayer(*p);
            }
        }
    }
    return 0;
}

// ===========================================================================
// CLASS
// ===========================================================================

// nx_AddPlayerClass(skin, x, y, z, angle, w1, a1, w2, a2, w3, a3) -> classid
static int nx_AddPlayerClass(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    IClassesComponent* classes = comp->getClasses();
    if (!classes) { lua_pushinteger(L, -1); return 1; }
    int   skin  = (int)luaL_checkinteger(L, 1);
    float x     = (float)luaL_checknumber(L, 2);
    float y     = (float)luaL_checknumber(L, 3);
    float z     = (float)luaL_checknumber(L, 4);
    float angle = (float)luaL_checknumber(L, 5);
    int w1 = (int)luaL_optinteger(L, 6, 0);  int a1 = (int)luaL_optinteger(L, 7, 0);
    int w2 = (int)luaL_optinteger(L, 8, 0);  int a2 = (int)luaL_optinteger(L, 9, 0);
    int w3 = (int)luaL_optinteger(L, 10, 0); int a3 = (int)luaL_optinteger(L, 11, 0);
    WeaponSlots weapons;
    weapons[0] = WeaponSlotData((uint8_t)w1, (uint32_t)a1);
    weapons[1] = WeaponSlotData((uint8_t)w2, (uint32_t)a2);
    weapons[2] = WeaponSlotData((uint8_t)w3, (uint32_t)a3);
    IClass* cls = classes->create(skin, 255, Vector3(x,y,z), angle, weapons);
    lua_pushinteger(L, cls ? cls->getID() : -1);
    return 1;
}

// nx_SetSpawnInfo(playerid, skin, x, y, z, angle, w1, a1, w2, a2, w3, a3)
static int nx_SetSpawnInfo(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid   = (int)luaL_checkinteger(L, 1);
    int skin  = (int)luaL_checkinteger(L, 2);
    float x   = (float)luaL_checknumber(L, 3);
    float y   = (float)luaL_checknumber(L, 4);
    float z   = (float)luaL_checknumber(L, 5);
    float ang = (float)luaL_checknumber(L, 6);
    int w1 = (int)luaL_optinteger(L, 7, 0);  int a1 = (int)luaL_optinteger(L, 8, 0);
    int w2 = (int)luaL_optinteger(L, 9, 0);  int a2 = (int)luaL_optinteger(L, 10, 0);
    int w3 = (int)luaL_optinteger(L, 11, 0); int a3 = (int)luaL_optinteger(L, 12, 0);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) {
        IPlayerClassData* cd = queryExtension<IPlayerClassData>(p);
        if (cd) {
            WeaponSlots weapons;
            weapons[0] = WeaponSlotData((uint8_t)w1, (uint32_t)a1);
            weapons[1] = WeaponSlotData((uint8_t)w2, (uint32_t)a2);
            weapons[2] = WeaponSlotData((uint8_t)w3, (uint32_t)a3);
            PlayerClass info(skin, 255, Vector3(x,y,z), ang, weapons);
            cd->setSpawnInfo(info);
        }
    }
    return 0;
}

// ===========================================================================
// CHECKPOINT
// ===========================================================================

// nx_SetPlayerCheckpoint(playerid, x, y, z, radius)
static int nx_SetPlayerCheckpoint(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid    = (int)luaL_checkinteger(L, 1);
    float x    = (float)luaL_checknumber(L, 2);
    float y    = (float)luaL_checknumber(L, 3);
    float z    = (float)luaL_checknumber(L, 4);
    float rad  = (float)luaL_checknumber(L, 5);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) {
        IPlayerCheckpointData* cd = queryExtension<IPlayerCheckpointData>(p);
        if (cd) {
            ICheckpointData& cp = cd->getCheckpoint();
            cp.setPosition(Vector3(x,y,z));
            cp.setRadius(rad);
            cp.enable();
        }
    }
    return 0;
}

// nx_DisablePlayerCheckpoint(playerid)
static int nx_DisablePlayerCheckpoint(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) {
        IPlayerCheckpointData* cd = queryExtension<IPlayerCheckpointData>(p);
        if (cd) cd->getCheckpoint().disable();
    }
    return 0;
}

// nx_IsPlayerInCheckpoint(playerid) -> bool
static int nx_IsPlayerInCheckpoint(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    bool inside = false;
    if (p) {
        IPlayerCheckpointData* cd = queryExtension<IPlayerCheckpointData>(p);
        if (cd) inside = cd->getCheckpoint().isPlayerInside();
    }
    lua_pushboolean(L, inside ? 1 : 0);
    return 1;
}

// ===========================================================================
// GANGZONE
// ===========================================================================

// nx_GangZoneCreate(minx, miny, maxx, maxy) -> zoneid
static int nx_GangZoneCreate(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    IGangZonesComponent* gz = comp->getGangZones();
    if (!gz) { lua_pushinteger(L, -1); return 1; }
    float minx = (float)luaL_checknumber(L, 1);
    float miny = (float)luaL_checknumber(L, 2);
    float maxx = (float)luaL_checknumber(L, 3);
    float maxy = (float)luaL_checknumber(L, 4);
    GangZonePos pos;
    pos.min = Vector2(minx, miny);
    pos.max = Vector2(maxx, maxy);
    IGangZone* zone = gz->create(pos);
    lua_pushinteger(L, zone ? zone->getID() : -1);
    return 1;
}

// nx_GangZoneDestroy(zoneid)
static int nx_GangZoneDestroy(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    IGangZonesComponent* gz = comp->getGangZones();
    int zid = (int)luaL_checkinteger(L, 1);
    if (gz) { IGangZone* zone = gz->get(zid); if (zone) gz->release(zid); }
    return 0;
}

// nx_GangZoneShowForPlayer(playerid, zoneid, color)
static int nx_GangZoneShowForPlayer(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    int zid = (int)luaL_checkinteger(L, 2);
    lua_Integer col = luaL_checkinteger(L, 3);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    IGangZonesComponent* gz = comp->getGangZones();
    if (p && gz) { IGangZone* zone = gz->get(zid); if (zone) zone->showForPlayer(*p, colourFromInt(col)); }
    return 0;
}

// nx_GangZoneHideForPlayer(playerid, zoneid)
static int nx_GangZoneHideForPlayer(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    int zid = (int)luaL_checkinteger(L, 2);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    IGangZonesComponent* gz = comp->getGangZones();
    if (p && gz) { IGangZone* zone = gz->get(zid); if (zone) zone->hideForPlayer(*p); }
    return 0;
}

// nx_GangZoneFlashForPlayer(playerid, zoneid, flashcolor)
static int nx_GangZoneFlashForPlayer(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    int zid = (int)luaL_checkinteger(L, 2);
    lua_Integer col = luaL_checkinteger(L, 3);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    IGangZonesComponent* gz = comp->getGangZones();
    if (p && gz) { IGangZone* zone = gz->get(zid); if (zone) zone->flashForPlayer(*p, colourFromInt(col)); }
    return 0;
}

// nx_GangZoneStopFlashForPlayer(playerid, zoneid)
static int nx_GangZoneStopFlashForPlayer(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    int zid = (int)luaL_checkinteger(L, 2);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    IGangZonesComponent* gz = comp->getGangZones();
    if (p && gz) { IGangZone* zone = gz->get(zid); if (zone) zone->stopFlashForPlayer(*p); }
    return 0;
}

// nx_GangZoneShowForAll(zoneid, color)
static int nx_GangZoneShowForAll(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int zid = (int)luaL_checkinteger(L, 1);
    lua_Integer col = luaL_checkinteger(L, 2);
    IGangZonesComponent* gz = comp->getGangZones();
    if (gz) {
        IGangZone* zone = gz->get(zid);
        if (zone) {
            for (IPlayer* p : comp->getCore()->getPlayers().entries())
                zone->showForPlayer(*p, colourFromInt(col));
        }
    }
    return 0;
}

// nx_GangZoneHideForAll(zoneid)
static int nx_GangZoneHideForAll(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int zid = (int)luaL_checkinteger(L, 1);
    IGangZonesComponent* gz = comp->getGangZones();
    if (gz) {
        IGangZone* zone = gz->get(zid);
        if (zone) {
            for (IPlayer* p : comp->getCore()->getPlayers().entries())
                zone->hideForPlayer(*p);
        }
    }
    return 0;
}

// ===========================================================================
// HOT RELOAD
// ===========================================================================

// nx_listFiles(path) -> table of filenames (strings)
// Lista arquivos e subpastas em um diretório relativo ao lua/
static int nx_listFiles(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    const char* rel = luaL_checkstring(L, 1);

    // Resolve path relative to the lua/ directory
    ghc::filesystem::path base = ghc::filesystem::absolute("lua");
    ghc::filesystem::path dir  = base / rel;

    lua_newtable(L);
    int idx = 1;

    if (!ghc::filesystem::exists(dir) || !ghc::filesystem::is_directory(dir))
        return 1; // empty table

    std::vector<std::string> entries;
    for (const auto& entry : ghc::filesystem::directory_iterator(dir))
    {
        std::string name = entry.path().filename().string();
        if (entry.is_directory())
            name += "/";
        entries.push_back(name);
    }
    std::sort(entries.begin(), entries.end());

    for (const auto& e : entries)
    {
        lua_pushstring(L, e.c_str());
        lua_rawseti(L, -2, idx++);
    }
    return 1;
}

// nx_isDirectory(path) -> bool
static int nx_isDirectory(lua_State* L)
{
    const char* rel = luaL_checkstring(L, 1);
    ghc::filesystem::path base = ghc::filesystem::absolute("lua");
    ghc::filesystem::path p    = base / rel;
    lua_pushboolean(L, ghc::filesystem::is_directory(p) ? 1 : 0);
    return 1;
}

// nx_reloadScript(path) -> bool, errmsg
// path is relative to lua/ directory
static int nx_reloadScript(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    const char* rel = luaL_checkstring(L, 1);

    ghc::filesystem::path base = ghc::filesystem::absolute("lua");
    ghc::filesystem::path full = base / rel;
    std::string fullStr = full.string();

    // Load and execute the file
    int status = luaL_loadfile(L, fullStr.c_str());
    if (status != LUA_OK)
    {
        std::string err = lua_tostring(L, -1);
        lua_pop(L, 1);
        comp->getCore()->printLn("[Reload] Syntax error in %s: %s", rel, err.c_str());
        lua_pushboolean(L, 0);
        lua_pushstring(L, err.c_str());
        return 2;
    }

    status = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (status != LUA_OK)
    {
        std::string err = lua_tostring(L, -1);
        lua_pop(L, 1);
        comp->getCore()->printLn("[Reload] Runtime error in %s: %s", rel, err.c_str());
        lua_pushboolean(L, 0);
        lua_pushstring(L, err.c_str());
        return 2;
    }

    comp->getCore()->printLn("[Reload] Reloaded: %s", rel);
    lua_pushboolean(L, 1);
    lua_pushstring(L, "");
    return 2;
}

// nx_getScriptDir() -> string  (absolute path to lua/ directory)
static int nx_getScriptDir(lua_State* L)
{
    std::string dir = ghc::filesystem::absolute("lua").string();
    lua_pushstring(L, dir.c_str());
    return 1;
}

static int nx_SetGameModeText(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    const char* text = luaL_checkstring(L, 1);
    comp->getCore()->setData(SettableCoreDataType::ModeText, StringView(text));
    return 0;
}

static int nx_GetTickCount(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    lua_pushinteger(L, (lua_Integer)comp->getCore()->getTickCount());
    return 1;
}

static int nx_print(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    const char* msg = luaL_checkstring(L, 1);
    comp->getCore()->printLn("%s", msg);
    return 0;
}

static int nx_GameModeExit(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    comp->getCore()->resetAll();
    return 0;
}

// nx_SetWorldTime(hour)
static int nx_SetWorldTime(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int hour = (int)luaL_checkinteger(L, 1);
    for (IPlayer* p : comp->getCore()->getPlayers().entries())
        p->setWorldTime(Hours(hour));
    return 0;
}

// nx_SetWeather(weatherid)
static int nx_SetWeather(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int weather = (int)luaL_checkinteger(L, 1);
    for (IPlayer* p : comp->getCore()->getPlayers().entries())
        p->setWeather(weather);
    return 0;
}

// nx_SetGravity(gravity)
static int nx_SetGravity(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    float g = (float)luaL_checknumber(L, 1);
    for (IPlayer* p : comp->getCore()->getPlayers().entries())
        p->setGravity(g);
    return 0;
}

// nx_GameTextForAll(text, time_ms, style)
static int nx_GameTextForAll(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    const char* text = luaL_checkstring(L, 1);
    int time  = (int)luaL_checkinteger(L, 2);
    int style = (int)luaL_checkinteger(L, 3);
    for (IPlayer* p : comp->getCore()->getPlayers().entries())
        p->sendGameText(StringView(text), Milliseconds(time), style);
    return 0;
}

// nx_GetMaxPlayers() -> int
static int nx_GetMaxPlayers(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int* ptr = comp->getCore()->getConfig().getInt("max_players");
    lua_pushinteger(L, ptr ? (lua_Integer)*ptr : 50);
    return 1;
}

// ===========================================================================
// TIMER
// ===========================================================================

struct LuaTimer {
    int funcRef;
    int interval;
    bool repeat;
    int elapsed;
    bool dead;
};

static std::vector<LuaTimer>* getTimers(lua_State* L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, "__nx_timers");
    auto* timers = static_cast<std::vector<LuaTimer>*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return timers;
}

static int nx_SetTimer(lua_State* L)
{
    auto* timers = getTimers(L);
    if (!timers) { lua_pushinteger(L, -1); return 1; }
    if (lua_isstring(L, 1)) lua_getglobal(L, lua_tostring(L, 1));
    else lua_pushvalue(L, 1);
    if (!lua_isfunction(L, -1)) { lua_pop(L, 1); lua_pushinteger(L, -1); return 1; }
    int ref      = luaL_ref(L, LUA_REGISTRYINDEX);
    int interval = (int)luaL_checkinteger(L, 2);
    bool repeat  = lua_toboolean(L, 3) != 0;
    LuaTimer t; t.funcRef = ref; t.interval = interval; t.repeat = repeat; t.elapsed = 0; t.dead = false;
    timers->push_back(t);
    lua_pushinteger(L, (lua_Integer)(timers->size() - 1));
    return 1;
}

static int nx_KillTimer(lua_State* L)
{
    auto* timers = getTimers(L);
    int id = (int)luaL_checkinteger(L, 1);
    if (timers && id >= 0 && id < (int)timers->size()) {
        LuaTimer& t = (*timers)[id];
        if (!t.dead) { luaL_unref(L, LUA_REGISTRYINDEX, t.funcRef); t.dead = true; }
    }
    return 0;
}

void lua_tick_timers(lua_State* L, int elapsedMs)
{
    auto* timers = getTimers(L);
    if (!timers) return;
    for (auto& t : *timers) {
        if (t.dead) continue;
        t.elapsed += elapsedMs;
        if (t.elapsed >= t.interval) {
            t.elapsed -= t.interval;
            lua_rawgeti(L, LUA_REGISTRYINDEX, t.funcRef);
            if (lua_isfunction(L, -1)) {
                if (lua_pcall(L, 0, 0, 0) != LUA_OK) lua_pop(L, 1);
            } else lua_pop(L, 1);
            if (!t.repeat) { luaL_unref(L, LUA_REGISTRYINDEX, t.funcRef); t.dead = true; }
        }
    }
}

// ===========================================================================
// DIALOG
// ===========================================================================

static int nx_ShowPlayerDialog(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid      = (int)luaL_checkinteger(L, 1);
    int dialogid = (int)luaL_checkinteger(L, 2);
    int style    = (int)luaL_checkinteger(L, 3);
    const char* title = luaL_checkstring(L, 4);
    const char* body  = luaL_checkstring(L, 5);
    const char* btn1  = luaL_checkstring(L, 6);
    const char* btn2  = luaL_optstring(L, 7, "");
    IPlayer* p = getPlayer(comp->getCore(), pid);
    IDialogsComponent* dialogs = comp->getDialogs();
    if (p && dialogs) {
        IPlayerDialogData* data = queryExtension<IPlayerDialogData>(p);
        if (data) data->show(*p, dialogid, (DialogStyle)style,
            StringView(title), StringView(body), StringView(btn1), StringView(btn2));
    }
    return 0;
}

static int nx_HidePlayerDialog(lua_State* L)
{
    LuaComponent* comp = getComponent(L);
    int pid = (int)luaL_checkinteger(L, 1);
    IPlayer* p = getPlayer(comp->getCore(), pid);
    if (p) { IPlayerDialogData* data = queryExtension<IPlayerDialogData>(p); if (data) data->hide(*p); }
    return 0;
}

// ===========================================================================
// REGISTRATION
// ===========================================================================

void lua_register_api(lua_State* L, LuaComponent* component)
{
    lua_pushlightuserdata(L, component);
    lua_setfield(L, LUA_REGISTRYINDEX, "__nx_component");

    auto* timers = new std::vector<LuaTimer>();
    lua_pushlightuserdata(L, timers);
    lua_setfield(L, LUA_REGISTRYINDEX, "__nx_timers");

    // --- Player: basic ---
    lua_register(L, "nx_SendClientMessage",       nx_SendClientMessage);
    lua_register(L, "nx_SendClientMessageToAll",  nx_SendClientMessageToAll);
    lua_register(L, "nx_GetPlayerName",           nx_GetPlayerName);
    lua_register(L, "nx_SetPlayerPos",            nx_SetPlayerPos);
    lua_register(L, "nx_GetPlayerPos",            nx_GetPlayerPos);
    lua_register(L, "nx_SetPlayerFacingAngle",    nx_SetPlayerFacingAngle);
    lua_register(L, "nx_GetPlayerFacingAngle",    nx_GetPlayerFacingAngle);
    lua_register(L, "nx_SetPlayerHealth",         nx_SetPlayerHealth);
    lua_register(L, "nx_GetPlayerHealth",         nx_GetPlayerHealth);
    lua_register(L, "nx_SetPlayerArmour",         nx_SetPlayerArmour);
    lua_register(L, "nx_GetPlayerArmour",         nx_GetPlayerArmour);
    lua_register(L, "nx_GivePlayerMoney",         nx_GivePlayerMoney);
    lua_register(L, "nx_GetPlayerMoney",          nx_GetPlayerMoney);
    lua_register(L, "nx_ResetPlayerMoney",        nx_ResetPlayerMoney);
    lua_register(L, "nx_SetPlayerScore",          nx_SetPlayerScore);
    lua_register(L, "nx_GetPlayerScore",          nx_GetPlayerScore);
    lua_register(L, "nx_SetPlayerSkin",           nx_SetPlayerSkin);
    lua_register(L, "nx_GetPlayerSkin",           nx_GetPlayerSkin);
    lua_register(L, "nx_SetPlayerColor",          nx_SetPlayerColor);
    lua_register(L, "nx_GetPlayerColor",          nx_GetPlayerColor);
    lua_register(L, "nx_SetPlayerTeam",           nx_SetPlayerTeam);
    lua_register(L, "nx_GetPlayerTeam",           nx_GetPlayerTeam);
    lua_register(L, "nx_GetPlayerState",          nx_GetPlayerState);
    lua_register(L, "nx_SetPlayerVirtualWorld",   nx_SetPlayerVirtualWorld);
    lua_register(L, "nx_GetPlayerVirtualWorld",   nx_GetPlayerVirtualWorld);
    lua_register(L, "nx_SetPlayerInterior",       nx_SetPlayerInterior);
    lua_register(L, "nx_GetPlayerInterior",       nx_GetPlayerInterior);
    lua_register(L, "nx_SpawnPlayer",             nx_SpawnPlayer);
    lua_register(L, "nx_KickPlayer",              nx_KickPlayer);
    lua_register(L, "nx_BanPlayer",               nx_BanPlayer);
    lua_register(L, "nx_IsPlayerConnected",       nx_IsPlayerConnected);
    lua_register(L, "nx_GetPlayerPing",           nx_GetPlayerPing);
    lua_register(L, "nx_GetPlayerIP",             nx_GetPlayerIP);
    lua_register(L, "nx_IsPlayerNPC",             nx_IsPlayerNPC);
    // --- Player: weapons / anim / misc ---
    lua_register(L, "nx_GivePlayerWeapon",        nx_GivePlayerWeapon);
    lua_register(L, "nx_ResetPlayerWeapons",      nx_ResetPlayerWeapons);
    lua_register(L, "nx_GetPlayerWeapon",         nx_GetPlayerWeapon);
    lua_register(L, "nx_GetPlayerAmmo",           nx_GetPlayerAmmo);
    lua_register(L, "nx_SetPlayerArmedWeapon",    nx_SetPlayerArmedWeapon);
    lua_register(L, "nx_ApplyAnimation",          nx_ApplyAnimation);
    lua_register(L, "nx_ClearAnimations",         nx_ClearAnimations);
    lua_register(L, "nx_SetPlayerSpecialAction",  nx_SetPlayerSpecialAction);
    lua_register(L, "nx_GetPlayerSpecialAction",  nx_GetPlayerSpecialAction);
    lua_register(L, "nx_SetPlayerFightingStyle",  nx_SetPlayerFightingStyle);
    lua_register(L, "nx_SetPlayerVelocity",       nx_SetPlayerVelocity);
    lua_register(L, "nx_GetPlayerVelocity",       nx_GetPlayerVelocity);
    lua_register(L, "nx_SetPlayerGravity",        nx_SetPlayerGravity);
    lua_register(L, "nx_SetPlayerWeather",        nx_SetPlayerWeather);
    lua_register(L, "nx_SetPlayerTime",           nx_SetPlayerTime);
    lua_register(L, "nx_TogglePlayerClock",       nx_TogglePlayerClock);
    lua_register(L, "nx_SetPlayerWantedLevel",    nx_SetPlayerWantedLevel);
    lua_register(L, "nx_GetPlayerWantedLevel",    nx_GetPlayerWantedLevel);
    lua_register(L, "nx_SetPlayerChatBubble",     nx_SetPlayerChatBubble);
    lua_register(L, "nx_GameTextForPlayer",       nx_GameTextForPlayer);
    lua_register(L, "nx_SetPlayerMapIcon",        nx_SetPlayerMapIcon);
    lua_register(L, "nx_RemovePlayerMapIcon",     nx_RemovePlayerMapIcon);
    lua_register(L, "nx_TogglePlayerControllable",nx_TogglePlayerControllable);
    lua_register(L, "nx_SetPlayerCameraPos",      nx_SetPlayerCameraPos);
    lua_register(L, "nx_SetCameraBehindPlayer",   nx_SetCameraBehindPlayer);
    lua_register(L, "nx_SetPlayerCameraLookAt",   nx_SetPlayerCameraLookAt);
    lua_register(L, "nx_PlaySoundForPlayer",      nx_PlaySoundForPlayer);
    lua_register(L, "nx_PlayAudioStreamForPlayer",nx_PlayAudioStreamForPlayer);
    lua_register(L, "nx_StopAudioStreamForPlayer",nx_StopAudioStreamForPlayer);
    lua_register(L, "nx_GetPlayerKeys",           nx_GetPlayerKeys);
    lua_register(L, "nx_PutPlayerInVehicle",      nx_PutPlayerInVehicle);
    lua_register(L, "nx_RemovePlayerFromVehicle", nx_RemovePlayerFromVehicle);
    lua_register(L, "nx_ForceClassSelection",     nx_ForceClassSelection);
    lua_register(L, "nx_SetPlayerDrunkLevel",     nx_SetPlayerDrunkLevel);
    lua_register(L, "nx_GetPlayerDrunkLevel",     nx_GetPlayerDrunkLevel);
    // --- Vehicle ---
    lua_register(L, "nx_CreateVehicle",           nx_CreateVehicle);
    lua_register(L, "nx_DestroyVehicle",          nx_DestroyVehicle);
    lua_register(L, "nx_SetVehiclePos",           nx_SetVehiclePos);
    lua_register(L, "nx_GetVehiclePos",           nx_GetVehiclePos);
    lua_register(L, "nx_SetVehicleZAngle",        nx_SetVehicleZAngle);
    lua_register(L, "nx_GetVehicleZAngle",        nx_GetVehicleZAngle);
    lua_register(L, "nx_SetVehicleHealth",        nx_SetVehicleHealth);
    lua_register(L, "nx_GetVehicleHealth",        nx_GetVehicleHealth);
    lua_register(L, "nx_RepairVehicle",           nx_RepairVehicle);
    lua_register(L, "nx_SetVehicleVelocity",      nx_SetVehicleVelocity);
    lua_register(L, "nx_GetVehicleVelocity",      nx_GetVehicleVelocity);
    lua_register(L, "nx_GetPlayerVehicleID",      nx_GetPlayerVehicleID);
    lua_register(L, "nx_IsPlayerInVehicle",       nx_IsPlayerInVehicle);
    lua_register(L, "nx_IsPlayerInAnyVehicle",    nx_IsPlayerInAnyVehicle);
    lua_register(L, "nx_ChangeVehicleColor",      nx_ChangeVehicleColor);
    lua_register(L, "nx_SetVehicleNumberPlate",   nx_SetVehicleNumberPlate);
    // --- Pickup ---
    lua_register(L, "nx_CreatePickup",            nx_CreatePickup);
    lua_register(L, "nx_DestroyPickup",           nx_DestroyPickup);
    // --- Object ---
    lua_register(L, "nx_CreateObject",            nx_CreateObject);
    lua_register(L, "nx_DestroyObject",           nx_DestroyObject);
    lua_register(L, "nx_SetObjectPos",            nx_SetObjectPos);
    lua_register(L, "nx_GetObjectPos",            nx_GetObjectPos);
    lua_register(L, "nx_SetObjectRot",            nx_SetObjectRot);
    lua_register(L, "nx_MoveObject",              nx_MoveObject);
    lua_register(L, "nx_StopObject",              nx_StopObject);
    // --- 3D Text Label ---
    lua_register(L, "nx_Create3DTextLabel",       nx_Create3DTextLabel);
    lua_register(L, "nx_Delete3DTextLabel",       nx_Delete3DTextLabel);
    lua_register(L, "nx_Update3DTextLabelText",   nx_Update3DTextLabelText);
    // --- TextDraw ---
    lua_register(L, "nx_TextDrawCreate",          nx_TextDrawCreate);
    lua_register(L, "nx_TextDrawDestroy",         nx_TextDrawDestroy);
    lua_register(L, "nx_TextDrawShowForPlayer",   nx_TextDrawShowForPlayer);
    lua_register(L, "nx_TextDrawHideForPlayer",   nx_TextDrawHideForPlayer);
    lua_register(L, "nx_TextDrawShowForAll",      nx_TextDrawShowForAll);
    lua_register(L, "nx_TextDrawHideForAll",      nx_TextDrawHideForAll);
    lua_register(L, "nx_TextDrawSetString",       nx_TextDrawSetString);
    lua_register(L, "nx_TextDrawColor",           nx_TextDrawColor);
    lua_register(L, "nx_TextDrawBoxColor",        nx_TextDrawBoxColor);
    lua_register(L, "nx_TextDrawUseBox",          nx_TextDrawUseBox);
    lua_register(L, "nx_TextDrawLetterSize",      nx_TextDrawLetterSize);
    lua_register(L, "nx_TextDrawTextSize",        nx_TextDrawTextSize);
    lua_register(L, "nx_TextDrawAlignment",       nx_TextDrawAlignment);
    lua_register(L, "nx_TextDrawFont",            nx_TextDrawFont);
    lua_register(L, "nx_TextDrawSetShadow",       nx_TextDrawSetShadow);
    lua_register(L, "nx_TextDrawSetOutline",      nx_TextDrawSetOutline);
    lua_register(L, "nx_TextDrawSetProportional", nx_TextDrawSetProportional);
    lua_register(L, "nx_TextDrawSetSelectable",   nx_TextDrawSetSelectable);
    // --- Class ---
    lua_register(L, "nx_AddPlayerClass",          nx_AddPlayerClass);
    lua_register(L, "nx_SetSpawnInfo",            nx_SetSpawnInfo);
    // --- Checkpoint ---
    lua_register(L, "nx_SetPlayerCheckpoint",     nx_SetPlayerCheckpoint);
    lua_register(L, "nx_DisablePlayerCheckpoint", nx_DisablePlayerCheckpoint);
    lua_register(L, "nx_IsPlayerInCheckpoint",    nx_IsPlayerInCheckpoint);
    // --- GangZone ---
    lua_register(L, "nx_GangZoneCreate",              nx_GangZoneCreate);
    lua_register(L, "nx_GangZoneDestroy",             nx_GangZoneDestroy);
    lua_register(L, "nx_GangZoneShowForPlayer",       nx_GangZoneShowForPlayer);
    lua_register(L, "nx_GangZoneHideForPlayer",       nx_GangZoneHideForPlayer);
    lua_register(L, "nx_GangZoneFlashForPlayer",      nx_GangZoneFlashForPlayer);
    lua_register(L, "nx_GangZoneStopFlashForPlayer",  nx_GangZoneStopFlashForPlayer);
    lua_register(L, "nx_GangZoneShowForAll",          nx_GangZoneShowForAll);
    lua_register(L, "nx_GangZoneHideForAll",          nx_GangZoneHideForAll);
    // --- Server ---
    lua_register(L, "nx_SetGameModeText",         nx_SetGameModeText);
    lua_register(L, "nx_GetTickCount",            nx_GetTickCount);
    lua_register(L, "nx_print",                   nx_print);
    lua_register(L, "nx_GameModeExit",            nx_GameModeExit);
    lua_register(L, "nx_SetWorldTime",            nx_SetWorldTime);
    lua_register(L, "nx_SetWeather",              nx_SetWeather);
    lua_register(L, "nx_SetGravity",              nx_SetGravity);
    lua_register(L, "nx_GameTextForAll",          nx_GameTextForAll);
    lua_register(L, "nx_GetMaxPlayers",           nx_GetMaxPlayers);
    // --- Hot Reload ---
    lua_register(L, "nx_listFiles",    nx_listFiles);
    lua_register(L, "nx_isDirectory",  nx_isDirectory);
    lua_register(L, "nx_reloadScript", nx_reloadScript);
    lua_register(L, "nx_getScriptDir", nx_getScriptDir);
    // --- Timer ---
    lua_register(L, "nx_SetTimer",               nx_SetTimer);
    lua_register(L, "nx_KillTimer",              nx_KillTimer);
    // --- Dialog ---
    lua_register(L, "nx_ShowPlayerDialog",        nx_ShowPlayerDialog);
    lua_register(L, "nx_HidePlayerDialog",        nx_HidePlayerDialog);

    // --- Dialog style constants ---
    lua_pushinteger(L, 0); lua_setglobal(L, "DIALOG_STYLE_MSGBOX");
    lua_pushinteger(L, 1); lua_setglobal(L, "DIALOG_STYLE_INPUT");
    lua_pushinteger(L, 2); lua_setglobal(L, "DIALOG_STYLE_LIST");
    lua_pushinteger(L, 3); lua_setglobal(L, "DIALOG_STYLE_PASSWORD");
    lua_pushinteger(L, 4); lua_setglobal(L, "DIALOG_STYLE_TABLIST");
    lua_pushinteger(L, 5); lua_setglobal(L, "DIALOG_STYLE_TABLIST_HEADERS");
    lua_pushinteger(L, 0); lua_setglobal(L, "DIALOG_RESPONSE_RIGHT");
    lua_pushinteger(L, 1); lua_setglobal(L, "DIALOG_RESPONSE_LEFT");

    // --- Player state constants ---
    lua_pushinteger(L, 0); lua_setglobal(L, "PLAYER_STATE_NONE");
    lua_pushinteger(L, 1); lua_setglobal(L, "PLAYER_STATE_ONFOOT");
    lua_pushinteger(L, 2); lua_setglobal(L, "PLAYER_STATE_DRIVER");
    lua_pushinteger(L, 3); lua_setglobal(L, "PLAYER_STATE_PASSENGER");
    lua_pushinteger(L, 7); lua_setglobal(L, "PLAYER_STATE_WASTED");
    lua_pushinteger(L, 8); lua_setglobal(L, "PLAYER_STATE_SPAWNED");
    lua_pushinteger(L, 9); lua_setglobal(L, "PLAYER_STATE_SPECTATING");

    // --- Weapon constants ---
    lua_pushinteger(L, 0);  lua_setglobal(L, "WEAPON_FIST");
    lua_pushinteger(L, 22); lua_setglobal(L, "WEAPON_COLT45");
    lua_pushinteger(L, 24); lua_setglobal(L, "WEAPON_DEAGLE");
    lua_pushinteger(L, 25); lua_setglobal(L, "WEAPON_SHOTGUN");
    lua_pushinteger(L, 28); lua_setglobal(L, "WEAPON_UZI");
    lua_pushinteger(L, 29); lua_setglobal(L, "WEAPON_MP5");
    lua_pushinteger(L, 30); lua_setglobal(L, "WEAPON_AK47");
    lua_pushinteger(L, 31); lua_setglobal(L, "WEAPON_M4");
    lua_pushinteger(L, 34); lua_setglobal(L, "WEAPON_SNIPER");
    lua_pushinteger(L, 46); lua_setglobal(L, "WEAPON_PARACHUTE");

    // --- Key constants ---
    lua_pushinteger(L, 1);   lua_setglobal(L, "KEY_ACTION");
    lua_pushinteger(L, 2);   lua_setglobal(L, "KEY_CROUCH");
    lua_pushinteger(L, 4);   lua_setglobal(L, "KEY_FIRE");
    lua_pushinteger(L, 8);   lua_setglobal(L, "KEY_SPRINT");
    lua_pushinteger(L, 32);  lua_setglobal(L, "KEY_JUMP");
    lua_pushinteger(L, 128); lua_setglobal(L, "KEY_HANDBRAKE");

    // --- Misc constants ---
    lua_pushinteger(L, 0xFFFF); lua_setglobal(L, "INVALID_PLAYER_ID");
    lua_pushinteger(L, 0xFFFF); lua_setglobal(L, "INVALID_VEHICLE_ID");
    lua_pushinteger(L, 0xFFFF); lua_setglobal(L, "INVALID_OBJECT_ID");
    lua_pushinteger(L, -1);     lua_setglobal(L, "INVALID_PICKUP_ID");
    lua_pushinteger(L, 255);    lua_setglobal(L, "NO_TEAM");
}
