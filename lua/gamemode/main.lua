-- ============================================================
--  Nexorix Framework — Gamemode Entry Point
--
--  Este é o ponto de entrada do seu gamemode Lua.
--  Coloque seus módulos na pasta gamemode/modulos/
--
--  Estrutura recomendada:
--    lua/
--    ├── include/
--    │   ├── nexorix.lua          (Framework core - NÃO EDITAR)
--    │   └── nexorix_voice.lua    (Voice API - NÃO EDITAR)
--    └── gamemode/
--        ├── main.lua             (Este arquivo)
--        └── modulos/
--            └── ...              (Seus módulos)
--
--  API disponível:
--    NX.Hook(callback, function)   — Registrar evento
--    NX.RegisterCommand(cmd, fn)   — Registrar comando /cmd
--    NX.Voice.*                    — Sistema de voz
--    nx_*                          — Funções nativas do servidor
--
--  
-- ============================================================

require("include/nexorix")

-- Carregue seus módulos aqui:
-- require("gamemode/modulos/config")
-- require("gamemode/modulos/auth")
-- require("gamemode/modulos/commands")

NX.Hook("OnGameModeInit", function()
    nx_SetGameModeText("Nexorix Lua")
    nx_print("------------------------------------------")
    nx_print(" Nexorix Framework Iniciado!")
    nx_print(" Edite lua/gamemode/main.lua para comecar")
    nx_print("------------------------------------------")
end)