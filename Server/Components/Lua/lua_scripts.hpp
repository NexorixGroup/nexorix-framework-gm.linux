#pragma once

#include <string>
#include <vector>

extern "C" {
#include <lua.h>
}

/// Manages loading and reloading of Lua scripts
class LuaScriptManager
{
public:
    /// Load all .lua files from the given directory (alphabetical order)
    void loadAll(lua_State* L, const std::string& directory);

    /// Reload all previously loaded scripts
    void reloadAll(lua_State* L);

    /// Load a single script file; returns false on error
    bool loadFile(lua_State* L, const std::string& path);

    const std::vector<std::string>& getLoadedFiles() const { return loadedFiles_; }

private:
    std::vector<std::string> loadedFiles_;
    std::string directory_;
};
