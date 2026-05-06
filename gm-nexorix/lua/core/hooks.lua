--[[
    ============================================================================
    Nexorix Framework - Hook System
    ============================================================================
    Gerencia a execução múltipla de funções em um mesmo callback sem conflitos.
    Permite definir prioridades de execução (HIGHEST até LOWEST).
    ============================================================================
]]

-- Prioridades de Hook
NX.HOOK_PRIORITY = {
    HIGHEST = 1,
    HIGH    = 2,
    NORMAL  = 3,
    LOW     = 4,
    LOWEST  = 5,
}

-- Armazenamento interno
local _hooks   = {}
local _hook_id = 0
local _hook_map = {}

-- Lista de callbacks gerenciados
local _callbacks = {
    "OnGameModeInit", "OnGameModeExit",
    "OnPlayerConnect", "OnPlayerDisconnect",
    "OnPlayerSpawn", "OnPlayerDeath",
    "OnDialogResponse", "OnTick",
    "OnPlayerText", "OnPlayerCommand", "OnPlayerUpdate",
}

-- Defaults de retorno por callback
local _defaults = {
    OnPlayerText    = true,
    OnPlayerUpdate  = true,
    OnPlayerCommand = false,
}

-- ============================================================
--  NX.Hook(callback, fn, priority) -> id
-- ============================================================
function NX.Hook(callback, fn, priority)
    assert(type(callback) == "string", "NX.Hook: callback deve ser string")
    assert(type(fn) == "function",     "NX.Hook: fn deve ser function")
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
    if not inserted then
        table.insert(list, { id=id, fn=fn, priority=priority })
    end

    _hook_map[id] = callback
    return id
end

-- ============================================================
--  NX.UnHook(id)
-- ============================================================
function NX.UnHook(hook_id)
    local callback = _hook_map[hook_id]
    if not callback then return end
    local list = _hooks[callback]
    if not list then return end
    for i, entry in ipairs(list) do
        if entry.id == hook_id then
            table.remove(list, i)
            break
        end
    end
    _hook_map[hook_id] = nil
end

-- ============================================================
--  NX._dispatch(callback, ...) -> resultado
-- ============================================================
function NX._dispatch(callback, ...)
    local list   = _hooks[callback]
    local result = _defaults[callback]
    local stop_on_true = (callback == "OnPlayerCommand")

    if not list or #list == 0 then return result end

    for _, entry in ipairs(list) do
        local ok, ret = pcall(entry.fn, ...)
        if not ok then
            nx_print("[NX.Hook] Erro em hook de " .. callback .. ": " .. tostring(ret))
        else
            if ret == false then
                return false
            elseif ret == true then
                result = true
                if stop_on_true then return true end
            end
        end
    end

    return result
end

-- ============================================================
--  NX._resetHooks() — limpa tudo (usado pelo hot reload)
-- ============================================================
function NX._resetHooks()
    for k in pairs(_hooks)    do _hooks[k]    = {} end
    for k in pairs(_hook_map) do _hook_map[k] = nil end
    _hook_id = 0
    for _, cb in ipairs(_callbacks) do
        _G[cb] = function(...) return NX._dispatch(cb, ...) end
    end
    -- Limpa o cache do require para que os módulos sejam reexecutados
    for _, mod in ipairs({
        "core/init", "core/hooks", "core/hotreload",
        "systems/discord", "systems/auth", "systems/admin",
    }) do
        package.loaded[mod] = nil
    end
    nx_print("[NX] Hooks resetados.")
end

-- ============================================================
--  NX.HookList(callback) -> string  (debug)
-- ============================================================
function NX.HookList(callback)
    local list = _hooks[callback]
    if not list or #list == 0 then
        return "[NX.Hook] Nenhum hook em " .. callback
    end
    local out = "[NX.Hook] " .. callback .. " (" .. #list .. " hooks):\n"
    for i, entry in ipairs(list) do
        out = out .. "  [" .. i .. "] id=" .. entry.id .. " priority=" .. entry.priority .. "\n"
    end
    return out
end

-- ============================================================
--  Instala os dispatchers globais
-- ============================================================
for _, cb in ipairs(_callbacks) do
    _G[cb] = function(...) return NX._dispatch(cb, ...) end
end

nx_print("[NX] Hook system carregado.")
