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
#include "colUsers.h"
#include "DcCommands.h"
#include "eventqueue.h"
#include "GlobalDataQueue.h"
#include "hashBanManager.h"
#include "hashRegManager.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
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
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "HubCommands.h"
//---------------------------------------------------------------------------
#include "IP2Country.h"
#include "LuaScript.h"
#include "TextFileManager.h"
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
    #include "../gui.win/RegisteredUsersDialog.h"
#endif
//---------------------------------------------------------------------------
HubCommands *HubCmds = NULL;
//---------------------------------------------------------------------------

HubCommands::HubCommands() {
    msg[0] = '\0';
}
//---------------------------------------------------------------------------

HubCommands::~HubCommands() {
}
//---------------------------------------------------------------------------

bool HubCommands::DoCommand(User * curUser, char * sCommand, const size_t &szCmdLen, bool fromPM/* = false*/) {
	size_t dlen;

    if(fromPM == false) {
    	// Hub commands: !me
		if(strncasecmp(sCommand+curUser->ui8NickLen, "me ", 3) == 0) {
	    	// %me command
    	    if(szCmdLen - (curUser->ui8NickLen + 4) > 4) {
                sCommand[0] = '*';
    			sCommand[1] = ' ';
    			memcpy(sCommand+2, curUser->sNick, curUser->ui8NickLen);
                colUsers->SendChat2All(curUser, sCommand, szCmdLen-4, NULL);
                return true;
    	    }
    	    return false;
        }

        // PPK ... optional reply commands in chat to PM
        fromPM = SettingManager->bBools[SETBOOL_REPLY_TO_HUB_COMMANDS_AS_PM];

        sCommand[szCmdLen-5] = '\0'; // get rid of the pipe
        sCommand += curUser->ui8NickLen;
        dlen = szCmdLen - (curUser->ui8NickLen + 5);
    } else {
        sCommand++;
        dlen = szCmdLen - (curUser->ui8NickLen + 6);
    }

    switch(tolower(sCommand[0])) {
        case 'g':
            // Hub commands: !getbans
			if(dlen == 7 && strncasecmp(sCommand+1, "etbans", 6) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::GETBANLIST) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                UncountDeflood(curUser, fromPM);

                int imsglen = CheckFromPm(curUser, fromPM);

                int iret = sprintf(msg+imsglen, "<%s> ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
                imsglen += iret;
                if(CheckSprintf1(iret, imsglen, 1024, "HubCommands::DoCommand2") == false) {
                    return true;
                }

				string BanList(msg, imsglen);
                bool bIsEmpty = true;

                if(hashBanManager->TempBanListS != NULL) {
                    uint32_t iBanNum = 0;
                    time_t acc_time;
                    time(&acc_time);

                    BanItem *nextBan = hashBanManager->TempBanListS;

                    while(nextBan != NULL) {
                        BanItem *curBan = nextBan;
    		            nextBan = curBan->next;

                        if(acc_time > curBan->tempbanexpire) {
							hashBanManager->Rem(curBan);
                            delete curBan;

							continue;
                        }

                        if(iBanNum == 0) {
							BanList += string(LanguageManager->sTexts[LAN_TEMP_BANS], (size_t)LanguageManager->ui16TextsLens[LAN_TEMP_BANS]) + ":\n\n";
                        }

                        iBanNum++;
						BanList += "[ " + string(iBanNum) + " ]";

                        if(curBan->sIp[0] != '\0') {
                            if(((curBan->ui8Bits & hashBanMan::IP) == hashBanMan::IP) == true) {
								BanList += " " + string(LanguageManager->sTexts[LAN_BANNED], (size_t)LanguageManager->ui16TextsLens[LAN_BANNED]);
                            }
							BanList += " " + string(LanguageManager->sTexts[LAN_IP], (size_t)LanguageManager->ui16TextsLens[LAN_IP]) + ": " + string(curBan->sIp);
                            if(((curBan->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == true) {
								BanList += " (" + string(LanguageManager->sTexts[LAN_FULL], (size_t)LanguageManager->ui16TextsLens[LAN_FULL]) + ")";
                            }
                        }

                        if(curBan->sNick != NULL) {
                            if(((curBan->ui8Bits & hashBanMan::NICK) == hashBanMan::NICK) == true) {
								BanList += " " + string(LanguageManager->sTexts[LAN_BANNED], (size_t)LanguageManager->ui16TextsLens[LAN_BANNED]);
                            }
							BanList += " " + string(LanguageManager->sTexts[LAN_NICK], (size_t)LanguageManager->ui16TextsLens[LAN_NICK]) + ": " + string(curBan->sNick);
                        }

                        if(curBan->sBy != NULL) {
							BanList += " " + string(LanguageManager->sTexts[LAN_BY], (size_t)LanguageManager->ui16TextsLens[LAN_BY]) + ": " + string(curBan->sBy);
                        }

                        if(curBan->sReason != NULL) {
							BanList += " " + string(LanguageManager->sTexts[LAN_REASON], (size_t)LanguageManager->ui16TextsLens[LAN_REASON]) + ": " + string(curBan->sReason);
                        }

                        struct tm *tm = localtime(&curBan->tempbanexpire);
                        strftime(msg, 256, "%c\n", tm);

						BanList += " " + string(LanguageManager->sTexts[LAN_EXPIRE], (size_t)LanguageManager->ui16TextsLens[LAN_EXPIRE]) + ": " + string(msg);
                    }

                    if(iBanNum > 0) {
                        bIsEmpty = false;
                        BanList += "\n\n";
                    }
                }

                if(hashBanManager->PermBanListS != NULL) {
                    bIsEmpty = false;

					BanList += string(LanguageManager->sTexts[LAN_PERM_BANS], (size_t)LanguageManager->ui16TextsLens[LAN_PERM_BANS]) + ":\n\n";

                    uint32_t iBanNum = 0;
                    BanItem *nextBan = hashBanManager->PermBanListS;

                    while(nextBan != NULL) {
                        BanItem *curBan = nextBan;
    		            nextBan = curBan->next;

                        iBanNum++;
						BanList += "[ " + string(iBanNum) + " ]";
                        
                        if(curBan->sIp[0] != '\0') {
                            if(((curBan->ui8Bits & hashBanMan::IP) == hashBanMan::IP) == true) {
								BanList += " " + string(LanguageManager->sTexts[LAN_BANNED], (size_t)LanguageManager->ui16TextsLens[LAN_BANNED]);
							}
							BanList += " " + string(LanguageManager->sTexts[LAN_IP], (size_t)LanguageManager->ui16TextsLens[LAN_IP]) + ": " + string(curBan->sIp);
                            if(((curBan->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == true) {
								BanList += " (" + string(LanguageManager->sTexts[LAN_FULL], (size_t)LanguageManager->ui16TextsLens[LAN_FULL]) + ")";
                            }
                        }

                        if(curBan->sNick != NULL) {
							if(((curBan->ui8Bits & hashBanMan::NICK) == hashBanMan::NICK) == true) {
								   BanList += " " + string(LanguageManager->sTexts[LAN_BANNED], (size_t)LanguageManager->ui16TextsLens[LAN_BANNED]);
                            }
							BanList += " " + string(LanguageManager->sTexts[LAN_NICK], (size_t)LanguageManager->ui16TextsLens[LAN_NICK]) + ": " + string(curBan->sNick);
                        }

                        if(curBan->sBy != NULL) {
							BanList += " " + string(LanguageManager->sTexts[LAN_BY], (size_t)LanguageManager->ui16TextsLens[LAN_BY]) + ": " + string(curBan->sBy);
                        }

                        if(curBan->sReason != NULL) {
							BanList += " " + string(LanguageManager->sTexts[LAN_REASON], (size_t)LanguageManager->ui16TextsLens[LAN_REASON]) + ": " + string(curBan->sReason);
                        }

                        BanList += "\n";
                    }
                }

                if(bIsEmpty == true) {
					BanList += string(LanguageManager->sTexts[LAN_NO_BANS_FOUND], (size_t)LanguageManager->ui16TextsLens[LAN_NO_BANS_FOUND]) + "...|";
                } else {
                    BanList += "|";
                }

                UserSendTextDelayed(curUser, BanList);
                return true;
            }

            // Hub commands: !gag
			if(strncasecmp(sCommand+1, "ag ", 3) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::GAG) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

               	if(dlen < 5 || sCommand[4] == '\0') {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cgag <%s>. %s!|", 
                        SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], 
                        SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR], 
                        LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand4") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                sCommand += 4;

                // Self-gag ?
				if(strcasecmp(sCommand, curUser->sNick) == 0) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_YOU_CANT_GAG_YOURSELF]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand6") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
           	    }

                if(dlen > 100) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cgag <%s>. %s!|",
                        SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
                        SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR],
                        LanguageManager->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand7") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                User *user = hashManager->FindUser(sCommand, dlen-4);
        		if(user == NULL) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

        			int iret = sprintf(msg+imsgLen, "<%s> *** %s: %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_ERROR], sCommand, LanguageManager->sTexts[LAN_IS_NOT_IN_USERLIST]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand8") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
        			return true;
                }

                if(((user->ui32BoolBits & User::BIT_GAGGED) == User::BIT_GAGGED) == true) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

        			int iret = sprintf(msg+imsgLen, "<%s> *** %s: %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_ERROR], user->sNick, LanguageManager->sTexts[LAN_IS_ALREDY_GAGGED]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand10") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
        			return true;
                }

        		// PPK don't gag user with higher profile
        		if(user->iProfile != -1 && curUser->iProfile > user->iProfile) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        LanguageManager->sTexts[LAN_NOT_ALW_TO_GAG], user->sNick);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand12") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
        			return true;
                }

				UncountDeflood(curUser, fromPM);

                user->ui32BoolBits |= User::BIT_GAGGED;
                int imsgLen = sprintf(msg, "<%s> %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                    LanguageManager->sTexts[LAN_YOU_GAGGED_BY], curUser->sNick);
                if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand13") == true) {
                    UserSendCharDelayed(user, msg, imsgLen);
                }

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {            
                    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                        imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                            SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick, 
                            LanguageManager->sTexts[LAN_HAS_GAGGED], user->sNick);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand14") == true) {
							g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                        }
                    } else {
                        imsgLen = sprintf(msg, "<%s> *** %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick,
                            LanguageManager->sTexts[LAN_HAS_GAGGED], user->sNick);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand15") == true) {
                            g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                        }
                    }
                } 
                        
                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    imsgLen = CheckFromPm(curUser, fromPM);

        			int iret = sprintf(msg+imsgLen, "<%s> %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], user->sNick, 
                        LanguageManager->sTexts[LAN_HAS_GAGGED]);
        			imsgLen += iret;
        			if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand17") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                }
                return true;
            }

            // Hub commands: !getinfo
			if(strncasecmp(sCommand+1, "etinfo ", 7) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::GETINFO) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen < 9 || sCommand[8] == 0) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cgetinfo <%s>. %s.|", 
                        SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], 
                        SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR], 
                        LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand19") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                if(dlen > 100) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cgetinfo <%s>. %s!|",
                        SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
                        SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR],
                        LanguageManager->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand20") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                sCommand += 8;

                User *user = hashManager->FindUser(sCommand, dlen-8);
                if(user == NULL) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s: %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_ERROR], sCommand, LanguageManager->sTexts[LAN_IS_NOT_IN_USERLIST]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand21") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }
                
				UncountDeflood(curUser, fromPM);

                int imsgLen = CheckFromPm(curUser, fromPM);

                int iret = sprintf(msg+imsgLen, "<%s> " "\n%s: %s", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                    LanguageManager->sTexts[LAN_NICK], user->sNick);
                imsgLen += iret;
                if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand23") == false) {
                    return true;
                }

                if(user->iProfile != -1) {
                    int iret = sprintf(msg+imsgLen, "\n%s: %s", LanguageManager->sTexts[LAN_PROFILE], ProfileMan->ProfilesTable[user->iProfile]->sName);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand24") == false) {
                        return true;
                    }
                }

                iret = sprintf(msg+imsgLen, "\n%s: %s ", LanguageManager->sTexts[LAN_STATUS], LanguageManager->sTexts[LAN_ONLINE_FROM]);
                imsgLen += iret;
                if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand25") == false) {
                    return true;
                }

                struct tm *tm = localtime(&user->LoginTime);
                iret = (int)strftime(msg+imsgLen, 256, "%c", tm);
                imsgLen += iret;
                if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand26") == false) {
                    return true;
                }

				if(user->sIPv4[0] != '\0') {
					iret = sprintf(msg+imsgLen, "\n%s: %s / %s\n%s: %0.02f %s", LanguageManager->sTexts[LAN_IP], user->sIP, user->sIPv4,
						LanguageManager->sTexts[LAN_SHARE_SIZE], (double)user->ui64SharedSize/1073741824, LanguageManager->sTexts[LAN_GIGA_BYTES]);
					imsgLen += iret;
					if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand27-1") == false) {
						return true;
					}
				} else {
					iret = sprintf(msg+imsgLen, "\n%s: %s\n%s: %0.02f %s", LanguageManager->sTexts[LAN_IP], user->sIP,
						LanguageManager->sTexts[LAN_SHARE_SIZE], (double)user->ui64SharedSize/1073741824, LanguageManager->sTexts[LAN_GIGA_BYTES]);
					imsgLen += iret;
					if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand27-2") == false) {
						return true;
					}
				}

                if(user->sDescription != NULL) {
                    int iret = sprintf(msg+imsgLen, "\n%s: ", LanguageManager->sTexts[LAN_DESCRIPTION]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand28") == false) {
                        return true;
                    }
                    memcpy(msg+imsgLen, user->sDescription, user->ui8DescriptionLen);
                    imsgLen += user->ui8DescriptionLen;
                }

                if(user->sTag != NULL) {
                    int iret = sprintf(msg+imsgLen, "\n%s: ", LanguageManager->sTexts[LAN_TAG]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand29") == false) {
                        return true;
                    }
                    memcpy(msg+imsgLen, user->sTag, user->ui8TagLen);
                    imsgLen += (int)user->ui8TagLen;
                }
                    
                if(user->sConnection != NULL) {
                    int iret = sprintf(msg+imsgLen, "\n%s: ", LanguageManager->sTexts[LAN_CONNECTION]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand30") == false) {
                        return true;
                    }
                    memcpy(msg+imsgLen, user->sConnection, user->ui8ConnectionLen);
                    imsgLen += user->ui8ConnectionLen;
                }

                if(user->sEmail != NULL) {
                    int iret = sprintf(msg+imsgLen, "\n%s: ", LanguageManager->sTexts[LAN_EMAIL]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand31") == false) {
                        return true;
                    }
                    memcpy(msg+imsgLen, user->sEmail, user->ui8EmailLen);
                    imsgLen += user->ui8EmailLen;
                }

                if(IP2Country->ui32Count != 0) {
                    iret = sprintf(msg+imsgLen, "\n%s: ", LanguageManager->sTexts[LAN_COUNTRY]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand32") == false) {
                        return true;
                    }
                    memcpy(msg+imsgLen, IP2Country->GetCountry(user->ui8Country, false), 2);
                    imsgLen += 2;
                }

                msg[imsgLen] = '|';
                msg[imsgLen+1] = '\0';  
                UserSendCharDelayed(curUser, msg, imsgLen+1);
                return true;
            }

            // Hub commands: !gettempbans
			if(dlen == 11 && strncasecmp(sCommand+1, "ettempbans", 10) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::GETBANLIST) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

				UncountDeflood(curUser, fromPM);

                int imsglen = CheckFromPm(curUser, fromPM);

                int iret = sprintf(msg+imsglen, "<%s> ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
                imsglen += iret;
                if(CheckSprintf1(iret, imsglen, 1024, "HubCommands::DoCommand33") == false) {
                    return true;
                }

				string BanList = string(msg, imsglen);

                if(hashBanManager->TempBanListS != NULL) {
                    uint32_t iBanNum = 0;

                    time_t acc_time;
                    time(&acc_time);

                    BanItem *nextBan = hashBanManager->TempBanListS;

                    while(nextBan != NULL) {
                        BanItem *curBan = nextBan;
    		            nextBan = curBan->next;

                        if(acc_time > curBan->tempbanexpire) {
							hashBanManager->Rem(curBan);
                            delete curBan;

							continue;
                        }

						if(iBanNum == 0) {
							BanList += string(LanguageManager->sTexts[LAN_TEMP_BANS], (size_t)LanguageManager->ui16TextsLens[LAN_TEMP_BANS]) + ":\n\n";
                        }

                        iBanNum++;
						BanList += "[ " + string(iBanNum) + " ]";

                        if(curBan->sIp[0] != '\0') {
                            if(((curBan->ui8Bits & hashBanMan::IP) == hashBanMan::IP) == true) {
								BanList += " " + string(LanguageManager->sTexts[LAN_BANNED], (size_t)LanguageManager->ui16TextsLens[LAN_BANNED]);
                            }
							BanList += " " + string(LanguageManager->sTexts[LAN_IP], (size_t)LanguageManager->ui16TextsLens[LAN_IP]) + ": " + string(curBan->sIp);
                            if(((curBan->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == true) {
								BanList += " (" + string(LanguageManager->sTexts[LAN_FULL], (size_t)LanguageManager->ui16TextsLens[LAN_FULL]) + ")";
                            }
                        }

                        if(curBan->sNick != NULL) {
                            if(((curBan->ui8Bits & hashBanMan::NICK) == hashBanMan::NICK) == true) {
								BanList += " " + string(LanguageManager->sTexts[LAN_BANNED], (size_t)LanguageManager->ui16TextsLens[LAN_BANNED]);
                            }
							BanList += " " + string(LanguageManager->sTexts[LAN_NICK], (size_t)LanguageManager->ui16TextsLens[LAN_NICK]) + ": " + string(curBan->sNick);
                        }

						if(curBan->sBy != NULL) {
							BanList += " " + string(LanguageManager->sTexts[LAN_BY], (size_t)LanguageManager->ui16TextsLens[LAN_BY]) + ": " + string(curBan->sBy);
                        }

                        if(curBan->sReason != NULL) {
							BanList += " " + string(LanguageManager->sTexts[LAN_REASON], (size_t)LanguageManager->ui16TextsLens[LAN_REASON]) + ": " + string(curBan->sReason);
                        }

                        struct tm *tm = localtime(&curBan->tempbanexpire);
                        strftime(msg, 256, "%c\n", tm);

						BanList += " " + string(LanguageManager->sTexts[LAN_EXPIRE], (size_t)LanguageManager->ui16TextsLens[LAN_EXPIRE]) + ": " + string(msg);
                    }

                    if(iBanNum > 0) {
                        BanList += "|";
                    } else {
						BanList += string(LanguageManager->sTexts[LAN_NO_TEMP_BANS_FOUND], (size_t)LanguageManager->ui16TextsLens[LAN_NO_TEMP_BANS_FOUND]) + "...|";
                    }
                } else {
					BanList += string(LanguageManager->sTexts[LAN_NO_TEMP_BANS_FOUND], (size_t)LanguageManager->ui16TextsLens[LAN_NO_TEMP_BANS_FOUND]) + "...|";
                }

				UserSendTextDelayed(curUser, BanList);
                return true;
            }

            // Hub commands: !getscripts
			if(dlen == 10 && strncasecmp(sCommand+1, "etscripts", 9) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::RSTSCRIPTS) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

				UncountDeflood(curUser, fromPM);

                int imsglen = CheckFromPm(curUser, fromPM);

                int iret = sprintf(msg+imsglen, "<%s> ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
                imsglen += iret;
                if(CheckSprintf1(iret, imsglen, 1024, "HubCommands::DoCommand35") == false) {
                    return true;
                }

				string ScriptList(msg, imsglen);

				ScriptList += string(LanguageManager->sTexts[LAN_SCRIPTS], (size_t)LanguageManager->ui16TextsLens[LAN_SCRIPTS]) + ":\n\n";

				for(uint8_t ui8i = 0; ui8i < ScriptManager->ui8ScriptCount; ui8i++) {
					ScriptList += "[ " + string(ScriptManager->ScriptTable[ui8i]->bEnabled == true ? "1" : "0") +
						" ] " + string(ScriptManager->ScriptTable[ui8i]->sName);

					if(ScriptManager->ScriptTable[ui8i]->bEnabled == true) {
                        ScriptList += " ("+string(ScriptGetGC(ScriptManager->ScriptTable[ui8i]))+" kB)\n";
                    } else {
                        ScriptList += "\n";
                    }
				}

                ScriptList += "|";
				UserSendTextDelayed(curUser, ScriptList);
                return true;
            }

            // Hub commands: !getpermbans
			if(dlen == 11 && strncasecmp(sCommand+1, "etpermbans", 10) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::GETBANLIST) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

				UncountDeflood(curUser, fromPM);

                int imsglen = CheckFromPm(curUser, fromPM);

                int iret = sprintf(msg+imsglen, "<%s> ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
                imsglen += iret;
                if(CheckSprintf1(iret, imsglen, 1024, "HubCommands::DoCommand37") == false) {
                    return true;
                }

				string BanList(msg, imsglen);

                if(hashBanManager->PermBanListS != NULL) {
					BanList += string(LanguageManager->sTexts[LAN_PERM_BANS], (size_t)LanguageManager->ui16TextsLens[LAN_PERM_BANS]) + ":\n\n";

                    uint32_t iBanNum = 0;
                    BanItem *nextBan = hashBanManager->PermBanListS;

                    while(nextBan != NULL) {
                        BanItem *curBan = nextBan;
    		            nextBan = curBan->next;

						iBanNum++;
						BanList += "[ " + string(iBanNum) + " ]";

                        if(curBan->sIp[0] != '\0') {
                            if(((curBan->ui8Bits & hashBanMan::IP) == hashBanMan::IP) == true) {
								BanList += " " + string(LanguageManager->sTexts[LAN_BANNED], (size_t)LanguageManager->ui16TextsLens[LAN_BANNED]);
                            }
							BanList += " " + string(LanguageManager->sTexts[LAN_IP], (size_t)LanguageManager->ui16TextsLens[LAN_IP]) + ": " + string(curBan->sIp);
                            if(((curBan->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == true) {
								BanList += " (" + string(LanguageManager->sTexts[LAN_FULL], (size_t)LanguageManager->ui16TextsLens[LAN_FULL]) + ")";
                            }
                        }

                        if(curBan->sNick != NULL) {
                            if(((curBan->ui8Bits & hashBanMan::NICK) == hashBanMan::NICK) == true) {
								BanList += " " + string(LanguageManager->sTexts[LAN_BANNED], (size_t)LanguageManager->ui16TextsLens[LAN_BANNED]);
                            }
							BanList += " " + string(LanguageManager->sTexts[LAN_NICK], (size_t)LanguageManager->ui16TextsLens[LAN_NICK]) + ": " + string(curBan->sNick);
                        }

                        if(curBan->sBy != NULL) {
							BanList += " " + string(LanguageManager->sTexts[LAN_BY], (size_t)LanguageManager->ui16TextsLens[LAN_BY]) + ": " + string(curBan->sBy);
                        }

                        if(curBan->sReason != NULL) {
							BanList += " " + string(LanguageManager->sTexts[LAN_REASON], (size_t)LanguageManager->ui16TextsLens[LAN_REASON]) + ": " + string(curBan->sReason);
                        }

                        BanList += "\n";
                    }

					BanList += "|";
                } else {
					BanList += string(LanguageManager->sTexts[LAN_NO_PERM_BANS_FOUND], (size_t)LanguageManager->ui16TextsLens[LAN_NO_PERM_BANS_FOUND]) + "...|";
                }

				UserSendTextDelayed(curUser, BanList);
                return true;
            }

            // Hub commands: !getrangebans
			if(dlen == 12 && strncasecmp(sCommand+1, "etrangebans", 11) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::GET_RANGE_BANS) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

				UncountDeflood(curUser, fromPM);

                int imsglen = CheckFromPm(curUser, fromPM);

                int iret = sprintf(msg+imsglen, "<%s> ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
                imsglen += iret;
                if(CheckSprintf1(iret, imsglen, 1024, "HubCommands::DoCommand39") == false) {
                    return true;
                }

				string BanList(msg, imsglen);
                bool bIsEmpty = true;

                if(hashBanManager->RangeBanListS != NULL) {
                    uint32_t iBanNum = 0;

                    time_t acc_time;
                    time(&acc_time);

                    RangeBanItem *nextBan = hashBanManager->RangeBanListS;

                    while(nextBan != NULL) {
                        RangeBanItem *curBan = nextBan;
    		            nextBan = curBan->next;

    		            if(((curBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == false)
                            continue;

                        if(acc_time > curBan->tempbanexpire) {
							hashBanManager->RemRange(curBan);
                            delete curBan;

							continue;
                        }

                        if(iBanNum == 0) {
							BanList += string(LanguageManager->sTexts[LAN_TEMP_RANGE_BANS], (size_t)LanguageManager->ui16TextsLens[LAN_TEMP_RANGE_BANS]) + ":\n\n";
                        }

                        iBanNum++;
						BanList += "[ " + string(iBanNum) + " ]";
						BanList += " " + string(LanguageManager->sTexts[LAN_RANGE], (size_t)LanguageManager->ui16TextsLens[LAN_RANGE]) +
							": " + string(curBan->sIpFrom) + "-" + string(curBan->sIpTo);

                        if(((curBan->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == true) {
							BanList += " (" + string(LanguageManager->sTexts[LAN_FULL], (size_t)LanguageManager->ui16TextsLens[LAN_FULL]) + ")";
                        }

                        if(curBan->sBy != NULL) {
							BanList += " " + string(LanguageManager->sTexts[LAN_BY], (size_t)LanguageManager->ui16TextsLens[LAN_BY]) + ": " + string(curBan->sBy);
                        }

                        if(curBan->sReason != NULL) {
							BanList += " " + string(LanguageManager->sTexts[LAN_REASON], (size_t)LanguageManager->ui16TextsLens[LAN_REASON]) +
								": " + string(curBan->sReason);
                        }

                        struct tm *tm = localtime(&curBan->tempbanexpire);
                        strftime(msg, 256, "%c\n", tm);

						BanList += " " + string(LanguageManager->sTexts[LAN_EXPIRE], (size_t)LanguageManager->ui16TextsLens[LAN_EXPIRE]) + ": " + string(msg);
                    }

                    if(iBanNum > 0) {
                        bIsEmpty = false;
                        BanList += "\n\n";
                    }

                    iBanNum = 0;
                    nextBan = hashBanManager->RangeBanListS;

                    while(nextBan != NULL) {
                        RangeBanItem *curBan = nextBan;
    		            nextBan = curBan->next;

    		            if(((curBan->ui8Bits & hashBanMan::PERM) == hashBanMan::PERM) == false)
                            continue;

                        if(iBanNum == 0) {
                            bIsEmpty = false;
							BanList += string(LanguageManager->sTexts[LAN_PERM_RANGE_BANS], (size_t)LanguageManager->ui16TextsLens[LAN_PERM_RANGE_BANS]) + ":\n\n";
                        }

                        iBanNum++;
						BanList += "[ " + string(iBanNum) + " ]";
						BanList += " " + string(LanguageManager->sTexts[LAN_RANGE], (size_t)LanguageManager->ui16TextsLens[LAN_RANGE]) +
							": " + string(curBan->sIpFrom) + "-" + string(curBan->sIpTo);

                        if(((curBan->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == true) {
							BanList += " (" + string(LanguageManager->sTexts[LAN_FULL], (size_t)LanguageManager->ui16TextsLens[LAN_FULL]) + ")";
                        }

                        if(curBan->sBy != NULL) {
							BanList += " " + string(LanguageManager->sTexts[LAN_BY], (size_t)LanguageManager->ui16TextsLens[LAN_BY]) + ": " + string(curBan->sBy);
                        }

                        if(curBan->sReason != NULL) {
							BanList += " " + string(LanguageManager->sTexts[LAN_REASON], (size_t)LanguageManager->ui16TextsLens[LAN_REASON]) +
								": " + string(curBan->sReason);
                        }

                        BanList += "\n";
                    }
                }

                if(bIsEmpty == true) {
					BanList += string(LanguageManager->sTexts[LAN_NO_RANGE_BANS_FOUND], (size_t)LanguageManager->ui16TextsLens[LAN_NO_RANGE_BANS_FOUND]) + "...|";
                } else {
                    BanList += "|";
                }

				UserSendTextDelayed(curUser, BanList);
                return true;
            }

            // Hub commands: !getrangepermbans
			if(dlen == 16 && strncasecmp(sCommand+1, "etrangepermbans", 15) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::GET_RANGE_BANS) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

				UncountDeflood(curUser, fromPM);

                int imsglen = CheckFromPm(curUser, fromPM);

                int iret = sprintf(msg+imsglen, "<%s> ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
                imsglen += iret;
                if(CheckSprintf1(iret, imsglen, 1024, "HubCommands::DoCommand41") == false) {
                    return true;
                }

				string BanList(msg, imsglen);
                bool bIsEmpty = true;

                if(hashBanManager->RangeBanListS != NULL) {
                    uint32_t iBanNum = 0;
                    RangeBanItem *nextBan = hashBanManager->RangeBanListS;

                    while(nextBan != NULL) {
                        RangeBanItem *curBan = nextBan;
    		            nextBan = curBan->next;

    		            if(((curBan->ui8Bits & hashBanMan::PERM) == hashBanMan::PERM) == false)
                            continue;

                        if(iBanNum == 0) {
                            bIsEmpty = false;

							BanList += string(LanguageManager->sTexts[LAN_PERM_RANGE_BANS], (size_t)LanguageManager->ui16TextsLens[LAN_PERM_RANGE_BANS]) + ":\n\n";
                        }

                        iBanNum++;
						BanList += "[ " + string(iBanNum) + " ]";
						BanList += " " + string(LanguageManager->sTexts[LAN_RANGE], (size_t)LanguageManager->ui16TextsLens[LAN_RANGE]) +
							": " + string(curBan->sIpFrom) + "-" + string(curBan->sIpTo);

                        if(((curBan->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == true) {
							BanList += " (" + string(LanguageManager->sTexts[LAN_FULL], (size_t)LanguageManager->ui16TextsLens[LAN_FULL]) + ")";
                        }

                        if(curBan->sBy != NULL) {
							BanList += " " + string(LanguageManager->sTexts[LAN_BY], (size_t)LanguageManager->ui16TextsLens[LAN_BY]) + ": " + string(curBan->sBy);
                        }

                        if(curBan->sReason != NULL) {
							BanList += " " + string(LanguageManager->sTexts[LAN_REASON], (size_t)LanguageManager->ui16TextsLens[LAN_REASON]) +
								": " + string(curBan->sReason);
                        }

                        BanList += "\n";
                    }
                }

                if(bIsEmpty == true) {
					BanList += string(LanguageManager->sTexts[LAN_NO_RANGE_PERM_BANS_FOUND], (size_t)LanguageManager->ui16TextsLens[LAN_NO_RANGE_PERM_BANS_FOUND]) + "...|";
                } else {
                    BanList += "|";
                }

				UserSendTextDelayed(curUser, BanList);
                return true;
            }

            // Hub commands: !getrangetempbans
			if(dlen == 16 && strncasecmp(sCommand+1, "etrangetempbans", 15) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::GET_RANGE_BANS) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

				UncountDeflood(curUser, fromPM);

                int imsglen = CheckFromPm(curUser, fromPM);

                int iret = sprintf(msg+imsglen, "<%s> ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
                imsglen += iret;
                if(CheckSprintf1(iret, imsglen, 1024, "HubCommands::DoCommand43") == false) {
                    return true;
                }

				string BanList(msg, imsglen);
                bool bIsEmpty = true;

                if(hashBanManager->RangeBanListS != NULL) {
                    uint32_t iBanNum = 0;

                    time_t acc_time;
                    time(&acc_time);

                    RangeBanItem *nextBan = hashBanManager->RangeBanListS;

                    while(nextBan != NULL) {
                        RangeBanItem *curBan = nextBan;
    		            nextBan = curBan->next;

    		            if(((curBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == false)
                            continue;

                        if(acc_time > curBan->tempbanexpire) {
							hashBanManager->RemRange(curBan);
                            delete curBan;

							continue;
                        }

                        if(iBanNum == 0) {
							BanList += string(LanguageManager->sTexts[LAN_TEMP_RANGE_BANS], (size_t)LanguageManager->ui16TextsLens[LAN_TEMP_RANGE_BANS]) + ":\n\n";
                        }

                        iBanNum++;
						BanList += "[ " + string(iBanNum) + " ]";
						BanList += " " + string(LanguageManager->sTexts[LAN_RANGE], (size_t)LanguageManager->ui16TextsLens[LAN_RANGE]) +
							": " + string(curBan->sIpFrom) + "-" + string(curBan->sIpTo);

                        if(((curBan->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == true) {
							BanList += " (" + string(LanguageManager->sTexts[LAN_FULL], (size_t)LanguageManager->ui16TextsLens[LAN_FULL]) + ")";
                        }

						if(curBan->sBy != NULL) {
							BanList += " " + string(LanguageManager->sTexts[LAN_BY], (size_t)LanguageManager->ui16TextsLens[LAN_BY]) + ": " + string(curBan->sBy);
                        }

                        if(curBan->sReason != NULL) {
							BanList += " " + string(LanguageManager->sTexts[LAN_REASON], (size_t)LanguageManager->ui16TextsLens[LAN_REASON]) +
								": " + string(curBan->sReason);
                        }

                        struct tm *tm = localtime(&curBan->tempbanexpire);
                        strftime(msg, 256, "%c\n", tm);

						BanList += " " + string(LanguageManager->sTexts[LAN_EXPIRE], (size_t)LanguageManager->ui16TextsLens[LAN_EXPIRE]) + ": " + string(msg);
                    }

                    if(iBanNum > 0) {
                        bIsEmpty = false;
                    }
                }

                if(bIsEmpty == true) {
					BanList += string(LanguageManager->sTexts[LAN_NO_RANGE_TEMP_BANS_FOUND], (size_t)LanguageManager->ui16TextsLens[LAN_NO_RANGE_TEMP_BANS_FOUND]) + "...|";
                } else {
                    BanList += "|";
                }

				UserSendTextDelayed(curUser, BanList);
                return true;
            }

            return false;

        case 'n':
            // Hub commands: !nickban nick reason
			if(strncasecmp(sCommand+1, "ickban ", 7) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::BAN) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen < 9) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cnickban <%s> <%s>. %s.|", 
                       SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], 
                       SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR], 
                       LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand45") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                sCommand += 8;
                size_t szNickLen;
                char *reason = strchr(sCommand, ' ');

                if(reason != NULL) {
                    szNickLen = reason - sCommand;
                    reason[0] = '\0';
                    if(reason[1] == '\0') {
                        reason = NULL;
                    } else {
                        reason++;
                    }
                } else {
                    szNickLen = dlen-8;
                }

                if(sCommand[0] == '\0') {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> %s %cnickban <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                        LanguageManager->sTexts[LAN_NICK_LWR], LanguageManager->sTexts[LAN_REASON_LWR], 
                        LanguageManager->sTexts[LAN_NO_NICK_SPECIFIED]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand47") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                if(szNickLen > 100) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> %s %cnickban <%s> <%s>. %s!|",
                        SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
                        SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR],
                        LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand48") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                // Self-ban ?
				if(strcasecmp(sCommand, curUser->sNick) == 0) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_YOU_CANT_BAN_YOURSELF]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand49") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
           	    }


                User *OtherUser = hashManager->FindUser(sCommand, szNickLen);
                if(OtherUser != NULL) {
                    // PPK don't nickban user with higher profile
                    if(OtherUser->iProfile != -1 && curUser->iProfile > OtherUser->iProfile) {
                        int imsgLen = CheckFromPm(curUser, fromPM);

                        int iret = sprintf(msg+imsgLen, "<%s> %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                            LanguageManager->sTexts[LAN_YOU_NOT_ALLOWED_TO], LanguageManager->sTexts[LAN_BAN_LWR], OtherUser->sNick);
                        imsgLen += iret;
                        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand51") == true) {
                            UserSendCharDelayed(curUser, msg, imsgLen);
                        }
                        return true;
                    }

                    UncountDeflood(curUser, fromPM);

                    int imsgLen;
                    if(reason != NULL && strlen(reason) > 512) {
                        imsgLen = sprintf(msg, "<%s> %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                            LanguageManager->sTexts[LAN_YOU_HAD_BEEN_BANNED_BCS]);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand52-1") == false) {
                   	        return true;
                        }
                        memcpy(msg+imsgLen, reason, 512);
                        imsgLen += (int)strlen(reason) + 2;
                        msg[imsgLen-2] = '.';
                        msg[imsgLen-1] = '|';
                        msg[imsgLen] = '\0';
                    } else {
                        imsgLen = sprintf(msg, "<%s> %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                            LanguageManager->sTexts[LAN_YOU_HAD_BEEN_BANNED_BCS],
                            reason == NULL ? LanguageManager->sTexts[LAN_NO_REASON_SPECIFIED] : reason);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand52-2") == false) {
                   	        return true;
                        }
                    }

                   	UserSendChar(OtherUser, msg, imsgLen);

                    if(hashBanManager->NickBan(OtherUser, NULL, reason, curUser->sNick) == true) {
                        imsgLen = sprintf(msg, "[SYS] User %s (%s) nickbanned by %s", OtherUser->sNick, OtherUser->sIP, curUser->sNick);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand53") == true) {
                            UdpDebug->Broadcast(msg, imsgLen);
                        }
                        UserClose(OtherUser);
                    } else {
                        UserClose(OtherUser);
                        imsgLen = CheckFromPm(curUser, fromPM);

                        int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                            LanguageManager->sTexts[LAN_NICK], OtherUser->sNick, LanguageManager->sTexts[LAN_IS_ALREDY_BANNED_DISCONNECT]);
                        imsgLen += iret;
                        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand55") == true) {
                            UserSendCharDelayed(curUser, msg, imsgLen);
                        }
                        return true;
                    }
                } else {
                    return NickBan(curUser, sCommand, reason, fromPM);
                }

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                        int imsgLen;
                        if(reason == NULL) {
                            imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                                SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick, 
                                LanguageManager->sTexts[LAN_ADDED_LWR], sCommand, LanguageManager->sTexts[LAN_TO_BANS]);
                            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand59") == false) {
                                return true;
                            }
                        } else {
                            if(strlen(reason) > 512) {
                                imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                                    SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand, LanguageManager->sTexts[LAN_HAS_BEEN_BANNED_BY],
                                    curUser->sNick, LanguageManager->sTexts[LAN_BECAUSE_LWR]);
                                if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand60-1") == false) {
                   	                return true;
                                }
                                memcpy(msg+imsgLen, reason, 512);
                                imsgLen += (int)strlen(reason) + 2;
                                msg[imsgLen-2] = '.';
                                msg[imsgLen-1] = '|';
                                msg[imsgLen] = '\0';
                            } else {
                                imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                                    SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand,
                                    LanguageManager->sTexts[LAN_HAS_BEEN_BANNED_BY], curUser->sNick, LanguageManager->sTexts[LAN_BECAUSE_LWR], reason);
                                if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand60-2") == false) {
                   	                return true;
                                }
                            }
                        }
						g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
        	        } else {
                        int imsgLen;
                        if(reason == NULL) {
                            imsgLen = sprintf(msg, "<%s> *** %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick, 
                                LanguageManager->sTexts[LAN_ADDED_LWR], sCommand, LanguageManager->sTexts[LAN_TO_BANS]);
                            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand61") == false) {
                                return true;
                            }
                        } else {
                            if(strlen(reason) > 512) {
                                imsgLen = sprintf(msg, "<%s> *** %s %s %s %s: ", 
                                    SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand, LanguageManager->sTexts[LAN_HAS_BEEN_BANNED_BY],
                                    curUser->sNick, LanguageManager->sTexts[LAN_BECAUSE_LWR]);
                                if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand62-1") == false) {
                   	                return true;
                                }
                                memcpy(msg+imsgLen, reason, 512);
                                imsgLen += (int)strlen(reason) + 2;
                                msg[imsgLen-2] = '.';
                                msg[imsgLen-1] = '|';
                                msg[imsgLen] = '\0';
                            } else {
                                imsgLen = sprintf(msg, "<%s> *** %s %s %s %s: %s.|",
                                    SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand,
                                    LanguageManager->sTexts[LAN_HAS_BEEN_BANNED_BY], curUser->sNick, LanguageManager->sTexts[LAN_BECAUSE_LWR], reason);
                                if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand62-2") == false) {
                   	                return true;
                                }
                            }
                        }

            	        g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                  	}
        		}

        		if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

            	   	int iret = sprintf(msg+imsgLen, "<%s> %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand, 
                       LanguageManager->sTexts[LAN_ADDED_TO_BANS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand64") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
        		}
                return true;
            }

            // Hub commands: !nicktempban nick time reason ...
            // PPK m = minutes, h = hours, d = day, w = weeks, M = months (30 day per month), Y = years (365 day per year)
			if(strncasecmp(sCommand+1, "icktempban ", 11) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::TEMP_BAN) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen < 15) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cnicktempban <%s> <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                        LanguageManager->sTexts[LAN_NICK_LWR], LanguageManager->sTexts[LAN_TIME_LWR], LanguageManager->sTexts[LAN_REASON_LWR], 
                        LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand66") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
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

                if(iCmdPartsLen[0] == 0 || iCmdPartsLen[1] == 0) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                   	int iret = sprintf(msg+imsgLen, "<%s> *** %s %cnicktempban <%s> <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                        LanguageManager->sTexts[LAN_NICK_LWR], LanguageManager->sTexts[LAN_TIME_LWR], LanguageManager->sTexts[LAN_REASON_LWR], 
                        LanguageManager->sTexts[LAN_BAD_PARAMS_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand68") == false) {
                   	    UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                if(iCmdPartsLen[0] > 100) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cnicktempban <%s> <%s> <%s>. %s!|",
                        SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
                        SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR],
                        LanguageManager->sTexts[LAN_TIME_LWR], LanguageManager->sTexts[LAN_REASON_LWR],
                        LanguageManager->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand69") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                // Self-ban ?
				if(strcasecmp(sCmdParts[0], curUser->sNick) == 0) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_YOU_CANT_BAN_YOURSELF]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand70") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
           	    }
           	    
                User *utempban = hashManager->FindUser(sCmdParts[0], iCmdPartsLen[0]);
                if(utempban != NULL) {
                    // PPK don't tempban user with higher profile
                    if(utempban->iProfile != -1 && curUser->iProfile > utempban->iProfile) {
                        int imsgLen = CheckFromPm(curUser, fromPM);
 
                        int iret = sprintf(msg+imsgLen, "<%s> %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                            LanguageManager->sTexts[LAN_YOU_NOT_ALLOWED_TO], LanguageManager->sTexts[LAN_TEMP_BAN_NICK], utempban->sNick);
                        imsgLen += iret;
                        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand72") == true) {
                            UserSendCharDelayed(curUser, msg, imsgLen);
                        }
                        return true;
                    }
                } else {
                    return TempNickBan(curUser, sCmdParts[0], sCmdParts[1], iCmdPartsLen[1], sCmdParts[2], fromPM);
                }

                char cTime = sCmdParts[1][iCmdPartsLen[1]-1];
                sCmdParts[1][iCmdPartsLen[1]-1] = '\0';
                int iTime = atoi(sCmdParts[1]);
                time_t acc_time, ban_time;

                if(iTime <= 0 || GenerateTempBanTime(cTime, (uint32_t)iTime, acc_time, ban_time) == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cnicktempban <%s> <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                        LanguageManager->sTexts[LAN_NICK_LWR], LanguageManager->sTexts[LAN_TIME_LWR], LanguageManager->sTexts[LAN_REASON_LWR], 
                        LanguageManager->sTexts[LAN_BAD_TIME_SPECIFIED]);
                        imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand74") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                if(hashBanManager->NickTempBan(utempban, NULL, sCmdParts[2], curUser->sNick, 0, ban_time) == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_NICK], utempban->sNick,
                        LanguageManager->sTexts[LAN_ALRD_BND_LNGR_TIME_DISCONNECTED]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand77") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }

                    imsgLen = sprintf(msg, "[SYS] Already temp banned user %s (%s) disconnected by %s", utempban->sNick, utempban->sIP, curUser->sNick);
                    if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand78") == true) {
                        UdpDebug->Broadcast(msg, imsgLen);
                    }

                    // Kick user
                    UserClose(utempban);
                    return true;
                }

				UncountDeflood(curUser, fromPM);

                char sTime[256];
                strcpy(sTime, formatTime((ban_time-acc_time)/60));

                // Send user a message that he has been tempbanned
                int imsgLen;
                if(sCmdParts[2] != NULL && iCmdPartsLen[2] > 512) {
                    imsgLen = sprintf(msg, "<%s> %s: %s %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_HAD_BEEN_TEMP_BANNED_TO], sTime,
                        LanguageManager->sTexts[LAN_BECAUSE_LWR]);
                    if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand81-1") == false) {
                   	    return true;
                    }
                    memcpy(msg+imsgLen, sCmdParts[2], 512);
                    imsgLen += iCmdPartsLen[2] + 2;
                    msg[imsgLen-2] = '.';
                    msg[imsgLen-1] = '|';
                    msg[imsgLen] = '\0';
                } else {
                    imsgLen = sprintf(msg, "<%s> %s: %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_HAD_BEEN_TEMP_BANNED_TO], sTime,
                        LanguageManager->sTexts[LAN_BECAUSE_LWR], sCmdParts[2] == NULL ? LanguageManager->sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);
                    if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand81-2") == false) {
                        return true;
                    }
                }

                UserSendChar(utempban, msg, imsgLen);

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                        int imsgLen;
                        if(sCmdParts[2] != NULL && iCmdPartsLen[2] > 512) {
                            imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s %s: %s %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                                SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCmdParts[0],
                                LanguageManager->sTexts[LAN_HAS_BEEN_TMPBND_BY], curUser->sNick, LanguageManager->sTexts[LAN_TO_LWR],
                                sTime, LanguageManager->sTexts[LAN_BECAUSE_LWR]);
                            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand82-1") == false) {
                       	        return true;
                            }
                            memcpy(msg+imsgLen, sCmdParts[2], 512);
                            imsgLen += iCmdPartsLen[2] + 2;
                            msg[imsgLen-2] = '.';
                            msg[imsgLen-1] = '|';
                            msg[imsgLen] = '\0';
                        } else {
                       	    imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s %s: %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                                SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCmdParts[0],
                                LanguageManager->sTexts[LAN_HAS_BEEN_TMPBND_BY], curUser->sNick, LanguageManager->sTexts[LAN_TO_LWR],
                                sTime, LanguageManager->sTexts[LAN_BECAUSE_LWR],
                                sCmdParts[2] == NULL ? LanguageManager->sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);
                            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand82-2") == false) {
                   	            return true;
                            }
                        }

						g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                   	} else {
                        int imsgLen;
                        if(sCmdParts[2] != NULL && iCmdPartsLen[2] > 512) {
                            imsgLen = sprintf(msg, "<%s> *** %s %s %s %s: %s %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCmdParts[0],
                                LanguageManager->sTexts[LAN_HAS_BEEN_TMPBND_BY], curUser->sNick, LanguageManager->sTexts[LAN_TO_LWR],
                                sTime, LanguageManager->sTexts[LAN_BECAUSE_LWR]);
                            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand83-1") == false) {
                       	        return true;
                            }
                            memcpy(msg+imsgLen, sCmdParts[2], 512);
                            imsgLen += iCmdPartsLen[2] + 2;
                            msg[imsgLen-2] = '.';
                            msg[imsgLen-1] = '|';
                            msg[imsgLen] = '\0';
                        } else {
                       	    imsgLen = sprintf(msg, "<%s> *** %s %s %s %s: %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCmdParts[0],
                                LanguageManager->sTexts[LAN_HAS_BEEN_TMPBND_BY], curUser->sNick, LanguageManager->sTexts[LAN_TO_LWR],
                                sTime, LanguageManager->sTexts[LAN_BECAUSE_LWR],
                                sCmdParts[2] == NULL ? LanguageManager->sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);
                            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand83-2") == false) {
                   	            return true;
                            }
                        }

                        g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                    }
                }

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    if(sCmdParts[2] != NULL && iCmdPartsLen[2] > 512) {
                   	    int iret = sprintf(msg+imsgLen, "<%s> %s %s: %s %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCmdParts[0],
                            LanguageManager->sTexts[LAN_BEEN_TEMP_BANNED_TO], sTime, LanguageManager->sTexts[LAN_BECAUSE_LWR]);
                        imsgLen += iret;
                        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand85-1") == false) {
                            return true;
                        }

                        memcpy(msg+imsgLen, sCmdParts[2], 512);
                        imsgLen += iCmdPartsLen[2] + 2;
                        msg[imsgLen-2] = '.';
                        msg[imsgLen-1] = '|';
                        msg[imsgLen] = '\0';
                    } else {
                   	    int iret = sprintf(msg+imsgLen, "<%s> %s %s: %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCmdParts[0],
                            LanguageManager->sTexts[LAN_BEEN_TEMP_BANNED_TO], sTime, LanguageManager->sTexts[LAN_BECAUSE_LWR],
                            sCmdParts[2] == NULL ? LanguageManager->sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);
                        imsgLen += iret;
                        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand85-2") == false) {
                            return true;
                        }
                    }

                    UserSendCharDelayed(curUser, msg, imsgLen);
                 }

                imsgLen = sprintf(msg, "[SYS] User %s (%s) tempbanned by %s", utempban->sNick, utempban->sIP, curUser->sNick);
                UserClose(utempban);
                if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand86") == false) {
                    return true;
                }

                UdpDebug->Broadcast(msg, imsgLen);
                return true;
            }

            return false;

        case 'b':
            // Hub commands: !banip ip reason
			if(strncasecmp(sCommand+1, "anip ", 5) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::BAN) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen < 12) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> %s %cbanip <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],  SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                        LanguageManager->sTexts[LAN_IP], LanguageManager->sTexts[LAN_REASON_LWR], 
                        LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand89") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                return BanIp(curUser, sCommand+6, fromPM, false);
            }

			if(strncasecmp(sCommand+1, "an ", 3) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::BAN) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen < 5) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cban <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                        LanguageManager->sTexts[LAN_NICK_LWR], LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand91") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }
                
                return Ban(curUser, sCommand+4, fromPM, false);
            }

            return false;

        case 't':
            // Hub commands: !tempban nick time reason ... PPK m = minutes, h = hours, d = day, w = weeks, M = months (30 day per month), Y = years (365 day per year)
			if(strncasecmp(sCommand+1, "empban ", 7) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::TEMP_BAN) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen < 11) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s %ctempban <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                        LanguageManager->sTexts[LAN_NICK_LWR], LanguageManager->sTexts[LAN_TIME_LWR], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand93") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                return TempBan(curUser, sCommand+8, dlen-8, fromPM, false);
            }

            // !tempbanip nick time reason ... PPK m = minutes, h = hours, d = day, w = weeks, M = months (30 day per month), Y = years (365 day per year)
			if(strncasecmp(sCommand+1, "empbanip ", 9) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::TEMP_BAN) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen < 14) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s %ctempbanip <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                        LanguageManager->sTexts[LAN_IP], LanguageManager->sTexts[LAN_TIME_LWR], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand95") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                return TempBanIp(curUser, sCommand+10, dlen-10, fromPM, false);
            }

            // Hub commands: !tempunban
			if(strncasecmp(sCommand+1, "empunban ", 9) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::TEMP_UNBAN) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen < 11 || sCommand[10] == '\0') {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %ctempunban <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                        LanguageManager->sTexts[LAN_IP_OR_NICK], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand97") == false) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                sCommand += 10;

                if(dlen > 100) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %ctempunban <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        LanguageManager->sTexts[LAN_IP_OR_NICK], LanguageManager->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand98") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                if(hashBanManager->TempUnban(sCommand) == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_SORRY], sCommand, LanguageManager->sTexts[LAN_IS_NOT_IN_MY_TEMP_BANS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand99") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

				UncountDeflood(curUser, fromPM);

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                   	if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
           	            int imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                            SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick, 
                            LanguageManager->sTexts[LAN_REMOVED_LWR], sCommand, LanguageManager->sTexts[LAN_FROM_TEMP_BANS]);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand100") == true) {
                            g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                        }
              	    } else {
                   	    int imsgLen = sprintf(msg, "<%s> *** %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick, 
                            LanguageManager->sTexts[LAN_REMOVED_LWR], sCommand, LanguageManager->sTexts[LAN_FROM_TEMP_BANS]);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand101") == true) {
          	        	    g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                        }
          	    	}
                }

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand, 
                        LanguageManager->sTexts[LAN_REMOVED_FROM_TEMP_BANS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand103") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                }
                return true;
            }

            //Hub-Commands: !topic
			if(strncasecmp(sCommand+1, "opic ", 5) == 0 ) {
           	    if(ProfileMan->IsAllowed(curUser, ProfileManager::TOPIC) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen < 7 || sCommand[6] == '\0') {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                   	int iret = sprintf(msg+imsgLen, "<%s> *** %s %ctopic <%s>, %ctopic <off>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                        LanguageManager->sTexts[LAN_NEW_TOPIC], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                        LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand105") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                sCommand += 6;

				UncountDeflood(curUser, fromPM);

                if(dlen-6 > 256) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                   	int iret = sprintf(msg+imsgLen, "<%s> *** %s %ctopic <%s>, %ctopic <off>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        LanguageManager->sTexts[LAN_NEW_TOPIC], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        LanguageManager->sTexts[LAN_MAX_ALWD_TOPIC_LEN_256_CHARS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand106") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

				if(strcasecmp(sCommand, "off") == 0) {
                    SettingManager->SetText(SETTXT_HUB_TOPIC, "", 0);

                    g_GlobalDataQueue->AddQueueItem(SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_NAME], SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_HUB_NAME], NULL, 0,
                        GlobalDataQueue::CMD_HUBNAME);

                    int imsgLen = CheckFromPm(curUser, fromPM);

        			int iret = sprintf(msg+imsgLen, "<%s> *** %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_TOPIC_HAS_BEEN_CLEARED]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand108") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                } else {
                    SettingManager->SetText(SETTXT_HUB_TOPIC, sCommand, dlen-6);

                    g_GlobalDataQueue->AddQueueItem(SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_NAME], SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_HUB_NAME], NULL, 0,
                        GlobalDataQueue::CMD_HUBNAME);

                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_TOPIC_HAS_BEEN_SET_TO], sCommand);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand111") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                }

                return true;
            }

            return false;

        case 'm':          
            //Hub-Commands: !myip
			if(dlen == 4 && strncasecmp(sCommand+1, "yip", 3) == 0) {
                int imsgLen = CheckFromPm(curUser, fromPM), iret = 0;

                if(curUser->sIPv4[0] != '\0') {
                    iret = sprintf(msg+imsgLen, "<%s> *** %s: %s / %s|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        LanguageManager->sTexts[LAN_YOUR_IP_IS], curUser->sIP, curUser->sIPv4);
                } else {
                    iret = sprintf(msg+imsgLen, "<%s> *** %s: %s|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        LanguageManager->sTexts[LAN_YOUR_IP_IS], curUser->sIP);
                }

                imsgLen += iret;
                if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand113") == true) {
                    UserSendCharDelayed(curUser, msg, imsgLen);
                }

                return true;
            }

            // Hub commands: !massmsg text
			if(strncasecmp(sCommand+1, "assmsg ", 7) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::MASSMSG) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen < 9) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cmassmsg <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                        LanguageManager->sTexts[LAN_MESSAGE_LWR], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand115") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

				UncountDeflood(curUser, fromPM);

                if(dlen > 64000) {
                    sCommand[64000] = '\0';
                }

                int imsgLen = sprintf(g_sBuffer, "%s $<%s> %s|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick, sCommand+8);
                if(CheckSprintf(imsgLen, g_szBufferSize, "HubCommands::DoCommand117") == true) {
					g_GlobalDataQueue->SingleItemStore(g_sBuffer, imsgLen, curUser, 0, GlobalDataQueue::SI_PM2ALL);
                }

                imsgLen = CheckFromPm(curUser, fromPM);

                int iret = sprintf(msg+imsgLen, "<%s> *** %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                    LanguageManager->sTexts[LAN_MASSMSG_TO_ALL_SENT]);
                imsgLen += iret;
                if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand119") == true) {
                    UserSendCharDelayed(curUser, msg, imsgLen);
                }
                return true;
            }
#ifdef _WIN32
            //Hub-Commands: !memstat !memstats
            if((dlen == 7 && strnicmp(sCommand+1, "emstat", 6) == 0) || (dlen == 8 && strnicmp(sCommand+1, "emstats", 7) == 0)) {
                int imsglen = CheckFromPm(curUser, fromPM);

                int iret = sprintf(msg+imsglen, "<%s>", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
                imsglen += iret;
                if(CheckSprintf1(iret, imsglen, 1024, "HubCommands::DoCommand memstat1") == false) {
                    return true;
                }

				string Statinfo(msg, imsglen);

                Statinfo+= "\n------------------------------------------------------------\n";
                Statinfo+="Current memory stats:\n";
                Statinfo+="------------------------------------------------------------\n";

                DWORD dwCommitted = 0, dwUnCommitted = 0;
                GetHeapStats(hPtokaXHeap, dwCommitted, dwUnCommitted);

				Statinfo+="PX total commited: "+string((uint32_t)dwCommitted)+ "\n";
				Statinfo+="PX total uncommited: "+string((uint32_t)dwUnCommitted)+ "\n";

                dwCommitted = 0, dwUnCommitted = 0;
                GetHeapStats(hRecvHeap, dwCommitted, dwUnCommitted);
				Statinfo+="Recv total commited: "+string((uint32_t)dwCommitted)+ "\n";
				Statinfo+="Recv total uncommited: "+string((uint32_t)dwUnCommitted)+ "\n";

                dwCommitted = 0, dwUnCommitted = 0;
                GetHeapStats(hSendHeap, dwCommitted, dwUnCommitted);
				Statinfo+="Send total commited: "+string((uint32_t)dwCommitted)+ "\n";
				Statinfo+="Send total uncommited: "+string((uint32_t)dwUnCommitted)+ "\n";

                dwCommitted = 0, dwUnCommitted = 0;
                GetHeapStats(hLuaHeap, dwCommitted, dwUnCommitted);
				Statinfo+="Lua total commited: "+string((uint32_t)dwCommitted)+ "\n";
				Statinfo+="Lua total uncommited: "+string((uint32_t)dwUnCommitted)+ "\n";

                Statinfo+="|";
				UserSendTextDelayed(curUser, Statinfo);
                return true;
            }
#endif
            return false;

        case 'r':
			if(dlen == 14 && strncasecmp(sCommand+1, "estartscripts", 13) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::RSTSCRIPTS) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(SettingManager->bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_ERR_SCRIPTS_DISABLED]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand121") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

				UncountDeflood(curUser, fromPM);

                // post restart message
                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                        int imsgLen = sprintf(msg, "%s $<%s> *** %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                            SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick, LanguageManager->sTexts[LAN_RESTARTED_SCRIPTS]);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand122") == true) {
							g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                        }
           	        } else {
                        int imsgLen = sprintf(msg, "<%s> *** %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick, 
                            LanguageManager->sTexts[LAN_RESTARTED_SCRIPTS]);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand123") == true) {
               	            g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                        }
               	    }
                }

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_SCRIPTS_RESTARTED]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand125") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                }

				ScriptManager->Restart();

                return true;
            }

			if(dlen == 7 && strncasecmp(sCommand+1, "estart", 6) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::RSTHUB) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

				UncountDeflood(curUser, fromPM);

                // Send message to all that we are going to restart the hub
                int imsgLen = sprintf(msg,"<%s> %s. %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                    LanguageManager->sTexts[LAN_HUB_WILL_BE_RESTARTED], LanguageManager->sTexts[LAN_BACK_IN_FEW_MINUTES]);
                if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand126") == true) {
                    colUsers->SendChat2All(curUser, msg, imsgLen, NULL);
                }

                // post a restart hub message
                eventqueue->AddNormal(eventq::EVENT_RESTART, NULL);
                return true;
            }

            //Hub-Commands: !reloadtxt
			if(dlen == 9 && strncasecmp(sCommand+1, "eloadtxt", 8) == 0) {
           	    if(ProfileMan->IsAllowed(curUser, ProfileManager::REFRESHTXT) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

               	if(SettingManager->bBools[SETBOOL_ENABLE_TEXT_FILES] == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                  	int iret = sprintf(msg+imsgLen, "<%s> %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_TXT_SUP_NOT_ENABLED]);
                  	imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand128") == true) {
        	           UserSendCharDelayed(curUser, msg, imsgLen);
                    }
        	        return true;
                }

				UncountDeflood(curUser, fromPM);

               	TextFileManager->RefreshTextFiles();

               	if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
              	      	int imsgLen = sprintf(msg, "%s $<%s> *** %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                            SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick, LanguageManager->sTexts[LAN_RELOAD_TXT_FILES_LWR]);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand129") == true) {
							g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                        }
        	        } else {
            	      	int imsgLen = sprintf(msg, "<%s> *** %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick, 
                            LanguageManager->sTexts[LAN_RELOAD_TXT_FILES_LWR]);
            	      	if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand130") == true) {
                            g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                        }
                    }
                }

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                  	int iret = sprintf(msg+imsgLen, "<%s> %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_TXT_FILES_RELOADED]);
                  	imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand132") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                }
                return true;
            }

			// Hub commands: !restartscript scriptname
			if(strncasecmp(sCommand+1, "estartscript ", 13) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::RSTSCRIPTS) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(SettingManager->bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_ERR_SCRIPTS_DISABLED]);
                  	imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand134") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                if(dlen < 15 || sCommand[14] == '\0') {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %crestartscript <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                        LanguageManager->sTexts[LAN_SCRIPTNAME_LWR], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                  	imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand136") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                sCommand += 14;

                if(dlen-14 > 256) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %crestartscript <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        LanguageManager->sTexts[LAN_SCRIPTNAME_LWR], LanguageManager->sTexts[LAN_MAX_ALWD_SCRIPT_NAME_LEN_256_CHARS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand137") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                Script * curScript = ScriptManager->FindScript(sCommand);
                if(curScript == NULL || curScript->bEnabled == false || curScript->LUA == NULL) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_ERROR_SCRIPT], sCommand, LanguageManager->sTexts[LAN_NOT_RUNNING]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand138") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
				}

				UncountDeflood(curUser, fromPM);

				// stop script
				ScriptManager->StopScript(curScript, false);

				// try to start script
				if(ScriptManager->StartScript(curScript, false) == true) {
                    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                        if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                            int imsgLen = sprintf(msg, "%s $<%s> *** %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                                SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick, 
                                LanguageManager->sTexts[LAN_RESTARTED_SCRIPT], sCommand);
                            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand139") == true) {
								g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                            }
                        } else {
                            int imsgLen = sprintf(msg, "<%s> *** %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick,
                                LanguageManager->sTexts[LAN_RESTARTED_SCRIPT], sCommand);
                            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand140") == true) {
                                g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                            }
                        }
                    }

                    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || 
                        ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                        int imsgLen = CheckFromPm(curUser, fromPM);

                        int iret = sprintf(msg+imsgLen, "<%s> %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                            LanguageManager->sTexts[LAN_SCRIPT], sCommand, LanguageManager->sTexts[LAN_RESTARTED_LWR]);
                        imsgLen += iret;
                        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand142") == true) {
                            UserSendCharDelayed(curUser, msg, imsgLen);
                        }
                    } 
                    return true;
				} else {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_ERROR_SCRIPT], sCommand, LanguageManager->sTexts[LAN_RESTART_FAILED]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand144") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }
            }

            // Hub commands: !rangeban fromip toip reason
			if(strncasecmp(sCommand+1, "angeban ", 8) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::RANGE_BAN) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen < 24) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s %crangeban <%s> <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_FROMIP], 
                        LanguageManager->sTexts[LAN_TOIP], LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand148") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                return RangeBan(curUser, sCommand+9, dlen-9, fromPM, false);
            }

            // Hub commands: !rangetempban fromip toip time reason
			if(strncasecmp(sCommand+1, "angetempban ", 12) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::RANGE_TBAN) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen < 31) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s %crangetempban <%s> <%s> <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                       LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_FROMIP], 
                       LanguageManager->sTexts[LAN_TOIP],  LanguageManager->sTexts[LAN_TIME_LWR], LanguageManager->sTexts[LAN_REASON_LWR], 
                       LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand150") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                return RangeTempBan(curUser, sCommand+13, dlen-13, fromPM, false);
            }

            // Hub commands: !rangeunban fromip toip
			if(strncasecmp(sCommand+1, "angeunban ", 10) == 0) {
                if(dlen < 26) {
                    if(ProfileMan->IsAllowed(curUser, ProfileManager::RANGE_UNBAN) == false && ProfileMan->IsAllowed(curUser, ProfileManager::RANGE_TUNBAN) == false) {
                        SendNoPermission(curUser, fromPM);
                        return true;
                    }

                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s %crangeunban <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                       LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                       LanguageManager->sTexts[LAN_FROMIP], LanguageManager->sTexts[LAN_TOIP], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand152") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::RANGE_UNBAN) == true) {
                    return RangeUnban(curUser, sCommand+11, fromPM);
                } else if(ProfileMan->IsAllowed(curUser, ProfileManager::RANGE_TUNBAN) == true) {
                    return RangeUnban(curUser, sCommand+11, fromPM, hashBanMan::TEMP);
                } else {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }
            }

            // Hub commands: !rangetempunban fromip toip
			if(strncasecmp(sCommand+1, "angetempunban ", 14) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::RANGE_TUNBAN) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen < 30) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s %crangetempunban <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                       LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                       LanguageManager->sTexts[LAN_FROMIP], LanguageManager->sTexts[LAN_TOIP], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand154") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                return RangeUnban(curUser, sCommand+15, fromPM, hashBanMan::TEMP);
            }

            // Hub commands: !rangepermunban fromip toip
			if(strncasecmp(sCommand+1, "angepermunban ", 14) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::RANGE_UNBAN) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen < 30) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s %crangepermunban <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                       LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                       LanguageManager->sTexts[LAN_FROMIP], LanguageManager->sTexts[LAN_TOIP], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand156") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                return RangeUnban(curUser, sCommand+15, fromPM, hashBanMan::PERM);
            }

            // !reguser <nick> <profile_name>
			if(strncasecmp(sCommand+1, "eguser ", 7) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::ADDREGUSER) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen > 255) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_CMD_TOO_LONG]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand::RegUser1") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                char * sNick = sCommand+8; // nick start

                char * sProfile = strchr(sCommand+8, ' ');
                if(sProfile == NULL) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s %creguser <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR],
                        LanguageManager->sTexts[LAN_PROFILENAME_LWR], LanguageManager->sTexts[LAN_BAD_PARAMS_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand::RegUser2") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                sProfile[0] = '\0';
                sProfile++;

                int iProfile = ProfileMan->GetProfileIndex(sProfile);
                if(iProfile == -1) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_ERR_NO_PROFILE_GIVEN_NAME_EXIST]);
               	    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand::RegUser3") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                // check hierarchy
                // deny if curUser is not Master and tries add equal or higher profile
                if(curUser->iProfile > 0 && iProfile <= curUser->iProfile) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_NOT_ALLOWED_TO_ADD_USER_THIS_PROFILE]);
               	    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand::RegUser4") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                size_t szNickLen = strlen(sNick);

                // check if user is registered
                if(hashRegManager->Find(sNick, szNickLen) != NULL) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_USER], sNick,
                        LanguageManager->sTexts[LAN_IS_ALREDY_REGISTERED]);
               	    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand::RegUser5") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                User * pUser = hashManager->FindUser(sNick, szNickLen);
                if(pUser == NULL) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_ERROR], sNick,
                        LanguageManager->sTexts[LAN_IS_NOT_ONLINE]);
               	    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand::RegUser6") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

				UncountDeflood(curUser, fromPM);

                if(pUser->uLogInOut == NULL) {
                    pUser->uLogInOut = new LoginLogout();
                    if(pUser->uLogInOut == NULL) {
                        pUser->ui32BoolBits |= User::BIT_ERROR;
                        UserClose(pUser);

                        AppendDebugLog("%s - [MEM] Cannot allocate new pUser->uLogInOut in HubCommands::DoCommand::RegUser\n", 0);
                        return true;
                    }
                }

                UserSetPasswd(pUser, sProfile);
                pUser->ui32BoolBits |= User::BIT_WAITING_FOR_PASS;

                int iMsgLen = sprintf(msg, "<%s> %s.|$GetPass|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_WERE_REGISTERED_PLEASE_ENTER_YOUR_PASSWORD]);
                if(CheckSprintf(iMsgLen, 1024, "HubCommands::DoCommand::RegUser7") == true) {
                    UserSendCharDelayed(pUser, msg, iMsgLen);
                }

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                        int imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],  SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                            curUser->sNick, LanguageManager->sTexts[LAN_REGISTERED], sNick, LanguageManager->sTexts[LAN_AS], sProfile);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand::RegUser8") == true) {
							g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                        }
                    } else {
                        int imsgLen = sprintf(msg, "<%s> *** %s %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick,
                            LanguageManager->sTexts[LAN_REGISTERED], sNick, LanguageManager->sTexts[LAN_AS], sProfile);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand::RegUser9") == true) {
                            g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                        }
                    }
                }

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sNick, LanguageManager->sTexts[LAN_REGISTERED],
                        LanguageManager->sTexts[LAN_AS], sProfile);
               	    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand::RegUser10") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                }

                return true;
            }

            return false;

        case 'i':
            if(((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
/*                // !ipinfo
                if(strnicmp(sCommand+1, "pinfo ", 6) == 0) {
               	    if(sqldb == NULL) {
                        int imsgLen = CheckFromPm(curUser, fromPM);

                        imsgLen += sprintf(msg+imsgLen, "<%s> *** UserStatistics are not running.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
                        UserSendCharDelayed(curUser, msg, imsgLen);
                        return true;
                    }

                    if(dlen < 8) {
                        int imsgLen = CheckFromPm(curUser, fromPM);

                        imsgLen += sprintf(msg+imsgLen, "<%s> *** Syntax error in command %cipinfo <ip>: No parameter given.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0]);
                        UserSendCharDelayed(curUser, msg, imsgLen);
                        return true;
                   	}
                   	curUser->iChatMsgs--;
                    sCommand += 7; // move to command parameter

                    uint32_t a, b, c, d;
                    if(sCommand[0] != 0 && GetIpParts(sCommand, a, b, c, d) == true) {
                        sqldb->GetUsersByIp(sCommand, curUser);
                        return true;
                    }
                }

                if(strnicmp(sCommand+1, "prangeinfo ", 11) == 0) {
                    if(hubForm->UserStatistics == NULL) {
                        int imsgLen = CheckFromPm(curUser, fromPM);

                        imsgLen += sprintf(msg+imsgLen, "<%s> *** UserStatistics are not running.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
                        UserSendCharDelayed(curUser, msg, imsgLen);
                        return true;
                    }

                    if(dlen < 13) {
                        int imsgLen = CheckFromPm(curUser, fromPM);

                        imsgLen += sprintf(msg+imsgLen, "<%s> *** Syntax error in command %ciprangeinfo <ip_part>: No parameter given.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0]);
                        UserSendCharDelayed(curUser, msg, imsgLen);
                        return true;
                   	}
                    curUser->iChatMsgs--;
                    sCommand += 12;

                    if(sCommand[0] != 0) {
                        TStringList *ReturnList=new TStringList();
                        hubForm->UserStatistics->GetUsersByIPRange(ReturnList, sCommand);
                        String IpInfo;
                        if(fromPM == true) {
                            int imsglen = sprintf(msg, "$To: %s From: %s $<%s> IP-info:\n", curUser->Nick, SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                                SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
                            IpInfo += String(msg, imsglen);
                        }
                        for(uint32_t actPos=0; actPos<ReturnList->Count; actPos++) {
                            IpInfo += ReturnList->Strings[actPos]+ "\n";
                        }

                        IpInfo += "|";

                        curUser->SendTextDelayed(IpInfo);
                        ReturnList->Clear();
                        delete ReturnList;
                        return true;
                    }
                }*/
            }

            return false;

        case 'u':
/*            if(((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                // !userinfo
                if(strnicmp(sCommand+1, "serinfo ", 8) == 0) {
               	    if(hubForm->UserStatistics == NULL) {
                        int imsgLen = CheckFromPm(curUser, fromPM);

                        imsgLen += sprintf(msg+imsgLen, "<%s> *** UserStatistics are not running.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
                        UserSendCharDelayed(curUser, msg, imsgLen);
                        return true;
                    }

                    if(dlen < 10) {
                        int imsgLen = CheckFromPm(curUser, fromPM);

                        imsgLen += sprintf(msg+imsgLen, "<%s> *** Syntax error in command %cuserinfo <username>: No parameter given.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0]);
                        UserSendCharDelayed(curUser, msg, imsgLen);
                        return true;
                   	}
                    curUser->iChatMsgs--;
                    sCommand += 9;

                    if(sCommand[0] != 0) {
               	        TStringList *ReturnList=new TStringList();
                   	    hubForm->UserStatistics->GetIPsByUsername(ReturnList, sCommand);
                   	    String UserInfo;
                        if(fromPM == true) {
                            int imsglen = sprintf(msg, "$To: %s From: %s $<%s> User-info:\n", curUser->Nick, SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                                SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
                            UserInfo += String(msg, imsglen);
                        }
                        for(uint32_t actPos=0; actPos<ReturnList->Count; actPos++) {
           	                UserInfo += ReturnList->Strings[actPos]+ "\n";
                        }

                        UserInfo += "|";

                        curUser->SendTextDelayed(UserInfo);
           	            ReturnList->Clear();
                        delete ReturnList;
                        return true;
                    }
                }
            }*/

            // Hub commands: !unban
			if(strncasecmp(sCommand+1, "nban ", 5) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::UNBAN) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen < 7 || sCommand[6] == '\0') {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cunban <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                        LanguageManager->sTexts[LAN_IP_OR_NICK], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand158") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                sCommand += 6;

                if(dlen > 100) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cunban <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        LanguageManager->sTexts[LAN_IP_OR_NICK], LanguageManager->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand159") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                if(hashBanManager->Unban(sCommand) == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_SORRY], sCommand, LanguageManager->sTexts[LAN_IS_NOT_IN_BANS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand160") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

				UncountDeflood(curUser, fromPM);

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                   	if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
           	            int imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                            SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick, LanguageManager->sTexts[LAN_REMOVED_LWR], 
                            sCommand, LanguageManager->sTexts[LAN_FROM_BANS]);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand161") == true) {
                            g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                        }
              	    } else {
                   	    int imsgLen = sprintf(msg, "<%s> *** %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick, 
                            LanguageManager->sTexts[LAN_REMOVED_LWR], sCommand, LanguageManager->sTexts[LAN_FROM_BANS]);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand162") == true) {
          	        	    g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                        }
          	    	}
                }

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || 
                    ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand, 
                        LanguageManager->sTexts[LAN_REMOVED_FROM_BANS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand164") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                }

                return true;
            }

            // Hub commands: !ungag
			if(strncasecmp(sCommand+1, "ngag ", 5) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::GAG) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

               	if(dlen < 7 || sCommand[6] == '\0') {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                   	int iret = sprintf(msg+imsgLen, "<%s> *** %s %cungag <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                        LanguageManager->sTexts[LAN_NICK_LWR], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand166") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                sCommand += 6;

                if(dlen > 100) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cungag <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        LanguageManager->sTexts[LAN_NICK_LWR], LanguageManager->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand167") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                User *user = hashManager->FindUser(sCommand, dlen-6);
                if(user == NULL) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_ERROR], sCommand, LanguageManager->sTexts[LAN_IS_NOT_IN_USERLIST]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand168") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                if(((user->ui32BoolBits & User::BIT_GAGGED) == User::BIT_GAGGED) == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

        			int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_ERROR], sCommand, LanguageManager->sTexts[LAN_IS_NOT_GAGGED]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand170") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
        			return true;
                }

				UncountDeflood(curUser, fromPM);

                user->ui32BoolBits &= ~User::BIT_GAGGED;
                int imsgLen = sprintf(msg, "<%s> %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                    LanguageManager->sTexts[LAN_YOU_ARE_UNGAGGED_BY], curUser->sNick);
                if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand171") == true) {
                    UserSendCharDelayed(user, msg, imsgLen);
                }

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
         	    	if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
          	    	    imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                            SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick, 
                            LanguageManager->sTexts[LAN_HAS_UNGAGGED], user->sNick);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand172") == true) {
							g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                        }
         	        } else {
                  		imsgLen = sprintf(msg, "<%s> *** %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick,
                            LanguageManager->sTexts[LAN_HAS_UNGAGGED], user->sNick);
                  		if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand173") == true) {
                            g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                        }
          		    }
                }

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || 
                    ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    imsgLen = CheckFromPm(curUser, fromPM);

        			int iret = sprintf(msg+imsgLen, "<%s> %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], user->sNick, 
                        LanguageManager->sTexts[LAN_HAS_UNGAGGED]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand175") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                }

                return true;
            }

            return false;

        case 'o':
            // Hub commands: !op nick
			if(strncasecmp(sCommand+1, "p ", 2) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::TEMPOP) == false || ((curUser->ui32BoolBits & User::BIT_TEMP_OPERATOR) == User::BIT_TEMP_OPERATOR) == true) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen < 4 || sCommand[3] == '\0') {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cop <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                        LanguageManager->sTexts[LAN_NICK_LWR], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand177") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                sCommand += 3;

                if(dlen > 100) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cop <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        LanguageManager->sTexts[LAN_NICK_LWR], LanguageManager->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand178") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                User *user = hashManager->FindUser(sCommand, dlen-3);
                if(user == NULL) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                   	int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_ERROR], sCommand, LanguageManager->sTexts[LAN_IS_NOT_IN_USERLIST]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand179") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                if(((user->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                   	int iret = sprintf(msg+imsgLen, "<%s> *** %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], user->sNick, 
                        LanguageManager->sTexts[LAN_ALREDY_IS_OP]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand181") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

				UncountDeflood(curUser, fromPM);

                int pidx = ProfileMan->GetProfileIndex("Operator");
                if(pidx == -1) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                   	int iret = sprintf(msg+imsgLen, "<%s> *** %s. %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_ERROR], LanguageManager->sTexts[LAN_OPERATOR_PROFILE_MISSING]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand183") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                user->ui32BoolBits |= User::BIT_OPERATOR;
                bool bAllowedOpChat = ProfileMan->IsAllowed(user, ProfileManager::ALLOWEDOPCHAT);
                user->iProfile = pidx;
                user->ui32BoolBits |= User::BIT_TEMP_OPERATOR; // PPK ... to disallow adding more tempop by tempop user ;)
                colUsers->Add2OpList(user);

                int imsgLen = 0;
                if(((user->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == false) {
                    imsgLen = sprintf(msg, "$LogedIn %s|", user->sNick);
                    if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand184") == false) {
                        return true;
                    }
                }
                int iret = sprintf(msg+imsgLen, "<%s> *** %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_GOT_TEMP_OP]);
                imsgLen += iret;
                if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand185") == true) {
                    UserSendCharDelayed(user, msg, imsgLen);
                }
                g_GlobalDataQueue->OpListStore(user->sNick);

                if(bAllowedOpChat != ProfileMan->IsAllowed(user, ProfileManager::ALLOWEDOPCHAT)) {
                    if(SettingManager->bBools[SETBOOL_REG_OP_CHAT] == true &&
                        (SettingManager->bBools[SETBOOL_REG_BOT] == false || SettingManager->bBotsSameNick == false)) {
                        if(((user->ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == false) {
                            UserSendCharDelayed(user, SettingManager->sPreTexts[SetMan::SETPRETXT_OP_CHAT_HELLO],
                                SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_OP_CHAT_HELLO]);
                        }
                        UserSendCharDelayed(user, SettingManager->sPreTexts[SetMan::SETPRETXT_OP_CHAT_MYINFO],
                            SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_OP_CHAT_MYINFO]);
                        imsgLen = sprintf(msg, "$OpList %s$$|", SettingManager->sTexts[SETTXT_OP_CHAT_NICK]);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand186") == true) {
                            UserSendCharDelayed(user, msg, imsgLen);
                        }
                    }
                }

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                        imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                            SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick, 
                            LanguageManager->sTexts[LAN_SETS_OP_MODE_TO], user->sNick);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand187") == true) {
							g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                        }
                    } else {
                        imsgLen = sprintf(msg, "<%s> *** %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick,
                            LanguageManager->sTexts[LAN_SETS_OP_MODE_TO], user->sNick);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand188") == true) {
                            g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                        }
                    }
                }

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || 
                    ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    imsgLen = CheckFromPm(curUser, fromPM);

                   	int iret = sprintf(msg+imsgLen, "<%s> *** %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], user->sNick, 
                        LanguageManager->sTexts[LAN_GOT_OP_STATUS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand190") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                }

                return true;
            }

            // Hub commands: !opmassmsg text
			if(strncasecmp(sCommand+1, "pmassmsg ", 9) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::MASSMSG) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen < 11) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %copmassmsg <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                        LanguageManager->sTexts[LAN_MESSAGE_LWR], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand192") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

				UncountDeflood(curUser, fromPM);

                if(dlen > 64000) {
                    sCommand[64000] = '\0';
                }

                int imsgLen = sprintf(g_sBuffer, "%s $<%s> %s|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick, sCommand+10);
                if(CheckSprintf(imsgLen, g_szBufferSize, "HubCommands::DoCommand193") == true) {
					g_GlobalDataQueue->SingleItemStore(g_sBuffer, imsgLen, curUser, 0, GlobalDataQueue::SI_PM2OPS);
                }

                imsgLen = CheckFromPm(curUser, fromPM);

                int iret = sprintf(msg+imsgLen, "<%s> *** %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                    LanguageManager->sTexts[LAN_MASSMSG_TO_OPS_SND]);
                imsgLen += iret;
                if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand196") == true) {
                    UserSendCharDelayed(curUser, msg, imsgLen);
                }
                return true;
            }

            return false;

        case 'd':
            // Hub commands: !drop
			if(strncasecmp(sCommand+1, "rop ", 4) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::DROP) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen < 6) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cdrop <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                        LanguageManager->sTexts[LAN_NICK_LWR], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand198") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                sCommand += 5;
                size_t szNickLen;

                char *reason = strchr(sCommand, ' ');
                if(reason != NULL) {
                    szNickLen = reason-sCommand;
                    reason[0] = '\0';
                    if(reason[1] == '\0') {
                        reason = NULL;
                    } else {
                        reason++;
                    }
                } else {
                    szNickLen = dlen-5;
                }

                if(szNickLen > 100) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cdrop <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        LanguageManager->sTexts[LAN_NICK_LWR], LanguageManager->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand199") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                if(sCommand[0] == '\0') {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cdrop <%s>. %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                       LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                       LanguageManager->sTexts[LAN_NICK_LWR], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand200") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }
                
                // Self-drop ?
				if(strcasecmp(sCommand, curUser->sNick) == 0) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret= sprintf(msg+imsgLen, "<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_YOU_CANT_DROP_YOURSELF]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand202") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
           	    }
           	    
                User *user = hashManager->FindUser(sCommand, szNickLen);
                if(user == NULL) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                   	int iret = sprintf(msg+imsgLen, "<%s> *** %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand, 
                        LanguageManager->sTexts[LAN_IS_NOT_IN_USERLIST]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand204") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }
                
        		// PPK don't drop user with higher profile
        		if(user->iProfile != -1 && curUser->iProfile > user->iProfile) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        LanguageManager->sTexts[LAN_YOU_NOT_ALLOWED_TO], LanguageManager->sTexts[LAN_DROP_LWR], user->sNick);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand206") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
        			return true;
        		}

				UncountDeflood(curUser, fromPM);

                hashBanManager->TempBan(user, reason, curUser->sNick, 0, 0, false);

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
               	    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                        int imsgLen;
                        if(reason != NULL && strlen(reason) > 512) {
                            imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s %s %s %s: |", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                            SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand, LanguageManager->sTexts[LAN_WITH_IP], user->sIP,
                            LanguageManager->sTexts[LAN_DROP_ADDED_TEMPBAN_BY], curUser->sNick, LanguageManager->sTexts[LAN_BECAUSE_LWR]);
                            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand207-1") == false) {
                       	        return true;
                            }
                            memcpy(msg+imsgLen, reason, 512);
                            imsgLen += (int)strlen(reason) + 2;
                            msg[imsgLen-2] = '.';
                            msg[imsgLen-1] = '|';
                            msg[imsgLen] = '\0';
                        } else {
        	       		    imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s %s %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                                SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand, LanguageManager->sTexts[LAN_WITH_IP], user->sIP,
                                LanguageManager->sTexts[LAN_DROP_ADDED_TEMPBAN_BY], curUser->sNick, LanguageManager->sTexts[LAN_BECAUSE_LWR],
                                reason == NULL ? LanguageManager->sTexts[LAN_NO_REASON_SPECIFIED] : reason);
                            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand207-2") == false) {
                       	        return true;
                            }
                        }

						g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
         	    	} else {
                        int imsgLen;
                        if(reason != NULL && strlen(reason) > 512) {
                            imsgLen = sprintf(msg, "<%s> *** %s %s %s %s %s %s: |", 
                            SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand, LanguageManager->sTexts[LAN_WITH_IP], user->sIP,
                            LanguageManager->sTexts[LAN_DROP_ADDED_TEMPBAN_BY], curUser->sNick, LanguageManager->sTexts[LAN_BECAUSE_LWR]);
                            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand207-1") == false) {
                       	        return true;
                            }
                            memcpy(msg+imsgLen, reason, 512);
                            imsgLen += (int)strlen(reason) + 2;
                            msg[imsgLen-2] = '.';
                            msg[imsgLen-1] = '|';
                            msg[imsgLen] = '\0';
                        } else {
        	       		    imsgLen = sprintf(msg, "<%s> *** %s %s %s %s %s %s: %s.|", 
                                SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand, LanguageManager->sTexts[LAN_WITH_IP], user->sIP,
                                LanguageManager->sTexts[LAN_DROP_ADDED_TEMPBAN_BY], curUser->sNick, LanguageManager->sTexts[LAN_BECAUSE_LWR],
                                reason == NULL ? LanguageManager->sTexts[LAN_NO_REASON_SPECIFIED] : reason);
                            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand207-2") == false) {
                       	        return true;
                            }
                        }

                        g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
         			}
                }

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    if(reason != NULL && strlen(reason) > 512) {
                       	int iret = sprintf(msg+imsgLen, "<%s> %s %s %s %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand,
                            LanguageManager->sTexts[LAN_WITH_IP], user->sIP, LanguageManager->sTexts[LAN_DROP_ADDED_TEMPBAN_BCS]);
                        imsgLen += iret;
                        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand210-1") == false) {
                            return true;
                        }

                        memcpy(msg+imsgLen, reason, 512);
                        imsgLen += (int)strlen(reason) + 2;
                        msg[imsgLen-2] = '.';
                        msg[imsgLen-1] = '|';
                        msg[imsgLen] = '\0';
                    } else {
                       	int iret = sprintf(msg+imsgLen, "<%s> %s %s %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand,
                            LanguageManager->sTexts[LAN_WITH_IP], user->sIP, LanguageManager->sTexts[LAN_DROP_ADDED_TEMPBAN_BCS],
                            reason == NULL ? LanguageManager->sTexts[LAN_NO_REASON_SPECIFIED] : reason);
                        imsgLen += iret;
                        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand210-2") == false) {
                            return true;
                        }
                    }

                    UserSendCharDelayed(curUser, msg, imsgLen);
                }
                UserClose(user);
                return true;
            }

            // !DelRegUser <nick>   
			if(strncasecmp(sCommand+1, "elreguser ", 10) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::DELREGUSER) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen > 255) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_CMD_TOO_LONG]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand212") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                } else if(dlen < 13) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cdelreguser <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                        LanguageManager->sTexts[LAN_NICK_LWR], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand214") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                sCommand += 11;
                size_t szNickLen = dlen-11;

                // find him
				RegUser *reg = hashRegManager->Find(sCommand, szNickLen);
                if(reg == NULL) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_ERROR], sCommand, LanguageManager->sTexts[LAN_IS_NOT_IN_REGS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand216") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                // check hierarchy
                // deny if curUser is not Master and tries delete equal or higher profile
                if(curUser->iProfile > 0 && reg->iProfile <= curUser->iProfile) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_YOURE_NOT_ALWD_TO_DLT_USER_THIS_PRFL]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand218") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

				UncountDeflood(curUser, fromPM);

                hashRegManager->Delete(reg);

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                        int imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                            SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick, LanguageManager->sTexts[LAN_REMOVED_LWR], 
                            sCommand, LanguageManager->sTexts[LAN_FROM_REGS]);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand219") == true) {
							g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                        }
                    } else {
                        int imsgLen = sprintf(msg, "<%s> *** %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick, 
                            LanguageManager->sTexts[LAN_REMOVED_LWR], sCommand, LanguageManager->sTexts[LAN_FROM_REGS]);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand220") == true) {
                            g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                        }
                    }
                }

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand, 
                        LanguageManager->sTexts[LAN_REMOVED_FROM_REGS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand222") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                }

                return true;
            }

            //Hub-Commands: !debug <port>|<list>|<off>
			if(strncasecmp(sCommand+1, "ebug ", 5) == 0) {
           	    if(((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen < 7) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                   	int iret = sprintf(msg+imsgLen, "<%s> *** %s %cdebug <%s>, %cdebug <off>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                        LanguageManager->sTexts[LAN_PORT_LWR], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                        LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand225") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                }

                sCommand += 6;

				if(strcasecmp(sCommand, "off") == 0) {
                    if(UdpDebug->CheckUdpSub(curUser) == true) {
                        if(UdpDebug->Remove(curUser) == true) {
                            UncountDeflood(curUser, fromPM);

                            int imsgLen = CheckFromPm(curUser, fromPM);

                            int iret = sprintf(msg+imsgLen, "<%s> *** %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                                LanguageManager->sTexts[LAN_UNSUB_FROM_UDP_DBG]);
                            imsgLen += iret;
                            if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand227") == true) {
        				        UserSendCharDelayed(curUser, msg, imsgLen);
                            }

                            imsgLen = sprintf(msg, "[SYS] Debug subscription cancelled: %s (%s)", curUser->sNick, curUser->sIP);
                            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand228") == true) {
                                UdpDebug->Broadcast(msg, imsgLen);
                            }
                            return true;
                        } else {
                            int imsgLen = CheckFromPm(curUser, fromPM);

                            int iret = sprintf(msg+imsgLen, "<%s> *** %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                                LanguageManager->sTexts[LAN_UNABLE_FIND_UDP_DBG_INTER]);
                            imsgLen += iret;
                            if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand230") == true) {
        				        UserSendCharDelayed(curUser, msg, imsgLen);
                            }
                            return true;
                        }
                    } else {
                        int imsgLen = CheckFromPm(curUser, fromPM);

                        int iret = sprintf(msg+imsgLen, "<%s> *** %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                            LanguageManager->sTexts[LAN_NOT_UDP_DEBUG_SUBSCRIB]);
                        imsgLen += iret;
                        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand232") == true) {
        				    UserSendCharDelayed(curUser, msg, imsgLen);
                        }
                        return true;
                    }
                } else {
                    if(UdpDebug->CheckUdpSub(curUser) == true) {
                        int imsgLen = CheckFromPm(curUser, fromPM);

                        int iret = sprintf(msg+imsgLen, "<%s> *** %s %cdebug <off> %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                            LanguageManager->sTexts[LAN_ALRD_UDP_SUBSCRIP_TO_UNSUB], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                            LanguageManager->sTexts[LAN_IN_MAINCHAT]);
                        imsgLen += iret;
                        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand234") == true) {
        				    UserSendCharDelayed(curUser, msg, imsgLen);
                        }
                        return true;
                    }

                    int dbg_port = atoi(sCommand);
                    if(dbg_port <= 0) {
                        int imsgLen = CheckFromPm(curUser, fromPM);

                        int iret = sprintf(msg+imsgLen, "<%s> *** %s %cdebug <%s>, %cdebug <off>.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                            LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], 
                            LanguageManager->sTexts[LAN_PORT_LWR], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0]);
                        imsgLen += iret;
                        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand236") == true) {
        				    UserSendCharDelayed(curUser, msg, imsgLen);
                        }
                        return true;
                    }

                    if(UdpDebug->New(curUser, dbg_port) == true) {
						UncountDeflood(curUser, fromPM);

                        int imsgLen = CheckFromPm(curUser, fromPM);

                        int iret = sprintf(msg+imsgLen, "<%s> *** %s %d. %s %cdebug <off> %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                            LanguageManager->sTexts[LAN_SUBSCIB_UDP_DEBUG_ON_PORT], dbg_port, LanguageManager->sTexts[LAN_TO_UNSUB_TYPE], 
                            SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_IN_MAINCHAT]);
                        imsgLen += iret;
                        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand238") == true) {
        				    UserSendCharDelayed(curUser, msg, imsgLen);
                        }

        				imsgLen = sprintf(msg, "[SYS] New Debug subscriber: %s (%s)", curUser->sNick, curUser->sIP);
        				if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand239") == true) {
                            UdpDebug->Broadcast(msg, imsgLen);
                        }
                        return true;
                    } else {
                        int imsgLen = CheckFromPm(curUser, fromPM);

                        int iret = sprintf(msg+imsgLen, "<%s> *** %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                            LanguageManager->sTexts[LAN_UDP_DEBUG_SUBSCRIB_FAILED]);
                        imsgLen += iret;
                        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand241") == true) {
        				    UserSendCharDelayed(curUser, msg, imsgLen);
                        }
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
                    if(UserPutInSendBuf(curUser, sData, iLen)) {
                        UserTry2Send(curUser);
                    }
                }

                free(sTmp);

                return true;
            }
*/
            // !clrtempbans
			if(dlen == 11 && strncasecmp(sCommand+1, "lrtempbans", 10) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::CLRTEMPBAN) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

				UncountDeflood(curUser, fromPM);
				hashBanManager->ClearTemp();

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                        int imsgLen = sprintf(msg, "%s $<%s> *** %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                            curUser->sNick, LanguageManager->sTexts[LAN_HAS_CLEARED_TEMPBANS]);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand242") == true) {
							g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                        }
                    } else {
                        int imsgLen = sprintf(msg, "<%s> *** %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick, LanguageManager->sTexts[LAN_HAS_CLEARED_TEMPBANS]);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand243") == true) {
                            g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                        }
                    }
                }

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

           	        int iret = sprintf(msg+imsgLen, "<%s> %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_TEMP_BANS_CLEARED]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand245") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                }
                return true;
            }

            // !clrpermbans
			if(dlen == 11 && strncasecmp(sCommand+1, "lrpermbans", 10) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::CLRPERMBAN) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

				UncountDeflood(curUser, fromPM);
				hashBanManager->ClearPerm();

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                        int imsgLen = sprintf(msg, "%s $<%s> *** %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                            curUser->sNick, LanguageManager->sTexts[LAN_HAS_CLEARED_PERMBANS]);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand246") == true) {
							g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                        }
                    } else {
                        int imsgLen = sprintf(msg, "<%s> *** %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick, LanguageManager->sTexts[LAN_HAS_CLEARED_PERMBANS]);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand247") == true) {
                            g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                        }
                    }
                }

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

           	        int iret = sprintf(msg+imsgLen, "<%s> %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_PERM_BANS_CLEARED]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand249") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                }
                return true;
            }

            // !clrrangetempbans
			if(dlen == 16 && strncasecmp(sCommand+1, "lrrangetempbans", 15) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::CLR_RANGE_TBANS) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

				UncountDeflood(curUser, fromPM);
				hashBanManager->ClearTempRange();

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                        int imsgLen = sprintf(msg, "%s $<%s> *** %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                            curUser->sNick, LanguageManager->sTexts[LAN_HAS_CLEARED_TEMP_RANGEBANS]);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand250") == true) {
							g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                        }
                    } else {
                        int imsgLen = sprintf(msg, "<%s> *** %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick, LanguageManager->sTexts[LAN_HAS_CLEARED_TEMP_RANGEBANS]);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand251") == true) {
                            g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                        }
                    }
                }

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

           	        int iret = sprintf(msg+imsgLen, "<%s> %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_TEMP_RANGE_BANS_CLEARED]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand253") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                }
                return true;
            }

            // !clrrangepermbans
			if(dlen == 16 && strncasecmp(sCommand+1, "lrrangepermbans", 15) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::CLR_RANGE_BANS) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

				UncountDeflood(curUser, fromPM);
				hashBanManager->ClearPermRange();

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                        int imsgLen = sprintf(msg, "%s $<%s> *** %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                            curUser->sNick, LanguageManager->sTexts[LAN_HAS_CLEARED_PERM_RANGEBANS]);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand254") == true) {
							g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                        }
                    } else {
                        int imsgLen = sprintf(msg, "<%s> *** %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick,
                            LanguageManager->sTexts[LAN_HAS_CLEARED_PERM_RANGEBANS]);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand255") == true) {
                            g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                        }
                    }
                }

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

           	        int iret = sprintf(msg+imsgLen, "<%s> %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_PERM_RANGE_BANS_CLEARED]);
           	        imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand257") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                }
                return true;
            }

            // !checknickban nick
			if(strncasecmp(sCommand+1, "hecknickban ", 12) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::GETBANLIST) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

				UncountDeflood(curUser, fromPM);

                if(dlen < 14 || sCommand[13] == '\0') {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                   	int iret = sprintf(msg+imsgLen, "<%s> ***%s %cchecknickban <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
                        SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
           	        imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand259") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
            		return true;
                }

                sCommand += 13;

                BanItem * Ban = hashBanManager->FindNick(sCommand, dlen-13);
                if(Ban == NULL) {
                    int imsgLen = CheckFromPm(curUser, fromPM);   

                   	int iret = sprintf(msg+imsgLen, "<%s> *** %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_NO_BAN_FOUND]);
           	        imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand261") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                int imsgLen = CheckFromPm(curUser, fromPM);                        

                int iret = sprintf(msg+imsgLen, "<%s> %s: %s", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_BANNED_NICK], Ban->sNick);
           	    imsgLen += iret;
                if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand263") == false) {
                    return true;
                }

                if(Ban->sIp[0] != '\0') {
                    if(((Ban->ui8Bits & hashBanMan::IP) == hashBanMan::IP) == true) {
                        int iret = sprintf(msg+imsgLen, " %s", LanguageManager->sTexts[LAN_BANNED]);
               	        imsgLen += iret;
                        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand264") == false) {
                            return true;
                        }
                    }
                    int iret = sprintf(msg+imsgLen, " %s: %s", LanguageManager->sTexts[LAN_IP], Ban->sIp);
           	        imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand265") == false) {
                        return true;
                    }
                    if(((Ban->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == true) {
                        int iret = sprintf(msg+imsgLen, " (%s)", LanguageManager->sTexts[LAN_FULL]);
           	            imsgLen += iret;
                        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand266") == false) {
                            return true;
                        }
                    }
                }

                if(Ban->sReason != NULL) {
                    int iret = sprintf(msg+imsgLen, " %s: %s", LanguageManager->sTexts[LAN_REASON], Ban->sReason);
           	        imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand267") == false) {
                        return true;
                    }
                }

                if(Ban->sBy != NULL) {
                    int iret = sprintf(msg+imsgLen, " %s: %s|", LanguageManager->sTexts[LAN_BANNED_BY], Ban->sBy);
           	        imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand268") == false) {
                        return true;
                    }
                }

                if(((Ban->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
                    char msg1[256];
                    struct tm *tm = localtime(&Ban->tempbanexpire);
                    strftime(msg1, 256, "%c", tm);
                    int iret = sprintf(msg+imsgLen, " %s: %s.|", LanguageManager->sTexts[LAN_EXPIRE], msg1);
           	        imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand269") == false) {
                        return true;
                    }
                } else {
                    msg[imsgLen] = '.';
                    msg[imsgLen+1] = '|';
                    msg[imsgLen+2] = '\0';
                    imsgLen += 2;
                }
                UserSendCharDelayed(curUser, msg, imsgLen);
                return true;
            }

            // !checkipban ip
			if(strncasecmp(sCommand+1, "heckipban ", 10) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::GETBANLIST) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

				UncountDeflood(curUser, fromPM);

                if(dlen < 18 || sCommand[11] == '\0') {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                   	int iret = sprintf(msg+imsgLen, "<%s> *** %s %ccheckipban <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
                        SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_IP], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand271") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
            		return true;
                }

                sCommand += 11;

				uint8_t ui128Hash[16];
				memset(ui128Hash, 0, 16);

                // check ip
                if(HashIP(sCommand, ui128Hash) == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s %ccheckipban <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
                        SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_IP], LanguageManager->sTexts[LAN_NO_VALID_IP_SPECIFIED]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand273") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }
                
                time_t acc_time;
                time(&acc_time);

                BanItem *nxtBan = hashBanManager->FindIP(ui128Hash, acc_time);

                if(nxtBan != NULL) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand275") == false) {
                        return true;
                    }

					string Bans(msg, imsgLen);
                    uint32_t iBanNum = 0;

                    while(nxtBan != NULL) {
                        BanItem *curBan = nxtBan;
                        nxtBan = curBan->hashiptablenext;

                        if(((curBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
                            // PPK ... check if temban expired
                            if(acc_time >= curBan->tempbanexpire) {
								hashBanManager->Rem(curBan);
                                delete curBan;

								continue;
                            }
                        }

                        iBanNum++;
                        imsgLen = sprintf(msg, "\n[%u] %s: %s", iBanNum, LanguageManager->sTexts[LAN_BANNED_IP], curBan->sIp);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand276") == false) {
                            return true;
                        }
                        if(((curBan->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == true) {
                            int iret = sprintf(msg+imsgLen, " (%s)", LanguageManager->sTexts[LAN_FULL]);
                            imsgLen += iret;
                            if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand277") == false) {
                                return true;
                            }
                        }

						Bans += string(msg, imsgLen);

                        if(curBan->sNick != NULL) {
                            imsgLen = 0;
                            if(((curBan->ui8Bits & hashBanMan::NICK) == hashBanMan::NICK) == true) {
                                imsgLen = sprintf(msg, " %s", LanguageManager->sTexts[LAN_BANNED]);
                                if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand278") == false) {
                                    return true;
                                }
                            }
                            int iret = sprintf(msg+imsgLen, " %s: %s", LanguageManager->sTexts[LAN_NICK], curBan->sNick);
                            imsgLen += iret;
                            if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand279") == false) {
                                return true;
                            }
							Bans += msg;
                        }

                        if(curBan->sReason != NULL) {
                            imsgLen = sprintf(msg, " %s: %s", LanguageManager->sTexts[LAN_REASON], curBan->sReason);
                            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand280") == false) {
                                return true;
                            }
							Bans += msg;
                        }

                        if(curBan->sBy != NULL) {
                            imsgLen = sprintf(msg, " %s: %s", LanguageManager->sTexts[LAN_BANNED_BY], curBan->sBy);
                            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand281") == false) {
                                return true;
                            }
							Bans += msg;
                        }

                        if(((curBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
                            struct tm *tm = localtime(&curBan->tempbanexpire);
                            strftime(msg, 256, "%c", tm);
							Bans += " " + string(LanguageManager->sTexts[LAN_EXPIRE], (size_t)LanguageManager->ui16TextsLens[LAN_EXPIRE]) + ": " + string(msg);
                        }
                    }

                    if(ProfileMan->IsAllowed(curUser, ProfileManager::GET_RANGE_BANS) == false) {
                        Bans += "|";

						UserSendTextDelayed(curUser, Bans);
                        return true;
                    }

					RangeBanItem *nxtRangeBan = hashBanManager->FindRange(ui128Hash, acc_time);

                    while(nxtRangeBan != NULL) {
                        RangeBanItem *curRangeBan = nxtRangeBan;
                        nxtRangeBan = curRangeBan->next;

						if(((curRangeBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
							// PPK ... check if temban expired
							if(acc_time >= curRangeBan->tempbanexpire) {
								hashBanManager->RemRange(curRangeBan);
								delete curRangeBan;

								continue;
							}
						}

                        if(memcmp(curRangeBan->ui128FromIpHash, ui128Hash, 16) <= 0 && memcmp(curRangeBan->ui128ToIpHash, ui128Hash, 16) >= 0) {
                            iBanNum++;
                            imsgLen = sprintf(msg, "\n[%u] %s: %s-%s", iBanNum, LanguageManager->sTexts[LAN_RANGE], curRangeBan->sIpFrom, curRangeBan->sIpTo);
                            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand282") == false) {
                                return true;
                            }
                            if(((curRangeBan->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == true) {
                                int iret= sprintf(msg+imsgLen, " (%s)", LanguageManager->sTexts[LAN_FULL]);
                                imsgLen += iret;
                                if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand283") == false) {
                                    return true;
                                }
                            }

							Bans += string(msg, imsgLen);

                            if(curRangeBan->sReason != NULL) {
                                imsgLen = sprintf(msg, " %s: %s", LanguageManager->sTexts[LAN_REASON], curRangeBan->sReason);
                                if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand284") == false) {
                                    return true;
                                }
								Bans += msg;
                            }

                            if(curRangeBan->sBy != NULL) {
                                imsgLen = sprintf(msg, " %s: %s", LanguageManager->sTexts[LAN_BANNED_BY], curRangeBan->sBy);
                                if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand285") == false) {
                                    return true;
                                }
								Bans += msg;
                            }

                            if(((curRangeBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
                                struct tm *tm = localtime(&curRangeBan->tempbanexpire);
                                strftime(msg, 256, "%c", tm);
								Bans += " " + string(LanguageManager->sTexts[LAN_EXPIRE], (size_t)LanguageManager->ui16TextsLens[LAN_EXPIRE]) + ": " + string(msg);
                            }
                        }
                    }

                    Bans += "|";

					UserSendTextDelayed(curUser, Bans);
                    return true;
                } else if(ProfileMan->IsAllowed(curUser, ProfileManager::GET_RANGE_BANS) == true) {
					RangeBanItem *nxtBan = hashBanManager->FindRange(ui128Hash, acc_time);

                    if(nxtBan != NULL) {
                        int imsgLen = CheckFromPm(curUser, fromPM);

                        int iret = sprintf(msg+imsgLen, "<%s> ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
                        imsgLen += iret;
                        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand287") == false) {
                            return true;
                        }

						string Bans(msg, imsgLen);
                        uint32_t iBanNum = 0;

                        while(nxtBan != NULL) {
                            RangeBanItem *curBan = nxtBan;
                            nxtBan = curBan->next;

							if(((curBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
								// PPK ... check if temban expired
								if(acc_time >= curBan->tempbanexpire) {
									hashBanManager->RemRange(curBan);
									delete curBan;

									continue;
								}
							}

                            if(memcmp(curBan->ui128FromIpHash, ui128Hash, 16) <= 0 && memcmp(curBan->ui128ToIpHash, ui128Hash, 16) >= 0) {
                                iBanNum++;
                                imsgLen = sprintf(msg, "\n[%u] %s: %s-%s", iBanNum, LanguageManager->sTexts[LAN_RANGE], curBan->sIpFrom, curBan->sIpTo);
                                if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand288") == false) {
                                    return true;
                                }
                                if(((curBan->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == true) {
                                    int iret = sprintf(msg+imsgLen, " (%s)", LanguageManager->sTexts[LAN_FULL]);
                                    imsgLen += iret;
                                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand289") == false) {
                                        return true;
                                    }
                                }

								Bans += string(msg, imsgLen);

                                if(curBan->sReason != NULL) {
                                    imsgLen = sprintf(msg, " %s: %s", LanguageManager->sTexts[LAN_REASON], curBan->sReason);
                                    if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand290") == false) {
                                        return true;
                                    }
									Bans += msg;
                                }

                                if(curBan->sBy != NULL) {
                                    imsgLen = sprintf(msg, " %s: %s", LanguageManager->sTexts[LAN_BANNED_BY], curBan->sBy);
                                    if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand291") == false) {
                                        return true;
                                    }
									Bans += msg;
                                }

                                if(((curBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
                                    struct tm *tm = localtime(&curBan->tempbanexpire);
                                    strftime(msg, 256, "%c", tm);
									Bans += " " + string(LanguageManager->sTexts[LAN_EXPIRE], (size_t)LanguageManager->ui16TextsLens[LAN_EXPIRE]) + ": " + string(msg);
                                }
                            }
                        }

                        Bans += "|";

						UserSendTextDelayed(curUser, Bans);
                        return true;
                    }
                }
                
                int imsgLen = CheckFromPm(curUser, fromPM);

                int iret = sprintf(msg+imsgLen, "<%s> *** %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_NO_BAN_FOUND]);
                imsgLen += iret;
                if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand293") == false) {
                    return true;
                }
                UserSendCharDelayed(curUser, msg, imsgLen);
                return true;
            }

            // !checkrangeban ipfrom ipto
			if(strncasecmp(sCommand+1, "heckrangeban ", 13) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::GET_RANGE_BANS) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

				UncountDeflood(curUser, fromPM);

                if(dlen < 26) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                   	int iret = sprintf(msg+imsgLen, "<%s> *** %s %ccheckrangeban <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_FROMIP],
                        LanguageManager->sTexts[LAN_TOIP], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand295") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
            		return true;
                }

                sCommand += 14;
                char *ipto = strchr(sCommand, ' ');
                
                if(ipto == NULL) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                   	int iret = sprintf(msg+imsgLen, "<%s> *** %s %ccheckrangeban <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_FROMIP],
                        LanguageManager->sTexts[LAN_TOIP], LanguageManager->sTexts[LAN_BAD_PARAMS_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand297") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
            		return true;
                }

                ipto[0] = '\0';
                ipto++;

                // check ipfrom
                if(sCommand[0] == '\0' || ipto[0] == '\0') {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                   	int iret = sprintf(msg+imsgLen, "<%s> *** %s %ccheckrangeban <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_FROMIP],
                        LanguageManager->sTexts[LAN_TOIP], LanguageManager->sTexts[LAN_BAD_PARAMS_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand299") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
            		return true;
                }

				uint8_t ui128FromHash[16], ui128ToHash[16];
				memset(ui128FromHash, 0, 16);
				memset(ui128ToHash, 0, 16);

                if(HashIP(sCommand, ui128FromHash) == false || HashIP(ipto, ui128ToHash) == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s %ccheckrangeban <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_FROMIP],
                        LanguageManager->sTexts[LAN_TOIP], LanguageManager->sTexts[LAN_NO_VALID_IP_RANGE_SPECIFIED]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand301") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                time_t acc_time;
                time(&acc_time);

				RangeBanItem *RangeBan = hashBanManager->FindRange(ui128FromHash, ui128ToHash, acc_time);
                if(RangeBan == NULL) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_NO_RANGE_BAN_FOUND]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand303") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }
                
                int imsgLen = CheckFromPm(curUser, fromPM);

                int iret = sprintf(msg+imsgLen, "<%s> %s: %s-%s", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_RANGE], RangeBan->sIpFrom, RangeBan->sIpTo);
                imsgLen += iret;
                if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand305") == false) {
                    return true;
                }
                    
                if(((RangeBan->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == true) {
                    int iret = sprintf(msg+imsgLen, " (%s)", LanguageManager->sTexts[LAN_FULL]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand306") == false) {
                        return true;
                    }
                }

                if(RangeBan->sReason != NULL) {
                    int iret = sprintf(msg+imsgLen, " %s: %s", LanguageManager->sTexts[LAN_REASON], RangeBan->sReason);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand307") == false) {
                        return true;
                    }
                }

                if(RangeBan->sBy != NULL) {
                    int iret = sprintf(msg+imsgLen, " %s: %s", LanguageManager->sTexts[LAN_BANNED_BY], RangeBan->sBy);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand308") == false) {
                        return true;
                    }
                }

                if(((RangeBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
                    char msg1[256];
                    struct tm *tm = localtime(&RangeBan->tempbanexpire);
                    strftime(msg1, 256, "%c", tm);
                    int iret = sprintf(msg+imsgLen, " %s: %s.|", LanguageManager->sTexts[LAN_EXPIRE], msg1);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand309") == false) {
                        return true;
                    }
                } else {
                    msg[imsgLen] = '.';
                    msg[imsgLen+1] = '|';
                    msg[imsgLen+2] = '\0';
                    imsgLen += 2;
                }

                UserSendCharDelayed(curUser, msg, imsgLen);
                return true;
            }

            return false;

        case 'a':
            // !addreguser <nick> <pass> <profile_idx>
			if(strncasecmp(sCommand+1, "ddreguser ", 10) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::ADDREGUSER) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen > 255) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_CMD_TOO_LONG]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand311") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
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
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s %caddreguser <%s> <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR],
                        LanguageManager->sTexts[LAN_PASSWORD_LWR], LanguageManager->sTexts[LAN_PROFILENAME_LWR], LanguageManager->sTexts[LAN_BAD_PARAMS_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand313") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                if(iCmdPartsLen[0] > 65) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand315") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

				if(strpbrk(sCmdParts[0], " $|") != NULL) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_NO_BAD_CHARS_IN_NICK]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand416") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                if(iCmdPartsLen[1] > 65) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_MAX_ALWD_PASS_LEN_64_CHARS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand317") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

				if(strchr(sCmdParts[1], '|') != NULL) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_NO_PIPE_IN_PASS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand439") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                int pidx = ProfileMan->GetProfileIndex(sCmdParts[2]);
                if(pidx == -1) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_ERR_NO_PROFILE_GIVEN_NAME_EXIST]);
               	    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand319") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                // check hierarchy
                // deny if curUser is not Master and tries add equal or higher profile
                if(curUser->iProfile > 0 && pidx <= curUser->iProfile) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_NOT_ALLOWED_TO_ADD_USER_THIS_PROFILE]);
               	    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand321") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                // try to add the user
                if(hashRegManager->AddNew(sCmdParts[0], sCmdParts[1], (uint16_t)pidx) == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_USER], sCmdParts[0],
                        LanguageManager->sTexts[LAN_IS_ALREDY_REGISTERED]);
               	    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand323") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);      
                    }
                    return true;
                }

				UncountDeflood(curUser, fromPM);

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                        int imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],  SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                            curUser->sNick, LanguageManager->sTexts[LAN_SUCCESSFULLY_ADDED], sCmdParts[0], LanguageManager->sTexts[LAN_TO_REGISTERED_USERS]);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand324") == true) {
							g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                        }
                    } else {
                        int imsgLen = sprintf(msg, "<%s> *** %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick,
                            LanguageManager->sTexts[LAN_SUCCESSFULLY_ADDED], sCmdParts[0], LanguageManager->sTexts[LAN_TO_REGISTERED_USERS]);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand325") == true) {
                            g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                        }
                    }
                }

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCmdParts[0],
                        LanguageManager->sTexts[LAN_SUCCESSFULLY_ADDED_TO_REGISTERED_USERS]);
               	    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand327") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                }
            
                User *AddedUser = hashManager->FindUser(sCmdParts[0], iCmdPartsLen[0]);
                if(AddedUser != NULL) {
                    bool bAllowedOpChat = ProfileMan->IsAllowed(AddedUser, ProfileManager::ALLOWEDOPCHAT);
                    AddedUser->iProfile = pidx;
                    if(((AddedUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                        if(ProfileMan->IsAllowed(AddedUser, ProfileManager::HASKEYICON) == true) {
                            AddedUser->ui32BoolBits |= User::BIT_OPERATOR;
                        } else {
                            AddedUser->ui32BoolBits &= ~User::BIT_OPERATOR;
                        }

                        if(((AddedUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                            colUsers->Add2OpList(AddedUser);
                            g_GlobalDataQueue->OpListStore(AddedUser->sNick);
                            if(bAllowedOpChat != ProfileMan->IsAllowed(AddedUser, ProfileManager::ALLOWEDOPCHAT)) {
                                if(SettingManager->bBools[SETBOOL_REG_OP_CHAT] == true &&
                                    (SettingManager->bBools[SETBOOL_REG_BOT] == false || SettingManager->bBotsSameNick == false)) {
                                    if(((AddedUser->ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == false) {
                                        UserSendCharDelayed(AddedUser, SettingManager->sPreTexts[SetMan::SETPRETXT_OP_CHAT_HELLO],
                                            SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_OP_CHAT_HELLO]);
                                    }
                                    UserSendCharDelayed(AddedUser, SettingManager->sPreTexts[SetMan::SETPRETXT_OP_CHAT_MYINFO],
                                        SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_OP_CHAT_MYINFO]);
                                    int imsgLen = sprintf(msg, "$OpList %s$$|", SettingManager->sTexts[SETTXT_OP_CHAT_NICK]);
                                    if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand328") == true) {
                                        UserSendCharDelayed(AddedUser, msg, imsgLen);
                                    }
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
                int imsglen = CheckFromPm(curUser, fromPM);

                int iret  = sprintf(msg+imsglen, "<%s> ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
               	imsglen += iret;
                if(CheckSprintf1(iret, imsglen, 1024, "HubCommands::DoCommand330") == false) {
                    return true;
                }

				string help(msg, imsglen);
                bool bFull = false;
                bool bTemp = false;

                imsglen = sprintf(msg, "%s:\n", LanguageManager->sTexts[LAN_FOLOW_COMMANDS_AVAILABLE_TO_YOU]);
                if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand331") == false) {
                    return true;
                }
				help += msg;

                if(curUser->iProfile != -1) {
                    imsglen = sprintf(msg, "\n%s:\n", LanguageManager->sTexts[LAN_PROFILE_SPECIFIC_CMDS]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand332") == false) {
                        return true;
                    }
					help += msg;

                    imsglen = sprintf(msg, "\t%cpasswd <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        LanguageManager->sTexts[LAN_NEW_PASSWORD], LanguageManager->sTexts[LAN_CHANGE_YOUR_PASSWORD]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand333") == false) {
                        return true;
                    }
					help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::BAN)) {
                    bFull = true;

                    imsglen = sprintf(msg, "\t%cban <%s> <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR],
                        LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_PERM_BAN_USER_GIVEN_NICK_DISCONNECT]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand334") == false) {
                        return true;
                    }
					help += msg;

                    imsglen = sprintf(msg, "\t%cbanip <%s> <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_IP],
                        LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_PERM_BAN_IP_ADDRESS]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand335") == false) {
                        return true;
                    }
					help += msg;

                    imsglen = sprintf(msg, "\t%cfullban <%s> <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR],
                        LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_PERM_BAN_USER_GIVEN_NICK_DISCONNECT]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand336") == false) {
                        return true;
                    }
					help += msg;

                    imsglen = sprintf(msg, "\t%cfullbanip <%s> <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_IP],
                        LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_PERM_BAN_IP_ADDRESS]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand337") == false) {
                        return true;
                    }
					help += msg;

                    imsglen = sprintf(msg, "\t%cnickban <%s> <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR],
                        LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_BAN_USERS_NICK_IFCONN_THENDISCONN]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand338") == false) {
                        return true;
                    }
                    help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::TEMP_BAN)) {
                    bFull = true; bTemp = true;

					imsglen = sprintf(msg, "\t%ctempban <%s> <%s> <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR],
                        LanguageManager->sTexts[LAN_TIME_LWR], LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_TEMP_BAN_USER_GIVEN_NICK_DISCONNECT]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand339") == false) {
                        return true;
                    }
					help += msg;

                    imsglen = sprintf(msg, "\t%ctempbanip <%s> <%s> <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_IP],
                        LanguageManager->sTexts[LAN_TIME_LWR], LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_TEMP_BAN_IP_ADDRESS]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand340") == false) {
                        return true;
                    }
					help += msg;

                    imsglen = sprintf(msg, "\t%cfulltempban <%s> <%s> <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR],
                        LanguageManager->sTexts[LAN_TIME_LWR], LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_TEMP_BAN_USER_GIVEN_NICK_DISCONNECT]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand341") == false) {
                        return true;
                    }
					help += msg;

                    imsglen = sprintf(msg, "\t%cfulltempbanip <%s> <%s> <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_IP],
                        LanguageManager->sTexts[LAN_TIME_LWR], LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_TEMP_BAN_IP_ADDRESS]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand342") == false) {
                        return true;
                    }
					help += msg;

                    imsglen = sprintf(msg, "\t%cnicktempban <%s> <%s> <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR],
                        LanguageManager->sTexts[LAN_TIME_LWR], LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_TEMP_BAN_USERS_NICK_IFCONN_THENDISCONN]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand343") == false) {
                        return true;
                    }
					help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::UNBAN) || ProfileMan->IsAllowed(curUser, ProfileManager::TEMP_UNBAN)) {
                    imsglen = sprintf(msg, "\t%cunban <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_IP_OR_NICK],
                        LanguageManager->sTexts[LAN_UNBAN_IP_OR_NICK]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand344") == false) {
                        return true;
					}
					help += msg;
				}

                if(ProfileMan->IsAllowed(curUser, ProfileManager::UNBAN)) {
                    imsglen = sprintf(msg, "\t%cpermunban <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_IP_OR_NICK],
                        LanguageManager->sTexts[LAN_UNBAN_PERM_BANNED_IP_OR_NICK]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand345") == false) {
                        return true;
                    }
					help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::TEMP_UNBAN)) {
                    imsglen = sprintf(msg, "\t%ctempunban <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_IP_OR_NICK],
                        LanguageManager->sTexts[LAN_UNBAN_TEMP_BANNED_IP_OR_NICK]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand346") == false) {
                        return true;
                    }
					help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::GETBANLIST)) {
                    imsglen = sprintf(msg, "\t%cgetbans - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_DISPLAY_LIST_OF_BANS]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand347") == false) {
                        return true;
                    }
					help += msg;

                    imsglen = sprintf(msg, "\t%cgetpermbans - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_DISPLAY_LIST_OF_PERM_BANS]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand348") == false) {
                        return true;
                    }
					help += msg;

                    imsglen = sprintf(msg, "\t%cgettempbans - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_DISPLAY_LIST_OF_TEMP_BANS]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand349") == false) {
                        return true;
                    }
					help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::CLRPERMBAN)) {
                    imsglen = sprintf(msg, "\t%cclrpermbans - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_CLEAR_PERM_BANS_LWR]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand350") == false) {
                        return true;
                    }
					help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::CLRTEMPBAN)) {
                    imsglen = sprintf(msg, "\t%cclrtempbans - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_CLEAR_TEMP_BANS_LWR]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand351") == false) {
                        return true;
                    }
					help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::RANGE_BAN)) {
                    bFull = true;

                    imsglen = sprintf(msg, "\t%crangeban <%s> <%s> <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_FROMIP],
                        LanguageManager->sTexts[LAN_TOIP], LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_PERM_BAN_GIVEN_IP_RANGE]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand352") == false) {
                        return true;
                    }
					help += msg;

                    imsglen = sprintf(msg, "\t%cfullrangeban <%s> <%s> <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_FROMIP],
                        LanguageManager->sTexts[LAN_TOIP], LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_PERM_BAN_GIVEN_IP_RANGE]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand353") == false) {
                        return true;
                    }
					help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::RANGE_TBAN)) {
                    bFull = true; bTemp = true;

                    imsglen = sprintf(msg, "\t%crangetempban <%s> <%s> <%s> <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        LanguageManager->sTexts[LAN_FROMIP], LanguageManager->sTexts[LAN_TOIP], LanguageManager->sTexts[LAN_TIME_LWR], LanguageManager->sTexts[LAN_REASON_LWR],
                        LanguageManager->sTexts[LAN_TEMP_BAN_GIVEN_IP_RANGE]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand354") == false) {
                        return true;
                    }
					help += msg;

                    imsglen = sprintf(msg, "\t%cfullrangetempban <%s> <%s> <%s> <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        LanguageManager->sTexts[LAN_FROMIP], LanguageManager->sTexts[LAN_TOIP], LanguageManager->sTexts[LAN_TIME_LWR], LanguageManager->sTexts[LAN_REASON_LWR],
                        LanguageManager->sTexts[LAN_TEMP_BAN_GIVEN_IP_RANGE]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand355") == false) {
                        return true;
                    }
					help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::RANGE_UNBAN) || ProfileMan->IsAllowed(curUser, ProfileManager::RANGE_TUNBAN)) {
                    imsglen = sprintf(msg, "\t%crangeunban <%s> <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_FROMIP],
                        LanguageManager->sTexts[LAN_TOIP], LanguageManager->sTexts[LAN_UNBAN_BANNED_IP_RANGE]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand356") == false) {
                        return true;
                    }
					help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::RANGE_UNBAN)) {
                    imsglen = sprintf(msg, "\t%crangepermunban <%s> <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_FROMIP],
                        LanguageManager->sTexts[LAN_TOIP], LanguageManager->sTexts[LAN_UNBAN_PERM_BANNED_IP_RANGE]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand357") == false) {
                        return true;
                    }
					help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::RANGE_TUNBAN)) {
                    imsglen = sprintf(msg, "\t%crangetempunban <%s> <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_FROMIP],
                        LanguageManager->sTexts[LAN_TOIP], LanguageManager->sTexts[LAN_UNBAN_TEMP_BANNED_IP_RANGE]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand358") == false) {
                        return true;
                    }
					help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::GET_RANGE_BANS)) {
                    bFull = true;

                    imsglen = sprintf(msg, "\t%cgetrangebans - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        LanguageManager->sTexts[LAN_DISPLAY_LIST_OF_RANGE_BANS]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand359") == false) {
                        return true;
                    }
					help += msg;

                    imsglen = sprintf(msg, "\t%cgetrangepermbans - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        LanguageManager->sTexts[LAN_DISPLAY_LIST_OF_RANGE_PERM_BANS]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand360") == false) {
                        return true;
                    }
					help += msg;

                    imsglen = sprintf(msg, "\t%cgetrangetempbans - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        LanguageManager->sTexts[LAN_DISPLAY_LIST_OF_RANGE_TEMP_BANS]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand361") == false) {
                        return true;
                    }
                    help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::CLR_RANGE_BANS)) {
                    imsglen = sprintf(msg, "\t%cclrrangepermbans - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        LanguageManager->sTexts[LAN_CLEAR_PERM_RANGE_BANS_LWR]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand362") == false) {
                        return true;
                    }
					help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::CLR_RANGE_TBANS)) {
                    imsglen = sprintf(msg, "\t%cclrrangetempbans - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        LanguageManager->sTexts[LAN_CLEAR_TEMP_RANGE_BANS_LWR]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand363") == false) {
                        return true;
                    }
					help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::GETBANLIST)) {
                    imsglen = sprintf(msg, "\t%cchecknickban <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR],
                        LanguageManager->sTexts[LAN_DISPLAY_BAN_FOUND_FOR_GIVEN_NICK]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand364") == false) {
                        return true;
                    }
					help += msg;

                    imsglen = sprintf(msg, "\t%ccheckipban <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_IP],
                        LanguageManager->sTexts[LAN_DISPLAY_BANS_FOUND_FOR_GIVEN_IP]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand365") == false) {
                        return true;
                    }
					help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::GET_RANGE_BANS)) {
                    imsglen = sprintf(msg, "\t%ccheckrangeban <%s> <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_FROMIP],
                        LanguageManager->sTexts[LAN_TOIP], LanguageManager->sTexts[LAN_DISPLAY_RANGE_BAN_FOR_GIVEN_RANGE]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand366") == false) {
                        return true;
                    }
					help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::DROP)) {
                    imsglen = sprintf(msg, "\t%cdrop <%s> <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR],
                        LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_DISCONNECT_WITH_TEMPBAN]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand367") == false) {
                        return true;
                    }
					help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::GETINFO)) {
                    imsglen = sprintf(msg, "\t%cgetinfo <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR],
                        LanguageManager->sTexts[LAN_DISPLAY_INFO_GIVEN_NICK]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand368") == false) {
                        return true;
                    }
					help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::TEMPOP)) {
                    imsglen = sprintf(msg, "\t%cop <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR],
                        LanguageManager->sTexts[LAN_GIVE_TEMP_OP]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand369") == false) {
                        return true;
                    }
					help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::GAG)) {
                    imsglen = sprintf(msg, "\t%cgag <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR],
                        LanguageManager->sTexts[LAN_DISALLOW_USER_TO_POST_IN_MAIN]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand370") == false) {
                        return true;
                    }
					help += msg;

                    imsglen = sprintf(msg, "\t%cungag <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR],
                        LanguageManager->sTexts[LAN_USER_CAN_POST_IN_MAIN_AGAIN]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand371") == false) {
                        return true;
                    }
					help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::RSTHUB)) {
                    imsglen = sprintf(msg, "\t%crestart - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_RESTART_HUB_LWR]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand372") == false) {
                        return true;
                    }
					help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::RSTSCRIPTS)) {
                    imsglen = sprintf(msg, "\t%cstartscript <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_FILENAME_LWR],
                        LanguageManager->sTexts[LAN_START_SCRIPT_GIVEN_FILENAME]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand373") == false) {
                        return true;
                    }
					help += msg;

                    imsglen = sprintf(msg, "\t%cstopscript <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_FILENAME_LWR],
                        LanguageManager->sTexts[LAN_STOP_SCRIPT_GIVEN_FILENAME]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand374") == false) {
                        return true;
                    }
					help += msg;

                    imsglen = sprintf(msg, "\t%crestartscript <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_FILENAME_LWR],
                        LanguageManager->sTexts[LAN_RESTART_SCRIPT_GIVEN_FILENAME]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand375") == false) {
                        return true;
                    }
					help += msg;

                    imsglen = sprintf(msg, "\t%crestartscripts - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        LanguageManager->sTexts[LAN_RESTART_SCRIPTING_PART_OF_THE_HUB]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand376") == false) {
                        return true;
                    }
					help += msg;

                    imsglen = sprintf(msg, "\t%cgetscripts - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_DISPLAY_LIST_OF_SCRIPTS]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand377") == false) {
                        return true;
                    }
					help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::REFRESHTXT)) {
                    imsglen = sprintf(msg, "\t%creloadtxt - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_RELOAD_ALL_TEXT_FILES]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand378") == false) {
                        return true;
                    }
					help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::ADDREGUSER)) {
                    imsglen = sprintf(msg, "\t%creguser <%s> <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR],
                        LanguageManager->sTexts[LAN_PROFILENAME_LWR], LanguageManager->sTexts[LAN_REG_USER_WITH_PROFILE]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand379-1") == false) {
                        return true;
                    }
					help += msg;

                    imsglen = sprintf(msg, "\t%caddreguser <%s> <%s> <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR],
                        LanguageManager->sTexts[LAN_PASSWORD_LWR], LanguageManager->sTexts[LAN_PROFILENAME_LWR], LanguageManager->sTexts[LAN_ADD_REG_USER_WITH_PROFILE]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand379-2") == false) {
                        return true;
                    }
					help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::DELREGUSER)) {
                    imsglen = sprintf(msg, "\t%cdelreguser <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR],
                        LanguageManager->sTexts[LAN_REMOVE_REG_USER]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand380") == false) {
                        return true;
                    }
					help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::TOPIC) == true) {
                    imsglen = sprintf(msg, "\t%ctopic <%s> - %s %ctopic <off> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        LanguageManager->sTexts[LAN_NEW_TOPIC], LanguageManager->sTexts[LAN_SET_NEW_TOPIC_OR], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],
                        LanguageManager->sTexts[LAN_CLEAR_TOPIC]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand381") == false) {
                        return true;
                    }
					help += msg;
                }

                if(ProfileMan->IsAllowed(curUser, ProfileManager::MASSMSG) == true) {
                    imsglen = sprintf(msg, "\t%cmassmsg <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_MESSAGE_LWR],
                        LanguageManager->sTexts[LAN_SEND_MSG_TO_ALL_USERS]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand382") == false) {
                        return true;
                    }
					help += msg;

                    imsglen = sprintf(msg, "\t%copmassmsg <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_MESSAGE_LWR],
                        LanguageManager->sTexts[LAN_SEND_MSG_TO_ALL_OPS]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand383") == false) {
                        return true;
                    }
					help += msg;
                }

                if(((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
//                    help += "\t"+String(SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0])+"stat - Show some hub statistics.\n\n";
//                    help += "\t"+String(SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0])+"ipinfo <IP> - Show all on/offline users with that IP\n";
//                    help += "\t"+String(SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0])+"iprangeinfo <IP> - Show all on/offline users within that IP range\n";
//                    help += "\t"+String(SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0])+"userinfo <username> - Show all visits of that user\n";
                }

                if(bFull == true) {
                    imsglen = sprintf(msg, "*** %s.\n", LanguageManager->sTexts[LAN_REASON_IS_ALWAYS_OPTIONAL]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand384") == false) {
                        return true;
                    }
					help += msg;

                    imsglen = sprintf(msg, "*** %s.\n", LanguageManager->sTexts[LAN_FULLBAN_HELP_TXT]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand385") == false) {
                        return true;
                    }
					help += msg;
                }

                if(bTemp == true) {
                    imsglen = sprintf(msg, "*** %s: m = %s, h = %s, d = %s, w = %s, M = %s, Y = %s.\n", LanguageManager->sTexts[LAN_TEMPBAN_TIME_VALUES],
                        LanguageManager->sTexts[LAN_MINUTES_LWR], LanguageManager->sTexts[LAN_HOURS_LWR], LanguageManager->sTexts[LAN_DAYS_LWR], LanguageManager->sTexts[LAN_WEEKS_LWR],
                        LanguageManager->sTexts[LAN_MONTHS_LWR], LanguageManager->sTexts[LAN_YEARS_LWR]);
                    if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand386") == false) {
                        return true;
                    }
                    help += msg;
                }

                imsglen = sprintf(msg, "\n%s:\n", LanguageManager->sTexts[LAN_GLOBAL_COMMANDS]);
                if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand387") == false) {
                    return true;
                }
				help += msg;

                imsglen = sprintf(msg, "\t%cme <%s> - %s.\n", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_MESSAGE_LWR],
                    LanguageManager->sTexts[LAN_SPEAK_IN_3RD_PERSON]);
                if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand388") == false) {
                    return true;
                }
				help += msg;

                imsglen = sprintf(msg, "\t%cmyip - %s.|", SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_SHOW_YOUR_IP]);
                if(CheckSprintf(imsglen, 1024, "HubCommands::DoCommand389") == false) {
                    return true;
                }
				help += msg;

//                help += "\t"+String(SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0])+"help - this help page.\n|";

				UserSendTextDelayed(curUser, help);
                return true;
            }

            return false;

		case 's':
			//Hub-Commands: !stat !stats
			if((dlen == 4 && strncasecmp(sCommand+1, "tat", 3) == 0) || (dlen == 5 && strncasecmp(sCommand+1, "tats", 4) == 0)) {
				int imsglen = CheckFromPm(curUser, fromPM);

				int iret = sprintf(msg+imsglen, "<%s>", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
				imsglen += iret;
				if(CheckSprintf1(iret, imsglen, 1024, "HubCommands::DoCommand391") == false) {
					return true;
				}

				string Statinfo(msg, imsglen);

				Statinfo+="\n------------------------------------------------------------\n";
				Statinfo+="Current stats:\n";
				Statinfo+="------------------------------------------------------------\n";
				Statinfo+="Uptime: "+string(iDays)+" days, "+string(iHours) + " hours, " + string(iMins) + " minutes\n";

                Statinfo+="Version: PtokaX DC Hub " PtokaXVersionString
#ifdef _WIN32
    #ifdef _M_X64
                " x64"
    #endif
#endif
#ifdef _PtokaX_TESTING_
                " [build " BUILD_NUMBER "]"
#endif
                " built on " __DATE__ " " __TIME__ "\n";
#ifdef _WIN32
				Statinfo+="OS: "+sOs+"\r\n";
#else

				struct utsname osname;
				if(uname(&osname) >= 0) {
					Statinfo+="OS: "+string(osname.sysname)+" "+string(osname.release)+" ("+string(osname.machine)+")\n";
				}
#endif
				Statinfo+="Users (Max/Actual Peak (Max Peak)/Logged): "+string(SettingManager->iShorts[SETSHORT_MAX_USERS])+" / "+string(ui32Peak)+" ("+
					string(SettingManager->iShorts[SETSHORT_MAX_USERS_PEAK])+") / "+string(ui32Logged)+ "\n";
                Statinfo+="Joins / Parts: "+string(ui32Joins)+" / "+string(ui32Parts)+"\n";
				Statinfo+="Users shared size: "+string(ui64TotalShare)+" Bytes / "+string(formatBytes(ui64TotalShare))+"\n";
				Statinfo+="Chat messages: "+string(DcCommands->iStatChat)+" x\n";
				Statinfo+="Unknown commands: "+string(DcCommands->iStatCmdUnknown)+" x\n";
				Statinfo+="PM commands: "+string(DcCommands->iStatCmdTo)+" x\n";
				Statinfo+="Key commands: "+string(DcCommands->iStatCmdKey)+" x\n";
				Statinfo+="Supports commands: "+string(DcCommands->iStatCmdSupports)+" x\n";
				Statinfo+="MyINFO commands: "+string(DcCommands->iStatCmdMyInfo)+" x\n";
				Statinfo+="ValidateNick commands: "+string(DcCommands->iStatCmdValidate)+" x\n";
				Statinfo+="GetINFO commands: "+string(DcCommands->iStatCmdGetInfo)+" x\n";
				Statinfo+="Password commands: "+string(DcCommands->iStatCmdMyPass)+" x\n";
				Statinfo+="Version commands: "+string(DcCommands->iStatCmdVersion)+" x\n";
				Statinfo+="GetNickList commands: "+string(DcCommands->iStatCmdGetNickList)+" x\n";
				Statinfo+="Search commands: "+string(DcCommands->iStatCmdSearch)+" x ("+string(DcCommands->iStatCmdMultiSearch)+" x)\n";
				Statinfo+="SR commands: "+string(DcCommands->iStatCmdSR)+" x\n";
				Statinfo+="CTM commands: "+string(DcCommands->iStatCmdConnectToMe)+" x ("+string(DcCommands->iStatCmdMultiConnectToMe)+" x)\n";
				Statinfo+="RevCTM commands: "+string(DcCommands->iStatCmdRevCTM)+" x\n";
				Statinfo+="BotINFO commands: "+string(DcCommands->iStatBotINFO)+" x\n";
				Statinfo+="Close commands: "+string(DcCommands->iStatCmdClose)+" x\n";
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
				int iLen = sprintf(cpuusage, "%.2f%%\r\n", cpuUsage/0.6);
				if(CheckSprintf(iLen, 32, "HubCommands::DoCommand3921") == false) {
					return true;
				}
				Statinfo+="CPU usage (60 sec avg): "+string(cpuusage, iLen);

				char cputime[64];
				iLen = sprintf(cputime, "%01I64d:%02d:%02d", icpuSec / (60*60), (int32_t)((icpuSec / 60) % 60), (int32_t)(icpuSec % 60));
				if(CheckSprintf(iLen, 64, "HubCommands::DoCommand392") == false) {
					return true;
				}
				Statinfo+="CPU time: "+string(cputime, iLen)+"\r\n";

				PROCESS_MEMORY_COUNTERS pmc;
				memset(&pmc, 0, sizeof(PROCESS_MEMORY_COUNTERS));
				pmc.cb = sizeof(pmc);

				typedef BOOL (WINAPI *PGPMI)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);
				PGPMI pGPMI = (PGPMI)GetProcAddress(LoadLibrary("psapi.dll"), "GetProcessMemoryInfo");

                if(pGPMI != NULL) {
					pGPMI(GetCurrentProcess(), &pmc, sizeof(pmc));

					Statinfo+="Mem usage (Peak): "+string(formatBytes(pmc.WorkingSetSize))+ " ("+string(formatBytes(pmc.PeakWorkingSetSize))+")\r\n";
					Statinfo+="VM size (Peak): "+string(formatBytes(pmc.PagefileUsage))+ " ("+string(formatBytes(pmc.PeakPagefileUsage))+")\r\n";
                }
#else // _WIN32
				char cpuusage[32];
				int iLen = sprintf(cpuusage, "%.2f%%\n", (cpuUsage/0.6)/(double)ui32CpuCount);
				if(CheckSprintf(iLen, 32, "HubCommands::DoCommand3921") == false) {
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
				if(CheckSprintf(iLen, 64, "HubCommands::DoCommand392") == false) {
					return true;
				}
				Statinfo+="CPU time: "+string(cputime, iLen)+"\n";

				FILE *fp = fopen("/proc/self/status", "r");
				if(fp != NULL) {
					string memrss, memhwm, memvms, memvmp, memstk, memlib;
					char buf[1024];
					while(fgets(buf, 1024, fp) != NULL) {
						if(strncmp(buf, "VmRSS:", 6) == 0) {
							char * tmp = buf+6;
							while(isspace(*tmp) && *tmp) {
								tmp++;
							}
							memrss = string(tmp, strlen(tmp)-1);
						} else if(strncmp(buf, "VmHWM:", 6) == 0) {
							char * tmp = buf+6;
							while(isspace(*tmp) && *tmp) {
								tmp++;
							}
							memhwm = string(tmp, strlen(tmp)-1);
						} else if(strncmp(buf, "VmSize:", 7) == 0) {
							char * tmp = buf+7;
							while(isspace(*tmp) && *tmp) {
								tmp++;
							}
							memvms = string(tmp, strlen(tmp)-1);
						} else if(strncmp(buf, "VmPeak:", 7) == 0) {
							char * tmp = buf+7;
							while(isspace(*tmp) && *tmp) {
								tmp++;
							}
							memvmp = string(tmp, strlen(tmp)-1);
						} else if(strncmp(buf, "VmStk:", 6) == 0) {
							char * tmp = buf+6;
							while(isspace(*tmp) && *tmp) {
								tmp++;
							}
							memstk = string(tmp, strlen(tmp)-1);
						} else if(strncmp(buf, "VmLib:", 6) == 0) {
							char * tmp = buf+6;
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
				Statinfo+="SendRests (Peak): "+string(srvLoop->iLastSendRest)+" ("+string(srvLoop->iSendRestsPeak)+")\n";
				Statinfo+="RecvRests (Peak): "+string(srvLoop->iLastRecvRest)+" ("+string(srvLoop->iRecvRestsPeak)+")\n";
				Statinfo+="Compression saved: "+string(formatBytes(ui64BytesSentSaved))+" ("+string(DcCommands->iStatZPipe)+")\n";
				Statinfo+="Data sent: "+string(formatBytes(ui64BytesSent))+ "\n";
				Statinfo+="Data received: "+string(formatBytes(ui64BytesRead))+ "\n";
				Statinfo+="Tx (60 sec avg): "+string(formatBytesPerSecond(iActualBytesSent))+" ("+string(formatBytesPerSecond(iAverageBytesSent/60))+")\n";
				Statinfo+="Rx (60 sec avg): "+string(formatBytesPerSecond(iActualBytesRead))+" ("+string(formatBytesPerSecond(iAverageBytesRead/60))+")|";
				UserSendTextDelayed(curUser, Statinfo);
				return true;
			}

			// Hub commands: !stopscript scriptname
			if(strncasecmp(sCommand+1, "topscript ", 10) == 0) {
				if(ProfileMan->IsAllowed(curUser, ProfileManager::RSTSCRIPTS) == false) {
					SendNoPermission(curUser, fromPM);
					return true;
				}

				if(SettingManager->bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
					int imsgLen = CheckFromPm(curUser, fromPM);

					int iret = sprintf(msg+imsgLen, "<%s> *** %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_ERR_SCRIPTS_DISABLED]);
					imsgLen += iret;
					if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand400") == true) {
						UserSendCharDelayed(curUser, msg, imsgLen);
					}
					return true;
				}

				if(dlen < 12) {
					int imsgLen = CheckFromPm(curUser, fromPM);

					int iret = sprintf(msg+imsgLen, "<%s> *** %s %cstopscript <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
                        SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_SCRIPTNAME_LWR], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
					imsgLen += iret;
					if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand402") == true) {
						UserSendCharDelayed(curUser, msg, imsgLen);
					}
					return true;
				}

				sCommand += 11;

                if(dlen-11 > 256) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cstopscript <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
                        SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_SCRIPTNAME_LWR], LanguageManager->sTexts[LAN_MAX_ALWD_SCRIPT_NAME_LEN_256_CHARS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand403") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

				Script * curScript = ScriptManager->FindScript(sCommand);
				if(curScript == NULL || curScript->bEnabled == false || curScript->LUA == NULL) {
					int imsgLen = CheckFromPm(curUser, fromPM);

					int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_ERROR_SCRIPT], sCommand,
                        LanguageManager->sTexts[LAN_NOT_RUNNING]);
					imsgLen += iret;
					if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand404") == true) {
						UserSendCharDelayed(curUser, msg, imsgLen);
					}
					return true;
				}

				UncountDeflood(curUser, fromPM);

				ScriptManager->StopScript(curScript, true);

				if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
					if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
						int imsgLen = sprintf(msg, "%s $<%s> *** %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                            curUser->sNick, LanguageManager->sTexts[LAN_STOPPED_SCRIPT], sCommand);
						if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand405") == true) {
							g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
						}
					} else {
						int imsgLen = sprintf(msg, "<%s> *** %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick, LanguageManager->sTexts[LAN_STOPPED_SCRIPT],
                            sCommand);
						if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand406") == true) {
							g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
						}
					}
				}

				if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false ||
					((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
					int imsgLen = CheckFromPm(curUser, fromPM);

					int iret = sprintf(msg+imsgLen, "<%s> %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SCRIPT], sCommand,
                        LanguageManager->sTexts[LAN_STOPPED_LWR]);
					imsgLen += iret;
					if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand408") == true) {
						UserSendCharDelayed(curUser, msg, imsgLen);
					}
				}

				return true;
			}

			// Hub commands: !startscript scriptname
			if(strncasecmp(sCommand+1, "tartscript ", 11) == 0) {
				if(ProfileMan->IsAllowed(curUser, ProfileManager::RSTSCRIPTS) == false) {
					SendNoPermission(curUser, fromPM);
					return true;
				}

				if(SettingManager->bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
					int imsgLen = CheckFromPm(curUser, fromPM);

					int iret = sprintf(msg+imsgLen, "<%s> *** %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_ERR_SCRIPTS_DISABLED]);
					imsgLen += iret;
					if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand412") == true) {
						UserSendCharDelayed(curUser, msg, imsgLen);
					}
					return true;
				}

				if(dlen < 13) {
					int imsgLen = CheckFromPm(curUser, fromPM);

					int iret = sprintf(msg+imsgLen, "<%s> *** %s %cstartscript <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
                        SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_SCRIPTNAME_LWR], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
					imsgLen += iret;
					if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand414") == true) {
						UserSendCharDelayed(curUser, msg, imsgLen);
					}
					return true;
				}

				sCommand += 12;

                if(dlen-12 > 256) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cstartscript <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
                        SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_SCRIPTNAME_LWR], LanguageManager->sTexts[LAN_MAX_ALWD_SCRIPT_NAME_LEN_256_CHARS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand415") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                char * sBadChar = strpbrk(sCommand, "/\\");
				if(sBadChar != NULL || FileExist((SCRIPT_PATH+sCommand).c_str()) == false) {
					int imsgLen = CheckFromPm(curUser, fromPM);

					int iret = sprintf(msg+imsgLen, "<%s> *** %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_ERROR_SCRIPT_NOT_EXIST]);
					imsgLen += iret;
					if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand426") == true) {
						UserSendCharDelayed(curUser, msg, imsgLen);
					}
					return true;
				}

				Script * curScript = ScriptManager->FindScript(sCommand);
				if(curScript != NULL) {
					if(curScript->bEnabled == true && curScript->LUA != NULL) {
						int imsgLen = CheckFromPm(curUser, fromPM);

						int iret = sprintf(msg+imsgLen, "<%s> *** %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_ERROR_SCRIPT_ALREDY_RUNNING]);
						imsgLen += iret;
						if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand416") == true) {
							UserSendCharDelayed(curUser, msg, imsgLen);
						}
						return true;
					}

					UncountDeflood(curUser, fromPM);

					if(ScriptManager->StartScript(curScript, true) == true) {
						if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
							if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
								int imsgLen = sprintf(msg, "%s $<%s> *** %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                                    curUser->sNick, LanguageManager->sTexts[LAN_STARTED_SCRIPT], sCommand);
								if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand419") == true) {
									g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
								}
							} else {
								int imsgLen = sprintf(msg, "<%s> *** %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick,
                                    LanguageManager->sTexts[LAN_STARTED_SCRIPT], sCommand);
								if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand420") == true) {
									g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
								}
							}
						}

						if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
							int imsgLen = CheckFromPm(curUser, fromPM);

							int iret = sprintf(msg+imsgLen, "<%s> %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SCRIPT], sCommand,
                                LanguageManager->sTexts[LAN_STARTED_LWR]);
							imsgLen += iret;
							if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand422") == true) {
								UserSendCharDelayed(curUser, msg, imsgLen);
							}
						}
						return true;
					} else {
						int imsgLen = CheckFromPm(curUser, fromPM);

						int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_ERROR_SCRIPT], sCommand,
                            LanguageManager->sTexts[LAN_START_FAILED]);
						imsgLen += iret;
						if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand424") == true) {
							UserSendCharDelayed(curUser, msg, imsgLen);
						}
						return true;
					}
				}

				UncountDeflood(curUser, fromPM);

				if(ScriptManager->AddScript(sCommand, true, true) == true && ScriptManager->StartScript(ScriptManager->ScriptTable[ScriptManager->ui8ScriptCount-1], false) == true) {
					if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
						if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
							int imsgLen = sprintf(msg, "%s $<%s> *** %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                                curUser->sNick, LanguageManager->sTexts[LAN_STARTED_SCRIPT], sCommand);
							if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand427") == true) {
								g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
							}
						} else {
							int imsgLen = sprintf(msg, "<%s> *** %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick, LanguageManager->sTexts[LAN_STARTED_SCRIPT],
                                sCommand);
							if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand428") == true) {
								g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
							}
						}
					}

					if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
						int imsgLen = CheckFromPm(curUser, fromPM);

						int iret = sprintf(msg+imsgLen, "<%s> %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SCRIPT], sCommand,
                            LanguageManager->sTexts[LAN_STARTED_LWR]);
						imsgLen += iret;
						if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand430") == true) {
							UserSendCharDelayed(curUser, msg, imsgLen);
						}
					}
					return true;
				} else {
					int imsgLen = CheckFromPm(curUser, fromPM);

					int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_ERROR_SCRIPT], sCommand,
                        LanguageManager->sTexts[LAN_START_FAILED]);
					imsgLen += iret;
					if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand432") == true) {
						UserSendCharDelayed(curUser, msg, imsgLen);
					}
					return true;
				}
			}

			return false;

		case 'p':
            //Hub-Commands: !passwd
			if(strncasecmp(sCommand+1, "asswd ", 6) == 0) {
                RegUser * pReg = hashRegManager->Find(curUser);
                if(curUser->iProfile == -1 || pReg == NULL) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_ARE_NOT_ALLOWED_TO_CHANGE_PASS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand434") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                if(dlen < 8) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cpasswd <%s>. %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
                        SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NEW_PASSWORD], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand436") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                if(dlen > 71) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_MAX_ALWD_PASS_LEN_64_CHARS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand438") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

            	if(strchr(sCommand+7, '|') != NULL) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_NO_PIPE_IN_PASS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand439") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                size_t szPassLen = strlen(sCommand+7);

                char * sOldPass = pReg->sPass;

#ifdef _WIN32
                pReg->sPass = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldPass, szPassLen+1);
#else
				pReg->sPass = (char *)realloc(sOldPass, szPassLen+1);
#endif
                if(pReg->sPass == NULL) {
                    pReg->sPass = sOldPass;

					AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes for Hub-Commands passwd\n", (uint64_t)(szPassLen+1));

                    return true;
                }   
                memcpy(pReg->sPass, sCommand+7, szPassLen);
                pReg->sPass[szPassLen] = '\0';

#ifdef _BUILD_GUI
                if(pRegisteredUsersDialog != NULL) {
                    pRegisteredUsersDialog->RemoveReg(pReg);
                    pRegisteredUsersDialog->AddReg(pReg);
                }
#endif

                int imsgLen = CheckFromPm(curUser, fromPM);

                int iret = sprintf(msg+imsgLen, "<%s> *** %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOUR_PASSWORD_UPDATE_SUCCESS]);
                imsgLen += iret;
                if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand442") == true) {
                    UserSendCharDelayed(curUser, msg, imsgLen);
                }
                return true;
            }

            // Hub commands: !permunban
			if(strncasecmp(sCommand+1, "ermunban ", 9) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::UNBAN) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen < 11 || sCommand[10] == '\0') {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cpermunban <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
                        SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_IP_OR_NICK], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand444") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                sCommand += 10;

                if(dlen > 100) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cpermunban <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
                        SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_IP_OR_NICK], LanguageManager->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand445") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                if(hashBanManager->PermUnban(sCommand) == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SORRY], sCommand,
                        LanguageManager->sTexts[LAN_IS_NOT_IN_BANS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand446") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

				UncountDeflood(curUser, fromPM);

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                   	if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
           	            int imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                            curUser->sNick, LanguageManager->sTexts[LAN_REMOVED_LWR], sCommand, LanguageManager->sTexts[LAN_FROM_BANS]);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand447") == true) {
                            g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                        }
              	    } else {
                   	    int imsgLen = sprintf(msg, "<%s> *** %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick, LanguageManager->sTexts[LAN_REMOVED_LWR],
                            sCommand, LanguageManager->sTexts[LAN_FROM_BANS]);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand448") == true) {
          	        	    g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                        }
          	    	}
                }

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand, LanguageManager->sTexts[LAN_REMOVED_FROM_BANS]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand450") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                }
                return true;
            }

            return false;

        case 'f':
            // Hub commands: !fullban nick reason
			if(strncasecmp(sCommand+1, "ullban ", 7) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::BAN) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen < 9) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cfullban <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
                        SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR], LanguageManager->sTexts[LAN_REASON_LWR],
                        LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand452") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                return Ban(curUser, sCommand+8, fromPM, true);
            }

            // Hub commands: !fullbanip ip reason
			if(strncasecmp(sCommand+1, "ullbanip ", 9) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::BAN) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen < 16) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cfullbanip <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
                        SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_IP], LanguageManager->sTexts[LAN_REASON_LWR],
                        LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand454") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                return BanIp(curUser, sCommand+10, fromPM, true);
            }

            // Hub commands: !fulltempban nick time reason ... PPK m = minutes, h = hours, d = day, w = weeks, M = months (30 day per month), Y = years (365 day per year)
			if(strncasecmp(sCommand+1, "ulltempban ", 11) == 0) {
            	if(ProfileMan->IsAllowed(curUser, ProfileManager::TEMP_BAN) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen < 16) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cfulltempban <%s> <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR],
                        LanguageManager->sTexts[LAN_TIME_LWR], LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand456") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                return TempBan(curUser, sCommand+12, dlen-12, fromPM, true);
            }

            // !fulltempbanip ip time reason ... PPK m = minutes, h = hours, d = day, w = weeks, M = months (30 day per month), Y = years (365 day per year)
			if(strncasecmp(sCommand+1, "ulltempbanip ", 13) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::TEMP_BAN) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen < 24) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cfulltempbanip <%s> <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_IP],
                        LanguageManager->sTexts[LAN_TIME_LWR], LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand458") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                return TempBanIp(curUser, sCommand+14, dlen-14, fromPM, true);
            }

            // Hub commands: !fullrangeban fromip toip reason
			if(strncasecmp(sCommand+1, "ullrangeban ", 12) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::RANGE_BAN) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen < 28) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cfullrangeban <%s> <%s> <%s>. %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_FROMIP],
                        LanguageManager->sTexts[LAN_TOIP], LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand460") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                return RangeBan(curUser, sCommand+13, dlen-13, fromPM, true);
            }

            // Hub commands: !fullrangetempban fromip toip time reason
			if(strncasecmp(sCommand+1, "ullrangetempban ", 16) == 0) {
                if(ProfileMan->IsAllowed(curUser, ProfileManager::RANGE_TBAN) == false) {
                    SendNoPermission(curUser, fromPM);
                    return true;
                }

                if(dlen < 35) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

               	    int iret = sprintf(msg+imsgLen, "<%s> *** %s %cfullrangetempban <%s> <%s> <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_FROMIP],
                        LanguageManager->sTexts[LAN_TOIP], LanguageManager->sTexts[LAN_TIME_LWR], LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_NO_PARAM_GIVEN]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand462") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    return true;
                }

                return RangeTempBan(curUser, sCommand+17, dlen-17, fromPM, true);
            }
            
            return false;
    }

/*	int imsgLen = sprintf(msg, "<%s> *** Unknown command.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
    UserSendCharDelayed(curUser, msg, imsgLen);
    
    return(sCommand[0] == SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0]); // returns TRUE for unknown command, FALSE for Not_a_command*/
	return false; // PPK ... and send to all as chat ;)
}
//---------------------------------------------------------------------------

bool HubCommands::Ban(User * curUser, char * sCommand, bool fromPM, bool bFull) {
    char *reason = strchr(sCommand, ' ');
    if(reason != NULL) {
        reason[0] = '\0';
        if(reason[1] == '\0') {
            reason = NULL;
        } else {
            reason++;
        }
    }

    if(sCommand[0] == '\0') {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> *** %s %c%sban <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
            SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager->sTexts[LAN_NICK_LWR],
            LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_BAD_PARAMS_GIVEN]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand464") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
        return true;
    }

    if(strlen(sCommand) > 100) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> *** %s %c%sban <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
            SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager->sTexts[LAN_NICK_LWR],
            LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand465") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
        return true;
    }

    // Self-ban ?
	if(strcasecmp(sCommand, curUser->sNick) == 0) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_CANT_BAN_YOURSELF]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand466") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
        return true;
    }

    User *uban = hashManager->FindUser(sCommand, strlen(sCommand));
    if(uban != NULL) {
        // PPK don't ban user with higher profile
        if(uban->iProfile != -1 && curUser->iProfile > uban->iProfile) {
            int imsgLen = CheckFromPm(curUser, fromPM);

            int iret = sprintf(msg+imsgLen, "<%s> %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_ARE_NOT_ALWD_TO_BAN], sCommand);
            imsgLen += iret;
            if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand470") == true) {
                UserSendCharDelayed(curUser, msg, imsgLen);
            }
            return true;
        }

        UncountDeflood(curUser, fromPM);

        // Ban user
        hashBanManager->Ban(uban, reason, curUser->sNick, bFull);

        // Send user a message that he has been banned
        int imsgLen;
        if(reason != NULL && strlen(reason) > 512) {
            imsgLen = sprintf(msg, "<%s> %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_HAD_BEEN_BANNED_BCS]);
            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand471-1") == false) {
                return true;
            }
            memcpy(msg+imsgLen, reason, 512);
            imsgLen += (int)strlen(reason) + 2;
            msg[imsgLen-2] = '.';
            msg[imsgLen-1] = '|';
            msg[imsgLen] = '\0';
        } else {
            imsgLen = sprintf(msg, "<%s> %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_HAD_BEEN_BANNED_BCS],
                reason == NULL ? LanguageManager->sTexts[LAN_NO_REASON_SPECIFIED] : reason);
            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand471-2") == false) {
                return true;
            }
        }

        UserSendChar(uban, msg, imsgLen);

        // Send message to all OPs that the user have been banned
        if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
            if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                if(reason != NULL && strlen(reason) > 512) {
                    imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s %s %s%s %s %s %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        sCommand, LanguageManager->sTexts[LAN_WITH_IP], uban->sIP, LanguageManager->sTexts[LAN_HAS_BEEN], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "",
                        LanguageManager->sTexts[LAN_BANNED_LWR], LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick, LanguageManager->sTexts[LAN_BECAUSE_LWR]);
                    if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand472-1") == false) {
                        return true;
                    }
                    memcpy(msg+imsgLen, reason, 512);
                    imsgLen += (int)strlen(reason) + 2;
                    msg[imsgLen-2] = '.';
                    msg[imsgLen-1] = '|';
                    msg[imsgLen] = '\0';
                } else {
                    imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s %s %s%s %s %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand, LanguageManager->sTexts[LAN_WITH_IP], uban->sIP, LanguageManager->sTexts[LAN_HAS_BEEN],
                        bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_BANNED_LWR], LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick,
                        LanguageManager->sTexts[LAN_BECAUSE_LWR], reason == NULL ? LanguageManager->sTexts[LAN_NO_REASON_SPECIFIED] : reason);
                    if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand472-2") == false) {
                        return true;
                    }
                }

                g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
            } else {
                if(reason != NULL && strlen(reason) > 512) {
                    imsgLen = sprintf(msg, "<%s> *** %s %s %s %s %s%s %s %s %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand, LanguageManager->sTexts[LAN_WITH_IP], uban->sIP,
                        LanguageManager->sTexts[LAN_HAS_BEEN], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_BANNED_LWR],
                        LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick, LanguageManager->sTexts[LAN_BECAUSE_LWR]);
                    if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand473-1") == false) {
                        return true;
                    }
                    memcpy(msg+imsgLen, reason, 512);
                    imsgLen += (int)strlen(reason) + 2;
                    msg[imsgLen-2] = '.';
                    msg[imsgLen-1] = '|';
                    msg[imsgLen] = '\0';
                } else {
                    imsgLen = sprintf(msg, "<%s> *** %s %s %s %s %s%s %s %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand, LanguageManager->sTexts[LAN_WITH_IP], uban->sIP,
                        LanguageManager->sTexts[LAN_HAS_BEEN], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_BANNED_LWR],
                        LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick, LanguageManager->sTexts[LAN_BECAUSE_LWR], reason == NULL ? LanguageManager->sTexts[LAN_NO_REASON_SPECIFIED] : reason);
                    if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand473-2") == false) {
                        return true;
                    }
                }

                g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
            }
        }

        if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
            imsgLen = CheckFromPm(curUser, fromPM);

            int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s %s %s%s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand, LanguageManager->sTexts[LAN_WITH_IP], uban->sIP,
                LanguageManager->sTexts[LAN_HAS_BEEN], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_BANNED_LWR]);
            imsgLen += iret;
            if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand475") == true) {
                UserSendCharDelayed(curUser, msg, imsgLen);
            }
        }

        // Finish him !
        imsgLen = sprintf(msg, "[SYS] User %s (%s) %sbanned by %s", uban->sNick, uban->sIP, bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", curUser->sNick);
        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand476") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(uban);
        return true;
    } else if(bFull == true) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_ERROR], sCommand,
            LanguageManager->sTexts[LAN_IS_NOT_ONLINE]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand468") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
        return true;
    } else {
        return NickBan(curUser, sCommand, reason, fromPM);
    }
}
//---------------------------------------------------------------------------

bool HubCommands::BanIp(User * curUser, char * sCommand, bool fromPM, bool bFull) {
    char *reason = strchr(sCommand, ' ');
    if(reason != NULL) {
        reason[0] = '\0';
        if(reason[1] == '\0') {
            reason = NULL;
        } else {
            reason++;
        }
    }

    if(sCommand[0] == '\0') {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> *** %s %c%sbanip <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
            SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager->sTexts[LAN_IP],
            LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_BAD_PARAMS_GIVEN]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand478") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
        return true;
    }

    switch(hashBanManager->BanIp(NULL, sCommand, reason, curUser->sNick, bFull)) {
        case 0: {
			UncountDeflood(curUser, fromPM);

			uint8_t ui128Hash[16];
			memset(ui128Hash, 0, 16);

            HashIP(sCommand, ui128Hash);
          
            User *next = hashManager->FindUser(ui128Hash);
            while(next != NULL) {
            	User *cur = next;
                next = cur->hashiptablenext;

                if(cur == curUser || (cur->iProfile != -1 && ProfileMan->IsAllowed(cur, ProfileManager::ENTERIFIPBAN) == true)) {
                    continue;
                }

                // PPK don't nickban user with higher profile
                if(cur->iProfile != -1 && curUser->iProfile > cur->iProfile) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_NOT_ALLOWED_TO],
                        LanguageManager->sTexts[LAN_BAN_LWR], cur->sNick);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::BanIp1") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    continue;
                }

                int imsgLen;
                if(reason != NULL && strlen(reason) > 512) {
                    imsgLen = sprintf(msg, "<%s> %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_HAD_BEEN_BANNED_BCS]);
                    if(CheckSprintf(imsgLen, 1024, "HubCommands::BanIp2-1") == false) {
                        return true;
                    }
                    memcpy(msg+imsgLen, reason, 512);
                    imsgLen += (int)strlen(reason) + 2;
                    msg[imsgLen-2] = '.';
                    msg[imsgLen-1] = '|';
                    msg[imsgLen] = '\0';
                } else {
                    imsgLen = sprintf(msg, "<%s> %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_HAD_BEEN_BANNED_BCS],
                        reason == NULL ? LanguageManager->sTexts[LAN_NO_REASON_SPECIFIED] : reason);
                    if(CheckSprintf(imsgLen, 1024, "HubCommands::BanIp2-2") == false) {
                        return true;
                    }
                }

                UserSendChar(cur, msg, imsgLen);

                imsgLen = sprintf(msg, "[SYS] User %s (%s) ipbanned by %s", cur->sNick, cur->sIP, curUser->sNick);
                if(CheckSprintf(imsgLen, 1024, "HubCommands::BanIp3") == true) {
                    UdpDebug->Broadcast(msg, imsgLen);
                }
                UserClose(cur);
            }

            if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                    int imsgLen;
                    if(reason == NULL) {
                        imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s%s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                            sCommand, LanguageManager->sTexts[LAN_IS_LWR], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_BANNED_LWR],
                            LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand479") == false) {
                            return true;
                        }
                    } else {
                        if(strlen(reason) > 512) {
                            imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s%s %s %s %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                                SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand, LanguageManager->sTexts[LAN_IS_LWR], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "",
                                LanguageManager->sTexts[LAN_BANNED_LWR], LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick, LanguageManager->sTexts[LAN_BECAUSE_LWR]);
                            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand480-1") == false) {
                                return true;
                            }
                            memcpy(msg+imsgLen, reason, 512);
                            imsgLen += (int)strlen(reason) + 2;
                            msg[imsgLen-2] = '.';
                            msg[imsgLen-1] = '|';
                            msg[imsgLen] = '\0';
                        } else {
                            imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s%s %s %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                                SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand, LanguageManager->sTexts[LAN_IS_LWR], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "",
                                LanguageManager->sTexts[LAN_BANNED_LWR], LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick, LanguageManager->sTexts[LAN_BECAUSE_LWR], reason);
                            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand480-2") == false) {
                                return true;
                           }
                        }
                    }

					g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                } else {
                    int imsgLen;
                    if(reason == NULL) {
                        imsgLen = sprintf(msg, "<%s> *** %s %s %s%s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand, LanguageManager->sTexts[LAN_IS_LWR],
                            bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_BANNED_LWR], LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick);
                        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand479") == false) {
                            return true;
                        }
                    } else {
                        if(strlen(reason) > 512) {
                            imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s%s %s %s %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                                SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand, LanguageManager->sTexts[LAN_IS_LWR], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "",
                                LanguageManager->sTexts[LAN_BANNED_LWR], LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick, LanguageManager->sTexts[LAN_BECAUSE_LWR]);
                            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand480-1") == false) {
                                return true;
                            }
                            memcpy(msg+imsgLen, reason, 512);
                            imsgLen += (int)strlen(reason) + 2;
                            msg[imsgLen-2] = '.';
                            msg[imsgLen-1] = '|';
                            msg[imsgLen] = '\0';
                        } else {
                            imsgLen = sprintf(msg, "<%s> *** %s %s %s%s %s %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand, LanguageManager->sTexts[LAN_IS_LWR],
                                bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_BANNED_LWR], LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick,
                                LanguageManager->sTexts[LAN_BECAUSE_LWR], reason);
                            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand480-2") == false) {
                                return true;
                           }
                        }
                    }

                    g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                }
            }

            if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                int imsgLen = CheckFromPm(curUser, fromPM);

                int iret = sprintf(msg+imsgLen, "<%s> %s %s %s%s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand, LanguageManager->sTexts[LAN_IS_LWR],
                    bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_BANNED_LWR]);
                imsgLen += iret;
                if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand484") == true) {
                    UserSendCharDelayed(curUser, msg, imsgLen);
                }
            }
            return true;
        }
        case 1: {
            int imsgLen = CheckFromPm(curUser, fromPM);

            int iret = sprintf(msg+imsgLen, "<%s> *** %s %c%sbanip <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
                SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager->sTexts[LAN_IP],
                LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_NO_VALID_IP_SPECIFIED]);
            imsgLen += iret;
            if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand486") == true) {
                UserSendCharDelayed(curUser, msg, imsgLen);
            }
            return true;
        }
        case 2: {
            int imsgLen = CheckFromPm(curUser, fromPM);

            int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s %s%s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_IP], sCommand,
                LanguageManager->sTexts[LAN_IS_ALREADY], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_BANNED_LWR]);
            imsgLen += iret;
            if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand488") == true) {
                UserSendCharDelayed(curUser, msg, imsgLen);
            }
            return true;
        }
        default:
            return true;
    }
}
//---------------------------------------------------------------------------

bool HubCommands::NickBan(User * curUser, char * sNick, char * sReason, bool bFromPM) {
    RegUser * pReg = hashRegManager->Find(sNick, strlen(sNick));

    // don't nickban user with higher profile
    if(pReg != NULL && curUser->iProfile > pReg->iProfile) {
        int imsgLen = CheckFromPm(curUser, bFromPM);

        int iret = sprintf(msg+imsgLen, "<%s> %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_NOT_ALLOWED_TO],
            LanguageManager->sTexts[LAN_BAN_LWR], sNick);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::NickBan0") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
        return true;
    }

    if(hashBanManager->NickBan(NULL, sNick, sReason, curUser->sNick) == true) {
        int imsgLen = sprintf(msg, "[SYS] User %s nickbanned by %s", sNick, curUser->sNick);
        if(CheckSprintf(imsgLen, 1024, "HubCommands::NickBan1") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
    } else {
        int imsgLen = CheckFromPm(curUser, bFromPM);

        int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_NICK], sNick,
            LanguageManager->sTexts[LAN_IS_ALREDY_BANNED]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::NickBan2") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }

        return true;
    }

    UncountDeflood(curUser, bFromPM);

    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        int imsgLen = 0;
        if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
            imsgLen = sprintf(msg, "%s $", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
            if(CheckSprintf(imsgLen, 1024, "HubCommands::NickBan3") == false) {
                return true;
            }
        }

        if(sReason == NULL) {
            int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick, LanguageManager->sTexts[LAN_ADDED_LWR], sNick,
                LanguageManager->sTexts[LAN_TO_BANS]);
            imsgLen += iret;
            if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::NickBan4") == false) {
                return true;
            }
        } else {
            if(strlen(sReason) > 512) {
                int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sNick, LanguageManager->sTexts[LAN_HAS_BEEN_BANNED_BY], curUser->sNick,
                    LanguageManager->sTexts[LAN_BECAUSE_LWR]);
                imsgLen += iret;
                if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::NickBan5") == false) {
                    return true;
                }
                memcpy(msg+imsgLen, sReason, 512);
                imsgLen += (int)strlen(sReason) + 2;
                msg[imsgLen-2] = '.';
                msg[imsgLen-1] = '|';
                msg[imsgLen] = '\0';
            } else {
                int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sNick, LanguageManager->sTexts[LAN_HAS_BEEN_BANNED_BY],
                    curUser->sNick, LanguageManager->sTexts[LAN_BECAUSE_LWR], sReason);
                imsgLen += iret;
                if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::NickBan6") == false) {
                    return true;
                }
            }
        }

        if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
            g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
        } else {
            g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
        }
    }

    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        int imsgLen = CheckFromPm(curUser, bFromPM);

        int iret = sprintf(msg+imsgLen, "<%s> %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sNick, LanguageManager->sTexts[LAN_ADDED_TO_BANS]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::NickBan7") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
    }

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::TempBan(User * curUser, char * sCommand, const size_t &dlen, bool fromPM, bool bFull) {
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

    if(iCmdPartsLen[0] == 0 || iCmdPartsLen[1] == 0) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> *** %s %c%stempban <%s> <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
            SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager->sTexts[LAN_NICK_LWR],
            LanguageManager->sTexts[LAN_TIME_LWR], LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_BAD_PARAMS_GIVEN]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand490") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
        return true;
    }

    if(iCmdPartsLen[0] > 100) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> *** %s %c%stempban <%s> <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
            SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager->sTexts[LAN_NICK_LWR],
            LanguageManager->sTexts[LAN_TIME_LWR], LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand491") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
        return true;
    }

    // Self-ban ?
	if(strcasecmp(sCmdParts[0], curUser->sNick) == 0) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_CANT_BAN_YOURSELF]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand492") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
        return true;
    }
           	    
    User *utempban = hashManager->FindUser(sCmdParts[0], iCmdPartsLen[0]);
    if(utempban != NULL) {
        // PPK don't tempban user with higher profile
        if(utempban->iProfile != -1 && curUser->iProfile > utempban->iProfile) {
            int imsgLen = CheckFromPm(curUser, fromPM);

            int iret = sprintf(msg+imsgLen, "<%s> %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_ARE_NOT_ALWD_TO_TEMPBAN], sCmdParts[0]);
            imsgLen += iret;
            if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand496") == true) {
                UserSendCharDelayed(curUser, msg, imsgLen);
            }
            return true;
        }

        char cTime = sCmdParts[1][iCmdPartsLen[1]-1];
        sCmdParts[1][iCmdPartsLen[1]-1] = '\0';
        int iTime = atoi(sCmdParts[1]);
        time_t acc_time, ban_time;

        if(iTime <= 0 || GenerateTempBanTime(cTime, (uint32_t)iTime, acc_time, ban_time) == false) {
            int imsgLen = CheckFromPm(curUser, fromPM);

            int iret = sprintf(msg+imsgLen, "<%s> *** %s %c%stempban <%s> <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
                SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager->sTexts[LAN_NICK_LWR],
                LanguageManager->sTexts[LAN_TIME_LWR], LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_BAD_TIME_SPECIFIED]);
                imsgLen += iret;
            if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand498") == true) {
                UserSendCharDelayed(curUser, msg, imsgLen);
            }
            return true;
        }

        hashBanManager->TempBan(utempban, sCmdParts[2], curUser->sNick, 0, ban_time, bFull);
        UncountDeflood(curUser, fromPM);
        static char sTime[256];
        strcpy(sTime, formatTime((ban_time-acc_time)/60));

        // Send user a message that he has been tempbanned
        int imsgLen;
        if(sCmdParts[2] != NULL && iCmdPartsLen[2] > 512) {
            imsgLen = sprintf(msg, "<%s> %s: %s %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_HAD_BEEN_TEMP_BANNED_TO], sTime,
                LanguageManager->sTexts[LAN_BECAUSE_LWR]);
            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand499-1") == false) {
                return true;
            }
            memcpy(msg+imsgLen, sCmdParts[2], 512);
            imsgLen += iCmdPartsLen[2] + 2;
            msg[imsgLen-2] = '.';
            msg[imsgLen-1] = '|';
            msg[imsgLen] = '\0';
        } else {
            imsgLen = sprintf(msg, "<%s> %s: %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_HAD_BEEN_TEMP_BANNED_TO], sTime,
                LanguageManager->sTexts[LAN_BECAUSE_LWR], sCmdParts[2] == NULL ? LanguageManager->sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);
            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand499-2") == false) {
                return true;
            }
        }

        UserSendChar(utempban, msg, imsgLen);

        if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
            if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                if(sCmdParts[2] != NULL && iCmdPartsLen[2] > 512) {
                    imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s %s %s%s %s %s %s: %s %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCmdParts[0], LanguageManager->sTexts[LAN_WITH_IP], utempban->sIP, LanguageManager->sTexts[LAN_HAS_BEEN],
                        bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_TEMP_BANNED], LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick,
                        LanguageManager->sTexts[LAN_TO_LWR], sTime, LanguageManager->sTexts[LAN_BECAUSE_LWR]);
                    if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand500-1") == false) {
                        return true;
                    }
                    memcpy(msg+imsgLen, sCmdParts[2], 512);
                    imsgLen += iCmdPartsLen[2] + 2;
                    msg[imsgLen-2] = '.';
                    msg[imsgLen-1] = '|';
                    msg[imsgLen] = '\0';
                } else {
                    imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s %s %s%s %s %s %s: %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCmdParts[0], LanguageManager->sTexts[LAN_WITH_IP], utempban->sIP, LanguageManager->sTexts[LAN_HAS_BEEN],
                        bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_TEMP_BANNED], LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick,
                        LanguageManager->sTexts[LAN_TO_LWR], sTime, LanguageManager->sTexts[LAN_BECAUSE_LWR], sCmdParts[2] == NULL ? LanguageManager->sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);
                    if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand500-2") == false) {
                        return true;
                    }
                }

                g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
            } else {
                if(sCmdParts[2] != NULL && iCmdPartsLen[2] > 512) {
                    imsgLen = sprintf(msg, "<%s> *** %s %s %s %s %s%s %s %s %s: %s %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCmdParts[0], LanguageManager->sTexts[LAN_WITH_IP],
                        utempban->sIP, LanguageManager->sTexts[LAN_HAS_BEEN], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_TEMP_BANNED],
                        LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick, LanguageManager->sTexts[LAN_TO_LWR], sTime, LanguageManager->sTexts[LAN_BECAUSE_LWR]);
                    if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand500-1") == false) {
                        return true;
                    }
                    memcpy(msg+imsgLen, sCmdParts[2], 512);
                    imsgLen += iCmdPartsLen[2] + 2;
                    msg[imsgLen-2] = '.';
                    msg[imsgLen-1] = '|';
                    msg[imsgLen] = '\0';
                } else {
                    imsgLen = sprintf(msg, "<%s> *** %s %s %s %s %s%s %s %s %s: %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCmdParts[0],
                        LanguageManager->sTexts[LAN_WITH_IP], utempban->sIP, LanguageManager->sTexts[LAN_HAS_BEEN], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "",
                        LanguageManager->sTexts[LAN_TEMP_BANNED], LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick, LanguageManager->sTexts[LAN_TO_LWR], sTime,
                        LanguageManager->sTexts[LAN_BECAUSE_LWR], sCmdParts[2] == NULL ? LanguageManager->sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);
                    if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand500-2") == false) {
                        return true;
                    }
                }

                g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
            }
        }

        if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
            imsgLen = CheckFromPm(curUser, fromPM);

            if(sCmdParts[2] != NULL && iCmdPartsLen[2] > 512) {
                int iret = sprintf(msg+imsgLen, "<%s> %s %s %s %s %s%s %s: %s %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCmdParts[0], LanguageManager->sTexts[LAN_WITH_IP],
                    utempban->sIP, LanguageManager->sTexts[LAN_HAS_BEEN], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_TEMP_BANNED],
                    LanguageManager->sTexts[LAN_TO_LWR], sTime, LanguageManager->sTexts[LAN_BECAUSE_LWR]);
                imsgLen += iret;
                if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand503-1") == false) {
                    return true;
                }

                memcpy(msg+imsgLen, sCmdParts[2], 512);
                imsgLen += iCmdPartsLen[2] + 2;
                msg[imsgLen-2] = '.';
                msg[imsgLen-1] = '|';
                msg[imsgLen] = '\0';
            } else {
                int iret = sprintf(msg+imsgLen, "<%s> %s %s %s %s %s%s %s: %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCmdParts[0], LanguageManager->sTexts[LAN_WITH_IP],
                    utempban->sIP, LanguageManager->sTexts[LAN_HAS_BEEN], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_TEMP_BANNED],
                    LanguageManager->sTexts[LAN_TO_LWR], sTime, LanguageManager->sTexts[LAN_BECAUSE_LWR], sCmdParts[2] == NULL ? LanguageManager->sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);
                imsgLen += iret;
                if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand503-2") == false) {
                    return true;
                }
            }

            UserSendCharDelayed(curUser, msg, imsgLen);
        }

        // Finish him !
        imsgLen = sprintf(msg, "[SYS] User %s (%s) %stemp banned by %s", utempban->sNick, utempban->sIP, bFull == true ? "full " : "", curUser->sNick);
        if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand504") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(utempban);
        return true;
    } else if(bFull == true) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_ERROR], sCmdParts[0],
            LanguageManager->sTexts[LAN_IS_NOT_ONLINE]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand494") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
        return true;
    } else {
        return TempNickBan(curUser, sCmdParts[0], sCmdParts[1], iCmdPartsLen[1], sCmdParts[2], fromPM);
    }
}
//---------------------------------------------------------------------------

bool HubCommands::TempBanIp(User * curUser, char * sCommand, const size_t &dlen, bool fromPM, bool bFull) {
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

    if(iCmdPartsLen[0] == 0 || iCmdPartsLen[1] == 0) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> *** %s %c%stempbanip <%s> <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
            SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager->sTexts[LAN_IP],
            LanguageManager->sTexts[LAN_TIME_LWR], LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_BAD_PARAMS_GIVEN]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand506") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
        return true;
    }

    char cTime = sCmdParts[1][iCmdPartsLen[1]-1];
    sCmdParts[1][iCmdPartsLen[1]-1] = '\0';
    int iTime = atoi(sCmdParts[1]);
    time_t acc_time, ban_time;

    if(iTime <= 0 || GenerateTempBanTime(cTime, (uint32_t)iTime, acc_time, ban_time) == false) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> *** %s %c%stempbanip <%s> <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
            SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager->sTexts[LAN_IP],
            LanguageManager->sTexts[LAN_TIME_LWR], LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_BAD_TIME_SPECIFIED]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand508") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
        return true;
    }

    switch(hashBanManager->TempBanIp(NULL, sCmdParts[0], sCmdParts[2], curUser->sNick, 0, ban_time, bFull)) {
        case 0: {
			uint8_t ui128Hash[16];
			memset(ui128Hash, 0, 16);

            HashIP(sCommand, ui128Hash);
          
            User *next = hashManager->FindUser(ui128Hash);
            while(next != NULL) {
            	User *cur = next;
                next = cur->hashiptablenext;

                if(cur == curUser || (cur->iProfile != -1 && ProfileMan->IsAllowed(cur, ProfileManager::ENTERIFIPBAN) == true)) {
                    continue;
                }

                // PPK don't nickban user with higher profile
                if(cur->iProfile != -1 && curUser->iProfile > cur->iProfile) {
                    int imsgLen = CheckFromPm(curUser, fromPM);

                    int iret = sprintf(msg+imsgLen, "<%s> %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_NOT_ALLOWED_TO],
                        LanguageManager->sTexts[LAN_BAN_LWR], cur->sNick);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::TempBanIp1") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                    continue;
                }

                int imsgLen;
                if(sCmdParts[2] != NULL && iCmdPartsLen[2] > 512) {
                    imsgLen = sprintf(msg, "<%s> %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_HAD_BEEN_BANNED_BCS]);
                    if(CheckSprintf(imsgLen, 1024, "HubCommands::TempBanIp2-1") == false) {
                        return true;
                    }
                    memcpy(msg+imsgLen, sCmdParts[2], 512);
                    imsgLen += iCmdPartsLen[2] + 2;
                    msg[imsgLen-2] = '.';
                    msg[imsgLen-1] = '|';
                    msg[imsgLen] = '\0';
                } else {
                    imsgLen = sprintf(msg, "<%s> %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_HAD_BEEN_BANNED_BCS],
                        sCmdParts[2] == NULL ? LanguageManager->sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);
                    if(CheckSprintf(imsgLen, 1024, "HubCommands::TempBanIp2-2") == false) {
                        return true;
                    }
                }

                UserSendChar(cur, msg, imsgLen);

                imsgLen = sprintf(msg, "[SYS] User %s (%s) tempipbanned by %s", cur->sNick, cur->sIP, curUser->sNick);
                if(CheckSprintf(imsgLen, 1024, "HubCommands::TempBanIp3") == true) {
                    UdpDebug->Broadcast(msg, imsgLen);
                }
                UserClose(cur);
            }
            break;
        }
        case 1: {
            int imsgLen = CheckFromPm(curUser, fromPM);

            int iret = sprintf(msg+imsgLen, "<%s> *** %s %c%sbanip <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
                SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager->sTexts[LAN_IP],
                LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_NO_VALID_IP_SPECIFIED]);
            imsgLen += iret;
            if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand510") == true) {
                UserSendCharDelayed(curUser, msg, imsgLen);
            }
            return true;
        }
        case 2: {
            int imsgLen = CheckFromPm(curUser, fromPM);

            int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s %s%s %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_IP], sCmdParts[0],
                LanguageManager->sTexts[LAN_IS_ALREADY], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_BANNED_LWR],
                LanguageManager->sTexts[LAN_TO_LONGER_TIME]);
            imsgLen += iret;
            if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand512") == true) {
                UserSendCharDelayed(curUser, msg, imsgLen);
            }
            return true;
        }
        default:
            return true;
    }

	UncountDeflood(curUser, fromPM);
    static char sTime[256];
    strcpy(sTime, formatTime((ban_time-acc_time)/60));

    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
            int imsgLen;
            if(sCmdParts[2] != NULL && iCmdPartsLen[2] > 512) {
                imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s%s %s %s %s: %s %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                    sCommand, LanguageManager->sTexts[LAN_HAS_BEEN], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_TEMP_BANNED],
                    LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick, LanguageManager->sTexts[LAN_TO_LWR], sTime, LanguageManager->sTexts[LAN_BECAUSE_LWR]);
                if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand513-1") == false) {
                    return true;
                }
                memcpy(msg+imsgLen, sCmdParts[2], 512);
                imsgLen += iCmdPartsLen[2] + 2;
                msg[imsgLen-2] = '.';
                msg[imsgLen-1] = '|';
                msg[imsgLen] = '\0';
            } else {
                imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s%s %s %s %s: %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                    sCommand, LanguageManager->sTexts[LAN_HAS_BEEN], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_TEMP_BANNED],
                    LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick, LanguageManager->sTexts[LAN_TO_LWR], sTime, LanguageManager->sTexts[LAN_BECAUSE_LWR],
                    sCmdParts[2] == NULL ? LanguageManager->sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);
                if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand513-2") == false) {
                    return true;
                }
            }

			g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
        } else {
            int imsgLen;
            if(sCmdParts[2] != NULL && iCmdPartsLen[2] > 512) {
                imsgLen = sprintf(msg, "<%s> *** %s %s %s%s %s %s %s: %s %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand, LanguageManager->sTexts[LAN_HAS_BEEN],
                    bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_TEMP_BANNED], LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick,
                    LanguageManager->sTexts[LAN_TO_LWR], sTime, LanguageManager->sTexts[LAN_BECAUSE_LWR]);
                if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand514-1") == false) {
                    return true;
                }
                memcpy(msg+imsgLen, sCmdParts[2], 512);
                imsgLen += iCmdPartsLen[2] + 2;
                msg[imsgLen-2] = '.';
                msg[imsgLen-1] = '|';
                msg[imsgLen] = '\0';
            } else {
                imsgLen = sprintf(msg, "<%s> *** %s %s %s%s %s %s %s: %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCommand, LanguageManager->sTexts[LAN_HAS_BEEN],
                    bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_TEMP_BANNED], LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick,
                    LanguageManager->sTexts[LAN_TO_LWR], sTime, LanguageManager->sTexts[LAN_BECAUSE_LWR], sCmdParts[2] == NULL ? LanguageManager->sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);
                if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand514-2") == false) {
                    return true;
                }
            }

            g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
        }
    }

    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        if(sCmdParts[2] != NULL && iCmdPartsLen[2] > 512) {
            int iret = sprintf(msg+imsgLen, "<%s> %s %s %s%s %s: %s %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCmdParts[0], LanguageManager->sTexts[LAN_HAS_BEEN],
                bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_TEMP_BANNED], LanguageManager->sTexts[LAN_TO_LWR], sTime,
                LanguageManager->sTexts[LAN_BECAUSE_LWR]);
            imsgLen += iret;
            if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand516-1") == false) {
                return true;
            }

            memcpy(msg+imsgLen, sCmdParts[2], 512);
            imsgLen += iCmdPartsLen[2] + 2;
            msg[imsgLen-2] = '.';
            msg[imsgLen-1] = '|';
            msg[imsgLen] = '\0';
        } else {
            int iret = sprintf(msg+imsgLen, "<%s> %s %s %s%s %s: %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sCmdParts[0], LanguageManager->sTexts[LAN_HAS_BEEN],
                bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_TEMP_BANNED], LanguageManager->sTexts[LAN_TO_LWR], sTime,
                LanguageManager->sTexts[LAN_BECAUSE_LWR], sCmdParts[2] == NULL ? LanguageManager->sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);
            imsgLen += iret;
            if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand516-2") == false) {
                return true;
            }
        }

        UserSendCharDelayed(curUser, msg, imsgLen);
    }

    int imsgLen = sprintf(msg, "[SYS] IP %s %stemp banned by %s", sCmdParts[0], bFull == true ? "full " : "", curUser->sNick);
    if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand517") == true) {
        UdpDebug->Broadcast(msg, imsgLen);
    }
    return true;
}
//---------------------------------------------------------------------------

bool HubCommands::TempNickBan(User * curUser, char * sNick, char * sTime, const size_t &szTimeLen, char * sReason, bool bFromPM) {
    RegUser * pReg = hashRegManager->Find(sNick, strlen(sNick));

    // don't nickban user with higher profile
    if(pReg != NULL && curUser->iProfile > pReg->iProfile) {
        int imsgLen = CheckFromPm(curUser, bFromPM);

        int iret = sprintf(msg+imsgLen, "<%s> %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_NOT_ALLOWED_TO],
            LanguageManager->sTexts[LAN_BAN_LWR], sNick);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::TempNickBan1") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
        return true;
    }

    char cTime = sTime[szTimeLen-1];
    sTime[szTimeLen-1] = '\0';
    int iTime = atoi(sTime);
    time_t acc_time, ban_time;

    if(iTime <= 0 || GenerateTempBanTime(cTime, (uint32_t)iTime, acc_time, ban_time) == false) {
        int imsgLen = CheckFromPm(curUser, bFromPM);

        int iret = sprintf(msg+imsgLen, "<%s> *** %s %cnicktempban <%s> <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
            SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager->sTexts[LAN_NICK_LWR], LanguageManager->sTexts[LAN_TIME_LWR], LanguageManager->sTexts[LAN_REASON_LWR],
            LanguageManager->sTexts[LAN_BAD_TIME_SPECIFIED]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::TempNickBan2") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }

        return true;
    }

    if(hashBanManager->NickTempBan(NULL, sNick, sReason, curUser->sNick, 0, ban_time) == false) {
        int imsgLen = CheckFromPm(curUser, bFromPM);

        int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_NICK], sNick,
            LanguageManager->sTexts[LAN_IS_ALRD_BANNED_LONGER_TIME]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::TempNickBan3") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }

        return true;
    }

	UncountDeflood(curUser, bFromPM);

    char sBanTime[256];
    strcpy(sBanTime, formatTime((ban_time-acc_time)/60));

    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        int imsgLen = 0;
        if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
            imsgLen = sprintf(msg, "%s $", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
            if(CheckSprintf(imsgLen, 1024, "HubCommands::TempNickBan4") == false) {
                return true;
            }
        }

        if(sReason != NULL && strlen(sReason) > 512) {
            int iRet = sprintf(msg+imsgLen, "<%s> *** %s %s %s %s: %s %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sNick, LanguageManager->sTexts[LAN_HAS_BEEN_TMPBND_BY],
                curUser->sNick, LanguageManager->sTexts[LAN_TO_LWR], sBanTime, LanguageManager->sTexts[LAN_BECAUSE_LWR]);
            imsgLen += iRet;
            if(CheckSprintf1(iRet, imsgLen, 1024, "HubCommands::TempNickBan5") == false) {
                return true;
            }
            memcpy(msg+imsgLen, sReason, 512);
            imsgLen += (int)strlen(sReason) + 2;
            msg[imsgLen-2] = '.';
            msg[imsgLen-1] = '|';
            msg[imsgLen] = '\0';
        } else {
            int iRet = sprintf(msg+imsgLen, "<%s> *** %s %s %s %s: %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sNick, LanguageManager->sTexts[LAN_HAS_BEEN_TMPBND_BY],
                curUser->sNick, LanguageManager->sTexts[LAN_TO_LWR], sBanTime, LanguageManager->sTexts[LAN_BECAUSE_LWR],
                sReason == NULL ? LanguageManager->sTexts[LAN_NO_REASON_SPECIFIED] : sReason);
            imsgLen += iRet;
            if(CheckSprintf1(iRet, imsgLen, 1024, "HubCommands::TempNickBan6") == false) {
                return true;
            }
        }

        if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
			g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
        } else {
            g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
        }
    }

    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        int imsgLen = CheckFromPm(curUser, bFromPM);

        if(sReason != NULL && strlen(sReason) > 512) {
            int iret = sprintf(msg+imsgLen, "<%s> %s %s: %s %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sNick, LanguageManager->sTexts[LAN_BEEN_TEMP_BANNED_TO], sBanTime,
                LanguageManager->sTexts[LAN_BECAUSE_LWR]);
            imsgLen += iret;
            if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::TempNickBan7") == false) {
                return true;
            }

            memcpy(msg+imsgLen, sReason, 512);
            imsgLen += (int)strlen(sReason) + 2;
            msg[imsgLen-2] = '.';
            msg[imsgLen-1] = '|';
            msg[imsgLen] = '\0';
        } else {
            int iret = sprintf(msg+imsgLen, "<%s> %s %s: %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sNick, LanguageManager->sTexts[LAN_BEEN_TEMP_BANNED_TO],
                sBanTime, LanguageManager->sTexts[LAN_BECAUSE_LWR], sReason == NULL ? LanguageManager->sTexts[LAN_NO_REASON_SPECIFIED] : sReason);
            imsgLen += iret;
            if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::TempNickBan8") == false) {
                return true;
            }
        }

        UserSendCharDelayed(curUser, msg, imsgLen);
    }

    int imsgLen = sprintf(msg, "[SYS] Nick %s tempbanned by %s", sNick, curUser->sNick);
    if(CheckSprintf(imsgLen, 1024, "HubCommands::TempNickBan9") == false) {
        return true;
    }

    UdpDebug->Broadcast(msg, imsgLen);
    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::RangeBan(User * curUser, char * sCommand, const size_t &dlen, bool fromPM, bool bFull) {
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

    if(iCmdPartsLen[0] > 15 || iCmdPartsLen[1] > 15) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> %s %c%srangeban <%s> <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
            SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager->sTexts[LAN_FROMIP],
            LanguageManager->sTexts[LAN_TOIP], LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_MAX_ALWD_IP_LEN_15_CHARS]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand518") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
        return true;
    }

	uint8_t ui128FromHash[16], ui128ToHash[16];
	memset(ui128FromHash, 0, 16);
	memset(ui128ToHash, 0, 16);

    if(iCmdPartsLen[0] == 0 || iCmdPartsLen[1] == 0 || HashIP(sCmdParts[0], ui128FromHash) == false || HashIP(sCmdParts[1], ui128ToHash) == false) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> %s %c%srangeban <%s> <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
            SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager->sTexts[LAN_FROMIP],
            LanguageManager->sTexts[LAN_TOIP], LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_BAD_PARAMS_GIVEN]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand519") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
        return true;
    }

    if(memcmp(ui128ToHash, ui128FromHash, 16) <= 0) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s %s %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_ERROR_FROM], sCmdParts[0],
            LanguageManager->sTexts[LAN_TO_LWR], sCmdParts[1], LanguageManager->sTexts[LAN_IS_NOT_VALID_RANGE]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand521") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
        return true;
    }

    if(hashBanManager->RangeBan(sCmdParts[0], ui128FromHash, sCmdParts[1], ui128ToHash, sCmdParts[2], curUser->sNick, bFull) == false) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> *** %s %s-%s %s %s%s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_RANGE], sCmdParts[0], sCmdParts[1],
            LanguageManager->sTexts[LAN_IS_ALREADY], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_BANNED_LWR]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand523") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
    }

	UncountDeflood(curUser, fromPM);

    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
            int imsgLen;
            if(sCmdParts[2] == NULL) {
                imsgLen = sprintf(msg, "%s $<%s> *** %s %s-%s %s %s%s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                    LanguageManager->sTexts[LAN_RANGE], sCmdParts[0], sCmdParts[1], LanguageManager->sTexts[LAN_IS_LWR], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "",
                    LanguageManager->sTexts[LAN_BANNED_LWR], LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick);
                if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand524") == false) {
                    return true;
                }
            } else {
                if(iCmdPartsLen[2] > 512) {
                    imsgLen = sprintf(msg, "%s $<%s> *** %s %s-%s %s %s%s %s %s %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        LanguageManager->sTexts[LAN_RANGE], sCmdParts[0], sCmdParts[1], LanguageManager->sTexts[LAN_IS_LWR], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "",
                        LanguageManager->sTexts[LAN_BANNED_LWR], LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick, LanguageManager->sTexts[LAN_BECAUSE_LWR]);
                    if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand525-1") == false) {
                        return true;
                    }
                    memcpy(msg+imsgLen, sCmdParts[2], 512);
                    imsgLen += iCmdPartsLen[2] + 2;
                    msg[imsgLen-2] = '.';
                    msg[imsgLen-1] = '|';
                    msg[imsgLen] = '\0';
                } else {
                    imsgLen = sprintf(msg, "%s $<%s> *** %s %s-%s %s %s%s %s %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_RANGE], sCmdParts[0], sCmdParts[1], LanguageManager->sTexts[LAN_IS_LWR],
                        bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_BANNED_LWR], LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick,
                        LanguageManager->sTexts[LAN_BECAUSE_LWR], sCmdParts[2]);
                    if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand525-2") == false) {
                        return true;
                    }
                }
            }

			g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
        } else {
            int imsgLen;
            if(sCmdParts[2] == NULL) {
                imsgLen = sprintf(msg, "<%s> *** %s %s-%s %s %s%s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_RANGE], sCmdParts[0], sCmdParts[1],
                    LanguageManager->sTexts[LAN_IS_LWR], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_BANNED_LWR],
                    LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick);
                if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand526") == false) {
                    return true;
                }
            } else {
                if(iCmdPartsLen[2] > 512) {
                    imsgLen = sprintf(msg, "<%s> *** %s %s-%s %s %s%s %s %s %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_RANGE], sCmdParts[0],
                        sCmdParts[1], LanguageManager->sTexts[LAN_IS_LWR], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_BANNED_LWR],
                        LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick, LanguageManager->sTexts[LAN_BECAUSE_LWR]);
                    if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand525-1") == false) {
                        return true;
                    }
                    memcpy(msg+imsgLen, sCmdParts[2], 512);
                    imsgLen += iCmdPartsLen[2] + 2;
                    msg[imsgLen-2] = '.';
                    msg[imsgLen-1] = '|';
                    msg[imsgLen] = '\0';
                } else {
                    imsgLen = sprintf(msg, "<%s> *** %s %s-%s %s %s%s %s %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_RANGE], sCmdParts[0],
                        sCmdParts[1], LanguageManager->sTexts[LAN_IS_LWR], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_BANNED_LWR],
                        LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick, LanguageManager->sTexts[LAN_BECAUSE_LWR], sCmdParts[2]);
                    if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand525-2") == false) {
                        return true;
                    }
                }
            }

            g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
        }
    }

    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> %s %s-%s %s %s%s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_RANGE], sCmdParts[0], sCmdParts[1],
            LanguageManager->sTexts[LAN_IS_LWR], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_BANNED_LWR]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand529") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
    }

    return true;
}
//---------------------------------------------------------------------------

bool HubCommands::RangeTempBan(User * curUser, char * sCommand, const size_t &dlen, bool fromPM, bool bFull) {
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

    if(iCmdPartsLen[0] > 15 || iCmdPartsLen[1] > 15) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> %s %c%srangetempban <%s> <%s> <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
            SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager->sTexts[LAN_FROMIP],
            LanguageManager->sTexts[LAN_TOIP], LanguageManager->sTexts[LAN_TIME_LWR], LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_MAX_ALWD_IP_LEN_15_CHARS]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand530") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
        return true;
    }

	uint8_t ui128FromHash[16], ui128ToHash[16];
	memset(ui128FromHash, 0, 16);
	memset(ui128ToHash, 0, 16);

    if(iCmdPartsLen[0] == 0 || iCmdPartsLen[1] == 0 || iCmdPartsLen[2] == 0 || HashIP(sCmdParts[0], ui128FromHash) == false || HashIP(sCmdParts[1], ui128ToHash) == false) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> %s %c%srangetempban <%s> <%s> <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
            SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager->sTexts[LAN_FROMIP],
            LanguageManager->sTexts[LAN_TOIP], LanguageManager->sTexts[LAN_TIME_LWR], LanguageManager->sTexts[LAN_REASON_LWR], LanguageManager->sTexts[LAN_BAD_PARAMS_GIVEN]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand531") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
        return true;
    }

    if(memcmp(ui128ToHash, ui128FromHash, 16) <= 0) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s %s %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_ERROR_FROM], sCmdParts[0],
            LanguageManager->sTexts[LAN_TO_LWR], sCmdParts[1], LanguageManager->sTexts[LAN_IS_NOT_VALID_RANGE]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand533") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
        return true;
    }

    char cTime = sCmdParts[2][iCmdPartsLen[2]-1];
    sCmdParts[2][iCmdPartsLen[2]-1] = '\0';
    int iTime = atoi(sCmdParts[2]);
    time_t acc_time, ban_time;

    if(iTime <= 0 || GenerateTempBanTime(cTime, (uint32_t)iTime, acc_time, ban_time) == false) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> *** %s %c%srangetempban <%s> <%s> <%s> <%s>. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
            LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "",
            LanguageManager->sTexts[LAN_FROMIP], LanguageManager->sTexts[LAN_TOIP], LanguageManager->sTexts[LAN_TIME_LWR], LanguageManager->sTexts[LAN_REASON_LWR],
            LanguageManager->sTexts[LAN_BAD_TIME_SPECIFIED]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand535") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
        return true;
    }

    if(hashBanManager->RangeTempBan(sCmdParts[0], ui128FromHash, sCmdParts[1], ui128ToHash, sCmdParts[3], curUser->sNick, 0, ban_time, bFull) == false) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> *** %s %s-%s %s %s%s %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_RANGE], sCmdParts[0], sCmdParts[1],
            LanguageManager->sTexts[LAN_IS_ALREADY], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_BANNED_LWR],
            LanguageManager->sTexts[LAN_TO_LONGER_TIME]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand537") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
        return true;
    }

	UncountDeflood(curUser, fromPM);
    static char sTime[256];
    strcpy(sTime, formatTime((ban_time-acc_time)/60));

    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
            int imsgLen;
            if(sCmdParts[3] == NULL) {
                imsgLen = sprintf(msg, "%s $<%s> *** %s %s-%s %s %s%s %s %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                    LanguageManager->sTexts[LAN_RANGE], sCmdParts[0], sCmdParts[1], LanguageManager->sTexts[LAN_IS_LWR], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "",
                    LanguageManager->sTexts[LAN_TEMP_BANNED],  LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick, LanguageManager->sTexts[LAN_TO_LWR], sTime);
                if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand538") == false) {
                    return true;
                }
            } else {
                if(iCmdPartsLen[3] > 512) {
                    imsgLen = sprintf(msg, "%s $<%s> *** %s %s-%s %s %s%s %s %s %s: %s %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_RANGE], sCmdParts[0], sCmdParts[1], LanguageManager->sTexts[LAN_IS_LWR],
                        bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_TEMP_BANNED], LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick,
                        LanguageManager->sTexts[LAN_TO_LWR], sTime, LanguageManager->sTexts[LAN_BECAUSE_LWR]);
                    if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand539-1") == false) {
                        return true;
                    }
                    memcpy(msg+imsgLen, sCmdParts[3], 512);
                    imsgLen += iCmdPartsLen[3] + 2;
                    msg[imsgLen-2] = '.';
                    msg[imsgLen-1] = '|';
                    msg[imsgLen] = '\0';
                } else {
                    imsgLen = sprintf(msg, "%s $<%s> *** %s %s-%s %s %s%s %s %s %s: %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_RANGE], sCmdParts[0], sCmdParts[1], LanguageManager->sTexts[LAN_IS_LWR],
                        bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_TEMP_BANNED], LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick,
                        LanguageManager->sTexts[LAN_TO_LWR], sTime, LanguageManager->sTexts[LAN_BECAUSE_LWR], sCmdParts[3]);
                    if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand539-2") == false) {
                        return true;
                    }
                }
            }

			g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
        } else {
            int imsgLen;
            if(sCmdParts[3] == NULL) {
                imsgLen = sprintf(msg, "<%s> *** %s %s-%s %s %s%s %s %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_RANGE], sCmdParts[0],
                    sCmdParts[1], LanguageManager->sTexts[LAN_IS_LWR], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_TEMP_BANNED],
                    LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick, LanguageManager->sTexts[LAN_TO_LWR], sTime);
                if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand540") == false) {
                    return true;
                }
            } else {
                if(iCmdPartsLen[3] > 512) {
                    imsgLen = sprintf(msg, "<%s> *** %s %s-%s %s %s%s %s %s %s: %s %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_RANGE], sCmdParts[0],
                        sCmdParts[1], LanguageManager->sTexts[LAN_IS_LWR], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_TEMP_BANNED],
                        LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick, LanguageManager->sTexts[LAN_TO_LWR], sTime, LanguageManager->sTexts[LAN_BECAUSE_LWR]);
                    if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand539-1") == false) {
                        return true;
                    }
                    memcpy(msg+imsgLen, sCmdParts[3], 512);
                    imsgLen += iCmdPartsLen[3] + 2;
                    msg[imsgLen-2] = '.';
                    msg[imsgLen-1] = '|';
                    msg[imsgLen] = '\0';
                } else {
                    imsgLen = sprintf(msg, "<%s> *** %s %s-%s %s %s%s %s %s %s: %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_RANGE],
                        sCmdParts[0], sCmdParts[1], LanguageManager->sTexts[LAN_IS_LWR], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_TEMP_BANNED],
                        LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick, LanguageManager->sTexts[LAN_TO_LWR], sTime, LanguageManager->sTexts[LAN_BECAUSE_LWR], sCmdParts[3]);
                    if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand539-2") == false) {
                        return true;
                    }
                }
            }

            g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
        }
    }

    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> %s %s-%s %s %s%s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_RANGE], sCmdParts[0], sCmdParts[1],
            LanguageManager->sTexts[LAN_IS_LWR], bFull == true ? LanguageManager->sTexts[LAN_FULL_LWR] : "", LanguageManager->sTexts[LAN_TEMP_BANNED], LanguageManager->sTexts[LAN_TO_LWR], sTime);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand543") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
    }
    return true;
}
//---------------------------------------------------------------------------

bool HubCommands::RangeUnban(User * curUser, char * sCommand, bool fromPM, unsigned char cType) {
	char *toip = strchr(sCommand, ' ');

	if(toip != NULL) {
        toip[0] = '\0';
        toip++;
    }

	uint8_t ui128FromHash[16], ui128ToHash[16];
	memset(ui128FromHash, 0, 16);
	memset(ui128ToHash, 0, 16);

	if(toip == NULL || sCommand[0] == '\0' || toip[0] == '\0' || HashIP(sCommand, ui128FromHash) == false || HashIP(toip, ui128ToHash) == false) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret= sprintf(msg+imsgLen, "<%s> *** %s. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
            LanguageManager->sTexts[LAN_BAD_PARAMS_GIVEN]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand545") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
        return true;
    }

    if(memcmp(ui128ToHash, ui128FromHash, 16) <= 0) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s %s %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_ERROR_FROM], sCommand,
            LanguageManager->sTexts[LAN_TO_LWR], toip, LanguageManager->sTexts[LAN_IS_NOT_VALID_RANGE]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand547") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
        return true;
    }

    if(hashBanManager->RangeUnban(ui128FromHash, ui128ToHash, cType) == false) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> %s %s-%s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_RANGE], sCommand, toip,
            LanguageManager->sTexts[LAN_IS_NOT_IN_MY_RANGE], cType == 1 ? LanguageManager->sTexts[LAN_TEMP_BANS_LWR] : LanguageManager->sTexts[LAN_PERM_BANS_LWR]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand549") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
    }

	UncountDeflood(curUser, fromPM);

    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
            int imsgLen = sprintf(msg, "%s $<%s> *** %s %s-%s %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                LanguageManager->sTexts[LAN_RANGE], sCommand, toip, LanguageManager->sTexts[LAN_IS_REMOVED_FROM_RANGE],
                cType == 1 ? LanguageManager->sTexts[LAN_TEMP_BANS_LWR] : LanguageManager->sTexts[LAN_PERM_BANS_LWR], LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick);
            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand550") == true) {
				g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
            }
        } else {
            int imsgLen = sprintf(msg, "<%s> *** %s %s-%s %s %s by %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_RANGE], sCommand, toip,
                LanguageManager->sTexts[LAN_IS_REMOVED_FROM_RANGE], cType == 1 ? LanguageManager->sTexts[LAN_TEMP_BANS_LWR] : LanguageManager->sTexts[LAN_PERM_BANS_LWR], curUser->sNick);
            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand551") == true) {
                g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
            }
        }
    }

    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> %s %s-%s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_RANGE], sCommand, toip,
            LanguageManager->sTexts[LAN_IS_REMOVED_FROM_RANGE], cType == 1 ? LanguageManager->sTexts[LAN_TEMP_BANS_LWR] : LanguageManager->sTexts[LAN_PERM_BANS_LWR]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand553") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
    }
    
    return true;
}
//---------------------------------------------------------------------------

bool HubCommands::RangeUnban(User * curUser, char * sCommand, bool fromPM) {
	char *toip = strchr(sCommand, ' ');

	if(toip != NULL) {
        toip[0] = '\0';
        toip++;
    }

	uint8_t ui128FromHash[16], ui128ToHash[16];
	memset(ui128FromHash, 0, 16);
	memset(ui128ToHash, 0, 16);

	if(toip == NULL || sCommand[0] == '\0' || toip[0] == '\0' || HashIP(sCommand, ui128FromHash) == false || HashIP(toip, ui128ToHash) == false) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> *** %s. %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SNTX_ERR_IN_CMD],
            LanguageManager->sTexts[LAN_BAD_PARAMS_GIVEN]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand555") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
        return true;
    }

    if(memcmp(ui128ToHash, ui128FromHash, 16) <= 0) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s %s %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_ERROR_FROM], sCommand,
            LanguageManager->sTexts[LAN_TO_LWR], toip, LanguageManager->sTexts[LAN_IS_NOT_VALID_RANGE]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand557") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
        return true;
    }

    if(hashBanManager->RangeUnban(ui128FromHash, ui128ToHash) == false) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> %s %s-%s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_RANGE], sCommand, toip,
            LanguageManager->sTexts[LAN_IS_NOT_IN_MY_RANGE_BANS]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand559") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
    }

    UncountDeflood(curUser, fromPM);

    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
            int imsgLen = sprintf(msg, "%s $<%s> *** %s %s-%s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                LanguageManager->sTexts[LAN_RANGE], sCommand, toip, LanguageManager->sTexts[LAN_IS_REMOVED_FROM_RANGE_BANS_BY], curUser->sNick);
            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand560") == true) {
                g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
            }
        } else {
            int imsgLen = sprintf(msg, "<%s> *** %s %s-%s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_RANGE], sCommand, toip,
                LanguageManager->sTexts[LAN_IS_REMOVED_FROM_RANGE_BANS_BY], curUser->sNick);
            if(CheckSprintf(imsgLen, 1024, "HubCommands::DoCommand561") == true) {
                g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
            }
        }
    }

    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        int imsgLen = CheckFromPm(curUser, fromPM);

        int iret = sprintf(msg+imsgLen, "<%s> %s %s-%s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_RANGE], sCommand, toip,
            LanguageManager->sTexts[LAN_IS_REMOVED_FROM_RANGE_BANS]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand563") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
    }
    
    return true;
}
//---------------------------------------------------------------------------

void HubCommands::SendNoPermission(User * curUser, const bool &fromPM) {
    int imsgLen = CheckFromPm(curUser, fromPM);

    int iret = sprintf(msg+imsgLen, "<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_ARE_NOT_ALWD_TO_USE_THIS_CMD]);
    imsgLen += iret;
    if(CheckSprintf1(iret, imsgLen, 1024, "HubCommands::DoCommand565") == true) {
        UserSendCharDelayed(curUser, msg, imsgLen);
    }
}
//---------------------------------------------------------------------------

int HubCommands::CheckFromPm(User * curUser, const bool &fromPM) {
    if(fromPM == false) {
        return 0;
    }

    int imsglen = sprintf(msg, "$To: %s From: %s $", curUser->sNick, SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
    if(CheckSprintf(imsglen, 1024, "HubCommands::CheckFromPm") == false) {
        return 0;
    }

    return imsglen;
}
//---------------------------------------------------------------------------

void HubCommands::UncountDeflood(User * curUser, const bool &fromPM) {
    if(fromPM == false) {
        if(curUser->ui16ChatMsgs != 0) {
			curUser->ui16ChatMsgs--;
			curUser->ui16ChatMsgs2--;
        }
    } else {
        if(curUser->ui16PMs != 0) {
            curUser->ui16PMs--;
            curUser->ui16PMs2--;
        }
    }
}
//---------------------------------------------------------------------------
