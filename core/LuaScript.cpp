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
#include "colUsers.h"
#include "eventqueue.h"
#include "GlobalDataQueue.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
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
#ifdef _BUILD_GUI
	#include "../gui.win/GuiUtil.h"
    #include "../gui.win/MainWindowPageScripts.h"
#endif
//---------------------------------------------------------------------------
char ScriptTimer::sDefaultTimerFunc[] = "OnTimer";
//---------------------------------------------------------------------------

static int ScriptPanic(lua_State * L) {
    size_t szLen = 0;
    char * stmp = (char*)lua_tolstring(L, -1, &szLen);

    string sMsg = "[LUA] At panic -> " + string(stmp, szLen);

    AppendLog(sMsg);

    return 0;
}
//------------------------------------------------------------------------------

ScriptBot::ScriptBot() : pPrev(NULL), pNext(NULL), sNick(NULL), sMyINFO(NULL), bIsOP(false) {
    clsScriptManager::mPtr->ui8BotsCount++;
}
//------------------------------------------------------------------------------

ScriptBot::~ScriptBot() {
#ifdef _WIN32
    if(sNick != NULL && HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sNick) == 0) {
		AppendDebugLog("%s - [MEM] Cannot deallocate sNick in ScriptBot::~ScriptBot\n");
    }
#else
	free(sNick);
#endif

#ifdef _WIN32
    if(sMyINFO != NULL && HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMyINFO) == 0) {
		AppendDebugLog("%s - [MEM] Cannot deallocate sMyINFO in ScriptBot::~ScriptBot\n");
    }
#else
	free(sMyINFO);
#endif

	clsScriptManager::mPtr->ui8BotsCount--;
}
//------------------------------------------------------------------------------

ScriptBot * ScriptBot::CreateScriptBot(char * sBotNick, const size_t &szNickLen, char * sDescription, const size_t &szDscrLen, char * sEmail, const size_t &szEmlLen, const bool &bOP) {
    ScriptBot * pScriptBot = new (std::nothrow) ScriptBot();

    if(pScriptBot == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate new pScriptBot in ScriptBot::CreateScriptBot\n");

        return NULL;
    }

#ifdef _WIN32
    pScriptBot->sNick = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szNickLen+1);
#else
	pScriptBot->sNick = (char *)malloc(szNickLen+1);
#endif
    if(pScriptBot->sNick == NULL) {
		AppendDebugLogFormat("[MEM] Cannot allocate %" PRIu64 " bytes for pScriptBot->sNick in ScriptBot::CreateScriptBot\n", (uint64_t)(szNickLen+1));

        delete pScriptBot;
		return NULL;
    }
    memcpy(pScriptBot->sNick, sBotNick, szNickLen);
    pScriptBot->sNick[szNickLen] = '\0';

    pScriptBot->bIsOP = bOP;

    size_t szWantLen = 24+szNickLen+szDscrLen+szEmlLen;

#ifdef _WIN32
    pScriptBot->sMyINFO = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szWantLen);
#else
	pScriptBot->sMyINFO = (char *)malloc(szWantLen);
#endif
    if(pScriptBot->sMyINFO == NULL) {
		AppendDebugLogFormat("[MEM] Cannot allocate %" PRIu64 " bytes for pScriptBot->sMyINFO in ScriptBot::CreateScriptBot\n", (uint64_t)szWantLen);

        delete pScriptBot;
		return NULL;
    }

	int iLen = sprintf(pScriptBot->sMyINFO, "$MyINFO $ALL %s %s$ $$%s$$|", sBotNick, sDescription != NULL ? sDescription : "", sEmail != NULL ? sEmail : "");

	CheckSprintf(iLen, szWantLen, "ScriptBot::CreateScriptBot");

    return pScriptBot;
}
//------------------------------------------------------------------------------

ScriptTimer::ScriptTimer() : 
#if defined(_WIN32) && !defined(_WIN_IOT)
	uiTimerId(NULL),
#else
	ui64Interval(0), ui64LastTick(0),
#endif
	pPrev(NULL), pNext(NULL), pLua(NULL), sFunctionName(NULL), iFunctionRef(0) {
	// ...
}
//------------------------------------------------------------------------------

ScriptTimer::~ScriptTimer() {
#ifdef _WIN32
	if(sFunctionName != NULL && sFunctionName != sDefaultTimerFunc) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sFunctionName) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sFunctionName in ScriptTimer::~ScriptTimer\n");
        }
#else
    if(sFunctionName != sDefaultTimerFunc) {
        free(sFunctionName);
#endif
    }
}
//------------------------------------------------------------------------------

#if defined(_WIN32) && !defined(_WIN_IOT)
ScriptTimer * ScriptTimer::CreateScriptTimer(UINT_PTR uiTmrId, char * sFunctName, const size_t &szLen, const int &iRef, lua_State * pLuaState) {
#else
ScriptTimer * ScriptTimer::CreateScriptTimer(char * sFunctName, const size_t &szLen, const int &iRef, lua_State * pLuaState) {
#endif
    ScriptTimer * pScriptTimer = new (std::nothrow) ScriptTimer();

    if(pScriptTimer == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate new pScriptTimer in ScriptTimer::CreateScriptTimer\n");

        return NULL;
    }

	pScriptTimer->pLua = pLuaState;

	if(sFunctName != NULL) {
        if(sFunctName != sDefaultTimerFunc) {
#ifdef _WIN32
            pScriptTimer->sFunctionName = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szLen+1);
#else
            pScriptTimer->sFunctionName = (char *)malloc(szLen+1);
#endif
            if(pScriptTimer->sFunctionName == NULL) {
                AppendDebugLogFormat("[MEM] Cannot allocate %" PRIu64 " bytes for pScriptTimer->sFunctionName in ScriptTimer::CreateScriptTimer\n", (uint64_t)(szLen+1));

                delete pScriptTimer;
                return NULL;
            }

            memcpy(pScriptTimer->sFunctionName, sFunctName, szLen);
            pScriptTimer->sFunctionName[szLen] = '\0';
        } else {
            pScriptTimer->sFunctionName = sDefaultTimerFunc;
        }
    } else {
        pScriptTimer->iFunctionRef = iRef;
    }

#if defined(_WIN32) && !defined(_WIN_IOT)
	pScriptTimer->uiTimerId = uiTmrId;
#endif

    return pScriptTimer;
}
//------------------------------------------------------------------------------

Script::Script() : pPrev(NULL), pNext(NULL), pBotList(NULL), pLUA(NULL), sName(NULL), ui32DataArrivals(4294967295U), ui16Functions(65535),
	bEnabled(false), bRegUDP(false), bProcessed(false) {
	// ...
}
//------------------------------------------------------------------------------

Script::~Script() {
    if(bRegUDP == true) {
		clsUdpDebug::mPtr->Remove(sName);
        bRegUDP = false;
    }

    if(pLUA != NULL) {
        lua_close(pLUA);
    }

#ifdef _WIN32
	if(sName != NULL && HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sName) == 0) {
        AppendDebugLog("%s - [MEM] Cannot deallocate sName in Script::~Script\n");
    }
#else
	free(sName);
#endif
}
//------------------------------------------------------------------------------

Script * Script::CreateScript(char * Name, const bool &enabled) {
    Script * pScript = new (std::nothrow) Script();

    if(pScript == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate new pScript in Script::CreateScript\n");

        return NULL;
    }

#ifdef _WIN32
	string ExtractedFilename = ExtractFileName(Name);
	size_t szNameLen = ExtractedFilename.size();

    pScript->sName = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szNameLen+1);
#else
    size_t szNameLen = strlen(Name);

    pScript->sName = (char *)malloc(szNameLen+1);
#endif
    if(pScript->sName == NULL) {
        AppendDebugLogFormat("[MEM] Cannot allocate %" PRIu64 " bytes in Script::CreateScript\n", (uint64_t)szNameLen+1);

        delete pScript;
        return NULL;
    }
#ifdef _WIN32
    memcpy(pScript->sName, ExtractedFilename.c_str(), ExtractedFilename.size());
#else
    memcpy(pScript->sName, Name, szNameLen);
#endif
    pScript->sName[szNameLen] = '\0';

    pScript->bEnabled = enabled;

    return pScript;
}
//------------------------------------------------------------------------------

static int OsExit(lua_State * /* L*/) {
	clsEventQueue::mPtr->AddNormal(clsEventQueue::EVENT_SHUTDOWN, NULL);

    return 0;
}
//------------------------------------------------------------------------------

static void AddSettingIds(lua_State * L) {
	int iTable = lua_gettop(L);

	lua_newtable(L);
	int iNewTable = lua_gettop(L);

	const uint8_t ui8Bools[] = { SETBOOL_ANTI_MOGLO, SETBOOL_AUTO_START, SETBOOL_REDIRECT_ALL, SETBOOL_REDIRECT_WHEN_HUB_FULL, SETBOOL_AUTO_REG, SETBOOL_REG_ONLY, 
		SETBOOL_REG_ONLY_REDIR, SETBOOL_SHARE_LIMIT_REDIR, SETBOOL_SLOTS_LIMIT_REDIR, SETBOOL_HUB_SLOT_RATIO_REDIR, SETBOOL_MAX_HUBS_LIMIT_REDIR, 
		SETBOOL_MODE_TO_MYINFO, SETBOOL_MODE_TO_DESCRIPTION, SETBOOL_STRIP_DESCRIPTION, SETBOOL_STRIP_TAG, SETBOOL_STRIP_CONNECTION, 
		SETBOOL_STRIP_EMAIL, SETBOOL_REG_BOT, SETBOOL_USE_BOT_NICK_AS_HUB_SEC, SETBOOL_REG_OP_CHAT, SETBOOL_TEMP_BAN_REDIR, SETBOOL_PERM_BAN_REDIR, 
		SETBOOL_ENABLE_SCRIPTING, SETBOOL_KEEP_SLOW_USERS, SETBOOL_CHECK_NEW_RELEASES, SETBOOL_ENABLE_TRAY_ICON, SETBOOL_START_MINIMIZED, 
		SETBOOL_FILTER_KICK_MESSAGES, SETBOOL_SEND_KICK_MESSAGES_TO_OPS, SETBOOL_SEND_STATUS_MESSAGES, SETBOOL_SEND_STATUS_MESSAGES_AS_PM, 
		SETBOOL_ENABLE_TEXT_FILES, SETBOOL_SEND_TEXT_FILES_AS_PM, SETBOOL_STOP_SCRIPT_ON_ERROR, SETBOOL_MOTD_AS_PM, SETBOOL_DEFLOOD_REPORT, 
		SETBOOL_REPLY_TO_HUB_COMMANDS_AS_PM, SETBOOL_DISABLE_MOTD, SETBOOL_DONT_ALLOW_PINGERS, SETBOOL_REPORT_PINGERS, SETBOOL_REPORT_3X_BAD_PASS, 
		SETBOOL_ADVANCED_PASS_PROTECTION, SETBOOL_BIND_ONLY_SINGLE_IP, SETBOOL_RESOLVE_TO_IP, SETBOOL_NICK_LIMIT_REDIR, SETBOOL_BAN_MSG_SHOW_IP, 
		SETBOOL_BAN_MSG_SHOW_RANGE, SETBOOL_BAN_MSG_SHOW_NICK, SETBOOL_BAN_MSG_SHOW_REASON, SETBOOL_BAN_MSG_SHOW_BY, SETBOOL_REPORT_SUSPICIOUS_TAG, 
		SETBOOL_LOG_SCRIPT_ERRORS, SETBOOL_NO_QUACK_SUPPORTS, SETBOOL_HASH_PASSWORDS,
#if defined(_WITH_SQLITE) || defined(_WITH_POSTGRES) || defined(_WITH_MYSQL)
		SETBOOL_ENABLE_DATABASE,
#endif
	};

	const char * pBoolsNames[] = { "AntiMoGlo", "AutoStart", "RedirectAll", "RedirectWhenHubFull", "AutoReg", "RegOnly",
		"RegOnlyRedir", "ShareLimitRedir", "SlotLimitRedir", "HubSlotRatioRedir", "MaxHubsLimitRedir", 
		"ModeToMyInfo", "ModeToDescription", "StripDescription", "StripTag", "StripConnection", 
		"StripEmail", "RegBot", "UseBotAsHubSec", "RegOpChat", "TempBanRedir", "PermBanRedir", 
		"EnableScripting", "KeepSlowUsers", "CheckNewReleases", "EnableTrayIcon", "StartMinimized",
		"FilterKickMessages", "SendKickMessagesToOps", "SendStatusMessages", "SendStatusMessagesAsPm",
		"EnableTextFiles", "SendTextFilesAsPm", "StopScriptOnError", "SendMotdAsPm", "DefloodReport",
		"ReplyToHubCommandsAsPm", "DisableMotd", "DontAllowPingers", "ReportPingers", "Report3xBadPass",
		"AdvancedPassProtection", "ListenOnlySingleIp", "ResolveToIp", "NickLimitRedir", "BanMsgShowIp",
		"BanMsgShowRange", "BanMsgShowNick", "BanMsgShowReason", "BanMsgShowBy", "ReportSuspiciousTag",
		"LogScriptErrors", "DisallowBadSupports", "HashPasswords", 
#if defined(_WITH_SQLITE) || defined(_WITH_POSTGRES) || defined(_WITH_MYSQL)
		"EnableDatabase",
#endif
	};

	for(uint8_t ui8i = 0; ui8i < sizeof(ui8Bools); ui8i++) {
		lua_pushinteger(L, ui8Bools[ui8i]);
		lua_setfield(L, iNewTable, pBoolsNames[ui8i]);
	}

	lua_setfield(L, iTable, "tBooleans");

	lua_newtable(L);
	iNewTable = lua_gettop(L);

	const uint8_t ui8Numbers[] = { SETSHORT_MAX_USERS, SETSHORT_MIN_SHARE_LIMIT, SETSHORT_MIN_SHARE_UNITS, SETSHORT_MAX_SHARE_LIMIT, SETSHORT_MAX_SHARE_UNITS, 
		SETSHORT_MIN_SLOTS_LIMIT, SETSHORT_MAX_SLOTS_LIMIT, SETSHORT_HUB_SLOT_RATIO_HUBS, SETSHORT_HUB_SLOT_RATIO_SLOTS, SETSHORT_MAX_HUBS_LIMIT, 
		SETSHORT_NO_TAG_OPTION, SETSHORT_FULL_MYINFO_OPTION, SETSHORT_MAX_CHAT_LEN, SETSHORT_MAX_CHAT_LINES, SETSHORT_MAX_PM_LEN, 
		SETSHORT_MAX_PM_LINES, SETSHORT_DEFAULT_TEMP_BAN_TIME, SETSHORT_MAX_PASIVE_SR, SETSHORT_MYINFO_DELAY, SETSHORT_MAIN_CHAT_MESSAGES, 
		SETSHORT_MAIN_CHAT_TIME, SETSHORT_MAIN_CHAT_ACTION, SETSHORT_SAME_MAIN_CHAT_MESSAGES, SETSHORT_SAME_MAIN_CHAT_TIME, SETSHORT_SAME_MAIN_CHAT_ACTION, 
		SETSHORT_SAME_MULTI_MAIN_CHAT_MESSAGES, SETSHORT_SAME_MULTI_MAIN_CHAT_LINES, SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION, SETSHORT_PM_MESSAGES, SETSHORT_PM_TIME, 
		SETSHORT_PM_ACTION, SETSHORT_SAME_PM_MESSAGES, SETSHORT_SAME_PM_TIME, SETSHORT_SAME_PM_ACTION, SETSHORT_SAME_MULTI_PM_MESSAGES, 
		SETSHORT_SAME_MULTI_PM_LINES, SETSHORT_SAME_MULTI_PM_ACTION, SETSHORT_SEARCH_MESSAGES, SETSHORT_SEARCH_TIME, SETSHORT_SEARCH_ACTION, 
		SETSHORT_SAME_SEARCH_MESSAGES, SETSHORT_SAME_SEARCH_TIME, SETSHORT_SAME_SEARCH_ACTION, SETSHORT_MYINFO_MESSAGES, SETSHORT_MYINFO_TIME, 
		SETSHORT_MYINFO_ACTION, SETSHORT_GETNICKLIST_MESSAGES, SETSHORT_GETNICKLIST_TIME, SETSHORT_GETNICKLIST_ACTION, SETSHORT_NEW_CONNECTIONS_COUNT, 
		SETSHORT_NEW_CONNECTIONS_TIME, SETSHORT_DEFLOOD_WARNING_COUNT, SETSHORT_DEFLOOD_WARNING_ACTION, SETSHORT_DEFLOOD_TEMP_BAN_TIME, SETSHORT_GLOBAL_MAIN_CHAT_MESSAGES, 
		SETSHORT_GLOBAL_MAIN_CHAT_TIME, SETSHORT_GLOBAL_MAIN_CHAT_TIMEOUT, SETSHORT_GLOBAL_MAIN_CHAT_ACTION, SETSHORT_MIN_SEARCH_LEN, SETSHORT_MAX_SEARCH_LEN, 
		SETSHORT_MIN_NICK_LEN, SETSHORT_MAX_NICK_LEN, SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE, SETSHORT_BRUTE_FORCE_PASS_PROTECT_TEMP_BAN_TIME, SETSHORT_MAX_PM_COUNT_TO_USER,
		SETSHORT_MAX_SIMULTANEOUS_LOGINS, SETSHORT_MAIN_CHAT_MESSAGES2, SETSHORT_MAIN_CHAT_TIME2, SETSHORT_MAIN_CHAT_ACTION2, SETSHORT_PM_MESSAGES2, 
		SETSHORT_PM_TIME2, SETSHORT_PM_ACTION2, SETSHORT_SEARCH_MESSAGES2, SETSHORT_SEARCH_TIME2, SETSHORT_SEARCH_ACTION2, 
		SETSHORT_MYINFO_MESSAGES2, SETSHORT_MYINFO_TIME2, SETSHORT_MYINFO_ACTION2, SETSHORT_MAX_MYINFO_LEN, SETSHORT_CTM_MESSAGES, 
		SETSHORT_CTM_TIME, SETSHORT_CTM_ACTION, SETSHORT_CTM_MESSAGES2, SETSHORT_CTM_TIME2, SETSHORT_CTM_ACTION2, 
		SETSHORT_RCTM_MESSAGES, SETSHORT_RCTM_TIME, SETSHORT_RCTM_ACTION, SETSHORT_RCTM_MESSAGES2, SETSHORT_RCTM_TIME2, 
		SETSHORT_RCTM_ACTION2, SETSHORT_MAX_CTM_LEN, SETSHORT_MAX_RCTM_LEN, SETSHORT_SR_MESSAGES, SETSHORT_SR_TIME, 
		SETSHORT_SR_ACTION, SETSHORT_SR_MESSAGES2, SETSHORT_SR_TIME2, SETSHORT_SR_ACTION2, SETSHORT_MAX_SR_LEN, 
		SETSHORT_MAX_DOWN_ACTION, SETSHORT_MAX_DOWN_KB, SETSHORT_MAX_DOWN_TIME, SETSHORT_MAX_DOWN_ACTION2, SETSHORT_MAX_DOWN_KB2, 
		SETSHORT_MAX_DOWN_TIME2, SETSHORT_CHAT_INTERVAL_MESSAGES, SETSHORT_CHAT_INTERVAL_TIME, SETSHORT_PM_INTERVAL_MESSAGES, SETSHORT_PM_INTERVAL_TIME, 
		SETSHORT_SEARCH_INTERVAL_MESSAGES, SETSHORT_SEARCH_INTERVAL_TIME, SETSHORT_MAX_CONN_SAME_IP, SETSHORT_MIN_RECONN_TIME,
#if defined(_WITH_SQLITE) || defined(_WITH_POSTGRES) || defined(_WITH_MYSQL)
		SETSHORT_DB_REMOVE_OLD_RECORDS,
#endif
	};

	const char * pNumbersNames[] = { "MaxUsers", "MinShareLimit", "MinShareUnits", "MaxShareLimit", "MaxShareUnits",
		"MinSlotsLimit", "MaxSlotsLimt", "HubSlotRatioHubs", "HubSlotRatioSlots", "MaxHubsLimit",
		"NoTagOption", "LongMyinfoOption", "MaxChatLen", "MaxChatLines", "MaxPmLen",
		"MaxPmLines", "DefaultTempBanTime", "MaxPasiveSr", "MyInfoDelay", "MainChatMessages",
		"MainChatTime", "MainChatAction", "SameMainChatMessages", "SameMainChatTime", "SameMainChatAction",
		"SameMultiMainChatMessages", "SameMultiMainChatLines", "SameMultiMainChatAction", "PmMessages", "PmTime",
		"PmAction", "SamePmMessages", "SamePmTime", "SamePmAction", "SameMultiPmMessages",
		"SameMultiPmLines", "SameMultiPmAction", "SearchMessages", "SearchTime", "SearchAction",
		"SameSearchMessages", "SameSearchTime", "SameSearchAction", "MyinfoMessages", "MyinfoTime",
		"MyinfoAction", "GetnicklistMessages", "GetnicklistTime", "GetnicklistAction", "NewConnectionsCount",
		"NewConnectionsTime", "DefloodWarningCount", "DefloodWarningAction", "DefloodTempBanTime", "GlobalMainChatMessages",
		"GlobalMainChatTime", "GlobalMainChatTimeout", "GlobalMainChatAction", "MinSearchLen", "MaxSearchLen",
		"MinNickLen", "MaxNickLen", "BruteForcePassProtectBanType", "BruteForcePassProtectTempBanTime", "MaxPmCountToUser",
		"MaxSimultaneousLogins", "MainChatMessages2", "MainChatTime2", "MainChatAction2", "PmMessages2", 
		"PmTime2", "PmAction2", "SearchMessages2", "SearchTime2", "SearchAction2",
		"MyinfoMessages2", "MyinfoTime2", "MyinfoAction2", "MaxMyinfoLen", "CtmMessages",
		"CtmTime", "CtmAction", "CtmMessages2", "CtmTime2", "CtmAction2", 
		"RctmMessages", "RctmTime", "RctmAction", "RctmMessages2", "RctmTime2",
		"RctmAction2", "MaxCtmLen", "MaxRctmLen", "SrMessages", "SrTime",
		"SrAction", "SrMessages2", "SrTime2", "SrAction2", "MaxSrLen",
		"MaxDownAction", "MaxDownKB", "MaxDownTime", "MaxDownAction2", "MaxDownKB2",
		"MaxDownTime2", "ChatIntervalMessages", "ChatIntervalTime", "PmIntervalMessages", "PmIntervalTime",
		"SearchIntervalMessages", "SearchIntervalTime", "MaxConnsSameIp", "MinReconnTime",
#if defined(_WITH_SQLITE) || defined(_WITH_POSTGRES) || defined(_WITH_MYSQL)
		"DbRemoveOldRecords",
#endif
	};

	for(uint8_t ui8i = 0; ui8i < sizeof(ui8Numbers); ui8i++) {
		lua_pushinteger(L, ui8Numbers[ui8i]);
		lua_setfield(L, iNewTable, pNumbersNames[ui8i]);
	}

	lua_setfield(L, iTable, "tNumbers");

	lua_newtable(L);
	iNewTable = lua_gettop(L);

	const uint8_t ui8Strings[] = { SETTXT_HUB_NAME, SETTXT_ADMIN_NICK, SETTXT_HUB_ADDRESS, SETTXT_TCP_PORTS, SETTXT_UDP_PORT, SETTXT_HUB_DESCRIPTION, SETTXT_REDIRECT_ADDRESS, 
		SETTXT_REGISTER_SERVERS, SETTXT_REG_ONLY_MSG, SETTXT_REG_ONLY_REDIR_ADDRESS, SETTXT_HUB_TOPIC, SETTXT_SHARE_LIMIT_MSG, SETTXT_SHARE_LIMIT_REDIR_ADDRESS, 
		SETTXT_SLOTS_LIMIT_MSG, SETTXT_SLOTS_LIMIT_REDIR_ADDRESS, SETTXT_HUB_SLOT_RATIO_MSG, SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS, SETTXT_MAX_HUBS_LIMIT_MSG, 
		SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS, SETTXT_NO_TAG_MSG, SETTXT_NO_TAG_REDIR_ADDRESS, SETTXT_BOT_NICK, SETTXT_BOT_DESCRIPTION, SETTXT_BOT_EMAIL, 
		SETTXT_OP_CHAT_NICK, SETTXT_OP_CHAT_DESCRIPTION, SETTXT_OP_CHAT_EMAIL, SETTXT_TEMP_BAN_REDIR_ADDRESS, SETTXT_PERM_BAN_REDIR_ADDRESS, 
		SETTXT_CHAT_COMMANDS_PREFIXES, SETTXT_HUB_OWNER_EMAIL, SETTXT_NICK_LIMIT_MSG, SETTXT_NICK_LIMIT_REDIR_ADDRESS, SETTXT_MSG_TO_ADD_TO_BAN_MSG, 
		SETTXT_LANGUAGE, SETTXT_IPV4_ADDRESS, SETTXT_IPV6_ADDRESS, SETTXT_ENCODING, 
#ifdef _WITH_POSTGRES
		SETTXT_POSTGRES_HOST, SETTXT_POSTGRES_PORT, SETTXT_POSTGRES_DBNAME, SETTXT_POSTGRES_USER, SETTXT_POSTGRES_PASS,
#elif _WITH_MYSQL
		SETTXT_MYSQL_HOST, SETTXT_MYSQL_PORT, SETTXT_MYSQL_DBNAME, SETTXT_MYSQL_USER, SETTXT_MYSQL_PASS,
#endif
	};

	const char * pStringsNames[] = { "HubName", "AdminNick", "HubAddress", "TCPPorts", "UDPPort", "HubDescription", "MainRedirectAddress",
		"HublistRegisterAddresses", "RegOnlyMessage", "RegOnlyRedirAddress", "HubTopic", "ShareLimitMessage", "ShareLimitRedirAddress", 
		"SlotLimitMessage", "SlotLimitRedirAddress", "HubSlotRatioMessage", "HubSlotRatioRedirAddress", "MaxHubsLimitMessage", 
		"MaxHubsLimitRedirAddress", "NoTagMessage", "NoTagRedirAddress", "HubBotNick", "HubBotDescription", "HubBotEmail", 
		"OpChatNick", "OpChatDescription", "OpChatEmail", "TempBanRedirAddress", "PermBanRedirAddress", 
		"ChatCommandsPrefixes", "HubOwnerEmail", "NickLimitMessage", "NickLimitRedirAddress", "MessageToAddToBanMessage",
		"Language", "IPv4Address", "IPv6Address", "Encoding", 
#ifdef _WITH_POSTGRES
		"PostgresHost", "PostgresPort", "PostgresDBName", "PostgresUser", "PostgresPass",
#elif _WITH_MYSQL
		"MySQLHost", "MySQLPort", "MySQLDBName", "MySQLUser", "MySQLPass",
#endif
	};

	for(uint8_t ui8i = 0; ui8i < sizeof(ui8Strings); ui8i++) {
		lua_pushinteger(L, ui8Strings[ui8i]);
		lua_setfield(L, iNewTable, pStringsNames[ui8i]);
	}

	lua_setfield(L, iTable, "tStrings");

    lua_pop(L, 1);
}
//------------------------------------------------------------------------------

static void AddPermissionsIds(lua_State * L) {
	int iTable = lua_gettop(L);

	lua_newtable(L);
	int iNewTable = lua_gettop(L);

	const uint8_t ui8Permissions[] = { clsProfileManager::HASKEYICON, clsProfileManager::NODEFLOODGETNICKLIST, clsProfileManager::NODEFLOODMYINFO, clsProfileManager::NODEFLOODSEARCH, clsProfileManager::NODEFLOODPM,
		clsProfileManager::NODEFLOODMAINCHAT, clsProfileManager::MASSMSG, clsProfileManager::TOPIC, clsProfileManager::TEMP_BAN, clsProfileManager::REFRESHTXT,
		clsProfileManager::NOTAGCHECK, clsProfileManager::TEMP_UNBAN, clsProfileManager::DELREGUSER, clsProfileManager::ADDREGUSER, clsProfileManager::NOCHATLIMITS,
		clsProfileManager::NOMAXHUBCHECK, clsProfileManager::NOSLOTHUBRATIO, clsProfileManager::NOSLOTCHECK, clsProfileManager::NOSHARELIMIT, clsProfileManager::CLRPERMBAN,
		clsProfileManager::CLRTEMPBAN, clsProfileManager::GETINFO, clsProfileManager::GETBANLIST, clsProfileManager::RSTSCRIPTS, clsProfileManager::RSTHUB,
		clsProfileManager::TEMPOP, clsProfileManager::GAG, clsProfileManager::REDIRECT, clsProfileManager::BAN, clsProfileManager::KICK, clsProfileManager::DROP,
		clsProfileManager::ENTERFULLHUB, clsProfileManager::ENTERIFIPBAN, clsProfileManager::ALLOWEDOPCHAT, clsProfileManager::SENDALLUSERIP, clsProfileManager::RANGE_BAN,
		clsProfileManager::RANGE_UNBAN, clsProfileManager::RANGE_TBAN, clsProfileManager::RANGE_TUNBAN, clsProfileManager::GET_RANGE_BANS, clsProfileManager::CLR_RANGE_BANS,
		clsProfileManager::CLR_RANGE_TBANS, clsProfileManager::UNBAN, clsProfileManager::NOSEARCHLIMITS, clsProfileManager:: SENDFULLMYINFOS, clsProfileManager::NOIPCHECK,
		clsProfileManager::CLOSE, clsProfileManager::NODEFLOODCTM, clsProfileManager::NODEFLOODRCTM, clsProfileManager::NODEFLOODSR, clsProfileManager::NODEFLOODRECV,
		clsProfileManager::NOCHATINTERVAL, clsProfileManager::NOPMINTERVAL, clsProfileManager::NOSEARCHINTERVAL, clsProfileManager::NOUSRSAMEIP, clsProfileManager::NORECONNTIME
	};

	const char * pPermissionsNames[] = { "IsOperator", "NoDefloodGetnicklist", "NoDefloodMyinfo", "NoDefloodSearch", "NoDefloodPm",
		"NoDefloodMainChat", "MassMsg", "Topic", "TempBan", "ReloadTxtFiles",
		"NoTagCheck", "TempUnban", "DelRegUser", "AddRegUser", "NoChatLimits",
		"NoMaxHubsCheck", "NoSlotHubRatioCheck", "NoSlotCheck", "NoShareLimit", "ClrPermBan",
		"ClrTempBan", "GetInfo", "GetBans", "ScriptControl", "RstHub",
		"TempOp", "GagUngag", "Redirect", "Ban", "Kick", "Drop",
		"EnterIfHubFull", "EnterIfIpBan", "AllowedOpChat", "SendAllUsersIp", "RangeBan",
		"RangeUnban", "RangeTempBan", "RangeTempUnban", "GetRangeBans", "ClrRangePermBans",
		"ClrRangeTempBans", "Unban", "NoSearchLimits", "SendLongMyinfos", "NoIpCheck",
		"Close", "NoDefloodCtm", "NoDefloodRctm", "NoDefloodSr", "NoDefloodRecv",
		"NoChatInterval", "NoPmInterval", "NoSearchInterval", "NoMaxUsrSameIp", "NoReconnTime"
	};

	for(uint8_t ui8i = 0; ui8i < sizeof(ui8Permissions); ui8i++) {
		lua_pushinteger(L, ui8Permissions[ui8i]);
		lua_setfield(L, iNewTable, pPermissionsNames[ui8i]);
	}

	lua_setfield(L, iTable, "tPermissions");

    lua_pop(L, 1);
}
//------------------------------------------------------------------------------

bool ScriptStart(Script * cur) {
	cur->ui16Functions = 65535;
	cur->ui32DataArrivals = 4294967295U;

	cur->pPrev = NULL;
	cur->pNext = NULL;

#ifdef _WIN32
	cur->pLUA = lua_newstate(LuaAlocator, NULL);
#else
    cur->pLUA = luaL_newstate();
#endif

    if(cur->pLUA == NULL) {
        return false;
    }

	luaL_openlibs(cur->pLUA);

	lua_atpanic(cur->pLUA, ScriptPanic);

    // replace internal lua os.exit with correct shutdown
    lua_getglobal(cur->pLUA, "os");

    if(lua_istable(cur->pLUA, -1)) {
        lua_pushcfunction(cur->pLUA, OsExit);
        lua_setfield(cur->pLUA, -2, "exit");

        lua_pop(cur->pLUA, 1);
    }

#if LUA_VERSION_NUM > 501
    luaL_requiref(cur->pLUA, "Core", RegCore, 1);
	lua_pop(cur->pLUA, 1);

    luaL_requiref(cur->pLUA, "SetMan", RegSetMan, 1);
	AddSettingIds(cur->pLUA);

    luaL_requiref(cur->pLUA, "RegMan", RegRegMan, 1);
    lua_pop(cur->pLUA, 1);

    luaL_requiref(cur->pLUA, "BanMan", RegBanMan, 1);
    lua_pop(cur->pLUA, 1);

    luaL_requiref(cur->pLUA, "ProfMan", RegProfMan, 1);
	AddPermissionsIds(cur->pLUA);

    luaL_requiref(cur->pLUA, "TmrMan", RegTmrMan, 1);
    lua_pop(cur->pLUA, 1);

    luaL_requiref(cur->pLUA, "UDPDbg", RegUDPDbg, 1);
    lua_pop(cur->pLUA, 1);

    luaL_requiref(cur->pLUA, "ScriptMan", RegScriptMan, 1);
    lua_pop(cur->pLUA, 1);

    luaL_requiref(cur->pLUA, "IP2Country", RegIP2Country, 1);
    lua_pop(cur->pLUA, 1);
#else
	RegCore(cur->pLUA);

	RegSetMan(cur->pLUA);

    lua_getglobal(cur->pLUA, "SetMan");

    if(lua_istable(cur->pLUA, -1)) {
        AddSettingIds(cur->pLUA);
    }

	RegRegMan(cur->pLUA);
	RegBanMan(cur->pLUA);

	RegProfMan(cur->pLUA);

    lua_getglobal(cur->pLUA, "ProfMan");

    if(lua_istable(cur->pLUA, -1)) {
        AddPermissionsIds(cur->pLUA);
    }

	RegTmrMan(cur->pLUA);
	RegUDPDbg(cur->pLUA);
	RegScriptMan(cur->pLUA);
	RegIP2Country(cur->pLUA);
#endif

	if(luaL_dofile(cur->pLUA, (clsServerManager::sScriptPath+cur->sName).c_str()) == 0) {
#ifdef _BUILD_GUI
        RichEditAppendText(clsMainWindowPageScripts::mPtr->hWndPageItems[clsMainWindowPageScripts::REDT_SCRIPTS_ERRORS],
            (string(clsLanguageManager::mPtr->sTexts[LAN_NO_SYNERR_IN_SCRIPT_FILE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_NO_SYNERR_IN_SCRIPT_FILE]) + " " + string(cur->sName)).c_str());
#endif

        return true;
	} else {
        size_t szLen = 0;
        char * stmp = (char*)lua_tolstring(cur->pLUA, -1, &szLen);

        string sMsg(stmp, szLen);

#ifdef _BUILD_GUI
        RichEditAppendText(clsMainWindowPageScripts::mPtr->hWndPageItems[clsMainWindowPageScripts::REDT_SCRIPTS_ERRORS],
            (string(clsLanguageManager::mPtr->sTexts[LAN_SYNTAX], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_SYNTAX]) + " " + sMsg).c_str());
#endif

		clsUdpDebug::mPtr->BroadcastFormat("[LUA] %s", sMsg.c_str());

        if(clsSettingManager::mPtr->bBools[SETBOOL_LOG_SCRIPT_ERRORS] == true) {
            AppendLog(sMsg, true);
        }

		lua_close(cur->pLUA);
		cur->pLUA = NULL;
    
        return false;
    }
}
//------------------------------------------------------------------------------

void ScriptStop(Script * cur) {
	if(cur->bRegUDP == true) {
        clsUdpDebug::mPtr->Remove(cur->sName);
        cur->bRegUDP = false;
    }

    ScriptTimer * pCurTmr = NULL,
        * pNextTmr = clsScriptManager::mPtr->pTimerListS;
    
    while(pNextTmr != NULL) {
        pCurTmr = pNextTmr;
        pNextTmr = pCurTmr->pNext;

		if(cur->pLUA == pCurTmr->pLua) {
#if defined(_WIN32) && !defined(_WIN_IOT)
	        if(pCurTmr->uiTimerId != 0) {
	            KillTimer(NULL, pCurTmr->uiTimerId);
	        }
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

			delete pCurTmr;
		}
    }

    if(cur->pLUA != NULL) {
        lua_close(cur->pLUA);
        cur->pLUA = NULL;
    }

    ScriptBot * bot = NULL,
        * next = cur->pBotList;
    
    while(next != NULL) {
        bot = next;
        next = bot->pNext;

        clsReservedNicksManager::mPtr->DelReservedNick(bot->sNick, true);

        if(clsServerManager::bServerRunning == true) {
   			clsUsers::mPtr->DelFromNickList(bot->sNick, bot->bIsOP);

            clsUsers::mPtr->DelBotFromMyInfos(bot->sMyINFO);

			int iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "$Quit %s|", bot->sNick);
           	if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "ScriptStop") == true) {
                clsGlobalDataQueue::mPtr->AddQueueItem(clsServerManager::pGlobalBuffer, iMsgLen, NULL, 0, clsGlobalDataQueue::CMD_QUIT);
            }
		}

		delete bot;
    }

    cur->pBotList = NULL;
}
//------------------------------------------------------------------------------

int ScriptGetGC(Script * cur) {
	return lua_gc(cur->pLUA, LUA_GCCOUNT, 0);
}
//------------------------------------------------------------------------------

void ScriptOnStartup(Script * cur) {
    lua_pushcfunction(cur->pLUA, ScriptTraceback);
    int iTraceback = lua_gettop(cur->pLUA);

    lua_getglobal(cur->pLUA, "OnStartup");
    int i = lua_gettop(cur->pLUA);

    if(lua_isfunction(cur->pLUA, i) == 0) {
		cur->ui16Functions &= ~Script::ONSTARTUP;
		lua_settop(cur->pLUA, 0);
        return;
    }

    if(lua_pcall(cur->pLUA, 0, 0, iTraceback) != 0) {
        ScriptError(cur);

        lua_settop(cur->pLUA, 0);
        return;
    }

    // clear the stack for sure
    lua_settop(cur->pLUA, 0);
}
//------------------------------------------------------------------------------

void ScriptOnExit(Script * cur) {
    lua_pushcfunction(cur->pLUA, ScriptTraceback);
    int iTraceback = lua_gettop(cur->pLUA);

    lua_getglobal(cur->pLUA, "OnExit");
    int i = lua_gettop(cur->pLUA);
    if(lua_isfunction(cur->pLUA, i) == 0) {
		cur->ui16Functions &= ~Script::ONEXIT;
		lua_settop(cur->pLUA, 0);
        return;
    }

    if(lua_pcall(cur->pLUA, 0, 0, iTraceback) != 0) {
        ScriptError(cur);

        lua_settop(cur->pLUA, 0);
        return;
	}

	// clear the stack for sure
    lua_settop(cur->pLUA, 0);
}
//------------------------------------------------------------------------------

static bool ScriptOnError(Script * cur, char * sErrorMsg, const size_t &szMsgLen) {
    lua_pushcfunction(cur->pLUA, ScriptTraceback);
    int iTraceback = lua_gettop(cur->pLUA);

	lua_getglobal(cur->pLUA, "OnError");
    int i = lua_gettop(cur->pLUA);
    if(lua_isfunction(cur->pLUA, i) == 0) {
		cur->ui16Functions &= ~Script::ONERROR;
		lua_settop(cur->pLUA, 0);
        return true;
    }

	clsScriptManager::mPtr->pActualUser = NULL;
    
    lua_pushlstring(cur->pLUA, sErrorMsg, szMsgLen);

	if(lua_pcall(cur->pLUA, 1, 0, iTraceback) != 0) { // 1 passed parameters, zero returned
		size_t szLen = 0;
		char * stmp = (char*)lua_tolstring(cur->pLUA, -1, &szLen);

		string sMsg(stmp, szLen);

#ifdef _BUILD_GUI
        RichEditAppendText(clsMainWindowPageScripts::mPtr->hWndPageItems[clsMainWindowPageScripts::REDT_SCRIPTS_ERRORS],
            (string(clsLanguageManager::mPtr->sTexts[LAN_SYNTAX], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_SYNTAX]) + " " + sMsg).c_str());
        RichEditAppendText(clsMainWindowPageScripts::mPtr->hWndPageItems[clsMainWindowPageScripts::REDT_SCRIPTS_ERRORS],
            (string(clsLanguageManager::mPtr->sTexts[LAN_FATAL_ERR_SCRIPT], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_FATAL_ERR_SCRIPT]) + " " + string(cur->sName) + " ! " +
			string(clsLanguageManager::mPtr->sTexts[LAN_SCRIPT_STOPPED], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_SCRIPT_STOPPED]) + "!").c_str());
#endif

		if(clsSettingManager::mPtr->bBools[SETBOOL_LOG_SCRIPT_ERRORS] == true) {
			AppendLog(sMsg, true);
		}

        lua_settop(cur->pLUA, 0);
        return false;
    }

    // clear the stack for sure
    lua_settop(cur->pLUA, 0);
    return true;
}
//------------------------------------------------------------------------------

void ScriptPushUser(lua_State * L, User * u, const bool &bFullTable/* = false*/) {
	lua_checkstack(L, 3); // we need 3 (1 table, 2 id, 3 value) empty slots in stack, check it to be sure

	lua_newtable(L);
	int i = lua_gettop(L);

	lua_pushliteral(L, "sNick");
	lua_pushlstring(L, u->sNick, u->ui8NickLen);
	lua_rawset(L, i);

	lua_pushliteral(L, "uptr");
	lua_pushlightuserdata(L, (void *)u);
	lua_rawset(L, i);

	lua_pushliteral(L, "sIP");
	lua_pushlstring(L, u->sIP, u->ui8IpLen);
	lua_rawset(L, i);

	lua_pushliteral(L, "iProfile");
	lua_pushinteger(L, u->i32Profile);
	lua_rawset(L, i);

    if(bFullTable == true) {
        ScriptPushUserExtended(L, u, i);
    }
}
//------------------------------------------------------------------------------

void ScriptPushUserExtended(lua_State * L, User * u, const int &iTable) {
	lua_pushliteral(L, "sMode");
	if(u->sModes[0] != '\0') {
		lua_pushstring(L, u->sModes);
	} else {
		lua_pushnil(L);
	}
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sMyInfoString");
	if(u->sMyInfoOriginal != NULL) {
		lua_pushlstring(L, u->sMyInfoOriginal, u->ui16MyInfoOriginalLen);
	} else {
		lua_pushnil(L);
	}
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sDescription");
	if(u->sDescription != NULL) {
		lua_pushlstring(L, u->sDescription, (size_t)u->ui8DescriptionLen);
	} else {
		lua_pushnil(L);
	}
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sTag");
	if(u->sTag != NULL) {
		lua_pushlstring(L, u->sTag, (size_t)u->ui8TagLen);
	} else {
		lua_pushnil(L);
	}
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sConnection");
	if(u->sConnection != NULL) {
		lua_pushlstring(L, u->sConnection, (size_t)u->ui8ConnectionLen);
	} else {
		lua_pushnil(L);
	}
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sEmail");
	if(u->sEmail != NULL) {
		lua_pushlstring(L, u->sEmail, (size_t)u->ui8EmailLen);
	} else {
		lua_pushnil(L);
	}
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sClient");
	if(u->sClient != NULL) {
		lua_pushlstring(L, u->sClient, (size_t)u->ui8ClientLen);
	} else {
		lua_pushnil(L);
	}
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sClientVersion");
	if(u->sTagVersion != NULL) {
		lua_pushlstring(L, u->sTagVersion, (size_t)u->ui8TagVersionLen);
	} else {
		lua_pushnil(L);
	}
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sVersion");
	if(u->sVersion != NULL) {
		lua_pushstring(L, u->sVersion);
	} else {
		lua_pushnil(L);
	}
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sCountryCode");
	if(clsIpP2Country::mPtr->ui32Count != 0) {
		lua_pushlstring(L, clsIpP2Country::mPtr->GetCountry(u->ui8Country, false), 2);
	} else {
		lua_pushnil(L);
	}
	lua_rawset(L, iTable);

	lua_pushliteral(L, "bConnected");
	u->ui8State == User::STATE_ADDED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "bActive");
	if((u->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
        (u->ui32BoolBits & User::BIT_IPV6_ACTIVE) == User::BIT_IPV6_ACTIVE ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
    } else {
	   (u->ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
    }
	lua_rawset(L, iTable);

	lua_pushliteral(L, "bOperator");
	(u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "bUserCommand");
	(u->ui32SupportBits & User::SUPPORTBIT_USERCOMMAND) == User::SUPPORTBIT_USERCOMMAND ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "bQuickList");
	(u->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "bSuspiciousTag");
	(u->ui32BoolBits & User::BIT_HAVE_BADTAG) == User::BIT_HAVE_BADTAG ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
	lua_rawset(L, iTable);

	lua_pushliteral(L, "iShareSize");
#if LUA_VERSION_NUM < 503
	lua_pushnumber(L, (double)u->ui64SharedSize);
#else
	lua_pushinteger(L, u->ui64SharedSize);
#endif
	lua_rawset(L, iTable);

	lua_pushliteral(L, "iHubs");
#if LUA_VERSION_NUM < 503
	lua_pushnumber(L, u->Hubs);
#else
	lua_pushinteger(L, u->Hubs);
#endif
	lua_rawset(L, iTable);

	lua_pushliteral(L, "iNormalHubs");
#if LUA_VERSION_NUM < 503
	(u->ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(L) : lua_pushnumber(L, u->iNormalHubs);
#else
	(u->ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(L) : lua_pushinteger(L, u->iNormalHubs);
#endif
	lua_rawset(L, iTable);
    
	lua_pushliteral(L, "iRegHubs");
#if LUA_VERSION_NUM < 503
	(u->ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(L) : lua_pushnumber(L, u->iRegHubs);
#else
	(u->ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(L) : lua_pushinteger(L, u->iRegHubs);
#endif
	lua_rawset(L, iTable);
    
	lua_pushliteral(L, "iOpHubs");
#if LUA_VERSION_NUM < 503
	(u->ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(L) : lua_pushnumber(L, u->iOpHubs);
#else
	(u->ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(L) : lua_pushinteger(L, u->iOpHubs);
#endif
	lua_rawset(L, iTable);

	lua_pushliteral(L, "iSlots");
#if LUA_VERSION_NUM < 503
	lua_pushnumber(L, u->Slots);
#else
	lua_pushinteger(L, u->Slots);
#endif
	lua_rawset(L, iTable);

	lua_pushliteral(L, "iLlimit");
#if LUA_VERSION_NUM < 503
	lua_pushnumber(L, u->LLimit);
#else
	lua_pushinteger(L, u->LLimit);
#endif
	lua_rawset(L, iTable);

	lua_pushliteral(L, "iDefloodWarns");
#if LUA_VERSION_NUM < 503
	lua_pushnumber(L, u->iDefloodWarnings);
#else
	lua_pushinteger(L, u->iDefloodWarnings);
#endif
	lua_rawset(L, iTable);

	lua_pushliteral(L, "iMagicByte");
#if LUA_VERSION_NUM < 503
	lua_pushnumber(L, u->ui8MagicByte);
#else
	lua_pushinteger(L, u->ui8MagicByte);
#endif
	lua_rawset(L, iTable);

	lua_pushliteral(L, "iLoginTime");
#if LUA_VERSION_NUM < 503
	lua_pushnumber(L, (double)u->tLoginTime);
#else
	lua_pushinteger(L, u->tLoginTime);
#endif
	lua_rawset(L, iTable);

	lua_pushliteral(L, "sMac");
    char sMac[18];
    if(GetMacAddress(u->sIP, sMac) == true) {
        lua_pushlstring(L, sMac, 17);
    } else {
        lua_pushnil(L);
    }
	lua_rawset(L, iTable);

    lua_pushliteral(L, "bDescriptionChanged");
    (u->ui32InfoBits & User::INFOBIT_DESCRIPTION_CHANGED) == User::INFOBIT_DESCRIPTION_CHANGED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
    lua_rawset(L, iTable);

    lua_pushliteral(L, "bTagChanged");
    (u->ui32InfoBits & User::INFOBIT_TAG_CHANGED) == User::INFOBIT_TAG_CHANGED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
    lua_rawset(L, iTable);

    lua_pushliteral(L, "bConnectionChanged");
    (u->ui32InfoBits & User::INFOBIT_CONNECTION_CHANGED) == User::INFOBIT_CONNECTION_CHANGED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
    lua_rawset(L, iTable);

    lua_pushliteral(L, "bEmailChanged");
    (u->ui32InfoBits & User::INFOBIT_EMAIL_CHANGED) == User::INFOBIT_EMAIL_CHANGED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
    lua_rawset(L, iTable);

    lua_pushliteral(L, "bShareChanged");
    (u->ui32InfoBits & User::INFOBIT_SHARE_CHANGED) == User::INFOBIT_SHARE_CHANGED ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
    lua_rawset(L, iTable);

    lua_pushliteral(L, "sScriptedDescriptionShort");
    if(u->sChangedDescriptionShort != NULL) {
		lua_pushlstring(L, u->sChangedDescriptionShort, u->ui8ChangedDescriptionShortLen);
	} else {
		lua_pushnil(L);
	}
    lua_rawset(L, iTable);

    lua_pushliteral(L, "sScriptedDescriptionLong");
    if(u->sChangedDescriptionLong != NULL) {
		lua_pushlstring(L, u->sChangedDescriptionLong, u->ui8ChangedDescriptionLongLen);
	} else {
		lua_pushnil(L);
	}
    lua_rawset(L, iTable);

    lua_pushliteral(L, "sScriptedTagShort");
    if(u->sChangedTagShort != NULL) {
		lua_pushlstring(L, u->sChangedTagShort, u->ui8ChangedTagShortLen);
	} else {
		lua_pushnil(L);
	}
    lua_rawset(L, iTable);

    lua_pushliteral(L, "sScriptedTagLong");
    if(u->sChangedTagLong != NULL) {
		lua_pushlstring(L, u->sChangedTagLong, u->ui8ChangedTagLongLen);
	} else {
		lua_pushnil(L);
	}
    lua_rawset(L, iTable);

    lua_pushliteral(L, "sScriptedConnectionShort");
    if(u->sChangedConnectionShort != NULL) {
		lua_pushlstring(L, u->sChangedConnectionShort, u->ui8ChangedConnectionShortLen);
	} else {
		lua_pushnil(L);
	}
    lua_rawset(L, iTable);

    lua_pushliteral(L, "sScriptedConnectionLong");
    if(u->sChangedConnectionLong != NULL) {
		lua_pushlstring(L, u->sChangedConnectionLong, u->ui8ChangedConnectionLongLen);
	} else {
		lua_pushnil(L);
	}
    lua_rawset(L, iTable);

    lua_pushliteral(L, "sScriptedEmailShort");
    if(u->sChangedEmailShort != NULL) {
		lua_pushlstring(L, u->sChangedEmailShort, u->ui8ChangedEmailShortLen);
	} else {
		lua_pushnil(L);
	}
    lua_rawset(L, iTable);

    lua_pushliteral(L, "sScriptedEmailLong");
    if(u->sChangedEmailLong != NULL) {
		lua_pushlstring(L, u->sChangedEmailLong, u->ui8ChangedEmailLongLen);
	} else {
		lua_pushnil(L);
	}
    lua_rawset(L, iTable);

    lua_pushliteral(L, "iScriptediShareSizeShort");
#if LUA_VERSION_NUM < 503
	lua_pushnumber(L, (double)u->ui64ChangedSharedSizeShort);
#else
    lua_pushinteger(L, u->ui64ChangedSharedSizeShort);
#endif
    lua_rawset(L, iTable);

    lua_pushliteral(L, "iScriptediShareSizeLong");
#if LUA_VERSION_NUM < 503
	lua_pushnumber(L, (double)u->ui64ChangedSharedSizeLong);
#else
    lua_pushinteger(L, u->ui64ChangedSharedSizeLong);
#endif
    lua_rawset(L, iTable);

    lua_pushliteral(L, "tIPs");
    lua_newtable(L);

    int t = lua_gettop(L);

#if LUA_VERSION_NUM < 503
	lua_pushnumber(L, 1);
#else
	lua_pushinteger(L, 1);
#endif
	lua_pushlstring(L, u->sIP, u->ui8IpLen);
	lua_rawset(L, t);

    if(u->sIPv4[0] != '\0') {
#if LUA_VERSION_NUM < 503
		lua_pushnumber(L, 2);
#else
        lua_pushinteger(L, 2);
#endif
        lua_pushlstring(L, u->sIPv4, u->ui8IPv4Len);
        lua_rawset(L, t);
    }

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

    User *u = reinterpret_cast<User *>(lua_touserdata(L, iTop+1));
                
    if(u == NULL) {
        return NULL;
    }

	if(u != clsScriptManager::mPtr->pActualUser) {
        lua_pushliteral(L, "sNick");
        lua_gettable(L, 1);

        if(lua_gettop(L) != iTop+2 || lua_type(L, iTop+2) != LUA_TSTRING) {
            luaL_error(L, "bad argument #1 to '%s' (it's not user table)", sFunction);
            return NULL;
        }
    
        size_t szNickLen;
        char * sNick = (char *)lua_tolstring(L, iTop+2, &szNickLen);

		if(u != clsHashManager::mPtr->FindUser(sNick, szNickLen)) {
            return NULL;
        }
    }

    return u;
}
//------------------------------------------------------------------------------

void ScriptError(Script * cur) {
	size_t szLen = 0;
	char * stmp = (char*)lua_tolstring(cur->pLUA, -1, &szLen);

	string sMsg(stmp, szLen);

#ifdef _BUILD_GUI
    RichEditAppendText(clsMainWindowPageScripts::mPtr->hWndPageItems[clsMainWindowPageScripts::REDT_SCRIPTS_ERRORS],
        (string(clsLanguageManager::mPtr->sTexts[LAN_SYNTAX], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_SYNTAX]) + " " + sMsg).c_str());
#endif

	clsUdpDebug::mPtr->BroadcastFormat("[LUA] %s", sMsg.c_str());

    if(clsSettingManager::mPtr->bBools[SETBOOL_LOG_SCRIPT_ERRORS] == true) {
        AppendLog(sMsg, true);
    }

	if((((cur->ui16Functions & Script::ONERROR) == Script::ONERROR) == true && ScriptOnError(cur, stmp, szLen) == false) ||
        clsSettingManager::mPtr->bBools[SETBOOL_STOP_SCRIPT_ON_ERROR] == true) {
        // PPK ... stop buggy script ;)
		clsEventQueue::mPtr->AddNormal(clsEventQueue::EVENT_STOPSCRIPT, cur->sName);
    }
}
//------------------------------------------------------------------------------

#if defined(_WIN32) && !defined(_WIN_IOT)
    void ScriptOnTimer(const UINT_PTR &uiTimerId) {
#else
	void ScriptOnTimer(const uint64_t &ui64ActualMillis) {
#endif
	lua_State * pLuaState = NULL;

    ScriptTimer * pCurTmr = NULL,
        * pNextTmr = clsScriptManager::mPtr->pTimerListS;
        
    while(pNextTmr != NULL) {
        pCurTmr = pNextTmr;
        pNextTmr = pCurTmr->pNext;

#if defined(_WIN32) && !defined(_WIN_IOT)
        if(pCurTmr->uiTimerId == uiTimerId) {
#else
		while((ui64ActualMillis - pCurTmr->ui64LastTick) >= pCurTmr->ui64Interval) {
			pCurTmr->ui64LastTick += pCurTmr->ui64Interval;
#endif
            lua_pushcfunction(pCurTmr->pLua, ScriptTraceback);
            int iTraceback = lua_gettop(pCurTmr->pLua);

            if(pCurTmr->sFunctionName != NULL) {
				lua_getglobal(pCurTmr->pLua, pCurTmr->sFunctionName);
				int i = lua_gettop(pCurTmr->pLua);

				if(lua_isfunction(pCurTmr->pLua, i) == 0) {
					lua_settop(pCurTmr->pLua, 0);
#if defined(_WIN32) && !defined(_WIN_IOT)
				    return;
#else
					continue;
#endif
				}
			} else {
                lua_rawgeti(pCurTmr->pLua, LUA_REGISTRYINDEX, pCurTmr->iFunctionRef);
            }

			clsScriptManager::mPtr->pActualUser = NULL;

			lua_checkstack(pCurTmr->pLua, 1); // we need 1 empty slots in stack, check it to be sure

#if defined(_WIN32) && !defined(_WIN_IOT)
            lua_pushlightuserdata(pCurTmr->pLua, (void *)uiTimerId);
#else
			lua_pushlightuserdata(pCurTmr->pLua, (void *)pCurTmr);
#endif

			pLuaState = pCurTmr->pLua; // For case when timer will be removed in OnTimer
	
			// 1 passed parameters, 0 returned
			if(lua_pcall(pCurTmr->pLua, 1, 0, iTraceback) != 0) {
				ScriptError(clsScriptManager::mPtr->FindScript(pCurTmr->pLua));
#if defined(_WIN32) && !defined(_WIN_IOT)
				return;
#else
				if(pNextTmr == NULL) {
					if(clsScriptManager::mPtr->pTimerListE != pCurTmr) {
						break;
					}
				} else if(pNextTmr->pPrev != pCurTmr) {
					break;
				}

				continue;
#endif
			}

			// clear the stack for sure
			lua_settop(pLuaState, 0);
#if defined(_WIN32) && !defined(_WIN_IOT)
			return;
#else
			if(pNextTmr == NULL) {
				if(clsScriptManager::mPtr->pTimerListE != pCurTmr) {
					break;
				}
			} else if(pNextTmr->pPrev != pCurTmr) {
				break;
			}

			continue;
#endif
        }

	}
}
//------------------------------------------------------------------------------

int ScriptTraceback(lua_State *L) {
#if LUA_VERSION_NUM > 501
    const char * sMsg = lua_tostring(L, 1);
    if(sMsg != NULL) {
        luaL_traceback(L, L, sMsg, 1);
        return 1;
    }

    return 0;
#else
    if(!lua_isstring(L, 1)) {
        return 1;
    }

    lua_getfield(L, LUA_GLOBALSINDEX, "debug");
    if(!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return 1;
    }

    lua_getfield(L, -1, "traceback");
    if(lua_isfunction(L, -1) == 0) {
        lua_pop(L, 2);
        return 1;
    }

    lua_pushvalue(L, 1);
    lua_pushinteger(L, 2);
    lua_call(L, 2, 1);
    return 1;
#endif
}
//------------------------------------------------------------------------------
