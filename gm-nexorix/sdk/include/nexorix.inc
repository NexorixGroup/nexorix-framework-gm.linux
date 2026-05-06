// ============================================================
//
//   nexorix.inc  —  Include principal do Nexorix SDK
//   Versao: 1.0.0
//
//   Uso: substitua todos os #include por apenas:
//        #include <nexorix>
//
//   O que esta incluido:
//     - Base SA-MP / open.mp
//     - Comandos (zcmd + Pawn.CMD)
//     - Parsing de strings (sscanf2)
//     - Streamer de objetos/veiculos/areas
//     - Banco de dados (MySQL + SQLite nativo)
//     - Iteradores (foreach)
//     - Utilitarios de string, tempo, arquivo
//     - Anti-cheat basico embutido
//     - Cores, macros e helpers prontos
//
// ============================================================

#if defined _nexorix_included
    #endinput
#endif
#define _nexorix_included

#define NEXORIX_VERSION     "1.0.0"
#define NEXORIX_VERSION_INT  100

// ============================================================
//  [1] BASE — SA-MP / open.mp
// ============================================================

#include <a_samp>

// ============================================================
//  [2] PARSING DE STRINGS
// ============================================================

#define SSCANF_NO_NICE_FEATURES
#include <sscanf2>

// ============================================================
//  [3] SISTEMA DE COMANDOS
// ============================================================

// zcmd e Pawn.CMD sao compativeis — zcmd e o padrao
#include <zcmd>

// ============================================================
//  [4] STREAMER (objetos, veiculos, areas dinamicas)
// ============================================================

#if !defined _streamer_included
    #include <streamer>
#endif

// ============================================================
//  [5] ITERADORES
// ============================================================

#if !defined _foreach_included
    #include <foreach>
#endif

// ============================================================
//  [6] BANCO DE DADOS
// ============================================================

// SQLite nativo (a_sampdb ja vem com a_samp, mas garantimos)
#if !defined _sampdb_included
    #include <a_sampdb>
#endif

// MySQL — so inclui se o plugin estiver presente
// Para ativar: coloque mysql.dll em ext/ e descomente abaixo
// #include <mysql>

// ============================================================
//  [7] UTILITARIOS
// ============================================================

#include <string>
#include <file>
#include <time>
#include <float>

// ============================================================
//  [8] CORES PADRAO NEXORIX
// ============================================================

// Basicas
#define NX_BRANCO           0xFFFFFFFF
#define NX_PRETO            0x000000FF
#define NX_CINZA            0xAAAAAFFF
#define NX_CINZA_ESCURO     0x555555FF

// Feedback
#define NX_VERDE            0x44FF44FF   // sucesso / ok
#define NX_VERMELHO         0xFF4444FF   // erro / perigo
#define NX_AMARELO          0xFFFF44FF   // aviso / info
#define NX_AZUL             0x4488FFFF   // sistema / neutro
#define NX_LARANJA          0xFF8800FF   // destaque

// Admin
#define NX_ADMIN            0xFF6600FF
#define NX_MOD              0x00AAFFFF

// Chat
#define NX_CHAT_GLOBAL      0xFFFFFFFF
#define NX_CHAT_LOCAL       0xC8C8C8FF
#define NX_CHAT_OOC         0xE0E0E0FF
#define NX_CHAT_PM          0xFFD700FF

// ============================================================
//  [9] MACROS UTEIS
// ============================================================

// Mensagens rapidas
#define NxMsg(%0,%1,%2)         SendClientMessage(%0, %1, %2)
#define NxMsgAll(%0,%1)         SendClientMessageToAll(%0, %1)
#define NxMsgFmt(%0,%1,%2,%3)   do { new _s[144]; format(_s,sizeof(_s),%2,%3); SendClientMessage(%0,%1,_s); } while(0)

// Verificacoes rapidas
#define NxIsValid(%0)           IsPlayerConnected(%0)
#define NxIsAdmin(%0,%1)        (NxGetAdmin(%0) >= %1)

// Posicao
#define NxGetPos(%0,%1,%2,%3)   GetPlayerPos(%0,%1,%2,%3)
#define NxSetPos(%0,%1,%2,%3)   SetPlayerPos(%0,%1,%2,%3)

// Dinheiro
#define NxDarDinheiro(%0,%1)    GivePlayerMoney(%0,%1)
#define NxSetDinheiro(%0,%1)    do { ResetPlayerMoney(%0); GivePlayerMoney(%0,%1); } while(0)

// Tempo (GetTickCount)
#define NxTick()                GetTickCount()

// ============================================================
//  [10] SISTEMA DE ANTI-CHEAT BASICO
// ============================================================

// Limites de deteccao
#define NX_AC_MAX_SPEED         250.0   // km/h — acima disso e speedhack
#define NX_AC_MAX_HEALTH        100.0   // HP maximo permitido
#define NX_AC_MAX_ARMOUR        100.0   // Armour maximo permitido
#define NX_AC_WEAPON_CHECK      true    // Verificar armas invalidas

// Armas validas no SA-MP: 0 (punho) ate 46
// Qualquer ID acima de 46 e invalido
#define NX_AC_INVALID_WEAPON(%0) (%0 > 46)

// Callback de deteccao — implemente no seu gamemode
// forward OnNxCheatDetected(playerid, tipo[], valor[]);

// Tipos de cheat para o callback
#define NX_CHEAT_SPEEDHACK      "speedhack"
#define NX_CHEAT_HEALTHHACK     "healthhack"
#define NX_CHEAT_ARMOURHACK     "armourhack"
#define NX_CHEAT_WEAPONHACK     "weaponhack"
#define NX_CHEAT_TELEPORT       "teleport"
#define NX_CHEAT_AIRBREAK       "airbreak"

// Verificacao de velocidade (use em OnPlayerUpdate)
stock bool:NxAC_CheckSpeed(playerid)
{
    if (!IsPlayerInAnyVehicle(playerid)) return false;
    new vehicleid = GetPlayerVehicleID(playerid);
    new Float:vx, Float:vy, Float:vz;
    GetVehicleVelocity(vehicleid, vx, vy, vz);
    new Float:speed = floatsqroot(vx*vx + vy*vy + vz*vz) * 180.0;
    if (speed > NX_AC_MAX_SPEED)
    {
        #if defined OnNxCheatDetected
            new val[16];
            format(val, sizeof(val), "%.1f", speed);
            OnNxCheatDetected(playerid, NX_CHEAT_SPEEDHACK, val);
        #endif
        return true;
    }
    return false;
}

// Verificacao de HP/Armour (use em OnPlayerUpdate)
stock bool:NxAC_CheckHealth(playerid)
{
    new pstate = GetPlayerState(playerid);
    if (pstate == 0 || pstate == 7 || pstate == 8) return false;

    new Float:hp, Float:arm;
    GetPlayerHealth(playerid, hp);
    GetPlayerArmour(playerid, arm);
    if (hp > NX_AC_MAX_HEALTH + 0.5)
    {
        #if defined OnNxCheatDetected
            new val[16];
            format(val, sizeof(val), "%.1f", hp);
            OnNxCheatDetected(playerid, NX_CHEAT_HEALTHHACK, val);
        #endif
        return true;
    }
    if (arm > NX_AC_MAX_ARMOUR + 0.5)
    {
        #if defined OnNxCheatDetected
            new val[16];
            format(val, sizeof(val), "%.1f", arm);
            OnNxCheatDetected(playerid, NX_CHEAT_ARMOURHACK, val);
        #endif
        return true;
    }
    return false;
}

// Verificacao de arma invalida
stock bool:NxAC_CheckWeapon(playerid)
{
    new pstate = GetPlayerState(playerid);
    if (pstate == 0 || pstate == 7 || pstate == 8) return false;

    new weapon = GetPlayerWeapon(playerid);
    if (NX_AC_INVALID_WEAPON(weapon))
    {
        #if defined OnNxCheatDetected
            new val[8];
            format(val, sizeof(val), "%d", weapon);
            OnNxCheatDetected(playerid, NX_CHEAT_WEAPONHACK, val);
        #endif
        return true;
    }
    return false;
}

// ============================================================
//  [11] HELPERS DE BANCO DE DADOS (SQLite nativo)
// ============================================================

// Abre ou cria um banco SQLite em data/
stock DB:NxDB_Abrir(const nome[])
{
    new caminho[64];
    format(caminho, sizeof(caminho), "%s.db", nome);
    return db_open(caminho);
}

// Executa uma query simples sem retorno
stock NxDB_Exec(DB:db, const query[])
{
    new DBResult:r = db_query(db, query);
    if (r) db_free_result(r);
}

// Executa query com format (ate 3 parametros string)
stock NxDB_ExecF(DB:db, const fmt[], {Float,_}:...)
{
    new query[512];
    format(query, sizeof(query), fmt, ___(2));
    NxDB_Exec(db, query);
}

// Retorna valor inteiro de uma coluna
stock NxDB_GetInt(DBResult:result, const coluna[])
{
    new val[32];
    db_get_field_assoc(result, coluna, val, sizeof(val));
    return strval(val);
}

// Retorna valor float de uma coluna
stock Float:NxDB_GetFloat(DBResult:result, const coluna[])
{
    new val[32];
    db_get_field_assoc(result, coluna, val, sizeof(val));
    return floatstr(val);
}

// Retorna valor string de uma coluna
stock NxDB_GetStr(DBResult:result, const coluna[], dest[], len)
{
    db_get_field_assoc(result, coluna, dest, len);
}

// ============================================================
//  [12] HELPERS DE TEMPO
// ============================================================

// Retorna string formatada da hora atual: "HH:MM:SS"
stock NxTempo_Hora(dest[], len)
{
    new h, m, s;
    gettime(h, m, s);
    format(dest, len, "%02d:%02d:%02d", h, m, s);
}

// Retorna string formatada da data atual: "DD/MM/AAAA"
stock NxTempo_Data(dest[], len)
{
    new y, mo, d;
    getdate(y, mo, d);
    format(dest, len, "%02d/%02d/%04d", d, mo, y);
}

// Retorna timestamp Unix aproximado
stock NxTempo_Unix()
{
    return gettime();
}

// Converte segundos em string legivel: "2h 30m 15s"
stock NxTempo_Formatar(segundos, dest[], len)
{
    new h = segundos / 3600;
    new m = (segundos % 3600) / 60;
    new s = segundos % 60;
    if (h > 0)      format(dest, len, "%dh %dm %ds", h, m, s);
    else if (m > 0) format(dest, len, "%dm %ds", m, s);
    else            format(dest, len, "%ds", s);
}

// ============================================================
//  [13] HELPERS DE JOGADOR
// ============================================================

// Retorna o nome do jogador direto como string
stock NxNome(playerid, dest[], len)
{
    GetPlayerName(playerid, dest, len);
}

// Retorna IP do jogador
stock NxIP(playerid, dest[], len)
{
    GetPlayerIp(playerid, dest, len);
}

// Kick com mensagem
stock NxKick(playerid, const motivo[])
{
    new str[128];
    format(str, sizeof(str), "[Kick] Voce foi removido: %s", motivo);
    SendClientMessage(playerid, NX_VERMELHO, str);
    // Delay minimo para a mensagem chegar antes do kick
    SetTimerEx("_NxKickTimer", 100, false, "i", playerid);
}

forward _NxKickTimer(playerid);
public  _NxKickTimer(playerid) { Kick(playerid); }

// Ban com mensagem
stock NxBan(playerid, const motivo[])
{
    new str[128];
    format(str, sizeof(str), "[Ban] Voce foi banido: %s", motivo);
    SendClientMessage(playerid, NX_VERMELHO, str);
    SetTimerEx("_NxBanTimer", 100, false, "i", playerid);
}

forward _NxBanTimer(playerid);
public  _NxBanTimer(playerid) { Ban(playerid); }

// Distancia entre dois jogadores
stock Float:NxDistancia(p1, p2)
{
    new Float:x1,Float:y1,Float:z1,Float:x2,Float:y2,Float:z2;
    GetPlayerPos(p1, x1, y1, z1);
    GetPlayerPos(p2, x2, y2, z2);
    return VectorSize(x2-x1, y2-y1, z2-z1);
}

// Distancia de um jogador a um ponto
stock Float:NxDistanciaPonto(playerid, Float:px, Float:py, Float:pz)
{
    new Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);
    return VectorSize(px-x, py-y, pz-z);
}

// ============================================================
//  [14] SISTEMA DE NIVEL DE ADMIN (base)
// ============================================================

// Array global de nivel de admin — declare no seu gamemode:
// new NxAdminLevel[MAX_PLAYERS];
// Ou use a macro NxGetAdmin/NxSetAdmin com seu proprio array.

// Para usar o sistema embutido, descomente:
// #define NX_USE_ADMIN_SYSTEM
#if defined NX_USE_ADMIN_SYSTEM
    new _NxAdmin[MAX_PLAYERS];
    stock NxGetAdmin(playerid)              { return _NxAdmin[playerid]; }
    stock NxSetAdmin(playerid, nivel)       { _NxAdmin[playerid] = nivel; }
    stock bool:NxIsAdminLevel(playerid, nivel) { return (_NxAdmin[playerid] >= nivel); }
#else
    // Stub — o desenvolvedor implementa o proprio sistema
    stock NxGetAdmin(playerid)              { playerid = playerid; return 0; }
    stock NxSetAdmin(playerid, nivel)       { playerid = playerid; nivel = nivel; }
#endif

// ============================================================
//  [15] ANTI-FLOOD GLOBAL
// ============================================================

new _NxFloodTick[MAX_PLAYERS];
#define NX_FLOOD_DELAY  1500   // ms — altere conforme necessario

stock bool:NxFlood(playerid)
{
    new tick = GetTickCount();
    if ((tick - _NxFloodTick[playerid]) < NX_FLOOD_DELAY)
    {
        SendClientMessage(playerid, NX_VERMELHO, "[!] Aguarde antes de usar outro comando.");
        return true;
    }
    _NxFloodTick[playerid] = tick;
    return false;
}

// Reset do flood ao conectar — chame em OnPlayerConnect:
// NxFlood_Reset(playerid);
stock NxFlood_Reset(playerid) { _NxFloodTick[playerid] = 0; }

// ============================================================
//  FIM — nexorix.inc
// ============================================================
