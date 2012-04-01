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
#include "LuaTmrManLib.h"
//---------------------------------------------------------------------------
#include "LuaScriptManager.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "LuaScript.h"
#ifndef _WIN32
	#include "scrtmrinc.h"
#endif
//---------------------------------------------------------------------------

static int AddTimer(lua_State * L) {
	Script * cur = ScriptManager->FindScript(L);
    if(cur == NULL) {
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
	}

    int n = lua_gettop(L);

	size_t szLen = 0;
    char * sFunctionName = NULL;

	if(n == 2) {
        if(lua_type(L, 1) != LUA_TNUMBER || lua_type(L, 2) != LUA_TSTRING) {
            luaL_checktype(L, 1, LUA_TNUMBER);
            luaL_checktype(L, 2, LUA_TSTRING);
    		lua_settop(L, 0);
    		lua_pushnil(L);
            return 1;
        }

        sFunctionName = (char *)lua_tolstring(L, 2, &szLen);

        if(szLen == 0) {
    		lua_settop(L, 0);
    		lua_pushnil(L);
            return 1;
        }

        lua_getglobal(L, sFunctionName);
        int i = lua_gettop(L);
        if(lua_isnil(L, i)) {
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

        lua_getglobal(L, "OnTimer");
        int i = lua_gettop(L);
        if(lua_isnil(L, i)) {
            lua_settop(L, 0);
            lua_pushnil(L);
			return 1;
		}
    } else {
        luaL_error(L, "bad argument count to 'AddTimer' (1 or 2 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

#ifdef _WIN32
	UINT_PTR timer = SetTimer(NULL, 0, (UINT)lua_tonumber(L, 1), NULL);

    if(timer == 0) {
        lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

	ScriptTimer * pNewtimer = new ScriptTimer(timer, sFunctionName, szLen);
#else
	ScriptTimer * pNewtimer = new ScriptTimer(sFunctionName, szLen);
#endif

    if(pNewtimer == NULL) {
#ifdef _WIN32
        KillTimer(NULL, timer);
#endif
        AppendDebugLog("%s - [MEM] Cannot allocate pNewtimer in TmrMan.AddTimer\n", 0);
		lua_pushnil(L);
        return 1;
    }

#ifndef _WIN32
    timer_t scrtimer;

    struct sigevent sigev;
    sigev.sigev_notify = SIGEV_SIGNAL;
    sigev.sigev_signo = SIGSCRTMR;
    sigev.sigev_value.sival_ptr = pNewtimer;

    int iRet = timer_create(CLOCK_REALTIME, &sigev, &scrtimer);

	if(iRet == -1) {
        lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

    pNewtimer->TimerId = scrtimer;

    uint32_t ui32Milis = (uint32_t)lua_tonumber(L, 1);// ms

    uint32_t ui32Sec = ui32Milis / 1000;
	ui32Milis = (ui32Milis-(ui32Sec*1000))*1000;

    struct itimerspec scrtmrspec;
    scrtmrspec.it_interval.tv_sec = ui32Sec;
    scrtmrspec.it_interval.tv_nsec = ui32Milis;
    scrtmrspec.it_value.tv_sec = ui32Sec;
    scrtmrspec.it_value.tv_nsec = ui32Milis;

    iRet = timer_settime(scrtimer, 0, &scrtmrspec, NULL);
    if(iRet == -1) {
        timer_delete(scrtimer);
        lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }
#endif

    lua_settop(L, 0);

#ifdef _WIN32
    lua_pushlightuserdata(L, (void *)pNewtimer->uiTimerId);
#else
    lua_pushlightuserdata(L, (void *)pNewtimer->TimerId);
#endif

    pNewtimer->prev = NULL;

    if(cur->TimerList == NULL) {
        pNewtimer->next = NULL;
    } else {
        pNewtimer->next = cur->TimerList;
        cur->TimerList->prev = pNewtimer;
    }

    cur->TimerList = pNewtimer;

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
    
	Script * cur = ScriptManager->FindScript(L);
    if(cur == NULL) {
		lua_settop(L, 0);
        return 0;
	}

#ifdef _WIN32
    UINT_PTR timer = (UINT_PTR)lua_touserdata(L, 1);
#else
	timer_t timer = (timer_t)lua_touserdata(L, 1);
#endif

    ScriptTimer * next = cur->TimerList;
    
    while(next != NULL) {
        ScriptTimer * tmr = next;
        next = tmr->next;

#ifdef _WIN32
        if(tmr->uiTimerId == timer) {
            KillTimer(NULL, tmr->uiTimerId);
#else
        if(tmr->TimerId == timer) {
            timer_delete(tmr->TimerId);
#endif

            if(tmr->prev == NULL) {
                if(tmr->next == NULL) {
                    cur->TimerList = NULL;
                } else {
                    tmr->next->prev = NULL;
                    cur->TimerList = tmr->next;
                }
            } else if(tmr->next == NULL) {
                tmr->prev->next = NULL;
            } else {
                tmr->prev->next = tmr->next;
                tmr->next->prev = tmr->prev;
            }

            delete tmr;
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

#if LUA_VERSION_NUM == 501
void RegTmrMan(lua_State * L) {
    luaL_register(L, "TmrMan", tmrman);
#else
int RegTmrMan(lua_State * L) {
    luaL_newlib(L, tmrman);
    return 1;
#endif
}
//---------------------------------------------------------------------------
