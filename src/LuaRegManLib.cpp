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
#include "LuaRegManLib.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "globalQueue.h"
#include "hashRegManager.h"
#include "hashUsrManager.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "SettingManager.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
//---------------------------------------------------------------------------
	#ifndef _SERVICE
		#include "TRegsForm.h"
	#endif
//---------------------------------------------------------------------------
	#ifndef _MSC_VER
		#pragma package(smart_init)
	#endif
#endif
//---------------------------------------------------------------------------

static void PushReg(lua_State * L, RegUser * r) {
	lua_checkstack(L, 3); // we need 3 (1 table, 2 id, 3 value) empty slots in stack, check it to be sure

    lua_newtable(L);
    int i = lua_gettop(L);

    lua_pushliteral(L, "sNick");
    lua_pushstring(L, r->sNick);
    lua_rawset(L, i);

    lua_pushliteral(L, "sPassword");
    lua_pushstring(L, r->sPass);
    lua_rawset(L, i);
            
    lua_pushliteral(L, "iProfile");
    lua_pushnumber(L, r->iProfile);
    lua_rawset(L, i);
}
//------------------------------------------------------------------------------

static int Save(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'Save' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        return 0;
    }

	hashRegManager->Save();

    return 0;
}
//------------------------------------------------------------------------------

static int GetRegsByProfile(lua_State * L) {
	if(lua_gettop(L) != 1) {
        luaL_error(L, "bad argument count to 'GetRegsByProfile' (1 expected, got %d)", lua_gettop(L));
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

    lua_settop(L, 0);

    lua_newtable(L);
    int t = lua_gettop(L), i = 0;

	RegUser *next = hashRegManager->RegListS;
        
    while(next != NULL) {
        RegUser *cur = next;
        next = cur->next;
        
		if(cur->iProfile == (int32_t)iProfile) {
            lua_pushnumber(L, ++i);
            PushReg(L, cur);
            lua_rawset(L, t);
        }
    }
    
    return 1;
}
//------------------------------------------------------------------------------

static int GetRegsByOpStatus(lua_State * L, const bool &bOperator) {
	if(lua_gettop(L) != 0) {
        if(bOperator == true) {
            luaL_error(L, "bad argument count to 'GetOps' (0 expected, got %d)", lua_gettop(L));
        } else {
            luaL_error(L, "bad argument count to 'GetNonOps' (0 expected, got %d)", lua_gettop(L));
        }
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }
    
    lua_newtable(L);
    int t = lua_gettop(L), i = 0;

    RegUser *next = hashRegManager->RegListS;

    while(next != NULL) {
        RegUser *curReg = next;
		next = curReg->next;

        if(ProfileMan->IsProfileAllowed(curReg->iProfile, ProfileManager::HASKEYICON) == bOperator) {
            lua_pushnumber(L, ++i);
			PushReg(L, curReg);
            lua_rawset(L, t);
        }
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetNonOps(lua_State * L) {
    return GetRegsByOpStatus(L, false);
}
//------------------------------------------------------------------------------

static int GetOps(lua_State * L) {
    return GetRegsByOpStatus(L, true);
}
//------------------------------------------------------------------------------

static int GetReg(lua_State * L) {
	if(lua_gettop(L) != 1) {
        luaL_error(L, "bad argument count to 'GetReg' (1 expected, got %d)", lua_gettop(L));
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
    char * sNick = (char *)lua_tolstring(L, 1, &iLen);

    if(iLen == 0) {
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

    RegUser * r = hashRegManager->Find(sNick, iLen);

    lua_settop(L, 0);

    if(r == NULL) {
		lua_pushnil(L);
        return 1;
    }

    PushReg(L, r);

    return 1;
}
//------------------------------------------------------------------------------

static int GetRegs(lua_State * L) {
	if(lua_gettop(L) != 0) {
        luaL_error(L, "bad argument count to 'GetRegs' (0 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }
    
    lua_newtable(L);
    int t = lua_gettop(L), i = 0;

    RegUser *next = hashRegManager->RegListS;

    while(next != NULL) {
        RegUser *curReg = next;
		next = curReg->next;

        lua_pushnumber(L, ++i);

		PushReg(L, curReg); 
    
        lua_rawset(L, t);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int AddReg(lua_State * L) {
	if(lua_gettop(L) != 3) {
        luaL_error(L, "bad argument count to 'AddReg' (3 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    if(lua_type(L, 1) != LUA_TSTRING || lua_type(L, 2) != LUA_TSTRING || lua_type(L, 3) != LUA_TNUMBER) {
        luaL_checktype(L, 1, LUA_TSTRING);
        luaL_checktype(L, 2, LUA_TSTRING);
        luaL_checktype(L, 3, LUA_TNUMBER);
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

    size_t iNickLen, iPassLen;
    char *sNick = (char *)lua_tolstring(L, 1, &iNickLen);
    char *sPass = (char *)lua_tolstring(L, 2, &iPassLen);
	uint16_t iProfile = (uint16_t)lua_tonumber(L, 3);

    if(iProfile > ProfileMan->iProfileCount-1 || iNickLen == 0 || iNickLen > 64 || iPassLen == 0 || iPassLen > 64 || 
        strpbrk(sNick, " $|<>:?*\"/\\") != NULL || strchr(sPass, '|') != NULL) {
        return 0;
    }

	RegUser *reg = hashRegManager->Find(sNick, iNickLen);

    if(reg != NULL) {
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

    RegUser *newUser = new RegUser(sNick, sPass, iProfile);

    lua_settop(L, 0);

    if(newUser == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate newUser in AddRegUser!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
        AppendSpecialLog(sDbgstr);
		lua_pushnil(L);
        return 1;
    }

	hashRegManager->Add(newUser);

#ifdef _WIN32
	#ifndef _SERVICE
		if(RegsForm != NULL) {
	        TListItem *ListItem = RegsForm->RegList->Items->Add();
	        ListItem->Caption = newUser->sNick;
	        ListItem->SubItems->Add(newUser->sPass);
	        ListItem->SubItems->Add(ProfileMan->ProfilesTable[iProfile]->sName);
	        ListItem->Data = (void *)newUser;
		}
	#endif
#endif

	User *AddedUser = hashManager->FindUser(newUser->sNick, iNickLen);

    if(AddedUser != NULL) {
        bool bAllowedOpChat = ProfileMan->IsAllowed(AddedUser, ProfileManager::ALLOWEDOPCHAT);
        AddedUser->iProfile = iProfile;

        if(((AddedUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
            if(ProfileMan->IsAllowed(AddedUser, ProfileManager::HASKEYICON) == true) {
                AddedUser->ui32BoolBits |= User::BIT_OPERATOR;
            } else {
                AddedUser->ui32BoolBits &= ~User::BIT_OPERATOR;
            }

            if(((AddedUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
				colUsers->Add2OpList(AddedUser->Nick, AddedUser->NickLen);
                globalQ->OpListStore(AddedUser->Nick);

                if(bAllowedOpChat != ProfileMan->IsAllowed(AddedUser, ProfileManager::ALLOWEDOPCHAT)) {
					if(SettingManager->bBools[SETBOOL_REG_OP_CHAT] == true &&
                        (SettingManager->bBools[SETBOOL_REG_BOT] == false || SettingManager->bBotsSameNick == false)) {
                        if(((AddedUser->ui32BoolBits & User::BIT_SUPPORT_NOHELLO) == User::BIT_SUPPORT_NOHELLO) == false) {
                            UserSendCharDelayed(AddedUser, SettingManager->sPreTexts[SetMan::SETPRETXT_OP_CHAT_HELLO],
                                SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_OP_CHAT_HELLO]);
                        }

                        UserSendCharDelayed(AddedUser, SettingManager->sPreTexts[SetMan::SETPRETXT_OP_CHAT_MYINFO],
                            SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_OP_CHAT_MYINFO]);

						int imsgLen = sprintf(ScriptManager->lua_msg, "$OpList %s$$|", SettingManager->sTexts[SETTXT_OP_CHAT_NICK]);
                        if(CheckSprintf(imsgLen, 131072, "AddReg") == true) {
                            UserSendCharDelayed(AddedUser, ScriptManager->lua_msg, imsgLen);
                        }
                    }
                }
            }
        }
    }

	hashRegManager->Save();

    lua_pushboolean(L, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int DelReg(lua_State * L) {
	if(lua_gettop(L) != 1) {
        luaL_error(L, "bad argument count to 'DelReg' (1 expected, got %d)", lua_gettop(L));
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

    size_t iNickLen;
    char *sNick = (char *)lua_tolstring(L, 1, &iNickLen);

    if(iNickLen == 0) {
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

	RegUser *reg = hashRegManager->Find(sNick, iNickLen);

    lua_settop(L, 0);

    if(reg == NULL) {
        lua_pushnil(L);
        return 1;
    }

#ifdef _WIN32
	#ifndef _SERVICE
		if(RegsForm != NULL) {
	        TListItem *ListItem = RegsForm->RegList->FindCaption(0, reg->sNick, false, true, false);
	        if(ListItem != NULL) {
	            RegsForm->RegList->Items->Delete(ListItem->Index);
	        }
		}
	#endif
#endif

	hashRegManager->Rem(reg);

    User *RemovedUser = hashManager->FindUser(reg->sNick, iNickLen);

    if(RemovedUser != NULL) {
        RemovedUser->iProfile = -1;
        if(((RemovedUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
            colUsers->DelFromOpList(RemovedUser->Nick);
            RemovedUser->ui32BoolBits &= ~User::BIT_OPERATOR;
            if(SettingManager->bBools[SETBOOL_REG_OP_CHAT] == true &&
                (SettingManager->bBools[SETBOOL_REG_BOT] == false || SettingManager->bBotsSameNick == false)) {
                int imsgLen = sprintf(ScriptManager->lua_msg, "$Quit %s|", SettingManager->sTexts[SETTXT_OP_CHAT_NICK]);
                if(CheckSprintf(imsgLen, 131072, "DelReg") == true) {
                    UserSendCharDelayed(RemovedUser, ScriptManager->lua_msg, imsgLen);
                }
            }
        }
    }

    delete reg;

	hashRegManager->Save();

    lua_pushboolean(L, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int ChangeReg(lua_State * L) {
	if(lua_gettop(L) != 3) {
        luaL_error(L, "bad argument count to 'ChangeReg' (3 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    if(lua_type(L, 1) != LUA_TSTRING || lua_type(L, 2) != LUA_TSTRING || lua_type(L, 3) != LUA_TNUMBER) {
        luaL_checktype(L, 1, LUA_TSTRING);
        luaL_checktype(L, 2, LUA_TSTRING);
        luaL_checktype(L, 3, LUA_TNUMBER);
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

    size_t iNickLen, iPassLen;
    char *sNick = (char *)lua_tolstring(L, 1, &iNickLen);
    char *sPass = (char *)lua_tolstring(L, 2, &iPassLen);
	uint16_t iProfile = (uint16_t)lua_tonumber(L, 3);

	if(iProfile > ProfileMan->iProfileCount-1 || iNickLen == 0 || iNickLen > 64 || iPassLen == 0 || iPassLen > 64 ||
        strpbrk(sNick, " $|<>:?*\"/\\") != NULL || strpbrk(sPass, "$|") != NULL) {
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

	RegUser *reg = hashRegManager->Find(sNick, iNickLen);

    if(reg == NULL) {
		lua_settop(L, 0);
		lua_pushnil(L);
        return 1;
    }

#ifdef _WIN32
	#ifndef _SERVICE
		if(RegsForm != NULL) {
	        TListItem *ListItem = RegsForm->RegList->FindCaption(0, reg->sNick, false, true, false);
	        if(ListItem != NULL) {
	            ListItem->SubItems->Strings[0] = sPass;
	            ListItem->SubItems->Strings[1] = ProfileMan->ProfilesTable[iProfile]->sName;
	        }
		}
	#endif
#endif

    if(strcmp(reg->sPass, sPass) != 0) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)reg->sPass) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate reg->sPass in ChangeReg! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }

        reg->sPass = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iPassLen+1);
#else
		free(reg->sPass);

		reg->sPass = (char *) malloc(iPassLen+1);
#endif
        if(reg->sPass == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iPassLen+1))+
				" bytes of memory for sPass in ChangeReg!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
    		lua_settop(L, 0);
    		lua_pushnil(L);
            return 1;
        }   
        memcpy(reg->sPass, sPass, iPassLen);
        reg->sPass[iPassLen] = '\0';
    }

    reg->iProfile = iProfile;

    User *ChangedUser = hashManager->FindUser(sNick, iNickLen);
    if(ChangedUser != NULL && ChangedUser->iProfile != (int32_t)iProfile) {
        bool bAllowedOpChat = ProfileMan->IsAllowed(ChangedUser, ProfileManager::ALLOWEDOPCHAT);

        ChangedUser->iProfile = (int32_t)iProfile;

        if(((ChangedUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) != 
            ProfileMan->IsAllowed(ChangedUser, ProfileManager::HASKEYICON)) {
            if(ProfileMan->IsAllowed(ChangedUser, ProfileManager::HASKEYICON) == true) {
                ChangedUser->ui32BoolBits |= User::BIT_OPERATOR;
                colUsers->Add2OpList(ChangedUser->Nick, ChangedUser->NickLen);
                globalQ->OpListStore(ChangedUser->Nick);
            } else {
                ChangedUser->ui32BoolBits &= ~User::BIT_OPERATOR;
                colUsers->DelFromOpList(ChangedUser->Nick);
            }
        }

        if(bAllowedOpChat != ProfileMan->IsAllowed(ChangedUser, ProfileManager::ALLOWEDOPCHAT)) {
            if(ProfileMan->IsAllowed(ChangedUser, ProfileManager::ALLOWEDOPCHAT) == true) {
                if(SettingManager->bBools[SETBOOL_REG_OP_CHAT] == true &&
                    (SettingManager->bBools[SETBOOL_REG_BOT] == false || SettingManager->bBotsSameNick == false)) {
                    if(((ChangedUser->ui32BoolBits & User::BIT_SUPPORT_NOHELLO) == User::BIT_SUPPORT_NOHELLO) == false) {
                        UserSendCharDelayed(ChangedUser, SettingManager->sPreTexts[SetMan::SETPRETXT_OP_CHAT_HELLO],
                        SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_OP_CHAT_HELLO]);
                    }

                    UserSendCharDelayed(ChangedUser, SettingManager->sPreTexts[SetMan::SETPRETXT_OP_CHAT_MYINFO],
                        SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_OP_CHAT_MYINFO]);

                    int imsgLen = sprintf(ScriptManager->lua_msg, "$OpList %s$$|", SettingManager->sTexts[SETTXT_OP_CHAT_NICK]);
                    if(CheckSprintf(imsgLen, 131072, "ChangeReg1") == true) {
                        UserSendCharDelayed(ChangedUser, ScriptManager->lua_msg, imsgLen);
                    }
                }
            } else {
                if(SettingManager->bBools[SETBOOL_REG_OP_CHAT] == true &&
                    (SettingManager->bBools[SETBOOL_REG_BOT] == false || SettingManager->bBotsSameNick == false)) {
                    int imsgLen = sprintf(ScriptManager->lua_msg, "$Quit %s|", SettingManager->sTexts[SETTXT_OP_CHAT_NICK]);
                    if(CheckSprintf(imsgLen, 131072, "ChangeReg2") == true) {
                        UserSendCharDelayed(ChangedUser, ScriptManager->lua_msg, imsgLen);
                    }
                }
            }
        }
    }

    hashRegManager->Save();

    lua_settop(L, 0);

    lua_pushboolean(L, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int ClrRegBadPass(lua_State * L) {
	if(lua_gettop(L) != 1) {
        luaL_error(L, "bad argument count to 'ClrRegBadPass' (1 expected, got %d)", lua_gettop(L));
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
    
    size_t iNickLen;
    char* sNick = (char*)lua_tolstring(L, 1, &iNickLen);

    if(iNickLen != 0) {
        RegUser *Reg = hashRegManager->Find(sNick, iNickLen);
        if(Reg != NULL) {
            Reg->iBadPassCount = 0;
        } else {
			lua_settop(L, 0);
			lua_pushnil(L);
			return 1;
        }
    }

	lua_settop(L, 0);

    lua_pushboolean(L, 1);
    return 1;
}
//------------------------------------------------------------------------------

static const luaL_reg regman_funcs[] =  {
    { "Save", Save }, 
	{ "GetRegsByProfile", GetRegsByProfile }, 
	{ "GetNonOps", GetNonOps }, 
	{ "GetOps", GetOps }, 
	{ "GetReg", GetReg }, 
	{ "GetRegs", GetRegs }, 
	{ "AddReg", AddReg }, 
	{ "DelReg", DelReg }, 
	{ "ChangeReg", ChangeReg }, 
	{ "ClrRegBadPass", ClrRegBadPass }, 
	{ NULL, NULL }
};
//---------------------------------------------------------------------------

void RegRegMan(lua_State * L) {
    luaL_register(L, "RegMan", regman_funcs);
}
//---------------------------------------------------------------------------
