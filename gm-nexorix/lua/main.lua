--[[
    ============================================================================
    Nexorix Framework - Main GameMode
    ============================================================================
    Autor: RickZin021
    Base: Nexorix
    Linguagem: Lua 5.4
    ============================================================================
]]

-- Carregamento do Core
require("core/init")

-- Constantes Locais
local COLOR_WHITE  = NX.COR.BRANCO
local COLOR_GREEN  = NX.COR.VERDE
local COLOR_YELLOW = NX.COR.AMARELO
local COLOR_GRAY   = NX.COR.CINZA
local COLOR_RED    = NX.COR.VERMELHO

local DLG_LOGIN    = 1
local DLG_REGISTRO = 2

local SPAWN_X, SPAWN_Y, SPAWN_Z = 1958.3783, 1343.1572, 15.3746
local db

--[[
    ============================================================================
    SISTEMA DE BANCO DE DADOS (SQLite)
    ============================================================================
]]

local function init_db()
    -- Fecha conexão anterior se existir (hot reload)
    if db then
        pcall(nx_db_close, db)
        db = nil
    end
    db = nx_db_open("lua_contas.db")
    nx_db_exec(db, [[
        CREATE TABLE IF NOT EXISTS contas (
            nome     TEXT PRIMARY KEY,
            senha    TEXT NOT NULL,
            dinheiro INTEGER DEFAULT 0,
            score    INTEGER DEFAULT 0
        )
    ]])
end

local function conta_existe(nome)
    local rows = nx_db_query(db, "SELECT nome FROM contas WHERE nome = ?", nome)
    return #rows > 0
end

local function criar_conta(nome, senha)
    nx_db_exec(db, "INSERT INTO contas (nome, senha) VALUES (?, ?)", nome, senha)
end

local function verificar_senha(nome, senha)
    local rows = nx_db_query(db, "SELECT senha FROM contas WHERE nome = ?", nome)
    if #rows == 0 then return false end
    return rows[1].senha == senha
end

local function carregar_conta(playerid, nome)
    local rows = nx_db_query(db, "SELECT dinheiro, score FROM contas WHERE nome = ?", nome)
    if #rows == 0 then return end
    NX.jogador[playerid].dinheiro = rows[1].dinheiro
    NX.jogador[playerid].score    = rows[1].score
end

local function salvar_conta(playerid)
    if not NX.logado(playerid) then return end
    local nome = nx_GetPlayerName(playerid)
    nx_db_exec(db,
        "UPDATE contas SET dinheiro = ?, score = ? WHERE nome = ?",
        nx_GetPlayerMoney(playerid),
        nx_GetPlayerScore(playerid),
        nome)
end

-- ============================================================
--  DIALOGS
-- ============================================================

local function mostrar_login(playerid)
    nx_ShowPlayerDialog(playerid, DLG_LOGIN, DIALOG_STYLE_PASSWORD,
        "Login - Nexorix",
        "Bem-vindo de volta!\nDigite sua senha:",
        "Entrar", "Sair")
end

local function mostrar_registro(playerid)
    nx_ShowPlayerDialog(playerid, DLG_REGISTRO, DIALOG_STYLE_PASSWORD,
        "Registro - Nexorix",
        "Primeira vez aqui!\nCrie uma senha (minimo 4 caracteres):",
        "Registrar", "Sair")
end

-- ============================================================
--  CALLBACKS — todos via NX.Hook para não sobrescrever o dispatcher
-- ============================================================

NX.Hook("OnGameModeInit", function()
    init_db()
    nx_SetGameModeText("Nexorix")
    nx_print("============================")
    nx_print("     NEXORIX SERVER         ")
    nx_print("  Name framework: Nexorix   ")
    nx_print("  Autor: RickZin021         ")
    nx_print("============================")
    nx_print("[Lua] GameMode iniciado.")
end, NX.HOOK_PRIORITY.NORMAL)

-- Garante que o banco está aberto mesmo após hot reload.
-- OnGameModeInit não é chamado novamente no reload, então
-- inicializamos o banco diretamente ao carregar o arquivo.
init_db()
nx_print("[Lua] Banco de dados inicializado.")

NX.Hook("OnGameModeExit", function()
    if db then
        nx_db_close(db)
        db = nil
    end
    nx_print("[Lua] GameMode encerrado.")
end, NX.HOOK_PRIORITY.NORMAL)

NX.Hook("OnPlayerConnect", function(playerid)
    NX.init_jogador(playerid)
    local nome = nx_GetPlayerName(playerid)
    nx_SendClientMessageToAll(COLOR_GRAY, "[+] " .. nome .. " entrou no servidor.")
    if conta_existe(nome) then
        mostrar_login(playerid)
    else
        mostrar_registro(playerid)
    end
end, NX.HOOK_PRIORITY.NORMAL)

NX.Hook("OnPlayerDisconnect", function(playerid, reason)
    salvar_conta(playerid)
    local nome = nx_GetPlayerName(playerid)
    nx_SendClientMessageToAll(COLOR_GRAY, "[-] " .. nome .. " saiu.")
    NX.limpar_jogador(playerid)
end, NX.HOOK_PRIORITY.NORMAL)

NX.Hook("OnPlayerSpawn", function(playerid)
    if not NX.logado(playerid) then
        nx_SetPlayerPos(playerid, 0.0, 0.0, -100.0)
        nx_SetPlayerVirtualWorld(playerid, playerid + 1)
        return
    end
    nx_SetPlayerPos(playerid, SPAWN_X, SPAWN_Y, SPAWN_Z)
    nx_SetPlayerVirtualWorld(playerid, 0)
    nx_SetPlayerScore(playerid, NX.jogador[playerid].score)
    nx_GivePlayerMoney(playerid, NX.jogador[playerid].dinheiro)
end, NX.HOOK_PRIORITY.NORMAL)

NX.Hook("OnPlayerDeath", function(playerid, killerid, reason)
    local nome = nx_GetPlayerName(playerid)
    if killerid ~= INVALID_PLAYER_ID then
        local nomeK = nx_GetPlayerName(killerid)
        nx_SendClientMessageToAll(COLOR_WHITE, nome .. " foi morto por " .. nomeK .. ".")
        nx_SetPlayerScore(killerid, nx_GetPlayerScore(killerid) + 1)
    else
        nx_SendClientMessageToAll(COLOR_WHITE, nome .. " morreu.")
    end
end, NX.HOOK_PRIORITY.NORMAL)

NX.Hook("OnPlayerText", function(playerid, text)
    if not NX.logado(playerid) then return false end
    -- nil = continua (permite a mensagem)
end, NX.HOOK_PRIORITY.HIGH)

NX.Hook("OnPlayerCommand", function(playerid, cmdtext)
    if cmdtext == "/ping" then
        local ping = nx_GetPlayerPing(playerid)
        nx_SendClientMessage(playerid, COLOR_YELLOW, "Seu ping: " .. ping .. "ms")
        return true
    end

    if cmdtext == "/salvar" then
        salvar_conta(playerid)
        nx_SendClientMessage(playerid, COLOR_GREEN, "[OK] Dados salvos!")
        return true
    end

    if cmdtext == "/pos" then
        local x, y, z = nx_GetPlayerPos(playerid)
        local angle    = nx_GetPlayerFacingAngle(playerid)
        local interior = nx_GetPlayerInterior(playerid)
        local vworld   = nx_GetPlayerVirtualWorld(playerid)
        nx_SendClientMessage(playerid, NX.COR.AMARELO,
            "══════════ Posição ══════════")
        nx_SendClientMessage(playerid, NX.COR.BRANCO,
            string.format("X: %.4f  Y: %.4f  Z: %.4f", x, y, z))
        nx_SendClientMessage(playerid, NX.COR.BRANCO,
            string.format("Ângulo: %.2f°  |  Interior: %d  |  VWorld: %d",
                angle, interior, vworld))
        nx_SendClientMessage(playerid, NX.COR.CINZA,
            string.format("SetPlayerPos(%d, %.4f, %.4f, %.4f)", playerid, x, y, z))
        return true
    end

    -- /ms <id> — muda de skin
    local skin_id = cmdtext:match("^/ms%s+(%d+)$")
    if skin_id then
        skin_id = tonumber(skin_id)
        -- Skins válidas no SA-MP: 0–311 (exceto 74)
        if skin_id < 0 or skin_id > 311 or skin_id == 74 then
            nx_SendClientMessage(playerid, NX.COR.VERMELHO,
                "[!] Skin inválida. Use um ID entre 0 e 311 (exceto 74).")
            return true
        end
        nx_SetPlayerSkin(playerid, skin_id)
        nx_SendClientMessage(playerid, NX.COR.VERDE,
            string.format("[OK] Skin alterada para ID %d.", skin_id))
        return true
    end

    if cmdtext == "/ms" then
        nx_SendClientMessage(playerid, NX.COR.AMARELO,
            "Uso: /ms <id>  |  IDs validos: 0-311 (exceto 74)")
        return true
    end

    -- /carro <modelid> — spawna um veículo na frente do jogador
    local veh_model = cmdtext:match("^/carro%s+(%d+)$")
    if veh_model then
        veh_model = tonumber(veh_model)
        -- Modelos válidos: 400–611
        if veh_model < 400 or veh_model > 611 then
            nx_SendClientMessage(playerid, NX.COR.VERMELHO,
                "[!] Modelo invalido. Use um ID entre 400 e 611.")
            return true
        end

        -- Pega posição e ângulo do jogador para spawnar na frente
        local x, y, z = nx_GetPlayerPos(playerid)
        local angle    = nx_GetPlayerFacingAngle(playerid)

        -- Offset de 5 unidades na direção que o jogador está olhando
        local rad = math.rad(angle)
        local sx  = x + 5.0 * math.sin(-rad)
        local sy  = y + 5.0 * math.cos(-rad)

        local vid = nx_CreateVehicle(veh_model, sx, sy, z, angle, -1, -1, 60)
        if vid == -1 then
            nx_SendClientMessage(playerid, NX.COR.VERMELHO,
                "[!] Falha ao criar o veiculo.")
            return true
        end

        nx_PutPlayerInVehicle(playerid, vid, 0)
        nx_SendClientMessage(playerid, NX.COR.VERDE,
            string.format("[OK] Veiculo %d criado! (ID: %d)", veh_model, vid))
        return true
    end

    if cmdtext == "/carro" then
        nx_SendClientMessage(playerid, NX.COR.AMARELO,
            "Uso: /carro <modelid>  |  IDs validos: 400-611")
        return true
    end
    -- nil = não tratou
end, NX.HOOK_PRIORITY.NORMAL)

NX.Hook("OnPlayerUpdate", function(playerid)
    return true
end, NX.HOOK_PRIORITY.LOWEST)

NX.Hook("OnDialogResponse", function(playerid, dialogid, response, listitem, inputtext)
    if dialogid == DLG_REGISTRO then
        if response == DIALOG_RESPONSE_RIGHT then
            nx_KickPlayer(playerid)
            return
        end
        if #inputtext < 4 or #inputtext > 31 then
            nx_ShowPlayerDialog(playerid, DLG_REGISTRO, DIALOG_STYLE_PASSWORD,
                "Registro - Nexorix",
                "Senha invalida! Use entre 4 e 31 caracteres.\n\nCrie uma senha:",
                "Registrar", "Sair")
            return
        end
        local nome = nx_GetPlayerName(playerid)
        criar_conta(nome, inputtext)
        NX.jogador[playerid].logado = true
        nx_SendClientMessage(playerid, COLOR_GREEN, "[OK] Conta criada! Bem-vindo ao Nexorix.")
        nx_SetTimer(function() nx_SpawnPlayer(playerid) end, 100, false)
        return
    end

    if dialogid == DLG_LOGIN then
        if response == DIALOG_RESPONSE_RIGHT then
            nx_KickPlayer(playerid)
            return
        end
        local nome = nx_GetPlayerName(playerid)
        if not verificar_senha(nome, inputtext) then
            NX.jogador[playerid].tentativas = NX.jogador[playerid].tentativas + 1
            if NX.jogador[playerid].tentativas >= MAX_TENTATIVAS then
                nx_SendClientMessage(playerid, COLOR_RED, "[!] Muitas tentativas. Desconectado.")
                nx_KickPlayer(playerid)
                return
            end
            local msg = "Senha incorreta. Tentativa " .. NX.jogador[playerid].tentativas .. "/" .. MAX_TENTATIVAS .. ".\n\nDigite sua senha:"
            nx_ShowPlayerDialog(playerid, DLG_LOGIN, DIALOG_STYLE_PASSWORD,
                "Login - Nexorix", msg, "Entrar", "Sair")
            return
        end
        carregar_conta(playerid, nome)
        NX.jogador[playerid].logado = true
        nx_SendClientMessage(playerid, COLOR_GREEN, "[OK] Login realizado!")
        nx_SetTimer(function() nx_SpawnPlayer(playerid) end, 100, false)
        return
    end
end, NX.HOOK_PRIORITY.NORMAL)

NX.Hook("OnTick", function(elapsed)
    -- elapsed: microsegundos desde o último tick
end, NX.HOOK_PRIORITY.LOWEST)
