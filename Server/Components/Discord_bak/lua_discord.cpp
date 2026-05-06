/*
 * Nexorix Discord Component — Lua API
 *
 * Expoe as funcoes discord_* para scripts Lua.
 * Este arquivo e compilado junto com o componente Discord.
 *
 * Funcoes disponiveis em Lua:
 *
 *   discord_sendMessage(channel_id, message)
 *   discord_setStatus(text)
 *   discord_onReady(callback)
 *   discord_onMessage(callback)
 *   discord_registerCommand(name, callback)
 *   discord_getChannelId() -> string
 */

#include "lua_discord.hpp"
#include "discord_main.hpp"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

static DiscordComponent* getDiscord(lua_State* L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, "__nx_discord");
    DiscordComponent* dc = static_cast<DiscordComponent*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return dc;
}

// discord_sendMessage(channel_id, message)
static int l_sendMessage(lua_State* L)
{
    DiscordComponent* dc = getDiscord(L);
    if (!dc) return 0;
    const char* channel = luaL_checkstring(L, 1);
    const char* message = luaL_checkstring(L, 2);
    dc->sendMessage(channel, message);
    return 0;
}

// discord_setStatus(text)
static int l_setStatus(lua_State* L)
{
    DiscordComponent* dc = getDiscord(L);
    if (!dc) return 0;
    const char* text = luaL_checkstring(L, 1);
    dc->setStatus(text);
    return 0;
}

// discord_onReady(callback)
static int l_onReady(lua_State* L)
{
    DiscordComponent* dc = getDiscord(L);
    if (!dc) return 0;
    luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_pushvalue(L, 1);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    dc->setOnReadyRef(L, ref);
    return 0;
}

// discord_onMessage(callback)
// callback(user, message)
static int l_onMessage(lua_State* L)
{
    DiscordComponent* dc = getDiscord(L);
    if (!dc) return 0;
    luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_pushvalue(L, 1);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    dc->setOnMessageRef(L, ref);
    return 0;
}

// discord_registerCommand(name, callback)
// callback(user, args_table)
static int l_registerCommand(lua_State* L)
{
    DiscordComponent* dc = getDiscord(L);
    if (!dc) return 0;
    const char* name = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    lua_pushvalue(L, 2);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    dc->registerCommand(name, L, ref);
    return 0;
}

// discord_getChannelId() -> string
static int l_getChannelId(lua_State* L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, "__nx_discord_channel");
    if (lua_isstring(L, -1))
        return 1;
    lua_pop(L, 1);
    lua_pushstring(L, "");
    return 1;
}

void lua_register_discord(lua_State* L, DiscordComponent* dc)
{
    // Store component pointer
    lua_pushlightuserdata(L, dc);
    lua_setfield(L, LUA_REGISTRYINDEX, "__nx_discord");

    // Register functions
    lua_register(L, "discord_sendMessage",     l_sendMessage);
    lua_register(L, "discord_setStatus",       l_setStatus);
    lua_register(L, "discord_onReady",         l_onReady);
    lua_register(L, "discord_onMessage",       l_onMessage);
    lua_register(L, "discord_registerCommand", l_registerCommand);
    lua_register(L, "discord_getChannelId",    l_getChannelId);
}
