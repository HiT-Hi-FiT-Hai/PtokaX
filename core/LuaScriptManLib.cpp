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
#include "LuaScriptManLib.h"
//---------------------------------------------------------------------------
#include "eventqueue.h"
#include "LuaScriptManager.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "LuaScript.h"
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
    #include "../gui.win/MainWindowPageScripts.h"
#endif
//---------------------------------------------------------------------------

static int GetScript(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'GetScript' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

	Script * cur = ScriptManager->FindScript(L);
	if(cur == NULL) {
        lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

	lua_newtable(L);
	int s = lua_gettop(L);

	lua_pushliteral(L, "sName");
	lua_pushstring(L, cur->sName);
	lua_rawset(L, s);

	lua_pushliteral(L, "bEnabled");
	lua_pushboolean(L, 1);
	lua_rawset(L, s);

	lua_pushliteral(L, "iMemUsage");
	lua_pushnumber(L, lua_gc(cur->LUA, LUA_GCCOUNT, 0));
	lua_rawset(L, s);

    return 1;
}
//------------------------------------------------------------------------------

static int GetScripts(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'GetScripts' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

	lua_newtable(L);

	int t = lua_gettop(L), n = 0;

	for(uint8_t ui8i = 0; ui8i < ScriptManager->ui8ScriptCount; ui8i++) {
		lua_pushnumber(L, ++n);

		lua_newtable(L);
		int s = lua_gettop(L);

		lua_pushliteral(L, "sName");
		lua_pushstring(L, ScriptManager->ScriptTable[ui8i]->sName);
		lua_rawset(L, s);

		lua_pushliteral(L, "bEnabled");
		ScriptManager->ScriptTable[ui8i]->bEnabled == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
		lua_rawset(L, s);

		lua_pushliteral(L, "iMemUsage");
		ScriptManager->ScriptTable[ui8i]->LUA == NULL ? lua_pushnil(L) :
            lua_pushnumber(L, lua_gc(ScriptManager->ScriptTable[ui8i]->LUA, LUA_GCCOUNT, 0));
		lua_rawset(L, s);

		lua_rawset(L, t);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int Move(lua_State * L, const bool &bUp) {
	if(lua_gettop(L) != 1) {
		luaL_error(L, "bad argument count to '%s' (1 expected, got %d)", bUp == true ? "MoveUp" : "MoveDown", lua_gettop(L));
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
    char *sName = (char *)lua_tolstring(L, 1, &szLen);
    
    if(szLen == 0) {
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

    uint8_t ui8idx = ScriptManager->FindScriptIdx(sName);
	if(ui8idx == ScriptManager->ui8ScriptCount || (bUp == true && ui8idx == 0) ||
		(bUp == false && ui8idx == ScriptManager->ui8ScriptCount-1)) {
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

    ScriptManager->PrepareMove(L);

	ScriptManager->MoveScript(ui8idx, bUp);

#ifdef _BUILD_GUI
    pMainWindowPageScripts->MoveScript(ui8idx, bUp);
#endif

    lua_settop(L, 0);
	lua_pushboolean(L, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int MoveUp(lua_State * L) {
	return Move(L, true);
}
//------------------------------------------------------------------------------

static int MoveDown(lua_State * L) {
    return Move(L, false);
}
//------------------------------------------------------------------------------

static int StartScript(lua_State * L) {
	if(lua_gettop(L) != 1) {
        luaL_error(L, "bad argument count to 'StartScript' (1 expected, got %d)", lua_gettop(L));
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
    char * sName = (char *)lua_tolstring(L, 1, &szLen);
    
    if(szLen == 0) {
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

	if(FileExist((SCRIPT_PATH+sName).c_str()) == false) {
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
	}

    Script * curScript = ScriptManager->FindScript(sName);
    if(curScript != NULL) {
        lua_settop(L, 0);

        if(curScript->LUA != NULL) {
    		lua_pushnil(L);
            return 1;
        }

		if(ScriptManager->StartScript(curScript, true) == false) {
    		lua_pushnil(L);
            return 1;
        }

    	lua_pushboolean(L, 1);
        return 1;
	}

	if(ScriptManager->AddScript(sName, true, true) == true && ScriptManager->StartScript(ScriptManager->ScriptTable[ScriptManager->ui8ScriptCount-1], false) == true) {
        lua_settop(L, 0);
    	lua_pushboolean(L, 1);
        return 1;
    }

    lua_settop(L, 0);
    lua_pushnil(L);
    return 1;
}
//------------------------------------------------------------------------------

static int RestartScript(lua_State * L) {
	if(lua_gettop(L) != 1) {
        luaL_error(L, "bad argument count to 'RestartScript' (1 expected, got %d)", lua_gettop(L));
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
    char *sName = (char *)lua_tolstring(L, 1, &szLen);
    
    if(szLen == 0) {
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

    Script * curScript = ScriptManager->FindScript(sName);
    if(curScript == NULL || curScript->LUA == NULL) {
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

    lua_settop(L, 0);

    if(curScript->LUA == L) {
        eventqueue->AddNormal(eventq::EVENT_RSTSCRIPT, curScript->sName);

    	lua_pushboolean(L, 1);
        return 1;
    }

    ScriptManager->StopScript(curScript, false);

    if(ScriptManager->StartScript(curScript, false) == true) {
    	lua_pushboolean(L, 1);
        return 1;
    }

	lua_pushnil(L);
    return 1;
}
//------------------------------------------------------------------------------

static int StopScript(lua_State * L) {
	if(lua_gettop(L) != 1) {
        luaL_error(L, "bad argument count to 'StopScript' (1 expected, got %d)", lua_gettop(L));
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
    char * sName = (char *)lua_tolstring(L, 1, &szLen);

    if(szLen == 0) {
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

    Script * curScript = ScriptManager->FindScript(sName);
    if(curScript == NULL || curScript->LUA == NULL) {
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
	}

	lua_settop(L, 0);

    if(curScript->LUA == L) {
		eventqueue->AddNormal(eventq::EVENT_STOPSCRIPT, curScript->sName);

    	lua_pushboolean(L, 1);
        return 1;
    }

    ScriptManager->StopScript(curScript, true);

	lua_pushboolean(L, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int Restart(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'Restart' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

    eventqueue->AddNormal(eventq::EVENT_RSTSCRIPTS, NULL);

    return 0;
}
//------------------------------------------------------------------------------

static int Refresh(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'Refresh' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

    ScriptManager->CheckForDeletedScripts();
	ScriptManager->CheckForNewScripts();

#ifdef _BUILD_GUI
	pMainWindowPageScripts->AddScriptsToList(true);
#endif

    return 0;
}
//------------------------------------------------------------------------------

static const luaL_Reg scriptman[] = {
	{ "GetScript", GetScript },
	{ "GetScripts", GetScripts }, 
	{ "MoveUp", MoveUp }, 
	{ "MoveDown", MoveDown }, 
	{ "StartScript", StartScript }, 
	{ "RestartScript", RestartScript }, 
    { "StopScript", StopScript }, 
	{ "Restart", Restart }, 
	{ "Refresh", Refresh }, 
	{ NULL, NULL }
};
//---------------------------------------------------------------------------

#if LUA_VERSION_NUM == 501
void RegScriptMan(lua_State * L) {
    luaL_register(L, "ScriptMan", scriptman);
#else
int RegScriptMan(lua_State * L) {
    luaL_newlib(L, scriptman);
    return 1;
#endif
}
//---------------------------------------------------------------------------
