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
#include "LuaProfManLib.h"
//---------------------------------------------------------------------------
#include "ProfileManager.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------

static void PushProfilePermissions(lua_State * L, const uint16_t &iProfile) {
    ProfileItem *Prof = ProfileMan->ProfilesTable[iProfile];

    lua_checkstack(L, 3); // we need 3 (1 table, 2 id, 3 value) empty slots in stack, check it to be sure

    lua_newtable(L);
    int t = lua_gettop(L);

    lua_pushliteral(L, "bIsOP");
    Prof->bPermissions[ProfileManager::HASKEYICON] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bNoDefloodGetNickList");
    Prof->bPermissions[ProfileManager::NODEFLOODGETNICKLIST] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bNoDefloodNMyINFO");
    Prof->bPermissions[ProfileManager::NODEFLOODMYINFO] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bNoDefloodSearch");
    Prof->bPermissions[ProfileManager::NODEFLOODSEARCH] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bNoDefloodPM");
    Prof->bPermissions[ProfileManager::NODEFLOODPM] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bNoDefloodMainChat");
    Prof->bPermissions[ProfileManager::NODEFLOODMAINCHAT] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bMassMsg");
    Prof->bPermissions[ProfileManager::MASSMSG] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bTopic");
    Prof->bPermissions[ProfileManager::TOPIC] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bTempBan");
    Prof->bPermissions[ProfileManager::TEMP_BAN] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bTempUnban");
    Prof->bPermissions[ProfileManager::TEMP_UNBAN] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);
    
    lua_pushliteral(L, "bRefreshTxt");
    Prof->bPermissions[ProfileManager::REFRESHTXT] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bNoTagCheck");
    Prof->bPermissions[ProfileManager::NOTAGCHECK] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bDelRegUser");
    Prof->bPermissions[ProfileManager::DELREGUSER] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bAddRegUser");
    Prof->bPermissions[ProfileManager::ADDREGUSER] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bNoChatLimits");
    Prof->bPermissions[ProfileManager::NOCHATLIMITS] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bNoMaxHubCheck");
    Prof->bPermissions[ProfileManager::NOMAXHUBCHECK] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bNoSlotHubRatio");
    Prof->bPermissions[ProfileManager::NOSLOTHUBRATIO] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bNoSlotCheck");
    Prof->bPermissions[ProfileManager::NOSLOTCHECK] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bNoShareLimit");
    Prof->bPermissions[ProfileManager::NOSHARELIMIT] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bClrPermBan");
    Prof->bPermissions[ProfileManager::CLRPERMBAN] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bClrTempBan");
    Prof->bPermissions[ProfileManager::CLRTEMPBAN] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bGetInfo");
    Prof->bPermissions[ProfileManager::GETINFO] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bGetBans");
    Prof->bPermissions[ProfileManager::GETBANLIST] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bRestartScripts");
    Prof->bPermissions[ProfileManager::RSTSCRIPTS] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bRestartHub");
    Prof->bPermissions[ProfileManager::RSTHUB] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bTempOP");
    Prof->bPermissions[ProfileManager::TEMPOP] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bGag");
    Prof->bPermissions[ProfileManager::GAG] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bRedirect");
    Prof->bPermissions[ProfileManager::REDIRECT] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bBan");
    Prof->bPermissions[ProfileManager::BAN] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bUnban");
    Prof->bPermissions[ProfileManager::UNBAN] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);
    
    lua_pushliteral(L, "bKick");
    Prof->bPermissions[ProfileManager::KICK] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bDrop");
    Prof->bPermissions[ProfileManager::DROP] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bEnterFullHub");
    Prof->bPermissions[ProfileManager::ENTERFULLHUB] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bEnterIfIPBan");
    Prof->bPermissions[ProfileManager::ENTERIFIPBAN] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bAllowedOPChat");
    Prof->bPermissions[ProfileManager::ALLOWEDOPCHAT] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bSendFullMyinfos");
    Prof->bPermissions[ProfileManager::SENDFULLMYINFOS] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);   
    
    lua_pushliteral(L, "bSendAllUserIP");
    Prof->bPermissions[ProfileManager::SENDALLUSERIP] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t); 
    
    lua_pushliteral(L, "bRangeBan");
    Prof->bPermissions[ProfileManager::RANGE_BAN] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);
    
    lua_pushliteral(L, "bRangeUnban");
    Prof->bPermissions[ProfileManager::RANGE_UNBAN] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bRangeTempBan");
    Prof->bPermissions[ProfileManager::RANGE_TBAN] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bRangeTempUnban");
    Prof->bPermissions[ProfileManager::RANGE_TUNBAN] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);
    
    lua_pushliteral(L, "bGetRangeBans");
    Prof->bPermissions[ProfileManager::GET_RANGE_BANS] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bClearRangePermBans");
    Prof->bPermissions[ProfileManager::CLR_RANGE_BANS] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bClearRangeTempBans");
    Prof->bPermissions[ProfileManager::CLR_RANGE_TBANS] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);
    
    lua_pushliteral(L, "bNoIpCheck");
    Prof->bPermissions[ProfileManager::NOIPCHECK] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);
    
    lua_pushliteral(L, "bClose");
    Prof->bPermissions[ProfileManager::CLOSE] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bNoSearchLimits");
    Prof->bPermissions[ProfileManager::NOSEARCHLIMITS] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bNoDefloodCTM");
    Prof->bPermissions[ProfileManager::NODEFLOODCTM] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bNoDefloodRCTM");
    Prof->bPermissions[ProfileManager::NODEFLOODRCTM] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bNoDefloodSR");
    Prof->bPermissions[ProfileManager::NODEFLOODSR] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bNoDefloodRecv");
    Prof->bPermissions[ProfileManager::NODEFLOODRECV] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bNoChatInterval");
    Prof->bPermissions[ProfileManager::NOCHATINTERVAL] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bNoPMInterval");
    Prof->bPermissions[ProfileManager::NOPMINTERVAL] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bNoSearchInterval");
    Prof->bPermissions[ProfileManager::NOSEARCHINTERVAL] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bNoMaxUsersSameIP");
    Prof->bPermissions[ProfileManager::NOUSRSAMEIP] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);

    lua_pushliteral(L, "bNoReConnTime");
    Prof->bPermissions[ProfileManager::NORECONNTIME] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, t);
}
//------------------------------------------------------------------------------

static void PushProfile(lua_State * L, const uint16_t &iProfile) {
	lua_checkstack(L, 3); // we need 3 (1 table, 2 id, 3 value) empty slots in stack, check it to be sure

    lua_newtable(L);
    int i = lua_gettop(L);

    lua_pushliteral(L, "sProfileName");
	lua_pushstring(L, ProfileMan->ProfilesTable[iProfile]->sName);
    lua_rawset(L, i);

    lua_pushliteral(L, "iProfileNumber");
	lua_pushnumber(L, iProfile);
    lua_rawset(L, i);

    lua_pushliteral(L, "tProfilePermissions");
	PushProfilePermissions(L, iProfile);
    lua_rawset(L, i);
}
//------------------------------------------------------------------------------

static int AddProfile(lua_State * L) {
	if(lua_gettop(L) != 1) {
        luaL_error(L, "bad argument count to 'AddProfile' (1 expected, got %d)", lua_gettop(L));
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
    char * sProfileName = (char *)lua_tolstring(L, 1, &szLen);

    if(szLen == 0 || szLen > 64) {
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

	int32_t idx = ProfileMan->AddProfile(sProfileName);
    if(idx == -1 || idx == -2) {
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    lua_settop(L, 0);
    lua_pushnumber(L, idx);
    return 1;
}
//------------------------------------------------------------------------------

static int RemoveProfile(lua_State * L) {
	if(lua_gettop(L) != 1) {
        luaL_error(L, "bad argument count to 'RemoveProfile' (1 expected, got %d)", lua_gettop(L));
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
    char * sProfileName = (char *)lua_tolstring(L, 1, &szLen);

    if(szLen == 0) {
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

    int32_t result = ProfileMan->RemoveProfileByName(sProfileName);
    if(result != 1) {
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

    lua_settop(L, 0);
    lua_pushboolean(L, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int MoveDown(lua_State * L) {
	if(lua_gettop(L) != 1) {
        luaL_error(L, "bad argument count to 'MoveDown' (1 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    if(lua_type(L, 1) != LUA_TNUMBER) {
        luaL_checktype(L, 1, LUA_TNUMBER);

		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

	uint16_t iProfile = (uint16_t)lua_tonumber(L, 1);
    
    // if the requested index is out of bounds return nil
    if(iProfile >= ProfileMan->iProfileCount-1) {
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
	}

    ProfileMan->MoveProfileDown(iProfile);

    lua_settop(L, 0);
    lua_pushboolean(L, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int MoveUp(lua_State * L) {
	if(lua_gettop(L) != 1) {
        luaL_error(L, "bad argument count to 'MoveUp' (1 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    if(lua_type(L, 1) != LUA_TNUMBER) {
        luaL_checktype(L, 1, LUA_TNUMBER);

		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

	uint16_t iProfile = (uint16_t)lua_tonumber(L, 1);
    
    // if the requested index is out of bounds return nil
    if(iProfile == 0 || iProfile >= ProfileMan->iProfileCount) {
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
	}

    ProfileMan->MoveProfileUp(iProfile);

    lua_settop(L, 0);
    lua_pushboolean(L, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int GetProfile(lua_State * L) {
	if(lua_gettop(L) != 1) {
        luaL_error(L, "bad argument count to 'GetProfile' (1 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    if(lua_type(L, 1) == LUA_TSTRING) {
        size_t szLen;
        char *profName = (char *)lua_tolstring(L, 1, &szLen);

        if(szLen == 0) {
            lua_pushnil(L);
            return 1;
        }

		int32_t idx = ProfileMan->GetProfileIndex(profName);

        lua_settop(L, 0);

        if(idx == -1) {
            lua_pushnil(L);
            return 1;
        }

        PushProfile(L, (uint16_t)idx);
        return 1;
    } else if(lua_type(L, 1) == LUA_TNUMBER) {
    	uint16_t idx = (uint16_t)lua_tonumber(L, 1);

    	lua_settop(L, 0);
    
        // if the requested index is out of bounds return nil
        if(idx >= ProfileMan->iProfileCount) {
            lua_pushnil(L);
            return 1;
        }
    
        PushProfile(L, idx);
        return 1;
    } else {
        luaL_error(L, "bad argument #1 to 'GetProfile' (string or number expected, got %d)", lua_typename(L, lua_type(L, 1)));
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }
}
//------------------------------------------------------------------------------

static int GetProfiles(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'GetProfiles' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    lua_newtable(L);
    int t = lua_gettop(L);

    for(uint16_t ui16i = 0; ui16i < ProfileMan->iProfileCount; ui16i++) {
        lua_pushnumber(L, (ui16i+1));
        PushProfile(L, ui16i);
        lua_rawset(L, t);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetProfilePermission(lua_State * L) {
	if(lua_gettop(L) != 2) {
        luaL_error(L, "bad argument count to 'GetProfilePermission' (2 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    if(lua_type(L, 1) != LUA_TNUMBER || lua_type(L, 2) != LUA_TNUMBER) {
        luaL_checktype(L, 1, LUA_TNUMBER);
        luaL_checktype(L, 2, LUA_TNUMBER);
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

    uint16_t iProfile = (uint16_t)lua_tonumber(L, 1);
    size_t szId = (size_t)lua_tonumber(L, 2);
    
    lua_settop(L, 0);
    
	if(iProfile >= ProfileMan->iProfileCount) {
        lua_pushnil(L);
        return 1;
    }

	if(szId > ProfileManager::NORECONNTIME) {
		luaL_error(L, "bad argument #2 to 'GetProfilePermission' (it's not valid id)");
		lua_pushnil(L);
		return 1;
	}

    ProfileMan->ProfilesTable[iProfile]->bPermissions[szId] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);

    return 1;
}
//------------------------------------------------------------------------------

static int GetProfilePermissions(lua_State * L) {
	if(lua_gettop(L) != 1) {
        luaL_error(L, "bad argument count to 'GetProfilePermissions' (1 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    if(lua_type(L, 1) != LUA_TNUMBER) {
        luaL_checktype(L, 1, LUA_TNUMBER);

		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

	uint16_t iProfile = (uint16_t)lua_tonumber(L, 1);
    
    // if the requested index is out of bounds return nil
    if(iProfile >= ProfileMan->iProfileCount) {
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
	}

    PushProfilePermissions(L, iProfile);

    return 1;
}
//------------------------------------------------------------------------------

static int SetProfileName(lua_State * L) {
	if(lua_gettop(L) != 2) {
        luaL_error(L, "bad argument count to 'SetProfileName' (2 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    if(lua_type(L, 1) != LUA_TNUMBER || lua_type(L, 2) != LUA_TSTRING) {
        luaL_checktype(L, 1, LUA_TNUMBER);
        luaL_checktype(L, 2, LUA_TSTRING);
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

    uint16_t iProfile = (uint16_t)lua_tonumber(L, 1);

    if(iProfile >= ProfileMan->iProfileCount) {
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    size_t szLen;
	char * sName = (char *)lua_tolstring(L, 2, &szLen);

    if(szLen == 0 || szLen > 64) {
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

	ProfileMan->ChangeProfileName(iProfile, sName, szLen);

    lua_settop(L, 0);
    lua_pushboolean(L, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int SetProfilePermission(lua_State * L) {
	if(lua_gettop(L) != 3) {
        luaL_error(L, "bad argument count to 'SetProfilePermission' (3 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    if(lua_type(L, 1) != LUA_TNUMBER || lua_type(L, 2) != LUA_TNUMBER || lua_type(L, 3) != LUA_TBOOLEAN) {
        luaL_checktype(L, 1, LUA_TNUMBER);
        luaL_checktype(L, 2, LUA_TNUMBER);
        luaL_checktype(L, 3, LUA_TBOOLEAN);
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

    uint16_t iProfile = (uint16_t)lua_tonumber(L, 1);

    size_t szId = (size_t)lua_tonumber(L, 2);

    bool bValue = lua_toboolean(L, 3) == 0 ? false : true;
    
    lua_settop(L, 0);
    
	if(iProfile >= ProfileMan->iProfileCount) {
		lua_pushnil(L);
		return 1;
	}

	if(szId > ProfileManager::NORECONNTIME) {
		luaL_error(L, "bad argument #2 to 'SetProfilePermission' (it's not valid id)");
		lua_pushnil(L);
		return 1;
	}

    ProfileMan->ChangeProfilePermission(iProfile, szId, bValue);

    lua_pushboolean(L, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int Save(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'Save' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

	ProfileMan->SaveProfiles();

    return 0;
}
//------------------------------------------------------------------------------

static const luaL_Reg profman[] = {
	{ "AddProfile", AddProfile },
	{ "RemoveProfile", RemoveProfile }, 
	{ "MoveDown", MoveDown }, 
	{ "MoveUp", MoveUp }, 
	{ "GetProfile", GetProfile }, 
	{ "GetProfiles", GetProfiles }, 
	{ "GetProfilePermission", GetProfilePermission }, 
	{ "GetProfilePermissions", GetProfilePermissions }, 
	{ "SetProfileName", SetProfileName }, 
	{ "SetProfilePermission", SetProfilePermission }, 
	{ "Save", Save },
	{ NULL, NULL }
};
//---------------------------------------------------------------------------

#if LUA_VERSION_NUM == 501
void RegProfMan(lua_State * L) {
    luaL_register(L, "ProfMan", profman);
#else
int RegProfMan(lua_State * L) {
    luaL_newlib(L, profman);
    return 1;
#endif
}
//---------------------------------------------------------------------------
