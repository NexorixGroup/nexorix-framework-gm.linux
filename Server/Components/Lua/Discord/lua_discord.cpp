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

// discord_sendEmbed(channel_id, embed_table)
// embed_table fields (all optional except title or description):
//   title       string
//   description string
//   color       integer  (decimal, ex: 0x00FF00 = 65280)
//   footer      string
//   fields      array of {name, value, inline}
//   thumbnail   string (url)
//   image       string (url)
//   author      string
static int l_sendEmbed(lua_State* L)
{
    DiscordComponent* dc = getDiscord(L);
    if (!dc) return 0;
    const char* channel = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    // Build embed JSON from Lua table
    std::string json = "{";

    // title
    lua_getfield(L, 2, "title");
    if (lua_isstring(L, -1)) {
        std::string v = lua_tostring(L, -1);
        // escape quotes
        std::string esc; for (char c : v) { if (c=='"') esc+="\\\""; else esc+=c; }
        json += "\"title\":\"" + esc + "\",";
    }
    lua_pop(L, 1);

    // description
    lua_getfield(L, 2, "description");
    if (lua_isstring(L, -1)) {
        std::string v = lua_tostring(L, -1);
        std::string esc; for (char c : v) { if (c=='"') esc+="\\\""; else if (c=='\n') esc+="\\n"; else esc+=c; }
        json += "\"description\":\"" + esc + "\",";
    }
    lua_pop(L, 1);

    // color
    lua_getfield(L, 2, "color");
    if (lua_isinteger(L, -1)) {
        json += "\"color\":" + std::to_string((int)lua_tointeger(L, -1)) + ",";
    } else if (lua_isnumber(L, -1)) {
        json += "\"color\":" + std::to_string((int)lua_tonumber(L, -1)) + ",";
    }
    lua_pop(L, 1);

    // author
    lua_getfield(L, 2, "author");
    if (lua_isstring(L, -1)) {
        std::string v = lua_tostring(L, -1);
        std::string esc; for (char c : v) { if (c=='"') esc+="\\\""; else esc+=c; }
        json += "\"author\":{\"name\":\"" + esc + "\"},";
    }
    lua_pop(L, 1);

    // thumbnail
    lua_getfield(L, 2, "thumbnail");
    if (lua_isstring(L, -1)) {
        std::string v = lua_tostring(L, -1);
        json += "\"thumbnail\":{\"url\":\"" + v + "\"},";
    }
    lua_pop(L, 1);

    // image
    lua_getfield(L, 2, "image");
    if (lua_isstring(L, -1)) {
        std::string v = lua_tostring(L, -1);
        json += "\"image\":{\"url\":\"" + v + "\"},";
    }
    lua_pop(L, 1);

    // fields: array of {name, value, inline}
    lua_getfield(L, 2, "fields");
    if (lua_istable(L, -1)) {
        json += "\"fields\":[";
        int n = (int)lua_rawlen(L, -1);
        for (int i = 1; i <= n; i++) {
            lua_rawgeti(L, -1, i);
            if (lua_istable(L, -1)) {
                if (i > 1) json += ",";
                json += "{";

                lua_getfield(L, -1, "name");
                std::string fname = lua_isstring(L, -1) ? lua_tostring(L, -1) : "";
                std::string fesc; for (char c : fname) { if (c=='"') fesc+="\\\""; else fesc+=c; }
                json += "\"name\":\"" + fesc + "\",";
                lua_pop(L, 1);

                lua_getfield(L, -1, "value");
                std::string fval = lua_isstring(L, -1) ? lua_tostring(L, -1) : "";
                std::string vesc; for (char c : fval) { if (c=='"') vesc+="\\\""; else if (c=='\n') vesc+="\\n"; else vesc+=c; }
                json += "\"value\":\"" + vesc + "\",";
                lua_pop(L, 1);

                lua_getfield(L, -1, "inline");
                bool finline = lua_toboolean(L, -1) != 0;
                json += "\"inline\":" + std::string(finline ? "true" : "false");
                lua_pop(L, 1);

                json += "}";
            }
            lua_pop(L, 1);
        }
        json += "],";
    }
    lua_pop(L, 1);

    // footer
    lua_getfield(L, 2, "footer");
    if (lua_isstring(L, -1)) {
        std::string v = lua_tostring(L, -1);
        std::string esc; for (char c : v) { if (c=='"') esc+="\\\""; else esc+=c; }
        json += "\"footer\":{\"text\":\"" + esc + "\"},";
    }
    lua_pop(L, 1);

    // Remove trailing comma if present
    if (!json.empty() && json.back() == ',')
        json.pop_back();
    json += "}";

    dc->sendEmbed(channel, json);
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

    // Store channel_id as a global string (accessible via discord_getChannelId)
    lua_pushstring(L, dc->getChannelId().c_str());
    lua_setfield(L, LUA_REGISTRYINDEX, "__nx_discord_channel");

    // Also expose as a Lua global for convenience
    lua_pushstring(L, dc->getChannelId().c_str());
    lua_setglobal(L, "DISCORD_CHANNEL_ID");

    // Register functions
    lua_register(L, "discord_sendMessage",     l_sendMessage);
    lua_register(L, "discord_sendEmbed",       l_sendEmbed);
    lua_register(L, "discord_setStatus",       l_setStatus);
    lua_register(L, "discord_onReady",         l_onReady);
    lua_register(L, "discord_onMessage",       l_onMessage);
    lua_register(L, "discord_registerCommand", l_registerCommand);
    lua_register(L, "discord_getChannelId",    l_getChannelId);
}
