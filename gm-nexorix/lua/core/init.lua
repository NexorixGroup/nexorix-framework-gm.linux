--[[
    ============================================================================
    Nexorix Framework - Core Initialization
    ============================================================================
    Este arquivo gerencia a carga inicial do framework, definindo o 
    namespace global NX e carregando os utilitários base.
    ============================================================================
]]

NX = NX or {}

-- Configurações Básicas
NX.VERSION           = "1.0.0"
NX.INVALID_PLAYER_ID = 0xFFFF
NX.MAX_PLAYERS       = 1000

-- Tabela Global de Jogadores
NX.jogador = {}

--[[
    ============================================================================
    SISTEMA DE CORES (HEX)
    ============================================================================
]]
NX.COR = {
    BRANCO       = 0xFFFFFFFF,
    PRETO        = 0x000000FF,
    CINZA        = 0xAAAAAAFF,
    VERDE        = 0x44FF44FF,
    VERMELHO     = 0xFF4444FF,
    AMARELO      = 0xFFFF44FF,
    AZUL         = 0x4488FFFF,
    LARANJA      = 0xFF8800FF,
    ADMIN        = 0xFF6600FF,
    MOD          = 0x00AAFFFF,
}

-- Inicializa os dados de um jogador com valores padrao
-- Sistemas podem estender esta funcao via NX.on_player_init
function NX.init_jogador(playerid)
    NX.jogador[playerid] = {
        logado     = false,
        tentativas = 0,
        dinheiro   = 0,
        score      = 0,
        admin      = 0,
    }
    -- Permite que sistemas adicionem seus proprios campos
    if NX.on_player_init then
        NX.on_player_init(playerid, NX.jogador[playerid])
    end
end

-- Limpa os dados de um jogador ao desconectar
function NX.limpar_jogador(playerid)
    if NX.on_player_cleanup then
        NX.on_player_cleanup(playerid, NX.jogador[playerid])
    end
    NX.jogador[playerid] = nil
end

-- ============================================================
--  Helpers globais
-- ============================================================

-- Envia mensagem formatada para um jogador
function NX.msg(playerid, cor, texto)
    nx_SendClientMessage(playerid, cor, texto)
end

-- Envia mensagem para todos
function NX.msg_all(cor, texto)
    nx_SendClientMessageToAll(cor, texto)
end

-- Verifica se jogador esta logado
function NX.logado(playerid)
    return NX.jogador[playerid] and NX.jogador[playerid].logado
end

-- Verifica nivel de admin
function NX.is_admin(playerid, nivel)
    nivel = nivel or 1
    return NX.jogador[playerid] and (NX.jogador[playerid].admin >= nivel)
end

-- ============================================================
--  Constantes de estado do jogador
-- ============================================================
NX.PLAYER_STATE = {
    NONE        = 0,
    ONFOOT      = 1,
    DRIVER      = 2,
    PASSENGER   = 3,
    WASTED      = 7,
    SPAWNED     = 8,
    SPECTATING  = 9,
}

-- ============================================================
--  Constantes de armas (mais usadas)
-- ============================================================
NX.WEAPON = {
    FIST      = 0,
    COLT45    = 22,
    DEAGLE    = 24,
    SHOTGUN   = 25,
    UZI       = 28,
    MP5       = 29,
    AK47      = 30,
    M4        = 31,
    SNIPER    = 34,
    PARACHUTE = 46,
}

-- ============================================================
--  Constantes de teclas
-- ============================================================
NX.KEY = {
    ACTION    = 1,
    CROUCH    = 2,
    FIRE      = 4,
    SPRINT    = 8,
    JUMP      = 32,
    HANDBRAKE = 128,
}

-- ============================================================
--  API completa disponivel em Lua (nx_* functions)
-- ============================================================
--
--  PLAYER
--    nx_SendClientMessage(pid, cor, msg)
--    nx_SendClientMessageToAll(cor, msg)
--    nx_GetPlayerName(pid) -> string
--    nx_SetPlayerPos(pid, x, y, z)
--    nx_GetPlayerPos(pid) -> x, y, z
--    nx_SetPlayerFacingAngle(pid, angle)
--    nx_GetPlayerFacingAngle(pid) -> angle
--    nx_SetPlayerHealth(pid, hp)
--    nx_GetPlayerHealth(pid) -> float
--    nx_SetPlayerArmour(pid, arm)
--    nx_GetPlayerArmour(pid) -> float
--    nx_GivePlayerMoney(pid, amount)
--    nx_GetPlayerMoney(pid) -> int
--    nx_ResetPlayerMoney(pid)
--    nx_SetPlayerScore(pid, score)
--    nx_GetPlayerScore(pid) -> int
--    nx_SetPlayerSkin(pid, skin)
--    nx_GetPlayerSkin(pid) -> int
--    nx_SetPlayerColor(pid, color)
--    nx_GetPlayerColor(pid) -> int
--    nx_SetPlayerTeam(pid, team)
--    nx_GetPlayerTeam(pid) -> int
--    nx_GetPlayerState(pid) -> int
--    nx_SetPlayerVirtualWorld(pid, vw)
--    nx_GetPlayerVirtualWorld(pid) -> int
--    nx_SetPlayerInterior(pid, interior)
--    nx_GetPlayerInterior(pid) -> int
--    nx_SpawnPlayer(pid)
--    nx_KickPlayer(pid)
--    nx_BanPlayer(pid)
--    nx_IsPlayerConnected(pid) -> bool
--    nx_GetPlayerPing(pid) -> int
--    nx_GetPlayerIP(pid) -> string
--    nx_IsPlayerNPC(pid) -> bool
--    nx_GivePlayerWeapon(pid, weapon, ammo)
--    nx_ResetPlayerWeapons(pid)
--    nx_GetPlayerWeapon(pid) -> int
--    nx_GetPlayerAmmo(pid) -> int
--    nx_SetPlayerArmedWeapon(pid, weapon)
--    nx_ApplyAnimation(pid, lib, name, delta, loop, lockx, locky, freeze, time, sync)
--    nx_ClearAnimations(pid, sync)
--    nx_SetPlayerSpecialAction(pid, action)
--    nx_GetPlayerSpecialAction(pid) -> int
--    nx_SetPlayerFightingStyle(pid, style)
--    nx_SetPlayerVelocity(pid, x, y, z)
--    nx_GetPlayerVelocity(pid) -> x, y, z
--    nx_SetPlayerGravity(pid, gravity)
--    nx_SetPlayerWeather(pid, weather)
--    nx_SetPlayerTime(pid, hour, minute)
--    nx_TogglePlayerClock(pid, enable)
--    nx_SetPlayerWantedLevel(pid, level)
--    nx_GetPlayerWantedLevel(pid) -> int
--    nx_SetPlayerChatBubble(pid, text, color, drawdist, expiretime)
--    nx_GameTextForPlayer(pid, text, time, style)
--    nx_SetPlayerMapIcon(pid, iconid, x, y, z, type, color, style)
--    nx_RemovePlayerMapIcon(pid, iconid)
--    nx_TogglePlayerControllable(pid, enable)
--    nx_SetPlayerCameraPos(pid, x, y, z)
--    nx_SetCameraBehindPlayer(pid)
--    nx_SetPlayerCameraLookAt(pid, x, y, z, cut)
--    nx_PlaySoundForPlayer(pid, sound, x, y, z)
--    nx_PlayAudioStreamForPlayer(pid, url)
--    nx_StopAudioStreamForPlayer(pid)
--    nx_GetPlayerKeys(pid) -> keys, updown, leftright
--    nx_PutPlayerInVehicle(pid, vid, seat)
--    nx_RemovePlayerFromVehicle(pid)
--    nx_ForceClassSelection(pid)
--    nx_SetPlayerDrunkLevel(pid, level)
--    nx_GetPlayerDrunkLevel(pid) -> int
--
--  VEHICLE
--    nx_CreateVehicle(model, x, y, z, angle, c1, c2, respawn) -> vid
--    nx_DestroyVehicle(vid)
--    nx_SetVehiclePos(vid, x, y, z)
--    nx_GetVehiclePos(vid) -> x, y, z
--    nx_SetVehicleZAngle(vid, angle)
--    nx_GetVehicleZAngle(vid) -> float
--    nx_SetVehicleHealth(vid, hp)
--    nx_GetVehicleHealth(vid) -> float
--    nx_RepairVehicle(vid)
--    nx_SetVehicleVelocity(vid, x, y, z)
--    nx_GetVehicleVelocity(vid) -> x, y, z
--    nx_GetPlayerVehicleID(pid) -> vid
--    nx_IsPlayerInVehicle(pid) -> bool
--    nx_IsPlayerInAnyVehicle(pid) -> bool
--    nx_ChangeVehicleColor(vid, c1, c2)
--    nx_SetVehicleNumberPlate(vid, plate)
--
--  PICKUP
--    nx_CreatePickup(model, type, x, y, z, vworld) -> id
--    nx_DestroyPickup(id)
--
--  OBJECT
--    nx_CreateObject(model, x, y, z, rx, ry, rz, dd) -> id
--    nx_DestroyObject(id)
--    nx_SetObjectPos(id, x, y, z)
--    nx_GetObjectPos(id) -> x, y, z
--    nx_SetObjectRot(id, rx, ry, rz)
--    nx_MoveObject(id, x, y, z, speed, rx, ry, rz)
--    nx_StopObject(id)
--
--  3D TEXT LABEL
--    nx_Create3DTextLabel(text, color, x, y, z, dd, vw, los) -> id
--    nx_Delete3DTextLabel(id)
--    nx_Update3DTextLabelText(id, color, text)
--
--  TEXTDRAW
--    nx_TextDrawCreate(x, y, text) -> id
--    nx_TextDrawDestroy(id)
--    nx_TextDrawShowForPlayer(pid, id)
--    nx_TextDrawHideForPlayer(pid, id)
--    nx_TextDrawShowForAll(id)
--    nx_TextDrawHideForAll(id)
--    nx_TextDrawSetString(id, text)
--    nx_TextDrawColor(id, color)
--    nx_TextDrawBoxColor(id, color)
--    nx_TextDrawUseBox(id, use)
--    nx_TextDrawLetterSize(id, x, y)
--    nx_TextDrawTextSize(id, x, y)
--    nx_TextDrawAlignment(id, align)
--    nx_TextDrawFont(id, font)
--    nx_TextDrawSetShadow(id, size)
--    nx_TextDrawSetOutline(id, size)
--    nx_TextDrawSetProportional(id, set)
--    nx_TextDrawSetSelectable(id, set)
--
--  CLASS
--    nx_AddPlayerClass(skin, x, y, z, angle, w1, a1, w2, a2, w3, a3) -> id
--    nx_SetSpawnInfo(pid, skin, x, y, z, angle, w1, a1, w2, a2, w3, a3)
--
--  CHECKPOINT
--    nx_SetPlayerCheckpoint(pid, x, y, z, radius)
--    nx_DisablePlayerCheckpoint(pid)
--    nx_IsPlayerInCheckpoint(pid) -> bool
--
--  GANGZONE
--    nx_GangZoneCreate(minx, miny, maxx, maxy) -> id
--    nx_GangZoneDestroy(id)
--    nx_GangZoneShowForPlayer(pid, id, color)
--    nx_GangZoneHideForPlayer(pid, id)
--    nx_GangZoneFlashForPlayer(pid, id, color)
--    nx_GangZoneStopFlashForPlayer(pid, id)
--    nx_GangZoneShowForAll(id, color)
--    nx_GangZoneHideForAll(id)
--
--  SERVER
--    nx_SetGameModeText(text)
--    nx_GetTickCount() -> int
--    nx_print(msg)
--    nx_GameModeExit()
--    nx_SetWorldTime(hour)
--    nx_SetWeather(weatherid)
--    nx_SetGravity(gravity)
--    nx_GameTextForAll(text, time, style)
--    nx_GetMaxPlayers() -> int
--
--  TIMER
--    nx_SetTimer(func, interval_ms, repeat) -> id
--    nx_KillTimer(id)
--
--  DIALOG
--    nx_ShowPlayerDialog(pid, dialogid, style, title, body, btn1, btn2)
--    nx_HidePlayerDialog(pid)
-- ============================================================

-- ============================================================
--  Carregamento automatico de sistemas
-- ============================================================

-- Lista de sistemas a carregar (ordem importa)
-- Os sistemas sao carregados ANTES do hook system ser instalado,
-- para que suas funcoes OnXxx sejam capturadas como hooks LOWEST.
local sistemas = {
    -- "systems/banco",      -- banco de dados compartilhado
    -- "systems/auth",       -- autenticacao (login/registro)
    -- "systems/admin",      -- sistema de administracao
    -- "systems/veiculo",    -- sistema de veiculos
    -- "systems/casa",       -- sistema de casas
    -- "systems/economia",   -- sistema economico
    -- "systems/chat",       -- sistema de chat (local, global, PM)
    -- "systems/spawn",      -- sistema de spawn
}

for _, sistema in ipairs(sistemas) do
    local ok, err = pcall(require, sistema)
    if not ok then
        nx_print("[NX] ERRO ao carregar sistema '" .. sistema .. "': " .. tostring(err))
    else
        nx_print("[NX] Sistema carregado: " .. sistema)
    end
end

-- ============================================================
--  Hook system — carregado antes dos sistemas que usam NX.Hook
-- ============================================================
require("core/hooks")

-- ============================================================
--  Hot Reload — carregado após hooks (usa NX.Hook internamente)
-- ============================================================
require("core/hotreload")

-- ============================================================
--  Sistemas que dependem do hook system (carregados depois)
-- ============================================================
local sistemas_pos_hooks = {
    "systems/discord",    -- discord connector (usa NX.Hook)
}

for _, sistema in ipairs(sistemas_pos_hooks) do
    local ok, err = pcall(require, sistema)
    if not ok then
        nx_print("[NX] ERRO ao carregar sistema '" .. sistema .. "': " .. tostring(err))
    else
        nx_print("[NX] Sistema carregado: " .. sistema)
    end
end

nx_print("[NX] Core inicializado. Versao " .. NX.VERSION)
