/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
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
#include "colUsers.h"
#include "DcCommands.h"
#include "eventqueue.h"
#include "GlobalDataQueue.h"
#include "hashBanManager.h"
#include "hashRegManager.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "LuaInc.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "serviceLoop.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
//#include "ZlibUtility.h"
//---------------------------------------------------------------------------
#include "HubCommands.h"
//---------------------------------------------------------------------------
#ifdef _WITH_SQLITE
	#include "DB-SQLite.h"
	#include <sqlite3.h>
#elif _WITH_POSTGRES
	#include "DB-PostgreSQL.h"
	#include <libpq-fe.h>
#elif _WITH_MYSQL
	#include "DB-MySQL.h"
	#include <mysql.h>
#endif
#include "IP2Country.h"
#include "LuaScript.h"
#include "TextFileManager.h"
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
    #include "../gui.win/RegisteredUsersDialog.h"
#endif
//---------------------------------------------------------------------------

bool clsHubCommands::DoCommand(User * pUser, char * sCommand, const size_t &szCmdLen, bool bFromPM/* = false*/) {
	size_t dlen;

    if(bFromPM == false) {
    	// Hub commands: !me
		if(strncasecmp(sCommand+pUser->ui8NickLen, "me ", 3) == 0) {
	    	// %me command
    	    if(szCmdLen - (pUser->ui8NickLen + 4) > 4) {
                sCommand[0] = '*';
    			sCommand[1] = ' ';
    			memcpy(sCommand+2, pUser->sNick, pUser->ui8NickLen);
                clsUsers::mPtr->SendChat2All(pUser, sCommand, szCmdLen-4, NULL);
                return true;
    	    }
    	    return false;
        }

        // PPK ... optional reply commands in chat to PM
        bFromPM = clsSettingManager::mPtr->bBools[SETBOOL_REPLY_TO_HUB_COMMANDS_AS_PM];

        sCommand[szCmdLen-5] = '\0'; // get rid of the pipe
        sCommand += pUser->ui8NickLen;
        dlen = szCmdLen - (pUser->ui8NickLen + 5);
    } else {
        sCommand++;
        dlen = szCmdLen - (pUser->ui8NickLen + 6);
    }

    switch(tolower(sCommand[0])) {
        case 'g':
            // Hub commands: !getbans
			if(dlen == 7 && strncasecmp(sCommand+1, "etbans", 6) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::GETBANLIST) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                UncountDeflood(pUser, bFromPM);

                int iMsgLen = CheckFromPm(pUser, bFromPM);

                int iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "<%s> ", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC]);
                iMsgLen += iRet;
                if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand2") == false) {
                    return true;
                }

				string BanList(clsServerManager::pGlobalBuffer, iMsgLen);
                bool bIsEmpty = true;

                if(clsBanManager::mPtr->pTempBanListS != NULL) {
                    uint32_t iBanNum = 0;
                    time_t acc_time;
                    time(&acc_time);

                    BanItem * curBan = NULL,
                        * nextBan = clsBanManager::mPtr->pTempBanListS;

                    while(nextBan != NULL) {
                        curBan = nextBan;
    		            nextBan = curBan->pNext;

                        if(acc_time > curBan->tTempBanExpire) {
							clsBanManager::mPtr->Rem(curBan);
                            delete curBan;

							continue;
                        }

                        if(iBanNum == 0) {
							BanList += string(clsLanguageManager::mPtr->sTexts[LAN_TEMP_BANS], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_TEMP_BANS]) + ":\n\n";
                        }

                        iBanNum++;
						BanList += "[ " + string(iBanNum) + " ]";

                        if(curBan->sIp[0] != '\0') {
                            if(((curBan->ui8Bits & clsBanManager::IP) == clsBanManager::IP) == true) {
								BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_BANNED], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_BANNED]);
                            }
							BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_IP], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_IP]) + ": " + string(curBan->sIp);
                            if(((curBan->ui8Bits & clsBanManager::FULL) == clsBanManager::FULL) == true) {
								BanList += " (" + string(clsLanguageManager::mPtr->sTexts[LAN_FULL], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_FULL]) + ")";
                            }
                        }

                        if(curBan->sNick != NULL) {
                            if(((curBan->ui8Bits & clsBanManager::NICK) == clsBanManager::NICK) == true) {
								BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_BANNED], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_BANNED]);
                            }
							BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_NICK], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_NICK]) + ": " + string(curBan->sNick);
                        }

                        if(curBan->sBy != NULL) {
							BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_BY], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_BY]) + ": " + string(curBan->sBy);
                        }

                        if(curBan->sReason != NULL) {
							BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_REASON], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_REASON]) + ": " + string(curBan->sReason);
                        }

                        struct tm *tm = localtime(&curBan->tTempBanExpire);
                        strftime(clsServerManager::pGlobalBuffer, 256, "%c\n", tm);

						BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_EXPIRE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_EXPIRE]) + ": " + string(clsServerManager::pGlobalBuffer);
                    }

                    if(iBanNum > 0) {
                        bIsEmpty = false;
                        BanList += "\n\n";
                    }
                }

                if(clsBanManager::mPtr->pPermBanListS != NULL) {
                    bIsEmpty = false;

					BanList += string(clsLanguageManager::mPtr->sTexts[LAN_PERM_BANS], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_PERM_BANS]) + ":\n\n";

                    uint32_t iBanNum = 0;
                    BanItem * curBan = NULL,
                        * nextBan = clsBanManager::mPtr->pPermBanListS;

                    while(nextBan != NULL) {
                        curBan = nextBan;
    		            nextBan = curBan->pNext;

                        iBanNum++;
						BanList += "[ " + string(iBanNum) + " ]";
                        
                        if(curBan->sIp[0] != '\0') {
                            if(((curBan->ui8Bits & clsBanManager::IP) == clsBanManager::IP) == true) {
								BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_BANNED], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_BANNED]);
							}
							BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_IP], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_IP]) + ": " + string(curBan->sIp);
                            if(((curBan->ui8Bits & clsBanManager::FULL) == clsBanManager::FULL) == true) {
								BanList += " (" + string(clsLanguageManager::mPtr->sTexts[LAN_FULL], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_FULL]) + ")";
                            }
                        }

                        if(curBan->sNick != NULL) {
							if(((curBan->ui8Bits & clsBanManager::NICK) == clsBanManager::NICK) == true) {
								   BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_BANNED], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_BANNED]);
                            }
							BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_NICK], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_NICK]) + ": " + string(curBan->sNick);
                        }

                        if(curBan->sBy != NULL) {
							BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_BY], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_BY]) + ": " + string(curBan->sBy);
                        }

                        if(curBan->sReason != NULL) {
							BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_REASON], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_REASON]) + ": " + string(curBan->sReason);
                        }

                        BanList += "\n";
                    }
                }

                if(bIsEmpty == true) {
					BanList += string(clsLanguageManager::mPtr->sTexts[LAN_NO_BANS_FOUND], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_NO_BANS_FOUND]) + "...|";
                } else {
                    BanList += "|";
                }

                pUser->SendCharDelayed(BanList.c_str(), BanList.size());
                return true;
            }

            // Hub commands: !gag
			if(strncasecmp(sCommand+1, "ag ", 3) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::GAG) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

               	if(dlen < 5 || sCommand[4] == '\0') {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->gag1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cgag <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], 
						clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                sCommand += 4;

                // Self-gag ?
				if(strcasecmp(sCommand, pUser->sNick) == 0) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->gag2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_CANT_GAG_YOURSELF]);
                    return true;
           	    }

                if(dlen > 100) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->gag3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cgag <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], 
						clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
                    return true;
                }

                User * pOtherUser = clsHashManager::mPtr->FindUser(sCommand, dlen-4);
        		if(pOtherUser == NULL) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->gag4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s: %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_ERROR], 
						sCommand,  clsLanguageManager::mPtr->sTexts[LAN_IS_NOT_IN_USERLIST]);
        			return true;
                }

                if(((pOtherUser->ui32BoolBits & User::BIT_GAGGED) == User::BIT_GAGGED) == true) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->gag5", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s: %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_ERROR], 
						pOtherUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_IS_ALREDY_GAGGED]);
        			return true;
                }

        		// PPK don't gag user with higher profile
        		if(pOtherUser->i32Profile != -1 && pUser->i32Profile > pOtherUser->i32Profile) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->gag6", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_NOT_ALW_TO_GAG], pOtherUser->sNick);
        			return true;
                }

				UncountDeflood(pUser, bFromPM);

                pOtherUser->ui32BoolBits |= User::BIT_GAGGED;
                pOtherUser->SendFormat("clsHubCommands::DoCommand->gag1", true, "<%s> %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_GAGGED_BY], pUser->sNick);

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                    clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::DoCommand->gag", "<%s> *** %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_HAS_GAGGED], pOtherUser->sNick);
                } 
                        
                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->gag7", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pOtherUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_HAS_GAGGED]);
                }
                return true;
            }

            // Hub commands: !getinfo
			if(strncasecmp(sCommand+1, "etinfo ", 7) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::GETINFO) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 9 || sCommand[8] == 0) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->getinfo1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cgetinfo <%s>. %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],  clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                if(dlen > 100) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->getinfo2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cgetinfo <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],  clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
                    return true;
                }

                sCommand += 8;

                User * pOtherUser = clsHashManager::mPtr->FindUser(sCommand, dlen-8);
                if(pOtherUser == NULL) {
#ifdef _WITH_SQLITE
					if(DBSQLite::mPtr->SearchNick(sCommand, uint8_t(dlen-8), pUser, bFromPM) == true) {
						UncountDeflood(pUser, bFromPM);
						return true;
					}
#elif _WITH_POSTGRES
					if(DBPostgreSQL::mPtr->SearchNick(sCommand, dlen-8, pUser, bFromPM) == true) {
						UncountDeflood(pUser, bFromPM);
						return true;
					}
#elif _WITH_MYSQL
					if(DBMySQL::mPtr->SearchNick(sCommand, dlen-8, pUser, bFromPM) == true) {
						UncountDeflood(pUser, bFromPM);
						return true;
					}
#endif
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->getinfo3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s: %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_ERROR], 
						sCommand, clsLanguageManager::mPtr->sTexts[LAN_NOT_FOUND]);
                    return true;
                }
                
				UncountDeflood(pUser, bFromPM);

                int iMsgLen = CheckFromPm(pUser, bFromPM);

                int iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "<%s> \n%s: %s", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_NICK], pOtherUser->sNick);
                iMsgLen += iRet;
                if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand23") == false) {
                    return true;
                }

                if(pOtherUser->i32Profile != -1) {
                    iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: %s", clsLanguageManager::mPtr->sTexts[LAN_PROFILE], clsProfileManager::mPtr->ppProfilesTable[pOtherUser->i32Profile]->sName);
                    iMsgLen += iRet;
                    if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand24") == false) {
                        return true;
                    }
                }

                iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: %s ", clsLanguageManager::mPtr->sTexts[LAN_STATUS], clsLanguageManager::mPtr->sTexts[LAN_ONLINE_FROM]);
                iMsgLen += iRet;
                if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand25") == false) {
                    return true;
                }

                struct tm *tm = localtime(&pOtherUser->tLoginTime);
                iRet = (int)strftime(clsServerManager::pGlobalBuffer+iMsgLen, 256, "%c", tm);
                iMsgLen += iRet;
                if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand26") == false) {
                    return true;
                }

				if(pOtherUser->sIPv4[0] != '\0') {
					iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: %s / %s\n%s: %0.02f %s", clsLanguageManager::mPtr->sTexts[LAN_IP], pOtherUser->sIP, pOtherUser->sIPv4, clsLanguageManager::mPtr->sTexts[LAN_SHARE_SIZE], (double)pOtherUser->ui64SharedSize/1073741824, clsLanguageManager::mPtr->sTexts[LAN_GIGA_BYTES]);
					iMsgLen += iRet;
					if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand27-1") == false) {
						return true;
					}
				} else {
					iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: %s\n%s: %0.02f %s", clsLanguageManager::mPtr->sTexts[LAN_IP], pOtherUser->sIP, clsLanguageManager::mPtr->sTexts[LAN_SHARE_SIZE], (double)pOtherUser->ui64SharedSize/1073741824, clsLanguageManager::mPtr->sTexts[LAN_GIGA_BYTES]);
					iMsgLen += iRet;
					if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand27-2") == false) {
						return true;
					}
				}

                if(pOtherUser->sDescription != NULL) {
                    iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: ", clsLanguageManager::mPtr->sTexts[LAN_DESCRIPTION]);
                    iMsgLen += iRet;
                    if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand28") == false) {
                        return true;
                    }
                    memcpy(clsServerManager::pGlobalBuffer+iMsgLen, pOtherUser->sDescription, pOtherUser->ui8DescriptionLen);
                    iMsgLen += pOtherUser->ui8DescriptionLen;
                }

                if(pOtherUser->sTag != NULL) {
                    iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: ", clsLanguageManager::mPtr->sTexts[LAN_TAG]);
                    iMsgLen += iRet;
                    if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand29") == false) {
                        return true;
                    }
                    memcpy(clsServerManager::pGlobalBuffer+iMsgLen, pOtherUser->sTag, pOtherUser->ui8TagLen);
                    iMsgLen += (int)pOtherUser->ui8TagLen;
                }
                    
                if(pOtherUser->sConnection != NULL) {
					iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: ", clsLanguageManager::mPtr->sTexts[LAN_CONNECTION]);
                    iMsgLen += iRet;
                    if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand30") == false) {
                        return true;
                    }
                    memcpy(clsServerManager::pGlobalBuffer+iMsgLen, pOtherUser->sConnection, pOtherUser->ui8ConnectionLen);
                    iMsgLen += pOtherUser->ui8ConnectionLen;
                }

                if(pOtherUser->sEmail != NULL) {
					iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: ", clsLanguageManager::mPtr->sTexts[LAN_EMAIL]);
                    iMsgLen += iRet;
                    if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand31") == false) {
                        return true;
                    }
                    memcpy(clsServerManager::pGlobalBuffer+iMsgLen, pOtherUser->sEmail, pOtherUser->ui8EmailLen);
                    iMsgLen += pOtherUser->ui8EmailLen;
                }

                if(clsIpP2Country::mPtr->ui32Count != 0) {
					iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: ", clsLanguageManager::mPtr->sTexts[LAN_COUNTRY]);
                    iMsgLen += iRet;
                    if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand32") == false) {
                        return true;
                    }
                    memcpy(clsServerManager::pGlobalBuffer+iMsgLen, clsIpP2Country::mPtr->GetCountry(pOtherUser->ui8Country, false), 2);
                    iMsgLen += 2;
                }

                clsServerManager::pGlobalBuffer[iMsgLen] = '|';
                clsServerManager::pGlobalBuffer[iMsgLen+1] = '\0';  
                pUser->SendCharDelayed(clsServerManager::pGlobalBuffer, iMsgLen+1);
                return true;
            }

            // Hub commands: !getipinfo
			if(strncasecmp(sCommand+1, "etipinfo ", 9) == 0) {
#if !defined(_WITH_SQLITE) && !defined(_WITH_POSTGRES) && !defined(_WITH_MYSQL)
				return false;
#endif
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::GETINFO) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 11 || sCommand[10] == 0) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->getipinfo1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cgetipinfo <%s>. %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_IP], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                if(dlen > 102) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->getipinfo2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cgetipinfo <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],  clsLanguageManager::mPtr->sTexts[LAN_IP], clsLanguageManager::mPtr->sTexts[LAN_MAX_ALWD_IP_LEN_39_CHARS]);
                    return true;
                }

                sCommand += 10;
#ifdef _WITH_SQLITE
				if(DBSQLite::mPtr->SearchIP(sCommand, pUser, bFromPM) == true) {
					UncountDeflood(pUser, bFromPM);
					return true;
				}
#elif _WITH_POSTGRES
				if(DBPostgreSQL::mPtr->SearchIP(sCommand, pUser, bFromPM) == true) {
					UncountDeflood(pUser, bFromPM);
					return true;
				}
#elif _WITH_MYSQL
				if(DBMySQL::mPtr->SearchIP(sCommand, pUser, bFromPM) == true) {
					UncountDeflood(pUser, bFromPM);
					return true;
				}
#endif
				pUser->SendFormatCheckPM("clsHubCommands::DoCommand->getipinfo3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s: %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_ERROR], 
					sCommand, clsLanguageManager::mPtr->sTexts[LAN_NOT_FOUND]);
                return true;
            }

            // Hub commands: !gettempbans
			if(dlen == 11 && strncasecmp(sCommand+1, "ettempbans", 10) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::GETBANLIST) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

				UncountDeflood(pUser, bFromPM);

                int iMsgLen = CheckFromPm(pUser, bFromPM);

                int iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "<%s> ", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC]);
                iMsgLen += iRet;
                if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand33") == false) {
                    return true;
                }

				string BanList = string(clsServerManager::pGlobalBuffer, iMsgLen);

                if(clsBanManager::mPtr->pTempBanListS != NULL) {
                    uint32_t iBanNum = 0;

                    time_t acc_time;
                    time(&acc_time);

                    BanItem * curBan = NULL,
                        * nextBan = clsBanManager::mPtr->pTempBanListS;

                    while(nextBan != NULL) {
                        curBan = nextBan;
    		            nextBan = curBan->pNext;

                        if(acc_time > curBan->tTempBanExpire) {
							clsBanManager::mPtr->Rem(curBan);
                            delete curBan;

							continue;
                        }

						if(iBanNum == 0) {
							BanList += string(clsLanguageManager::mPtr->sTexts[LAN_TEMP_BANS], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_TEMP_BANS]) + ":\n\n";
                        }

                        iBanNum++;
						BanList += "[ " + string(iBanNum) + " ]";

                        if(curBan->sIp[0] != '\0') {
                            if(((curBan->ui8Bits & clsBanManager::IP) == clsBanManager::IP) == true) {
								BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_BANNED], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_BANNED]);
                            }
							BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_IP], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_IP]) + ": " + string(curBan->sIp);
                            if(((curBan->ui8Bits & clsBanManager::FULL) == clsBanManager::FULL) == true) {
								BanList += " (" + string(clsLanguageManager::mPtr->sTexts[LAN_FULL], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_FULL]) + ")";
                            }
                        }

                        if(curBan->sNick != NULL) {
                            if(((curBan->ui8Bits & clsBanManager::NICK) == clsBanManager::NICK) == true) {
								BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_BANNED], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_BANNED]);
                            }
							BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_NICK], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_NICK]) + ": " + string(curBan->sNick);
                        }

						if(curBan->sBy != NULL) {
							BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_BY], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_BY]) + ": " + string(curBan->sBy);
                        }

                        if(curBan->sReason != NULL) {
							BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_REASON], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_REASON]) + ": " + string(curBan->sReason);
                        }

                        struct tm *tm = localtime(&curBan->tTempBanExpire);
                        strftime(clsServerManager::pGlobalBuffer, 256, "%c\n", tm);

						BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_EXPIRE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_EXPIRE]) + ": " + string(clsServerManager::pGlobalBuffer);
                    }

                    if(iBanNum > 0) {
                        BanList += "|";
                    } else {
						BanList += string(clsLanguageManager::mPtr->sTexts[LAN_NO_TEMP_BANS_FOUND], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_NO_TEMP_BANS_FOUND]) + "...|";
                    }
                } else {
					BanList += string(clsLanguageManager::mPtr->sTexts[LAN_NO_TEMP_BANS_FOUND], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_NO_TEMP_BANS_FOUND]) + "...|";
                }

				pUser->SendCharDelayed(BanList.c_str(), BanList.size());
                return true;
            }

            // Hub commands: !getscripts
			if(dlen == 10 && strncasecmp(sCommand+1, "etscripts", 9) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::RSTSCRIPTS) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

				UncountDeflood(pUser, bFromPM);

                int iMsgLen = CheckFromPm(pUser, bFromPM);

                int iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "<%s> ", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC]);
                iMsgLen += iRet;
                if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand35") == false) {
                    return true;
                }

				string ScriptList(clsServerManager::pGlobalBuffer, iMsgLen);

				ScriptList += string(clsLanguageManager::mPtr->sTexts[LAN_SCRIPTS], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_SCRIPTS]) + ":\n\n";

				for(uint8_t ui8i = 0; ui8i < clsScriptManager::mPtr->ui8ScriptCount; ui8i++) {
					ScriptList += "[ " + string(clsScriptManager::mPtr->ppScriptTable[ui8i]->bEnabled == true ? "1" : "0") +
						" ] " + string(clsScriptManager::mPtr->ppScriptTable[ui8i]->sName);

					if(clsScriptManager::mPtr->ppScriptTable[ui8i]->bEnabled == true) {
                        ScriptList += " ("+string(ScriptGetGC(clsScriptManager::mPtr->ppScriptTable[ui8i]))+" kB)\n";
                    } else {
                        ScriptList += "\n";
                    }
				}

                ScriptList += "|";
				pUser->SendCharDelayed(ScriptList.c_str(), ScriptList.size());
                return true;
            }

            // Hub commands: !getpermbans
			if(dlen == 11 && strncasecmp(sCommand+1, "etpermbans", 10) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::GETBANLIST) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

				UncountDeflood(pUser, bFromPM);

                int iMsgLen = CheckFromPm(pUser, bFromPM);

                int iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "<%s> ", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC]);
                iMsgLen += iRet;
                if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand37") == false) {
                    return true;
                }

				string BanList(clsServerManager::pGlobalBuffer, iMsgLen);

                if(clsBanManager::mPtr->pPermBanListS != NULL) {
					BanList += string(clsLanguageManager::mPtr->sTexts[LAN_PERM_BANS], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_PERM_BANS]) + ":\n\n";

                    uint32_t iBanNum = 0;
                    BanItem * curBan = NULL,
                        * nextBan = clsBanManager::mPtr->pPermBanListS;

                    while(nextBan != NULL) {
                        curBan = nextBan;
    		            nextBan = curBan->pNext;

						iBanNum++;
						BanList += "[ " + string(iBanNum) + " ]";

                        if(curBan->sIp[0] != '\0') {
                            if(((curBan->ui8Bits & clsBanManager::IP) == clsBanManager::IP) == true) {
								BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_BANNED], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_BANNED]);
                            }
							BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_IP], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_IP]) + ": " + string(curBan->sIp);
                            if(((curBan->ui8Bits & clsBanManager::FULL) == clsBanManager::FULL) == true) {
								BanList += " (" + string(clsLanguageManager::mPtr->sTexts[LAN_FULL], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_FULL]) + ")";
                            }
                        }

                        if(curBan->sNick != NULL) {
                            if(((curBan->ui8Bits & clsBanManager::NICK) == clsBanManager::NICK) == true) {
								BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_BANNED], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_BANNED]);
                            }
							BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_NICK], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_NICK]) + ": " + string(curBan->sNick);
                        }

                        if(curBan->sBy != NULL) {
							BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_BY], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_BY]) + ": " + string(curBan->sBy);
                        }

                        if(curBan->sReason != NULL) {
							BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_REASON], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_REASON]) + ": " + string(curBan->sReason);
                        }

                        BanList += "\n";
                    }

					BanList += "|";
                } else {
					BanList += string(clsLanguageManager::mPtr->sTexts[LAN_NO_PERM_BANS_FOUND], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_NO_PERM_BANS_FOUND]) + "...|";
                }

				pUser->SendCharDelayed(BanList.c_str(), BanList.size());
                return true;
            }

            // Hub commands: !getrangebans
			if(dlen == 12 && strncasecmp(sCommand+1, "etrangebans", 11) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::GET_RANGE_BANS) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

				UncountDeflood(pUser, bFromPM);

                int iMsgLen = CheckFromPm(pUser, bFromPM);

                int iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "<%s> ", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC]);
                iMsgLen += iRet;
                if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand39") == false) {
                    return true;
                }

				string BanList(clsServerManager::pGlobalBuffer, iMsgLen);
                bool bIsEmpty = true;

                if(clsBanManager::mPtr->pRangeBanListS != NULL) {
                    uint32_t iBanNum = 0;

                    time_t acc_time;
                    time(&acc_time);

                    RangeBanItem * curBan = NULL,
                        * nextBan = clsBanManager::mPtr->pRangeBanListS;

                    while(nextBan != NULL) {
                        curBan = nextBan;
    		            nextBan = curBan->pNext;

    		            if(((curBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP) == false)
                            continue;

                        if(acc_time > curBan->tTempBanExpire) {
							clsBanManager::mPtr->RemRange(curBan);
                            delete curBan;

							continue;
                        }

                        if(iBanNum == 0) {
							BanList += string(clsLanguageManager::mPtr->sTexts[LAN_TEMP_RANGE_BANS], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_TEMP_RANGE_BANS]) + ":\n\n";
                        }

                        iBanNum++;
						BanList += "[ " + string(iBanNum) + " ]";
						BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_RANGE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_RANGE]) +
							": " + string(curBan->sIpFrom) + "-" + string(curBan->sIpTo);

                        if(((curBan->ui8Bits & clsBanManager::FULL) == clsBanManager::FULL) == true) {
							BanList += " (" + string(clsLanguageManager::mPtr->sTexts[LAN_FULL], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_FULL]) + ")";
                        }

                        if(curBan->sBy != NULL) {
							BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_BY], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_BY]) + ": " + string(curBan->sBy);
                        }

                        if(curBan->sReason != NULL) {
							BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_REASON], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_REASON]) +
								": " + string(curBan->sReason);
                        }

                        struct tm *tm = localtime(&curBan->tTempBanExpire);
                        strftime(clsServerManager::pGlobalBuffer, 256, "%c\n", tm);

						BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_EXPIRE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_EXPIRE]) + ": " + string(clsServerManager::pGlobalBuffer);
                    }

                    if(iBanNum > 0) {
                        bIsEmpty = false;
                        BanList += "\n\n";
                    }

                    iBanNum = 0;
                    nextBan = clsBanManager::mPtr->pRangeBanListS;

                    while(nextBan != NULL) {
                        curBan = nextBan;
    		            nextBan = curBan->pNext;

    		            if(((curBan->ui8Bits & clsBanManager::PERM) == clsBanManager::PERM) == false)
                            continue;

                        if(iBanNum == 0) {
                            bIsEmpty = false;
							BanList += string(clsLanguageManager::mPtr->sTexts[LAN_PERM_RANGE_BANS], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_PERM_RANGE_BANS]) + ":\n\n";
                        }

                        iBanNum++;
						BanList += "[ " + string(iBanNum) + " ]";
						BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_RANGE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_RANGE]) +
							": " + string(curBan->sIpFrom) + "-" + string(curBan->sIpTo);

                        if(((curBan->ui8Bits & clsBanManager::FULL) == clsBanManager::FULL) == true) {
							BanList += " (" + string(clsLanguageManager::mPtr->sTexts[LAN_FULL], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_FULL]) + ")";
                        }

                        if(curBan->sBy != NULL) {
							BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_BY], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_BY]) + ": " + string(curBan->sBy);
                        }

                        if(curBan->sReason != NULL) {
							BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_REASON], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_REASON]) +
								": " + string(curBan->sReason);
                        }

                        BanList += "\n";
                    }
                }

                if(bIsEmpty == true) {
					BanList += string(clsLanguageManager::mPtr->sTexts[LAN_NO_RANGE_BANS_FOUND], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_NO_RANGE_BANS_FOUND]) + "...|";
                } else {
                    BanList += "|";
                }

				pUser->SendCharDelayed(BanList.c_str(), BanList.size());
                return true;
            }

            // Hub commands: !getrangepermbans
			if(dlen == 16 && strncasecmp(sCommand+1, "etrangepermbans", 15) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::GET_RANGE_BANS) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

				UncountDeflood(pUser, bFromPM);

                int iMsgLen = CheckFromPm(pUser, bFromPM);

                int iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "<%s> ", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC]);
                iMsgLen += iRet;
                if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand41") == false) {
                    return true;
                }

				string BanList(clsServerManager::pGlobalBuffer, iMsgLen);
                bool bIsEmpty = true;

                if(clsBanManager::mPtr->pRangeBanListS != NULL) {
                    uint32_t iBanNum = 0;
                    RangeBanItem * curBan = NULL,
                        * nextBan = clsBanManager::mPtr->pRangeBanListS;

                    while(nextBan != NULL) {
                        curBan = nextBan;
    		            nextBan = curBan->pNext;

    		            if(((curBan->ui8Bits & clsBanManager::PERM) == clsBanManager::PERM) == false)
                            continue;

                        if(iBanNum == 0) {
                            bIsEmpty = false;

							BanList += string(clsLanguageManager::mPtr->sTexts[LAN_PERM_RANGE_BANS], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_PERM_RANGE_BANS]) + ":\n\n";
                        }

                        iBanNum++;
						BanList += "[ " + string(iBanNum) + " ]";
						BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_RANGE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_RANGE]) +
							": " + string(curBan->sIpFrom) + "-" + string(curBan->sIpTo);

                        if(((curBan->ui8Bits & clsBanManager::FULL) == clsBanManager::FULL) == true) {
							BanList += " (" + string(clsLanguageManager::mPtr->sTexts[LAN_FULL], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_FULL]) + ")";
                        }

                        if(curBan->sBy != NULL) {
							BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_BY], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_BY]) + ": " + string(curBan->sBy);
                        }

                        if(curBan->sReason != NULL) {
							BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_REASON], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_REASON]) +
								": " + string(curBan->sReason);
                        }

                        BanList += "\n";
                    }
                }

                if(bIsEmpty == true) {
					BanList += string(clsLanguageManager::mPtr->sTexts[LAN_NO_RANGE_PERM_BANS_FOUND], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_NO_RANGE_PERM_BANS_FOUND]) + "...|";
                } else {
                    BanList += "|";
                }

				pUser->SendCharDelayed(BanList.c_str(), BanList.size());
                return true;
            }

            // Hub commands: !getrangetempbans
			if(dlen == 16 && strncasecmp(sCommand+1, "etrangetempbans", 15) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::GET_RANGE_BANS) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

				UncountDeflood(pUser, bFromPM);

                int iMsgLen = CheckFromPm(pUser, bFromPM);

                int iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "<%s> ", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC]);
                iMsgLen += iRet;
                if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand43") == false) {
                    return true;
                }

				string BanList(clsServerManager::pGlobalBuffer, iMsgLen);
                bool bIsEmpty = true;

                if(clsBanManager::mPtr->pRangeBanListS != NULL) {
                    uint32_t iBanNum = 0;

                    time_t acc_time;
                    time(&acc_time);

                    RangeBanItem * curBan = NULL,
                        * nextBan = clsBanManager::mPtr->pRangeBanListS;

                    while(nextBan != NULL) {
                        curBan = nextBan;
    		            nextBan = curBan->pNext;

    		            if(((curBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP) == false)
                            continue;

                        if(acc_time > curBan->tTempBanExpire) {
							clsBanManager::mPtr->RemRange(curBan);
                            delete curBan;

							continue;
                        }

                        if(iBanNum == 0) {
							BanList += string(clsLanguageManager::mPtr->sTexts[LAN_TEMP_RANGE_BANS], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_TEMP_RANGE_BANS]) + ":\n\n";
                        }

                        iBanNum++;
						BanList += "[ " + string(iBanNum) + " ]";
						BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_RANGE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_RANGE]) +
							": " + string(curBan->sIpFrom) + "-" + string(curBan->sIpTo);

                        if(((curBan->ui8Bits & clsBanManager::FULL) == clsBanManager::FULL) == true) {
							BanList += " (" + string(clsLanguageManager::mPtr->sTexts[LAN_FULL], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_FULL]) + ")";
                        }

						if(curBan->sBy != NULL) {
							BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_BY], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_BY]) + ": " + string(curBan->sBy);
                        }

                        if(curBan->sReason != NULL) {
							BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_REASON], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_REASON]) +
								": " + string(curBan->sReason);
                        }

                        struct tm *tm = localtime(&curBan->tTempBanExpire);
                        strftime(clsServerManager::pGlobalBuffer, 256, "%c\n", tm);

						BanList += " " + string(clsLanguageManager::mPtr->sTexts[LAN_EXPIRE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_EXPIRE]) + ": " + string(clsServerManager::pGlobalBuffer);
                    }

                    if(iBanNum > 0) {
                        bIsEmpty = false;
                    }
                }

                if(bIsEmpty == true) {
					BanList += string(clsLanguageManager::mPtr->sTexts[LAN_NO_RANGE_TEMP_BANS_FOUND], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_NO_RANGE_TEMP_BANS_FOUND]) + "...|";
                } else {
                    BanList += "|";
                }

				pUser->SendCharDelayed(BanList.c_str(), BanList.size());
                return true;
            }

            return false;

        case 'n':
            // Hub commands: !nickban nick reason
			if(strncasecmp(sCommand+1, "ickban ", 7) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::BAN) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 9) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->nickban1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cnickban <%s> <%s>. %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],  clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                sCommand += 8;
                size_t szNickLen;
                char * sReason = strchr(sCommand, ' ');

                if(sReason != NULL) {
                    szNickLen = sReason - sCommand;
                    sReason[0] = '\0';
                    if(sReason[1] == '\0') {
                        sReason = NULL;
                    } else {
                        sReason++;

	                    if(strlen(sReason) > 512) {
	                    	sReason[510] = '.';
	                    	sReason[511] = '.';
	                        sReason[512] = '.';
	                        sReason[513] = '\0';
	                    }
                    }
                } else {
                    szNickLen = dlen-8;
                }

                if(sCommand[0] == '\0') {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->nickban2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %cnickban <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_NICK_SPECIFIED]);
                    return true;
                }

                if(szNickLen > 100) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->nickban3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %cnickban <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
                    return true;
                }

                // Self-ban ?
				if(strcasecmp(sCommand, pUser->sNick) == 0) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->nickban4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_CANT_BAN_YOURSELF]);
                    return true;
           	    }

                User * pOtherUser = clsHashManager::mPtr->FindUser(sCommand, szNickLen);
                if(pOtherUser != NULL) {
                    // PPK don't nickban user with higher profile
                    if(pOtherUser->i32Profile != -1 && pUser->i32Profile > pOtherUser->i32Profile) {
                        pUser->SendFormatCheckPM("clsHubCommands::DoCommand->nickban5", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_NOT_ALLOWED_TO], 
							clsLanguageManager::mPtr->sTexts[LAN_BAN_LWR], pOtherUser->sNick);
                        return true;
                    }

                    UncountDeflood(pUser, bFromPM);

                   	pOtherUser->SendFormat("clsHubCommands::DoCommand->nickban", false, "<%s> %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_HAD_BEEN_BANNED_BCS], sReason == NULL ? clsLanguageManager::mPtr->sTexts[LAN_NO_REASON_SPECIFIED] : sReason);

                    if(clsBanManager::mPtr->NickBan(pOtherUser, NULL, sReason, pUser->sNick) == true) {
						clsUdpDebug::mPtr->BroadcastFormat("[SYS] User %s (%s) nickbanned by %s", pOtherUser->sNick, pOtherUser->sIP, pUser->sNick);
						pOtherUser->Close();
                    } else {
                        pUser->SendFormatCheckPM("clsHubCommands::DoCommand->nickban6", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_NICK], 
							pOtherUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_IS_ALREDY_BANNED_DISCONNECT]);

                        pOtherUser->Close();
                        return true;
                    }
                } else {
                    return NickBan(pUser, sCommand, sReason, bFromPM);
                }

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                  	clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::DoCommand->nickban", "<%s> *** %s %s %s %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sCommand, clsLanguageManager::mPtr->sTexts[LAN_HAS_BEEN_BANNED_BY], pUser->sNick, 
					  clsLanguageManager::mPtr->sTexts[LAN_BECAUSE_LWR], sReason == NULL ? clsLanguageManager::mPtr->sTexts[LAN_NO_REASON_SPECIFIED] : sReason);
        		}

        		if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->nickban7", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sCommand, clsLanguageManager::mPtr->sTexts[LAN_ADDED_TO_BANS]);
        		}
                return true;
            }

            // Hub commands: !nicktempban nick time reason ...
            // PPK m = minutes, h = hours, d = day, w = weeks, M = months (30 day per month), Y = years (365 day per year)
			if(strncasecmp(sCommand+1, "icktempban ", 11) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::TEMP_BAN) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 15) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->nicktempban1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cnicktempban <%s> <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_TIME_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                // Now in sCommand we have nick, time and maybe reason
                char *sCmdParts[] = { NULL, NULL, NULL };
                uint16_t iCmdPartsLen[] = { 0, 0, 0 };

                uint8_t cPart = 0;

                sCmdParts[cPart] = sCommand+12; // nick start

                for(size_t szi = 12; szi < dlen; szi++) {
                    if(sCommand[szi] == ' ') {
                        sCommand[szi] = '\0';
                        iCmdPartsLen[cPart] = (uint16_t)((sCommand+szi)-sCmdParts[cPart]);

                        // are we on last space ???
                        if(cPart == 1) {
                            sCmdParts[2] = sCommand+szi+1;
                            iCmdPartsLen[2] = (uint16_t)(dlen-szi-1);
                            break;
                        }

                        cPart++;
                        sCmdParts[cPart] = sCommand+szi+1;
                    }
                }

                if(sCmdParts[2] == NULL && iCmdPartsLen[1] == 0 && sCmdParts[1] != NULL) {
                    iCmdPartsLen[1] = (uint16_t)(dlen-(sCmdParts[1]-sCommand));
                }

                if(sCmdParts[2] != NULL && iCmdPartsLen[2] == 0) {
                    sCmdParts[2] = NULL;
                }

                if(iCmdPartsLen[2] > 512) {
                	sCmdParts[2][510] = '.';
                	sCmdParts[2][511] = '.';
                    sCmdParts[2][512] = '.';
                    sCmdParts[2][513] = '\0';
                }

                if(iCmdPartsLen[0] == 0 || iCmdPartsLen[1] == 0) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->nicktempban2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cnicktempban <%s> <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_TIME_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_BAD_PARAMS_GIVEN]);
                    return true;
                }

                if(iCmdPartsLen[0] > 100) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->nicktempban3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cnicktempban <%s> <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_TIME_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], 
						clsLanguageManager::mPtr->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
                    return true;
                }

                // Self-ban ?
				if(strcasecmp(sCmdParts[0], pUser->sNick) == 0) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->nicktempban4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_CANT_BAN_YOURSELF]);
                    return true;
           	    }
           	    
                User * pOtherUser = clsHashManager::mPtr->FindUser(sCmdParts[0], iCmdPartsLen[0]);
                if(pOtherUser != NULL) {
                    // PPK don't tempban user with higher profile
                    if(pOtherUser->i32Profile != -1 && pUser->i32Profile > pOtherUser->i32Profile) {
                        pUser->SendFormatCheckPM("clsHubCommands::DoCommand->nicktempban5", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_NOT_ALLOWED_TO], 
							clsLanguageManager::mPtr->sTexts[LAN_TEMP_BAN_NICK], pOtherUser->sNick);
                        return true;
                    }
                } else {
                    return TempNickBan(pUser, sCmdParts[0], sCmdParts[1], iCmdPartsLen[1], sCmdParts[2], bFromPM);
                }

                char cTime = sCmdParts[1][iCmdPartsLen[1]-1];
                sCmdParts[1][iCmdPartsLen[1]-1] = '\0';
                int iTime = atoi(sCmdParts[1]);
                time_t acc_time, ban_time;

                if(iTime <= 0 || GenerateTempBanTime(cTime, (uint32_t)iTime, acc_time, ban_time) == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->nicktempban6", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cnicktempban <%s> <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_TIME_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_BAD_TIME_SPECIFIED]);
                    return true;
                }

                if(clsBanManager::mPtr->NickTempBan(pOtherUser, NULL, sCmdParts[2], pUser->sNick, 0, ban_time) == false) {
					pUser->SendFormatCheckPM("clsHubCommands::DoCommand->nicktempban7", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_NICK], 
						pOtherUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_ALRD_BND_LNGR_TIME_DISCONNECTED]);
					clsUdpDebug::mPtr->BroadcastFormat("[SYS] Already temp banned user %s (%s) disconnected by %s", pOtherUser->sNick, pOtherUser->sIP, pUser->sNick);

                    // Kick user
                    pOtherUser->Close();
                    return true;
                }

				UncountDeflood(pUser, bFromPM);

                char sTime[256];
                strcpy(sTime, formatTime((ban_time-acc_time)/60));

                // Send user a message that he has been tempbanned
                pOtherUser->SendFormat("clsHubCommands::DoCommand->nicktempban", false, "<%s> %s: %s %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_HAD_BEEN_TEMP_BANNED_TO], sTime, clsLanguageManager::mPtr->sTexts[LAN_BECAUSE_LWR], 
					sCmdParts[2] == NULL ? clsLanguageManager::mPtr->sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                    clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::DoCommand->nicktempban", "<%s> *** %s %s %s %s: %s %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sCmdParts[0], clsLanguageManager::mPtr->sTexts[LAN_HAS_BEEN_TMPBND_BY], pUser->sNick, 
						clsLanguageManager::mPtr->sTexts[LAN_TO_LWR], sTime, clsLanguageManager::mPtr->sTexts[LAN_BECAUSE_LWR], sCmdParts[2] == NULL ? clsLanguageManager::mPtr->sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);
                }

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->nicktempban8", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s: %s %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sCmdParts[0], 
						clsLanguageManager::mPtr->sTexts[LAN_BEEN_TEMP_BANNED_TO], sTime, clsLanguageManager::mPtr->sTexts[LAN_BECAUSE_LWR], sCmdParts[2] == NULL ? clsLanguageManager::mPtr->sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);
				}

				clsUdpDebug::mPtr->BroadcastFormat("[SYS] User %s (%s) tempbanned by %s", pOtherUser->sNick, pOtherUser->sIP, pUser->sNick);

				pOtherUser->Close();
                return true;
            }

            return false;

        case 'b':
            // Hub commands: !banip ip reason
			if(strncasecmp(sCommand+1, "anip ", 5) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::BAN) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 12) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->banip", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %cbanip <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD],  
						clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_IP], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                return BanIp(pUser, sCommand+6, bFromPM, false);
            }

			if(strncasecmp(sCommand+1, "an ", 3) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::BAN) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 5) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->ban", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cban <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], 
						clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }
                
                return Ban(pUser, sCommand+4, bFromPM, false);
            }

            return false;

        case 't':
            // Hub commands: !tempban nick time reason ... PPK m = minutes, h = hours, d = day, w = weeks, M = months (30 day per month), Y = years (365 day per year)
			if(strncasecmp(sCommand+1, "empban ", 7) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::TEMP_BAN) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 11) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->tempban", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %ctempban <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_TIME_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                return TempBan(pUser, sCommand+8, dlen-8, bFromPM, false);
            }

            // !tempbanip nick time reason ... PPK m = minutes, h = hours, d = day, w = weeks, M = months (30 day per month), Y = years (365 day per year)
			if(strncasecmp(sCommand+1, "empbanip ", 9) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::TEMP_BAN) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 14) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->tempbanip", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %ctempbanip <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_IP], clsLanguageManager::mPtr->sTexts[LAN_TIME_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                return TempBanIp(pUser, sCommand+10, dlen-10, bFromPM, false);
            }

            // Hub commands: !tempunban
			if(strncasecmp(sCommand+1, "empunban ", 9) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::TEMP_UNBAN) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 11 || sCommand[10] == '\0') {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->tempunban1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %ctempunban <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_IP_OR_NICK], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                sCommand += 10;

                if(dlen > 100) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->tempunban2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %ctempunban <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_IP_OR_NICK], clsLanguageManager::mPtr->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
                    return true;
                }

                if(clsBanManager::mPtr->TempUnban(sCommand) == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->tempunban3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SORRY], 
						sCommand, clsLanguageManager::mPtr->sTexts[LAN_IS_NOT_IN_MY_TEMP_BANS]);
                    return true;
                }

				UncountDeflood(pUser, bFromPM);

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
          	    	clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::DoCommand->tempunban", "<%s> *** %s %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_REMOVED_LWR], sCommand, clsLanguageManager::mPtr->sTexts[LAN_FROM_TEMP_BANS]);
                }

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->tempunban4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sCommand, 
						clsLanguageManager::mPtr->sTexts[LAN_REMOVED_FROM_TEMP_BANS]);
                }

                return true;
            }

            //Hub-Commands: !topic
			if(strncasecmp(sCommand+1, "opic ", 5) == 0 ) {
           	    if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::TOPIC) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 7 || sCommand[6] == '\0') {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->topic1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %ctopic <%s>, %ctopic <off>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NEW_TOPIC], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                sCommand += 6;

				UncountDeflood(pUser, bFromPM);

                if(dlen-6 > 256) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->topic2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %ctopic <%s>, %ctopic <off>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NEW_TOPIC], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_MAX_ALWD_TOPIC_LEN_256_CHARS]);
                    return true;
                }

				if(strcasecmp(sCommand, "off") == 0) {
                    clsSettingManager::mPtr->SetText(SETTXT_HUB_TOPIC, "", 0);

                    clsGlobalDataQueue::mPtr->AddQueueItem(clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_NAME], clsSettingManager::mPtr->ui16PreTextsLens[clsSettingManager::SETPRETXT_HUB_NAME], NULL, 0, clsGlobalDataQueue::CMD_HUBNAME);

                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->topic3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_TOPIC_HAS_BEEN_CLEARED]);
                } else {
                    clsSettingManager::mPtr->SetText(SETTXT_HUB_TOPIC, sCommand, dlen-6);

                    clsGlobalDataQueue::mPtr->AddQueueItem(clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_NAME], clsSettingManager::mPtr->ui16PreTextsLens[clsSettingManager::SETPRETXT_HUB_NAME], NULL, 0, clsGlobalDataQueue::CMD_HUBNAME);

                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->topic4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_TOPIC_HAS_BEEN_SET_TO], 
						sCommand);
                }

                return true;
            }

            return false;

        case 'm':          
            //Hub-Commands: !myip
			if(dlen == 4 && strncasecmp(sCommand+1, "yip", 3) == 0) {
                if(pUser->sIPv4[0] != '\0') {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->myip1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s: %s / %s|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOUR_IP_IS], 
						pUser->sIP, pUser->sIPv4);
                } else {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->myip2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s: %s|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOUR_IP_IS], pUser->sIP);
                }

                return true;
            }

            // Hub commands: !massmsg text
			if(strncasecmp(sCommand+1, "assmsg ", 7) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::MASSMSG) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 9) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->massmsg1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cmassmsg <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_MESSAGE_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

				UncountDeflood(pUser, bFromPM);

                if(dlen > 64000) {
                    sCommand[64000] = '\0';
                }

                int iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "%s $<%s> %s|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, sCommand+8);
                if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand117") == true) {
					clsGlobalDataQueue::mPtr->SingleItemStore(clsServerManager::pGlobalBuffer, iMsgLen, pUser, 0, clsGlobalDataQueue::SI_PM2ALL);
                }

                pUser->SendFormatCheckPM("clsHubCommands::DoCommand->massmsg2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_MASSMSG_TO_ALL_SENT]);
                return true;
            }

            return false;

        case 'r':
			if(dlen == 14 && strncasecmp(sCommand+1, "estartscripts", 13) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::RSTSCRIPTS) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->restartscripts1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_ERR_SCRIPTS_DISABLED]);
                    return true;
                }

				UncountDeflood(pUser, bFromPM);

                // post restart message
                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
               	    clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::DoCommand->restartscripts", "<%s> *** %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_RESTARTED_SCRIPTS]);
                }

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->restartscripts2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SCRIPTS_RESTARTED]);
                }

				clsScriptManager::mPtr->Restart();

                return true;
            }

			if(dlen == 7 && strncasecmp(sCommand+1, "estart", 6) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::RSTHUB) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

				UncountDeflood(pUser, bFromPM);

                // Send message to all that we are going to restart the hub
                int iMsgLen = sprintf(clsServerManager::pGlobalBuffer,"<%s> %s. %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_HUB_WILL_BE_RESTARTED], clsLanguageManager::mPtr->sTexts[LAN_BACK_IN_FEW_MINUTES]);
                if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand126") == true) {
                    clsUsers::mPtr->SendChat2All(pUser, clsServerManager::pGlobalBuffer, iMsgLen, NULL);
                }

                // post a restart hub message
                clsEventQueue::mPtr->AddNormal(clsEventQueue::EVENT_RESTART, NULL);
                return true;
            }

            //Hub-Commands: !reloadtxt
			if(dlen == 9 && strncasecmp(sCommand+1, "eloadtxt", 8) == 0) {
           	    if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::REFRESHTXT) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

               	if(clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_TEXT_FILES] == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->reloadtxt1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_TXT_SUP_NOT_ENABLED]);
        	        return true;
                }

				UncountDeflood(pUser, bFromPM);

               	clsTextFilesManager::mPtr->RefreshTextFiles();

               	if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                    clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::DoCommand->reloadtxt", "<%s> *** %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_RELOAD_TXT_FILES_LWR]);
                }

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->reloadtxt2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_TXT_FILES_RELOADED]);
                }
                return true;
            }

			// Hub commands: !restartscript scriptname
			if(strncasecmp(sCommand+1, "estartscript ", 13) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::RSTSCRIPTS) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->restartscript1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_ERR_SCRIPTS_DISABLED]);
                    return true;
                }

                if(dlen < 15 || sCommand[14] == '\0') {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->restartscript2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %crestartscript <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_SCRIPTNAME_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                sCommand += 14;

                if(dlen-14 > 256) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->restartscript3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %crestartscript <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_SCRIPTNAME_LWR], clsLanguageManager::mPtr->sTexts[LAN_MAX_ALWD_SCRIPT_NAME_LEN_256_CHARS]);
                    return true;
                }

                Script * curScript = clsScriptManager::mPtr->FindScript(sCommand);
                if(curScript == NULL || curScript->bEnabled == false || curScript->pLUA == NULL) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->restartscript4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_ERROR_SCRIPT], sCommand, clsLanguageManager::mPtr->sTexts[LAN_NOT_RUNNING]);
                    return true;
				}

				UncountDeflood(pUser, bFromPM);

				// stop script
				clsScriptManager::mPtr->StopScript(curScript, false);

				// try to start script
				if(clsScriptManager::mPtr->StartScript(curScript, false) == true) {
                    if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                        clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::DoCommand->restartscript", "<%s> *** %s %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_RESTARTED_SCRIPT], sCommand);
                    }

                    if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                        pUser->SendFormatCheckPM("clsHubCommands::DoCommand->restartscript5", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SCRIPT], 
							sCommand, clsLanguageManager::mPtr->sTexts[LAN_RESTARTED_LWR]);
                    } 
                    return true;
				} else {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->restartscript5", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_ERROR_SCRIPT], sCommand, clsLanguageManager::mPtr->sTexts[LAN_RESTART_FAILED]);
                    return true;
                }
            }

            // Hub commands: !rangeban fromip toip reason
			if(strncasecmp(sCommand+1, "angeban ", 8) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::RANGE_BAN) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 24) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->rangeban", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %crangeban <%s> <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],  clsLanguageManager::mPtr->sTexts[LAN_FROMIP], clsLanguageManager::mPtr->sTexts[LAN_TOIP], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                return RangeBan(pUser, sCommand+9, dlen-9, bFromPM, false);
            }

            // Hub commands: !rangetempban fromip toip time reason
			if(strncasecmp(sCommand+1, "angetempban ", 12) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::RANGE_TBAN) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 31) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->rangetempban", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %crangetempban <%s> <%s> <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_FROMIP], clsLanguageManager::mPtr->sTexts[LAN_TOIP],  clsLanguageManager::mPtr->sTexts[LAN_TIME_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], 
						clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                return RangeTempBan(pUser, sCommand+13, dlen-13, bFromPM, false);
            }

            // Hub commands: !rangeunban fromip toip
			if(strncasecmp(sCommand+1, "angeunban ", 10) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::RANGE_UNBAN) == false && clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::RANGE_TUNBAN) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 26) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->rangeunban", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %crangeunban <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_FROMIP], clsLanguageManager::mPtr->sTexts[LAN_TOIP], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::RANGE_UNBAN) == true) {
                    return RangeUnban(pUser, sCommand+11, bFromPM);
                } else if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::RANGE_TUNBAN) == true) {
                    return RangeUnban(pUser, sCommand+11, bFromPM, clsBanManager::TEMP);
                } else {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }
            }

            // Hub commands: !rangetempunban fromip toip
			if(strncasecmp(sCommand+1, "angetempunban ", 14) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::RANGE_TUNBAN) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 30) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->rangetempunban", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %crangetempunban <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_FROMIP], clsLanguageManager::mPtr->sTexts[LAN_TOIP], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                return RangeUnban(pUser, sCommand+15, bFromPM, clsBanManager::TEMP);
            }

            // Hub commands: !rangepermunban fromip toip
			if(strncasecmp(sCommand+1, "angepermunban ", 14) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::RANGE_UNBAN) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 30) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->rangepermunban", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %crangepermunban <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_FROMIP], clsLanguageManager::mPtr->sTexts[LAN_TOIP], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                return RangeUnban(pUser, sCommand+15, bFromPM, clsBanManager::PERM);
            }

            // !reguser <nick> <profile_name>
			if(strncasecmp(sCommand+1, "eguser ", 7) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::ADDREGUSER) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen > 255) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->reguser1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_CMD_TOO_LONG]);
                    return true;
                }

                char * sNick = sCommand+8; // nick start

                char * sProfile = strchr(sCommand+8, ' ');
                if(sProfile == NULL) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->reguser2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %creguser <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_PROFILENAME_LWR], clsLanguageManager::mPtr->sTexts[LAN_BAD_PARAMS_GIVEN]);
                    return true;
                }

                sProfile[0] = '\0';
                sProfile++;

                int iProfile = clsProfileManager::mPtr->GetProfileIndex(sProfile);
                if(iProfile == -1) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->reguser3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_ERR_NO_PROFILE_GIVEN_NAME_EXIST]);
                    return true;
                }

                // check hierarchy
                // deny if pUser is not Master and tries add equal or higher profile
                if(pUser->i32Profile > 0 && iProfile <= pUser->i32Profile) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->reguser4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_YOU_NOT_ALLOWED_TO_ADD_USER_THIS_PROFILE]);
                    return true;
                }

                size_t szNickLen = strlen(sNick);

                // check if user is registered
                if(clsRegManager::mPtr->Find(sNick, szNickLen) != NULL) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->reguser5", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_USER], 
						sNick, clsLanguageManager::mPtr->sTexts[LAN_IS_ALREDY_REGISTERED]);
                    return true;
                }

                User * pOtherUser = clsHashManager::mPtr->FindUser(sNick, szNickLen);
                if(pOtherUser == NULL) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->reguser6", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_ERROR], 
						sNick, clsLanguageManager::mPtr->sTexts[LAN_IS_NOT_ONLINE]);
                    return true;
                }

				UncountDeflood(pUser, bFromPM);

                if(pOtherUser->pLogInOut == NULL) {
                    pOtherUser->pLogInOut = new (std::nothrow) LoginLogout();
                    if(pOtherUser->pLogInOut == NULL) {
                        pOtherUser->ui32BoolBits |= User::BIT_ERROR;
                        pOtherUser->Close();

                        AppendDebugLog("%s - [MEM] Cannot allocate new pOtherUser->pLogInOut in clsHubCommands::DoCommand::RegUser\n");
                        return true;
                    }
                }

                pOtherUser->SetBuffer(sProfile);
                pOtherUser->ui32BoolBits |= User::BIT_WAITING_FOR_PASS;

                pOtherUser->SendFormat("clsHubCommands::DoCommand->reguser", true, "<%s> %s.|$GetPass|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_WERE_REGISTERED_PLEASE_ENTER_YOUR_PASSWORD]);

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                    clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::DoCommand->reguser", "<%s> *** %s %s %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_REGISTERED], sNick, clsLanguageManager::mPtr->sTexts[LAN_AS], sProfile);
                }

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->reguser7", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sNick, clsLanguageManager::mPtr->sTexts[LAN_REGISTERED], 
						clsLanguageManager::mPtr->sTexts[LAN_AS], sProfile);
                }

                return true;
            }

            return false;

        case 'u':
            // Hub commands: !unban
			if(strncasecmp(sCommand+1, "nban ", 5) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::UNBAN) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 7 || sCommand[6] == '\0') {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->unban1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cunban <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_IP_OR_NICK], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                sCommand += 6;

                if(dlen > 100) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->unban2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cunban <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_IP_OR_NICK], clsLanguageManager::mPtr->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
                    return true;
                }

                if(clsBanManager::mPtr->Unban(sCommand) == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->unban3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SORRY], 
						sCommand, clsLanguageManager::mPtr->sTexts[LAN_IS_NOT_IN_BANS]);
                    return true;
                }

				UncountDeflood(pUser, bFromPM);

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
          	    	clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::DoCommand->unban", "<%s> *** %s %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_REMOVED_LWR], sCommand, clsLanguageManager::mPtr->sTexts[LAN_FROM_BANS]);
                }

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->unban4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sCommand, clsLanguageManager::mPtr->sTexts[LAN_REMOVED_FROM_BANS]);
                }

                return true;
            }

            // Hub commands: !ungag
			if(strncasecmp(sCommand+1, "ngag ", 5) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::GAG) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

               	if(dlen < 7 || sCommand[6] == '\0') {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->ungag1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cungag <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                sCommand += 6;

                if(dlen > 100) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->ungag2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cungag <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
                    return true;
                }

                User * pOtherUser = clsHashManager::mPtr->FindUser(sCommand, dlen-6);
                if(pOtherUser == NULL) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->ungag3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_ERROR], 
						sCommand, clsLanguageManager::mPtr->sTexts[LAN_IS_NOT_IN_USERLIST]);
                    return true;
                }

                if(((pOtherUser->ui32BoolBits & User::BIT_GAGGED) == User::BIT_GAGGED) == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->ungag4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_ERROR], 
						sCommand, clsLanguageManager::mPtr->sTexts[LAN_IS_NOT_GAGGED]);
        			return true;
                }

				UncountDeflood(pUser, bFromPM);

                pOtherUser->ui32BoolBits &= ~User::BIT_GAGGED;

                pOtherUser->SendFormat("clsHubCommands::DoCommand->ungag", bFromPM, "<%s> %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_ARE_UNGAGGED_BY], pUser->sNick);

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
          		    clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::DoCommand->ungag", "<%s> *** %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_HAS_UNGAGGED], pOtherUser->sNick);
                }

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->ungag5", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pOtherUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_HAS_UNGAGGED]);
                }

                return true;
            }

            return false;

        case 'o':
            // Hub commands: !op nick
			if(strncasecmp(sCommand+1, "p ", 2) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::TEMPOP) == false || ((pUser->ui32BoolBits & User::BIT_TEMP_OPERATOR) == User::BIT_TEMP_OPERATOR) == true) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 4 || sCommand[3] == '\0') {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->op1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cop <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                sCommand += 3;

                if(dlen > 100) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->op2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cop <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
                    return true;
                }

                User * pOtherUser = clsHashManager::mPtr->FindUser(sCommand, dlen-3);
                if(pOtherUser == NULL) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->op3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_ERROR], 
						sCommand, clsLanguageManager::mPtr->sTexts[LAN_IS_NOT_IN_USERLIST]);
                    return true;
                }

                if(((pOtherUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->op4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pOtherUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_ALREDY_IS_OP]);
                    return true;
                }

				UncountDeflood(pUser, bFromPM);

                int pidx = clsProfileManager::mPtr->GetProfileIndex("Operator");
                if(pidx == -1) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->op5", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s. %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_ERROR], 
						clsLanguageManager::mPtr->sTexts[LAN_OPERATOR_PROFILE_MISSING]);
                    return true;
                }

                pOtherUser->ui32BoolBits |= User::BIT_OPERATOR;
                bool bAllowedOpChat = clsProfileManager::mPtr->IsAllowed(pOtherUser, clsProfileManager::ALLOWEDOPCHAT);
                pOtherUser->i32Profile = pidx;
                pOtherUser->ui32BoolBits |= User::BIT_TEMP_OPERATOR; // PPK ... to disallow adding more tempop by tempop user ;)
                clsUsers::mPtr->Add2OpList(pOtherUser);

                if(((pOtherUser->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == false) {
                    pOtherUser->SendFormat("clsHubCommands::DoCommand->op1", true, "$LogedIn %s|<%s> *** %s.|", pOtherUser->sNick, clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_GOT_TEMP_OP]);
                } else {
                	pOtherUser->SendFormat("clsHubCommands::DoCommand->op2", true, "<%s> *** %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_GOT_TEMP_OP]);
				}

                clsGlobalDataQueue::mPtr->OpListStore(pOtherUser->sNick);

                if(bAllowedOpChat != clsProfileManager::mPtr->IsAllowed(pOtherUser, clsProfileManager::ALLOWEDOPCHAT)) {
                    if(clsSettingManager::mPtr->bBools[SETBOOL_REG_OP_CHAT] == true &&
                        (clsSettingManager::mPtr->bBools[SETBOOL_REG_BOT] == false || clsSettingManager::mPtr->bBotsSameNick == false)) {
                        if(((pOtherUser->ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == false) {
                            pOtherUser->SendCharDelayed(clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_OP_CHAT_HELLO], clsSettingManager::mPtr->ui16PreTextsLens[clsSettingManager::SETPRETXT_OP_CHAT_HELLO]);
                        }
                        pOtherUser->SendCharDelayed(clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_OP_CHAT_MYINFO], clsSettingManager::mPtr->ui16PreTextsLens[clsSettingManager::SETPRETXT_OP_CHAT_MYINFO]);
                        pOtherUser->SendFormat("clsHubCommands::DoCommand->op3", true, "$OpList %s$$|", clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_NICK]);
                    }
                }

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
					clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::DoCommand->op", "<%s> *** %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_SETS_OP_MODE_TO], pOtherUser->sNick);
                }

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->op6", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pOtherUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_GOT_OP_STATUS]);
                }

                return true;
            }

            // Hub commands: !opmassmsg text
			if(strncasecmp(sCommand+1, "pmassmsg ", 9) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::MASSMSG) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 11) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->opmassmsg1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %copmassmsg <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_MESSAGE_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

				UncountDeflood(pUser, bFromPM);

                int iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "%s $<%s> %s|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, sCommand+10);
                if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand193") == true) {
					clsGlobalDataQueue::mPtr->SingleItemStore(clsServerManager::pGlobalBuffer, iMsgLen, pUser, 0, clsGlobalDataQueue::SI_PM2OPS);
                }

                pUser->SendFormatCheckPM("clsHubCommands::DoCommand->opmassmsg2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_MASSMSG_TO_OPS_SND]);
                return true;
            }

            return false;

        case 'd':
            // Hub commands: !drop
			if(strncasecmp(sCommand+1, "rop ", 4) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::DROP) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 6) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->drop1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cdrop <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                sCommand += 5;
                size_t szNickLen;

                char * sReason = strchr(sCommand, ' ');
                if(sReason != NULL) {
                    szNickLen = sReason-sCommand;
                    sReason[0] = '\0';
                    if(sReason[1] == '\0') {
                        sReason = NULL;
                    } else {
                        sReason++;

	                    if(strlen(sReason) > 512) {
	                    	sReason[510] = '.';
	                    	sReason[511] = '.';
	                        sReason[512] = '.';
	                        sReason[513] = '\0';
	                    }
                    }
                } else {
                    szNickLen = dlen-5;
                }

                if(szNickLen > 100) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->drop2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cdrop <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
                    return true;
                }

                if(sCommand[0] == '\0') {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->drop3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cdrop <%s>. %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }
                
                // Self-drop ?
				if(strcasecmp(sCommand, pUser->sNick) == 0) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->drop4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_CANT_DROP_YOURSELF]);
                    return true;
           	    }
           	    
                User * pOtherUser = clsHashManager::mPtr->FindUser(sCommand, szNickLen);
                if(pOtherUser == NULL) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->drop5", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sCommand, clsLanguageManager::mPtr->sTexts[LAN_IS_NOT_IN_USERLIST]);
                    return true;
                }
                
        		// PPK don't drop user with higher profile
        		if(pOtherUser->i32Profile != -1 && pUser->i32Profile > pOtherUser->i32Profile) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->drop6", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_NOT_ALLOWED_TO], 
						clsLanguageManager::mPtr->sTexts[LAN_DROP_LWR], pOtherUser->sNick);
        			return true;
        		}

				UncountDeflood(pUser, bFromPM);

                clsBanManager::mPtr->TempBan(pOtherUser, sReason, pUser->sNick, 0, 0, false);

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
         			clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::DoCommand->drop", "<%s> *** %s %s %s %s %s %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sCommand, clsLanguageManager::mPtr->sTexts[LAN_WITH_IP], pOtherUser->sIP, clsLanguageManager::mPtr->sTexts[LAN_DROP_ADDED_TEMPBAN_BY], 
					 	pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_BECAUSE_LWR], sReason == NULL ? clsLanguageManager::mPtr->sTexts[LAN_NO_REASON_SPECIFIED] : sReason);
                }

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->drop7", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sCommand, 
						clsLanguageManager::mPtr->sTexts[LAN_WITH_IP], pOtherUser->sIP, clsLanguageManager::mPtr->sTexts[LAN_DROP_ADDED_TEMPBAN_BCS], sReason == NULL ? clsLanguageManager::mPtr->sTexts[LAN_NO_REASON_SPECIFIED] : sReason);
                }

                pOtherUser->Close();
                return true;
            }

            // !DelRegUser <nick>   
			if(strncasecmp(sCommand+1, "elreguser ", 10) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::DELREGUSER) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen > 255) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->delreguser1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_CMD_TOO_LONG]);
                    return true;
                } else if(dlen < 13) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->delreguser2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cdelreguser <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                sCommand += 11;
                size_t szNickLen = dlen-11;

                // find him
				RegUser * pReg = clsRegManager::mPtr->Find(sCommand, szNickLen);
                if(pReg == NULL) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->delreguser3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_ERROR], 
						sCommand, clsLanguageManager::mPtr->sTexts[LAN_IS_NOT_IN_REGS]);
                    return true;
                }

                // check hierarchy
                // deny if pUser is not Master and tries delete equal or higher profile
                if(pUser->i32Profile > 0 && pReg->ui16Profile <= pUser->i32Profile) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->delreguser4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_YOURE_NOT_ALWD_TO_DLT_USER_THIS_PRFL]);
                    return true;
                }

				UncountDeflood(pUser, bFromPM);

                clsRegManager::mPtr->Delete(pReg);

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                    clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::DoCommand->delreguser", "<%s> *** %s %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_REMOVED_LWR], sCommand, clsLanguageManager::mPtr->sTexts[LAN_FROM_REGS]);
                }

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->delreguser5", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sCommand, clsLanguageManager::mPtr->sTexts[LAN_REMOVED_FROM_REGS]);
                }

                return true;
            }

            //Hub-Commands: !debug <port>|<list>|<off>
			if(strncasecmp(sCommand+1, "ebug ", 5) == 0) {
           	    if(((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 7) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->debug1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cdebug <%s>, %cdebug off. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_PORT_LWR], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                }

                sCommand += 6;

				if(strcasecmp(sCommand, "off") == 0) {
                    if(clsUdpDebug::mPtr->CheckUdpSub(pUser) == true) {
                        if(clsUdpDebug::mPtr->Remove(pUser) == true) {
                            UncountDeflood(pUser, bFromPM);

                            pUser->SendFormatCheckPM("clsHubCommands::DoCommand->debug2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_UNSUB_FROM_UDP_DBG]);

							clsUdpDebug::mPtr->BroadcastFormat("[SYS] Debug subscription cancelled: %s (%s)", pUser->sNick, pUser->sIP);

                            return true;
                        } else {
                            pUser->SendFormatCheckPM("clsHubCommands::DoCommand->debug3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_UNABLE_FIND_UDP_DBG_INTER]);
                            return true;
                        }
                    } else {
                        pUser->SendFormatCheckPM("clsHubCommands::DoCommand->debug4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_NOT_UDP_DEBUG_SUBSCRIB]);
                        return true;
                    }
                } else {
                    if(clsUdpDebug::mPtr->CheckUdpSub(pUser) == true) {
                        pUser->SendFormatCheckPM("clsHubCommands::DoCommand->debug5", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cdebug off %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
							clsLanguageManager::mPtr->sTexts[LAN_ALRD_UDP_SUBSCRIP_TO_UNSUB], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_IN_MAINCHAT]);
                        return true;
                    }

                    int dbg_port = atoi(sCommand);
                    if(dbg_port <= 0 || dbg_port > 65535) {
                        pUser->SendFormatCheckPM("clsHubCommands::DoCommand->debug6", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cdebug <%s>, %cdebug off.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
							clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_PORT_LWR], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0]);
                        return true;
                    }

                    if(clsUdpDebug::mPtr->New(pUser, (uint16_t)dbg_port) == true) {
						UncountDeflood(pUser, bFromPM);

                        pUser->SendFormatCheckPM("clsHubCommands::DoCommand->debug7", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %d. %s %cdebug off %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
							clsLanguageManager::mPtr->sTexts[LAN_SUBSCIB_UDP_DEBUG_ON_PORT], dbg_port, clsLanguageManager::mPtr->sTexts[LAN_TO_UNSUB_TYPE], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_IN_MAINCHAT]);

						clsUdpDebug::mPtr->BroadcastFormat("[SYS] New Debug subscriber: %s (%s)", pUser->sNick, pUser->sIP);
                        return true;
                    } else {
                        pUser->SendFormatCheckPM("clsHubCommands::DoCommand->debug8", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_UDP_DEBUG_SUBSCRIB_FAILED]);
                        return true;
                    }
                }
            }

            return false;

        case 'c':
/*            // !crash
			if(strncasecmp(sCommand+1, "rash", 4) == 0) {
                char * sTmp = (char*)calloc(392*1024*1024, 1);

                uint32_t iLen = 0;
                char *sData = ZlibUtility->CreateZPipe(sTmp, 392*1024*1024, iLen);

                if(iLen == 0) {
                    printf("0!");
                } else {
                    printf("LEN %u", iLen);
                    if(UserPutInSendBuf(pUser, sData, iLen)) {
                        UserTry2Send(pUser);
                    }
                }

                free(sTmp);

                return true;
            }
*/
            // !clrtempbans
			if(dlen == 11 && strncasecmp(sCommand+1, "lrtempbans", 10) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::CLRTEMPBAN) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

				UncountDeflood(pUser, bFromPM);
				clsBanManager::mPtr->ClearTemp();

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                    clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::DoCommand->clrtempbans", "<%s> *** %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_HAS_CLEARED_TEMPBANS]);
                }

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->clrtempbans", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_TEMP_BANS_CLEARED]);
                }
                return true;
            }

            // !clrpermbans
			if(dlen == 11 && strncasecmp(sCommand+1, "lrpermbans", 10) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::CLRPERMBAN) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

				UncountDeflood(pUser, bFromPM);
				clsBanManager::mPtr->ClearPerm();

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                    clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::DoCommand->clrpermbans", "<%s> *** %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_HAS_CLEARED_PERMBANS]);
                }

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->clrpermbans", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_PERM_BANS_CLEARED]);
                }
                return true;
            }

            // !clrrangetempbans
			if(dlen == 16 && strncasecmp(sCommand+1, "lrrangetempbans", 15) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::CLR_RANGE_TBANS) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

				UncountDeflood(pUser, bFromPM);
				clsBanManager::mPtr->ClearTempRange();

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                    clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::DoCommand->clrrangetempbans", "<%s> *** %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_HAS_CLEARED_TEMP_RANGEBANS]);
                }

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->clrrangetempbans", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_TEMP_RANGE_BANS_CLEARED]);
                }
                return true;
            }

            // !clrrangepermbans
			if(dlen == 16 && strncasecmp(sCommand+1, "lrrangepermbans", 15) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::CLR_RANGE_BANS) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

				UncountDeflood(pUser, bFromPM);
				clsBanManager::mPtr->ClearPermRange();

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                    clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::DoCommand->clrrangepermbans", "<%s> *** %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_HAS_CLEARED_PERM_RANGEBANS]);
                }

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->clrrangepermbans", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_PERM_RANGE_BANS_CLEARED]);
                }
                return true;
            }

            // !checknickban nick
			if(strncasecmp(sCommand+1, "hecknickban ", 12) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::GETBANLIST) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

				UncountDeflood(pUser, bFromPM);

                if(dlen < 14 || sCommand[13] == '\0') {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->checknickban1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> ***%s %cchecknickban <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
            		return true;
                }

                sCommand += 13;

                BanItem * pBan = clsBanManager::mPtr->FindNick(sCommand, dlen-13);
                if(pBan == NULL) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->checknickban2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_NO_BAN_FOUND]);
                    return true;
                }

                int iMsgLen = CheckFromPm(pUser, bFromPM);                        

                int iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "<%s> %s: %s", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_BANNED_NICK], pBan->sNick);
           	    iMsgLen += iRet;
                if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand263") == false) {
                    return true;
                }

                if(pBan->sIp[0] != '\0') {
                    if(((pBan->ui8Bits & clsBanManager::IP) == clsBanManager::IP) == true) {
						iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, " %s", clsLanguageManager::mPtr->sTexts[LAN_BANNED]);
               	        iMsgLen += iRet;
                        if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand264") == false) {
                            return true;
                        }
                    }
					iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, " %s: %s", clsLanguageManager::mPtr->sTexts[LAN_IP], pBan->sIp);
           	        iMsgLen += iRet;
                    if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand265") == false) {
                        return true;
                    }
                    if(((pBan->ui8Bits & clsBanManager::FULL) == clsBanManager::FULL) == true) {
						iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, " (%s)", clsLanguageManager::mPtr->sTexts[LAN_FULL]);
           	            iMsgLen += iRet;
                        if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand266") == false) {
                            return true;
                        }
                    }
                }

                if(pBan->sReason != NULL) {
					iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, " %s: %s", clsLanguageManager::mPtr->sTexts[LAN_REASON], pBan->sReason);
           	        iMsgLen += iRet;
                    if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand267") == false) {
                        return true;
                    }
                }

                if(pBan->sBy != NULL) {
                    iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, " %s: %s|", clsLanguageManager::mPtr->sTexts[LAN_BANNED_BY], pBan->sBy);
           	        iMsgLen += iRet;
                    if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand268") == false) {
                        return true;
                    }
                }

                if(((pBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP) == true) {
					iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, " %s: ", clsLanguageManager::mPtr->sTexts[LAN_EXPIRE]);
           	        iMsgLen += iRet;
                    if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand269") == false) {
                        return true;
                    }
                    struct tm *tm = localtime(&pBan->tTempBanExpire);
                    size_t szRet = strftime(clsServerManager::pGlobalBuffer+iMsgLen, 256, "%c.|", tm);
                    iMsgLen += (int)szRet;
                } else {
                    clsServerManager::pGlobalBuffer[iMsgLen] = '.';
                    clsServerManager::pGlobalBuffer[iMsgLen+1] = '|';
                    clsServerManager::pGlobalBuffer[iMsgLen+2] = '\0';
                    iMsgLen += 2;
                }
                pUser->SendCharDelayed(clsServerManager::pGlobalBuffer, iMsgLen);
                return true;
            }

            // !checkipban ip
			if(strncasecmp(sCommand+1, "heckipban ", 10) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::GETBANLIST) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

				UncountDeflood(pUser, bFromPM);

                if(dlen < 15 || sCommand[11] == '\0') {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->checkipban1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %ccheckipban <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_IP], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
            		return true;
                }

                sCommand += 11;

				uint8_t ui128Hash[16];
				memset(ui128Hash, 0, 16);

                // check ip
                if(HashIP(sCommand, ui128Hash) == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->checkipban2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %ccheckipban <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_IP], clsLanguageManager::mPtr->sTexts[LAN_NO_VALID_IP_SPECIFIED]);
                    return true;
                }
                
                time_t acc_time;
                time(&acc_time);

                BanItem * pNextBan = clsBanManager::mPtr->FindIP(ui128Hash, acc_time);

                if(pNextBan != NULL) {
                    int iMsgLen = CheckFromPm(pUser, bFromPM);

                    int iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "<%s> ", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC]);
                    iMsgLen += iRet;
                    if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand275") == false) {
                        return true;
                    }

					string Bans(clsServerManager::pGlobalBuffer, iMsgLen);
                    uint32_t iBanNum = 0;
                    BanItem * pCurBan = NULL;

                    while(pNextBan != NULL) {
                        pCurBan = pNextBan;
                        pNextBan = pCurBan->pHashIpTableNext;

                        if(((pCurBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP) == true) {
                            // PPK ... check if temban expired
                            if(acc_time >= pCurBan->tTempBanExpire) {
								clsBanManager::mPtr->Rem(pCurBan);
                                delete pCurBan;

								continue;
                            }
                        }

                        iBanNum++;
                        iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\n[%u] %s: %s", iBanNum, clsLanguageManager::mPtr->sTexts[LAN_BANNED_IP], pCurBan->sIp);
                        if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand276") == false) {
                            return true;
                        }
                        if(((pCurBan->ui8Bits & clsBanManager::FULL) == clsBanManager::FULL) == true) {
							iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, " (%s)", clsLanguageManager::mPtr->sTexts[LAN_FULL]);
                            iMsgLen += iRet;
                            if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand277") == false) {
                                return true;
                            }
                        }

						Bans += string(clsServerManager::pGlobalBuffer, iMsgLen);

                        if(pCurBan->sNick != NULL) {
                            iMsgLen = 0;
                            if(((pCurBan->ui8Bits & clsBanManager::NICK) == clsBanManager::NICK) == true) {
                                iMsgLen = sprintf(clsServerManager::pGlobalBuffer, " %s", clsLanguageManager::mPtr->sTexts[LAN_BANNED]);
                                if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand278") == false) {
                                    return true;
                                }
                            }
							iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, " %s: %s", clsLanguageManager::mPtr->sTexts[LAN_NICK], pCurBan->sNick);
                            iMsgLen += iRet;
                            if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand279") == false) {
                                return true;
                            }
							Bans += clsServerManager::pGlobalBuffer;
                        }

                        if(pCurBan->sReason != NULL) {
                            iMsgLen = sprintf(clsServerManager::pGlobalBuffer, " %s: %s", clsLanguageManager::mPtr->sTexts[LAN_REASON], pCurBan->sReason);
                            if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand280") == false) {
                                return true;
                            }
							Bans += clsServerManager::pGlobalBuffer;
                        }

                        if(pCurBan->sBy != NULL) {
                            iMsgLen = sprintf(clsServerManager::pGlobalBuffer, " %s: %s", clsLanguageManager::mPtr->sTexts[LAN_BANNED_BY], pCurBan->sBy);
                            if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand281") == false) {
                                return true;
                            }
							Bans += clsServerManager::pGlobalBuffer;
                        }

                        if(((pCurBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP) == true) {
                            struct tm *tm = localtime(&pCurBan->tTempBanExpire);
                            strftime(clsServerManager::pGlobalBuffer, 256, "%c", tm);
							Bans += " " + string(clsLanguageManager::mPtr->sTexts[LAN_EXPIRE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_EXPIRE]) + ": " + string(clsServerManager::pGlobalBuffer);
                        }
                    }

                    if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::GET_RANGE_BANS) == false) {
                        Bans += "|";

						pUser->SendCharDelayed(Bans.c_str(), Bans.size());
                        return true;
                    }

					RangeBanItem * pCurRangeBan = NULL,
                        * pNextRangeBan = clsBanManager::mPtr->FindRange(ui128Hash, acc_time);

                    while(pNextRangeBan != NULL) {
                        pCurRangeBan = pNextRangeBan;
                        pNextRangeBan = pCurRangeBan->pNext;

						if(((pCurRangeBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP) == true) {
							// PPK ... check if temban expired
							if(acc_time >= pCurRangeBan->tTempBanExpire) {
								clsBanManager::mPtr->RemRange(pCurRangeBan);
								delete pCurRangeBan;

								continue;
							}
						}

                        if(memcmp(pCurRangeBan->ui128FromIpHash, ui128Hash, 16) <= 0 && memcmp(pCurRangeBan->ui128ToIpHash, ui128Hash, 16) >= 0) {
                            iBanNum++;
                            iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\n[%u] %s: %s-%s", iBanNum, clsLanguageManager::mPtr->sTexts[LAN_RANGE], pCurRangeBan->sIpFrom, pCurRangeBan->sIpTo);
                            if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand282") == false) {
                                return true;
                            }
                            if(((pCurRangeBan->ui8Bits & clsBanManager::FULL) == clsBanManager::FULL) == true) {
								iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, " (%s)", clsLanguageManager::mPtr->sTexts[LAN_FULL]);
                                iMsgLen += iRet;
                                if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand283") == false) {
                                    return true;
                                }
                            }

							Bans += string(clsServerManager::pGlobalBuffer, iMsgLen);

                            if(pCurRangeBan->sReason != NULL) {
                                iMsgLen = sprintf(clsServerManager::pGlobalBuffer, " %s: %s", clsLanguageManager::mPtr->sTexts[LAN_REASON], pCurRangeBan->sReason);
                                if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand284") == false) {
                                    return true;
                                }
								Bans += clsServerManager::pGlobalBuffer;
                            }

                            if(pCurRangeBan->sBy != NULL) {
                                iMsgLen = sprintf(clsServerManager::pGlobalBuffer, " %s: %s", clsLanguageManager::mPtr->sTexts[LAN_BANNED_BY], pCurRangeBan->sBy);
                                if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand285") == false) {
                                    return true;
                                }
								Bans += clsServerManager::pGlobalBuffer;
                            }

                            if(((pCurRangeBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP) == true) {
                                struct tm *tm = localtime(&pCurRangeBan->tTempBanExpire);
                                strftime(clsServerManager::pGlobalBuffer, 256, "%c", tm);
								Bans += " " + string(clsLanguageManager::mPtr->sTexts[LAN_EXPIRE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_EXPIRE]) + ": " + string(clsServerManager::pGlobalBuffer);
                            }
                        }
                    }

                    Bans += "|";

					pUser->SendCharDelayed(Bans.c_str(), Bans.size());
                    return true;
                } else if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::GET_RANGE_BANS) == true) {
					RangeBanItem * pNextRangeBan = clsBanManager::mPtr->FindRange(ui128Hash, acc_time);

                    if(pNextRangeBan != NULL) {
                        int iMsgLen = CheckFromPm(pUser, bFromPM);

                        int iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "<%s> ", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC]);
                        iMsgLen += iRet;
                        if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand287") == false) {
                            return true;
                        }

						string Bans(clsServerManager::pGlobalBuffer, iMsgLen);
                        uint32_t iBanNum = 0;
                        RangeBanItem * pCurRangeBan = NULL;

                        while(pNextRangeBan != NULL) {
                            pCurRangeBan = pNextRangeBan;
                            pNextRangeBan = pCurRangeBan->pNext;

							if(((pCurRangeBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP) == true) {
								// PPK ... check if temban expired
								if(acc_time >= pCurRangeBan->tTempBanExpire) {
									clsBanManager::mPtr->RemRange(pCurRangeBan);
									delete pCurRangeBan;

									continue;
								}
							}

                            if(memcmp(pCurRangeBan->ui128FromIpHash, ui128Hash, 16) <= 0 && memcmp(pCurRangeBan->ui128ToIpHash, ui128Hash, 16) >= 0) {
                                iBanNum++;
                                iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\n[%u] %s: %s-%s", iBanNum, clsLanguageManager::mPtr->sTexts[LAN_RANGE], pCurRangeBan->sIpFrom, pCurRangeBan->sIpTo);
                                if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand288") == false) {
                                    return true;
                                }
                                if(((pCurRangeBan->ui8Bits & clsBanManager::FULL) == clsBanManager::FULL) == true) {
									iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, " (%s)", clsLanguageManager::mPtr->sTexts[LAN_FULL]);
                                    iMsgLen += iRet;
                                    if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand289") == false) {
                                        return true;
                                    }
                                }

								Bans += string(clsServerManager::pGlobalBuffer, iMsgLen);

                                if(pCurRangeBan->sReason != NULL) {
                                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, " %s: %s", clsLanguageManager::mPtr->sTexts[LAN_REASON], pCurRangeBan->sReason);
                                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand290") == false) {
                                        return true;
                                    }
									Bans += clsServerManager::pGlobalBuffer;
                                }

                                if(pCurRangeBan->sBy != NULL) {
                                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, " %s: %s", clsLanguageManager::mPtr->sTexts[LAN_BANNED_BY], pCurRangeBan->sBy);
                                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand291") == false) {
                                        return true;
                                    }
									Bans += clsServerManager::pGlobalBuffer;
                                }

                                if(((pCurRangeBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP) == true) {
                                    struct tm *tm = localtime(&pCurRangeBan->tTempBanExpire);
                                    strftime(clsServerManager::pGlobalBuffer, 256, "%c", tm);
									Bans += " " + string(clsLanguageManager::mPtr->sTexts[LAN_EXPIRE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_EXPIRE]) + ": " + string(clsServerManager::pGlobalBuffer);
                                }
                            }
                        }

                        Bans += "|";

						pUser->SendCharDelayed(Bans.c_str(), Bans.size());
                        return true;
                    }
                }

                pUser->SendFormatCheckPM("clsHubCommands::DoCommand->checkipban3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_NO_BAN_FOUND]);
                return true;
            }

            // !checkrangeban ipfrom ipto
			if(strncasecmp(sCommand+1, "heckrangeban ", 13) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::GET_RANGE_BANS) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

				UncountDeflood(pUser, bFromPM);

                if(dlen < 26) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->checkrangeban1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %ccheckrangeban <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_FROMIP], clsLanguageManager::mPtr->sTexts[LAN_TOIP], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
            		return true;
                }

                sCommand += 14;
                char * sIpTo = strchr(sCommand, ' ');
                
                if(sIpTo == NULL) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->checkrangeban2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %ccheckrangeban <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_FROMIP], clsLanguageManager::mPtr->sTexts[LAN_TOIP], clsLanguageManager::mPtr->sTexts[LAN_BAD_PARAMS_GIVEN]);
            		return true;
                }

                sIpTo[0] = '\0';
                sIpTo++;

                // check ipfrom
                if(sCommand[0] == '\0' || sIpTo[0] == '\0') {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->checkrangeban3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %ccheckrangeban <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_FROMIP], clsLanguageManager::mPtr->sTexts[LAN_TOIP], clsLanguageManager::mPtr->sTexts[LAN_BAD_PARAMS_GIVEN]);
            		return true;
                }

				uint8_t ui128FromHash[16], ui128ToHash[16];
				memset(ui128FromHash, 0, 16);
				memset(ui128ToHash, 0, 16);

                if(HashIP(sCommand, ui128FromHash) == false || HashIP(sIpTo, ui128ToHash) == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->checkrangeban4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %ccheckrangeban <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_FROMIP], clsLanguageManager::mPtr->sTexts[LAN_TOIP], clsLanguageManager::mPtr->sTexts[LAN_NO_VALID_IP_RANGE_SPECIFIED]);
                    return true;
                }

                time_t acc_time;
                time(&acc_time);

				RangeBanItem * pRangeBan = clsBanManager::mPtr->FindRange(ui128FromHash, ui128ToHash, acc_time);
                if(pRangeBan == NULL) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->checkrangeban5", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_NO_RANGE_BAN_FOUND]);
                    return true;
                }
                
                int iMsgLen = CheckFromPm(pUser, bFromPM);

                int iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "<%s> %s: %s-%s", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_RANGE], pRangeBan->sIpFrom, pRangeBan->sIpTo);
                iMsgLen += iRet;
                if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand305") == false) {
                    return true;
                }
                    
                if(((pRangeBan->ui8Bits & clsBanManager::FULL) == clsBanManager::FULL) == true) {
                    iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, " (%s)", clsLanguageManager::mPtr->sTexts[LAN_FULL]);
                    iMsgLen += iRet;
                    if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand306") == false) {
                        return true;
                    }
                }

                if(pRangeBan->sReason != NULL) {
					iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, " %s: %s", clsLanguageManager::mPtr->sTexts[LAN_REASON], pRangeBan->sReason);
                    iMsgLen += iRet;
                    if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand307") == false) {
                        return true;
                    }
                }

                if(pRangeBan->sBy != NULL) {
					iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, " %s: %s", clsLanguageManager::mPtr->sTexts[LAN_BANNED_BY], pRangeBan->sBy);
                    iMsgLen += iRet;
                    if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand308") == false) {
                        return true;
                    }
                }

                if(((pRangeBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP) == true) {
					iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, " %s: ", clsLanguageManager::mPtr->sTexts[LAN_EXPIRE]);
                    iMsgLen += iRet;
                    if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand309") == false) {
                        return true;
                    }
                    struct tm *tm = localtime(&pRangeBan->tTempBanExpire);
                    size_t szRet = strftime(clsServerManager::pGlobalBuffer+iMsgLen, 256, "%c.|", tm);
                    iMsgLen += (int)szRet;
                } else {
                    clsServerManager::pGlobalBuffer[iMsgLen] = '.';
                    clsServerManager::pGlobalBuffer[iMsgLen+1] = '|';
                    clsServerManager::pGlobalBuffer[iMsgLen+2] = '\0';
                    iMsgLen += 2;
                }

                pUser->SendCharDelayed(clsServerManager::pGlobalBuffer, iMsgLen);
                return true;
            }

            return false;

        case 'a':
            // !addreguser <nick> <pass> <profile_idx>
			if(strncasecmp(sCommand+1, "ddreguser ", 10) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::ADDREGUSER) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen > 255) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->addreguser1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_CMD_TOO_LONG]);
                    return true;
                }

                char *sCmdParts[] = { NULL, NULL, NULL };
                uint16_t iCmdPartsLen[] = { 0, 0, 0 };

                uint8_t cPart = 0;

                sCmdParts[cPart] = sCommand+11; // nick start

				for(size_t szi = 11; szi < dlen; szi++) {
					if(sCommand[szi] == ' ') {
						sCommand[szi] = '\0';
						iCmdPartsLen[cPart] = (uint16_t)((sCommand+szi)-sCmdParts[cPart]);

						// are we on last space ???
						if(cPart == 1) {
							sCmdParts[2] = sCommand+szi+1;
							iCmdPartsLen[2] = (uint16_t)(dlen-szi-1);
							for(size_t szj = dlen; szj > szi; szj--) {
								if(sCommand[szj] == ' ') {
									sCommand[szj] = '\0';

									sCmdParts[2] = sCommand+szj+1;
									iCmdPartsLen[2] = (uint16_t)(dlen-szj-1);

									sCommand[szi] = ' ';
									iCmdPartsLen[1] = (uint16_t)((sCommand+szj)-sCmdParts[1]);

									break;
								}
							}
							break;
                        }

                        cPart++;
						sCmdParts[cPart] = sCommand+szi+1;
                    }
                }

                if(iCmdPartsLen[0] == 0 || iCmdPartsLen[1] == 0 || iCmdPartsLen[2] == 0) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->addreguser2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %caddreguser <%s> <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_PASSWORD_LWR], clsLanguageManager::mPtr->sTexts[LAN_PROFILENAME_LWR], 
						clsLanguageManager::mPtr->sTexts[LAN_BAD_PARAMS_GIVEN]);
                    return true;
                }

                if(iCmdPartsLen[0] > 65) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->addreguser3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
                    return true;
                }

				if(strpbrk(sCmdParts[0], " $|") != NULL) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->addreguser4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_NO_BAD_CHARS_IN_NICK]);
                    return true;
                }

                if(iCmdPartsLen[1] > 65) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->addreguser5", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_MAX_ALWD_PASS_LEN_64_CHARS]);
                    return true;
                }

				if(strchr(sCmdParts[1], '|') != NULL) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->addreguser6", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_NO_PIPE_IN_PASS]);
                    return true;
                }

                int pidx = clsProfileManager::mPtr->GetProfileIndex(sCmdParts[2]);
                if(pidx == -1) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->addreguser7", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_ERR_NO_PROFILE_GIVEN_NAME_EXIST]);
                    return true;
                }

                // check hierarchy
                // deny if pUser is not Master and tries add equal or higher profile
                if(pUser->i32Profile > 0 && pidx <= pUser->i32Profile) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->addreguser8", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_YOU_NOT_ALLOWED_TO_ADD_USER_THIS_PROFILE]);
                    return true;
                }

                // try to add the user
                if(clsRegManager::mPtr->AddNew(sCmdParts[0], sCmdParts[1], (uint16_t)pidx) == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->addreguser9", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_USER], 
						sCmdParts[0], clsLanguageManager::mPtr->sTexts[LAN_IS_ALREDY_REGISTERED]);
                    return true;
                }

				UncountDeflood(pUser, bFromPM);

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                    clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::DoCommand->addreguser", "<%s> *** %s %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_SUCCESSFULLY_ADDED], sCmdParts[0], 
						clsLanguageManager::mPtr->sTexts[LAN_TO_REGISTERED_USERS]);
                }

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->addreguser10", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sCmdParts[0], 
						clsLanguageManager::mPtr->sTexts[LAN_SUCCESSFULLY_ADDED_TO_REGISTERED_USERS]);
                }
            
                User * pOtherUser = clsHashManager::mPtr->FindUser(sCmdParts[0], iCmdPartsLen[0]);
                if(pOtherUser != NULL) {
                    bool bAllowedOpChat = clsProfileManager::mPtr->IsAllowed(pOtherUser, clsProfileManager::ALLOWEDOPCHAT);
                    pOtherUser->i32Profile = pidx;
                    if(((pOtherUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                        if(clsProfileManager::mPtr->IsAllowed(pOtherUser, clsProfileManager::HASKEYICON) == true) {
                            pOtherUser->ui32BoolBits |= User::BIT_OPERATOR;
                        } else {
                            pOtherUser->ui32BoolBits &= ~User::BIT_OPERATOR;
                        }

                        if(((pOtherUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                            clsUsers::mPtr->Add2OpList(pOtherUser);
                            clsGlobalDataQueue::mPtr->OpListStore(pOtherUser->sNick);
                            if(bAllowedOpChat != clsProfileManager::mPtr->IsAllowed(pOtherUser, clsProfileManager::ALLOWEDOPCHAT)) {
                                if(clsSettingManager::mPtr->bBools[SETBOOL_REG_OP_CHAT] == true && (clsSettingManager::mPtr->bBools[SETBOOL_REG_BOT] == false || clsSettingManager::mPtr->bBotsSameNick == false)) {
                                    if(((pOtherUser->ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == false) {
                                        pOtherUser->SendCharDelayed(clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_OP_CHAT_HELLO], clsSettingManager::mPtr->ui16PreTextsLens[clsSettingManager::SETPRETXT_OP_CHAT_HELLO]);
                                    }
                                    pOtherUser->SendCharDelayed(clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_OP_CHAT_MYINFO], clsSettingManager::mPtr->ui16PreTextsLens[clsSettingManager::SETPRETXT_OP_CHAT_MYINFO]);
                                    pOtherUser->SendFormat("clsHubCommands::DoCommand->addreguser", true, "$OpList %s$$|", clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_NICK]);
                                }
                            }
                        }
                    } 
                }         
                return true;
            }

            return false;

        case 'h':
            // Hub commands: !help
			if(dlen == 4 && strncasecmp(sCommand+1, "elp", 3) == 0) {
                int iMsgLen = CheckFromPm(pUser, bFromPM);

                int iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "<%s> ", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC]);
               	iMsgLen += iRet;
                if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand330") == false) {
                    return true;
                }

				string help(clsServerManager::pGlobalBuffer, iMsgLen);
                bool bFull = false;
                bool bTemp = false;

                iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "%s:\n", clsLanguageManager::mPtr->sTexts[LAN_FOLOW_COMMANDS_AVAILABLE_TO_YOU]);
                if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand331") == false) {
                    return true;
                }
				help += clsServerManager::pGlobalBuffer;

                if(pUser->i32Profile != -1) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\n%s:\n", clsLanguageManager::mPtr->sTexts[LAN_PROFILE_SPECIFIC_CMDS]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand332") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cpasswd <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        clsLanguageManager::mPtr->sTexts[LAN_NEW_PASSWORD], clsLanguageManager::mPtr->sTexts[LAN_CHANGE_YOUR_PASSWORD]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand333") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::BAN)) {
                    bFull = true;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cban <%s> <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR],
                        clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_PERM_BAN_USER_GIVEN_NICK_DISCONNECT]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand334") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cbanip <%s> <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_IP],
                        clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_PERM_BAN_IP_ADDRESS]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand335") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cfullban <%s> <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR],
                        clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_PERM_BAN_USER_GIVEN_NICK_DISCONNECT]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand336") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cfullbanip <%s> <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_IP],
                        clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_PERM_BAN_IP_ADDRESS]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand337") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cnickban <%s> <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR],
                        clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_BAN_USERS_NICK_IFCONN_THENDISCONN]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand338") == false) {
                        return true;
                    }
                    help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::TEMP_BAN)) {
                    bFull = true; bTemp = true;

					iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%ctempban <%s> <%s> <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR],
                        clsLanguageManager::mPtr->sTexts[LAN_TIME_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_TEMP_BAN_USER_GIVEN_NICK_DISCONNECT]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand339") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%ctempbanip <%s> <%s> <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_IP],
                        clsLanguageManager::mPtr->sTexts[LAN_TIME_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_TEMP_BAN_IP_ADDRESS]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand340") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cfulltempban <%s> <%s> <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR],
                        clsLanguageManager::mPtr->sTexts[LAN_TIME_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_TEMP_BAN_USER_GIVEN_NICK_DISCONNECT]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand341") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cfulltempbanip <%s> <%s> <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_IP],
                        clsLanguageManager::mPtr->sTexts[LAN_TIME_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_TEMP_BAN_IP_ADDRESS]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand342") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cnicktempban <%s> <%s> <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR],
                        clsLanguageManager::mPtr->sTexts[LAN_TIME_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_TEMP_BAN_USERS_NICK_IFCONN_THENDISCONN]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand343") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::UNBAN) || clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::TEMP_UNBAN)) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cunban <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_IP_OR_NICK],
                        clsLanguageManager::mPtr->sTexts[LAN_UNBAN_IP_OR_NICK]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand344") == false) {
                        return true;
					}
					help += clsServerManager::pGlobalBuffer;
				}

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::UNBAN)) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cpermunban <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_IP_OR_NICK],
                        clsLanguageManager::mPtr->sTexts[LAN_UNBAN_PERM_BANNED_IP_OR_NICK]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand345") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::TEMP_UNBAN)) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%ctempunban <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_IP_OR_NICK],
                        clsLanguageManager::mPtr->sTexts[LAN_UNBAN_TEMP_BANNED_IP_OR_NICK]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand346") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::GETBANLIST)) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cgetbans - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_DISPLAY_LIST_OF_BANS]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand347") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cgetpermbans - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_DISPLAY_LIST_OF_PERM_BANS]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand348") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cgettempbans - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_DISPLAY_LIST_OF_TEMP_BANS]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand349") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::CLRPERMBAN)) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cclrpermbans - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_CLEAR_PERM_BANS_LWR]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand350") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::CLRTEMPBAN)) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cclrtempbans - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_CLEAR_TEMP_BANS_LWR]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand351") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::RANGE_BAN)) {
                    bFull = true;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%crangeban <%s> <%s> <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_FROMIP],
                        clsLanguageManager::mPtr->sTexts[LAN_TOIP], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_PERM_BAN_GIVEN_IP_RANGE]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand352") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cfullrangeban <%s> <%s> <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_FROMIP],
                        clsLanguageManager::mPtr->sTexts[LAN_TOIP], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_PERM_BAN_GIVEN_IP_RANGE]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand353") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::RANGE_TBAN)) {
                    bFull = true; bTemp = true;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%crangetempban <%s> <%s> <%s> <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        clsLanguageManager::mPtr->sTexts[LAN_FROMIP], clsLanguageManager::mPtr->sTexts[LAN_TOIP], clsLanguageManager::mPtr->sTexts[LAN_TIME_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR],
                        clsLanguageManager::mPtr->sTexts[LAN_TEMP_BAN_GIVEN_IP_RANGE]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand354") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cfullrangetempban <%s> <%s> <%s> <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        clsLanguageManager::mPtr->sTexts[LAN_FROMIP], clsLanguageManager::mPtr->sTexts[LAN_TOIP], clsLanguageManager::mPtr->sTexts[LAN_TIME_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR],
                        clsLanguageManager::mPtr->sTexts[LAN_TEMP_BAN_GIVEN_IP_RANGE]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand355") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::RANGE_UNBAN) || clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::RANGE_TUNBAN)) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%crangeunban <%s> <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_FROMIP],
                        clsLanguageManager::mPtr->sTexts[LAN_TOIP], clsLanguageManager::mPtr->sTexts[LAN_UNBAN_BANNED_IP_RANGE]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand356") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::RANGE_UNBAN)) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%crangepermunban <%s> <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_FROMIP],
                        clsLanguageManager::mPtr->sTexts[LAN_TOIP], clsLanguageManager::mPtr->sTexts[LAN_UNBAN_PERM_BANNED_IP_RANGE]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand357") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::RANGE_TUNBAN)) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%crangetempunban <%s> <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_FROMIP],
                        clsLanguageManager::mPtr->sTexts[LAN_TOIP], clsLanguageManager::mPtr->sTexts[LAN_UNBAN_TEMP_BANNED_IP_RANGE]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand358") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::GET_RANGE_BANS)) {
                    bFull = true;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cgetrangebans - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        clsLanguageManager::mPtr->sTexts[LAN_DISPLAY_LIST_OF_RANGE_BANS]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand359") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cgetrangepermbans - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        clsLanguageManager::mPtr->sTexts[LAN_DISPLAY_LIST_OF_RANGE_PERM_BANS]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand360") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cgetrangetempbans - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        clsLanguageManager::mPtr->sTexts[LAN_DISPLAY_LIST_OF_RANGE_TEMP_BANS]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand361") == false) {
                        return true;
                    }
                    help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::CLR_RANGE_BANS)) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cclrrangepermbans - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        clsLanguageManager::mPtr->sTexts[LAN_CLEAR_PERM_RANGE_BANS_LWR]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand362") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::CLR_RANGE_TBANS)) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cclrrangetempbans - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        clsLanguageManager::mPtr->sTexts[LAN_CLEAR_TEMP_RANGE_BANS_LWR]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand363") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::GETBANLIST)) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cchecknickban <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR],
                        clsLanguageManager::mPtr->sTexts[LAN_DISPLAY_BAN_FOUND_FOR_GIVEN_NICK]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand364") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%ccheckipban <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_IP],
                        clsLanguageManager::mPtr->sTexts[LAN_DISPLAY_BANS_FOUND_FOR_GIVEN_IP]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand365") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::GET_RANGE_BANS)) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%ccheckrangeban <%s> <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_FROMIP],
                        clsLanguageManager::mPtr->sTexts[LAN_TOIP], clsLanguageManager::mPtr->sTexts[LAN_DISPLAY_RANGE_BAN_FOR_GIVEN_RANGE]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand366") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::DROP)) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cdrop <%s> <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR],
                        clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_DISCONNECT_WITH_TEMPBAN]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand367") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::GETINFO)) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cgetinfo <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR],
                        clsLanguageManager::mPtr->sTexts[LAN_DISPLAY_INFO_GIVEN_NICK]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand368") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;

#if defined(_WITH_SQLITE) || defined(_WITH_POSTGRES) || defined(_WITH_MYSQL)
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cgetipinfo <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_IP],
                        clsLanguageManager::mPtr->sTexts[LAN_DISPLAY_INFO_GIVEN_IP]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand368-1") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
#endif
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::TEMPOP)) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cop <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR],
                        clsLanguageManager::mPtr->sTexts[LAN_GIVE_TEMP_OP]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand369") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::GAG)) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cgag <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR],
                        clsLanguageManager::mPtr->sTexts[LAN_DISALLOW_USER_TO_POST_IN_MAIN]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand370") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cungag <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR],
                        clsLanguageManager::mPtr->sTexts[LAN_USER_CAN_POST_IN_MAIN_AGAIN]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand371") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::RSTHUB)) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%crestart - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_RESTART_HUB_LWR]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand372") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::RSTSCRIPTS)) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cstartscript <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_FILENAME_LWR],
                        clsLanguageManager::mPtr->sTexts[LAN_START_SCRIPT_GIVEN_FILENAME]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand373") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cstopscript <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_FILENAME_LWR],
                        clsLanguageManager::mPtr->sTexts[LAN_STOP_SCRIPT_GIVEN_FILENAME]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand374") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%crestartscript <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_FILENAME_LWR],
                        clsLanguageManager::mPtr->sTexts[LAN_RESTART_SCRIPT_GIVEN_FILENAME]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand375") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%crestartscripts - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        clsLanguageManager::mPtr->sTexts[LAN_RESTART_SCRIPTING_PART_OF_THE_HUB]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand376") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cgetscripts - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_DISPLAY_LIST_OF_SCRIPTS]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand377") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::REFRESHTXT)) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%creloadtxt - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_RELOAD_ALL_TEXT_FILES]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand378") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::ADDREGUSER)) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%creguser <%s> <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR],
                        clsLanguageManager::mPtr->sTexts[LAN_PROFILENAME_LWR], clsLanguageManager::mPtr->sTexts[LAN_REG_USER_WITH_PROFILE]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand379-1") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%caddreguser <%s> <%s> <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR],
                        clsLanguageManager::mPtr->sTexts[LAN_PASSWORD_LWR], clsLanguageManager::mPtr->sTexts[LAN_PROFILENAME_LWR], clsLanguageManager::mPtr->sTexts[LAN_ADD_REG_USER_WITH_PROFILE]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand379-2") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::DELREGUSER)) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cdelreguser <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR],
                        clsLanguageManager::mPtr->sTexts[LAN_REMOVE_REG_USER]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand380") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::TOPIC) == true) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%ctopic <%s> - %s %ctopic <off> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        clsLanguageManager::mPtr->sTexts[LAN_NEW_TOPIC], clsLanguageManager::mPtr->sTexts[LAN_SET_NEW_TOPIC_OR], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        clsLanguageManager::mPtr->sTexts[LAN_CLEAR_TOPIC]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand381") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::MASSMSG) == true) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cmassmsg <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_MESSAGE_LWR],
                        clsLanguageManager::mPtr->sTexts[LAN_SEND_MSG_TO_ALL_USERS]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand382") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%copmassmsg <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_MESSAGE_LWR],
                        clsLanguageManager::mPtr->sTexts[LAN_SEND_MSG_TO_ALL_OPS]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand383") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(bFull == true) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "*** %s.\n", clsLanguageManager::mPtr->sTexts[LAN_REASON_IS_ALWAYS_OPTIONAL]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand384") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;

                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "*** %s.\n", clsLanguageManager::mPtr->sTexts[LAN_FULLBAN_HELP_TXT]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand385") == false) {
                        return true;
                    }
					help += clsServerManager::pGlobalBuffer;
                }

                if(bTemp == true) {
                    iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "*** %s: m = %s, h = %s, d = %s, w = %s, M = %s, Y = %s.\n", clsLanguageManager::mPtr->sTexts[LAN_TEMPBAN_TIME_VALUES],
                        clsLanguageManager::mPtr->sTexts[LAN_MINUTES_LWR], clsLanguageManager::mPtr->sTexts[LAN_HOURS_LWR], clsLanguageManager::mPtr->sTexts[LAN_DAYS_LWR], clsLanguageManager::mPtr->sTexts[LAN_WEEKS_LWR],
                        clsLanguageManager::mPtr->sTexts[LAN_MONTHS_LWR], clsLanguageManager::mPtr->sTexts[LAN_YEARS_LWR]);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand386") == false) {
                        return true;
                    }
                    help += clsServerManager::pGlobalBuffer;
                }

                iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\n%s:\n", clsLanguageManager::mPtr->sTexts[LAN_GLOBAL_COMMANDS]);
                if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand387") == false) {
                    return true;
                }
				help += clsServerManager::pGlobalBuffer;

                iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cme <%s> - %s.\n", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_MESSAGE_LWR],
                    clsLanguageManager::mPtr->sTexts[LAN_SPEAK_IN_3RD_PERSON]);
                if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand388") == false) {
                    return true;
                }
				help += clsServerManager::pGlobalBuffer;

                iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "\t%cmyip - %s.|", clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_SHOW_YOUR_IP]);
                if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand389") == false) {
                    return true;
                }
				help += clsServerManager::pGlobalBuffer;

				pUser->SendCharDelayed(help.c_str(), help.size());
                return true;
            }

            return false;

		case 's':
			//Hub-Commands: !stat !stats
			if((dlen == 4 && strncasecmp(sCommand+1, "tat", 3) == 0) || (dlen == 5 && strncasecmp(sCommand+1, "tats", 4) == 0)) {
				int iMsgLen = CheckFromPm(pUser, bFromPM);

				int iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "<%s>", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC]);
				iMsgLen += iRet;
				if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::DoCommand391") == false) {
					return true;
				}

				string Statinfo(clsServerManager::pGlobalBuffer, iMsgLen);

				Statinfo+="\n------------------------------------------------------------\n";
				Statinfo+="Current stats:\n";
				Statinfo+="------------------------------------------------------------\n";
				Statinfo+="Uptime: "+string(clsServerManager::ui64Days)+" days, "+string(clsServerManager::ui64Hours) + " hours, " + string(clsServerManager::ui64Mins) + " minutes\n";

                Statinfo+="Version: PtokaX DC Hub " PtokaXVersionString
#ifdef _WIN32
    #ifdef _M_X64
                " x64"
    #endif
#endif
#ifdef _PtokaX_TESTING_
                " [build " BUILD_NUMBER "]"
#endif
                " built on " __DATE__ " " __TIME__ "\n"
#if LUA_VERSION_NUM > 501
				"Lua: " LUA_VERSION_MAJOR "." LUA_VERSION_MINOR "." LUA_VERSION_RELEASE "\n";
#else
                LUA_RELEASE "\n";
#endif
                
#ifdef _WITH_SQLITE
				Statinfo+="SQLite: " SQLITE_VERSION "\n";
#elif _WITH_POSTGRES
				Statinfo+="PostgreSQL: "+string(PQlibVersion())+"\n";
#elif _WITH_MYSQL
				Statinfo+="MySQL: " MYSQL_SERVER_VERSION "\n";
#endif
#ifdef _WIN32
				Statinfo+="OS: "+clsServerManager::sOS+"\r\n";
#else

				struct utsname osname;
				if(uname(&osname) >= 0) {
					Statinfo+="OS: "+string(osname.sysname)+" "+string(osname.release)+" ("+string(osname.machine)+")"
#ifdef __clang__
						" / Clang " __clang_version__
#elif __GNUC__
						" / GCC " __VERSION__
#endif
						"\n";
				}
#endif
				Statinfo+="Users (Max/Actual Peak (Max Peak)/Logged): "+string(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_USERS])+" / "+string(clsServerManager::ui32Peak)+" ("+
					string(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_USERS_PEAK])+") / "+string(clsServerManager::ui32Logged)+ "\n";
                Statinfo+="Joins / Parts: "+string(clsServerManager::ui32Joins)+" / "+string(clsServerManager::ui32Parts)+"\n";
				Statinfo+="Users shared size: "+string(clsServerManager::ui64TotalShare)+" Bytes / "+string(formatBytes(clsServerManager::ui64TotalShare))+"\n";
				Statinfo+="Chat messages: "+string(clsDcCommands::mPtr->iStatChat)+" x\n";
				Statinfo+="Unknown commands: "+string(clsDcCommands::mPtr->iStatCmdUnknown)+" x\n";
				Statinfo+="PM commands: "+string(clsDcCommands::mPtr->iStatCmdTo)+" x\n";
				Statinfo+="Key commands: "+string(clsDcCommands::mPtr->iStatCmdKey)+" x\n";
				Statinfo+="Supports commands: "+string(clsDcCommands::mPtr->iStatCmdSupports)+" x\n";
				Statinfo+="MyINFO commands: "+string(clsDcCommands::mPtr->iStatCmdMyInfo)+" x\n";
				Statinfo+="ValidateNick commands: "+string(clsDcCommands::mPtr->iStatCmdValidate)+" x\n";
				Statinfo+="GetINFO commands: "+string(clsDcCommands::mPtr->iStatCmdGetInfo)+" x\n";
				Statinfo+="Password commands: "+string(clsDcCommands::mPtr->iStatCmdMyPass)+" x\n";
				Statinfo+="Version commands: "+string(clsDcCommands::mPtr->iStatCmdVersion)+" x\n";
				Statinfo+="GetNickList commands: "+string(clsDcCommands::mPtr->iStatCmdGetNickList)+" x\n";
				Statinfo+="Search commands: "+string(clsDcCommands::mPtr->iStatCmdSearch)+" x ("+string(clsDcCommands::mPtr->iStatCmdMultiSearch)+" x)\n";
				Statinfo+="SR commands: "+string(clsDcCommands::mPtr->iStatCmdSR)+" x\n";
				Statinfo+="CTM commands: "+string(clsDcCommands::mPtr->iStatCmdConnectToMe)+" x ("+string(clsDcCommands::mPtr->iStatCmdMultiConnectToMe)+" x)\n";
				Statinfo+="RevCTM commands: "+string(clsDcCommands::mPtr->iStatCmdRevCTM)+" x\n";
				Statinfo+="BotINFO commands: "+string(clsDcCommands::mPtr->iStatBotINFO)+" x\n";
				Statinfo+="Close commands: "+string(clsDcCommands::mPtr->iStatCmdClose)+" x\n";
				//Statinfo+="------------------------------------------------------------\n";
				//Statinfo+="ClientSocket Errors: "+string(iStatUserSocketErrors)+" x\n";
				Statinfo+="------------------------------------------------------------\n";
#ifdef _WIN32
				FILETIME tmpa, tmpb, kernelTimeFT, userTimeFT;
				GetProcessTimes(GetCurrentProcess(), &tmpa, &tmpb, &kernelTimeFT, &userTimeFT);
				int64_t kernelTime = kernelTimeFT.dwLowDateTime | (((int64_t)kernelTimeFT.dwHighDateTime) << 32);
				int64_t userTime = userTimeFT.dwLowDateTime | (((int64_t)userTimeFT.dwHighDateTime) << 32);
				int64_t icpuSec = (kernelTime + userTime) / (10000000I64);

				char cpuusage[32];
				int iLen = sprintf(cpuusage, "%.2f%%\r\n", clsServerManager::dCpuUsage/0.6);
				if(CheckSprintf(iLen, 32, "clsHubCommands::DoCommand3921") == false) {
					return true;
				}
				Statinfo+="CPU usage (60 sec avg): "+string(cpuusage, iLen);

				char cputime[64];
				iLen = sprintf(cputime, "%01I64d:%02d:%02d", icpuSec / (60*60), (int32_t)((icpuSec / 60) % 60), (int32_t)(icpuSec % 60));
				if(CheckSprintf(iLen, 64, "clsHubCommands::DoCommand392") == false) {
					return true;
				}
				Statinfo+="CPU time: "+string(cputime, iLen)+"\r\n";

				PROCESS_MEMORY_COUNTERS pmc;
				memset(&pmc, 0, sizeof(PROCESS_MEMORY_COUNTERS));
				pmc.cb = sizeof(pmc);

				::GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));

				Statinfo+="Mem usage (Peak): "+string(formatBytes(pmc.WorkingSetSize))+ " ("+string(formatBytes(pmc.PeakWorkingSetSize))+")\r\n";
				Statinfo+="VM size (Peak): "+string(formatBytes(pmc.PagefileUsage))+ " ("+string(formatBytes(pmc.PeakPagefileUsage))+")\r\n";
#else // _WIN32
				char cpuusage[32];
				int iLen = sprintf(cpuusage, "%.2f%%\n", (clsServerManager::dCpuUsage/0.6)/(double)clsServerManager::ui32CpuCount);
				if(CheckSprintf(iLen, 32, "clsHubCommands::DoCommand3921") == false) {
					return true;
				}
				Statinfo+="CPU usage (60 sec avg): "+string(cpuusage, iLen);

				struct rusage rs;

				getrusage(RUSAGE_SELF, &rs);

				uint64_t ui64cpuSec = uint64_t(rs.ru_utime.tv_sec) + (uint64_t(rs.ru_utime.tv_usec)/1000000) +
					uint64_t(rs.ru_stime.tv_sec) + (uint64_t(rs.ru_stime.tv_usec)/1000000);

				char cputime[64];
				iLen = sprintf(cputime, "%01" PRIu64 ":%02" PRIu64 ":%02" PRIu64, ui64cpuSec / (60*60),
					(ui64cpuSec / 60) % 60, ui64cpuSec % 60);
				if(CheckSprintf(iLen, 64, "clsHubCommands::DoCommand392") == false) {
					return true;
				}
				Statinfo+="CPU time: "+string(cputime, iLen)+"\n";

				FILE *fp = fopen("/proc/self/status", "r");
				if(fp != NULL) {
					string memrss, memhwm, memvms, memvmp, memstk, memlib;
					char buf[1024];
					char * tmp = NULL;

					while(fgets(buf, 1024, fp) != NULL) {
						if(strncmp(buf, "VmRSS:", 6) == 0) {
							tmp = buf+6;
							while(isspace(*tmp) && *tmp) {
								tmp++;
							}
							memrss = string(tmp, strlen(tmp)-1);
						} else if(strncmp(buf, "VmHWM:", 6) == 0) {
							tmp = buf+6;
							while(isspace(*tmp) && *tmp) {
								tmp++;
							}
							memhwm = string(tmp, strlen(tmp)-1);
						} else if(strncmp(buf, "VmSize:", 7) == 0) {
							tmp = buf+7;
							while(isspace(*tmp) && *tmp) {
								tmp++;
							}
							memvms = string(tmp, strlen(tmp)-1);
						} else if(strncmp(buf, "VmPeak:", 7) == 0) {
							tmp = buf+7;
							while(isspace(*tmp) && *tmp) {
								tmp++;
							}
							memvmp = string(tmp, strlen(tmp)-1);
						} else if(strncmp(buf, "VmStk:", 6) == 0) {
							tmp = buf+6;
							while(isspace(*tmp) && *tmp) {
								tmp++;
							}
							memstk = string(tmp, strlen(tmp)-1);
						} else if(strncmp(buf, "VmLib:", 6) == 0) {
							tmp = buf+6;
							while(isspace(*tmp) && *tmp) {
								tmp++;
							}
							memlib = string(tmp, strlen(tmp)-1);
						}
					}

					fclose(fp);

					if(memhwm.size() != 0 && memrss.size() != 0) {
						Statinfo+="Mem usage (Peak): "+memrss+ " ("+memhwm+")\n";
					} else if(memrss.size() != 0) {
						Statinfo+="Mem usage: "+memrss+"\n";
					}

					if(memvmp.size() != 0 && memvms.size() != 0) {
						Statinfo+="VM size (Peak): "+memvms+ " ("+memvmp+")\n";
					} else if(memrss.size() != 0) {
						Statinfo+="VM size: "+memvms+"\n";
					}

					if(memstk.size() != 0 && memlib.size() != 0) {
						Statinfo+="Stack size / Libs size: "+memstk+ " / "+memlib+"\n";
					}
				}
#endif
				Statinfo+="------------------------------------------------------------\n";
				Statinfo+="SendRests (Peak): "+string(clsServiceLoop::mPtr->ui32LastSendRest)+" ("+string(clsServiceLoop::mPtr->ui32SendRestsPeak)+")\n";
				Statinfo+="RecvRests (Peak): "+string(clsServiceLoop::mPtr->ui32LastRecvRest)+" ("+string(clsServiceLoop::mPtr->ui32RecvRestsPeak)+")\n";
				Statinfo+="Compression saved: "+string(formatBytes(clsServerManager::ui64BytesSentSaved))+" ("+string(clsDcCommands::mPtr->iStatZPipe)+")\n";
				Statinfo+="Data sent: "+string(formatBytes(clsServerManager::ui64BytesSent))+ "\n";
				Statinfo+="Data received: "+string(formatBytes(clsServerManager::ui64BytesRead))+ "\n";
				Statinfo+="Tx (60 sec avg): "+string(formatBytesPerSecond(clsServerManager::ui32ActualBytesSent))+" ("+string(formatBytesPerSecond(clsServerManager::ui32AverageBytesSent/60))+")\n";
				Statinfo+="Rx (60 sec avg): "+string(formatBytesPerSecond(clsServerManager::ui32ActualBytesRead))+" ("+string(formatBytesPerSecond(clsServerManager::ui32AverageBytesRead/60))+")|";
				pUser->SendCharDelayed(Statinfo.c_str(), Statinfo.size());
				return true;
			}

			// Hub commands: !stopscript scriptname
			if(strncasecmp(sCommand+1, "topscript ", 10) == 0) {
				if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::RSTSCRIPTS) == false) {
					SendNoPermission(pUser, bFromPM);
					return true;
				}

				if(clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
					pUser->SendFormatCheckPM("clsHubCommands::DoCommand->stopscript1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_ERR_SCRIPTS_DISABLED]);
					return true;
				}

				if(dlen < 12) {
					pUser->SendFormatCheckPM("clsHubCommands::DoCommand->stopscript2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cstopscript <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_SCRIPTNAME_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
					return true;
				}

				sCommand += 11;

                if(dlen-11 > 256) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->stopscript3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cstopscript <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_SCRIPTNAME_LWR], clsLanguageManager::mPtr->sTexts[LAN_MAX_ALWD_SCRIPT_NAME_LEN_256_CHARS]);
                    return true;
                }

				Script * curScript = clsScriptManager::mPtr->FindScript(sCommand);
				if(curScript == NULL || curScript->bEnabled == false || curScript->pLUA == NULL) {
					pUser->SendFormatCheckPM("clsHubCommands::DoCommand->stopscript4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_ERROR_SCRIPT], 
						sCommand, clsLanguageManager::mPtr->sTexts[LAN_NOT_RUNNING]);
					return true;
				}

				UncountDeflood(pUser, bFromPM);

				clsScriptManager::mPtr->StopScript(curScript, true);

				if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
					clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::DoCommand->stopscript", "<%s> *** %s %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_STOPPED_SCRIPT], sCommand);
				}

				if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
					pUser->SendFormatCheckPM("clsHubCommands::DoCommand->stopscript5", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SCRIPT], 
						sCommand, clsLanguageManager::mPtr->sTexts[LAN_STOPPED_LWR]);
				}

				return true;
			}

			// Hub commands: !startscript scriptname
			if(strncasecmp(sCommand+1, "tartscript ", 11) == 0) {
				if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::RSTSCRIPTS) == false) {
					SendNoPermission(pUser, bFromPM);
					return true;
				}

				if(clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
					pUser->SendFormatCheckPM("clsHubCommands::DoCommand->startscript1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_ERR_SCRIPTS_DISABLED]);
					return true;
				}

				if(dlen < 13) {
					pUser->SendFormatCheckPM("clsHubCommands::DoCommand->startscript2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cstartscript <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_SCRIPTNAME_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
					return true;
				}

				sCommand += 12;

                if(dlen-12 > 256) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->startscript3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cstartscript <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_SCRIPTNAME_LWR], clsLanguageManager::mPtr->sTexts[LAN_MAX_ALWD_SCRIPT_NAME_LEN_256_CHARS]);
                    return true;
                }

                char * sBadChar = strpbrk(sCommand, "/\\");
				if(sBadChar != NULL || FileExist((clsServerManager::sScriptPath+sCommand).c_str()) == false) {
					pUser->SendFormatCheckPM("clsHubCommands::DoCommand->startscript4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_ERROR_SCRIPT_NOT_EXIST]);
					return true;
				}

				Script * pScript = clsScriptManager::mPtr->FindScript(sCommand);
				if(pScript != NULL) {
					if(pScript->bEnabled == true && pScript->pLUA != NULL) {
						pUser->SendFormatCheckPM("clsHubCommands::DoCommand->startscript5", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
							clsLanguageManager::mPtr->sTexts[LAN_ERROR_SCRIPT_ALREDY_RUNNING]);
						return true;
					}

					UncountDeflood(pUser, bFromPM);

					if(clsScriptManager::mPtr->StartScript(pScript, true) == true) {
						if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
							clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::DoCommand->startscript1", "<%s> *** %s %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_STARTED_SCRIPT], sCommand);
						}

						if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
							pUser->SendFormatCheckPM("clsHubCommands::DoCommand->startscript6", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SCRIPT], 
								sCommand, clsLanguageManager::mPtr->sTexts[LAN_STARTED_LWR]);
						}
						return true;
					} else {
						pUser->SendFormatCheckPM("clsHubCommands::DoCommand->startscript7", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_ERROR_SCRIPT], 
							sCommand, clsLanguageManager::mPtr->sTexts[LAN_START_FAILED]);
						return true;
					}
				}

				UncountDeflood(pUser, bFromPM);

				if(clsScriptManager::mPtr->AddScript(sCommand, true, true) == true && clsScriptManager::mPtr->StartScript(clsScriptManager::mPtr->ppScriptTable[clsScriptManager::mPtr->ui8ScriptCount-1], false) == true) {
					if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
						clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::DoCommand->startscript2", "<%s> *** %s %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_STARTED_SCRIPT], sCommand);
					}

					if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
						pUser->SendFormatCheckPM("clsHubCommands::DoCommand->startscript8", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SCRIPT], 
							sCommand, clsLanguageManager::mPtr->sTexts[LAN_STARTED_LWR]);
					}
					return true;
				} else {
					pUser->SendFormatCheckPM("clsHubCommands::DoCommand->startscript9", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_ERROR_SCRIPT], 
						sCommand, clsLanguageManager::mPtr->sTexts[LAN_START_FAILED]);
					return true;
				}
			}

			return false;

		case 'p':
            //Hub-Commands: !passwd
			if(strncasecmp(sCommand+1, "asswd ", 6) == 0) {
                RegUser * pReg = clsRegManager::mPtr->Find(pUser);
                if(pUser->i32Profile == -1 || pReg == NULL) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->passwd1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_YOU_ARE_NOT_ALLOWED_TO_CHANGE_PASS]);
                    return true;
                }

                if(dlen < 8) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->passwd2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cpasswd <%s>. %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NEW_PASSWORD], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                if(dlen > 71) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->passwd3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_MAX_ALWD_PASS_LEN_64_CHARS]);
                    return true;
                }

            	if(strchr(sCommand+7, '|') != NULL) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->passwd4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_NO_PIPE_IN_PASS]);
                    return true;
                }

                size_t szPassLen = strlen(sCommand+7);

                if(pReg->UpdatePassword(sCommand+7, szPassLen) == false) {
                    return true;
                }

                clsRegManager::mPtr->Save(true);

#ifdef _BUILD_GUI
                if(clsRegisteredUsersDialog::mPtr != NULL) {
                    clsRegisteredUsersDialog::mPtr->RemoveReg(pReg);
                    clsRegisteredUsersDialog::mPtr->AddReg(pReg);
                }
#endif
                pUser->SendFormatCheckPM("clsHubCommands::DoCommand->passwd5", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOUR_PASSWORD_UPDATE_SUCCESS]);
                return true;
            }

            // Hub commands: !permunban
			if(strncasecmp(sCommand+1, "ermunban ", 9) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::UNBAN) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 11 || sCommand[10] == '\0') {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->permunban1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cpermunban <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_IP_OR_NICK], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                sCommand += 10;

                if(dlen > 100) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->permunban2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cpermunban <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_IP_OR_NICK], clsLanguageManager::mPtr->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
                    return true;
                }

                if(clsBanManager::mPtr->PermUnban(sCommand) == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->permunban3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SORRY], 
						sCommand, clsLanguageManager::mPtr->sTexts[LAN_IS_NOT_IN_BANS]);
                    return true;
                }

				UncountDeflood(pUser, bFromPM);

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
          	    	clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::DoCommand->permunban", "<%s> *** %s %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_REMOVED_LWR], sCommand, clsLanguageManager::mPtr->sTexts[LAN_FROM_BANS]);
                }

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->permunban4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sCommand, clsLanguageManager::mPtr->sTexts[LAN_REMOVED_FROM_BANS]);
                }
                return true;
            }

            return false;

        case 'f':
            // Hub commands: !fullban nick reason
			if(strncasecmp(sCommand+1, "ullban ", 7) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::BAN) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 9) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->fullban", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cfullban <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                return Ban(pUser, sCommand+8, bFromPM, true);
            }

            // Hub commands: !fullbanip ip reason
			if(strncasecmp(sCommand+1, "ullbanip ", 9) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::BAN) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 16) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->fullbanip", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cfullbanip <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_IP], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                return BanIp(pUser, sCommand+10, bFromPM, true);
            }

            // Hub commands: !fulltempban nick time reason ... PPK m = minutes, h = hours, d = day, w = weeks, M = months (30 day per month), Y = years (365 day per year)
			if(strncasecmp(sCommand+1, "ulltempban ", 11) == 0) {
            	if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::TEMP_BAN) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 16) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->fulltempban", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cfulltempban <%s> <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_TIME_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                return TempBan(pUser, sCommand+12, dlen-12, bFromPM, true);
            }

            // !fulltempbanip ip time reason ... PPK m = minutes, h = hours, d = day, w = weeks, M = months (30 day per month), Y = years (365 day per year)
			if(strncasecmp(sCommand+1, "ulltempbanip ", 13) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::TEMP_BAN) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 24) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->fulltempbanip", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cfulltempbanip <%s> <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_IP], clsLanguageManager::mPtr->sTexts[LAN_TIME_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                return TempBanIp(pUser, sCommand+14, dlen-14, bFromPM, true);
            }

            // Hub commands: !fullrangeban fromip toip reason
			if(strncasecmp(sCommand+1, "ullrangeban ", 12) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::RANGE_BAN) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 28) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->fullrangeban", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cfullrangeban <%s> <%s> <%s>. %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_FROMIP], clsLanguageManager::mPtr->sTexts[LAN_TOIP], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                return RangeBan(pUser, sCommand+13, dlen-13, bFromPM, true);
            }

            // Hub commands: !fullrangetempban fromip toip time reason
			if(strncasecmp(sCommand+1, "ullrangetempban ", 16) == 0) {
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::RANGE_TBAN) == false) {
                    SendNoPermission(pUser, bFromPM);
                    return true;
                }

                if(dlen < 35) {
                    pUser->SendFormatCheckPM("clsHubCommands::DoCommand->fullrangetempban", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cfullrangetempban <%s> <%s> <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
						clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], clsLanguageManager::mPtr->sTexts[LAN_FROMIP], clsLanguageManager::mPtr->sTexts[LAN_TOIP], clsLanguageManager::mPtr->sTexts[LAN_TIME_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], 
						clsLanguageManager::mPtr->sTexts[LAN_NO_PARAM_GIVEN]);
                    return true;
                }

                return RangeTempBan(pUser, sCommand+17, dlen-17, bFromPM, true);
            }
            
            return false;
    }

	return false; // PPK ... and send to all as chat ;)
}
//---------------------------------------------------------------------------

bool clsHubCommands::Ban(User * pUser, char * sCommand, bool bFromPM, bool bFull) {
    char * sReason = strchr(sCommand, ' ');
    if(sReason != NULL) {
        sReason[0] = '\0';
        if(sReason[1] == '\0') {
            sReason = NULL;
        } else {
            sReason++;

			if(strlen(sReason) > 512) {
				sReason[510] = '.';
				sReason[511] = '.';
				sReason[512] = '.';
				sReason[513] = '\0';
			}
        }
    }

    if(sCommand[0] == '\0') {
        pUser->SendFormatCheckPM("clsHubCommands::Ban1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %c%sban <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], 
			clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_BAD_PARAMS_GIVEN]);
        return true;
    }

    if(strlen(sCommand) > 100) {
        pUser->SendFormatCheckPM("clsHubCommands::Ban2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %c%sban <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], 
			clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
        return true;
    }

    // Self-ban ?
	if(strcasecmp(sCommand, pUser->sNick) == 0) {
        pUser->SendFormatCheckPM("clsHubCommands::Ban3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_CANT_BAN_YOURSELF]);
        return true;
    }

    User * pOtherUser = clsHashManager::mPtr->FindUser(sCommand, strlen(sCommand));
    if(pOtherUser != NULL) {
        // PPK don't ban user with higher profile
        if(pOtherUser->i32Profile != -1 && pUser->i32Profile > pOtherUser->i32Profile) {
            pUser->SendFormatCheckPM("clsHubCommands::Ban4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_ARE_NOT_ALWD_TO_BAN], sCommand);
            return true;
        }

        UncountDeflood(pUser, bFromPM);

        // Ban user
        clsBanManager::mPtr->Ban(pOtherUser, sReason, pUser->sNick, bFull);

        // Send user a message that he has been banned
        pOtherUser->SendFormat("clsHubCommands::Ban", false, "<%s> %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_HAD_BEEN_BANNED_BCS], sReason == NULL ? clsLanguageManager::mPtr->sTexts[LAN_NO_REASON_SPECIFIED] : sReason);

        // Send message to all OPs that the user have been banned
        if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
            clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::Ban", "<%s> *** %s %s %s %s %s%s %s %s %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sCommand, clsLanguageManager::mPtr->sTexts[LAN_WITH_IP], pOtherUser->sIP, clsLanguageManager::mPtr->sTexts[LAN_HAS_BEEN], 
				bFull == true ? clsLanguageManager::mPtr->sTexts[LAN_FULL_LWR] : "", clsLanguageManager::mPtr->sTexts[LAN_BANNED_LWR], clsLanguageManager::mPtr->sTexts[LAN_BY_LWR], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_BECAUSE_LWR], sReason == NULL ? clsLanguageManager::mPtr->sTexts[LAN_NO_REASON_SPECIFIED] : sReason);
        }

        if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
            pUser->SendFormatCheckPM("clsHubCommands::Ban5", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s %s %s%s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sCommand, clsLanguageManager::mPtr->sTexts[LAN_WITH_IP], 
				pOtherUser->sIP, clsLanguageManager::mPtr->sTexts[LAN_HAS_BEEN], bFull == true ? clsLanguageManager::mPtr->sTexts[LAN_FULL_LWR] : "", clsLanguageManager::mPtr->sTexts[LAN_BANNED_LWR]);
        }

        // Finish him !
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] User %s (%s) %sbanned by %s", pOtherUser->sNick, pOtherUser->sIP, bFull == true ? clsLanguageManager::mPtr->sTexts[LAN_FULL_LWR] : "", pUser->sNick);

        pOtherUser->Close();
        return true;
    } else if(bFull == true) {
        pUser->SendFormatCheckPM("clsHubCommands::Ban6", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_ERROR], sCommand, 
			clsLanguageManager::mPtr->sTexts[LAN_IS_NOT_ONLINE]);
        return true;
    } else {
        return NickBan(pUser, sCommand, sReason, bFromPM);
    }
}
//---------------------------------------------------------------------------

bool clsHubCommands::BanIp(User * pUser, char * sCommand, bool bFromPM, bool bFull) {
    char * sReason = strchr(sCommand, ' ');
    if(sReason != NULL) {
        sReason[0] = '\0';
        if(sReason[1] == '\0') {
            sReason = NULL;
        } else {
            sReason++;

			if(strlen(sReason) > 512) {
				sReason[510] = '.';
				sReason[511] = '.';
				sReason[512] = '.';
				sReason[513] = '\0';
			}
        }
    }

    if(sCommand[0] == '\0') {
        pUser->SendFormatCheckPM("clsHubCommands::BanIp1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %c%sbanip <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], 
			clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", clsLanguageManager::mPtr->sTexts[LAN_IP], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_BAD_PARAMS_GIVEN]);
        return true;
    }

    switch(clsBanManager::mPtr->BanIp(NULL, sCommand, sReason, pUser->sNick, bFull)) {
        case 0: {
			UncountDeflood(pUser, bFromPM);

			uint8_t ui128Hash[16];
			memset(ui128Hash, 0, 16);

            HashIP(sCommand, ui128Hash);
          
            User * pCur = NULL,
                * pNext = clsHashManager::mPtr->FindUser(ui128Hash);

            while(pNext != NULL) {
            	pCur = pNext;
                pNext = pCur->pHashIpTableNext;

                if(pCur == pUser || (pCur->i32Profile != -1 && clsProfileManager::mPtr->IsAllowed(pCur, clsProfileManager::ENTERIFIPBAN) == true)) {
                    continue;
                }

                // PPK don't nickban user with higher profile
                if(pCur->i32Profile != -1 && pUser->i32Profile > pCur->i32Profile) {
                    pUser->SendFormatCheckPM("clsHubCommands::BanIp2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_NOT_ALLOWED_TO], 
						clsLanguageManager::mPtr->sTexts[LAN_BAN_LWR], pCur->sNick);
                    continue;
                }

                pCur->SendFormat("clsHubCommands::BanIp", false, "<%s> %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_HAD_BEEN_BANNED_BCS], sReason == NULL ? clsLanguageManager::mPtr->sTexts[LAN_NO_REASON_SPECIFIED] : sReason);

				clsUdpDebug::mPtr->BroadcastFormat("[SYS] User %s (%s) ipbanned by %s", pCur->sNick, pCur->sIP, pUser->sNick);

                pCur->Close();
            }

            if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::BanIp", "<%s> *** %s %s %s%s %s %s %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sCommand, clsLanguageManager::mPtr->sTexts[LAN_IS_LWR], bFull == true ? clsLanguageManager::mPtr->sTexts[LAN_FULL_LWR] : "", 
					clsLanguageManager::mPtr->sTexts[LAN_BANNED_LWR], clsLanguageManager::mPtr->sTexts[LAN_BY_LWR], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_BECAUSE_LWR], sReason == NULL ? clsLanguageManager::mPtr->sTexts[LAN_NO_REASON_SPECIFIED] : sReason);
            }

            if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                pUser->SendFormatCheckPM("clsHubCommands::BanIp3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s%s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sCommand, clsLanguageManager::mPtr->sTexts[LAN_IS_LWR], 
				bFull == true ? clsLanguageManager::mPtr->sTexts[LAN_FULL_LWR] : "", clsLanguageManager::mPtr->sTexts[LAN_BANNED_LWR]);
            }
            return true;
        }
        case 1: {
            pUser->SendFormatCheckPM("clsHubCommands::BanIp4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %c%sbanip <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], 
				clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", clsLanguageManager::mPtr->sTexts[LAN_IP], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_VALID_IP_SPECIFIED]);
            return true;
        }
        case 2: {
            pUser->SendFormatCheckPM("clsHubCommands::BanIp5", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s %s%s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_IP], sCommand, 
				clsLanguageManager::mPtr->sTexts[LAN_IS_ALREADY], bFull == true ? clsLanguageManager::mPtr->sTexts[LAN_FULL_LWR] : "", clsLanguageManager::mPtr->sTexts[LAN_BANNED_LWR]);
            return true;
        }
        default:
            return true;
    }
}
//---------------------------------------------------------------------------

bool clsHubCommands::NickBan(User * pUser, char * sNick, char * sReason, bool bFromPM) {
    RegUser * pReg = clsRegManager::mPtr->Find(sNick, strlen(sNick));

    // don't nickban user with higher profile
    if(pReg != NULL && pUser->i32Profile > pReg->ui16Profile) {
        pUser->SendFormatCheckPM("clsHubCommands::NickBan1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_NOT_ALLOWED_TO], 
			clsLanguageManager::mPtr->sTexts[LAN_BAN_LWR], sNick);
        return true;
    }

    if(clsBanManager::mPtr->NickBan(NULL, sNick, sReason, pUser->sNick) == true) {
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] User %s nickbanned by %s", sNick, pUser->sNick);
    } else {
		pUser->SendFormatCheckPM("clsHubCommands::NickBan2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_NICK], sNick, 
			clsLanguageManager::mPtr->sTexts[LAN_IS_ALREDY_BANNED]);
        return true;
    }

    UncountDeflood(pUser, bFromPM);

    if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::NickBan", "<%s> *** %s %s %s %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sNick, clsLanguageManager::mPtr->sTexts[LAN_HAS_BEEN_BANNED_BY], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_BECAUSE_LWR], 
			sReason == NULL ? clsLanguageManager::mPtr->sTexts[LAN_NO_REASON_SPECIFIED] : sReason);
    }

    if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        pUser->SendFormatCheckPM("clsHubCommands::NickBan3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sNick, clsLanguageManager::mPtr->sTexts[LAN_ADDED_TO_BANS]);
    }

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool clsHubCommands::TempBan(User * pUser, char * sCommand, const size_t &dlen, bool bFromPM, bool bFull) {
    char *sCmdParts[] = { NULL, NULL, NULL };
    uint16_t iCmdPartsLen[] = { 0, 0, 0 };

    uint8_t cPart = 0;

    sCmdParts[cPart] = sCommand; // nick start

    for(size_t szi = 0; szi < dlen; szi++) {
        if(sCommand[szi] == ' ') {
            sCommand[szi] = '\0';
            iCmdPartsLen[cPart] = (uint16_t)((sCommand+szi)-sCmdParts[cPart]);

            // are we on last space ???
            if(cPart == 1) {
                sCmdParts[2] = sCommand+szi+1;
                iCmdPartsLen[2] = (uint16_t)(dlen-szi-1);
                break;
            }

            cPart++;
            sCmdParts[cPart] = sCommand+szi+1;
        }
    }

    if(sCmdParts[2] == NULL && iCmdPartsLen[1] == 0 && sCmdParts[1] != NULL) {
        iCmdPartsLen[1] = (uint16_t)(dlen-(sCmdParts[1]-sCommand));
    }

    if(sCmdParts[2] != NULL && iCmdPartsLen[2] == 0) {
        sCmdParts[2] = NULL;
    }

	if(iCmdPartsLen[2] > 512) {
		sCmdParts[2][510] = '.';
		sCmdParts[2][511] = '.';
		sCmdParts[2][512] = '.';
		sCmdParts[2][513] = '\0';
	}

    if(iCmdPartsLen[0] == 0 || iCmdPartsLen[1] == 0) {
        pUser->SendFormatCheckPM("clsHubCommands::TempBan1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %c%stempban <%s> <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], 
			clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_TIME_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_BAD_PARAMS_GIVEN]);
        return true;
    }

    if(iCmdPartsLen[0] > 100) {
        pUser->SendFormatCheckPM("clsHubCommands::TempBan2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %c%stempban <%s> <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], 
			clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_TIME_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
        return true;
    }

    // Self-ban ?
	if(strcasecmp(sCmdParts[0], pUser->sNick) == 0) {
        pUser->SendFormatCheckPM("clsHubCommands::TempBan3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_CANT_BAN_YOURSELF]);
        return true;
    }
           	    
    User * pOtherUser = clsHashManager::mPtr->FindUser(sCmdParts[0], iCmdPartsLen[0]);
    if(pOtherUser != NULL) {
        // PPK don't tempban user with higher profile
        if(pOtherUser->i32Profile != -1 && pUser->i32Profile > pOtherUser->i32Profile) {
            pUser->SendFormatCheckPM("clsHubCommands::TempBan4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_ARE_NOT_ALWD_TO_TEMPBAN], sCmdParts[0]);
            return true;
        }

        char cTime = sCmdParts[1][iCmdPartsLen[1]-1];
        sCmdParts[1][iCmdPartsLen[1]-1] = '\0';
        int iTime = atoi(sCmdParts[1]);
        time_t acc_time, ban_time;

        if(iTime <= 0 || GenerateTempBanTime(cTime, (uint32_t)iTime, acc_time, ban_time) == false) {
            pUser->SendFormatCheckPM("clsHubCommands::TempBan5", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %c%stempban <%s> <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], 
				clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_TIME_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_BAD_TIME_SPECIFIED]);
            return true;
        }

        clsBanManager::mPtr->TempBan(pOtherUser, sCmdParts[2], pUser->sNick, 0, ban_time, bFull);
        UncountDeflood(pUser, bFromPM);
        static char sTime[256];
        strcpy(sTime, formatTime((ban_time-acc_time)/60));

        // Send user a message that he has been tempbanned
        pOtherUser->SendFormat("clsHubCommands::TempBan", false, "<%s> %s: %s %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_HAD_BEEN_TEMP_BANNED_TO], sTime, clsLanguageManager::mPtr->sTexts[LAN_BECAUSE_LWR], 
			sCmdParts[2] == NULL ? clsLanguageManager::mPtr->sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);

        if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
            clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::TempBan", "<%s> *** %s %s %s %s %s%s %s %s %s: %s %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sCmdParts[0], clsLanguageManager::mPtr->sTexts[LAN_WITH_IP], pOtherUser->sIP, clsLanguageManager::mPtr->sTexts[LAN_HAS_BEEN], 
				bFull == true ? clsLanguageManager::mPtr->sTexts[LAN_FULL_LWR] : "", clsLanguageManager::mPtr->sTexts[LAN_TEMP_BANNED], clsLanguageManager::mPtr->sTexts[LAN_BY_LWR], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_TO_LWR], sTime, clsLanguageManager::mPtr->sTexts[LAN_BECAUSE_LWR], 
				sCmdParts[2] == NULL ? clsLanguageManager::mPtr->sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);
        }

        if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
            pUser->SendFormatCheckPM("clsHubCommands::TempBan6", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s %s %s%s %s: %s %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sCmdParts[0], clsLanguageManager::mPtr->sTexts[LAN_WITH_IP], 
				pOtherUser->sIP, clsLanguageManager::mPtr->sTexts[LAN_HAS_BEEN], bFull == true ? clsLanguageManager::mPtr->sTexts[LAN_FULL_LWR] : "", clsLanguageManager::mPtr->sTexts[LAN_TEMP_BANNED], clsLanguageManager::mPtr->sTexts[LAN_TO_LWR], sTime, clsLanguageManager::mPtr->sTexts[LAN_BECAUSE_LWR], 
				sCmdParts[2] == NULL ? clsLanguageManager::mPtr->sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);
        }

        // Finish him !
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] User %s (%s) %stemp banned by %s", pOtherUser->sNick, pOtherUser->sIP, bFull == true ? "full " : "", pUser->sNick);

        pOtherUser->Close();
        return true;
    } else if(bFull == true) {
        pUser->SendFormatCheckPM("clsHubCommands::TempBan7", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_ERROR], sCmdParts[0], 
			clsLanguageManager::mPtr->sTexts[LAN_IS_NOT_ONLINE]);
        return true;
    } else {
        return TempNickBan(pUser, sCmdParts[0], sCmdParts[1], iCmdPartsLen[1], sCmdParts[2], bFromPM, true);
    }
}
//---------------------------------------------------------------------------

bool clsHubCommands::TempBanIp(User * pUser, char * sCommand, const size_t &dlen, bool bFromPM, bool bFull) {
    char *sCmdParts[] = { NULL, NULL, NULL };
    uint16_t iCmdPartsLen[] = { 0, 0, 0 };

    uint8_t cPart = 0;

    sCmdParts[cPart] = sCommand; // nick start

    for(size_t szi = 0; szi < dlen; szi++) {
        if(sCommand[szi] == ' ') {
            sCommand[szi] = '\0';
            iCmdPartsLen[cPart] = (uint16_t)((sCommand+szi)-sCmdParts[cPart]);

            // are we on last space ???
            if(cPart == 1) {
                sCmdParts[2] = sCommand+szi+1;
                iCmdPartsLen[2] = (uint16_t)(dlen-szi-1);
                break;
            }

            cPart++;
            sCmdParts[cPart] = sCommand+szi+1;
        }
    }

    if(sCmdParts[2] == NULL && iCmdPartsLen[1] == 0 && sCmdParts[1] != NULL) {
        iCmdPartsLen[1] = (uint16_t)(dlen-(sCmdParts[1]-sCommand));
    }

    if(sCmdParts[2] != NULL && iCmdPartsLen[2] == 0) {
        sCmdParts[2] = NULL;
    }

	if(iCmdPartsLen[2] > 512) {
		sCmdParts[2][510] = '.';
		sCmdParts[2][511] = '.';
		sCmdParts[2][512] = '.';
		sCmdParts[2][513] = '\0';
	}

    if(iCmdPartsLen[0] == 0 || iCmdPartsLen[1] == 0) {
        pUser->SendFormatCheckPM("clsHubCommands::TempBanIp1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %c%stempbanip <%s> <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], 
			clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", clsLanguageManager::mPtr->sTexts[LAN_IP], clsLanguageManager::mPtr->sTexts[LAN_TIME_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_BAD_PARAMS_GIVEN]);
        return true;
    }

    char cTime = sCmdParts[1][iCmdPartsLen[1]-1];
    sCmdParts[1][iCmdPartsLen[1]-1] = '\0';
    int iTime = atoi(sCmdParts[1]);
    time_t acc_time, ban_time;

    if(iTime <= 0 || GenerateTempBanTime(cTime, (uint32_t)iTime, acc_time, ban_time) == false) {
        pUser->SendFormatCheckPM("clsHubCommands::TempBanIp2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %c%stempbanip <%s> <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], 
			clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", clsLanguageManager::mPtr->sTexts[LAN_IP], clsLanguageManager::mPtr->sTexts[LAN_TIME_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_BAD_TIME_SPECIFIED]);
        return true;
    }

    switch(clsBanManager::mPtr->TempBanIp(NULL, sCmdParts[0], sCmdParts[2], pUser->sNick, 0, ban_time, bFull)) {
        case 0: {
			uint8_t ui128Hash[16];
			memset(ui128Hash, 0, 16);

            HashIP(sCommand, ui128Hash);
          
            User * pCur = NULL,
                * pNext = clsHashManager::mPtr->FindUser(ui128Hash);
            while(pNext != NULL) {
            	pCur = pNext;
                pNext = pCur->pHashIpTableNext;

                if(pCur == pUser || (pCur->i32Profile != -1 && clsProfileManager::mPtr->IsAllowed(pCur, clsProfileManager::ENTERIFIPBAN) == true)) {
                    continue;
                }

                // PPK don't nickban user with higher profile
                if(pCur->i32Profile != -1 && pUser->i32Profile > pCur->i32Profile) {
                    pUser->SendFormatCheckPM("clsHubCommands::TempBanIp3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_NOT_ALLOWED_TO], 
						clsLanguageManager::mPtr->sTexts[LAN_BAN_LWR], pCur->sNick);
                    continue;
                }

                pCur->SendFormat("clsHubCommands::TempBanIp", false, "<%s> %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_HAD_BEEN_BANNED_BCS], sCmdParts[2] == NULL ? clsLanguageManager::mPtr->sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);

				clsUdpDebug::mPtr->BroadcastFormat("[SYS] User %s (%s) tempipbanned by %s", pCur->sNick, pCur->sIP, pUser->sNick);

                pCur->Close();
            }
            break;
        }
        case 1: {
            pUser->SendFormatCheckPM("clsHubCommands::TempBanIp4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %c%sbanip <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], 
				clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", clsLanguageManager::mPtr->sTexts[LAN_IP], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_NO_VALID_IP_SPECIFIED]);
            return true;
        }
        case 2: {
            pUser->SendFormatCheckPM("clsHubCommands::TempBanIp5", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s %s%s %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_IP], sCmdParts[0], 
				clsLanguageManager::mPtr->sTexts[LAN_IS_ALREADY], bFull == true ? clsLanguageManager::mPtr->sTexts[LAN_FULL_LWR] : "", clsLanguageManager::mPtr->sTexts[LAN_BANNED_LWR], clsLanguageManager::mPtr->sTexts[LAN_TO_LONGER_TIME]);
            return true;
        }
        default:
            return true;
    }

	UncountDeflood(pUser, bFromPM);
    static char sTime[256];
    strcpy(sTime, formatTime((ban_time-acc_time)/60));

    if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::TempBanIp", "<%s> *** %s %s %s%s %s %s %s: %s %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sCommand, clsLanguageManager::mPtr->sTexts[LAN_HAS_BEEN], bFull == true ? clsLanguageManager::mPtr->sTexts[LAN_FULL_LWR] : "", 
			clsLanguageManager::mPtr->sTexts[LAN_TEMP_BANNED], clsLanguageManager::mPtr->sTexts[LAN_BY_LWR], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_TO_LWR], sTime, clsLanguageManager::mPtr->sTexts[LAN_BECAUSE_LWR], sCmdParts[2] == NULL ? clsLanguageManager::mPtr->sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);
    }

    if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        pUser->SendFormatCheckPM("clsHubCommands::TempBanIp6", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s%s %s: %s %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sCmdParts[0], clsLanguageManager::mPtr->sTexts[LAN_HAS_BEEN], 
			bFull == true ? clsLanguageManager::mPtr->sTexts[LAN_FULL_LWR] : "", clsLanguageManager::mPtr->sTexts[LAN_TEMP_BANNED], clsLanguageManager::mPtr->sTexts[LAN_TO_LWR], sTime, clsLanguageManager::mPtr->sTexts[LAN_BECAUSE_LWR], sCmdParts[2] == NULL ? clsLanguageManager::mPtr->sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);
    }

	clsUdpDebug::mPtr->BroadcastFormat("[SYS] IP %s %stemp banned by %s", sCmdParts[0], bFull == true ? "full " : "", pUser->sNick);

    return true;
}
//---------------------------------------------------------------------------

bool clsHubCommands::TempNickBan(User * pUser, char * sNick, char * sTime, const size_t &szTimeLen, char * sReason, bool bFromPM, bool bNotNickBan/* = false*/) {
    RegUser * pReg = clsRegManager::mPtr->Find(sNick, strlen(sNick));

    // don't nickban user with higher profile
    if(pReg != NULL && pUser->i32Profile > pReg->ui16Profile) {
        pUser->SendFormatCheckPM("clsHubCommands::TempNickBan1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_NOT_ALLOWED_TO], 
			clsLanguageManager::mPtr->sTexts[LAN_BAN_LWR], sNick);
        return true;
    }

    char cTime = sTime[szTimeLen-1];
    sTime[szTimeLen-1] = '\0';
    int iTime = atoi(sTime);
    time_t acc_time, ban_time;

    if(iTime <= 0 || GenerateTempBanTime(cTime, (uint32_t)iTime, acc_time, ban_time) == false) {
		pUser->SendFormatCheckPM("clsHubCommands::TempNickBan2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %c%stempban <%s> <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], 
			clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bNotNickBan == false ? "nick" : "", clsLanguageManager::mPtr->sTexts[LAN_NICK_LWR], clsLanguageManager::mPtr->sTexts[LAN_TIME_LWR], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_BAD_TIME_SPECIFIED]);
        return true;
    }

    if(clsBanManager::mPtr->NickTempBan(NULL, sNick, sReason, pUser->sNick, 0, ban_time) == false) {
        pUser->SendFormatCheckPM("clsHubCommands::TempNickBan3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_NICK], sNick, 
			clsLanguageManager::mPtr->sTexts[LAN_IS_ALRD_BANNED_LONGER_TIME]);
        return true;
    }

	UncountDeflood(pUser, bFromPM);

    char sBanTime[256];
    strcpy(sBanTime, formatTime((ban_time-acc_time)/60));

    if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::TempNickBan", "<%s> *** %s %s %s %s: %s %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sNick, clsLanguageManager::mPtr->sTexts[LAN_HAS_BEEN_TMPBND_BY], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_TO_LWR], 
			sBanTime, clsLanguageManager::mPtr->sTexts[LAN_BECAUSE_LWR], sReason == NULL ? clsLanguageManager::mPtr->sTexts[LAN_NO_REASON_SPECIFIED] : sReason);
    }

    if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        pUser->SendFormatCheckPM("clsHubCommands::TempNickBan4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s: %s %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sNick, clsLanguageManager::mPtr->sTexts[LAN_BEEN_TEMP_BANNED_TO], 
			sBanTime, clsLanguageManager::mPtr->sTexts[LAN_BECAUSE_LWR], sReason == NULL ? clsLanguageManager::mPtr->sTexts[LAN_NO_REASON_SPECIFIED] : sReason);
    }

    clsUdpDebug::mPtr->BroadcastFormat("[SYS] Nick %s tempbanned by %s", sNick, pUser->sNick);
    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool clsHubCommands::RangeBan(User * pUser, char * sCommand, const size_t &dlen, bool bFromPM, bool bFull) {
    char *sCmdParts[] = { NULL, NULL, NULL };
    uint16_t iCmdPartsLen[] = { 0, 0, 0 };

    uint8_t cPart = 0;

    sCmdParts[cPart] = sCommand; // nick start

    for(size_t szi = 0; szi < dlen; szi++) {
        if(sCommand[szi] == ' ') {
            sCommand[szi] = '\0';
            iCmdPartsLen[cPart] = (uint16_t)((sCommand+szi)-sCmdParts[cPart]);

            // are we on last space ???
            if(cPart == 1) {
                sCmdParts[2] = sCommand+szi+1;
                iCmdPartsLen[2] = (uint16_t)(dlen-szi-1);
                break;
            }

            cPart++;
            sCmdParts[cPart] = sCommand+szi+1;
        }
    }

    if(sCmdParts[2] == NULL && iCmdPartsLen[1] == 0 && sCmdParts[1] != NULL) {
        iCmdPartsLen[1] = (uint16_t)(dlen-(sCmdParts[1]-sCommand));
    }

    if(sCmdParts[2] != NULL && iCmdPartsLen[2] == 0) {
        sCmdParts[2] = NULL;
    }

	if(iCmdPartsLen[2] > 512) {
		sCmdParts[2][510] = '.';
		sCmdParts[2][511] = '.';
		sCmdParts[2][512] = '.';
		sCmdParts[2][513] = '\0';
	}

    if(iCmdPartsLen[0] > 39 || iCmdPartsLen[1] > 39) {
        pUser->SendFormatCheckPM("clsHubCommands::RangeBan1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %c%srangeban <%s> <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], 
			clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", clsLanguageManager::mPtr->sTexts[LAN_FROMIP], clsLanguageManager::mPtr->sTexts[LAN_TOIP], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_MAX_ALWD_IP_LEN_39_CHARS]);
        return true;
    }

	uint8_t ui128FromHash[16], ui128ToHash[16];
	memset(ui128FromHash, 0, 16);
	memset(ui128ToHash, 0, 16);

    if(iCmdPartsLen[0] == 0 || iCmdPartsLen[1] == 0 || HashIP(sCmdParts[0], ui128FromHash) == false || HashIP(sCmdParts[1], ui128ToHash) == false) {
        pUser->SendFormatCheckPM("clsHubCommands::RangeBan2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %c%srangeban <%s> <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], 
			clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", clsLanguageManager::mPtr->sTexts[LAN_FROMIP], clsLanguageManager::mPtr->sTexts[LAN_TOIP], clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_BAD_PARAMS_GIVEN]);
        return true;
    }

    if(memcmp(ui128ToHash, ui128FromHash, 16) <= 0) {
        pUser->SendFormatCheckPM("clsHubCommands::RangeBan3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s %s %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_ERROR_FROM], 
			sCmdParts[0], clsLanguageManager::mPtr->sTexts[LAN_TO_LWR], sCmdParts[1], clsLanguageManager::mPtr->sTexts[LAN_IS_NOT_VALID_RANGE]);
        return true;
    }

    if(clsBanManager::mPtr->RangeBan(sCmdParts[0], ui128FromHash, sCmdParts[1], ui128ToHash, sCmdParts[2], pUser->sNick, bFull) == false) {
        pUser->SendFormatCheckPM("clsHubCommands::RangeBan4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s-%s %s %s%s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_RANGE], sCmdParts[0], 
			sCmdParts[1], clsLanguageManager::mPtr->sTexts[LAN_IS_ALREADY], bFull == true ? clsLanguageManager::mPtr->sTexts[LAN_FULL_LWR] : "", clsLanguageManager::mPtr->sTexts[LAN_BANNED_LWR]);
        return true;
    }

	UncountDeflood(pUser, bFromPM);

    if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::RangeBan", "<%s> *** %s %s-%s %s %s%s %s %s %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_RANGE], sCmdParts[0], sCmdParts[1], clsLanguageManager::mPtr->sTexts[LAN_IS_LWR], 
			bFull == true ? clsLanguageManager::mPtr->sTexts[LAN_FULL_LWR] : "", clsLanguageManager::mPtr->sTexts[LAN_BANNED_LWR], clsLanguageManager::mPtr->sTexts[LAN_BY_LWR], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_BECAUSE_LWR], sCmdParts[2] == NULL ? clsLanguageManager::mPtr->sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);
    }

    if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        pUser->SendFormatCheckPM("clsHubCommands::RangeBan5", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s-%s %s %s%s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_RANGE], sCmdParts[0], 
			sCmdParts[1], clsLanguageManager::mPtr->sTexts[LAN_IS_LWR], bFull == true ? clsLanguageManager::mPtr->sTexts[LAN_FULL_LWR] : "", clsLanguageManager::mPtr->sTexts[LAN_BANNED_LWR]);
    }

    return true;
}
//---------------------------------------------------------------------------

bool clsHubCommands::RangeTempBan(User * pUser, char * sCommand, const size_t &dlen, bool bFromPM, bool bFull) {
    char *sCmdParts[] = { NULL, NULL, NULL, NULL };
    uint16_t iCmdPartsLen[] = { 0, 0, 0, 0 };

    uint8_t cPart = 0;

    sCmdParts[cPart] = sCommand; // nick start

    for(size_t szi = 0; szi < dlen; szi++) {
        if(sCommand[szi] == ' ') {
            sCommand[szi] = '\0';
            iCmdPartsLen[cPart] = (uint16_t)((sCommand+szi)-sCmdParts[cPart]);

            // are we on last space ???
            if(cPart == 2) {
                sCmdParts[3] = sCommand+szi+1;
                iCmdPartsLen[3] = (uint16_t)(dlen-szi-1);
                break;
            }

            cPart++;
            sCmdParts[cPart] = sCommand+szi+1;
        }
    }

    if(sCmdParts[3] == NULL && iCmdPartsLen[2] == 0 && sCmdParts[2] != NULL) {
        iCmdPartsLen[2] = (uint16_t)(dlen-(sCmdParts[2]-sCommand));
    }

    if(sCmdParts[3] != NULL && iCmdPartsLen[3] == 0) {
        sCmdParts[3] = NULL;
    }

	if(iCmdPartsLen[3] > 512) {
		sCmdParts[3][510] = '.';
		sCmdParts[3][511] = '.';
		sCmdParts[3][512] = '.';
		sCmdParts[3][513] = '\0';
	}

    if(iCmdPartsLen[0] > 39 || iCmdPartsLen[1] > 39) {
        pUser->SendFormatCheckPM("clsHubCommands::RangeTempBan1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %c%srangetempban <%s> <%s> <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
			clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", clsLanguageManager::mPtr->sTexts[LAN_FROMIP], clsLanguageManager::mPtr->sTexts[LAN_TOIP], clsLanguageManager::mPtr->sTexts[LAN_TIME_LWR], 
			clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_MAX_ALWD_IP_LEN_39_CHARS]);
        return true;
    }

	uint8_t ui128FromHash[16], ui128ToHash[16];
	memset(ui128FromHash, 0, 16);
	memset(ui128ToHash, 0, 16);

    if(iCmdPartsLen[0] == 0 || iCmdPartsLen[1] == 0 || iCmdPartsLen[2] == 0 || HashIP(sCmdParts[0], ui128FromHash) == false || HashIP(sCmdParts[1], ui128ToHash) == false) {
        pUser->SendFormatCheckPM("clsHubCommands::RangeTempBan2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %c%srangetempban <%s> <%s> <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
			clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", clsLanguageManager::mPtr->sTexts[LAN_FROMIP], clsLanguageManager::mPtr->sTexts[LAN_TOIP], clsLanguageManager::mPtr->sTexts[LAN_TIME_LWR], 
			clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_BAD_PARAMS_GIVEN]);
        return true;
    }

    if(memcmp(ui128ToHash, ui128FromHash, 16) <= 0) {
        pUser->SendFormatCheckPM("clsHubCommands::RangeTempBan3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s %s %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_ERROR_FROM], 
			sCmdParts[0], clsLanguageManager::mPtr->sTexts[LAN_TO_LWR], sCmdParts[1], clsLanguageManager::mPtr->sTexts[LAN_IS_NOT_VALID_RANGE]);
        return true;
    }

    char cTime = sCmdParts[2][iCmdPartsLen[2]-1];
    sCmdParts[2][iCmdPartsLen[2]-1] = '\0';
    int iTime = atoi(sCmdParts[2]);
    time_t acc_time, ban_time;

    if(iTime <= 0 || GenerateTempBanTime(cTime, (uint32_t)iTime, acc_time, ban_time) == false) {
        pUser->SendFormatCheckPM("clsHubCommands::RangeTempBan4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %c%srangetempban <%s> <%s> <%s> <%s>. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], 
			clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", clsLanguageManager::mPtr->sTexts[LAN_FROMIP], clsLanguageManager::mPtr->sTexts[LAN_TOIP], clsLanguageManager::mPtr->sTexts[LAN_TIME_LWR], 
			clsLanguageManager::mPtr->sTexts[LAN_REASON_LWR], clsLanguageManager::mPtr->sTexts[LAN_BAD_TIME_SPECIFIED]);
        return true;
    }

    if(clsBanManager::mPtr->RangeTempBan(sCmdParts[0], ui128FromHash, sCmdParts[1], ui128ToHash, sCmdParts[3], pUser->sNick, 0, ban_time, bFull) == false) {
        pUser->SendFormatCheckPM("clsHubCommands::RangeTempBan5", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s-%s %s %s%s %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_RANGE], 
			sCmdParts[0], sCmdParts[1], clsLanguageManager::mPtr->sTexts[LAN_IS_ALREADY], bFull == true ? clsLanguageManager::mPtr->sTexts[LAN_FULL_LWR] : "", clsLanguageManager::mPtr->sTexts[LAN_BANNED_LWR], clsLanguageManager::mPtr->sTexts[LAN_TO_LONGER_TIME]);
        return true;
    }

	UncountDeflood(pUser, bFromPM);
    static char sTime[256];
    strcpy(sTime, formatTime((ban_time-acc_time)/60));

    if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::RangeTempBan", "<%s> *** %s %s-%s %s %s%s %s %s %s: %s %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_RANGE], sCmdParts[0], sCmdParts[1], clsLanguageManager::mPtr->sTexts[LAN_IS_LWR], 
			bFull == true ? clsLanguageManager::mPtr->sTexts[LAN_FULL_LWR] : "", clsLanguageManager::mPtr->sTexts[LAN_TEMP_BANNED], clsLanguageManager::mPtr->sTexts[LAN_BY_LWR], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_TO_LWR], sTime, clsLanguageManager::mPtr->sTexts[LAN_BECAUSE_LWR], 
			sCmdParts[3] == NULL ? clsLanguageManager::mPtr->sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[3]);
    }

    if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        pUser->SendFormatCheckPM("clsHubCommands::RangeTempBan6", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s-%s %s %s%s %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_RANGE], 
			sCmdParts[0], sCmdParts[1], clsLanguageManager::mPtr->sTexts[LAN_IS_LWR], bFull == true ? clsLanguageManager::mPtr->sTexts[LAN_FULL_LWR] : "", clsLanguageManager::mPtr->sTexts[LAN_TEMP_BANNED], clsLanguageManager::mPtr->sTexts[LAN_TO_LWR], sTime);
    }
    return true;
}
//---------------------------------------------------------------------------

bool clsHubCommands::RangeUnban(User * pUser, char * sCommand, bool bFromPM, unsigned char cType) {
	char * sToIp = strchr(sCommand, ' ');
	if(sToIp != NULL) {
        sToIp[0] = '\0';
        sToIp++;
    }

	uint8_t ui128FromHash[16], ui128ToHash[16];
	memset(ui128FromHash, 0, 16);
	memset(ui128ToHash, 0, 16);

	if(sToIp == NULL || sCommand[0] == '\0' || sToIp[0] == '\0' || HashIP(sCommand, ui128FromHash) == false || HashIP(sToIp, ui128ToHash) == false) {
        pUser->SendFormatCheckPM("clsHubCommands::RangeUnban1", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], 
			clsLanguageManager::mPtr->sTexts[LAN_BAD_PARAMS_GIVEN]);
        return true;
    }

    if(memcmp(ui128ToHash, ui128FromHash, 16) <= 0) {
        pUser->SendFormatCheckPM("clsHubCommands::RangeUnban2", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s %s %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_ERROR_FROM], 
			sCommand, clsLanguageManager::mPtr->sTexts[LAN_TO_LWR], sToIp, clsLanguageManager::mPtr->sTexts[LAN_IS_NOT_VALID_RANGE]);
        return true;
    }

    if(clsBanManager::mPtr->RangeUnban(ui128FromHash, ui128ToHash, cType) == false) {
        pUser->SendFormatCheckPM("clsHubCommands::RangeUnban3", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s-%s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_RANGE], sCommand, 
			sToIp, clsLanguageManager::mPtr->sTexts[LAN_IS_NOT_IN_MY_RANGE], cType == 1 ? clsLanguageManager::mPtr->sTexts[LAN_TEMP_BANS_LWR] : clsLanguageManager::mPtr->sTexts[LAN_PERM_BANS_LWR]);
        return true;
    }

	UncountDeflood(pUser, bFromPM);

    if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::RangeUnban", "<%s> *** %s %s-%s %s %s by %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_RANGE], sCommand, sToIp, clsLanguageManager::mPtr->sTexts[LAN_IS_REMOVED_FROM_RANGE], 
			cType == 1 ? clsLanguageManager::mPtr->sTexts[LAN_TEMP_BANS_LWR] : clsLanguageManager::mPtr->sTexts[LAN_PERM_BANS_LWR], pUser->sNick);
    }

    if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        pUser->SendFormatCheckPM("clsHubCommands::RangeUnban4", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s-%s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_RANGE], sCommand, sToIp, 
			clsLanguageManager::mPtr->sTexts[LAN_IS_REMOVED_FROM_RANGE], cType == 1 ? clsLanguageManager::mPtr->sTexts[LAN_TEMP_BANS_LWR] : clsLanguageManager::mPtr->sTexts[LAN_PERM_BANS_LWR]);
    }
    
    return true;
}
//---------------------------------------------------------------------------

bool clsHubCommands::RangeUnban(User * pUser, char * sCommand, bool bFromPM) {
	char * sToIp = strchr(sCommand, ' ');

	if(sToIp != NULL) {
        sToIp[0] = '\0';
        sToIp++;
    }

	uint8_t ui128FromHash[16], ui128ToHash[16];
	memset(ui128FromHash, 0, 16);
	memset(ui128ToHash, 0, 16);

	if(sToIp == NULL || sCommand[0] == '\0' || sToIp[0] == '\0' || HashIP(sCommand, ui128FromHash) == false || HashIP(sToIp, ui128ToHash) == false) {
        pUser->SendFormatCheckPM("clsHubCommands::RangeUnban21", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s. %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SNTX_ERR_IN_CMD], 
			clsLanguageManager::mPtr->sTexts[LAN_BAD_PARAMS_GIVEN]);
        return true;
    }

    if(memcmp(ui128ToHash, ui128FromHash, 16) <= 0) {
        pUser->SendFormatCheckPM("clsHubCommands::RangeUnban22", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s %s %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_ERROR_FROM], 
			sCommand, clsLanguageManager::mPtr->sTexts[LAN_TO_LWR], sToIp, clsLanguageManager::mPtr->sTexts[LAN_IS_NOT_VALID_RANGE]);
        return true;
    }

    if(clsBanManager::mPtr->RangeUnban(ui128FromHash, ui128ToHash) == false) {
        pUser->SendFormatCheckPM("clsHubCommands::RangeUnban23", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s-%s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_RANGE], sCommand, 
			sToIp, clsLanguageManager::mPtr->sTexts[LAN_IS_NOT_IN_MY_RANGE_BANS]);
        return true;
    }

    UncountDeflood(pUser, bFromPM);

    if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        clsGlobalDataQueue::mPtr->StatusMessageFormat("clsHubCommands::RangeUnban2", "<%s> *** %s %s-%s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_RANGE], sCommand, sToIp, clsLanguageManager::mPtr->sTexts[LAN_IS_REMOVED_FROM_RANGE_BANS_BY], pUser->sNick);
    }

    if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        pUser->SendFormatCheckPM("clsHubCommands::RangeUnban24", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s-%s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_RANGE], sCommand, sToIp, 
			clsLanguageManager::mPtr->sTexts[LAN_IS_REMOVED_FROM_RANGE_BANS]);
    }
    
    return true;
}
//---------------------------------------------------------------------------

void clsHubCommands::SendNoPermission(User * pUser, const bool &bFromPM) {
    pUser->SendFormatCheckPM("clsHubCommands::SendNoPermission", bFromPM == true ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_ARE_NOT_ALWD_TO_USE_THIS_CMD]);
}
//---------------------------------------------------------------------------

int clsHubCommands::CheckFromPm(User * pUser, const bool &bFromPM) {
    if(bFromPM == false) {
        return 0;
    }

    int iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "$To: %s From: %s $", pUser->sNick, clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC]);
    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsHubCommands::CheckFromPm") == false) {
        return 0;
    }

    return iMsgLen;
}
//---------------------------------------------------------------------------

void clsHubCommands::UncountDeflood(User * pUser, const bool &bFromPM) {
    if(bFromPM == false) {
        if(pUser->ui16ChatMsgs != 0) {
			pUser->ui16ChatMsgs--;
			pUser->ui16ChatMsgs2--;
        }
    } else {
        if(pUser->ui16PMs != 0) {
            pUser->ui16PMs--;
            pUser->ui16PMs2--;
        }
    }
}
//---------------------------------------------------------------------------
