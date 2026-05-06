#pragma once

extern "C" {
#include <lua.h>
}

void lua_register_db(lua_State* L);
