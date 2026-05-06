#pragma once

extern "C" {
#include <lua.h>
}

class LuaComponent;

/// Register all nx_* functions into the Lua state
void lua_register_api(lua_State* L, LuaComponent* component);
