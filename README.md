# 🌌 Nexorix Framework
> **O motor de próxima geração para servidores de alta performance.**

[![Versão](https://img.shields.io/badge/Versão-1.0.0--beta-blueviolet?style=for-the-badge)](https://github.com/nexorix)
[![Lua](https://img.shields.io/badge/Lua-5.4-blue?style=for-the-badge&logo=lua)](https://www.lua.org/)
[![C++](https://img.shields.io/badge/C++-17-00599C?style=for-the-badge&logo=c%2B%2B)](https://isocpp.org/)
[![Plataforma](https://img.shields.io/badge/Plataforma-Windows%20%7C%20Linux-lightgrey?style=for-the-badge)](https://github.com/nexorix)

O **Nexorix Framework** é uma solução de nível empresarial para o desenvolvimento de servidores multiplayer. Construído em C++17, ele oferece uma camada de abstração poderosa que permite aos desenvolvedores criar GameModes complexos utilizando **Lua 5.4**, garantindo a velocidade nativa com a flexibilidade de uma linguagem de script moderna.

---

## � Sumário
- [🚀 Visão Geral](#-visão-geral)
- [🛠️ Pré-requisitos](#️-pré-requisitos)
- [📦 Instalação e Configuração](#-instalação-e-configuração)
- [🏗️ Arquitetura do Projeto](#️-arquitetura-do-projeto)
- [⚓ Sistema de Hooks (Callbacks)](#-sistema-de-hooks-callbacks)
- [🤖 Integração com Discord](#-integração-com-discord)
- [🗄️ Persistência de Dados (SQLite)](#️-persistência-de-dados-sqlite)
- [🛠️ API Nativa (nx_)](#️-api-nativa-nx_)
- [🛡️ Segurança e Anti-Cheat](#️-segurança-e-anti-cheat)
- [🔄 Hot-Reload (Recarregamento Vivo)](#-hot-reload-recarregamento-vivo)
- [🏗️ Proteção de Código (Bytecode)](#️-proteção-de-código-bytecode)
- [🎨 Crie seu Servidor](#-crie-seu-próprio-servidor)
- [📞 Suporte](#-suporte-e-comunidade)

---

## 🚀 Visão Geral

Diferente de frameworks tradicionais, o Nexorix foi desenhado com foco em **modularidade** e **escalabilidade**. 

- **Performance Extrema**: Núcleo em C++ com gerenciamento de memória otimizado.
- **Hot-Reload**: Sistema inteligente de recarregamento de scripts sem derrubar o servidor.
- **API Unificada**: Acesso simplificado a veículos, objetos, jogadores e labels.
- **Segurança Nativa**: Proteções contra ataques comuns e validação de pacotes no kernel do servidor.

---

## 🛠️ Pré-requisitos

Para rodar o Nexorix Server, você precisará de:
- **Windows**: Windows 10/11 ou Windows Server 2019+.
- **Linux**: Ubuntu 20.04+, Debian 11+ ou CentOS 8+.
- **Dependências**: 
  - Visual C++ Redistributable (Windows)
  - liblua5.4-0 (Linux)
  - SQLite3 Runtime

---

## 📦 Instalação e Configuração

1. Clone o repositório ou baixe a release mais recente.
2. Configure o arquivo `config.json` na raiz do projeto:

```json
{
    "name": "Meu Servidor Nexorix",
    "max_players": 100,
    "port": 7777,
    "scripting_mode": "lua",
    "lua": {
        "main_scripts": ["main"]
    }
}
```

3. Execute o binário `nx-server.exe` (Windows) ou `./nx-server` (Linux).

---

## 🏗️ Arquitetura do Projeto

```text
📁 gm-nexorix/
├── 📁 lua/                 # Scripts do GameMode
│   ├── 📁 core/            # Núcleo do framework (hooks, hotreload)
│   ├── 📁 modules/         # Módulos de gameplay (sistemas, comandos)
│   └── 📄 main.lua         # Ponto de entrada principal
├── 📁 sdk/                 # Arquivos de desenvolvimento e headers
├── 📁 Tools/               # Ferramentas auxiliares
├── 📄 config.json          # Configurações globais
└── 📄 lua_contas.db        # Banco de dados SQLite padrão
```

---

## ⚓ Sistema de Hooks (Callbacks)

Os Hooks permitem que você interaja com eventos em tempo real. O Nexorix utiliza um sistema de eventos baseado em nomes globais.

### Exemplo Profissional de Módulo:

```lua
-- modules/player_system.lua

function OnPlayerConnect(playerid)
    local name = nx_GetPlayerName(playerid)
    nx_print(string.format("[LOG] %s (ID: %d) conectou.", name, playerid))
    
    nx_SendClientMessage(playerid, 0x00FF00FF, "Bem-vindo ao Nexorix Framework!")
end

function OnPlayerCommand(playerid, cmdtext)
    local cmd, params = cmdtext:match("^/(%w+)%s*(.*)$")
    
    if cmd == "pos" then
        local x, y, z = nx_GetPlayerPos(playerid)
        nx_SendClientMessage(playerid, -1, string.format("Sua posição: %.2f, %.2f, %.2f", x, y, z))
        return true
    end
    return false
end
```

---

## 🤖 Integração com Discord

O Nexorix possui um bot Discord de alta performance embutido, permitindo logs, comandos remotos e sincronização de chat.

### Configuração de Comandos Discord:

```lua
-- Sincronizar chat do servidor com Discord
function OnPlayerText(playerid, text)
    local name = nx_GetPlayerName(playerid)
    discord_sendMessage(discord_getChannelId(), "**" .. name .. "**: " .. text)
    return true
end

-- Comando direto no Discord
discord_registerCommand("status", function(args)
    return "O servidor está online com " .. nx_GetMaxPlayers() .. " slots!"
end)

-- Outro exemplo: Chutar um jogador via Discord
discord_registerCommand("kick", function(args)
    local playerid = tonumber(args[1])
    if playerid and nx_IsPlayerConnected(playerid) then
        local name = nx_GetPlayerName(playerid)
        nx_KickPlayer(playerid)
        return "O jogador " .. name .. " foi kickado via Discord!"
    end
    return "Uso: !kick [playerid]"
end)
```

---

## �️ Persistência de Dados (SQLite)

Utilizamos o SQLite 3 nativamente para garantir que não haja gargalos de I/O em consultas simples.

```lua
local db = nx_db_open("database.db")

function SavePlayerData(playerid)
    local name = nx_GetPlayerName(playerid)
    local money = nx_GetPlayerMoney(playerid)
    
    nx_db_exec(db, "UPDATE players SET money = ? WHERE name = ?", money, name)
end
```

---

## �️ API Nativa (nx_)

A API `nx_` é o coração da interação entre Lua e C++. Algumas categorias principais:

- **Players**: `nx_SetPlayerPos`, `nx_GetPlayerHealth`, `nx_GivePlayerWeapon`.
- **World**: `nx_CreateObject`, `nx_Create3DTextLabel`, `nx_SetWorldTime`.
- **Vehicles**: `nx_CreateVehicle`, `nx_SetVehiclePos`, `nx_RepairVehicle`.
- **UI**: `nx_TextDrawCreate`, `nx_SendClientMessage`.

> Para uma lista completa, consulte a documentação detalhada em [CAPI/apidocs/api.json](file:///c:/Users/rickd/OneDrive/Documentos/nexorix-server/CAPI/apidocs/api.json).

---

## 🛡️ Segurança e Anti-Cheat

O framework foi projetado sob o princípio de "Desconfie do Cliente".

1. **Validação de Sincronia**: O servidor valida se os movimentos do jogador são fisicamente possíveis antes de replicar para outros.
2. **Anti-Money Hack**: O dinheiro é controlado no lado do servidor. Qualquer alteração feita por cheats no cliente é puramente visual.
3. **Hooks de Proteção**:
   - `OnPlayerUpdate`: Permite cancelar pacotes de sincronia suspeitos.
   - `OnIncomingConnection`: Bloqueio de IPs ou VPNs antes mesmo da conexão ser estabelecida.

### Exemplo de Anti-SpeedHack:

```lua
local lastPos = {}

function OnPlayerUpdate(playerid)
    local x, y, z = nx_GetPlayerPos(playerid)
    
    if lastPos[playerid] then
        local dist = math.sqrt((x - lastPos[playerid].x)^2 + (y - lastPos[playerid].y)^2)
        
        -- Se o jogador se mover mais de 50 metros entre frames de sincronia (sem estar em veículo)
        if dist > 50.0 and not nx_IsPlayerInAnyVehicle(playerid) then
            nx_print(string.format("[ANTICHEAT] %s detectado com Teleport/SpeedHack!", nx_GetPlayerName(playerid)))
            nx_SendClientMessage(playerid, 0xFF0000FF, "Você foi expulso por movimentação suspeita.")
            nx_KickPlayer(playerid)
            return false -- Rejeita o pacote de sincronia
        end
    end
    
    lastPos[playerid] = {x = x, y = y, z = z}
    return true
end
```

---

## 🔄 Hot-Reload (Recarregamento Vivo)

O Nexorix Framework elimina a necessidade de reiniciar o servidor para aplicar mudanças no código. O sistema de **Hot-Reload** permite que você atualize scripts Lua instantaneamente enquanto o servidor está rodando.

### Como funciona?
- **Detecção Automática**: O motor monitora alterações nos arquivos da pasta `lua/`.
- **Persistência de Estado**: O sistema é desenhado para recarregar a lógica sem desconectar os jogadores ou perder dados em memória (quando configurado corretamente).
- **Comando Interno**: Você pode forçar o recarregamento via console ou comando in-game (se implementado no GameMode).

### Exemplo de Comando de Reload:

Você pode criar um comando administrativo para recarregar módulos específicos sem precisar reiniciar o servidor.

```lua
function OnPlayerCommand(playerid, cmdtext)
    local cmd, params = cmdtext:match("^/(%w+)%s*(.*)$")
    
    if cmd == "reload" then
        if not nx_IsPlayerAdmin(playerid) then 
            return nx_SendClientMessage(playerid, 0xFF0000FF, "Erro: Apenas administradores!") 
        end
        
        if params == "" then 
            return nx_SendClientMessage(playerid, 0xFFFF00FF, "Uso: /reload [caminho/do/arquivo.lua]") 
        end
        
        local success, error = nx_reloadScript(params)
        
        if success then
            nx_SendClientMessageToAll(0x00FF00FF, string.format("[SISTEMA] O módulo '%s' foi atualizado!", params))
        else
            nx_SendClientMessage(playerid, 0xFF0000FF, "Erro no Reload: " .. error)
        end
        return true
    end
    return false
end
```

---

## 🏗️ Proteção de Código (Bytecode)

O Nexorix oferece suporte nativo à execução de **Lua Bytecode (5.4)**, permitindo que desenvolvedores protejam sua propriedade intelectual e garantam a integridade dos sistemas em ambientes de produção.

### 🛡️ Benefícios da Compilação
- **Ofuscação de Lógica**: Transforma o código-fonte legível (`.lua`) em um formato binário (`.luac`), impedindo a engenharia reversa e o plágio de sistemas exclusivos.
- **Imutabilidade**: Garante que o código executado no servidor seja exatamente o que você desenvolveu, evitando modificações maliciosas ou acidentais por terceiros.
- **Otimização de Inicialização**: O bytecode pula a fase de análise sintática (parsing), permitindo que scripts complexos sejam carregados instantaneamente.

---

## 🎨 Crie seu Próprio Servidor

O Nexorix foi projetado para que você não precise se preocupar com a complexidade do C++. O objetivo é que você foque 100% na sua criatividade usando Lua.

1. **Estruture sua Lógica**: Use a pasta `lua/modules/` para organizar seus sistemas (Empregos, Concessionárias, Facções).
2. **Desenvolva com Agilidade**: Utilize o sistema de Hot-Reload para testar alterações em tempo real.
3. **Poder Total**: Explore todas as nativas `nx_` para criar experiências únicas e personalizadas.
4. **Comunidade**: Compartilhe seus módulos ou aprenda com outros desenvolvedores na nossa comunidade.

Seja você um iniciante ou um expert, o Nexorix é a tela em branco para o seu próximo grande projeto.

## 📞 Suporte e Comunidade

- **Discord**: [Junte-se à nossa comunidade](https://discord.gg/FmwRjEYXec)
- **Documentação**: [Wiki Oficial](https://wiki.nexorix.com) *(Em breve)*
- **Issues**: [Reportar um bug](https://github.com/nexorix/issues)

---

*Desenvolvido pela **NexorixGroup**. Excelência em Scripting e Performance.*
