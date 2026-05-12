# Nexorix Framework — Linux

Servidor SA-MP/open.mp com scripting em **Lua 5.4**. Substitui completamente o Pawn por uma linguagem moderna, rápida e fácil de usar.

## Início Rápido

```bash
chmod +x nx-server start.sh
./start.sh
```

Ou diretamente:
```bash
export LD_LIBRARY_PATH=".:./NexorixPlugins:$LD_LIBRARY_PATH"
./nx-server
```

## Estrutura

```
├── nx-server              Executável do servidor
├── start.sh               Script de inicialização
├── config.json            Configuração do servidor
├── NexorixPlugins/        Plugins do servidor (não mexer)
│   └── Lua.so             Plugin Lua (core + voice)
└── lua/
    ├── include/           Framework (compilado, não editável)
    │   ├── nexorix.luac   Core: hooks, comandos, constantes
    │   └── nexorix_voice.luac  API de voz
    └── gamemode/
        ├── main.lua       Ponto de entrada (edite aqui)
        └── modulos/       Seus módulos
```

## Como Funciona

### Entry Point

O servidor carrega `lua/gamemode/main.lua` automaticamente. Nele você faz `require` dos seus módulos:

```lua
require("include/nexorix")          -- Framework (obrigatório)
require("include/nexorix_voice")    -- Voice API (opcional)

require("gamemode/modulos/meu_modulo")

NX.Hook("OnGameModeInit", function()
    nx_print("Servidor iniciado!")
end)
```

### Hooks (Eventos)

```lua
NX.Hook("OnPlayerConnect", function(playerid)
    nx_SendClientMessage(playerid, 0xFFFFFFFF, "Bem-vindo!")
end)

NX.Hook("OnPlayerDeath", function(playerid, killerid, reason)
    -- lógica aqui
end)
```

Eventos disponíveis:
- `OnGameModeInit`, `OnGameModeExit`
- `OnPlayerConnect`, `OnPlayerDisconnect`
- `OnPlayerSpawn`, `OnPlayerDeath`
- `OnPlayerText`, `OnPlayerCommand`
- `OnPlayerUpdate`, `OnPlayerKeyStateChange`
- `OnPlayerEnterVehicle`, `OnPlayerExitVehicle`
- `OnPlayerStateChange`, `OnPlayerRequestClass`
- `OnDialogResponse`, `OnPlayerPickUpPickup`
- `OnPlayerClickMap`, `OnPlayerClickTextDraw`
- `OnTick`
- `OnPlayerVoiceConnect`, `OnPlayerVoiceDisconnect`
- `OnPlayerVoiceStart`, `OnPlayerVoiceStop`

### Comandos

```lua
NX.RegisterCommand("teste", function(playerid, params)
    nx_SendClientMessage(playerid, 0x00FF00FF, "Funciona! Params: " .. params)
end)
```

O processador de comandos já vem embutido. Quando o player digita `/teste algo`, a função recebe `playerid` e `"algo"`.

### Funções Nativas (nx_*)

Todas as funções SA-MP estão disponíveis com prefixo `nx_`:

```lua
-- Player
nx_SetPlayerPos(playerid, x, y, z)
nx_GetPlayerPos(playerid)              -- retorna x, y, z
nx_SetPlayerHealth(playerid, 100)
nx_GivePlayerWeapon(playerid, 31, 500) -- M4 com 500 balas
nx_SendClientMessage(playerid, cor, "texto")
nx_SendClientMessageToAll(cor, "texto")
nx_ShowPlayerDialog(playerid, id, style, titulo, corpo, btn1, btn2)
nx_Kick(playerid)
nx_IsPlayerConnected(playerid)
nx_GetPlayerName(playerid)

-- Veículo
nx_CreateVehicle(model, x, y, z, angle, cor1, cor2, respawn)
nx_DestroyVehicle(vehicleid)
nx_PutPlayerInVehicle(playerid, vehicleid, seat)

-- Mundo
nx_SetWorldTime(hora)
nx_SetWeather(weather)
nx_SetGravity(gravity)

-- Timer
local id = nx_SetTimer(function() ... end, intervalo_ms, repeating)
nx_KillTimer(id)
```

### Sistema de Voz

```lua
require("include/nexorix_voice")

-- Criar stream de proximidade
local stream = NX.Voice.CreateProximityStream(50.0)
stream:AttachToPlayer(playerid)
stream:AddPlayer(listener_id)

-- Criar canal de rádio
local radio = NX.Voice.CreateChannel({
    name = "Policia",
    type = "global",
    channel_index = 1,
})
radio:AddPlayer(playerid)

-- Efeitos
local effect = NX.Voice.CreateRadioEffect()
radio:SetEffect(effect)

-- Mute
NX.Voice.SetPlayerMuted(playerid, true)

-- Push-to-talk
NX.Voice.SetPushToTalkKey(playerid, 0x42, {0}) -- B key, channel 0
```

### Discord Bot

```lua
-- Enviar mensagem para o Discord
Discord_SendMessage("Servidor online!")
Discord_SendEmbed("Titulo", "Descricao", 65280)

-- Comandos do Discord (!players, !status, !say)
-- Configurados automaticamente via config.json
```

> **Nota:** O Discord bot requer OpenSSL. No Linux sem OpenSSL, o bot fica desabilitado automaticamente mas o resto do servidor funciona normalmente.

### Database (SQLite)

```lua
-- Abrir banco de dados
local db = nx_db_open("meu_banco.db")

-- Criar tabela
nx_db_exec(db, "CREATE TABLE IF NOT EXISTS players (id INTEGER PRIMARY KEY, nome TEXT, nivel INTEGER)")

-- Inserir com parâmetros (seguro contra SQL injection)
nx_db_exec(db, "INSERT INTO players (nome, nivel) VALUES (?, ?)", "Rick", 10)

-- Consultar
local rows = nx_db_query(db, "SELECT * FROM players WHERE nome = ?", "Rick")
for _, row in ipairs(rows) do
    nx_print(row.id .. " | " .. row.nome .. " | Nivel: " .. row.nivel)
end

-- Último ID inserido
local id = nx_db_last_insert_id(db)

-- Linhas afetadas pelo último INSERT/UPDATE/DELETE
local changes = nx_db_changes(db)

-- Fechar banco
nx_db_close(db)
```

Funções disponíveis:
- `nx_db_open(path)` — Abre/cria banco SQLite
- `nx_db_close(db)` — Fecha o banco
- `nx_db_exec(db, sql, ...)` — Executa INSERT/UPDATE/DELETE/CREATE
- `nx_db_query(db, sql, ...)` — Executa SELECT, retorna tabela de rows
- `nx_db_last_insert_id(db)` — Último rowid inserido
- `nx_db_changes(db)` — Linhas afetadas
- `nx_db_prepare(db, sql)` — Prepared statement
- `nx_db_bind(stmt, index, value)` — Bind valor ao statement
- `nx_db_step(stmt)` — Executa step ("row"/"done"/"error")
- `nx_db_column(stmt, index)` — Lê coluna (0-based)
- `nx_db_reset(stmt)` — Reset statement para reusar
- `nx_db_finalize(stmt)` — Libera statement

## config.json

```json
{
    "name": "Meu Servidor",
    "max_players": 100,
    "network": { "port": 7777 },
    "lua": { "entry": "lua/gamemode" },
    "discord": {
        "enabled": true,
        "token": "SEU_TOKEN",
        "channel_id": "SEU_CHANNEL"
    },
    "voice": {
        "enabled": true,
        "port": 2020
    }
}
```

## Constantes

```lua
NX.COLOR.WHITE   -- 0xFFFFFFFF
NX.COLOR.RED     -- 0xFF4444FF
NX.COLOR.GREEN   -- 0x44FF44FF
NX.COLOR.YELLOW  -- 0xFFFF44FF
NX.COLOR.BLUE    -- 0x4488FFFF

NX.INVALID_PLAYER_ID  -- 0xFFFF
NX.MAX_PLAYERS        -- 1000
```

## Requisitos Linux

- Ubuntu 18.04+ / Debian 10+
- Arquitetura x86 (32-bit libs necessárias)
- Porta 7777 (jogo) e 2020 (voice) abertas

```bash
# Se necessário instalar libs 32-bit:
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install libc6:i386 libstdc++6:i386
```

## Dicas

- Organize seu código em módulos na pasta `gamemode/modulos/`
- Use `nx_print("msg")` para logs no console
- Hooks suportam prioridade: `NX.Hook("evento", fn, NX.HOOK_PRIORITY.HIGH)`
- O framework é thread-safe e otimizado para performance
- Voice requer o client SampVoice instalado nos players

## Hot-Reload

O framework suporta recarregamento de módulos em tempo real sem reiniciar o servidor.

### Comandos

```
/reload          — Recarrega todos os módulos automaticamente
/reload commands — Recarrega um módulo específico
/modules         — Lista todos os módulos detectados
```

### Como funciona

- Escaneia recursivamente `gamemode/modulos/` e `include/`
- Limpa o cache `package.loaded` do módulo
- Re-executa o arquivo com `loadfile` + `pcall`
- Hooks e comandos são re-registrados automaticamente

### Exemplo de uso

1. Edite `gamemode/modulos/commands.lua` e adicione um novo comando
2. No jogo, digite `/reload commands`
3. O novo comando já está disponível sem reiniciar

### Permissão

Requer admin nível 4+ (Gerente). Configure no sistema admin.

> **Nota:** O reload funciona para a maioria dos casos. Se algo não atualizar, reinicie o servidor.

## Créditos

Nexorix Framework — Desenvolvido por Rick_Spooky
