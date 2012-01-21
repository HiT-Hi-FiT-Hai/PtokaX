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
#include "LuaIP2CountryLib.h"
//---------------------------------------------------------------------------
#include "LuaScriptManager.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "IP2Country.h"
#include "LuaScript.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#ifndef _MSC_VER
		#pragma package(smart_init)
	#endif
#endif
//---------------------------------------------------------------------------

static int GetCountryCode(lua_State * L) {
	if(lua_gettop(L) != 1) {
		luaL_error(L, "bad argument count to 'GetCountryCode' (1 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    if(lua_type(L, 1) != LUA_TSTRING) {
        luaL_error(L, "bad argument to 'GetCountryCode' (string expected, got %s)", lua_typename(L, lua_type(L, 1)));
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

    size_t iLen;
    char *sIP = (char *)lua_tolstring(L, 1, &iLen);

    uint8_t ui128Hash[16];
    memset(ui128Hash, 0, 16);

    if(iLen == 0 || HashIP(sIP, ui128Hash) == false) {
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    const char * sCountry = IP2Country->Find(ui128Hash, false);

    lua_settop(L, 0);

    lua_pushlstring(L, sCountry, 2);

    return 1;
}
//------------------------------------------------------------------------------

static int GetCountryName(lua_State * L) {
	if(lua_gettop(L) != 1) {
		luaL_error(L, "bad argument count to 'GetCountryName' (1 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    const char * sCountry;

    if(lua_type(L, 1) == LUA_TSTRING) {
        size_t iLen;
        char *sIP = (char *)lua_tolstring(L, 1, &iLen);

        uint8_t ui128Hash[16];
        memset(ui128Hash, 0, 16);

        if(iLen == 0 || HashIP(sIP, ui128Hash) == false) {
            lua_settop(L, 0);
            lua_pushnil(L);
            return 1;
        }

        sCountry = IP2Country->Find(ui128Hash, true);
    } else if(lua_type(L, 1) == LUA_TTABLE) {
		User * u = ScriptGetUser(L, 1, "GetCountryName");
        if(u == NULL) {
    		lua_settop(L, 0);
            lua_pushnil(L);
            return 1;
        }

        sCountry = IP2Country->GetCountry(u->ui8Country, true);
    } else {
        luaL_error(L, "bad argument to 'GetCountryName' (string or table expected, got %s)", lua_typename(L, lua_type(L, 1)));
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

    lua_settop(L, 0);

    lua_pushstring(L, sCountry);

    return 1;
}
//------------------------------------------------------------------------------

#if LUA_VERSION_NUM == 501
static const luaL_reg ip2country_funcs[] =  {
#else
static const luaL_Reg ip2country[] = {
#endif
	{ "GetCountryCode", GetCountryCode },
	{ "GetCountryName", GetCountryName }, 
	{ NULL, NULL }
};
//---------------------------------------------------------------------------

#if LUA_VERSION_NUM == 501
void RegIP2Country(lua_State * L) {
    luaL_register(L, "IP2Country", ip2country_funcs);
#else
int RegIP2Country(lua_State * L) {
    luaL_newlib(L, ip2country);
    return 1;
#endif
}
//---------------------------------------------------------------------------
