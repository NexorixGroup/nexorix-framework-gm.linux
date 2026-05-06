#pragma once

extern "C" {
#include <lua.h>
}

class DiscordComponent;

/// Register all discord_* functions into the Lua state
void lua_register_discord(lua_State* L, DiscordComponent* dc);
