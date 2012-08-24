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
#include "LuaCoreLib.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "eventqueue.h"
#include "GlobalDataQueue.h"
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

    size_t szNickLen, szDescrLen, szEmailLen;

    char *nick = (char *)lua_tolstring(L, 1, &szNickLen);
    char *description = (char *)lua_tolstring(L, 2, &szDescrLen);
    char *email = (char *)lua_tolstring(L, 3, &szEmailLen);

    bool bIsOP = lua_toboolean(L, 4) == 0 ? false : true;

    if(szNickLen == 0 || szNickLen > 64 || strpbrk(nick, " $|") != NULL || szDescrLen > 64 || strpbrk(description, "$|") != NULL ||
        szEmailLen > 64 || strpbrk(email, "$|") != NULL || hashManager->FindUser(nick, szNickLen) != NULL || ResNickManager->CheckReserved(nick, HashNick(nick, szNickLen)) != false) {
		lua_settop(L, 0);

		lua_pushnil(L);
        return 1;
    }

    ScriptBot * pNewBot = new ScriptBot(nick, szNickLen, description, szDescrLen, email, szEmailLen, bIsOP);
    if(pNewBot == NULL || pNewBot->sNick == NULL || pNewBot->sMyINFO == NULL) {
        if(pNewBot == NULL) {
            AppendDebugLog("%s - [MEM] Cannot allocate pNewBot in Core.RegBot\n", 0);
        } else if(pNewBot->sNick == NULL) {
            delete pNewBot;
            AppendDebugLog("%s - [MEM] Cannot allocate pNewBot->sNick in Core.RegBot\n", 0);
        } else if(pNewBot->sMyINFO == NULL) {
            delete pNewBot;
            AppendDebugLog("%s - [MEM] Cannot allocate pNewBot->sMyINFO in Core.RegBot\n", 0);
        }

		lua_settop(L, 0);

		lua_pushnil(L);
        return 1;
    }

    // PPK ... finally we can clear stack
    lua_settop(L, 0);

	Script * obj = ScriptManager->FindScript(L);
	if(obj == NULL) {
		delete pNewBot;

		lua_pushnil(L);
        return 1;
    }

    ScriptBot * next = obj->BotList;

    while(next != NULL) {
        ScriptBot * cur = next;
        next = cur->next;

		if(strcasecmp(pNewBot->sNick, cur->sNick) == 0) {
            delete pNewBot;

            lua_pushnil(L);
            return 1;
        }
    }

    if(obj->BotList == NULL) {
        obj->BotList = pNewBot;
    } else {
        obj->BotList->prev = pNewBot;
        pNewBot->next = obj->BotList;
        obj->BotList = pNewBot;
    }

	ResNickManager->AddReservedNick(pNewBot->sNick, true);

	colUsers->AddBot2NickList(pNewBot->sNick, szNickLen, pNewBot->bIsOP);

    colUsers->AddBot2MyInfos(pNewBot->sMyINFO);

    // PPK ... fixed hello sending only to users without NoHello
    int iMsgLen = sprintf(g_sBuffer, "$Hello %s|", pNewBot->sNick);
    if(CheckSprintf(iMsgLen, g_szBufferSize, "RegBot") == true) {
        g_GlobalDataQueue->AddQueueItem(g_sBuffer, iMsgLen, NULL, 0, GlobalDataQueue::CMD_HELLO);
    }
    
    g_GlobalDataQueue->AddQueueItem(pNewBot->sMyINFO, strlen(pNewBot->sMyINFO), NULL, 0, GlobalDataQueue::CMD_MYINFO);
        
    if(pNewBot->bIsOP == true) {
        g_GlobalDataQueue->OpListStore(pNewBot->sNick);
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

    size_t szLen;
    char * botnick = (char *)lua_tolstring(L, 1, &szLen);
    
    if(szLen == 0) {
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

		if(strcasecmp(botnick, cur->sNick) == 0) {
            ResNickManager->DelReservedNick(cur->sNick, true);

            colUsers->DelFromNickList(cur->sNick, cur->bIsOP);

            colUsers->DelBotFromMyInfos(cur->sMyINFO);

            int iMsgLen = sprintf(g_sBuffer, "$Quit %s|", cur->sNick);
            if(CheckSprintf(iMsgLen, g_szBufferSize, "UnregBot") == true) {
                g_GlobalDataQueue->AddQueueItem(g_sBuffer, iMsgLen, NULL, 0, GlobalDataQueue::CMD_QUIT);
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

static int GetHubIPs(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'GetHubIPs' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }

    if(sHubIP[0] == '\0' && sHubIP6[0] == '\0') {
        lua_pushnil(L);
        return 1;
    }

    lua_newtable(L);
    int t = lua_gettop(L), i = 0;

    if(sHubIP6[0] != '\0') {
        i++;

        lua_pushnumber(L, i);
		lua_pushstring(L, sHubIP6);
		lua_rawset(L, t);
	}

    if(sHubIP[0] != '\0') {
        i++;

        lua_pushnumber(L, i);
		lua_pushstring(L, sHubIP);
		lua_rawset(L, t);
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

        if(curUser->ui8State == User::STATE_ADDED && ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == bOperator) {
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
        if(curUser->ui8State == User::STATE_ADDED && curUser->iProfile != -1) {
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

	        if(curUser->ui8State == User::STATE_ADDED) {
	            lua_pushnumber(L, ++i);
				ScriptPushUser(L, curUser, bFullTable);
	            lua_rawset(L, t);
	        }
	    }
    } else {
		while(next != NULL) {
		    User *curUser = next;
		    next = curUser->next;

		    if(curUser->ui8State == User::STATE_ADDED && curUser->iProfile == iProfile) {
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

    size_t szLen = 0;

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

        nick = (char *)lua_tolstring(L, 1, &szLen);

        bFullTable = lua_toboolean(L, 2) == 0 ? false : true;
    } else if(n == 1) {
        if(lua_type(L, 1) != LUA_TSTRING) {
            luaL_checktype(L, 1, LUA_TSTRING);

            lua_settop(L, 0);
    
            lua_pushnil(L);
            return 1;
        }

        nick = (char *)lua_tolstring(L, 1, &szLen);
    } else {
        luaL_error(L, "bad argument count to 'GetUser' (1 or 2 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        
        lua_pushnil(L);
        return 1;
    }

    if(szLen == 0) {
        lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }
    
    User *u = hashManager->FindUser(nick, szLen);

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

    size_t szLen = 0;

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

        sIP = (char *)lua_tolstring(L, 1, &szLen);

        bFullTable = lua_toboolean(L, 2) == 0 ? false : true;
    } else if(n == 1) {
        if(lua_type(L, 1) != LUA_TSTRING) {
            luaL_checktype(L, 1, LUA_TSTRING);

            lua_settop(L, 0);
    
            lua_pushnil(L);
            return 1;
        }

        sIP = (char *)lua_tolstring(L, 1, &szLen);
    } else {
        luaL_error(L, "bad argument count to 'GetUsers' (1 or 2 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        
        lua_pushnil(L);
        return 1;
    }

    uint8_t ui128Hash[16];
    memset(ui128Hash, 0, 16);

    if(szLen == 0 || HashIP(sIP, ui128Hash) == false) {
        lua_settop(L, 0);
        lua_pushnil(L);

        return 1;
    }

    User *next = hashManager->FindUser(ui128Hash);

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
        	if(u->sModes[0] != '\0') {
				lua_pushstring(L, u->sModes);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        case 1:
        	lua_pushliteral(L, "sMyInfoString");
        	if(u->sMyInfoOriginal != NULL) {
				lua_pushlstring(L, u->sMyInfoOriginal, u->ui16MyInfoOriginalLen);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        case 2:
        	lua_pushliteral(L, "sDescription");
        	if(u->sDescription != NULL) {
				lua_pushlstring(L, u->sDescription, (size_t)u->ui8DescriptionLen);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        case 3:
        	lua_pushliteral(L, "sTag");
        	if(u->sTag != NULL) {
				lua_pushlstring(L, u->sTag, (size_t)u->ui8TagLen);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        case 4:
        	lua_pushliteral(L, "sConnection");
        	if(u->sConnection != NULL) {
				lua_pushlstring(L, u->sConnection, (size_t)u->ui8ConnectionLen);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        case 5:
        	lua_pushliteral(L, "sEmail");
        	if(u->sEmail != NULL) {
				lua_pushlstring(L, u->sEmail, (size_t)u->ui8EmailLen);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        case 6:
        	lua_pushliteral(L, "sClient");
        	if(u->sClient != NULL) {
				lua_pushlstring(L, u->sClient, (size_t)u->ui8ClientLen);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        case 7:
        	lua_pushliteral(L, "sClientVersion");
        	if(u->sTagVersion != NULL) {
				lua_pushlstring(L, u->sTagVersion, (size_t)u->ui8TagVersionLen);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        case 8:
        	lua_pushliteral(L, "sVersion");
        	if(u->sVersion!= NULL) {
				lua_pushstring(L, u->sVersion);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        case 9:
        	lua_pushliteral(L, "bConnected");
        	u->ui8State == User::STATE_ADDED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	lua_rawset(L, 1);
            break;
        case 10:
        	lua_pushliteral(L, "bActive");
        	if((u->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
                (u->ui32BoolBits & User::BIT_IPV6_ACTIVE) == User::BIT_IPV6_ACTIVE ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
            } else {
                (u->ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
            }
        	lua_rawset(L, 1);
            break;
        case 11:
        	lua_pushliteral(L, "bOperator");
        	(u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	lua_rawset(L, 1);
            break;
        case 12:
        	lua_pushliteral(L, "bUserCommand");
        	(u->ui32SupportBits & User::SUPPORTBIT_USERCOMMAND) == User::SUPPORTBIT_USERCOMMAND ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	lua_rawset(L, 1);
            break;
        case 13:
        	lua_pushliteral(L, "bQuickList");
        	(u->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
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
        	lua_pushnumber(L, (double)u->ui64SharedSize);
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
        case 27: {
            lua_pushliteral(L, "sMac");
            char sMac[18];
            if(GetMacAddress(u->sIP, sMac) == true) {
                lua_pushlstring(L, sMac, 17);
            } else {
                lua_pushnil(L);
            }
        	lua_rawset(L, 1);
            break;
        }
        case 28:
            lua_pushliteral(L, "bDescriptionChanged");
        	(u->ui32InfoBits & User::INFOBIT_DESCRIPTION_CHANGED) == User::INFOBIT_DESCRIPTION_CHANGED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	lua_rawset(L, 1);
            break;
        case 29:
            lua_pushliteral(L, "bTagChanged");
        	(u->ui32InfoBits & User::INFOBIT_TAG_CHANGED) == User::INFOBIT_TAG_CHANGED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	lua_rawset(L, 1);
            break;
        case 30:
            lua_pushliteral(L, "bConnectionChanged");
        	(u->ui32InfoBits & User::INFOBIT_CONNECTION_CHANGED) == User::INFOBIT_CONNECTION_CHANGED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	lua_rawset(L, 1);
            break;
        case 31:
            lua_pushliteral(L, "bEmailChanged");
        	(u->ui32InfoBits & User::INFOBIT_EMAIL_CHANGED) == User::INFOBIT_EMAIL_CHANGED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	lua_rawset(L, 1);
            break;
        case 32:
            lua_pushliteral(L, "bShareChanged");
        	(u->ui32InfoBits & User::INFOBIT_SHARE_CHANGED) == User::INFOBIT_SHARE_CHANGED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	lua_rawset(L, 1);
            break;
        case 33:
            lua_pushliteral(L, "sScriptedDescriptionShort");
        	if(u->sChangedDescriptionShort != NULL) {
				lua_pushlstring(L, u->sChangedDescriptionShort, u->ui8ChangedDescriptionShortLen);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        case 34:
            lua_pushliteral(L, "sScriptedDescriptionLong");
        	if(u->sChangedDescriptionLong != NULL) {
				lua_pushlstring(L, u->sChangedDescriptionLong, u->ui8ChangedDescriptionLongLen);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        case 35:
            lua_pushliteral(L, "sScriptedTagShort");
        	if(u->sChangedTagShort != NULL) {
				lua_pushlstring(L, u->sChangedTagShort, u->ui8ChangedTagShortLen);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        case 36:
            lua_pushliteral(L, "sScriptedTagLong");
        	if(u->sChangedTagLong != NULL) {
				lua_pushlstring(L, u->sChangedTagLong, u->ui8ChangedTagLongLen);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        case 37:
            lua_pushliteral(L, "sScriptedConnectionShort");
        	if(u->sChangedConnectionShort != NULL) {
				lua_pushlstring(L, u->sChangedConnectionShort, u->ui8ChangedConnectionShortLen);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        case 38:
            lua_pushliteral(L, "sScriptedConnectionLong");
        	if(u->sChangedConnectionLong != NULL) {
				lua_pushlstring(L, u->sChangedConnectionLong, u->ui8ChangedConnectionLongLen);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        case 39:
            lua_pushliteral(L, "sScriptedEmailShort");
        	if(u->sChangedEmailShort != NULL) {
				lua_pushlstring(L, u->sChangedEmailShort, u->ui8ChangedEmailShortLen);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        case 40:
            lua_pushliteral(L, "sScriptedEmailLong");
        	if(u->sChangedEmailLong != NULL) {
				lua_pushlstring(L, u->sChangedEmailLong, u->ui8ChangedEmailLongLen);
			} else {
				lua_pushnil(L);
			}
        	lua_rawset(L, 1);
            break;
        case 41:
            lua_pushliteral(L, "iScriptediShareSizeShort");
        	lua_pushnumber(L, (double)u->ui64ChangedSharedSizeShort);
        	lua_rawset(L, 1);
            break;
        case 42:
            lua_pushliteral(L, "iScriptediShareSizeLong");
        	lua_pushnumber(L, (double)u->ui64ChangedSharedSizeLong);
        	lua_rawset(L, 1);
            break;
        case 43: {
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

        	lua_rawset(L, 1);
            break;
        }
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
        	if(u->sModes[0] != '\0') {
				lua_pushstring(L, u->sModes);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 1:
        	if(u->sMyInfoOriginal != NULL) {
				lua_pushlstring(L, u->sMyInfoOriginal, u->ui16MyInfoOriginalLen);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 2:
        	if(u->sDescription != NULL) {
				lua_pushlstring(L, u->sDescription, u->ui8DescriptionLen);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 3:
        	if(u->sTag != NULL) {
				lua_pushlstring(L, u->sTag, u->ui8TagLen);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 4:
        	if(u->sConnection != NULL) {
				lua_pushlstring(L, u->sConnection, u->ui8ConnectionLen);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 5:
        	if(u->sEmail != NULL) {
				lua_pushlstring(L, u->sEmail, u->ui8EmailLen);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 6:
        	if(u->sClient != NULL) {
				lua_pushlstring(L, u->sClient, u->ui8ClientLen);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 7:
        	if(u->sTagVersion != NULL) {
				lua_pushlstring(L, u->sTagVersion, u->ui8TagVersionLen);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 8:
        	if(u->sVersion != NULL) {
				lua_pushstring(L, u->sVersion);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 9:
        	u->ui8State == User::STATE_ADDED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	return 1;
        case 10:
            if((u->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
                (u->ui32BoolBits & User::BIT_IPV6_ACTIVE) == User::BIT_IPV6_ACTIVE ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
            } else {
        	   (u->ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
            }
        	return 1;
        case 11:
        	(u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	return 1;
        case 12:
        	(u->ui32SupportBits & User::SUPPORTBIT_USERCOMMAND) == User::SUPPORTBIT_USERCOMMAND ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	return 1;
        case 13:
        	(u->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	return 1;
        case 14:
        	(u->ui32BoolBits & User::BIT_HAVE_BADTAG) == User::BIT_HAVE_BADTAG ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	return 1;
        case 15:
        	lua_pushnumber(L, u->iProfile);
        	return 1;
        case 16:
        	lua_pushnumber(L, (double)u->ui64SharedSize);
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
            char sMac[18];
            if(GetMacAddress(u->sIP, sMac) == true) {
                lua_pushlstring(L, sMac, 17);
                return 1;
            } else {
                lua_pushnil(L);
                return 1;
            }
        }
        case 28:
        	(u->ui32InfoBits & User::INFOBIT_DESCRIPTION_CHANGED) == User::INFOBIT_DESCRIPTION_CHANGED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	return 1;
        case 29:
        	(u->ui32InfoBits & User::INFOBIT_TAG_CHANGED) == User::INFOBIT_TAG_CHANGED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	return 1;
        case 30:
        	(u->ui32InfoBits & User::INFOBIT_CONNECTION_CHANGED) == User::INFOBIT_CONNECTION_CHANGED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	return 1;
        case 31:
        	(u->ui32InfoBits & User::INFOBIT_EMAIL_CHANGED) == User::INFOBIT_EMAIL_CHANGED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	return 1;
        case 32:
        	(u->ui32InfoBits & User::INFOBIT_SHARE_CHANGED) == User::INFOBIT_SHARE_CHANGED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
        	return 1;
        case 33:
        	if(u->sChangedDescriptionShort != NULL) {
				lua_pushlstring(L, u->sChangedDescriptionShort, u->ui8ChangedDescriptionShortLen);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 34:
        	if(u->sChangedDescriptionLong != NULL) {
				lua_pushlstring(L, u->sChangedDescriptionLong, u->ui8ChangedDescriptionLongLen);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 35:
        	if(u->sChangedTagShort != NULL) {
				lua_pushlstring(L, u->sChangedTagShort, u->ui8ChangedTagShortLen);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 36:
        	if(u->sChangedTagLong != NULL) {
				lua_pushlstring(L, u->sChangedTagLong, u->ui8ChangedTagLongLen);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 37:
        	if(u->sChangedConnectionShort != NULL) {
				lua_pushlstring(L, u->sChangedConnectionShort, u->ui8ChangedConnectionShortLen);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 38:
        	if(u->sChangedConnectionLong != NULL) {
				lua_pushlstring(L, u->sChangedConnectionLong, u->ui8ChangedConnectionLongLen);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 39:
        	if(u->sChangedEmailShort != NULL) {
				lua_pushlstring(L, u->sChangedEmailShort, u->ui8ChangedEmailShortLen);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 40:
        	if(u->sChangedEmailLong != NULL) {
				lua_pushlstring(L, u->sChangedEmailLong, u->ui8ChangedEmailLongLen);
			} else {
				lua_pushnil(L);
			}
        	return 1;
        case 41:
        	lua_pushnumber(L, (double)u->ui64ChangedSharedSizeShort);
        	return 1;
        case 42:
        	lua_pushnumber(L, (double)u->ui64ChangedSharedSizeLong);
        	return 1;
        case 43: {
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
        size_t szLen;
        char * nick = (char *)lua_tolstring(L, 1, &szLen);
    
        if(szLen == 0) {
            return 0;
        }
    
        u = hashManager->FindUser(nick, szLen);

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

//    int imsgLen = sprintf(g_sBuffer, "[SYS] User %s (%s) disconnected by script.", u->Nick, u->IP);
//    UdpDebug->Broadcast(g_sBuffer, imsgLen);
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

    size_t szKickerLen, szReasonLen;
    char *sKicker = (char *)lua_tolstring(L, 2, &szKickerLen);
    char *sReason = (char *)lua_tolstring(L, 3, &szReasonLen);

    if(szKickerLen == 0 || szKickerLen > 64 || szReasonLen == 0 || szReasonLen > 128000) {
    	lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }

    hashBanManager->TempBan(u, sReason, sKicker, 0, 0, false);

    int imsgLen = sprintf(g_sBuffer, "<%s> %s: %s|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_BEING_KICKED_BCS], sReason);
    if(CheckSprintf(imsgLen, g_szBufferSize, "Kick5") == true) {
    	UserSendCharDelayed(u, g_sBuffer, imsgLen);
    }

    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
    	if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
    	    imsgLen = sprintf(g_sBuffer, "%s $<%s> *** %s %s IP %s %s %s %s: %s|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], u->sNick, LanguageManager->sTexts[LAN_WITH_LWR], u->sIP, LanguageManager->sTexts[LAN_WAS_KICKED_BY], sKicker,
                LanguageManager->sTexts[LAN_BECAUSE_LWR], sReason);
            if(CheckSprintf(imsgLen, g_szBufferSize, "Kick6") == true) {
				g_GlobalDataQueue->SingleItemStore(g_sBuffer, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
            }
    	} else {
    	    imsgLen = sprintf(g_sBuffer, "<%s> *** %s %s IP %s %s %s %s: %s|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], u->sNick,
                LanguageManager->sTexts[LAN_WITH_LWR], u->sIP, LanguageManager->sTexts[LAN_WAS_KICKED_BY], sKicker, LanguageManager->sTexts[LAN_BECAUSE_LWR], sReason);
            if(CheckSprintf(imsgLen, g_szBufferSize, "Kick7") == true) {
                g_GlobalDataQueue->AddQueueItem(g_sBuffer, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
            }
    	}
    }

    // disconnect the user
    imsgLen = sprintf(g_sBuffer, "[SYS] User %s (%s) kicked by script.", u->sNick, u->sIP);
    if(CheckSprintf(imsgLen, g_szBufferSize, "Kick8") == true) {
        UdpDebug->Broadcast(g_sBuffer, imsgLen);
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

    size_t szAddressLen, szReasonLen;
    char *sAddress = (char *)lua_tolstring(L, 2, &szAddressLen);
    char *sReason = (char *)lua_tolstring(L, 3, &szReasonLen);

    if(szAddressLen == 0 || szAddressLen > 1024 || szReasonLen == 0 || szReasonLen > 128000) {
		lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }

    int imsgLen = sprintf(g_sBuffer, "<%s> %s %s. %s: %s|$ForceMove %s|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_REDIR_TO],
        sAddress, LanguageManager->sTexts[LAN_MESSAGE], sReason, sAddress);
    if(CheckSprintf(imsgLen, g_szBufferSize, "Redirect2") == true) {
        UserSendChar(u, g_sBuffer, imsgLen);
    }

    //int imsgLen = sprintf(g_sBuffer, "[SYS] User %s (%s) redirected by script.", u->Nick, u->IP);
    //UdpDebug->Broadcast(g_sBuffer, imsgLen);

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
                imsgLen = sprintf(g_sBuffer, "%s $<%s> *** %s %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                    SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_FLOODER], u->sNick, LanguageManager->sTexts[LAN_WITH_IP], u->sIP,
                    LanguageManager->sTexts[LAN_DISCONN_BY_SCRIPT]);
                if(CheckSprintf(imsgLen, g_szBufferSize, "DefloodWarn1") == true) {
                    g_GlobalDataQueue->SingleItemStore(g_sBuffer, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                }
            } else {
                imsgLen = sprintf(g_sBuffer, "<%s> *** %s %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_FLOODER], u->sNick,
                    LanguageManager->sTexts[LAN_WITH_IP], u->sIP, LanguageManager->sTexts[LAN_DISCONN_BY_SCRIPT]);
                if(CheckSprintf(imsgLen, g_szBufferSize, "DefloodWarn2") == true) {
                    g_GlobalDataQueue->AddQueueItem(g_sBuffer, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                }
            }
        }

        imsgLen = sprintf(g_sBuffer, "[SYS] Flood from %s (%s) - user closed by script.", u->sNick, u->sIP);
        if(CheckSprintf(imsgLen, g_szBufferSize, "DefloodWarn3") == true) {
            UdpDebug->Broadcast(g_sBuffer, imsgLen);
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
        size_t szLen;
        char * sData = (char *)lua_tolstring(L, 1, &szLen);
        if(sData[0] != '\0' && szLen < 128001) {
			if(sData[szLen-1] != '|') {
                memcpy(g_sBuffer, sData, szLen);
                g_sBuffer[szLen] = '|';
                g_sBuffer[szLen+1] = '\0';
				g_GlobalDataQueue->AddQueueItem(g_sBuffer, szLen+1, NULL, 0, GlobalDataQueue::CMD_LUA);
			} else {
				g_GlobalDataQueue->AddQueueItem(sData, szLen, NULL, 0, GlobalDataQueue::CMD_LUA);
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

    size_t szNickLen, szDataLen;
    char *sNick = (char *)lua_tolstring(L, 1, &szNickLen);
    char *sData = (char *)lua_tolstring(L, 2, &szDataLen);

    if(szNickLen == 0 || szDataLen == 0 || szDataLen > 128000) {
        lua_settop(L, 0);
        return 0;
    }

    User *u = hashManager->FindUser(sNick, szNickLen);
    if(u != NULL) {
        if(sData[szDataLen-1] != '|') {
            memcpy(g_sBuffer, sData, szDataLen);
            g_sBuffer[szDataLen] = '|';
            g_sBuffer[szDataLen+1] = '\0';
            UserSendCharDelayed(u, g_sBuffer, szDataLen+1);
        } else {
            UserSendCharDelayed(u, sData, szDataLen);
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

    size_t szDataLen;
    char * sData = (char *)lua_tolstring(L, 1, &szDataLen);

    if(szDataLen == 0 || szDataLen > 128000) {
        lua_settop(L, 0);
        return 0;
    }

    if(SettingManager->bBools[SETBOOL_REG_OP_CHAT] == true) {
        int iLen = sprintf(g_sBuffer, "%s $<%s> %s|", SettingManager->sTexts[SETTXT_OP_CHAT_NICK], SettingManager->sTexts[SETTXT_OP_CHAT_NICK], sData);
        if(CheckSprintf(iLen, g_szBufferSize, "SendToOpChat") == true) {
			g_GlobalDataQueue->SingleItemStore(g_sBuffer, iLen, NULL, 0, GlobalDataQueue::SI_OPCHAT);
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

    size_t szLen;
    char * sData = (char *)lua_tolstring(L, 1, &szLen);
    
    if(szLen == 0 || szLen > 128000) {
        lua_settop(L, 0);
        return 0;
    }

    if(sData[szLen-1] != '|') {
        memcpy(g_sBuffer, sData, szLen);
        g_sBuffer[szLen] = '|';
        g_sBuffer[szLen+1] = '\0';
        g_GlobalDataQueue->AddQueueItem(g_sBuffer, szLen+1, NULL, 0, GlobalDataQueue::CMD_OPS);
    } else {
        g_GlobalDataQueue->AddQueueItem(sData, szLen, NULL, 0, GlobalDataQueue::CMD_OPS);
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

    int32_t i32Profile = (int32_t)lua_tonumber(L, 1);

    size_t szDataLen;
    char * sData = (char *)lua_tolstring(L, 2, &szDataLen);

    if(szDataLen == 0 || szDataLen > 128000) {
        lua_settop(L, 0);
        return 0;
    }

    if(sData[szDataLen-1] != '|') {
        memcpy(g_sBuffer, sData, szDataLen);
		g_sBuffer[szDataLen] = '|';
        g_sBuffer[szDataLen+1] = '\0';
		g_GlobalDataQueue->SingleItemStore(g_sBuffer, szDataLen+1, NULL, i32Profile, GlobalDataQueue::SI_TOPROFILE);
    } else {
		g_GlobalDataQueue->SingleItemStore(sData, szDataLen, NULL, i32Profile, GlobalDataQueue::SI_TOPROFILE);
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

    size_t szLen;
    char * sData = (char *)lua_tolstring(L, 2, &szLen);

    if(szLen == 0 || szLen > 128000) {
        lua_settop(L, 0);
        return 0;
    }

    if(sData[szLen-1] != '|') {
        memcpy(g_sBuffer, sData, szLen);
        g_sBuffer[szLen] = '|';
        g_sBuffer[szLen+1] = '\0';
    	UserSendCharDelayed(u, g_sBuffer, szLen+1);
    } else {
        UserSendCharDelayed(u, sData, szLen);
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

    size_t szFromLen, szDataLen;
    char * sFrom = (char *)lua_tolstring(L, 1, &szFromLen);
    char * sData = (char *)lua_tolstring(L, 2, &szDataLen);

    if(szFromLen == 0 || szFromLen > 64 || szDataLen == 0 || szDataLen > 128000) {
        lua_settop(L, 0); 
        return 0;
    }

    int imsgLen = sprintf(g_sBuffer, "%s $<%s> %s|", sFrom, sFrom, sData);
    if(CheckSprintf(imsgLen, g_szBufferSize, "SendPmToAll") == true) {
		g_GlobalDataQueue->SingleItemStore(g_sBuffer, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2ALL);
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

    size_t szToLen, szFromLen, szDataLen;
    char * sTo = (char *)lua_tolstring(L, 1, &szToLen);
    char * sFrom = (char *)lua_tolstring(L, 2, &szFromLen);
    char * sData = (char *)lua_tolstring(L, 3, &szDataLen);

    if(szToLen == 0 || szFromLen == 0 || szFromLen > 64 || szDataLen == 0 || szDataLen > 128000) {
        lua_settop(L, 0);
        return 0;
    }

    User *u = hashManager->FindUser(sTo, szToLen);
    if(u != NULL) {
        int iMsgLen = sprintf(g_sBuffer, "$To: %s From: %s $<%s> %s|", sTo, sFrom, sFrom, sData);
        if(CheckSprintf(iMsgLen, g_szBufferSize, "SendPmToNick") == true) {
            UserSendCharDelayed(u, g_sBuffer, iMsgLen);
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

    size_t szFromLen, szDataLen;
    char * sFrom = (char *)lua_tolstring(L, 1, &szFromLen);
    char * sData = (char *)lua_tolstring(L, 2, &szDataLen);

    if(szFromLen == 0 || szFromLen > 64 || szDataLen == 0 || szDataLen > 128000) {
        lua_settop(L, 0);
        return 0;
    }

    int imsgLen = sprintf(g_sBuffer, "%s $<%s> %s|", sFrom, sFrom, sData);
    if(CheckSprintf(imsgLen, g_szBufferSize, "SendPmToOps") == true) {
		g_GlobalDataQueue->SingleItemStore(g_sBuffer, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
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

    size_t szFromLen, szDataLen;
    char * sFrom = (char *)lua_tolstring(L, 2, &szFromLen);
    char * sData = (char *)lua_tolstring(L, 3, &szDataLen);

    if(szFromLen == 0 || szFromLen > 64 || szDataLen == 0 || szDataLen > 128000) {
        lua_settop(L, 0);
        return 0;
    }

    int imsgLen = sprintf(g_sBuffer, "%s $<%s> %s|", sFrom, sFrom, sData);
    if(CheckSprintf(imsgLen, g_szBufferSize, "SendPmToProfile") == true) {
		g_GlobalDataQueue->SingleItemStore(g_sBuffer, imsgLen, NULL, iProfile, GlobalDataQueue::SI_PM2PROFILE);
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

    size_t szFromLen, szDataLen;
    char * sFrom = (char *)lua_tolstring(L, 2, &szFromLen);
    char * sData = (char *)lua_tolstring(L, 3, &szDataLen);
    
    if(szFromLen == 0 || szFromLen > 64 || szDataLen == 0 || szDataLen > 128000) {
		lua_settop(L, 0);
        return 0;
    }

    int imsgLen = sprintf(g_sBuffer, "$To: %s From: %s $<%s> %s|", u->sNick, sFrom, sFrom, sData);
    if(CheckSprintf(imsgLen, g_szBufferSize, "SendPmToUser") == true) {
        UserSendCharDelayed(u, g_sBuffer, imsgLen);
    }

    lua_settop(L, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SetUserInfo(lua_State * L) {
	if(lua_gettop(L) != 4) {
        luaL_error(L, "bad argument count to 'SetUserInfo' (4 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

    if(lua_type(L, 1) != LUA_TTABLE || lua_type(L, 2) != LUA_TNUMBER || lua_type(L, 4) != LUA_TBOOLEAN) {
        luaL_checktype(L, 1, LUA_TTABLE);
        luaL_checktype(L, 2, LUA_TNUMBER);
        luaL_checktype(L, 4, LUA_TBOOLEAN);
		lua_settop(L, 0);
        return 0;
    }

    User * pUser = ScriptGetUser(L, 4, "SetUserInfo");

    if(pUser == NULL) {
		lua_settop(L, 0);
        return 0;
    }

    int32_t iDataToChange = (int32_t)lua_tonumber(L, 2);

    if(iDataToChange < 0 || iDataToChange > 9) {
		lua_settop(L, 0);
        return 0;
    }

    bool bPermanent = lua_toboolean(L, 4) == 0 ? false : true;

    static const char * sMyInfoPartsNames[] = { "sChangedDescriptionShort", "sChangedDescriptionLong", "sChangedTagShort", "sChangedTagLong",
        "sChangedConnectionShort", "sChangedConnectionLong", "sChangedEmailShort", "sChangedEmailLong" };

    if(lua_type(L, 3) == LUA_TSTRING) {
        if(iDataToChange > 7) {
            lua_settop(L, 0);
            return 0;
        }

        size_t szDataLen;
        char * sData = (char *)lua_tolstring(L, 3, &szDataLen);

        if(szDataLen > 64 || strpbrk(sData, "$|") != NULL) {
            lua_settop(L, 0);
            return 0;
        }

        switch(iDataToChange) {
            case 0:
                pUser->sChangedDescriptionShort = UserSetUserInfo(pUser->sChangedDescriptionShort, pUser->ui8ChangedDescriptionShortLen, sData, szDataLen, sMyInfoPartsNames[iDataToChange]);
                if(bPermanent == true) {
                    pUser->ui32InfoBits |= User::INFOBIT_DESCRIPTION_SHORT_PERM;
                } else {
                    pUser->ui32InfoBits &= ~User::INFOBIT_DESCRIPTION_SHORT_PERM;
                }
                break;
            case 1:
                pUser->sChangedDescriptionLong = UserSetUserInfo(pUser->sChangedDescriptionLong, pUser->ui8ChangedDescriptionLongLen, sData, szDataLen, sMyInfoPartsNames[iDataToChange]);
                if(bPermanent == true) {
                    pUser->ui32InfoBits |= User::INFOBIT_DESCRIPTION_LONG_PERM;
                } else {
                    pUser->ui32InfoBits &= ~User::INFOBIT_DESCRIPTION_LONG_PERM;
                }
                break;
            case 2:
                pUser->sChangedTagShort = UserSetUserInfo(pUser->sChangedTagShort, pUser->ui8ChangedTagShortLen, sData, szDataLen, sMyInfoPartsNames[iDataToChange]);
                if(bPermanent == true) {
                    pUser->ui32InfoBits |= User::INFOBIT_TAG_SHORT_PERM;
                } else {
                    pUser->ui32InfoBits &= ~User::INFOBIT_TAG_SHORT_PERM;
                }
                break;
            case 3:
                pUser->sChangedTagLong = UserSetUserInfo(pUser->sChangedTagLong, pUser->ui8ChangedTagLongLen, sData, szDataLen, sMyInfoPartsNames[iDataToChange]);
                if(bPermanent == true) {
                    pUser->ui32InfoBits |= User::INFOBIT_TAG_LONG_PERM;
                } else {
                    pUser->ui32InfoBits &= ~User::INFOBIT_TAG_LONG_PERM;
                }
                break;
            case 4:
                pUser->sChangedConnectionShort = UserSetUserInfo(pUser->sChangedConnectionShort, pUser->ui8ChangedConnectionShortLen, sData, szDataLen, sMyInfoPartsNames[iDataToChange]);
                if(bPermanent == true) {
                    pUser->ui32InfoBits |= User::INFOBIT_CONNECTION_SHORT_PERM;
                } else {
                    pUser->ui32InfoBits &= ~User::INFOBIT_CONNECTION_SHORT_PERM;
                }
                break;
            case 5:
                pUser->sChangedConnectionLong = UserSetUserInfo(pUser->sChangedConnectionLong, pUser->ui8ChangedConnectionLongLen, sData, szDataLen, sMyInfoPartsNames[iDataToChange]);
                if(bPermanent == true) {
                    pUser->ui32InfoBits |= User::INFOBIT_CONNECTION_LONG_PERM;
                } else {
                    pUser->ui32InfoBits &= ~User::INFOBIT_CONNECTION_LONG_PERM;
                }
                break;
            case 6:
                pUser->sChangedEmailShort = UserSetUserInfo(pUser->sChangedEmailShort, pUser->ui8ChangedEmailShortLen, sData, szDataLen, sMyInfoPartsNames[iDataToChange]);
                if(bPermanent == true) {
                    pUser->ui32InfoBits |= User::INFOBIT_EMAIL_SHORT_PERM;
                } else {
                    pUser->ui32InfoBits &= ~User::INFOBIT_EMAIL_SHORT_PERM;
                }
                break;
            case 7:
                pUser->sChangedEmailLong = UserSetUserInfo(pUser->sChangedEmailLong, pUser->ui8ChangedEmailLongLen, sData, szDataLen, sMyInfoPartsNames[iDataToChange]);
                if(bPermanent == true) {
                    pUser->ui32InfoBits |= User::INFOBIT_EMAIL_LONG_PERM;
                } else {
                    pUser->ui32InfoBits &= ~User::INFOBIT_EMAIL_LONG_PERM;
                }
                break;
            default:
                break;
        }
    } else if(lua_type(L, 3) == LUA_TNIL) {
        switch(iDataToChange) {
            case 0:
                if(pUser->sChangedDescriptionShort != NULL) {
                    UserFreeInfo(pUser->sChangedDescriptionShort, sMyInfoPartsNames[iDataToChange]);
                    pUser->sChangedDescriptionShort = NULL;
                    pUser->ui8ChangedDescriptionShortLen = 0;
                }
                pUser->ui32InfoBits &= ~User::INFOBIT_DESCRIPTION_SHORT_PERM;
                break;
            case 1:
                if(pUser->sChangedDescriptionLong != NULL) {
                    UserFreeInfo(pUser->sChangedDescriptionLong, sMyInfoPartsNames[iDataToChange]);
                    pUser->sChangedDescriptionLong = NULL;
                    pUser->ui8ChangedDescriptionLongLen = 0;
                }
                pUser->ui32InfoBits &= ~User::INFOBIT_DESCRIPTION_LONG_PERM;
                break;
            case 2:
                if(pUser->sChangedTagShort != NULL) {
                    UserFreeInfo(pUser->sChangedTagShort, sMyInfoPartsNames[iDataToChange]);
                    pUser->sChangedTagShort = NULL;
                    pUser->ui8ChangedTagShortLen = 0;
                }
                pUser->ui32InfoBits &= ~User::INFOBIT_TAG_SHORT_PERM;
                break;
            case 3:
                if(pUser->sChangedTagLong != NULL) {
                    UserFreeInfo(pUser->sChangedTagLong, sMyInfoPartsNames[iDataToChange]);
                    pUser->sChangedTagLong = NULL;
                    pUser->ui8ChangedTagLongLen = 0;
                }
                pUser->ui32InfoBits &= ~User::INFOBIT_TAG_LONG_PERM;
                break;
            case 4:
                if(pUser->sChangedConnectionShort != NULL) {
                    UserFreeInfo(pUser->sChangedConnectionShort, sMyInfoPartsNames[iDataToChange]);
                    pUser->sChangedConnectionShort = NULL;
                    pUser->ui8ChangedConnectionShortLen = 0;
                }
                pUser->ui32InfoBits &= ~User::INFOBIT_CONNECTION_SHORT_PERM;
                break;
            case 5:
                if(pUser->sChangedConnectionLong != NULL) {
                    UserFreeInfo(pUser->sChangedConnectionLong, sMyInfoPartsNames[iDataToChange]);
                    pUser->sChangedConnectionLong = NULL;
                    pUser->ui8ChangedConnectionLongLen = 0;
                }
                pUser->ui32InfoBits &= ~User::INFOBIT_CONNECTION_LONG_PERM;
                break;
            case 6:
                if(pUser->sChangedEmailShort != NULL) {
                    UserFreeInfo(pUser->sChangedEmailShort, sMyInfoPartsNames[iDataToChange]);
                    pUser->sChangedEmailShort = NULL;
                    pUser->ui8ChangedEmailShortLen = 0;
                }
                pUser->ui32InfoBits &= ~User::INFOBIT_EMAIL_SHORT_PERM;
                break;
            case 7:
                if(pUser->sChangedEmailLong != NULL) {
                    UserFreeInfo(pUser->sChangedEmailLong, sMyInfoPartsNames[iDataToChange]);
                    pUser->sChangedEmailLong = NULL;
                    pUser->ui8ChangedEmailLongLen = 0;
                }
                pUser->ui32InfoBits &= ~User::INFOBIT_EMAIL_LONG_PERM;
                break;
            case 8:
                pUser->ui64ChangedSharedSizeShort = pUser->ui64SharedSize;
                pUser->ui32InfoBits &= ~User::INFOBIT_SHARE_SHORT_PERM;
                break;
            case 9:
                pUser->ui64ChangedSharedSizeLong = pUser->ui64SharedSize;
                pUser->ui32InfoBits &= ~User::INFOBIT_SHARE_LONG_PERM;
                break;
            default:
                break;
        }
    } else if(lua_type(L, 3) == LUA_TNUMBER) {
        if(iDataToChange < 8) {
            lua_settop(L, 0);
            return 0;
        }

        if(iDataToChange == 8) {
            pUser->ui64ChangedSharedSizeShort = (uint64_t)lua_tonumber(L, 3);
            if(bPermanent == true) {
                pUser->ui32InfoBits |= User::INFOBIT_SHARE_SHORT_PERM;
            } else {
                pUser->ui32InfoBits &= ~User::INFOBIT_SHARE_SHORT_PERM;
            }
        } else if(iDataToChange == 9) {
            pUser->ui64ChangedSharedSizeLong = (uint64_t)lua_tonumber(L, 3);
            if(bPermanent == true) {
                pUser->ui32InfoBits |= User::INFOBIT_SHARE_LONG_PERM;
            } else {
                pUser->ui32InfoBits &= ~User::INFOBIT_SHARE_LONG_PERM;
            }
        }
    } else {
        luaL_error(L, "bad argument #3 to 'SetUserInfo' (string or number or nil expected, got %s)", lua_typename(L, lua_type(L, 3)));
        lua_settop(L, 0);
        return 0;
    }

    lua_settop(L, 0);
    return 0;
}
//------------------------------------------------------------------------------

static const luaL_Reg core[] = {
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
	{ "GetHubIPs", GetHubIPs },
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
	{ "SetUserInfo", SetUserInfo },
	{ NULL, NULL }
};
//---------------------------------------------------------------------------

#if LUA_VERSION_NUM == 501
void RegCore(lua_State * L) {
    luaL_register(L, "Core", core);
#else
int RegCore(lua_State * L) {
    luaL_newlib(L, core);
#endif

    lua_pushliteral(L, "Version");
	lua_pushliteral(L, PtokaXVersionString);
	lua_settable(L, -3);
	lua_pushliteral(L, "BuildNumber");
#ifdef _WIN32
	lua_pushnumber(L, (double)_strtoui64(BUILD_NUMBER, NULL, 10));
#else
    lua_pushnumber(L, (double)strtoull(BUILD_NUMBER, NULL, 10));
#endif
	lua_settable(L, -3);

#if LUA_VERSION_NUM != 501
    return 1;
#endif
}
//---------------------------------------------------------------------------
