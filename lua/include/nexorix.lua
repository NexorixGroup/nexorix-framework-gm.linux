-- ============================================================
--  Nexorix "Parent Include"
--  Unified entry point for Lua developers.
-- ============================================================

NX = NX or {}
NX.VERSION = "1.0.0"

NX.INVALID_PLAYER_ID = 0xFFFF
NX.MAX_PLAYERS       = 1000

NX.COLOR = {
    WHITE  = 0xFFFFFFFF,
    BLACK  = 0x000000FF,
    GREEN  = 0x44FF44FF,
    RED    = 0xFF4444FF,
    YELLOW = 0xFFFF44FF,
    BLUE   = 0x4488FFFF,
}

NX.PLAYER_STATE = {
    NONE=0, ONFOOT=1, DRIVER=2, PASSENGER=3,
    WASTED=7, SPAWNED=8, SPECTATING=9,
}

NX.KEY = {
    ACTION=1, CROUCH=2, FIRE=4, SPRINT=8, JUMP=32, HANDBRAKE=128,
}

NX.HOOK_PRIORITY = { HIGHEST = 1, HIGH = 2, NORMAL = 3, LOW = 4, LOWEST = 5 }

local _hooks   = {}
local _hook_id = 0
local _hook_map = {}

local _callbacks = {
    "OnGameModeInit", "OnGameModeExit",
    "OnPlayerConnect", "OnPlayerDisconnect",
    "OnPlayerSpawn", "OnPlayerDeath",
    "OnPlayerRequestClass",
    "OnDialogResponse", "OnTick",
    "OnPlayerText", "OnPlayerCommand", "OnPlayerUpdate",
    "OnPlayerKeyStateChange", "OnPlayerEnterVehicle",
    "OnPlayerExitVehicle", "OnPlayerStateChange",
    "OnPlayerPickUpPickup", "OnPlayerClickMap"
}

local _defaults = {
    OnPlayerText    = true,
    OnPlayerUpdate  = true,
    OnPlayerCommand = false,
}

function NX.Hook(callback, fn, priority)
    priority = priority or NX.HOOK_PRIORITY.NORMAL
    _hook_id = _hook_id + 1
    local id = _hook_id
    if not _hooks[callback] then _hooks[callback] = {} end
    local list, inserted = _hooks[callback], false
    for i, entry in ipairs(list) do
        if priority < entry.priority then
            table.insert(list, i, { id=id, fn=fn, priority=priority })
            inserted = true
            break
        end
    end
    if not inserted then table.insert(list, { id=id, fn=fn, priority=priority }) end
    _hook_map[id] = callback
    return id
end

function NX.UnHook(hook_id)
    local callback = _hook_map[hook_id]
    if not callback then return end
    local list = _hooks[callback]
    if not list then return end
    for i, entry in ipairs(list) do
        if entry.id == hook_id then table.remove(list, i); break end
    end
    _hook_map[hook_id] = nil
end

function NX._dispatch(callback, ...)
    local list   = _hooks[callback]
    local result = _defaults[callback]
    local stop_on_true = (callback == "OnPlayerCommand")
    if not list or #list == 0 then return result end
    for _, entry in ipairs(list) do
        local ok, ret = pcall(entry.fn, ...)
        if not ok then
            nx_print("[NX.Hook] Error in " .. callback .. ": " .. tostring(ret))
        else
            if ret == false then return false
            elseif ret == true then result = true; if stop_on_true then return true end end
        end
    end
    return result
end

for _, cb in ipairs(_callbacks) do
    _G[cb] = function(...) return NX._dispatch(cb, ...) end
end

local _commands = {}

function NX.RegisterCommand(name, fn)
    _commands[name:lower()] = fn
end

NX.Hook("OnPlayerCommand", function(playerid, cmdtext)
    local text = cmdtext:sub(2)
    local name, params = text:match("^(%S+)%s*(.*)$")
    if not name then return false end
    local fn = _commands[name:lower()]
    if fn then fn(playerid, params); return true end
    return false
end, NX.HOOK_PRIORITY.HIGHEST)

nx_print("[NX] Nexorix Parent Include Loaded.")
