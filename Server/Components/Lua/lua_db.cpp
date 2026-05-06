/*
 * Nexorix Lua Component - SQLite API
 *
 * Exposes nx_db_* functions to Lua scripts.
 *
 * Usage from Lua:
 *
 *   local db = nx_db_open("mydb.db")
 *
 *   nx_db_exec(db, "CREATE TABLE IF NOT EXISTS t (id INTEGER PRIMARY KEY, name TEXT)")
 *
 *   nx_db_exec(db, "INSERT INTO t (name) VALUES (?)", "Rick")
 *
 *   local rows = nx_db_query(db, "SELECT * FROM t WHERE name = ?", "Rick")
 *   for _, row in ipairs(rows) do
 *       nx_print(row.id .. " " .. row.name)
 *   end
 *
 *   nx_db_close(db)
 */

#include "lua_db.hpp"
#include <sqlite3.h>
#include <cstring>
#include <cstdio>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

// ---------------------------------------------------------------------------
// Metatable name for the db handle userdata
// ---------------------------------------------------------------------------
#define LUA_DB_MT    "nx_db_handle"
#define LUA_STMT_MT  "nx_stmt_handle"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static sqlite3* check_db(lua_State* L, int idx)
{
    sqlite3** pp = static_cast<sqlite3**>(luaL_checkudata(L, idx, LUA_DB_MT));
    if (!*pp)
        luaL_error(L, "nx_db: attempt to use a closed database");
    return *pp;
}

static sqlite3_stmt* check_stmt(lua_State* L, int idx)
{
    sqlite3_stmt** pp = static_cast<sqlite3_stmt**>(luaL_checkudata(L, idx, LUA_STMT_MT));
    if (!*pp)
        luaL_error(L, "nx_db: attempt to use a finalized statement");
    return *pp;
}

// Bind Lua values at stack positions [first, top] to a prepared statement.
// Binding starts at SQLite index 1.
static void bind_params(lua_State* L, sqlite3_stmt* stmt, int first)
{
    int top = lua_gettop(L);
    int si  = 1; // sqlite bind index
    for (int i = first; i <= top; ++i, ++si)
    {
        int t = lua_type(L, i);
        if (t == LUA_TNIL)
        {
            sqlite3_bind_null(stmt, si);
        }
        else if (t == LUA_TBOOLEAN)
        {
            sqlite3_bind_int(stmt, si, lua_toboolean(L, i));
        }
        else if (t == LUA_TNUMBER)
        {
            if (lua_isinteger(L, i))
                sqlite3_bind_int64(stmt, si, lua_tointeger(L, i));
            else
                sqlite3_bind_double(stmt, si, lua_tonumber(L, i));
        }
        else
        {
            size_t len = 0;
            const char* s = lua_tolstring(L, i, &len);
            sqlite3_bind_text(stmt, si, s, static_cast<int>(len), SQLITE_TRANSIENT);
        }
    }
}

// Push a single column value from a statement onto the Lua stack
static void push_column(lua_State* L, sqlite3_stmt* stmt, int col)
{
    switch (sqlite3_column_type(stmt, col))
    {
    case SQLITE_INTEGER:
        lua_pushinteger(L, static_cast<lua_Integer>(sqlite3_column_int64(stmt, col)));
        break;
    case SQLITE_FLOAT:
        lua_pushnumber(L, sqlite3_column_double(stmt, col));
        break;
    case SQLITE_TEXT:
        lua_pushstring(L, reinterpret_cast<const char*>(sqlite3_column_text(stmt, col)));
        break;
    case SQLITE_BLOB:
    {
        int bytes = sqlite3_column_bytes(stmt, col);
        lua_pushlstring(L, static_cast<const char*>(sqlite3_column_blob(stmt, col)), bytes);
        break;
    }
    case SQLITE_NULL:
    default:
        lua_pushnil(L);
        break;
    }
}

// ---------------------------------------------------------------------------
// nx_db_open(path) -> db_handle
// ---------------------------------------------------------------------------
static int nx_db_open(lua_State* L)
{
    const char* path = luaL_checkstring(L, 1);

    sqlite3** pp = static_cast<sqlite3**>(lua_newuserdata(L, sizeof(sqlite3*)));
    *pp = nullptr;

    luaL_setmetatable(L, LUA_DB_MT);

    int rc = sqlite3_open(path, pp);
    if (rc != SQLITE_OK)
    {
        const char* err = sqlite3_errmsg(*pp);
        sqlite3_close(*pp);
        *pp = nullptr;
        luaL_error(L, "nx_db_open: %s", err);
    }

    // Enable WAL mode for better concurrent read performance
    sqlite3_exec(*pp, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    // Foreign keys on by default
    sqlite3_exec(*pp, "PRAGMA foreign_keys=ON;", nullptr, nullptr, nullptr);

    return 1;
}

// ---------------------------------------------------------------------------
// nx_db_close(db)
// ---------------------------------------------------------------------------
static int nx_db_close(lua_State* L)
{
    sqlite3** pp = static_cast<sqlite3**>(luaL_checkudata(L, 1, LUA_DB_MT));
    if (*pp)
    {
        sqlite3_close(*pp);
        *pp = nullptr;
    }
    return 0;
}

// GC metamethod for db handle
static int db_gc(lua_State* L)
{
    return nx_db_close(L);
}

// ---------------------------------------------------------------------------
// nx_db_exec(db, sql [, param1, param2, ...]) -> true | error
//
// Executes a statement that returns no rows (INSERT, UPDATE, DELETE, CREATE…).
// Returns true on success, raises a Lua error on failure.
// ---------------------------------------------------------------------------
static int nx_db_exec(lua_State* L)
{
    sqlite3*    db  = check_db(L, 1);
    const char* sql = luaL_checkstring(L, 2);

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
        luaL_error(L, "nx_db_exec prepare: %s", sqlite3_errmsg(db));

    // Bind optional parameters (stack positions 3..top)
    bind_params(L, stmt, 3);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE && rc != SQLITE_ROW)
        luaL_error(L, "nx_db_exec: %s", sqlite3_errmsg(db));

    lua_pushboolean(L, 1);
    return 1;
}

// ---------------------------------------------------------------------------
// nx_db_query(db, sql [, param1, param2, ...]) -> table of rows
//
// Each row is a table with both integer keys (1-based) and column-name keys.
// Example: rows[1].name  or  rows[1][1]
// ---------------------------------------------------------------------------
static int nx_db_query(lua_State* L)
{
    sqlite3*    db  = check_db(L, 1);
    const char* sql = luaL_checkstring(L, 2);

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
        luaL_error(L, "nx_db_query prepare: %s", sqlite3_errmsg(db));

    bind_params(L, stmt, 3);

    // Result table
    lua_newtable(L);
    int row_idx = 1;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        int cols = sqlite3_column_count(stmt);

        lua_newtable(L); // row table

        for (int c = 0; c < cols; ++c)
        {
            push_column(L, stmt, c);

            // integer key (1-based)
            lua_rawseti(L, -2, c + 1);

            // duplicate value for named key
            push_column(L, stmt, c);
            const char* col_name = sqlite3_column_name(stmt, c);
            lua_setfield(L, -2, col_name);
        }

        lua_rawseti(L, -2, row_idx++);
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
        luaL_error(L, "nx_db_query: %s", sqlite3_errmsg(db));

    return 1; // returns the result table
}

// ---------------------------------------------------------------------------
// nx_db_last_insert_id(db) -> integer
// ---------------------------------------------------------------------------
static int nx_db_last_insert_id(lua_State* L)
{
    sqlite3* db = check_db(L, 1);
    lua_pushinteger(L, static_cast<lua_Integer>(sqlite3_last_insert_rowid(db)));
    return 1;
}

// ---------------------------------------------------------------------------
// nx_db_changes(db) -> integer
// Number of rows affected by the last INSERT/UPDATE/DELETE
// ---------------------------------------------------------------------------
static int nx_db_changes(lua_State* L)
{
    sqlite3* db = check_db(L, 1);
    lua_pushinteger(L, sqlite3_changes(db));
    return 1;
}

// ---------------------------------------------------------------------------
// Prepared statement API
// ---------------------------------------------------------------------------

// nx_db_prepare(db, sql) -> stmt_handle
static int nx_db_prepare(lua_State* L)
{
    sqlite3*    db  = check_db(L, 1);
    const char* sql = luaL_checkstring(L, 2);

    sqlite3_stmt** pp = static_cast<sqlite3_stmt**>(lua_newuserdata(L, sizeof(sqlite3_stmt*)));
    *pp = nullptr;

    luaL_setmetatable(L, LUA_STMT_MT);

    int rc = sqlite3_prepare_v2(db, sql, -1, pp, nullptr);
    if (rc != SQLITE_OK)
        luaL_error(L, "nx_db_prepare: %s", sqlite3_errmsg(db));

    return 1;
}

// nx_db_bind(stmt, index, value)
// Bind a single value to a prepared statement (1-based index)
static int nx_db_bind(lua_State* L)
{
    sqlite3_stmt* stmt = check_stmt(L, 1);
    int idx = static_cast<int>(luaL_checkinteger(L, 2));

    int t = lua_type(L, 3);
    int rc = SQLITE_OK;

    if (t == LUA_TNIL)
        rc = sqlite3_bind_null(stmt, idx);
    else if (t == LUA_TBOOLEAN)
        rc = sqlite3_bind_int(stmt, idx, lua_toboolean(L, 3));
    else if (t == LUA_TNUMBER)
    {
        if (lua_isinteger(L, 3))
            rc = sqlite3_bind_int64(stmt, idx, lua_tointeger(L, 3));
        else
            rc = sqlite3_bind_double(stmt, idx, lua_tonumber(L, 3));
    }
    else
    {
        size_t len = 0;
        const char* s = lua_tolstring(L, 3, &len);
        rc = sqlite3_bind_text(stmt, idx, s, static_cast<int>(len), SQLITE_TRANSIENT);
    }

    if (rc != SQLITE_OK)
        luaL_error(L, "nx_db_bind: error code %d", rc);

    lua_pushboolean(L, 1);
    return 1;
}

// nx_db_step(stmt) -> "row" | "done" | "error"
static int nx_db_step(lua_State* L)
{
    sqlite3_stmt* stmt = check_stmt(L, 1);
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
        lua_pushstring(L, "row");
    else if (rc == SQLITE_DONE)
        lua_pushstring(L, "done");
    else
        lua_pushstring(L, "error");
    return 1;
}

// nx_db_column(stmt, index) -> value   (0-based column index)
static int nx_db_column(lua_State* L)
{
    sqlite3_stmt* stmt = check_stmt(L, 1);
    int col = static_cast<int>(luaL_checkinteger(L, 2));
    push_column(L, stmt, col);
    return 1;
}

// nx_db_reset(stmt)
static int nx_db_reset(lua_State* L)
{
    sqlite3_stmt* stmt = check_stmt(L, 1);
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
    lua_pushboolean(L, 1);
    return 1;
}

// nx_db_finalize(stmt)
static int nx_db_finalize(lua_State* L)
{
    sqlite3_stmt** pp = static_cast<sqlite3_stmt**>(luaL_checkudata(L, 1, LUA_STMT_MT));
    if (*pp)
    {
        sqlite3_finalize(*pp);
        *pp = nullptr;
    }
    return 0;
}

static int stmt_gc(lua_State* L)
{
    return nx_db_finalize(L);
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

void lua_register_db(lua_State* L)
{
    // Create metatable for db handles
    luaL_newmetatable(L, LUA_DB_MT);
    lua_pushcfunction(L, db_gc);
    lua_setfield(L, -2, "__gc");
    lua_pushstring(L, "nx_db_handle");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    // Create metatable for stmt handles
    luaL_newmetatable(L, LUA_STMT_MT);
    lua_pushcfunction(L, stmt_gc);
    lua_setfield(L, -2, "__gc");
    lua_pushstring(L, "nx_stmt_handle");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    // Register global functions
    lua_register(L, "nx_db_open",           nx_db_open);
    lua_register(L, "nx_db_close",          nx_db_close);
    lua_register(L, "nx_db_exec",           nx_db_exec);
    lua_register(L, "nx_db_query",          nx_db_query);
    lua_register(L, "nx_db_last_insert_id", nx_db_last_insert_id);
    lua_register(L, "nx_db_changes",        nx_db_changes);
    lua_register(L, "nx_db_prepare",        nx_db_prepare);
    lua_register(L, "nx_db_bind",           nx_db_bind);
    lua_register(L, "nx_db_step",           nx_db_step);
    lua_register(L, "nx_db_column",         nx_db_column);
    lua_register(L, "nx_db_reset",          nx_db_reset);
    lua_register(L, "nx_db_finalize",       nx_db_finalize);
}
