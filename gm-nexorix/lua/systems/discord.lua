--[[
    ============================================================================
    Nexorix Framework - Discord Connector
    ============================================================================
    Integração bidirecional entre o servidor Nexorix e o Discord.
    ============================================================================
]]

if not discord_sendMessage then
    nx_print("[Discord] AVISO: funcoes discord_* nao encontradas.")
    return
end

local function CH() return DISCORD_CHANNEL_ID or discord_getChannelId() or "" end

-- Cores dos embeds (decimal)
local COR = {
    VERDE    = 0x44FF44,
    VERMELHO = 0xFF4444,
    AZUL     = 0x4488FF,
    AMARELO  = 0xFFFF44,
    CINZA    = 0xAAAAAA,
    LARANJA  = 0xFF8800,
    ROXO     = 0x9B59B6,
    BRANCO   = 0xFFFFFF,
}

nx_print("[Discord] Sistema carregado. Channel: " .. CH())

-- ============================================================
--  onReady
-- ============================================================
discord_onReady(function()
    nx_print("[Discord] onReady disparado! Channel=" .. CH())
    local ch = CH()
    if ch == "" then nx_print("[Discord] ERRO: channel_id vazio!") return end

    discord_sendEmbed(ch, {
        title       = "✅ Nexorix Online",
        description = "O servidor foi iniciado com sucesso.",
        color       = COR.VERDE,
        footer      = "Nexorix Framework",
    })
    discord_setStatus("Nexorix | 0/" .. nx_GetMaxPlayers() .. " slots")
end)

-- ============================================================
--  onMessage (log)
-- ============================================================
discord_onMessage(function(user, msg)
    nx_print("[Discord] " .. tostring(user) .. ": " .. tostring(msg))
end)

-- ============================================================
--  Hooks servidor -> Discord
-- ============================================================
NX.Hook("OnPlayerConnect", function(playerid)
    local ch = CH(); if ch == "" then return end
    discord_sendEmbed(ch, {
        description = "🟢 **" .. nx_GetPlayerName(playerid) .. "** entrou no servidor.",
        color       = COR.VERDE,
    })
end, NX.HOOK_PRIORITY.LOW)

NX.Hook("OnPlayerDisconnect", function(playerid, reason)
    local ch = CH(); if ch == "" then return end
    local motivos = { [0]="timeout", [1]="saiu", [2]="kick/ban" }
    discord_sendEmbed(ch, {
        description = "🔴 **" .. nx_GetPlayerName(playerid) .. "** saiu (" .. (motivos[reason] or "?") .. ").",
        color       = COR.VERMELHO,
    })
end, NX.HOOK_PRIORITY.LOW)

NX.Hook("OnPlayerDeath", function(playerid, killerid, reason)
    local ch = CH(); if ch == "" then return end
    local nome = nx_GetPlayerName(playerid)
    if killerid ~= INVALID_PLAYER_ID then
        local nomeK = nx_GetPlayerName(killerid)
        discord_sendEmbed(ch, {
            description = "💀 **" .. nome .. "** foi morto por **" .. nomeK .. "**.",
            color       = COR.LARANJA,
        })
    else
        discord_sendEmbed(ch, {
            description = "💀 **" .. nome .. "** morreu.",
            color       = COR.CINZA,
        })
    end
end, NX.HOOK_PRIORITY.LOW)

-- ============================================================
--  Comandos Discord
-- ============================================================

-- !players
discord_registerCommand("players", function(user, args)
    local ch = CH()
    local count, lista = 0, {}
    for pid = 0, nx_GetMaxPlayers() - 1 do
        if nx_IsPlayerConnected(pid) then
            count = count + 1
            lista[#lista + 1] = nx_GetPlayerName(pid)
        end
    end

    local desc = count == 0
        and "*Nenhum jogador online no momento.*"
        or  table.concat(lista, "\n")

    discord_sendEmbed(ch, {
        title       = "👥 Jogadores Online",
        description = desc,
        color       = count > 0 and COR.AZUL or COR.CINZA,
        footer      = count .. "/" .. nx_GetMaxPlayers() .. " jogadores",
    })
end)

-- !status
discord_registerCommand("status", function(user, args)
    local ch = CH()
    local count = 0
    for pid = 0, nx_GetMaxPlayers() - 1 do
        if nx_IsPlayerConnected(pid) then count = count + 1 end
    end

    discord_sendEmbed(ch, {
        title  = "📊 Status do Servidor",
        color  = COR.AZUL,
        fields = {
            { name = "Jogadores",  value = count .. "/" .. nx_GetMaxPlayers(), inline = true  },
            { name = "Tick",       value = tostring(nx_GetTickCount()),         inline = true  },
            { name = "Modo",       value = "Nexorix Lua",                       inline = true  },
        },
        footer = "Nexorix Framework",
    })
end)

-- !say <mensagem>
discord_registerCommand("say", function(user, args)
    local ch = CH()
    if #args == 0 then
        discord_sendEmbed(ch, { description = "❌ Uso: `!say <mensagem>`", color = COR.VERMELHO })
        return
    end
    local text = table.concat(args, " ")
    nx_SendClientMessageToAll(NX.COR.AZUL, "[Discord] " .. user .. ": " .. text)
    discord_sendEmbed(ch, {
        description = "📢 Mensagem enviada ao servidor.",
        color       = COR.VERDE,
        footer      = "Por: " .. user,
    })
end)

-- !kick <nome>
discord_registerCommand("kick", function(user, args)
    local ch = CH()
    if #args == 0 then
        discord_sendEmbed(ch, { description = "❌ Uso: `!kick <nome>`", color = COR.VERMELHO })
        return
    end
    local target = args[1]:lower()
    for pid = 0, nx_GetMaxPlayers() - 1 do
        if nx_IsPlayerConnected(pid) and nx_GetPlayerName(pid):lower() == target then
            nx_KickPlayer(pid)
            discord_sendEmbed(ch, {
                description = "✅ **" .. args[1] .. "** foi kickado.",
                color       = COR.VERDE,
                footer      = "Por: " .. user,
            })
            return
        end
    end
    discord_sendEmbed(ch, {
        description = "❌ Jogador **" .. args[1] .. "** não encontrado.",
        color       = COR.VERMELHO,
    })
end)

-- !perfil <nick>
discord_registerCommand("perfil", function(user, args)
    local ch = CH()
    if #args == 0 then
        discord_sendEmbed(ch, { description = "❌ Uso: `!perfil <Nickname>`", color = COR.VERMELHO })
        return
    end

    local nick  = args[1]
    local db_p  = nx_db_open("lua_contas.db")
    local rows  = nx_db_query(db_p,
        "SELECT nome, dinheiro, score FROM contas WHERE nome = ? COLLATE NOCASE", nick)
    nx_db_close(db_p)

    if #rows == 0 then
        discord_sendEmbed(ch, {
            description = "❌ Conta **" .. nick .. "** não encontrada.",
            color       = COR.VERMELHO,
        })
        return
    end

    local nome     = rows[1].nome
    local dinheiro = rows[1].dinheiro
    local score    = rows[1].score

    local online, ping, hp, estado = false, 0, 0, "Offline"
    local estados = {[0]="Nenhum",[1]="A pé",[2]="Dirigindo",[3]="Passageiro",
                     [7]="Morto",[8]="Spawnado",[9]="Espectando"}
    for pid = 0, nx_GetMaxPlayers() - 1 do
        if nx_IsPlayerConnected(pid) and nx_GetPlayerName(pid):lower() == nome:lower() then
            online = true
            ping   = nx_GetPlayerPing(pid)
            hp     = math.floor(nx_GetPlayerHealth(pid))
            estado = estados[nx_GetPlayerState(pid)] or "?"
            break
        end
    end

    local function fmt(n)
        local s, r = tostring(math.abs(n)), ""
        for i = 1, #s do
            if i > 1 and (#s - i + 1) % 3 == 0 then r = r .. "." end
            r = r .. s:sub(i,i)
        end
        return (n < 0 and "-" or "") .. "R$" .. r
    end

    local fields = {
        { name = "� Dinheiro", value = fmt(dinheiro),                    inline = true  },
        { name = "⭐ Score",    value = tostring(score),                   inline = true  },
        { name = "📍 Status",   value = online and "🟢 Online" or "⚫ Offline", inline = true },
    }
    if online then
        table.insert(fields, { name = "🏓 Ping",   value = ping .. "ms",  inline = true })
        table.insert(fields, { name = "❤️ Vida",   value = hp .. " HP",   inline = true })
        table.insert(fields, { name = "🎮 Estado", value = estado,         inline = true })
    end

    discord_sendEmbed(ch, {
        title  = "👤 Perfil — " .. nome,
        color  = online and COR.VERDE or COR.CINZA,
        fields = fields,
        footer = "Nexorix Framework",
    })
end)

-- !help
discord_registerCommand("help", function(user, args)
    local ch = CH()
    discord_sendEmbed(ch, {
        title  = "📖 Comandos Disponíveis",
        color  = COR.ROXO,
        fields = {
            { name = "!players",       value = "Lista jogadores online",      inline = false },
            { name = "!status",        value = "Status do servidor",          inline = false },
            { name = "!perfil <nick>", value = "Perfil de uma conta",         inline = false },
            { name = "!say <msg>",     value = "Envia mensagem ao servidor",  inline = false },
            { name = "!kick <nick>",   value = "Kicka um jogador",            inline = false },
            { name = "!help",          value = "Esta mensagem",               inline = false },
        },
        footer = "Nexorix Framework",
    })
end)

nx_print("[Discord] " .. 6 .. " comandos registrados.")
