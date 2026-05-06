# 🚀 Nexorix Framework & Mode GameMode

Bem-vindo ao **Nexorix**, um framework de alto desempenho para servidores multiplayer, focado em modularidade, velocidade e facilidade de desenvolvimento usando **Lua 5.4**. Junto a ele, apresentamos o **Mode**, o GameMode base construído sobre esta arquitetura.

---

## 🛠️ O Framework Nexorix

O Nexorix foi desenhado para ser leve e extensível, permitindo que desenvolvedores criem sistemas complexos sem se preocupar com conflitos de callbacks ou lentidão de processamento.

### Principais Características:
- **Core em Lua 5.4**: Aproveite as últimas funcionalidades da linguagem Lua para uma lógica de script rápida e moderna.
- **Sistema de Hooks (NX.Hook)**: Registre múltiplos ouvintes para o mesmo callback com controle de prioridade. Chega de conflitos entre scripts!
- **Hot Reload Nativo**: Atualize seus scripts em tempo real com o comando `/reload`. Desenvolva sem precisar reiniciar o servidor.
- **Arquitetura Modular**: Pasta `core/` para o motor do framework e `systems/` para suas funcionalidades.
- **Namespace NX**: Todas as ferramentas e dados globais organizados sob uma única tabela protegida.

---

## 🎮 Mode GameMode

O **Mode** é o ponto de partida ideal para seu servidor. Ele já vem com as necessidades básicas de qualquer projeto:

### Funcionalidades Inclusas:
- **Sistema de Contas**: Registro e Login integrados via **SQLite**.
- **Persistência de Dados**: Salvamento automático de Dinheiro, Score e Posição.
- **Comandos Base**: `/ping`, `/salvar`, `/pos`, `/ms` (skins) e `/carro`.
- **Integração Discord**: Conector bidirecional para logs, status do servidor e comandos remotos via bot.
- **Feedback Visual**: Interface via TextDraws otimizada para logs de sistema e notificações.

---

## 📂 Estrutura de Pastas

```text
lua/
├── core/               # O coração do framework
│   ├── hooks.lua       # Gerenciador de eventos e prioridades
│   ├── hotreload.lua   # Lógica de recarregamento ao vivo
│   └── init.lua        # Inicialização e constantes globais
├── systems/            # Seus sistemas modulares
│   └── discord.lua     # Ponte entre Servidor e Discord
└── main.lua            # O GameMode 'Mode' (lógica principal)
```

---

## 🚀 Como Iniciar

1. Certifique-se de ter o componente Lua instalado no seu servidor Nexorix.
2. Coloque a pasta `lua/` dentro do diretório do seu servidor.
3. Configure o seu `config.json` para carregar o script principal:
   ```json
   "lua": {
       "main_scripts": ["main"]
   }
   ```
4. Inicie o servidor e aproveite a velocidade do Nexorix!

---

## ⌨️ Comandos de Desenvolvedor

- `/reload <arquivo.lua>`: Recarrega um script específico.
- `/reload <pasta/>`: Recarrega todos os arquivos de uma pasta.
- `/reload all`: Recarrega todo o framework e o gamemode instantaneamente.

---

## ✍️ Créditos

- **Autor**: RickZin021
- **Base**: Nexorix Framework
- **Linguagem**: Lua 5.4

---
*Desenvolvido com ❤️ para a comunidade Nexorix.*
