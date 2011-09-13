/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
 * Copyright (C) 2004-2011  Petr Kozelka, PPK at PtokaX dot org

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
#include "stdinc.h"
//---------------------------------------------------------------------------
#include "LuaInc.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "eventqueue.h"
#include "globalQueue.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "LuaScript.h"
//---------------------------------------------------------------------------
#include "IP2Country.h"
#include "LuaCoreLib.h"
#include "LuaBanManLib.h"
#include "LuaIP2CountryLib.h"
#include "LuaProfManLib.h"
#include "LuaRegManLib.h"
#include "LuaScriptManLib.h"
#include "LuaSetManLib.h"
#include "LuaTmrManLib.h"
#include "LuaUDPDbgLib.h"
#include "ResNickManager.h"
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
	#include "../gui.win/GuiUtil.h"
    #include "../gui.win/MainWindowPageScripts.h"
#endif
//---------------------------------------------------------------------------

static int ScriptPanic(lua_State * L) {
    size_t iLen = 0;
    char * stmp = (char*)lua_tolstring(L, -1, &iLen);

    string sMsg = "[LUA] At panic -> " + string(stmp, iLen);

    AppendSpecialLog(sMsg);
    UdpDebug->Broadcast(sMsg);

    return 0;
}
//------------------------------------------------------------------------------

ScriptBot::ScriptBot(char * Nick, const size_t &iNickLen, char * Description, const size_t &iDscrLen, 
    char * Email, const size_t &iEmlLen, const bool &isOP) {
    prev = NULL;
    next = NULL;

#ifdef _WIN32
    sNick = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iNickLen+1);
#else
	sNick = (char *) malloc(iNickLen+1);
#endif
    if(sNick == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iNickLen+1))+
			" bytes of memory for sNick in ScriptBot::ScriptBot!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
		return;
    }
    memcpy(sNick, Nick, iNickLen);
    sNick[iNickLen] = '\0';

    bIsOP = isOP;

    size_t iWantLen = 24+iNickLen+iDscrLen+iEmlLen;

#ifdef _WIN32
    sMyINFO = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iWantLen);
#else
	sMyINFO = (char *) malloc(iWantLen);
#endif
    if(sMyINFO == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)iWantLen)+
			" bytes of memory for sMyINFO in ScriptBot::ScriptBot!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
		return;
    }

	int iLen = sprintf(sMyINFO, "$MyINFO $ALL %s %s$ $$%s$$|", sNick, Description != NULL ? Description : "", Email != NULL ? Email : "");

	CheckSprintf(iLen, iWantLen, "ScriptBot::ScriptBot");

	ScriptManager->ui8BotsCount++;
}
//------------------------------------------------------------------------------

ScriptBot::~ScriptBot() {
    prev = NULL;
    next = NULL;

#ifdef _WIN32
    if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sNick) == 0) {
		string sDbgstr = "[BUF] Cannot deallocate sNick in ~ScriptBot! "+string((uint32_t)GetLastError())+" "+
			string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
		AppendSpecialLog(sDbgstr);
    }
#else
	free(sNick);
#endif

    sNick = NULL;

#ifdef _WIN32
    if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMyINFO) == 0) {
		string sDbgstr = "[BUF] Cannot deallocate sMyINFO in ~ScriptBot! "+string((uint32_t)GetLastError())+" "+
			string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
		AppendSpecialLog(sDbgstr);
    }
#else
	free(sMyINFO);
#endif

    sMyINFO = NULL;

	ScriptManager->ui8BotsCount--;
}
//------------------------------------------------------------------------------

#ifdef _WIN32
	ScriptTimer::ScriptTimer(UINT_PTR uiTmrId, char * sFunctName, const size_t &iLen) {
#else
	ScriptTimer::ScriptTimer(char * sFunctName, const size_t &iLen) {
#endif
	if(sFunctName != NULL) {
#ifdef _WIN32
        sFunctionName = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iLen+1);
#else
		sFunctionName = (char *) malloc(iLen+1);
#endif
        if(sFunctionName == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iLen+1))+
				" bytes of memory for sFunctionName in ScriptTimer::ScriptTimer!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
			return;
        }
        memcpy(sFunctionName, sFunctName, iLen);
        sFunctionName[iLen] = '\0';
    } else {
        sFunctionName = NULL;
    }

    prev = NULL;
    next = NULL;

#ifdef _WIN32
	uiTimerId = uiTmrId;
#else
	TimerId = 0;
#endif
}
//------------------------------------------------------------------------------

ScriptTimer::~ScriptTimer() {
#ifdef _WIN32
	if(sFunctionName != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sFunctionName) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate sFunctionName in ~ScriptTimer! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
    }
#else
	free(sFunctionName);
#endif
}
//------------------------------------------------------------------------------

Script::Script(char *Name, const bool &enabled) {
    bEnabled = enabled;

#ifdef _WIN32
	string ExtractedFilename = ExtractFileName(Name);
    sName = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, ExtractedFilename.size()+1);
#else
    size_t iLen = strlen(Name);

    sName = (char *) malloc(iLen+1);
#endif
    if(sName == NULL) {
#ifdef _WIN32
		string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(ExtractedFilename.size()+1))+
			" bytes of memory in Script::Script! "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#else
		string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iLen+1))+
			" bytes of memory in Script::Script!";
#endif
        AppendSpecialLog(sDbgstr);
		UdpDebug->Broadcast(sDbgstr);
        return;
    }   
#ifdef _WIN32
    memcpy(sName, ExtractedFilename.c_str(), ExtractedFilename.size());
    sName[ExtractedFilename.size()] = '\0';
#else
    memcpy(sName, Name, iLen);
    sName[iLen] = '\0';
#endif

    ui16Functions = 65535;
    ui32DataArrivals = 4294967295U;

    prev = NULL;
    next = NULL;

    BotList = NULL;

    TimerList = NULL;

    LUA = NULL;

    bRegUDP = false;
    bProcessed = false;
}
//------------------------------------------------------------------------------

Script::~Script() {
    if(bRegUDP == true) {
        UdpDebug->Remove(sName);
        bRegUDP = false;
    }

    ScriptTimer * next = TimerList;
    
    while(next != NULL) {
        ScriptTimer * tmr = next;
        next = tmr->next;

#ifdef _WIN32
        if(tmr->uiTimerId != 0) {
            KillTimer(NULL, tmr->uiTimerId);
#else
		if(tmr->TimerId != 0) {
            timer_delete(tmr->TimerId);
#endif
        }

		delete tmr;
    }

    if(LUA != NULL) {
        lua_close(LUA);
    }

#ifdef _WIN32
	if(sName != NULL) {
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sName) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate sName in ~Script! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hRecvHeap, HEAP_NO_SERIALIZE, 0));

            AppendSpecialLog(sDbgstr);
        }
    }
#else
	free(sName);
#endif
}
//------------------------------------------------------------------------------

bool ScriptStart(Script * cur) {
	cur->ui16Functions = 65535;
#ifdef _WIN32
	cur->ui32DataArrivals = 4294967295;
#else
	cur->ui32DataArrivals = 4294967295U;
#endif

	cur->prev = NULL;
	cur->next = NULL;

#ifdef _WIN32
	cur->LUA = lua_newstate(LuaAlocator, NULL);
#else
    cur->LUA = luaL_newstate();
#endif

    if(cur->LUA == NULL) {
        return false;
    }

	luaL_openlibs(cur->LUA);

	lua_atpanic(cur->LUA, ScriptPanic);

	RegCore(cur->LUA);
	RegSetMan(cur->LUA);
	RegRegMan(cur->LUA);
	RegBanMan(cur->LUA);
	RegProfMan(cur->LUA);
	RegTmrMan(cur->LUA);
	RegUDPDbg(cur->LUA);
	RegScriptMan(cur->LUA);
	RegIP2Country(cur->LUA);

	if(luaL_dofile(cur->LUA, (SCRIPT_PATH+cur->sName).c_str()) == 0) {
#ifdef _BUILD_GUI
        RichEditAppendText(pMainWindowPageScripts->hWndPageItems[MainWindowPageScripts::REDT_SCRIPTS_ERRORS],
            (string(LanguageManager->sTexts[LAN_NO_SYNERR_IN_SCRIPT_FILE], (size_t)LanguageManager->ui16TextsLens[LAN_NO_SYNERR_IN_SCRIPT_FILE]) + " " + string(cur->sName)).c_str());
#endif

        return true;
	} else {
        size_t iLen = 0;
        char * stmp = (char*)lua_tolstring(cur->LUA, -1, &iLen);

        string sMsg(stmp, iLen);

#ifdef _BUILD_GUI
        RichEditAppendText(pMainWindowPageScripts->hWndPageItems[MainWindowPageScripts::REDT_SCRIPTS_ERRORS],
            (string(LanguageManager->sTexts[LAN_SYNTAX], (size_t)LanguageManager->ui16TextsLens[LAN_SYNTAX]) + " " + sMsg).c_str());
#endif

		UdpDebug->Broadcast("[LUA] "+sMsg);

        if(SettingManager->bBools[SETBOOL_LOG_SCRIPT_ERRORS] == true) {
            AppendLog(sMsg, true);
        }

		lua_close(cur->LUA);
		cur->LUA = NULL;
    
        return false;
    }
}
//------------------------------------------------------------------------------

void ScriptStop(Script * cur) {
	if(cur->bRegUDP == true) {
        UdpDebug->Remove(cur->sName);
        cur->bRegUDP = false;
    }

	ScriptTimer * tmrnext = cur->TimerList;
    
	while(tmrnext != NULL) {
		ScriptTimer * tmr = tmrnext;
		tmrnext = tmr->next;

#ifdef _WIN32
        if(tmr->uiTimerId != 0) {
            KillTimer(NULL, tmr->uiTimerId);
#else
		if(tmr->TimerId != 0) {
            timer_delete(tmr->TimerId);
#endif
        }

		delete tmr;
    }

    cur->TimerList = NULL;

    if(cur->LUA != NULL) {
        lua_close(cur->LUA);
        cur->LUA = NULL;
    }

    ScriptBot * next = cur->BotList;
    
    while(next != NULL) {
        ScriptBot * bot = next;
        next = bot->next;

        ResNickManager->DelReservedNick(bot->sNick, true);

        if(bServerRunning == true) {
   			colUsers->DelFromNickList(bot->sNick, bot->bIsOP);

            colUsers->DelBotFromMyInfos(bot->sMyINFO);

			int iMsgLen = sprintf(ScriptManager->lua_msg, "$Quit %s|", bot->sNick);
           	if(CheckSprintf(iMsgLen, 128, "ScriptStop") == true) {
                globalQ->InfoStore(ScriptManager->lua_msg, iMsgLen);
            }
		}

		delete bot;
    }

    cur->BotList = NULL;
}
//------------------------------------------------------------------------------

int ScriptGetGC(Script * cur) {
	return lua_gc(cur->LUA, LUA_GCCOUNT, 0);
}
//------------------------------------------------------------------------------

void ScriptOnStartup(Script * cur) {
    lua_getglobal(cur->LUA, "OnStartup");
    int i = lua_gettop(cur->LUA);
    if(lua_isnil(cur->LUA, i)) {
		cur->ui16Functions &= ~Script::ONSTARTUP;
		lua_settop(cur->LUA, 0);
        return;
    }

    if(lua_pcall(cur->LUA, 0, 0, 0) != 0) {
        ScriptError(cur);

        lua_settop(cur->LUA, 0);
        return;
    }

    // clear the stack for sure
    lua_settop(cur->LUA, 0);
}
//------------------------------------------------------------------------------

void ScriptOnExit(Script * cur) {
    lua_getglobal(cur->LUA, "OnExit");
    int i = lua_gettop(cur->LUA);
    if(lua_isnil(cur->LUA, i)) {
		cur->ui16Functions &= ~Script::ONEXIT;
		lua_settop(cur->LUA, 0);
        return;
    }

    if(lua_pcall(cur->LUA, 0, 0, 0) != 0) {
        ScriptError(cur);

        lua_settop(cur->LUA, 0);
        return;
	}

	// clear the stack for sure
    lua_settop(cur->LUA, 0);
}
//------------------------------------------------------------------------------

static bool ScriptOnError(Script * cur, char * ErrorMsg, const size_t &iLen) {
	lua_getglobal(cur->LUA, "OnError");
    int i = lua_gettop(cur->LUA);
    if(lua_isnil(cur->LUA, i)) {
		cur->ui16Functions &= ~Script::ONERROR;
		lua_settop(cur->LUA, 0);
        return true;
    }

	ScriptManager->ActualUser = NULL;
    
    lua_pushlstring(cur->LUA, ErrorMsg, iLen);

	if(lua_pcall(cur->LUA, 1, 0, 0) != 0) { // 1 passed parameters, zero returned
		size_t iLen = 0;
		char * stmp = (char*)lua_tolstring(cur->LUA, -1, &iLen);

		string sMsg(stmp, iLen);

#ifdef _BUILD_GUI
        RichEditAppendText(pMainWindowPageScripts->hWndPageItems[MainWindowPageScripts::REDT_SCRIPTS_ERRORS],
            (string(LanguageManager->sTexts[LAN_SYNTAX], (size_t)LanguageManager->ui16TextsLens[LAN_SYNTAX]) + " " + sMsg).c_str());
        RichEditAppendText(pMainWindowPageScripts->hWndPageItems[MainWindowPageScripts::REDT_SCRIPTS_ERRORS],
            (string(LanguageManager->sTexts[LAN_FATAL_ERR_SCRIPT], (size_t)LanguageManager->ui16TextsLens[LAN_FATAL_ERR_SCRIPT]) + " " + string(cur->sName) + " ! " +
			string(LanguageManager->sTexts[LAN_SCRIPT_STOPPED], (size_t)LanguageManager->ui16TextsLens[LAN_SCRIPT_STOPPED]) + "!").c_str());
#endif

		if(SettingManager->bBools[SETBOOL_LOG_SCRIPT_ERRORS] == true) {
			AppendLog(sMsg, true);
		}

        lua_settop(cur->LUA, 0);
        return false;
    }

    // clear the stack for sure
    lua_settop(cur->LUA, 0);
    return true;
}
//------------------------------------------------------------------------------

void ScriptPushUser(lua_State * L, User * u, const bool &bFullTable/* = false*/) {
	lua_checkstack(L, 3); // we need 3 (1 table, 2 id, 3 value) empty slots in stack, check it to be sure

	lua_newtable(L);
	int i = lua_gettop(L);

	lua_pushliteral(L, "sNick");
	lua_pushlstring(L, u->sNick, u->ui8NickLen);
	lua_rawset(L, i);

	lua_pushliteral(L, "uptr");
	lua_pushlightuserdata(L, (void *)u);
	lua_rawset(L, i);

	lua_pushliteral(L, "sIP");
	lua_pushlstring(L, u->sIP, u->ui8IpLen);
	lua_rawset(L, i);

	lua_pushliteral(L, "iProfile");
	lua_pushnumber(L, u->iProfile);
	lua_rawset(L, i);

    if(bFullTable == true) {
        ScriptPushUserExtended(L, u, i);
    }
}
//------------------------------------------------------------------------------

void ScriptPushUserExtended(lua_State * L, User * u, const int &iTable) {
	lua_pushliteral(L, "sMode");
	if(u->cMode != '\0') {
		lua_pushlstring(L, (char *)&u->cMode, 1);
	} else {
		lua_pushnil(L);
	}
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sMyInfoString");
	if(u->sMyInfoOriginal != NULL) {
		lua_pushlstring(L, u->sMyInfoOriginal, u->ui16MyInfoOriginalLen);
	} else {
		lua_pushnil(L);
	}
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sDescription");
	if(u->sDescription != NULL) {
		lua_pushlstring(L, u->sDescription, (size_t)u->ui8DescriptionLen);
	} else {
		lua_pushnil(L);
	}
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sTag");
	if(u->sTag != NULL) {
		lua_pushlstring(L, u->sTag, (size_t)u->ui8TagLen);
	} else {
		lua_pushnil(L);
	}
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sConnection");
	if(u->sConnection != NULL) {
		lua_pushlstring(L, u->sConnection, (size_t)u->ui8ConnectionLen);
	} else {
		lua_pushnil(L);
	}
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sEmail");
	if(u->sEmail != NULL) {
		lua_pushlstring(L, u->sEmail, (size_t)u->ui8EmailLen);
	} else {
		lua_pushnil(L);
	}
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sClient");
	if(u->sClient != NULL) {
		lua_pushlstring(L, u->sClient, (size_t)u->ui8ClientLen);
	} else {
		lua_pushnil(L);
	}
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sClientVersion");
	if(u->sTagVersion != NULL) {
		lua_pushlstring(L, u->sTagVersion, (size_t)u->ui8TagVersionLen);
	} else {
		lua_pushnil(L);
	}
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sVersion");
	if(u->sVersion != NULL) {
		lua_pushstring(L, u->sVersion);
	} else {
		lua_pushnil(L);
	}
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sCountryCode");
	if(IP2Country->ui32Count != 0) {
		lua_pushlstring(L, IP2Country->GetCountry(u->ui8Country, false), 2);
	} else {
		lua_pushnil(L);
	}
	lua_rawset(L, iTable);

	lua_pushliteral(L, "bConnected");
	u->ui8State == User::STATE_ADDED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "bActive");
	(u->ui32BoolBits & User::BIT_ACTIVE) == User::BIT_ACTIVE ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "bOperator");
	(u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "bUserCommand");
	(u->ui32BoolBits & User::BIT_SUPPORT_USERCOMMAND) == User::BIT_SUPPORT_USERCOMMAND ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "bQuickList");
	(u->ui32BoolBits & User::BIT_SUPPORT_QUICKLIST) == User::BIT_SUPPORT_QUICKLIST ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "bSuspiciousTag");
	(u->ui32BoolBits & User::BIT_HAVE_BADTAG) == User::BIT_HAVE_BADTAG ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "iShareSize");
	lua_pushnumber(L, (double)u->ui64SharedSize);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "iHubs");
	lua_pushnumber(L, u->Hubs);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "iNormalHubs");
	(u->ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(L) : lua_pushnumber(L, u->iNormalHubs);
	lua_rawset(L, iTable);
    
	lua_pushliteral(L, "iRegHubs");
	(u->ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(L) : lua_pushnumber(L, u->iRegHubs);
	lua_rawset(L, iTable);
    
	lua_pushliteral(L, "iOpHubs");
	(u->ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(L) : lua_pushnumber(L, u->iOpHubs);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "iSlots");
	lua_pushnumber(L, u->Slots);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "iLlimit");
	lua_pushnumber(L, u->LLimit);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "iDefloodWarns");
	lua_pushnumber(L, u->iDefloodWarnings);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "iMagicByte");
	lua_pushnumber(L, u->MagicByte);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "iLoginTime");
	lua_pushnumber(L, (double)u->LoginTime);
	lua_rawset(L, iTable);
}
//------------------------------------------------------------------------------

User * ScriptGetUser(lua_State * L, const int &iTop, const char * sFunction) {
    lua_pushliteral(L, "uptr");
    lua_gettable(L, 1);

    if(lua_gettop(L) != iTop+1 || lua_type(L, iTop+1) != LUA_TLIGHTUSERDATA) {
        luaL_error(L, "bad argument #1 to '%s' (it's not user table)", sFunction);
        return NULL;
    }

    User *u = (User *)lua_touserdata(L, iTop+1);
                
    if(u == NULL) {
        return NULL;
    }

	if(u != ScriptManager->ActualUser) {
        lua_pushliteral(L, "sNick");
        lua_gettable(L, 1);

        if(lua_gettop(L) != iTop+2 || lua_type(L, iTop+2) != LUA_TSTRING) {
            luaL_error(L, "bad argument #1 to '%s' (it's not user table)", sFunction);
            return NULL;
        }
    
        size_t iNickLen;
        char * sNick = (char *)lua_tolstring(L, iTop+2, &iNickLen);

		if(u != hashManager->FindUser(sNick, iNickLen)) {
            return NULL;
        }
    }

    return u;
}
//------------------------------------------------------------------------------

void ScriptError(Script * cur) {
	size_t iLen = 0;
	char * stmp = (char*)lua_tolstring(cur->LUA, -1, &iLen);

	string sMsg(stmp, iLen);

#ifdef _BUILD_GUI
    RichEditAppendText(pMainWindowPageScripts->hWndPageItems[MainWindowPageScripts::REDT_SCRIPTS_ERRORS],
        (string(LanguageManager->sTexts[LAN_SYNTAX], (size_t)LanguageManager->ui16TextsLens[LAN_SYNTAX]) + " " + sMsg).c_str());
#endif

	UdpDebug->Broadcast("[LUA] " + sMsg);

    if(SettingManager->bBools[SETBOOL_LOG_SCRIPT_ERRORS] == true) {
        AppendLog(sMsg, true);
    }

	if((((cur->ui16Functions & Script::ONERROR) == Script::ONERROR) == true && ScriptOnError(cur, stmp, iLen) == false) ||
        SettingManager->bBools[SETBOOL_STOP_SCRIPT_ON_ERROR] == true) {
        // PPK ... stop buggy script ;)
		eventqueue->AddNormal(eventq::EVENT_STOPSCRIPT, cur->sName);
    }
}
//------------------------------------------------------------------------------

#ifdef _WIN32
    void ScriptOnTimer(const UINT_PTR &uiTimerId) {
#else
	void ScriptOnTimer(ScriptTimer * AccTimer) {
#endif
	Script *next = ScriptManager->RunningScriptS;

    while(next != NULL) {
    	Script *cur = next;
        next = cur->next;

        ScriptTimer * next = cur->TimerList;
        
        while(next != NULL) {
            ScriptTimer * tmr = next;
            next = tmr->next;

#ifdef _WIN32
            if(tmr->uiTimerId == uiTimerId) {
                if(tmr->sFunctionName == NULL) {
#else
			if(tmr == AccTimer) {
				if(tmr->sFunctionName == NULL) {
#endif
					lua_getglobal(cur->LUA, "OnTimer");
				} else {
					lua_getglobal(cur->LUA, tmr->sFunctionName);
				}

				ScriptManager->ActualUser = NULL;

				lua_checkstack(cur->LUA, 1); // we need 1 empty slots in stack, check it to be sure

#ifdef _WIN32
                lua_pushlightuserdata(cur->LUA, (void *)uiTimerId);
#else
				lua_pushlightuserdata(cur->LUA, (void *)tmr->TimerId);
#endif

				// 1 passed parameters, 0 returned
				if(lua_pcall(cur->LUA, 1, 0, 0) != 0) {
					ScriptError(cur);
					return;
				}

				// clear the stack for sure
				lua_settop(cur->LUA, 0);
				return;
            }
        }

	}
}
//------------------------------------------------------------------------------
