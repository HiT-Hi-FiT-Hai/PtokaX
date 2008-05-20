/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
 * Copyright (C) 2004-2008  Petr Kozelka, PPK at PtokaX dot org

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.

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

    sNick = (char *) malloc(iNickLen+1);
    if(sNick == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iNickLen+1))+
			" bytes of memory for sNick in ScriptBot::ScriptBot!";
		AppendSpecialLog(sDbgstr);
		return;
    }
    memcpy(sNick, Nick, iNickLen);
    sNick[iNickLen] = '\0';

    bIsOP = isOP;

    size_t iWantLen = 24+iNickLen+iDscrLen+iEmlLen;

    sMyINFO = (char *) malloc(iWantLen);
    if(sMyINFO == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)iWantLen)+
			" bytes of memory for sMyINFO in ScriptBot::ScriptBot!";
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

    free(sNick);
    sNick = NULL;

    free(sMyINFO);
    sMyINFO = NULL;

	ScriptManager->ui8BotsCount--;
}
//------------------------------------------------------------------------------

ScriptTimer::ScriptTimer(char * sFunctName, const size_t &iLen, Script * cur) {
	if(sFunctName != NULL) {
        sFunctionName = (char *) malloc(iLen+1);
        if(sFunctionName == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iLen+1))+
				" bytes of memory for sFunctionName in ScriptTimer::ScriptTimer!";
			AppendSpecialLog(sDbgstr);
			return;
        }
        memcpy(sFunctionName, sFunctName, iLen);
        sFunctionName[iLen] = '\0';
    } else {
        sFunctionName = NULL;
    }

    TimerId = 0;

    prev = NULL;

    if(cur->TimerList == NULL) {
        next = NULL;
    } else {
        next = cur->TimerList;
        cur->TimerList->prev = this;
    }

    cur->TimerList = this;
}
//------------------------------------------------------------------------------

ScriptTimer::~ScriptTimer() {
	if(sFunctionName != NULL) {
        free(sFunctionName);
        sFunctionName = NULL;
    }
}
//------------------------------------------------------------------------------

Script::Script(char *Name, const bool &enabled) {
    bEnabled = enabled;

    size_t iLen = strlen(Name);

    sName = (char *) malloc(iLen+1);
    if(sName == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iLen+1))+
			" bytes of memory in Script::Script!";
        AppendSpecialLog(sDbgstr);
		UdpDebug->Broadcast(sDbgstr);
        return;
    }   
    memcpy(sName, Name, iLen);
    sName[iLen] = '\0';

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

        if(tmr->TimerId != 0) {
            timer_delete(tmr->TimerId);
        }

		delete tmr;
    }

    if(LUA != NULL) {
        lua_close(LUA);
    }

	if(sName != NULL) {
		free(sName);
		sName = NULL;
    }
}
//------------------------------------------------------------------------------

bool ScriptStart(Script * cur) {
	cur->ui16Functions = 65535;
	cur->ui32DataArrivals = 4294967295U;

	cur->prev = NULL;
	cur->next = NULL;

	cur->LUA = lua_open();

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
        return true;
	} else {
        size_t iLen = 0;
        char * stmp = (char*)lua_tolstring(cur->LUA, -1, &iLen);

        string sMsg(stmp, iLen);

		UdpDebug->Broadcast("[LUA] "+sMsg);

        if(SettingManager->bBools[SETBOOL_LOG_SCRIPT_ERRORS] == true) {
            AppendLog(sMsg, true);
        }

		lua_close(cur->LUA);
		cur->LUA = NULL;

        cur->bEnabled = false;
    
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

        if(tmr->TimerId != 0) {
            timer_delete(tmr->TimerId);
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
	lua_pushlstring(L, u->Nick, u->NickLen);
	lua_rawset(L, i);

	lua_pushliteral(L, "uptr");
	lua_pushlightuserdata(L, (void *)u);
	lua_rawset(L, i);

	lua_pushliteral(L, "sIP");
	lua_pushlstring(L, u->IP, u->ui8IpLen);
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
	u->Mode != '\0' ? lua_pushlstring(L, (char *)&u->Mode, 1) : lua_pushnil(L);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sMyInfoString");
	u->MyInfoTag ? lua_pushlstring(L, u->MyInfoTag, u->iMyInfoTagLen) : lua_pushnil(L);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sDescription");
	u->Description != NULL ? lua_pushlstring(L, u->Description, (size_t)u->ui8DescrLen) : lua_pushnil(L);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sTag");
	u->Tag ? lua_pushlstring(L, u->Tag, (size_t)u->ui8TagLen) : lua_pushnil(L);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sConnection");
	u->Connection != NULL ? lua_pushlstring(L, u->Connection, (size_t)u->ui8ConnLen) : lua_pushnil(L);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sEmail");
	u->Email != NULL ? lua_pushlstring(L, u->Email, (size_t)u->ui8EmailLen) : lua_pushnil(L);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sClient");
	u->Client != NULL ? lua_pushlstring(L, u->Client, (size_t)u->ui8ClientLen) : lua_pushnil(L);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sClientVersion");
	u->Ver != NULL ? lua_pushlstring(L, u->Ver, (size_t)u->ui8VerLen) : lua_pushnil(L);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sVersion");
	u->Version ? lua_pushstring(L, u->Version) : lua_pushnil(L);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sCountryCode");
	IP2Country->ui32Count != 0 ? lua_pushlstring(L, IP2Country->GetCountry(u->ui8Country, false), 2) : lua_pushnil(L);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "bConnected");
	u->iState == User::STATE_ADDED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
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
	lua_pushnumber(L, (double)u->sharedSize);
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

void ScriptOnTimer(ScriptTimer * AccTimer) {
	Script *next = ScriptManager->RunningScriptS;

    while(next != NULL) {
    	Script *cur = next;
        next = cur->next;

        ScriptTimer * next = cur->TimerList;

        while(next != NULL) {
            ScriptTimer * tmr = next;
            next = tmr->next;

			if(tmr == AccTimer) {
				if(tmr->sFunctionName == NULL) {
					lua_getglobal(cur->LUA, "OnTimer");
				} else {
					lua_getglobal(cur->LUA, tmr->sFunctionName);
				}

				ScriptManager->ActualUser = NULL;

				lua_checkstack(cur->LUA, 1); // we need 1 empty slots in stack, check it to be sure

				lua_pushlightuserdata(cur->LUA, (void *)tmr->TimerId);

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
