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
#include "LuaCoreLib.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "eventqueue.h"
#include "globalQueue.h"
#include "hashBanManager.h"
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
#include "IP2Country.h"
#include "ResNickManager.h"
#include "LuaScript.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#ifndef _MSC_VER
		#pragma package(smart_init)
	#endif
#endif
//---------------------------------------------------------------------------

static int Restart(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'Restart' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

	eventqueue->AddNormal(eventq::EVENT_RESTART, NULL);

    return 0;
}
//------------------------------------------------------------------------------

static int Shutdown(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'Shutdown' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

	eventqueue->AddNormal(eventq::EVENT_SHUTDOWN, NULL);

    return 0;
}
//------------------------------------------------------------------------------

static int ResumeAccepts(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'ResumeAccepts' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

	ServerResumeAccepts();

    return 0;
}
//------------------------------------------------------------------------------

static int SuspendAccepts(lua_State * L) {
    int n = lua_gettop(L);

    if(n == 0) {
        ServerSuspendAccepts(0);
    } else if(n == 1) {
        if(lua_type(L, 1) != LUA_TNUMBER) {
            luaL_checktype(L, 1, LUA_TNUMBER);
    		lua_settop(L, 0);
            return 0;
        }
    
    	uint32_t iSec = (uint32_t)lua_tonumber(L, 1);
    
        if(iSec != 0) {
            ServerSuspendAccepts(iSec);
        }
        lua_settop(L, 0);
    } else {
        luaL_error(L, "bad argument count to 'SuspendAccepts' (0 or 1 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
    }
    
    return 0;
}
//------------------------------------------------------------------------------

static int RegBot(lua_State * L) {
    if(ScriptManager->ui8BotsCount > 63) {
        lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }

	if(lua_gettop(L) != 4) {
        luaL_error(L, "bad argument count to 'RegBot' (4 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }

    if(lua_type(L, 1) != LUA_TSTRING || lua_type(L, 2) != LUA_TSTRING || lua_type(L, 3) != LUA_TSTRING || lua_type(L, 4) != LUA_TBOOLEAN) {
        luaL_checktype(L, 1, LUA_TSTRING);
        luaL_checktype(L, 2, LUA_TSTRING);
        luaL_checktype(L, 3, LUA_TSTRING);
        luaL_checktype(L, 4, LUA_TBOOLEAN);

		lua_settop(L, 0);

		lua_pushnil(L);
        return 1;
    }

    size_t iNickLen, iDescrLen, iEmailLen;

    char *nick = (char *)lua_tolstring(L, 1, &iNickLen);
    char *description = (char *)lua_tolstring(L, 2, &iDescrLen);
    char *email = (char *)lua_tolstring(L, 3, &iEmailLen);

    bool bIsOP = lua_toboolean(L, 4) == 0 ? false : true;

    if(iNickLen == 0 || iNickLen > 64 || strpbrk(nick, " $|<>:?*\"/\\") != NULL ||
        iDescrLen > 64 || strpbrk(description, "$|") != NULL ||
        iEmailLen > 64 || strpbrk(email, "$|") != NULL ||
		hashManager->FindUser(nick, iNickLen) != NULL ||
        ResNickManager->CheckReserved(nick, HashNick(nick, iNickLen)) != false) {
		lua_settop(L, 0);

		lua_pushnil(L);
        return 1;
    }

    ScriptBot *bot = new ScriptBot(nick, iNickLen, description, iDescrLen, email, iEmailLen, bIsOP);
    if(bot == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate new ScriptBot!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
        AppendSpecialLog(sDbgstr);

		lua_settop(L, 0);

		lua_pushnil(L);
        return 1;
    }

    // PPK ... finally we can clear stack
    lua_settop(L, 0);

	Script * obj = ScriptManager->FindScript(L);
	if(obj == NULL) {
		delete bot;

		lua_pushnil(L);
        return 1;
    }

    ScriptBot * next = obj->BotList;

    while(next != NULL) {
        ScriptBot * cur = next;
        next = cur->next;

#ifdef _WIN32
  	    if(stricmp(bot->sNick, cur->sNick) == 0) {
#else
		if(strcasecmp(bot->sNick, cur->sNick) == 0) {
#endif
            delete bot;

            lua_pushnil(L);
            return 1;
        }
    }

    if(obj->BotList == NULL) {
        obj->BotList = bot;
    } else {
        obj->BotList->prev = bot;
        bot->next = obj->BotList;
        obj->BotList = bot;
    }

	ResNickManager->AddReservedNick(bot->sNick, true);

	colUsers->Add2NickList(bot->sNick, iNickLen, bot->bIsOP);

    colUsers->AddBot2MyInfos(bot->sMyINFO);

    // PPK ... fixed hello sending only to users without NoHello
    int iMsgLen = sprintf(ScriptManager->lua_msg, "$Hello %s|", bot->sNick);
    if(CheckSprintf(iMsgLen, 131072, "RegBot") == true) {
        globalQ->HStore(ScriptManager->lua_msg, iMsgLen);
    }
    
    globalQ->InfoStore(bot->sMyINFO, strlen(bot->sMyINFO));
        
    if(bot->bIsOP == true) {
        globalQ->OpListStore(bot->sNick);
    }

    lua_pushboolean(L, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int UnregBot(lua_State * L) {
	if(lua_gettop(L) != 1) {
        luaL_error(L, "bad argument count to 'UnregBot' (1 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }

    if(lua_type(L, 1) != LUA_TSTRING) {
        luaL_checktype(L, 1, LUA_TSTRING);

		lua_settop(L, 0);

		lua_pushnil(L);
        return 1;
    }

    size_t iLen;
    char *botnick = (char *)lua_tolstring(L, 1, &iLen);
    
    if(iLen == 0) {
		lua_settop(L, 0);

		lua_pushnil(L);
        return 1;
    }

	Script * obj = ScriptManager->FindScript(L);
	if(obj == NULL) {
		lua_settop(L, 0);

		lua_pushnil(L);
        return 1;
    }

    ScriptBot * next = obj->BotList;

    while(next != NULL) {
        ScriptBot * cur = next;
        next = cur->next;

#ifdef _WIN32
  	    if(stricmp(botnick, cur->sNick) == 0) {
#else
		if(strcasecmp(botnick, cur->sNick) == 0) {
#endif
            ResNickManager->DelReservedNick(cur->sNick, true);

            colUsers->DelFromNickList(cur->sNick, cur->bIsOP);

            colUsers->DelBotFromMyInfos(cur->sMyINFO);

            int iMsgLen = sprintf(ScriptManager->lua_msg, "$Quit %s|", cur->sNick);
            if(CheckSprintf(iMsgLen, 131072, "UnregBot") == true) {
                globalQ->InfoStore(ScriptManager->lua_msg, iMsgLen);
            }

            if(cur->prev == NULL) {
                if(cur->next == NULL) {
                    obj->BotList = NULL;
                } else {
                    cur->next->prev = NULL;
                    obj->BotList = cur->next;
                }
            } else if(cur->next == NULL) {
                cur->prev->next = NULL;
            } else {
                cur->prev->next = cur->next;
                cur->next->prev = cur->prev;
            }

            delete cur;

            lua_settop(L, 0);

            lua_pushboolean(L, 1);
            return 1;
        }
    }

    lua_settop(L, 0);

    lua_pushnil(L);
    return 1;
}
//------------------------------------------------------------------------------

static int GetBots(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'GetBots' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    lua_newtable(L);

    int t = lua_gettop(L), i = 0;

	Script *next = ScriptManager->RunningScriptS;

    while(next != NULL) {
    	Script *cur = next;
        next = cur->next;

        ScriptBot * next = cur->BotList;
        
        while(next != NULL) {
            ScriptBot * bot = next;
            next = bot->next;

            lua_pushnumber(L, ++i);

            lua_newtable(L);

            int b = lua_gettop(L);

        	lua_pushliteral(L, "sNick");
        	lua_pushstring(L, bot->sNick);
        	lua_rawset(L, b);

        	lua_pushliteral(L, "sMyINFO");
        	lua_pushstring(L, bot->sMyINFO);
        	lua_rawset(L, b);

        	lua_pushliteral(L, "bIsOP");
        	bot->bIsOP == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
        	lua_rawset(L, b);

        	lua_pushliteral(L, "sScriptName");
        	lua_pushstring(L, cur->sName);
        	lua_rawset(L, b);

            lua_rawset(L, t);
        }

    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetActualUsersPeak(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'GetActualUsersPeak' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }
    
    lua_pushnumber(L, ui32Peak);

    return 1;
}
//------------------------------------------------------------------------------

static int GetMaxUsersPeak(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'GetMaxUsersPeak' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }
    
    lua_pushnumber(L, SettingManager->iShorts[SETSHORT_MAX_USERS_PEAK]);

    return 1;
}
//------------------------------------------------------------------------------

static int GetCurrentSharedSize(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'GetCurrentSharedSize' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }
    
    lua_pushnumber(L, (double)ui64TotalShare);

    return 1;
}
//------------------------------------------------------------------------------

static int GetHubIP(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'GetHubIP' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }

    if(sHubIP[0] != '\0') {
		lua_pushstring(L, sHubIP);
	} else {
		lua_pushnil(L);
	}

    return 1;
}
//------------------------------------------------------------------------------

static int GetHubSecAlias(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'GetHubSecAlias' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }
    
    lua_pushlstring(L, SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
        (size_t)SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_HUB_SEC]);

    return 1;
}
//------------------------------------------------------------------------------

static int GetPtokaXPath(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'GetPtokaXPath' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }

#ifdef _WIN32
    lua_pushlstring(L, PATH_LUA.c_str(), PATH_LUA.size());
#else
    string path = PATH+"/";
    lua_pushlstring(L, path.c_str(), path.size());
#endif

    return 1;
}
//------------------------------------------------------------------------------

static int GetUsersCount(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'GetUsersCount' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }
    
    lua_pushnumber(L, ui32Logged);

    return 1;
}
//------------------------------------------------------------------------------

static int GetUpTime(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'GetUpTime' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }

	time_t t;
    time(&t);

    lua_pushnumber(L, (double)(t-starttime));

    return 1;
}
//------------------------------------------------------------------------------

static int GetOnlineByOpStatus(lua_State * L, const bool &bOperator) {
    bool bFullTable = false;

    int n = lua_gettop(L);

	if(n != 0) {
        if(n != 1) {
            if(bOperator == true) {
                luaL_error(L, "bad argument count to 'GetOnlineOps' (0 or 1 expected, got %d)", lua_gettop(L));
            } else {
                luaL_error(L, "bad argument count to 'GetOnlineNonOps' (0 or 1 expected, got %d)", lua_gettop(L));
            }
            lua_settop(L, 0);
    
            lua_pushnil(L);
            return 1;
        } else if(lua_type(L, 1) != LUA_TBOOLEAN) {
            luaL_checktype(L, 1, LUA_TBOOLEAN);

            lua_settop(L, 0);

            lua_pushnil(L);
            return 1;
        } else {
            bFullTable = lua_toboolean(L, 1) == 0 ? false : true;

            lua_settop(L, 0);
        }
    }

    lua_newtable(L);

    int t = lua_gettop(L), i = 0;

    User *next = colUsers->llist;

    while(next != NULL) {
        User *curUser = next;
        next = curUser->next;

        if(curUser->iState == User::STATE_ADDED && ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == bOperator) {
            lua_pushnumber(L, ++i);
			ScriptPushUser(L, curUser, bFullTable);
            lua_rawset(L, t);
        }
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetOnlineNonOps(lua_State * L) {
	return GetOnlineByOpStatus(L, false);
}
//------------------------------------------------------------------------------

static int GetOnlineOps(lua_State * L) {
    return GetOnlineByOpStatus(L, true);
}
//------------------------------------------------------------------------------

static int GetOnlineRegs(lua_State * L) {
    bool bFullTable = false;

    int n = lua_gettop(L);

	if(n != 0) {
        if(n != 1) {
            luaL_error(L, "bad argument count to 'GetOnlineRegs' (0 or 1 expected, got %d)", lua_gettop(L));
            lua_settop(L, 0);
    
            lua_pushnil(L);
            return 1;
        } else if(lua_type(L, 1) != LUA_TBOOLEAN) {
            luaL_checktype(L, 1, LUA_TBOOLEAN);

            lua_settop(L, 0);

            lua_pushnil(L);
            return 1;
        } else {
            bFullTable = lua_toboolean(L, 1) == 0 ? false : true;

            lua_settop(L, 0);
        }
    }

    lua_newtable(L);

    int t = lua_gettop(L), i = 0;

    User *next = colUsers->llist;

    while(next != NULL) {
        User *curUser = next;
        next = curUser->next;
        if(curUser->iState == User::STATE_ADDED && curUser->iProfile != -1) {
            lua_pushnumber(L, ++i);
			ScriptPushUser(L, curUser, bFullTable);
            lua_rawset(L, t);
        }
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetOnlineUsers(lua_State * L) {
    bool bFullTable = false;

    int32_t iProfile = -2;

    int n = lua_gettop(L);

    if(n == 2) {
        if(lua_type(L, 1) != LUA_TNUMBER || lua_type(L, 2) != LUA_TBOOLEAN) {
            luaL_checktype(L, 1, LUA_TNUMBER);
            luaL_checktype(L, 2, LUA_TBOOLEAN);
    
            lua_settop(L, 0);
    
            lua_pushnil(L);
            return 1;
        }

        iProfile = (int32_t)lua_tonumber(L, 1);
        bFullTable = lua_toboolean(L, 2) == 0 ? false : true;

        lua_settop(L, 0);
    } else if(n == 1) {
        if(lua_type(L, 1) == LUA_TNUMBER) {
            iProfile = (int32_t)lua_tonumber(L, 1);
        } else if(lua_type(L, 1) == LUA_TBOOLEAN) {
            bFullTable = lua_toboolean(L, 1) == 0 ? false : true;
        } else {
            luaL_error(L, "bad argument #1 to 'GetOnlineUsers' (number or boolean expected, got %s)", lua_typename(L, lua_type(L, 1)));
            lua_settop(L, 0);
        
            lua_pushnil(L);
            return 1;
        }

        lua_settop(L, 0);
    } else if(n != 0) {
        luaL_error(L, "bad argument count to 'GetOnlineUsers' (0, 1 or 2 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        
        lua_pushnil(L);
        return 1;
    }

    lua_newtable(L);

    int t = lua_gettop(L), i = 0;

    User *next = colUsers->llist;

    if(iProfile == -2) {
	    while(next != NULL) {
	        User *curUser = next;
	        next = curUser->next;

	        if(curUser->iState == User::STATE_ADDED) {
	            lua_pushnumber(L, ++i);
				ScriptPushUser(L, curUser, bFullTable);
	            lua_rawset(L, t);
	        }
	    }
    } else {
		while(next != NULL) {
		    User *curUser = next;
		    next = curUser->next;

		    if(curUser->iState == User::STATE_ADDED && curUser->iProfile == iProfile) {
		        lua_pushnumber(L, ++i);
				ScriptPushUser(L, curUser, bFullTable);
		        lua_rawset(L, t);
		    }
		}
    }
    
    return 1;
}
//------------------------------------------------------------------------------

static int GetUser(lua_State * L) {
    bool bFullTable = false;

    size_t iLen = 0;

    char * nick;
    
    int n = lua_gettop(L);

    if(n == 2) {
        if(lua_type(L, 1) != LUA_TSTRING || lua_type(L, 2) != LUA_TBOOLEAN) {
            luaL_checktype(L, 1, LUA_TSTRING);
            luaL_checktype(L, 2, LUA_TBOOLEAN);
    
            lua_settop(L, 0);
    
            lua_pushnil(L);
            return 1;
        }

        nick = (char *)lua_tolstring(L, 1, &iLen);

        bFullTable = lua_toboolean(L, 2) == 0 ? false : true;
    } else if(n == 1) {
        if(lua_type(L, 1) != LUA_TSTRING) {
            luaL_checktype(L, 1, LUA_TSTRING);

            lua_settop(L, 0);
    
            lua_pushnil(L);
            return 1;
        }

        nick = (char *)lua_tolstring(L, 1, &iLen);
    } else {
        luaL_error(L, "bad argument count to 'GetUser' (1 or 2 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        
        lua_pushnil(L);
        return 1;
    }

    if(iLen == 0) {
        lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }
    
    User *u = hashManager->FindUser(nick, iLen);

    lua_settop(L, 0);

    if(u != NULL) {
		ScriptPushUser(L, u, bFullTable);
	} else {
        lua_pushnil(L);
    }
    
    return 1;
}
//------------------------------------------------------------------------------

static int GetUsers(lua_State * L) {
    bool bFullTable = false;

    size_t iLen = 0;

    char * sIP;
    
    int n = lua_gettop(L);

    if(n == 2) {
        if(lua_type(L, 1) != LUA_TSTRING || lua_type(L, 2) != LUA_TBOOLEAN) {
            luaL_checktype(L, 1, LUA_TSTRING);
            luaL_checktype(L, 2, LUA_TBOOLEAN);
    
            lua_settop(L, 0);
    
            lua_pushnil(L);
            return 1;
        }

        sIP = (char *)lua_tolstring(L, 1, &iLen);

        bFullTable = lua_toboolean(L, 2) == 0 ? false : true;
    } else if(n == 1) {
        if(lua_type(L, 1) != LUA_TSTRING) {
            luaL_checktype(L, 1, LUA_TSTRING);

            lua_settop(L, 0);
    
            lua_pushnil(L);
            return 1;
        }

        sIP = (char *)lua_tolstring(L, 1, &iLen);
    } else {
        luaL_error(L, "bad argument count to 'GetUsers' (1 or 2 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        
        lua_pushnil(L);
        return 1;
    }

    uint32_t ui32Hash = 0;

    if(iLen == 0 || HashIP(sIP, iLen, ui32Hash) == false) {
        lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }

    User *next = hashManager->FindUser(ui32Hash);

    lua_settop(L, 0);

    if(next == NULL) {
        lua_pushnil(L);
        return 1;
    }

    lua_newtable(L);
    
    int t = lua_gettop(L), i = 0;
    
    while(next != NULL) {
		User *curUser = next;
        next = curUser->hashiptablenext;

        lua_pushnumber(L, ++i);
    	ScriptPushUser(L, curUser, bFullTable);
        lua_rawset(L, t);
    }
    
    return 1;
}
//------------------------------------------------------------------------------

static int GetUserAllData(lua_State * L) {
	if(lua_gettop(L) != 1) {
        luaL_error(L, "bad argument count to 'GetUserAllData' (1 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    if(lua_type(L, 1) != LUA_TTABLE) {
        luaL_checktype(L, 1, LUA_TTABLE);
		lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    User *u = ScriptGetUser(L, 1, "GetUserAllData");

    if(u == NULL) {
		lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    ScriptPushUserExtended(L, u, 1);

    lua_settop(L, 0);

    lua_pushboolean(L, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int GetUserData(lua_State * L) {
	if(lua_gettop(L) != 2) {
        luaL_error(L, "bad argument count to 'GetUserData' (2 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    if(lua_type(L, 1) != LUA_TTABLE || lua_type(L, 2) != LUA_TNUMBER) {
        luaL_checktype(L, 1, LUA_TTABLE);
        luaL_checktype(L, 2, LUA_TNUMBER);
		lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    User *u = ScriptGetUser(L, 2, "GetUserData");

    if(u == NULL) {
		lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    uint8_t ui8DataId = (uint8_t)lua_tonumber(L, 2);

    switch(ui8DataId) {
        case 0:
        	lua_pushliteral(L, "sMode");
        	if(u->Mode != '\0') {
				lua_pushlstring(L, (char *)&u->Mode, 1);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        case 1:
        	lua_pushliteral(L, "sMyInfoString");
        	if(u->MyInfoTag != NULL) {
				lua_pushlstring(L, u->MyInfoTag, u->iMyInfoTagLen);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        case 2:
        	lua_pushliteral(L, "sDescription");
        	if(u->Description != NULL) {
				lua_pushlstring(L, u->Description, (size_t)u->ui8DescrLen);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        case 3:
        	lua_pushliteral(L, "sTag");
        	if(u->Tag != NULL) {
				lua_pushlstring(L, u->Tag, (size_t)u->ui8TagLen);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        case 4:
        	lua_pushliteral(L, "sConnection");
        	if(u->Connection != NULL) {
				lua_pushlstring(L, u->Connection, (size_t)u->ui8ConnLen);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        case 5:
        	lua_pushliteral(L, "sEmail");
        	if(u->Email != NULL) {
				lua_pushlstring(L, u->Email, (size_t)u->ui8EmailLen);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        case 6:
        	lua_pushliteral(L, "sClient");
        	if(u->Client != NULL) {
				lua_pushlstring(L, u->Client, (size_t)u->ui8ClientLen);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        case 7:
        	lua_pushliteral(L, "sClientVersion");
        	if(u->Ver != NULL) {
				lua_pushlstring(L, u->Ver, (size_t)u->ui8VerLen);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        case 8:
        	lua_pushliteral(L, "sVersion");
        	if(u->Version!= NULL) {
				lua_pushstring(L, u->Version);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        case 9:
        	lua_pushliteral(L, "bConnected");
        	u->iState == User::STATE_ADDED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	lua_rawset(L, 1);
            break;
        case 10:
        	lua_pushliteral(L, "bActive");
        	(u->ui32BoolBits & User::BIT_ACTIVE) == User::BIT_ACTIVE ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	lua_rawset(L, 1);
            break;
        case 11:
        	lua_pushliteral(L, "bOperator");
        	(u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	lua_rawset(L, 1);
            break;
        case 12:
        	lua_pushliteral(L, "bUserCommand");
        	(u->ui32BoolBits & User::BIT_SUPPORT_USERCOMMAND) == User::BIT_SUPPORT_USERCOMMAND ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	lua_rawset(L, 1);
            break;
        case 13:
        	lua_pushliteral(L, "bQuickList");
        	(u->ui32BoolBits & User::BIT_SUPPORT_QUICKLIST) == User::BIT_SUPPORT_QUICKLIST ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	lua_rawset(L, 1);
            break;
        case 14:
        	lua_pushliteral(L, "bSuspiciousTag");
        	(u->ui32BoolBits & User::BIT_HAVE_BADTAG) == User::BIT_HAVE_BADTAG ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	lua_rawset(L, 1);
            break;
        case 15:
        	lua_pushliteral(L, "iProfile");
        	lua_pushnumber(L, u->iProfile);
        	lua_rawset(L, 1);
            break;
        case 16:
        	lua_pushliteral(L, "iShareSize");
        	lua_pushnumber(L, (double)u->sharedSize);
        	lua_rawset(L, 1);
            break;
        case 17:
        	lua_pushliteral(L, "iHubs");
        	lua_pushnumber(L, u->Hubs);
        	lua_rawset(L, 1);
            break;
        case 18:
        	lua_pushliteral(L, "iNormalHubs");
        	(u->ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(L) : lua_pushnumber(L, u->iNormalHubs);
        	lua_rawset(L, 1);
            break;
        case 19:
        	lua_pushliteral(L, "iRegHubs");
        	(u->ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(L) : lua_pushnumber(L, u->iRegHubs);
        	lua_rawset(L, 1);
            break;
        case 20:
        	lua_pushliteral(L, "iOpHubs");
        	(u->ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(L) : lua_pushnumber(L, u->iOpHubs);
        	lua_rawset(L, 1);
            break;
        case 21:
        	lua_pushliteral(L, "iSlots");
        	lua_pushnumber(L, u->Slots);
        	lua_rawset(L, 1);
            break;
        case 22:
        	lua_pushliteral(L, "iLlimit");
        	lua_pushnumber(L, u->LLimit);
        	lua_rawset(L, 1);
            break;
        case 23:
        	lua_pushliteral(L, "iDefloodWarns");
        	lua_pushnumber(L, u->iDefloodWarnings);
        	lua_rawset(L, 1);
            break;
        case 24:
        	lua_pushliteral(L, "iMagicByte");
        	lua_pushnumber(L, u->MagicByte);
        	lua_rawset(L, 1);
            break;   
        case 25:
        	lua_pushliteral(L, "iLoginTime");
        	lua_pushnumber(L, (double)u->LoginTime);
        	lua_rawset(L, 1);
            break;
        case 26:
        	lua_pushliteral(L, "sCountryCode");
        	if(IP2Country->ui32Count != 0) {
				lua_pushlstring(L, IP2Country->GetCountry(u->ui8Country, false), 2);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        default:
            luaL_error(L, "bad argument #2 to 'GetUserData' (it's not valid id)");
    		lua_settop(L, 0);
    
            lua_pushnil(L);
            return 1;       
    }

    lua_settop(L, 0);

    lua_pushboolean(L, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int GetUserValue(lua_State * L) {
	if(lua_gettop(L) != 2) {
        luaL_error(L, "bad argument count to 'GetUserValue' (2 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    if(lua_type(L, 1) != LUA_TTABLE || lua_type(L, 2) != LUA_TNUMBER) {
        luaL_checktype(L, 1, LUA_TTABLE);
        luaL_checktype(L, 2, LUA_TNUMBER);
		lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    User *u = ScriptGetUser(L, 2, "GetUserValue");

    if(u == NULL) {
		lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    uint8_t ui8DataId = (uint8_t)lua_tonumber(L, 2);

    lua_settop(L, 0);

    switch(ui8DataId) {
        case 0:
        	if(u->Mode != '\0') {
				lua_pushlstring(L, (char *)&u->Mode, 1);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 1:
        	if(u->MyInfoTag!= NULL) {
				lua_pushlstring(L, u->MyInfoTag, u->iMyInfoTagLen);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 2:
        	if(u->Description != NULL) {
				lua_pushlstring(L, u->Description, (size_t)u->ui8DescrLen);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 3:
        	if(u->Tag != NULL) {
				lua_pushlstring(L, u->Tag, (size_t)u->ui8TagLen);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 4:
        	if(u->Connection != NULL) {
				lua_pushlstring(L, u->Connection, (size_t)u->ui8ConnLen);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 5:
        	if(u->Email != NULL) {
				lua_pushlstring(L, u->Email, (size_t)u->ui8EmailLen);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 6:
        	if(u->Client != NULL) {
				lua_pushlstring(L, u->Client, (size_t)u->ui8ClientLen);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 7:
        	if(u->Ver != NULL) {
				lua_pushlstring(L, u->Ver, (size_t)u->ui8VerLen);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 8:
        	if(u->Version != NULL) {
				lua_pushstring(L, u->Version);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 9:
        	u->iState == User::STATE_ADDED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	return 1;
        case 10:
        	(u->ui32BoolBits & User::BIT_ACTIVE) == User::BIT_ACTIVE ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	return 1;
        case 11:
        	(u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	return 1;
        case 12:
        	(u->ui32BoolBits & User::BIT_SUPPORT_USERCOMMAND) == User::BIT_SUPPORT_USERCOMMAND ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	return 1;
        case 13:
        	(u->ui32BoolBits & User::BIT_SUPPORT_QUICKLIST) == User::BIT_SUPPORT_QUICKLIST ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	return 1;
        case 14:
        	(u->ui32BoolBits & User::BIT_HAVE_BADTAG) == User::BIT_HAVE_BADTAG ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	return 1;
        case 15:
        	lua_pushnumber(L, u->iProfile);
        	return 1;
        case 16:
        	lua_pushnumber(L, (double)u->sharedSize);
        	return 1;
        case 17:
        	lua_pushnumber(L, u->Hubs);
        	return 1;
        case 18:
        	(u->ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(L) : lua_pushnumber(L, u->iNormalHubs);
        	return 1;
        case 19:
        	(u->ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(L) : lua_pushnumber(L, u->iRegHubs);
        	return 1;
        case 20:
        	(u->ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(L) : lua_pushnumber(L, u->iOpHubs);
        	return 1;
        case 21:
        	lua_pushnumber(L, u->Slots);
        	return 1;
        case 22:
        	lua_pushnumber(L, u->LLimit);
        	return 1;
        case 23:
        	lua_pushnumber(L, u->iDefloodWarnings);
        	return 1;
        case 24:
        	lua_pushnumber(L, u->MagicByte);
        	return 1;  
        case 25:
        	lua_pushnumber(L, (double)u->LoginTime);
        	return 1;
        case 26:
        	if(IP2Country->ui32Count != 0) {
				lua_pushlstring(L, IP2Country->GetCountry(u->ui8Country, false), 2);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 27: {
#ifdef _WIN32
            uint32_t uiIP = ::inet_addr(u->IP);
            MIB_IPNETTABLE * pINT = (MIB_IPNETTABLE *)&ScriptManager->lua_msg;
            ULONG ulSize = 131072;
            DWORD dwRes = ::GetIpNetTable(pINT, &ulSize, TRUE);
            if(dwRes == NO_ERROR) {
                for(DWORD dwi = 0; dwi < pINT->dwNumEntries; dwi++) {
                    if(pINT->table[dwi].dwAddr == uiIP && pINT->table[dwi].dwType != MIB_IPNET_TYPE_INVALID) {
                        char sMac[18];
                        sprintf(sMac, "%02x:%02x:%02x:%02x:%02x:%02x", pINT->table[dwi].bPhysAddr[0], pINT->table[dwi].bPhysAddr[1], pINT->table[dwi].bPhysAddr[2],
                            pINT->table[dwi].bPhysAddr[3], pINT->table[dwi].bPhysAddr[4], pINT->table[dwi].bPhysAddr[5]);
                        lua_pushlstring(L, sMac, 17);
                        return 1;
                    }
                }
            }
#else
            FILE *fp = fopen("/proc/net/arp", "r");
            if(fp != NULL) {
                char buf[1024];
                while(fgets(buf, 1024, fp) != NULL) {
                    if(strncmp(buf, u->IP, u->ui8IpLen) == 0 && buf[u->ui8IpLen] == ' ') {
                        bool bLastCharSpace = true;
                        uint8_t ui8NonSpaces = 0;
                        uint16_t ui16i = u->ui8IpLen;
                        while(buf[ui16i] != '\0') {
                            if(buf[ui16i] == ' ') {
                                bLastCharSpace = true;
                            } else {
                                if(bLastCharSpace == true) {
                                    bLastCharSpace = false;
                                    ui8NonSpaces++;
                                }
                            }

                            if(ui8NonSpaces == 3) {
                                lua_pushlstring(L, buf + ui16i, 17);
                                fclose(fp);
                                return 1;
                            }

                            ui16i++;
                        }
                    }
                }

                fclose(fp);
            }
#endif
            lua_pushnil(L);
            return 1;
        }
        default:
            luaL_error(L, "bad argument #2 to 'GetUserValue' (it's not valid id)");
            
            lua_pushnil(L);
            return 1;
    }
}
//------------------------------------------------------------------------------

static int Disconnect(lua_State * L) {
    if(lua_gettop(L) != 1) {
        luaL_error(L, "bad argument count to 'Disconnect' (1 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    User * u;

    if(lua_type(L, 1) == LUA_TTABLE) {
        u = ScriptGetUser(L, 1, "Disconnect");

        if(u == NULL) {
    		lua_settop(L, 0);
            lua_pushnil(L);
            return 1;
        }
    } else if(lua_type(L, 1) == LUA_TSTRING) {
        size_t iLen;
        char *nick = (char *)lua_tolstring(L, 1, &iLen);
    
        if(iLen == 0) {
            return 0;
        }
    
        u = hashManager->FindUser(nick, iLen);

        if(u == NULL) {
    		lua_settop(L, 0);
    
            lua_pushnil(L);
            return 1;
        }
    } else {
        luaL_error(L, "bad argument #1 to 'Disconnect' (user table or string expected, got %s)", lua_typename(L, lua_type(L, 1)));
        lua_settop(L, 0);
        
        lua_pushnil(L);
        return 1;
    }

//    int imsgLen = sprintf(ScriptManager->lua_msg, "[SYS] User %s (%s) disconnected by script.", u->Nick, u->IP);
//    UdpDebug->Broadcast(ScriptManager->lua_msg, imsgLen);
    UserClose(u);

    lua_settop(L, 0);

    lua_pushboolean(L, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int Kick(lua_State * L) {
    if(lua_gettop(L) != 3) {
        luaL_error(L, "bad argument count to 'Kick' (3 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    if(lua_type(L, 1) != LUA_TTABLE || lua_type(L, 2) != LUA_TSTRING || lua_type(L, 3) != LUA_TSTRING) {
        luaL_checktype(L, 1, LUA_TTABLE);
        luaL_checktype(L, 2, LUA_TSTRING);
        luaL_checktype(L, 3, LUA_TSTRING);
		lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }

    User *u = ScriptGetUser(L, 3, "Kick");

    if(u == NULL) {
		lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    size_t iKickerLen, iReasonLen;
    char *sKicker = (char *)lua_tolstring(L, 2, &iKickerLen);
    char *sReason = (char *)lua_tolstring(L, 3, &iReasonLen);

    if(iKickerLen == 0 || iKickerLen > 64 || iReasonLen == 0 || iReasonLen > 128000) {
    	lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }

    hashBanManager->TempBan(u, sReason, sKicker, 0, 0, false);

    int imsgLen = sprintf(ScriptManager->lua_msg, "<%s> %s: %s|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
		LanguageManager->sTexts[LAN_YOU_BEING_KICKED_BCS], sReason);
    if(CheckSprintf(imsgLen, 131072, "Kick5") == true) {
    	UserSendCharDelayed(u, ScriptManager->lua_msg, imsgLen);
    }

    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
    	if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
    	    imsgLen = sprintf(ScriptManager->lua_msg, "%s $<%s> *** %s %s IP %s %s %s %s: %s|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], u->Nick, LanguageManager->sTexts[LAN_WITH_LWR], 
                u->IP, LanguageManager->sTexts[LAN_WAS_KICKED_BY], sKicker, 
                LanguageManager->sTexts[LAN_BECAUSE_LWR], sReason);
            if(CheckSprintf(imsgLen, 131072, "Kick6") == true) {
				QueueDataItem *newItem = globalQ->CreateQueueDataItem(ScriptManager->lua_msg, imsgLen, NULL, 0, globalqueue::PM2OPS);
                globalQ->SingleItemsStore(newItem);
            }
    	} else {
    	    imsgLen = sprintf(ScriptManager->lua_msg, "<%s> *** %s %s IP %s %s %s %s: %s|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], u->Nick, 
                LanguageManager->sTexts[LAN_WITH_LWR], u->IP, LanguageManager->sTexts[LAN_WAS_KICKED_BY], sKicker, 
                LanguageManager->sTexts[LAN_BECAUSE_LWR], sReason);
            if(CheckSprintf(imsgLen, 131072, "Kick7") == true) {
                globalQ->OPStore(ScriptManager->lua_msg, imsgLen);
            }
    	}
    }

    // disconnect the user
    imsgLen = sprintf(ScriptManager->lua_msg, "[SYS] User %s (%s) kicked by script.", u->Nick, u->IP);
    if(CheckSprintf(imsgLen, 131072, "Kick8") == true) {
        UdpDebug->Broadcast(ScriptManager->lua_msg, imsgLen);
    }

    UserClose(u);
    
    lua_settop(L, 0);

    lua_pushboolean(L, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int Redirect(lua_State * L) {
    if(lua_gettop(L) != 3) {
        luaL_error(L, "bad argument count to 'Redirect' (3 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    if(lua_type(L, 1) != LUA_TTABLE || lua_type(L, 2) != LUA_TSTRING || lua_type(L, 3) != LUA_TSTRING) {
        luaL_checktype(L, 1, LUA_TTABLE);
        luaL_checktype(L, 2, LUA_TSTRING);
        luaL_checktype(L, 3, LUA_TSTRING);
		lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    User *u = ScriptGetUser(L, 3, "Redirect");

    if(u == NULL) {
		lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    size_t iAddressLen, iReasonLen;
    char *sAddress = (char *)lua_tolstring(L, 2, &iAddressLen);
    char *sReason = (char *)lua_tolstring(L, 3, &iReasonLen);

    if(iAddressLen == 0 || iAddressLen > 1024 || iReasonLen == 0 || iReasonLen > 128000) {
		lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }

    int imsgLen = sprintf(ScriptManager->lua_msg, "<%s> %s %s. %s: %s|$ForceMove %s|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
        LanguageManager->sTexts[LAN_YOU_REDIR_TO], sAddress, LanguageManager->sTexts[LAN_MESSAGE], sReason, sAddress);
    if(CheckSprintf(imsgLen, 131072, "Redirect2") == true) {
        UserSendChar(u, ScriptManager->lua_msg, imsgLen);
    }

    //int imsgLen = sprintf(ScriptManager->lua_msg, "[SYS] User %s (%s) redirected by script.", u->Nick, u->IP);
    //UdpDebug->Broadcast(ScriptManager->lua_msg, imsgLen);

    UserClose(u);
    
    lua_settop(L, 0);

    lua_pushboolean(L, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int DefloodWarn(lua_State * L) {
    if(lua_gettop(L) != 1) {
        luaL_error(L, "bad argument count to 'DefloodWarn' (1 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    if(lua_type(L, 1) != LUA_TTABLE) {
        luaL_checktype(L, 1, LUA_TTABLE);
		lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }

    User *u = ScriptGetUser(L, 1, "DefloodWarn");

    if(u == NULL) {
		lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    u->iDefloodWarnings++;

    if(u->iDefloodWarnings >= (uint32_t)SettingManager->iShorts[SETSHORT_DEFLOOD_WARNING_COUNT]) {
        int imsgLen;
        switch(SettingManager->iShorts[SETSHORT_DEFLOOD_WARNING_ACTION]) {
            case 0:
                break;
            case 1:
				hashBanManager->TempBan(u, LanguageManager->sTexts[LAN_FLOODING], NULL, 0, 0, false);
                break;
            case 2:
                hashBanManager->TempBan(u, LanguageManager->sTexts[LAN_FLOODING], NULL, 
                    SettingManager->iShorts[SETSHORT_DEFLOOD_TEMP_BAN_TIME], 0, false);
                break;
            case 3: {
                hashBanManager->Ban(u, LanguageManager->sTexts[LAN_FLOODING], NULL, false);
                break;
            }
            default:
                break;
        }

        if(SettingManager->bBools[SETBOOL_DEFLOOD_REPORT] == true) {
            if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                imsgLen = sprintf(ScriptManager->lua_msg, "%s $<%s> *** %s %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                    SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_FLOODER], 
                    u->Nick, LanguageManager->sTexts[LAN_WITH_IP], u->IP, 
                    LanguageManager->sTexts[LAN_DISCONN_BY_SCRIPT]);
                if(CheckSprintf(imsgLen, 131072, "DefloodWarn1") == true) {
                    QueueDataItem *newItem = globalQ->CreateQueueDataItem(ScriptManager->lua_msg, imsgLen, NULL, 0, globalqueue::PM2OPS);
                    globalQ->SingleItemsStore(newItem);
                }
            } else {
                imsgLen = sprintf(ScriptManager->lua_msg, "<%s> *** %s %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                    LanguageManager->sTexts[LAN_FLOODER], u->Nick, LanguageManager->sTexts[LAN_WITH_IP], 
                    u->IP, LanguageManager->sTexts[LAN_DISCONN_BY_SCRIPT]);
                if(CheckSprintf(imsgLen, 131072, "DefloodWarn2") == true) {
                    globalQ->OPStore(ScriptManager->lua_msg, imsgLen);
                }
            }
        }

        imsgLen = sprintf(ScriptManager->lua_msg, "[SYS] Flood from %s (%s) - user closed by script.", u->Nick, u->IP);
        if(CheckSprintf(imsgLen, 131072, "DefloodWarn3") == true) {
            UdpDebug->Broadcast(ScriptManager->lua_msg, imsgLen);
        }

        UserClose(u);
    }

    lua_settop(L, 0);

    lua_pushboolean(L, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int SendToAll(lua_State * L) {
	if(lua_gettop(L) != 1) {
        luaL_error(L, "bad argument count to 'SendToAll' (1 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

    if(lua_type(L, 1) != LUA_TSTRING) {
        luaL_checktype(L, 1, LUA_TSTRING);
		lua_settop(L, 0);
        return 0;
    }

    if(lua_gettop(L) == 1) {
        size_t iLen;
        char *sData = (char *)lua_tolstring(L, 1, &iLen);
        if(sData[0] != '\0' && iLen < 128001) {
			if(sData[iLen-1] != '|') {
                memcpy(ScriptManager->lua_msg, sData, iLen);
                ScriptManager->lua_msg[iLen] = '|';
                ScriptManager->lua_msg[iLen+1] = '\0';
				globalQ->Store(ScriptManager->lua_msg, iLen+1);
			} else {
				globalQ->Store(sData, iLen);
            }
        }
    }

	lua_settop(L, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendToNick(lua_State * L) {
	if(lua_gettop(L) != 2) {
        luaL_error(L, "bad argument count to 'SendToNick' (2 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

    if(lua_type(L, 1) != LUA_TSTRING || lua_type(L, 2) != LUA_TSTRING) {
        luaL_checktype(L, 1, LUA_TSTRING);
        luaL_checktype(L, 2, LUA_TSTRING);
		lua_settop(L, 0);
        return 0;
    }

    size_t iNickLen, iDataLen;
    char *sNick = (char *)lua_tolstring(L, 1, &iNickLen);
    char *sData = (char *)lua_tolstring(L, 2, &iDataLen);

    if(iNickLen == 0 || iDataLen == 0 || iDataLen > 128000) {
        lua_settop(L, 0);
        return 0;
    }

    User *u = hashManager->FindUser(sNick, iNickLen);
    if(u != NULL) {
        if(sData[iDataLen-1] != '|') {
            memcpy(ScriptManager->lua_msg, sData, iDataLen);
            ScriptManager->lua_msg[iDataLen] = '|';
            ScriptManager->lua_msg[iDataLen+1] = '\0';
            UserSendCharDelayed(u, ScriptManager->lua_msg, iDataLen+1);
        } else {
            UserSendCharDelayed(u, sData, iDataLen);
        }
    }

    lua_settop(L, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendToOpChat(lua_State * L) {
	if(lua_gettop(L) != 1) {
        luaL_error(L, "bad argument count to 'SendToOpChat' (1 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

    if(lua_type(L, 1) != LUA_TSTRING) {
        luaL_checktype(L, 1, LUA_TSTRING);
		lua_settop(L, 0);
        return 0;
    }

    size_t iDataLen;
    char *sData = (char *)lua_tolstring(L, 1, &iDataLen);

    if(iDataLen == 0 || iDataLen > 128000) {
        lua_settop(L, 0);
        return 0;
    }

    if(SettingManager->bBools[SETBOOL_REG_OP_CHAT] == true) {
        int iLen = sprintf(ScriptManager->lua_msg, "%s $<%s> %s|", SettingManager->sTexts[SETTXT_OP_CHAT_NICK], SettingManager->sTexts[SETTXT_OP_CHAT_NICK], sData);
        if(CheckSprintf(iLen, 131072, "SendToOpChat") == true) {
			QueueDataItem *newItem = globalQ->CreateQueueDataItem(ScriptManager->lua_msg, iLen, NULL, 0, globalqueue::OPCHAT);
            globalQ->SingleItemsStore(newItem);
        }
    }

    lua_settop(L, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendToOps(lua_State * L) {
	if(lua_gettop(L) != 1) {
        luaL_error(L, "bad argument count to 'SendToOps' (1 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

    if(lua_type(L, 1) != LUA_TSTRING) {
        luaL_checktype(L, 1, LUA_TSTRING);
		lua_settop(L, 0);
        return 0;
    }

    size_t iLen;
    char *sData = (char *)lua_tolstring(L, 1, &iLen);
    
    if(iLen == 0 || iLen > 128000) {
        lua_settop(L, 0);
        return 0;
    }

    if(sData[iLen-1] != '|') {
        memcpy(ScriptManager->lua_msg, sData, iLen);
        ScriptManager->lua_msg[iLen] = '|';
        ScriptManager->lua_msg[iLen+1] = '\0';
        globalQ->OPStore(ScriptManager->lua_msg, iLen+1);
    } else {
        globalQ->OPStore(sData, iLen);
    }

    lua_settop(L, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendToProfile(lua_State * L) {
	if(lua_gettop(L) != 2) {
        luaL_error(L, "bad argument count to 'SendToProfile' (2 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

    if(lua_type(L, 1) != LUA_TNUMBER || lua_type(L, 2) != LUA_TSTRING) {
        luaL_checktype(L, 1, LUA_TNUMBER);
        luaL_checktype(L, 2, LUA_TSTRING);
		lua_settop(L, 0);
        return 0;
    }

    int32_t iProfile = (int32_t)lua_tonumber(L, 1);

    size_t iDataLen;
    char *sData = (char *)lua_tolstring(L, 2, &iDataLen);

    if(iDataLen == 0 || iDataLen > 128000) {
        lua_settop(L, 0);
        return 0;
    }

    if(sData[iDataLen-1] != '|') {
        memcpy(ScriptManager->lua_msg, sData, iDataLen);
		ScriptManager->lua_msg[iDataLen] = '|';
        ScriptManager->lua_msg[iDataLen+1] = '\0';
		QueueDataItem *newItem = globalQ->CreateQueueDataItem(ScriptManager->lua_msg, iDataLen+1, NULL, iProfile, globalqueue::TOPROFILE);
        globalQ->SingleItemsStore(newItem);
    } else {
		QueueDataItem *newItem = globalQ->CreateQueueDataItem(sData, iDataLen, NULL, iProfile, globalqueue::TOPROFILE);
		globalQ->SingleItemsStore(newItem);
    }

    lua_settop(L, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendToUser(lua_State * L) {
	if(lua_gettop(L) != 2) {
        luaL_error(L, "bad argument count to 'SendToUser' (2 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

    if(lua_type(L, 1) != LUA_TTABLE || lua_type(L, 2) != LUA_TSTRING) {
        luaL_checktype(L, 1, LUA_TTABLE);
        luaL_checktype(L, 2, LUA_TSTRING);
		lua_settop(L, 0);
        return 0;
    }

    User *u = ScriptGetUser(L, 2, "SendToUser");

    if(u == NULL) {
		lua_settop(L, 0);
        return 0;
    }

    size_t iLen;
    char *sData = (char *)lua_tolstring(L, 2, &iLen);

    if(iLen == 0 || iLen > 128000) {
        lua_settop(L, 0);
        return 0;
    }

    if(sData[iLen-1] != '|') {
        memcpy(ScriptManager->lua_msg, sData, iLen);
        ScriptManager->lua_msg[iLen] = '|';
        ScriptManager->lua_msg[iLen+1] = '\0';
    	UserSendCharDelayed(u, ScriptManager->lua_msg, iLen+1);
    } else {
        UserSendCharDelayed(u, sData, iLen);
    }

    lua_settop(L, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendPmToAll(lua_State * L) {
	if(lua_gettop(L) != 2) {
        luaL_error(L, "bad argument count to 'SendPmToAll' (2 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

    if(lua_type(L, 1) != LUA_TSTRING || lua_type(L, 2) != LUA_TSTRING) {
        luaL_checktype(L, 1, LUA_TSTRING);
        luaL_checktype(L, 2, LUA_TSTRING);
		lua_settop(L, 0);
        return 0;
    }

    size_t iFromLen, iDataLen;
    char *sFrom = (char *)lua_tolstring(L, 1, &iFromLen);
    char *sData = (char *)lua_tolstring(L, 2, &iDataLen);

    if(iFromLen == 0 || iFromLen > 64 || iDataLen == 0 || iDataLen > 128000) {
        lua_settop(L, 0); 
        return 0;
    }

    int imsgLen = sprintf(ScriptManager->lua_msg, "%s $<%s> %s|", sFrom, sFrom, sData);
    if(CheckSprintf(imsgLen, 131072, "SendPmToAll") == true) {
		QueueDataItem *newItem = globalQ->CreateQueueDataItem(ScriptManager->lua_msg, imsgLen, NULL, 0, globalqueue::PM2ALL);
        globalQ->SingleItemsStore(newItem);
    }

    lua_settop(L, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendPmToNick(lua_State * L) {
	if(lua_gettop(L) != 3) {
        luaL_error(L, "bad argument count to 'SendPmToNick' (3 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

    if(lua_type(L, 1) != LUA_TSTRING || lua_type(L, 2) != LUA_TSTRING || lua_type(L, 3) != LUA_TSTRING) {
        luaL_checktype(L, 1, LUA_TSTRING);
        luaL_checktype(L, 2, LUA_TSTRING);
        luaL_checktype(L, 3, LUA_TSTRING);
		lua_settop(L, 0);
        return 0;
    }

    size_t iToLen, iFromLen, iDataLen;
    char *sTo = (char *)lua_tolstring(L, 1, &iToLen);
    char *sFrom = (char *)lua_tolstring(L, 2, &iFromLen);
    char *sData = (char *)lua_tolstring(L, 3, &iDataLen);

    if(iToLen == 0 || iFromLen == 0 || iFromLen > 64 || iDataLen == 0 || iDataLen > 128000) {
        lua_settop(L, 0);
        return 0;
    }

    User *u = hashManager->FindUser(sTo, iToLen);
    if(u != NULL) {
        int iMsgLen = sprintf(ScriptManager->lua_msg, "$To: %s From: %s $<%s> %s|", sTo, sFrom, sFrom, sData);
        if(CheckSprintf(iMsgLen, 131072, "SendPmToNick") == true) {
            UserSendCharDelayed(u, ScriptManager->lua_msg, iMsgLen);
        }
    }

    lua_settop(L, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendPmToOps(lua_State * L) {
	if(lua_gettop(L) != 2) {
        luaL_error(L, "bad argument count to 'SendPmToOps' (2 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

    if(lua_type(L, 1) != LUA_TSTRING || lua_type(L, 2) != LUA_TSTRING) {
        luaL_checktype(L, 1, LUA_TSTRING);
        luaL_checktype(L, 2, LUA_TSTRING);
		lua_settop(L, 0);
        return 0;
    }

    size_t iFromLen, iDataLen;
    char *sFrom = (char *)lua_tolstring(L, 1, &iFromLen);
    char *sData = (char *)lua_tolstring(L, 2, &iDataLen);

    if(iFromLen == 0 || iFromLen > 64 || iDataLen == 0 || iDataLen > 128000) {
        lua_settop(L, 0);
        return 0;
    }

    int imsgLen = sprintf(ScriptManager->lua_msg, "%s $<%s> %s|", sFrom, sFrom, sData);
    if(CheckSprintf(imsgLen, 131072, "SendPmToOps") == true) {
		QueueDataItem *newItem = globalQ->CreateQueueDataItem(ScriptManager->lua_msg, imsgLen, NULL, 0, globalqueue::PM2OPS);
        globalQ->SingleItemsStore(newItem);
    }

    lua_settop(L, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendPmToProfile(lua_State * L) {
	if(lua_gettop(L) != 3) {
        luaL_error(L, "bad argument count to 'SendPmToProfile' (3 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

    if(lua_type(L, 1) != LUA_TNUMBER || lua_type(L, 2) != LUA_TSTRING || lua_type(L, 3) != LUA_TSTRING) {
        luaL_checktype(L, 1, LUA_TNUMBER);
        luaL_checktype(L, 2, LUA_TSTRING);
        luaL_checktype(L, 3, LUA_TSTRING);
		lua_settop(L, 0);
        return 0;
    }

    int32_t iProfile = (int32_t)lua_tonumber(L, 1);

    size_t iFromLen, iDataLen;
    char *sFrom = (char *)lua_tolstring(L, 2, &iFromLen);
    char *sData = (char *)lua_tolstring(L, 3, &iDataLen);

    if(iFromLen == 0 || iFromLen > 64 || iDataLen == 0 || iDataLen > 128000) {
        lua_settop(L, 0);
        return 0;
    }

    int imsgLen = sprintf(ScriptManager->lua_msg, "%s $<%s> %s|", sFrom, sFrom, sData);
    if(CheckSprintf(imsgLen, 131072, "SendPmToProfile") == true) {
		QueueDataItem *newItem = globalQ->CreateQueueDataItem(ScriptManager->lua_msg, imsgLen, NULL, iProfile, globalqueue::PM2PROFILE);
        globalQ->SingleItemsStore(newItem);
    }

    lua_settop(L, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendPmToUser(lua_State * L) {
	if(lua_gettop(L) != 3) {
        luaL_error(L, "bad argument count to 'SendPmToUser' (3 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

    if(lua_type(L, 1) != LUA_TTABLE || lua_type(L, 2) != LUA_TSTRING || lua_type(L, 3) != LUA_TSTRING) {
        luaL_checktype(L, 1, LUA_TTABLE);
        luaL_checktype(L, 2, LUA_TSTRING);
        luaL_checktype(L, 3, LUA_TSTRING);
		lua_settop(L, 0);
        return 0;
    }

    User *u = ScriptGetUser(L, 3, "SendPmToUser");

    if(u == NULL) {
		lua_settop(L, 0);
        return 0;
    }

    size_t iFromLen, iDataLen;
    char *sFrom = (char *)lua_tolstring(L, 2, &iFromLen);
    char *sData = (char *)lua_tolstring(L, 3, &iDataLen);
    
    if(iFromLen == 0 || iFromLen > 64 || iDataLen == 0 || iDataLen > 128000) {
		lua_settop(L, 0);
        return 0;
    }

    int imsgLen = sprintf(ScriptManager->lua_msg, "$To: %s From: %s $<%s> %s|", u->Nick, sFrom, sFrom, sData);
    if(CheckSprintf(imsgLen, 131072, "SendPmToUser") == true) {
        UserSendCharDelayed(u, ScriptManager->lua_msg, imsgLen);
    }

    lua_settop(L, 0);
    return 0;
}
//------------------------------------------------------------------------------

static const luaL_reg core_funcs[] =  {
	{ "Restart", Restart }, 
	{ "Shutdown", Shutdown }, 
	{ "ResumeAccepts", ResumeAccepts }, 
	{ "SuspendAccepts", SuspendAccepts }, 
	{ "RegBot", RegBot },  
	{ "UnregBot", UnregBot }, 
	{ "GetBots", GetBots }, 
	{ "GetActualUsersPeak", GetActualUsersPeak }, 
	{ "GetMaxUsersPeak", GetMaxUsersPeak }, 
	{ "GetCurrentSharedSize", GetCurrentSharedSize }, 
	{ "GetHubIP", GetHubIP }, 
	{ "GetHubSecAlias", GetHubSecAlias }, 
	{ "GetPtokaXPath", GetPtokaXPath }, 
	{ "GetUsersCount", GetUsersCount }, 
	{ "GetUpTime", GetUpTime }, 
	{ "GetOnlineNonOps", GetOnlineNonOps }, 
	{ "GetOnlineOps", GetOnlineOps }, 
	{ "GetOnlineRegs", GetOnlineRegs }, 
	{ "GetOnlineUsers", GetOnlineUsers }, 
	{ "GetUser", GetUser }, 
	{ "GetUsers", GetUsers }, 
	{ "GetUserAllData", GetUserAllData }, 
	{ "GetUserData", GetUserData }, 
	{ "GetUserValue", GetUserValue }, 
	{ "Disconnect", Disconnect }, 
	{ "Kick", Kick }, 
	{ "Redirect", Redirect }, 
	{ "DefloodWarn", DefloodWarn }, 
    { "SendToAll", SendToAll }, 
	{ "SendToNick", SendToNick }, 
	{ "SendToOpChat", SendToOpChat }, 
	{ "SendToOps", SendToOps }, 
	{ "SendToProfile", SendToProfile }, 
	{ "SendToUser", SendToUser }, 
	{ "SendPmToAll", SendPmToAll }, 
	{ "SendPmToNick", SendPmToNick }, 
	{ "SendPmToOps", SendPmToOps }, 
	{ "SendPmToProfile", SendPmToProfile }, 
	{ "SendPmToUser", SendPmToUser }, 
	{ NULL, NULL }
};
//---------------------------------------------------------------------------

void RegCore(lua_State * L) {
    luaL_register(L, "Core", core_funcs);

    lua_pushliteral(L, "Version");
	lua_pushliteral(L, PtokaXVersionString);
	lua_settable(L, -3);
	lua_pushliteral(L, "BuildNumber");
	lua_pushnumber(L, (double)_strtoui64(BUILD_NUMBER, NULL, 10));
	lua_settable(L, -3);
}
//---------------------------------------------------------------------------
