/*
 * Nexorix Lua Component — Script Manager
 */

#include "lua_scripts.hpp"
#include <ghc/filesystem.hpp>
#include <algorithm>
#include <cstdio>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

void LuaScriptManager::loadAll(lua_State* L, const std::string& directory)
{
    directory_ = directory;
    loadedFiles_.clear();

    ghc::filesystem::path dir(directory);
    if (!ghc::filesystem::exists(dir) || !ghc::filesystem::is_directory(dir))
    {
        fprintf(stderr, "[Lua] Script directory not found: %s\n", directory.c_str());
        return;
    }

    // Collect .lua files
    std::vector<std::string> files;
    for (const auto& entry : ghc::filesystem::directory_iterator(dir))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".lua")
        {
            files.push_back(entry.path().string());
        }
    }

    // Sort alphabetically so load order is deterministic
    std::sort(files.begin(), files.end());

    for (const auto& file : files)
    {
        if (loadFile(L, file))
        {
            loadedFiles_.push_back(file);
        }
    }
}

void LuaScriptManager::reloadAll(lua_State* L)
{
    // Re-run all previously loaded files in the same order
    std::vector<std::string> prev = loadedFiles_;
    loadedFiles_.clear();
    for (const auto& file : prev)
    {
        if (loadFile(L, file))
        {
            loadedFiles_.push_back(file);
        }
    }
}

bool LuaScriptManager::loadFile(lua_State* L, const std::string& path)
{
    int status = luaL_loadfile(L, path.c_str());
    if (status != LUA_OK)
    {
        fprintf(stderr, "[Lua] Syntax error in %s: %s\n", path.c_str(), lua_tostring(L, -1));
        lua_pop(L, 1);
        return false;
    }

    status = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (status != LUA_OK)
    {
        fprintf(stderr, "[Lua] Runtime error in %s: %s\n", path.c_str(), lua_tostring(L, -1));
        lua_pop(L, 1);
        return false;
    }

    fprintf(stdout, "[Lua] Loaded script: %s\n", path.c_str());
    return true;
}
