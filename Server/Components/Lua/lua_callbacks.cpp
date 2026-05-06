/*
 * Nexorix Lua Component — Callback Documentation
 *
 * This file documents the Lua callbacks dispatched by LuaComponent.
 * The actual dispatch logic lives in lua_main.cpp.
 *
 * Callbacks available to Lua scripts:
 *
 *   OnGameModeInit()
 *       Called once after all scripts are loaded.
 *
 *   OnGameModeExit()
 *       Called when the server shuts down or the component is unloaded.
 *
 *   OnPlayerConnect(playerid)
 *       Called when a player connects.
 *
 *   OnPlayerDisconnect(playerid, reason)
 *       Called when a player disconnects.
 *       reason: 0 = timeout, 1 = quit, 2 = kick/ban
 *
 *   OnPlayerSpawn(playerid)
 *       Called when a player spawns.
 *
 *   OnPlayerDeath(playerid, killerid, reason)
 *       Called when a player dies.
 *       killerid: 0xFFFF if no killer (suicide/environment)
 *       reason: weapon ID
 *
 *   OnPlayerText(playerid, text) -> bool
 *       Called when a player sends a chat message.
 *       Return false to suppress the message.
 *
 *   OnPlayerCommand(playerid, cmdtext) -> bool
 *       Called when a player types a command (starts with '/').
 *       Return true if the command was handled, false otherwise.
 *
 *   OnPlayerUpdate(playerid) -> bool
 *       Called on every player sync update.
 *       Return false to reject the update.
 *
 *   OnTick(elapsed)
 *       Called every server tick.
 *       elapsed: microseconds since last tick (lua_Integer)
 *
 * Example usage:
 *
 *   function OnGameModeInit()
 *       nx_SetGameModeText("My Server")
 *       nx_print("[Lua] Server started!")
 *   end
 *
 *   function OnPlayerConnect(playerid)
 *       local name = nx_GetPlayerName(playerid)
 *       nx_SendClientMessageToAll(0xAAAAAAFF, "[+] " .. name .. " joined.")
 *   end
 *
 *   function OnPlayerCommand(playerid, cmdtext)
 *       if cmdtext == "/help" then
 *           nx_SendClientMessage(playerid, 0xFFFF44FF, "Commands: /help")
 *           return true
 *       end
 *       return false
 *   end
 */

// This file is intentionally left without executable code.
// All dispatch logic is in lua_main.cpp.
