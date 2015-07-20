/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2004-2015  Petr Kozelka, PPK at PtokaX dot org

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
#include "LuaTmrManLib.h"
//---------------------------------------------------------------------------
#include "LuaScriptManager.h"
#include "ServerManager.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "LuaScript.h"
//---------------------------------------------------------------------------

static int AddTimer(lua_State * L) {
	Script * cur = clsScriptManager::mPtr->FindScript(L);
    if(cur == NULL) {
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
	}

    int n = lua_gettop(L);

	size_t szLen = 0;
    char * sFunctionName = NULL;
    int iRef = 0;

	if(n == 2) {
        if(lua_type(L, 1) != LUA_TNUMBER) {
            luaL_checktype(L, 1, LUA_TNUMBER);
    		lua_settop(L, 0);
    		lua_pushnil(L);
            return 1;
        }

        if(lua_type(L, 2) == LUA_TSTRING) {
            sFunctionName = (char *)lua_tolstring(L, 2, &szLen);

            if(szLen == 0) {
				lua_settop(L, 0);
				lua_pushnil(L);
                return 1;
            }
        } else if(lua_type(L, 2) == LUA_TFUNCTION) {
            iRef = luaL_ref(L, LUA_REGISTRYINDEX);
        } else {
            luaL_error(L, "bad argument #2 to 'AddTimer' (string or function expected, got %s)", lua_typename(L, lua_type(L, 2)));
    		lua_settop(L, 0);
    		lua_pushnil(L);
            return 1;
        }
	} else if(n == 1) {
        if(lua_type(L, 1) != LUA_TNUMBER) {
            luaL_checktype(L, 1, LUA_TNUMBER);
    		lua_settop(L, 0);
    		lua_pushnil(L);
            return 1;
		}

        sFunctionName = ScriptTimer::sDefaultTimerFunc;
    } else {
        luaL_error(L, "bad argument count to 'AddTimer' (1 or 2 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    if(sFunctionName != NULL) {
        lua_getglobal(L, sFunctionName);
        int i = lua_gettop(L);
        if(lua_isfunction(L, i) == 0) {
            lua_settop(L, 0);
            lua_pushnil(L);
			return 1;
        }
    }

#ifdef _WIN32
#if LUA_VERSION_NUM < 503
	UINT_PTR timer = SetTimer(NULL, 0, (UINT)lua_tonumber(L, 1), NULL);
#else
	UINT_PTR timer = SetTimer(NULL, 0, (UINT)lua_tointeger(L, 1), NULL);
#endif

    if(timer == 0) {
        lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

	ScriptTimer * pNewtimer = ScriptTimer::CreateScriptTimer(timer, sFunctionName, szLen, iRef, cur->pLUA);
#else
	ScriptTimer * pNewtimer = ScriptTimer::CreateScriptTimer(sFunctionName, szLen, iRef, cur->pLUA);
#endif

    if(pNewtimer == NULL) {
#ifdef _WIN32
        KillTimer(NULL, timer);
#endif
        AppendDebugLog("%s - [MEM] Cannot allocate pNewtimer in TmrMan.AddTimer\n");
        lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

#ifndef _WIN32
#if LUA_VERSION_NUM < 503
	pNewtimer->ui64Interval = (uint64_t)lua_tonumber(L, 1);// ms
#else
    pNewtimer->ui64Interval = (uint64_t)lua_tointeger(L, 1);// ms
#endif
	#ifdef __MACH__
		mach_timespec_t mts;
		clock_get_time(clsServerManager::csMachClock, &mts);
		pNewtimer->ui64LastTick = (uint64_t(mts.tv_sec) * 1000) + (uint64_t(mts.tv_nsec) / 1000000);
	#else
		timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		pNewtimer->ui64LastTick = (uint64_t(ts.tv_sec) * 1000) + (uint64_t(ts.tv_nsec) / 1000000);
	#endif
#endif

    lua_settop(L, 0);

#ifdef _WIN32
    lua_pushlightuserdata(L, (void *)pNewtimer->uiTimerId);
#else
    lua_pushlightuserdata(L, (void *)pNewtimer);
#endif

    if(clsScriptManager::mPtr->pTimerListS == NULL) {
        clsScriptManager::mPtr->pTimerListS = pNewtimer;
        clsScriptManager::mPtr->pTimerListE = pNewtimer;
    } else {
    	pNewtimer->pPrev = clsScriptManager::mPtr->pTimerListE;
    	clsScriptManager::mPtr->pTimerListE->pNext = pNewtimer;
    	clsScriptManager::mPtr->pTimerListE = pNewtimer;
    }

    return 1;
}
//------------------------------------------------------------------------------

static int RemoveTimer(lua_State * L) {
    if(lua_gettop(L) != 1) {
        luaL_error(L, "bad argument count to 'RemoveTimer' (1 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

    if(lua_type(L, 1) != LUA_TLIGHTUSERDATA) {
        luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
		lua_settop(L, 0);
        return 0;
    }
    
	Script * cur = clsScriptManager::mPtr->FindScript(L);
    if(cur == NULL) {
		lua_settop(L, 0);
        return 0;
	}

#ifdef _WIN32
    UINT_PTR timer = (UINT_PTR)lua_touserdata(L, 1);
#else
	ScriptTimer * timer = (ScriptTimer *)lua_touserdata(L, 1);
#endif

    ScriptTimer * pCurTmr = NULL,
        * pNextTmr = clsScriptManager::mPtr->pTimerListS;

    while(pNextTmr != NULL) {
        pCurTmr = pNextTmr;
        pNextTmr = pCurTmr->pNext;

#ifdef _WIN32
        if(pCurTmr->uiTimerId == timer) {
            KillTimer(NULL, pCurTmr->uiTimerId);
#else
        if(pCurTmr == timer) {
#endif
			if(pCurTmr->pPrev == NULL) {
				if(pCurTmr->pNext == NULL) {
					clsScriptManager::mPtr->pTimerListS = NULL;
					clsScriptManager::mPtr->pTimerListE = NULL;
				} else {
					clsScriptManager::mPtr->pTimerListS = pCurTmr->pNext;
					clsScriptManager::mPtr->pTimerListS->pPrev = NULL;
				}
			} else if(pCurTmr->pNext == NULL) {
				clsScriptManager::mPtr->pTimerListE = pCurTmr->pPrev;
				clsScriptManager::mPtr->pTimerListE->pNext = NULL;
			} else {
				pCurTmr->pPrev->pNext = pCurTmr->pNext;
				pCurTmr->pNext->pPrev = pCurTmr->pPrev;
			}

            if(pCurTmr->sFunctionName == NULL) {
                luaL_unref(L, LUA_REGISTRYINDEX, pCurTmr->iFunctionRef);
            }

            delete pCurTmr;

            break;
        }
    }

    lua_settop(L, 0);
    return 0;
}
//------------------------------------------------------------------------------

static const luaL_Reg tmrman[] = {
	{ "AddTimer", AddTimer },
	{ "RemoveTimer", RemoveTimer }, 
	{ NULL, NULL }
};
//---------------------------------------------------------------------------

#if LUA_VERSION_NUM > 501
int RegTmrMan(lua_State * L) {
    luaL_newlib(L, tmrman);
    return 1;
#else
void RegTmrMan(lua_State * L) {
    luaL_register(L, "TmrMan", tmrman);
#endif
}
//---------------------------------------------------------------------------
