/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
 * Copyright (C) 2004-2010  Petr Kozelka, PPK at PtokaX dot org

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3
 * as published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//---------------------------------------------------------------------------
#ifndef LuaScriptH
#define LuaScriptH
//---------------------------------------------------------------------------
struct User;
struct Script;
//---------------------------------------------------------------------------

struct ScriptBot {
    ScriptBot(char * Nick, const size_t &iNickLen, char * Description, const size_t &iDscrLen, 
        char * Email, const size_t &iEmlLen, const bool &isOP);
    ~ScriptBot();

    char *sNick;
    char *sMyINFO;
    
    ScriptBot *prev, *next;

    bool bIsOP;
};
//------------------------------------------------------------------------------

struct ScriptTimer {
#ifdef _WIN32
    ScriptTimer(UINT_PTR uiTmrId, char * sFunctName, const size_t &iLen, Script * cur);
#else
	ScriptTimer(char * sFunctName, const size_t &iLen, Script * cur);
#endif
    ~ScriptTimer();

#ifdef _WIN32
    UINT_PTR uiTimerId;
#else
	timer_t TimerId;
#endif

    char * sFunctionName;

    ScriptTimer *prev, *next;
};
//------------------------------------------------------------------------------

struct Script {
    Script(char *Name, const bool &enabled);
    ~Script();

    enum LuaFunctions {
		ONSTARTUP         = 0x1,
		ONEXIT            = 0x2,
		ONERROR           = 0x4,
		USERCONNECTED     = 0x8,
		REGCONNECTED      = 0x10,
		OPCONNECTED       = 0x20,
		USERDISCONNECTED  = 0x40,
		REGDISCONNECTED   = 0x80,
		OPDISCONNECTED    = 0x100
	};

    uint32_t ui32DataArrivals;

    char * sName;

    lua_State * LUA;

    Script *prev, *next;

    ScriptBot *BotList;

    ScriptTimer *TimerList;

	uint16_t ui16Functions;

    bool bEnabled, bRegUDP, bProcessed;
};
//------------------------------------------------------------------------------

bool ScriptStart(Script * cur);
void ScriptStop(Script * cur);

int ScriptGetGC(Script * cur);
#ifdef _WIN32
	void ScriptGetGC(Script * cur, const uint32_t &i);
#endif

void ScriptOnStartup(Script * cur);
void ScriptOnExit(Script * cur);

void ScriptPushUser(lua_State * L, User * u, const bool &bFullTable = false);
void ScriptPushUserExtended(lua_State * L, User * u, const int &iTable);

User * ScriptGetUser(lua_State * L, const int &iTop, const char * sFunction);

void ScriptError(Script * cur);

#ifdef _WIN32
    #ifndef _SERVICE
        VOID CALLBACK ScriptTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
        VOID CALLBACK ScriptTimerCustomProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
    #else
        void ScriptOnTimer(const UINT_PTR &uiTimerId);
    #endif
#else
	void ScriptOnTimer(ScriptTimer * AccTimer);
#endif
//------------------------------------------------------------------------------

#endif
