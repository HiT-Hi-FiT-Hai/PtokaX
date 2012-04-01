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
#include "LuaSetManLib.h"
//---------------------------------------------------------------------------
#include "eventqueue.h"
#include "hashUsrManager.h"
#include "LuaScriptManager.h"
#include "SettingManager.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------

static int Save(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'Save' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

    ScriptManager->SaveScripts();

	SettingManager->Save();

    return 0;
}
//------------------------------------------------------------------------------

static int GetMOTD(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'GetMOTD' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 0;
    }

	if(SettingManager->sMOTD != NULL) {
        lua_pushlstring(L, SettingManager->sMOTD, (size_t)SettingManager->ui16MOTDLen);
    } else {
        lua_pushnil(L);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int SetMOTD(lua_State * L) {
	if(lua_gettop(L) != 1) {
        luaL_error(L, "bad argument count to 'SetMOTD' (1 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

    if(lua_type(L, 1) != LUA_TSTRING) {
        luaL_checktype(L, 1, LUA_TSTRING);
		lua_settop(L, 0);
        return 0;
    }

    size_t szLen = 0;
    char * sTxt = (char *)lua_tolstring(L, 1, &szLen);

	SettingManager->SetMOTD(sTxt, szLen);

	SettingManager->UpdateMOTD();

    lua_settop(L, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int GetBool(lua_State * L) {
	if(lua_gettop(L) != 1) {
        luaL_error(L, "bad argument count to 'GetBool' (1 expected, got %d)", lua_gettop(L));
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

    size_t szId = (size_t)lua_tonumber(L, 1);

    lua_settop(L, 0);

    if(szId >= SETBOOL_IDS_END) {
        luaL_error(L, "bad argument #1 to 'GetBool' (it's not valid id)");
        lua_pushnil(L);
        return 1;
    }  

    SettingManager->bBools[szId] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);

    return 1;
}
//------------------------------------------------------------------------------

static int SetBool(lua_State * L) {
	if(lua_gettop(L) != 2) {
        luaL_error(L, "bad argument count to 'SetBool' (2 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

    if(lua_type(L, 1) != LUA_TNUMBER || lua_type(L, 2) != LUA_TBOOLEAN) {
        luaL_checktype(L, 1, LUA_TNUMBER);
        luaL_checktype(L, 2, LUA_TBOOLEAN);
        lua_settop(L, 0);
        return 0;
    }

    size_t szId = (size_t)lua_tonumber(L, 1);
    bool bValue = (lua_toboolean(L, 2) == 0 ? false : true);

    lua_settop(L, 0);

    if(szId >= SETBOOL_IDS_END) {
        luaL_error(L, "bad argument #1 to 'SetBool' (it's not valid id)");
        return 0;
    }  

    if(szId == 22 && bValue == false) {
        eventqueue->AddNormal(eventq::EVENT_STOP_SCRIPTING, NULL);
        return 0;
    }

    SettingManager->SetBool(szId, bValue);

    return 0;
}
//------------------------------------------------------------------------------

static int GetNumber(lua_State * L) {
	if(lua_gettop(L) != 1) {
        luaL_error(L, "bad argument count to 'GetNumber' (1 expected, got %d)", lua_gettop(L));
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

    size_t szId = (size_t)lua_tonumber(L, 1);

    lua_settop(L, 0);

	if(szId >= (SETSHORT_IDS_END-1)) {
        luaL_error(L, "bad argument #1 to 'GetNumber' (it's not valid id)");
        lua_pushnil(L);
        return 1;
    }  

    lua_pushnumber(L, SettingManager->iShorts[szId]);

    return 1;
}
//------------------------------------------------------------------------------

static int SetNumber(lua_State * L) {
	if(lua_gettop(L) != 2) {
        luaL_error(L, "bad argument count to 'SetNumber' (2 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

    if(lua_type(L, 1) != LUA_TNUMBER || lua_type(L, 2) != LUA_TNUMBER) {
        luaL_checktype(L, 1, LUA_TNUMBER);
        luaL_checktype(L, 2, LUA_TNUMBER);
        lua_settop(L, 0);
        return 0;
    }

    size_t szId = (size_t)lua_tonumber(L, 1);
    int16_t iValue = (int16_t)lua_tonumber(L, 2);

    lua_settop(L, 0);

	if(szId >= (SETSHORT_IDS_END-1)) {
        luaL_error(L, "bad argument #1 to 'SetNumber' (it's not valid id)");
        return 0;
    }  

    SettingManager->SetShort(szId, iValue);

    return 0;
}
//------------------------------------------------------------------------------

static int GetString(lua_State * L) {
	if(lua_gettop(L) != 1) {
        luaL_error(L, "bad argument count to 'GetString' (1 expected, got %d)", lua_gettop(L));
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

    size_t szId = (size_t)lua_tonumber(L, 1);

    lua_settop(L, 0);

    if(szId >= SETTXT_IDS_END) {
        luaL_error(L, "bad argument #1 to 'GetString' (it's not valid id)");
        lua_pushnil(L);
        return 1;
    }  

    if(SettingManager->sTexts[szId] != NULL) {
        lua_pushlstring(L, SettingManager->sTexts[szId], (size_t)SettingManager->ui16TextsLens[szId]);
    } else {
        lua_pushnil(L);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int SetString(lua_State * L) {
	if(lua_gettop(L) != 2) {
        luaL_error(L, "bad argument count to 'SetString' (2 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

    if(lua_type(L, 1) != LUA_TNUMBER || lua_type(L, 2) != LUA_TSTRING) {
        luaL_checktype(L, 1, LUA_TNUMBER);
        luaL_checktype(L, 2, LUA_TSTRING);
        lua_settop(L, 0);
        return 0;
    }

    size_t szId = (size_t)lua_tonumber(L, 1);

    if(szId >= SETTXT_IDS_END) {
        luaL_error(L, "bad argument #1 to 'SetString' (it's not valid id)");
        lua_settop(L, 0);
        return 0;
    }  

    size_t szLen;
    char * sValue = (char *)lua_tolstring(L, 2, &szLen);

    SettingManager->SetText(szId, sValue, szLen);

    lua_settop(L, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int GetMinShare(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'GetMinShare' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 0;
    }
    
    lua_pushnumber(L, (double)SettingManager->ui64MinShare);

    return 1;
}
//------------------------------------------------------------------------------

static int SetMinShare(lua_State * L) {
    int n = lua_gettop(L);

    if(n == 2) {
        if(lua_type(L, 1) != LUA_TNUMBER || lua_type(L, 2) != LUA_TNUMBER) {
            luaL_checktype(L, 1, LUA_TNUMBER);
            luaL_checktype(L, 2, LUA_TNUMBER);
            lua_settop(L, 0);
            return 0;
        }

		SettingManager->bUpdateLocked = true;

        SettingManager->SetShort(SETSHORT_MIN_SHARE_LIMIT, (int16_t)lua_tonumber(L, 1));
		SettingManager->SetShort(SETSHORT_MIN_SHARE_UNITS, (int16_t)lua_tonumber(L, 2));

		SettingManager->bUpdateLocked = false;
    } else if(n == 1) {
        if(lua_type(L, 1) != LUA_TNUMBER) {
            luaL_checktype(L, 1, LUA_TNUMBER);
            lua_settop(L, 0);
            return 0;
        }

		double dBytes = lua_tonumber(L, 1);
        uint16_t iter = 0;
		for(; dBytes > 1024; iter++) {
			dBytes /= 1024;
		}

		SettingManager->bUpdateLocked = true;

        SettingManager->SetShort(SETSHORT_MIN_SHARE_LIMIT, (int16_t)dBytes);
		SettingManager->SetShort(SETSHORT_MIN_SHARE_UNITS, iter);

		SettingManager->bUpdateLocked = false;
    } else {
        luaL_error(L, "bad argument count to 'SetMinShare' (1 or 2 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

	SettingManager->UpdateMinShare();
	SettingManager->UpdateShareLimitMessage();

    lua_settop(L, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int GetMaxShare(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'GetMaxShare' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 0;
    }
    
    lua_pushnumber(L, (double)SettingManager->ui64MaxShare);

    return 1;
}
//------------------------------------------------------------------------------

static int SetMaxShare(lua_State * L) {
    int n = lua_gettop(L);

    if(n == 2) {
        if(lua_type(L, 1) != LUA_TNUMBER || lua_type(L, 2) != LUA_TNUMBER) {
            luaL_checktype(L, 1, LUA_TNUMBER);
            luaL_checktype(L, 2, LUA_TNUMBER);
            lua_settop(L, 0);
            return 0;
        }

		SettingManager->bUpdateLocked = true;

        SettingManager->SetShort(SETSHORT_MAX_SHARE_LIMIT, (int16_t)lua_tonumber(L, 1));
		SettingManager->SetShort(SETSHORT_MAX_SHARE_UNITS, (int16_t)lua_tonumber(L, 2));

		SettingManager->bUpdateLocked = false;
    } else if(n == 1) {
        if(lua_type(L, 1) != LUA_TNUMBER) {
            luaL_checktype(L, 1, LUA_TNUMBER);
            lua_settop(L, 0);
            return 0;
        }

       	double dBytes = (double)lua_tonumber(L, 1);
        uint16_t iter = 0;
		for(; dBytes > 1024; iter++) {
			dBytes /= 1024;
		}

		SettingManager->bUpdateLocked = true;

		SettingManager->SetShort(SETSHORT_MAX_SHARE_LIMIT, (int16_t)dBytes);
		SettingManager->SetShort(SETSHORT_MAX_SHARE_UNITS, iter);

		SettingManager->bUpdateLocked = false;
    } else {
        luaL_error(L, "bad argument count to 'SetMaxShare' (1 or 2 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

	SettingManager->UpdateMaxShare();
	SettingManager->UpdateShareLimitMessage();

    lua_settop(L, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SetHubSlotRatio(lua_State * L) {
	if(lua_gettop(L) != 2) {
        luaL_error(L, "bad argument count to 'SetHubSlotRatio' (2 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

    if(lua_type(L, 1) != LUA_TNUMBER || lua_type(L, 2) != LUA_TNUMBER) {
        luaL_checktype(L, 1, LUA_TNUMBER);
        luaL_checktype(L, 2, LUA_TNUMBER);
        lua_settop(L, 0);
        return 0;
    }

	SettingManager->bUpdateLocked = true;

    SettingManager->SetShort(SETSHORT_HUB_SLOT_RATIO_HUBS, (int16_t)lua_tonumber(L, 1));
	SettingManager->SetShort(SETSHORT_HUB_SLOT_RATIO_SLOTS, (int16_t)lua_tonumber(L, 2));

	SettingManager->bUpdateLocked = false;

	SettingManager->UpdateHubSlotRatioMessage();

    lua_settop(L, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int GetOpChat(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'GetOpChat' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 0;
    }

	lua_newtable(L);
	int i = lua_gettop(L);

	lua_pushliteral(L, "sNick");
    if(SettingManager->sTexts[SETTXT_OP_CHAT_NICK] == NULL) {
		lua_pushnil(L);
	} else {
        lua_pushlstring(L, SettingManager->sTexts[SETTXT_OP_CHAT_NICK], 
        (size_t)SettingManager->ui16TextsLens[SETTXT_OP_CHAT_NICK]);
	}
	lua_rawset(L, i);

	lua_pushliteral(L, "sDescription");
    if(SettingManager->sTexts[SETTXT_OP_CHAT_DESCRIPTION] == NULL) {
		lua_pushnil(L);
	} else {
        lua_pushlstring(L, SettingManager->sTexts[SETTXT_OP_CHAT_DESCRIPTION], 
        (size_t)SettingManager->ui16TextsLens[SETTXT_OP_CHAT_DESCRIPTION]);
	}
	lua_rawset(L, i);

	lua_pushliteral(L, "sEmail");
    if(SettingManager->sTexts[SETTXT_OP_CHAT_EMAIL] == NULL) {
		lua_pushnil(L);
	} else {
        lua_pushlstring(L, SettingManager->sTexts[SETTXT_OP_CHAT_EMAIL], 
        (size_t)SettingManager->ui16TextsLens[SETTXT_OP_CHAT_EMAIL]);
	}
	lua_rawset(L, i);

    lua_pushliteral(L, "bEnabled");
    SettingManager->bBools[SETBOOL_REG_OP_CHAT] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, i);

    return 1;
}
//------------------------------------------------------------------------------

static int SetOpChat(lua_State * L) {
	if(lua_gettop(L) != 4) {
        luaL_error(L, "bad argument count to 'SetOpChat' (4 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

    if(lua_type(L, 1) != LUA_TBOOLEAN || lua_type(L, 2) != LUA_TSTRING || lua_type(L, 3) != LUA_TSTRING || lua_type(L, 4) != LUA_TSTRING) {
        luaL_checktype(L, 1, LUA_TBOOLEAN);
        luaL_checktype(L, 2, LUA_TSTRING);
        luaL_checktype(L, 3, LUA_TSTRING);
        luaL_checktype(L, 4, LUA_TSTRING);
        lua_settop(L, 0);
        return 0;
    }

    char *NewBotName, *NewBotDescription, *NewBotEmail;
    size_t szNameLen, szDescrLen, szEmailLen;

    NewBotName = (char *)lua_tolstring(L, 2, &szNameLen);
    NewBotDescription = (char *)lua_tolstring(L, 3, &szDescrLen);
    NewBotEmail = (char *)lua_tolstring(L, 4, &szEmailLen);

    if(szNameLen == 0 || szNameLen > 64 || szDescrLen > 64 || szEmailLen > 64 ||
        strpbrk(NewBotName, " $|") != NULL || strpbrk(NewBotDescription, "$|") != NULL ||
		strpbrk(NewBotEmail, "$|") != NULL || hashManager->FindUser(NewBotName, szNameLen) != NULL) {
        lua_settop(L, 0);
        return 0;
    }

    bool bBotHaveNewNick = (strcmp(SettingManager->sTexts[SETTXT_OP_CHAT_NICK], NewBotName) != 0);

    if(SettingManager->bBools[SETBOOL_REG_OP_CHAT] == true) {
        SettingManager->DisableOpChat(bBotHaveNewNick);
    }

    SettingManager->bUpdateLocked = true;

    SettingManager->SetBool(SETBOOL_REG_OP_CHAT, (lua_toboolean(L, 1) == 0 ? false : true));

    if(bBotHaveNewNick == true) {
        SettingManager->SetText(SETTXT_OP_CHAT_NICK, NewBotName);
    }

    if(SettingManager->sTexts[SETTXT_OP_CHAT_DESCRIPTION] == NULL || 
        strcmp(SettingManager->sTexts[SETTXT_OP_CHAT_DESCRIPTION], NewBotDescription) != 0) {
        SettingManager->SetText(SETTXT_OP_CHAT_DESCRIPTION, NewBotDescription);
    }

    if(SettingManager->sTexts[SETTXT_OP_CHAT_EMAIL] == NULL || 
        strcmp(SettingManager->sTexts[SETTXT_OP_CHAT_EMAIL], NewBotEmail) != 0) {
        SettingManager->SetText(SETTXT_OP_CHAT_EMAIL, NewBotEmail);
    }

	SettingManager->bUpdateLocked = false;

    SettingManager->UpdateBotsSameNick();

    if(SettingManager->bBools[SETBOOL_REG_OP_CHAT] == true) {
        SettingManager->UpdateOpChat(bBotHaveNewNick);
    }

    lua_settop(L, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int GetHubBot(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'GetHubBot' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 0;
    }

	lua_newtable(L);
	int i = lua_gettop(L);

	lua_pushliteral(L, "sNick");
    if(SettingManager->sTexts[SETTXT_BOT_NICK] == NULL) {
		lua_pushnil(L);
	} else {
        lua_pushlstring(L, SettingManager->sTexts[SETTXT_BOT_NICK],
        (size_t)SettingManager->ui16TextsLens[SETTXT_BOT_NICK]);
	}
	lua_rawset(L, i);

	lua_pushliteral(L, "sDescription");
    if(SettingManager->sTexts[SETTXT_BOT_DESCRIPTION] == NULL) {
		lua_pushnil(L); 
	} else {
        lua_pushlstring(L, SettingManager->sTexts[SETTXT_BOT_DESCRIPTION], 
        (size_t)SettingManager->ui16TextsLens[SETTXT_BOT_DESCRIPTION]);
	}
	lua_rawset(L, i);

	lua_pushliteral(L, "sEmail");
    if(SettingManager->sTexts[SETTXT_BOT_EMAIL] == NULL) {
		lua_pushnil(L); 
	} else {
        lua_pushlstring(L, SettingManager->sTexts[SETTXT_BOT_EMAIL], 
        (size_t)SettingManager->ui16TextsLens[SETTXT_BOT_EMAIL]);
	}
	lua_rawset(L, i);

    lua_pushliteral(L, "bEnabled");
    SettingManager->bBools[SETBOOL_REG_BOT] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, i);

    lua_pushliteral(L, "bUsedAsHubSecAlias");
    SettingManager->bBools[SETBOOL_USE_BOT_NICK_AS_HUB_SEC] == true ? lua_pushboolean(L, 1) : lua_pushnil(L);
    lua_rawset(L, i);

    return 1;
}
//------------------------------------------------------------------------------

static int SetHubBot(lua_State * L) {
	if(lua_gettop(L) != 5) {
        luaL_error(L, "bad argument count to 'SetHubBot' (5 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

    if(lua_type(L, 1) != LUA_TBOOLEAN || lua_type(L, 2) != LUA_TSTRING || lua_type(L, 3) != LUA_TSTRING || 
        lua_type(L, 4) != LUA_TSTRING || lua_type(L, 5) != LUA_TBOOLEAN) {
        luaL_checktype(L, 1, LUA_TBOOLEAN);
        luaL_checktype(L, 2, LUA_TSTRING);
        luaL_checktype(L, 3, LUA_TSTRING);
        luaL_checktype(L, 4, LUA_TSTRING);
        luaL_checktype(L, 5, LUA_TBOOLEAN);
        lua_settop(L, 0);
        return 0;
    }

    char *NewBotName, *NewBotDescription, *NewBotEmail;
    size_t szNameLen, szDescrLen, szEmailLen;

    NewBotName = (char *)lua_tolstring(L, 2, &szNameLen);
    NewBotDescription = (char *)lua_tolstring(L, 3, &szDescrLen);
    NewBotEmail = (char *)lua_tolstring(L, 4, &szEmailLen);

    if(szNameLen == 0 || szNameLen > 64 || szDescrLen > 64 || szEmailLen > 64 ||
        strpbrk(NewBotName, " $|") != NULL || strpbrk(NewBotDescription, "$|") != NULL ||
        strpbrk(NewBotEmail, "$|") != NULL || hashManager->FindUser(NewBotName, szNameLen) != NULL) {
        lua_settop(L, 0);
        return 0;
    }

    bool bBotHaveNewNick = (strcmp(SettingManager->sTexts[SETTXT_BOT_NICK], NewBotName) != 0);

    if(SettingManager->bBools[SETBOOL_REG_BOT] == true) {
        SettingManager->DisableBot(bBotHaveNewNick);
    }

    SettingManager->bUpdateLocked = true;

    SettingManager->SetBool(SETBOOL_REG_BOT, (lua_toboolean(L, 1) == 0 ? false : true));

    if(bBotHaveNewNick == true) {
        SettingManager->SetText(SETTXT_BOT_NICK, NewBotName);
    }

    if(NewBotDescription != NULL && (SettingManager->sTexts[SETTXT_BOT_DESCRIPTION] == NULL ||
        strcmp(SettingManager->sTexts[SETTXT_BOT_DESCRIPTION], NewBotDescription) != 0))
        SettingManager->SetText(SETTXT_BOT_DESCRIPTION, NewBotDescription);

    if(NewBotEmail != NULL && (SettingManager->sTexts[SETTXT_BOT_EMAIL] == NULL ||
        strcmp(SettingManager->sTexts[SETTXT_BOT_EMAIL], NewBotEmail) != 0))
        SettingManager->SetText(SETTXT_BOT_EMAIL, NewBotEmail);

    SettingManager->SetBool(SETBOOL_USE_BOT_NICK_AS_HUB_SEC, (lua_toboolean(L, 5) == 0 ? false : true));

    SettingManager->bUpdateLocked = false;

    SettingManager->UpdateHubSec();
    SettingManager->UpdateMOTD();
    SettingManager->UpdateHubNameWelcome();
    SettingManager->UpdateRegOnlyMessage();
    SettingManager->UpdateShareLimitMessage();
    SettingManager->UpdateSlotsLimitMessage();
    SettingManager->UpdateHubSlotRatioMessage();
    SettingManager->UpdateMaxHubsLimitMessage();
    SettingManager->UpdateNoTagMessage();
    SettingManager->UpdateNickLimitMessage();
    SettingManager->UpdateBotsSameNick();

    if(SettingManager->bBools[SETBOOL_REG_BOT] == true) {
        SettingManager->UpdateBot(bBotHaveNewNick);
    }

    return 0;
}
//------------------------------------------------------------------------------

static const luaL_Reg setman[] = {
    { "Save", Save },
	{ "GetMOTD", GetMOTD }, 
	{ "SetMOTD", SetMOTD }, 
	{ "GetBool", GetBool }, 
	{ "SetBool", SetBool }, 
	{ "GetNumber", GetNumber }, 
	{ "SetNumber", SetNumber }, 
	{ "GetString", GetString }, 
	{ "SetString", SetString }, 
	{ "GetMinShare", GetMinShare }, 
	{ "SetMinShare", SetMinShare }, 
	{ "GetMaxShare", GetMaxShare }, 
	{ "SetMaxShare", SetMaxShare }, 
	{ "SetHubSlotRatio", SetHubSlotRatio }, 
	{ "GetOpChat", GetOpChat }, 
	{ "SetOpChat", SetOpChat }, 
	{ "GetHubBot", GetHubBot }, 
	{ "SetHubBot", SetHubBot }, 
	{ NULL, NULL }
};
//---------------------------------------------------------------------------

#if LUA_VERSION_NUM == 501
void RegSetMan(lua_State * L) {
    luaL_register(L, "SetMan", setman);
#else
int RegSetMan(lua_State * L) {
    luaL_newlib(L, setman);
    return 1;
#endif
}
//---------------------------------------------------------------------------
