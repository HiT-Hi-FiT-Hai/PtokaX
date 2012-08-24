/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
 * Copyright (C) 2004-2012  Petr Kozelka, PPK at PtokaX dot org

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
#include "GlobalDataQueue.h"
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
    size_t szLen = 0;
    char * stmp = (char*)lua_tolstring(L, -1, &szLen);

    string sMsg = "[LUA] At panic -> " + string(stmp, szLen);

    AppendLog(sMsg);

    return 0;
}
//------------------------------------------------------------------------------

ScriptBot::ScriptBot(char * sBotNick, const size_t &szNickLen, char * sDescription, const size_t &szDscrLen, char * sEmail, const size_t &szEmlLen, const bool &bOP) {
    ScriptManager->ui8BotsCount++;

    prev = NULL;
    next = NULL;

#ifdef _WIN32
    sNick = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szNickLen+1);
#else
	sNick = (char *)malloc(szNickLen+1);
#endif
    if(sNick == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sNick in ScriptBot::ScriptBot\n", (uint64_t)(szNickLen+1));

		return;
    }
    memcpy(sNick, sBotNick, szNickLen);
    sNick[szNickLen] = '\0';

    bIsOP = bOP;

    size_t szWantLen = 24+szNickLen+szDscrLen+szEmlLen;

#ifdef _WIN32
    sMyINFO = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szWantLen);
#else
	sMyINFO = (char *)malloc(szWantLen);
#endif
    if(sMyINFO == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sMyINFO in ScriptBot::ScriptBot\n", (uint64_t)szWantLen);

		return;
    }

	int iLen = sprintf(sMyINFO, "$MyINFO $ALL %s %s$ $$%s$$|", sNick, sDescription != NULL ? sDescription : "", sEmail != NULL ? sEmail : "");

	CheckSprintf(iLen, szWantLen, "ScriptBot::ScriptBot");
}
//------------------------------------------------------------------------------

ScriptBot::~ScriptBot() {
#ifdef _WIN32
    if(sNick != NULL && HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sNick) == 0) {
		AppendDebugLog("%s - [MEM] Cannot deallocate sNick in ScriptBot::~ScriptBot\n", 0);
    }
#else
	free(sNick);
#endif

#ifdef _WIN32
    if(sMyINFO != NULL && HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMyINFO) == 0) {
		AppendDebugLog("%s - [MEM] Cannot deallocate sMyINFO in ScriptBot::~ScriptBot\n", 0);
    }
#else
	free(sMyINFO);
#endif

	ScriptManager->ui8BotsCount--;
}
//------------------------------------------------------------------------------

#ifdef _WIN32
	ScriptTimer::ScriptTimer(UINT_PTR uiTmrId, char * sFunctName, const size_t &szLen) {
#else
	ScriptTimer::ScriptTimer(char * sFunctName, const size_t &szLen) {
#endif
	if(sFunctName != NULL) {
#ifdef _WIN32
        sFunctionName = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szLen+1);
#else
		sFunctionName = (char *)malloc(szLen+1);
#endif
        if(sFunctionName == NULL) {
			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sFunctionName in ScriptTimer::ScriptTimer\n", (uint64_t)(szLen+1));

			return;
        }

        memcpy(sFunctionName, sFunctName, szLen);
        sFunctionName[szLen] = '\0';
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
			AppendDebugLog("%s - [MEM] Cannot deallocate sFunctionName in ScriptTimer::~ScriptTimer\n", 0);
        }
    }
#else
	free(sFunctionName);
#endif
}
//------------------------------------------------------------------------------

Script::Script(char * Name, const bool &enabled) {
    bEnabled = enabled;

#ifdef _WIN32
	string ExtractedFilename = ExtractFileName(Name);
	size_t szNameLen = ExtractedFilename.size();

    sName = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szNameLen+1);
#else
    size_t szNameLen = strlen(Name);

    sName = (char *)malloc(szNameLen+1);
#endif
    if(sName == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes in Script::Script\n", (uint64_t)szNameLen+1);

        return;
    }   
#ifdef _WIN32
    memcpy(sName, ExtractedFilename.c_str(), ExtractedFilename.size());
#else
    memcpy(sName, Name, szNameLen);
#endif
    sName[szNameLen] = '\0';

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
	if(sName != NULL && HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sName) == 0) {
        AppendDebugLog("%s - [MEM] Cannot deallocate sName in Script::~Script\n", 0);
    }
#else
	free(sName);
#endif
}
//------------------------------------------------------------------------------

bool ScriptStart(Script * cur) {
	cur->ui16Functions = 65535;
	cur->ui32DataArrivals = 4294967295U;

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

#if LUA_VERSION_NUM == 501
	RegCore(cur->LUA);
	RegSetMan(cur->LUA);
	RegRegMan(cur->LUA);
	RegBanMan(cur->LUA);
	RegProfMan(cur->LUA);
	RegTmrMan(cur->LUA);
	RegUDPDbg(cur->LUA);
	RegScriptMan(cur->LUA);
	RegIP2Country(cur->LUA);
#else
    luaL_requiref(cur->LUA, "Core", RegCore, 1);
    lua_pop(cur->LUA, 1);

    luaL_requiref(cur->LUA, "SetMan", RegSetMan, 1);
    lua_pop(cur->LUA, 1);

    luaL_requiref(cur->LUA, "RegMan", RegRegMan, 1);
    lua_pop(cur->LUA, 1);

    luaL_requiref(cur->LUA, "BanMan", RegBanMan, 1);
    lua_pop(cur->LUA, 1);

    luaL_requiref(cur->LUA, "ProfMan", RegProfMan, 1);
    lua_pop(cur->LUA, 1);

    luaL_requiref(cur->LUA, "TmrMan", RegTmrMan, 1);
    lua_pop(cur->LUA, 1);

    luaL_requiref(cur->LUA, "UDPDbg", RegUDPDbg, 1);
    lua_pop(cur->LUA, 1);

    luaL_requiref(cur->LUA, "ScriptMan", RegScriptMan, 1);
    lua_pop(cur->LUA, 1);

    luaL_requiref(cur->LUA, "IP2Country", RegIP2Country, 1);
    lua_pop(cur->LUA, 1);
#endif

	if(luaL_dofile(cur->LUA, (SCRIPT_PATH+cur->sName).c_str()) == 0) {
#ifdef _BUILD_GUI
        RichEditAppendText(pMainWindowPageScripts->hWndPageItems[MainWindowPageScripts::REDT_SCRIPTS_ERRORS],
            (string(LanguageManager->sTexts[LAN_NO_SYNERR_IN_SCRIPT_FILE], (size_t)LanguageManager->ui16TextsLens[LAN_NO_SYNERR_IN_SCRIPT_FILE]) + " " + string(cur->sName)).c_str());
#endif

        return true;
	} else {
        size_t szLen = 0;
        char * stmp = (char*)lua_tolstring(cur->LUA, -1, &szLen);

        string sMsg(stmp, szLen);

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

			int iMsgLen = sprintf(g_sBuffer, "$Quit %s|", bot->sNick);
           	if(CheckSprintf(iMsgLen, g_szBufferSize, "ScriptStop") == true) {
                g_GlobalDataQueue->AddQueueItem(g_sBuffer, iMsgLen, NULL, 0, GlobalDataQueue::CMD_QUIT);
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

static bool ScriptOnError(Script * cur, char * sErrorMsg, const size_t &szMsgLen) {
	lua_getglobal(cur->LUA, "OnError");
    int i = lua_gettop(cur->LUA);
    if(lua_isnil(cur->LUA, i)) {
		cur->ui16Functions &= ~Script::ONERROR;
		lua_settop(cur->LUA, 0);
        return true;
    }

	ScriptManager->ActualUser = NULL;
    
    lua_pushlstring(cur->LUA, sErrorMsg, szMsgLen);

	if(lua_pcall(cur->LUA, 1, 0, 0) != 0) { // 1 passed parameters, zero returned
		size_t szLen = 0;
		char * stmp = (char*)lua_tolstring(cur->LUA, -1, &szLen);

		string sMsg(stmp, szLen);

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
	if(u->sModes[0] != '\0') {
		lua_pushstring(L, u->sModes);
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
	if((u->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
        (u->ui32BoolBits & User::BIT_IPV6_ACTIVE) == User::BIT_IPV6_ACTIVE ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
    } else {
	   (u->ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
    }
	lua_rawset(L, iTable);

	lua_pushliteral(L, "bOperator");
	(u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "bUserCommand");
	(u->ui32SupportBits & User::SUPPORTBIT_USERCOMMAND) == User::SUPPORTBIT_USERCOMMAND ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "bQuickList");
	(u->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
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

	lua_pushliteral(L, "sMac");
    char sMac[18];
    if(GetMacAddress(u->sIP, sMac) == true) {
        lua_pushlstring(L, sMac, 17);
    } else {
        lua_pushnil(L);
    }
	lua_rawset(L, iTable);

    lua_pushliteral(L, "bDescriptionChanged");
    (u->ui32InfoBits & User::INFOBIT_DESCRIPTION_CHANGED) == User::INFOBIT_DESCRIPTION_CHANGED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
    lua_rawset(L, iTable);

    lua_pushliteral(L, "bTagChanged");
    (u->ui32InfoBits & User::INFOBIT_TAG_CHANGED) == User::INFOBIT_TAG_CHANGED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
    lua_rawset(L, iTable);

    lua_pushliteral(L, "bConnectionChanged");
    (u->ui32InfoBits & User::INFOBIT_CONNECTION_CHANGED) == User::INFOBIT_CONNECTION_CHANGED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
    lua_rawset(L, iTable);

    lua_pushliteral(L, "bEmailChanged");
    (u->ui32InfoBits & User::INFOBIT_EMAIL_CHANGED) == User::INFOBIT_EMAIL_CHANGED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
    lua_rawset(L, iTable);

    lua_pushliteral(L, "bShareChanged");
    (u->ui32InfoBits & User::INFOBIT_SHARE_CHANGED) == User::INFOBIT_SHARE_CHANGED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
    lua_rawset(L, iTable);

    lua_pushliteral(L, "sScriptedDescriptionShort");
    if(u->sChangedDescriptionShort != NULL) {
		lua_pushlstring(L, u->sChangedDescriptionShort, u->ui8ChangedDescriptionShortLen);
	} else {
		lua_pushnil(L);
	}
    lua_rawset(L, iTable);

    lua_pushliteral(L, "sScriptedDescriptionLong");
    if(u->sChangedDescriptionLong != NULL) {
		lua_pushlstring(L, u->sChangedDescriptionLong, u->ui8ChangedDescriptionLongLen);
	} else {
		lua_pushnil(L);
	}
    lua_rawset(L, iTable);

    lua_pushliteral(L, "sScriptedTagShort");
    if(u->sChangedTagShort != NULL) {
		lua_pushlstring(L, u->sChangedTagShort, u->ui8ChangedTagShortLen);
	} else {
		lua_pushnil(L);
	}
    lua_rawset(L, iTable);

    lua_pushliteral(L, "sScriptedTagLong");
    if(u->sChangedTagLong != NULL) {
		lua_pushlstring(L, u->sChangedTagLong, u->ui8ChangedTagLongLen);
	} else {
		lua_pushnil(L);
	}
    lua_rawset(L, iTable);

    lua_pushliteral(L, "sScriptedConnectionShort");
    if(u->sChangedConnectionShort != NULL) {
		lua_pushlstring(L, u->sChangedConnectionShort, u->ui8ChangedConnectionShortLen);
	} else {
		lua_pushnil(L);
	}
    lua_rawset(L, iTable);

    lua_pushliteral(L, "sScriptedConnectionLong");
    if(u->sChangedConnectionLong != NULL) {
		lua_pushlstring(L, u->sChangedConnectionLong, u->ui8ChangedConnectionLongLen);
	} else {
		lua_pushnil(L);
	}
    lua_rawset(L, iTable);

    lua_pushliteral(L, "sScriptedEmailShort");
    if(u->sChangedEmailShort != NULL) {
		lua_pushlstring(L, u->sChangedEmailShort, u->ui8ChangedEmailShortLen);
	} else {
		lua_pushnil(L);
	}
    lua_rawset(L, iTable);

    lua_pushliteral(L, "sScriptedEmailLong");
    if(u->sChangedEmailLong != NULL) {
		lua_pushlstring(L, u->sChangedEmailLong, u->ui8ChangedEmailLongLen);
	} else {
		lua_pushnil(L);
	}
    lua_rawset(L, iTable);

    lua_pushliteral(L, "iScriptediShareSizeShort");
    lua_pushnumber(L, (double)u->ui64ChangedSharedSizeShort);
    lua_rawset(L, iTable);

    lua_pushliteral(L, "iScriptediShareSizeLong");
    lua_pushnumber(L, (double)u->ui64ChangedSharedSizeLong);
    lua_rawset(L, iTable);

    lua_pushliteral(L, "tIPs");
    lua_newtable(L);

    int t = lua_gettop(L);

	lua_pushnumber(L, 1);
	lua_pushlstring(L, u->sIP, u->ui8IpLen);
	lua_rawset(L, t);

    if(u->sIPv4[0] != '\0') {
        lua_pushnumber(L, 2);
        lua_pushlstring(L, u->sIPv4, u->ui8IPv4Len);
        lua_rawset(L, t);
    }

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
    
        size_t szNickLen;
        char * sNick = (char *)lua_tolstring(L, iTop+2, &szNickLen);

		if(u != hashManager->FindUser(sNick, szNickLen)) {
            return NULL;
        }
    }

    return u;
}
//------------------------------------------------------------------------------

void ScriptError(Script * cur) {
	size_t szLen = 0;
	char * stmp = (char*)lua_tolstring(cur->LUA, -1, &szLen);

	string sMsg(stmp, szLen);

#ifdef _BUILD_GUI
    RichEditAppendText(pMainWindowPageScripts->hWndPageItems[MainWindowPageScripts::REDT_SCRIPTS_ERRORS],
        (string(LanguageManager->sTexts[LAN_SYNTAX], (size_t)LanguageManager->ui16TextsLens[LAN_SYNTAX]) + " " + sMsg).c_str());
#endif

	UdpDebug->Broadcast("[LUA] " + sMsg);

    if(SettingManager->bBools[SETBOOL_LOG_SCRIPT_ERRORS] == true) {
        AppendLog(sMsg, true);
    }

	if((((cur->ui16Functions & Script::ONERROR) == Script::ONERROR) == true && ScriptOnError(cur, stmp, szLen) == false) ||
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
