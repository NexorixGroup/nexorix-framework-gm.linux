--[[
    ============================================================================
    Nexorix Framework - Hot Reload System
    ============================================================================
    Permite atualizar scripts em tempo real sem reiniciar o servidor.
    Comandos: /reload <arquivo>, /reload <pasta/>, /reload all
    ============================================================================
]]

-- ============================================================
--  Rastreia todos os scripts carregados (path relativo a lua/)
-- ============================================================
NX._loadedScripts = NX._loadedScripts or {}

-- ============================================================
--  TextDraw de Reload (Global)
-- ============================================================
NX.reloadTD = NX.reloadTD or -1

-- Inicializa a TextDraw se não existir
if NX.reloadTD == -1 then
    NX.reloadTD = nx_TextDrawCreate(320.0, 437.0, "_")
    nx_TextDrawAlignment(NX.reloadTD, 2)
    nx_TextDrawLetterSize(NX.reloadTD, 0.22, 1.1)
    nx_TextDrawColor(NX.reloadTD, 0xFFFFFFFF)
    nx_TextDrawSetOutline(NX.reloadTD, 1)
    nx_TextDrawSetProportional(NX.reloadTD, 1)
    nx_TextDrawFont(NX.reloadTD, 1)
end

-- Função para mostrar a mensagem no centro inferior
local function show_reload_td(text, ms)
    if NX.reloadTD == -1 then return end
    
    nx_TextDrawSetString(NX.reloadTD, text)
    nx_TextDrawShowForAll(NX.reloadTD)
    
    -- Gerencia o timer de ocultação
    if NX._reloadHideTimer then nx_KillTimer(NX._reloadHideTimer) end
    NX._reloadHideTimer = nx_SetTimer(function()
        nx_TextDrawHideForAll(NX.reloadTD)
        NX._reloadHideTimer = nil
    end, ms or 3000, false)
end

-- Registra um script como carregado
local function _trackScript(relPath)
    -- Normaliza separadores
    relPath = relPath:gsub("\\", "/")
    for _, v in ipairs(NX._loadedScripts) do
        if v == relPath then return end -- já registrado
    end
    table.insert(NX._loadedScripts, relPath)
end

-- Registra os scripts já carregados pelo servidor
-- (main.lua e os sistemas do core/init.lua)
_trackScript("main.lua")
_trackScript("core/init.lua")
_trackScript("core/hooks.lua")
_trackScript("core/hotreload.lua")

-- Registra sistemas carregados via require
local _orig_require = require
require = function(modname)
    local result = _orig_require(modname)
    -- Converte "systems/discord" -> "systems/discord.lua"
    local path = modname:gsub("%.", "/")
    if path:sub(-4) ~= ".lua" then path = path .. ".lua" end
    _trackScript(path)
    return result
end

-- ============================================================
--  reloadScript(relPath) -> ok (bool), err (string)
--  relPath: relativo ao diretório lua/
-- ============================================================
function reloadScript(relPath)
    relPath = relPath:gsub("\\", "/")
    -- Garante extensão .lua
    if relPath:sub(-4) ~= ".lua" then relPath = relPath .. ".lua" end

    local ok, err = nx_reloadScript(relPath)
    if ok then
        _trackScript(relPath)
        -- Reinstala o hook system se recarregou o core
        if relPath == "core/hooks.lua" or relPath == "core/init.lua" then
            nx_print("[HotReload] Core recarregado — hooks reinstalados.")
        end
    end
    return ok, err
end

-- ============================================================
--  reloadFolder(folderPath) -> count (int), errors (table)
--  folderPath: relativo ao diretório lua/, deve terminar com /
-- ============================================================
function reloadFolder(folderPath)
    folderPath = folderPath:gsub("\\", "/")
    if folderPath:sub(-1) ~= "/" then folderPath = folderPath .. "/" end

    local files = nx_listFiles(folderPath)
    if not files or #files == 0 then
        nx_print("[HotReload] Nenhum arquivo em: " .. folderPath)
        return 0, {}
    end

    -- Filtra e ordena apenas .lua (ignora _prefixo e subpastas)
    local luaFiles = {}
    for _, name in ipairs(files) do
        if name:sub(-4) == ".lua" and name:sub(1,1) ~= "_" then
            table.insert(luaFiles, name)
        end
    end
    table.sort(luaFiles)

    local count, errors = 0, {}
    for _, name in ipairs(luaFiles) do
        local fullRel = folderPath .. name
        local ok, err = reloadScript(fullRel)
        if ok then
            count = count + 1
        else
            table.insert(errors, fullRel .. ": " .. (err or "?"))
            nx_print("[HotReload] ERRO: " .. fullRel .. " — " .. (err or "?"))
        end
    end

    nx_print("[HotReload] Pasta '" .. folderPath .. "': " .. count .. " scripts recarregados.")
    return count, errors
end

-- ============================================================
--  reloadFolderRecursive(folderPath) -> count, errors
-- ============================================================
function reloadFolderRecursive(folderPath)
    folderPath = folderPath:gsub("\\", "/")
    if folderPath:sub(-1) ~= "/" then folderPath = folderPath .. "/" end

    local items = nx_listFiles(folderPath)
    local count, errors = 0, {}

    if not items then return 0, {} end

    -- Separa arquivos e subpastas
    local luaFiles, subDirs = {}, {}
    for _, name in ipairs(items) do
        if name:sub(-1) == "/" then
            table.insert(subDirs, name)
        elseif name:sub(-4) == ".lua" and name:sub(1,1) ~= "_" then
            table.insert(luaFiles, name)
        end
    end
    table.sort(luaFiles)
    table.sort(subDirs)

    -- Recarrega arquivos desta pasta
    for _, name in ipairs(luaFiles) do
        local fullRel = folderPath .. name
        local ok, err = reloadScript(fullRel)
        if ok then count = count + 1
        else table.insert(errors, fullRel .. ": " .. (err or "?")) end
    end

    -- Recursão nas subpastas
    for _, dir in ipairs(subDirs) do
        local c, e = reloadFolderRecursive(folderPath .. dir)
        count = count + c
        for _, err in ipairs(e) do table.insert(errors, err) end
    end

    return count, errors
end

-- ============================================================
--  reloadAll(caller_pid) -> count, errors
--  caller_pid: playerid de quem executou (ou -1 para console)
-- ============================================================
function reloadAll(caller_pid)
    nx_print("[HotReload] Iniciando reload global...")

    caller_pid = caller_pid or -1

    -- Mensagem no chat só para quem executou
    local function msg_caller(cor, text)
        if caller_pid >= 0 and nx_IsPlayerConnected(caller_pid) then
            nx_SendClientMessage(caller_pid, cor, text)
        end
    end

    -- Avisa todos que o reload começou
    show_reload_td("~y~Recarregando tudo...", 3000)

    -- Copia a lista antes de resetar
    local scripts = {}
    for _, v in ipairs(NX._loadedScripts) do
        table.insert(scripts, v)
    end

    -- RESET: limpa todos os hooks
    if NX._resetHooks then NX._resetHooks() end

    -- Ordena: core/* primeiro, systems/* depois, main.lua por último
    local function priority(path)
        if path == "core/hooks.lua"     then return 1 end
        if path == "core/hotreload.lua" then return 2 end
        if path == "core/init.lua"      then return 3 end
        if path:sub(1,8) == "systems/"  then return 4 end
        if path == "main.lua"           then return 5 end
        return 3
    end

    table.sort(scripts, function(a, b)
        return priority(a) < priority(b)
    end)

    local count, errors = 0, {}
    local loaded_list = {}

    for _, relPath in ipairs(scripts) do
        local ok, err = nx_reloadScript(relPath)
        if ok then
            count = count + 1
            table.insert(loaded_list, relPath)
            msg_caller(NX.COR.CINZA, "  [~] " .. relPath)
        else
            table.insert(errors, relPath .. ": " .. (err or "?"))
            nx_print("[HotReload] ERRO: " .. relPath)
            msg_caller(NX.COR.VERMELHO, "  [!] ERRO: " .. relPath)
        end
    end

    -- TextDraw sequencial para TODOS: mostra cada arquivo
    local delay = 0
    for _, relPath in ipairs(loaded_list) do
        local txt = "~w~[Reload]: ~g~" .. relPath
        nx_SetTimer(function()
            show_reload_td(txt, 1000)
        end, delay, false)
        delay = delay + 600 -- Reduzi o delay para ser mais fluido
    end

    -- TextDraw final para TODOS: resultado
    local final_ok  = "~g~" .. count .. " scripts~w~ recarregados"
    local final_err = #errors > 0 and (" ~r~| " .. #errors .. " erros") or ""
    nx_SetTimer(function()
        show_reload_td("~w~[Reload] " .. final_ok .. final_err, 3000)
    end, delay, false)

    nx_print("[HotReload] Reload ALL: " .. count .. " scripts recarregados, "
        .. #errors .. " erros.")
    return count, errors
end

-- ============================================================
--  Comando /reload (in-game)
-- ============================================================
NX.Hook("OnPlayerCommand", function(playerid, cmdtext)
    -- Extrai comando e argumento
    local cmd, arg = cmdtext:match("^(/[^%s]+)%s*(.*)")
    if cmd ~= "/reload" then return end

    -- Verifica admin (nivel 1+) — comente para liberar a todos durante desenvolvimento
    if not NX.is_admin(playerid, 1) then
        -- Permite ao dono do servidor (playerid 0) sempre usar
        if playerid ~= 0 then
            nx_SendClientMessage(playerid, NX.COR.VERMELHO,
                "[!] Sem permissao para usar /reload.")
            return true
        end
    end

    arg = arg:match("^%s*(.-)%s*$") -- trim

    if arg == "" then
        nx_SendClientMessage(playerid, NX.COR.AMARELO,
            "Uso: /reload <arquivo.lua | pasta/ | all>")
        return true
    end

    -- /reload all
    if arg == "all" then
        nx_SendClientMessage(playerid, NX.COR.AMARELO, "[Reload] Recarregando tudo...")
        local count, errors = reloadAll(playerid)
        if #errors == 0 then
            nx_SendClientMessage(playerid, NX.COR.VERDE,
                "[Reload] " .. count .. " scripts recarregados com sucesso!")
        else
            nx_SendClientMessage(playerid, NX.COR.AMARELO,
                "[Reload] " .. count .. " ok, " .. #errors .. " erros. Veja o console.")
        end
        return true
    end
    -- /reload pasta/
    if arg:sub(-1) == "/" then
        nx_SendClientMessage(playerid, NX.COR.AMARELO,
            "[Reload] Recarregando pasta: " .. arg)
        local count, errors = reloadFolder(arg)
        if #errors == 0 then
            show_reload_td("~w~[Reload]: ~g~" .. count .. " scripts", 3000)
            nx_SendClientMessage(playerid, NX.COR.VERDE,
                "[Reload] " .. count .. " scripts recarregados!")
        else
            show_reload_td("~w~[Reload]: ~g~" .. count .. " ok, ~r~" .. #errors .. " erros", 3000)
            nx_SendClientMessage(playerid, NX.COR.AMARELO,
                "[Reload] " .. count .. " ok, " .. #errors .. " erros.")
        end
        return true
    end

    -- /reload arquivo.lua (ou sem extensão)
    local ok, err = reloadScript(arg)
    if ok then
        show_reload_td("~w~[Reload]: ~g~" .. arg, 2000)
        nx_SendClientMessage(playerid, NX.COR.VERDE,
            "[Reload] '" .. arg .. "' recarregado!")
    else
        show_reload_td("~r~[Reload] Erro: ~w~" .. arg, 3000)
        nx_SendClientMessage(playerid, NX.COR.VERMELHO,
            "[Reload] Erro: " .. (err or "falha desconhecida"))
    end
    return true

end, NX.HOOK_PRIORITY.HIGH)

nx_print("[NX] Hot Reload carregado.")
