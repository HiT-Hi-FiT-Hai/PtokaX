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
#include "DcCommands.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "GlobalDataQueue.h"
#include "hashBanManager.h"
#include "hashRegManager.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
#include "ZlibUtility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "DeFlood.h"
#include "HubCommands.h"
#include "IP2Country.h"
#include "ResNickManager.h"
#include "TextFileManager.h"
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
	#include "../gui.win/GuiUtil.h"
    #include "../gui.win/MainWindowPageUsersChat.h"
#endif
//---------------------------------------------------------------------------
cDcCommands *DcCommands = NULL;
//---------------------------------------------------------------------------

struct PassBf {
	int iCount;
	PassBf *prev, *next;
	uint8_t ui128IpHash[16];

	PassBf(const uint8_t * ui128Hash);
	~PassBf(void) { };
};
//---------------------------------------------------------------------------

PassBf::PassBf(const uint8_t * ui128Hash) {
	memcpy(ui128IpHash, ui128Hash, 16);
    iCount = 1;
    prev = NULL;
    next = NULL;
}
//---------------------------------------------------------------------------

cDcCommands::cDcCommands() {
	PasswdBfCheck = NULL;
    iStatChat = iStatCmdUnknown = iStatCmdTo = iStatCmdMyInfo = iStatCmdSearch = iStatCmdSR = iStatCmdRevCTM = 0;
    iStatCmdOpForceMove = iStatCmdMyPass = iStatCmdValidate = iStatCmdKey = iStatCmdGetInfo = iStatCmdGetNickList = 0;
	iStatCmdConnectToMe = iStatCmdVersion = iStatCmdKick = iStatCmdSupports = iStatBotINFO = iStatZPipe = 0;
    iStatCmdMultiSearch = iStatCmdMultiConnectToMe = iStatCmdClose = 0;
    msg[0] = '\0';
}
//---------------------------------------------------------------------------

cDcCommands::~cDcCommands() {
	PassBf *next = PasswdBfCheck;

    while(next != NULL) {
        PassBf *cur = next;
		next = cur->next;
		delete cur;
    }

	PasswdBfCheck = NULL;
}
//---------------------------------------------------------------------------

// Process DC data form User
void cDcCommands::PreProcessData(User * curUser, char * sData, const bool &bCheck, const uint32_t &iLen) {
#ifdef _BUILD_GUI
    // Full raw data trace for better logging
    if(::SendMessage(pMainWindowPageUsersChat->hWndPageItems[MainWindowPageUsersChat::BTN_SHOW_COMMANDS], BM_GETCHECK, 0, 0) == BST_CHECKED) {
		int imsglen = sprintf(msg, "%s (%s) > ", curUser->sNick, curUser->sIP);
		if(CheckSprintf(imsglen, 1024, "cDcCommands::PreProcessData1") == true) {
			RichEditAppendText(pMainWindowPageUsersChat->hWndPageItems[MainWindowPageUsersChat::REDT_CHAT], (msg+string(sData, iLen)).c_str());
		}
    }
#endif

    // micro spam
    if(iLen < 5 && bCheck) {
    	#ifdef _DEBUG
    	    int iret = sprintf(msg, ">>> Garbage DATA from %s (%s) -> %s", curUser->sNick, curUser->sIP, sData);
    	    if(CheckSprintf(iret, 1024, "cDcCommands::PreProcessData2") == true) {
                Memo(msg);
            }
        #endif
        int imsgLen = sprintf(msg, "[SYS] Garbage DATA from %s (%s) -> %s", curUser->sNick, curUser->sIP, sData);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::PreProcessData3") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(curUser);
        return;
    }

    static const uint32_t ui32ulti = *((uint32_t *)"ulti");
    static const uint32_t ui32ick = *((uint32_t *)"ick ");

    switch(curUser->ui8State) {
        case User::STATE_KEY_OR_SUP: {
            if(sData[0] == '$') {
                if(memcmp(sData+1, "Supports ", 9) == 0) {
                    iStatCmdSupports++;
                    Supports(curUser, sData, iLen);
                    return;
                } else if(*((uint32_t *)(sData+1)) == *((uint32_t *)"Key ")) {
					iStatCmdKey++;
                    Key(curUser, sData, iLen);
                    return;
                } else if(memcmp(sData+1, "MyNick ", 7) == 0) {
                    MyNick(curUser, sData, iLen);
                    return;
                }
            }
            break;
        }
        case User::STATE_VALIDATE: {
            if(sData[0] == '$') {
                switch(sData[1]) {
                    case 'K':
                        if(memcmp(sData+2, "ey ", 3) == 0) {
                            iStatCmdKey++;
                            if(((curUser->ui32BoolBits & User::BIT_HAVE_SUPPORTS) == User::BIT_HAVE_SUPPORTS) == false)
                                Key(curUser, sData, iLen);

                            return;
                        }
                        break;
                    case 'V':
                        if(memcmp(sData+2, "alidateNick ", 12) == 0) {
                            iStatCmdValidate++;
                            ValidateNick(curUser, sData, iLen);
                            return;
                        }
                        break;
                    case 'M':
                        if(memcmp(sData+2, "yINFO $ALL ", 11) == 0) {
                            iStatCmdMyInfo++;
                            if(((curUser->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == false) {
                                // bad state
                                #ifdef _DBG
                                    int iret = sprintf(msg, "%s (%s) bad state in case $MyINFO: %d", curUser->Nick, curUser->IP, curUser->iState);
                                    if(CheckSprintf(iret, 1024, "cDcCommands::PreProcessData4") == true) {
                                        Memo(msg);
                                    }
                                #endif
                                int imsgLen = sprintf(msg, "[SYS] Bad state (%d) in $MyINFO %s (%s) - user closed.", (int)curUser->ui8State, curUser->sNick, curUser->sIP);
                                if(CheckSprintf(imsgLen, 1024, "cDcCommands::PreProcessData5") == true) {
                                    UdpDebug->Broadcast(msg, imsgLen);
                                }
                                UserClose(curUser);
                                return;
                            }
                            
                            if(MyINFODeflood(curUser, sData, iLen, bCheck) == false) {
                                return;
                            }
                            
                            // PPK [ Strikes back ;) ] ... get nick from MyINFO
                            char *cTemp;
                            if((cTemp = strchr(sData+13, ' ')) == NULL) {
                                if(iLen > 65000) {
                                    sData[65000] = '\0';
                                }

                                int imsgLen = sprintf(g_sBuffer, "[SYS] Attempt to validate empty nick  from %s (%s) - user closed. (QuickList -> %s)", curUser->sNick, curUser->sIP, sData);
                                if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::PreProcessData6") == true) {
                                    UdpDebug->Broadcast(g_sBuffer, imsgLen);
                                }

                                UserClose(curUser);
                                return;
                            }
                            // PPK ... one null please :)
                            cTemp[0] = '\0';
                            
                            if(ValidateUserNick(curUser, sData+13, (cTemp-sData)-13, false) == false) return;
                            
                            cTemp[0] = ' ';
                            
                            // 1st time MyINFO, user is being added to nicklist
                            if(MyINFO(curUser, sData, iLen) == false || curUser->uLogInOut->sPassword != NULL || 
                                ((curUser->ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == true)
                                return;

                            UserAddMeOrIPv4Check(curUser);

                            return;
                        }
                        break;
                    case 'G':
                        if(iLen == 13 && memcmp(sData+2, "etNickList", 10) == 0) {
                            iStatCmdGetNickList++;
                            if(((curUser->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == false &&
                                ((curUser->ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == false) {
                                // bad state
                                #ifdef _DBG
                                    int iret = sprintf(msg, "%s (%s) bad state in case $GetNickList: %d", curUser->Nick, curUser->IP, curUser->iState);
                                    if(CheckSprintf(iret, 1024, "cDcCommands::PreProcessData8") == true) {
                                        Memo(msg);
                                    }
                                #endif
                                int imsgLen = sprintf(msg, "[SYS] Bad state (%d) in $GetNickList %s (%s) - user closed.", (int)curUser->ui8State, curUser->sNick, curUser->sIP);
                                if(CheckSprintf(imsgLen, 1024, "cDcCommands::PreProcessData9") == true) {
                                    UdpDebug->Broadcast(msg, imsgLen);
                                }
                                UserClose(curUser);
                                return;
                            }
                            GetNickList(curUser, sData, iLen, bCheck);
                            return;
                        }
                        break;
                    default:
                        break;
                }                                            
            }
            break;
        }
        case User::STATE_VERSION_OR_MYPASS: {
            if(sData[0] == '$') {
                switch(sData[1]) {
                    case 'V':
                        if(memcmp(sData+2, "ersion ", 7) == 0) {
                            iStatCmdVersion++;
                            Version(curUser, sData, iLen);
                            return;
                        }
                        break;
                    case 'G':
                        if(iLen == 13 && memcmp(sData+2, "etNickList", 10) == 0) {
                            iStatCmdGetNickList++;
                            if(GetNickList(curUser, sData, iLen, bCheck) == true && 
                                ((curUser->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == false) {
                        	   curUser->ui32BoolBits |= User::BIT_GETNICKLIST;
                            }
                            return;
                        }
                        break;
                    case 'M':
                        if(sData[2] == 'y') {
                            if(memcmp(sData+3, "INFO $ALL ", 10) == 0) {
                                iStatCmdMyInfo++;
                                if(MyINFODeflood(curUser, sData, iLen, bCheck) == false) {
                                    return;
                                }
                                
                                // Am I sending MyINFO of someone other ?
                                // OR i try to fuck up hub with some chars after my nick ??? ... PPK
                                if((sData[13+curUser->ui8NickLen] != ' ') || (memcmp(curUser->sNick, sData+13, curUser->ui8NickLen) != 0)) {
                                    if(iLen > 65000) {
                                        sData[65000] = '\0';
                                    }

                                    int imsgLen = sprintf(g_sBuffer, "[SYS] Nick spoofing in myinfo from %s (%s) - user closed. (%s)", curUser->sNick, curUser->sIP, sData);
                                    if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::PreProcessData10") == true) {
                                        UdpDebug->Broadcast(g_sBuffer, imsgLen);
                                    }

                                    UserClose(curUser);
                                    return;
                                }
        
                                if(MyINFO(curUser, sData, iLen) == false || curUser->uLogInOut->sPassword != NULL || 
                                    ((curUser->ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == true)
                                    return;
                                
                                curUser->ui8State = User::STATE_ADDME;

                                UserAddMeOrIPv4Check(curUser);

                                return;
                            } else if(memcmp(sData+3, "Pass ", 5) == 0) {
                                iStatCmdMyPass++;
                                MyPass(curUser, sData, iLen);
                                return;
                            }
                        }
                        break;
                    default:
                        break;
                }
            }
            break;
        }
        case User::STATE_GETNICKLIST_OR_MYINFO: {
            if(sData[0] == '$') {
                if(iLen == 13 && memcmp(sData+1, "GetNickList", 11) == 0) {
                    iStatCmdGetNickList++;
                    if(GetNickList(curUser, sData, iLen, bCheck) == true) {
                        curUser->ui32BoolBits |= User::BIT_GETNICKLIST;
                    }
                    return;
                } else if(memcmp(sData+1, "MyINFO $ALL ", 12) == 0) {
                    iStatCmdMyInfo++;
                    if(MyINFODeflood(curUser, sData, iLen, bCheck) == false) {
                        return;
                    }
                                
                    // Am I sending MyINFO of someone other ?
                    // OR i try to fuck up hub with some chars after my nick ??? ... PPK
                    if((sData[13+curUser->ui8NickLen] != ' ') || (memcmp(curUser->sNick, sData+13, curUser->ui8NickLen) != 0)) {
                        if(iLen > 65000) {
                            sData[65000] = '\0';
                        }

                        int imsgLen = sprintf(g_sBuffer, "[SYS] Nick spoofing in myinfo from %s (%s) - user closed. (%s)", curUser->sNick, curUser->sIP, sData);
                        if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::PreProcessData12") == true) {
                            UdpDebug->Broadcast(g_sBuffer, imsgLen);
                        }

                        UserClose(curUser);
                        return;
                    }
        
                    if(MyINFO(curUser, sData, iLen) == false || curUser->uLogInOut->sPassword != NULL || 
                        ((curUser->ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == true)
                        return;
                    
                    UserAddMeOrIPv4Check(curUser);

                    return;
                }
            }
            break;
        }
        case User::STATE_IPV4_CHECK:
        case User::STATE_ADDME:
        case User::STATE_ADDME_1LOOP: {
            if(sData[0] == '$') {
                switch(sData[1]) {
                    case 'G':
                        if(iLen == 13 && memcmp(sData+2, "etNickList", 10) == 0) {
                            iStatCmdGetNickList++;
                            if(GetNickList(curUser, sData, iLen, bCheck) == true) {
                                curUser->ui32BoolBits |= User::BIT_GETNICKLIST;
                            }
                            return;
                        }
                        break;
                    case 'M': {
                        if(memcmp(sData+2, "yINFO $ALL ", 11) == 0) {
                            iStatCmdMyInfo++;
                            if(MyINFODeflood(curUser, sData, iLen, bCheck) == false) {
                                return;
                            }

                            // Am I sending MyINFO of someone other ?
                            // OR i try to fuck up hub with some chars after my nick ??? ... PPK
                            if((sData[13+curUser->ui8NickLen] != ' ') || (memcmp(curUser->sNick, sData+13, curUser->ui8NickLen) != 0)) {
                                if(iLen > 65000) {
                                    sData[65000] = '\0';
                                }

                                int imsgLen = sprintf(g_sBuffer, "[SYS] Nick spoofing in myinfo from %s (%s) - user closed. (%s)", curUser->sNick, curUser->sIP, sData);
                                if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::PreProcessData14") == true) {
                                    UdpDebug->Broadcast(g_sBuffer, imsgLen);
                                }

                                UserClose(curUser);
                                return;
                            }

                            MyINFO(curUser, sData, iLen);
                            
                            return;
                        } else if(memcmp(sData+2, "ultiSearch ", 11) == 0) {
                            iStatCmdMultiSearch++;
                            SearchDeflood(curUser, sData, iLen, bCheck, true);
                            return;
                        }
                        break;
                    }
                    case 'S':
                        if(memcmp(sData+2, "earch ", 6) == 0) {
                            iStatCmdSearch++;
                            SearchDeflood(curUser, sData, iLen, bCheck, false);
                            return;
                        }
                        break;
                    default:
                        break;
                }
            } else if(sData[0] == '<') {
                iStatChat++;
                ChatDeflood(curUser, sData, iLen, bCheck);
                return;
            }
            break;
        }
        case User::STATE_ADDED: {
            if(sData[0] == '$') {
                switch(sData[1]) {
                    case 'S': {
                        if(memcmp(sData+2, "earch ", 6) == 0) {
                            iStatCmdSearch++;
                            if(SearchDeflood(curUser, sData, iLen, bCheck, false) == true) {
                                Search(curUser, sData, iLen, bCheck, false);
                            }
                            return;
                        } else if(*((uint16_t *)(sData+2)) == *((uint16_t *)"R ")) {
                            iStatCmdSR++;
                            SR(curUser, sData, iLen, bCheck);
                            return;
                        }
                        break;
                    }
                    case 'C':
                        if(memcmp(sData+2, "onnectToMe ", 11) == 0) {
                            iStatCmdConnectToMe++;
                            ConnectToMe(curUser, sData, iLen, bCheck, false);
                            return;
                        } else if(memcmp(sData+2, "lose ", 5) == 0) {
                            iStatCmdClose++;
                            Close(curUser, sData, iLen);
                            return;
                        }
                        break;
                    case 'R':
                        if(memcmp(sData+2, "evConnectToMe ", 14) == 0) {
                            iStatCmdRevCTM++;
                            RevConnectToMe(curUser, sData, iLen, bCheck);
                            return;
                        }
                        break;
                    case 'M':
                        if(memcmp(sData+2, "yINFO $ALL ", 11) == 0) {
                            iStatCmdMyInfo++;
                            if(MyINFODeflood(curUser, sData, iLen, bCheck) == false) {
                                return;
                            }
                                    
                            // Am I sending MyINFO of someone other ?
                            // OR i try to fuck up hub with some chars after my nick ??? ... PPK
                            if((sData[13+curUser->ui8NickLen] != ' ') || (memcmp(curUser->sNick, sData+13, curUser->ui8NickLen) != 0)) {
                                if(iLen > 65000) {
                                    sData[65000] = '\0';
                                }

                                int imsgLen = sprintf(g_sBuffer, "[SYS] Nick spoofing in myinfo from %s (%s) - user closed. (%s)", curUser->sNick, curUser->sIP, sData);
                                if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::PreProcessData16") == true) {
                                    UdpDebug->Broadcast(g_sBuffer, imsgLen);
                                }

                                UserClose(curUser);
                                return;
                            }
                                                  
                            if(MyINFO(curUser, sData, iLen) == true) {
                                curUser->ui32BoolBits |= User::BIT_PRCSD_MYINFO;
                            }
                            return;
                        } else if(*((uint32_t *)(sData+2)) == ui32ulti) {
                            if(memcmp(sData+6, "Search ", 7) == 0) {
                                iStatCmdMultiSearch++;
                                if(SearchDeflood(curUser, sData, iLen, bCheck, true) == true) {
                                    Search(curUser, sData, iLen, bCheck, true);
                                }
                                return;
                            } else if(memcmp(sData+6, "ConnectToMe ", 12) == 0) {
                                iStatCmdMultiConnectToMe++;
                                ConnectToMe(curUser, sData, iLen, bCheck, true);
                                return;
                            }
                        } else if(memcmp(sData+2, "yPass ", 6) == 0) {
                            iStatCmdMyPass++;
                            //MyPass(curUser, sData, iLen);
                            if((curUser->ui32BoolBits & User::BIT_WAITING_FOR_PASS) == User::BIT_WAITING_FOR_PASS) {
                                curUser->ui32BoolBits &= ~User::BIT_WAITING_FOR_PASS;

                                if(curUser->uLogInOut != NULL && curUser->uLogInOut->sPassword != NULL) {
                                    int iProfile = ProfileMan->GetProfileIndex(curUser->uLogInOut->sPassword);
                                    if(iProfile == -1) {
               	                        int iMsgLen = sprintf(msg, "<%s> %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_ERR_NO_PROFILE_GIVEN_NAME_EXIST]);
                                        if(CheckSprintf(iMsgLen, 1024, "cDcCommands::PreProcessData::MyPass->RegUser") == true) {
                                            UserSendCharDelayed(curUser, msg, iMsgLen);
                                        }

                                        delete curUser->uLogInOut;
                                        curUser->uLogInOut = NULL;

                                        return;
                                    }
                                    
                                    if(iLen > 73) {
                                        int iMsgLen = sprintf(msg, "<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_MAX_ALWD_PASS_LEN_64_CHARS]);
                                        if(CheckSprintf(iMsgLen, 1024, "cDcCommands::PreProcessData::MyPass->RegUser1") == true) {
                                            UserSendCharDelayed(curUser, msg, iMsgLen);
                                        }

                                        delete curUser->uLogInOut;
                                        curUser->uLogInOut = NULL;

                                        return;
                                    }

                                    sData[iLen-1] = '\0'; // cutoff pipe

                                    if(hashRegManager->AddNew(curUser->sNick, sData+8, (uint16_t)iProfile) == false) {
                                        int iMsgLen = sprintf(msg, "<%s> %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                                            LanguageManager->sTexts[LAN_SORRY_YOU_ARE_ALREADY_REGISTERED]);
                                        if(CheckSprintf(iMsgLen, 1024, "cDcCommands::PreProcessData::MyPass->RegUser2") == true) {
                                            UserSendCharDelayed(curUser, msg, iMsgLen);
                                        }
                                    } else {
                                        int iMsgLen = sprintf(msg, "<%s> %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                                            LanguageManager->sTexts[LAN_THANK_YOU_FOR_PASSWORD_YOU_ARE_NOW_REGISTERED_AS], curUser->uLogInOut->sPassword);
                                        if(CheckSprintf(iMsgLen, 1024, "cDcCommands::PreProcessData::MyPass->RegUser2") == true) {
                                            UserSendCharDelayed(curUser, msg, iMsgLen);
                                        }
                                    }

                                    delete curUser->uLogInOut;
                                    curUser->uLogInOut = NULL;

                                    curUser->iProfile = iProfile;

                                    if(((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                                        if(ProfileMan->IsAllowed(curUser, ProfileManager::HASKEYICON) == false) {
                                            return;
                                        }
                                        
                                        curUser->ui32BoolBits |= User::BIT_OPERATOR;

                                        colUsers->Add2OpList(curUser);
                                        g_GlobalDataQueue->OpListStore(curUser->sNick);

                                        if(ProfileMan->IsAllowed(curUser, ProfileManager::ALLOWEDOPCHAT) == true) {
                                            if(SettingManager->bBools[SETBOOL_REG_OP_CHAT] == true &&
                                                (SettingManager->bBools[SETBOOL_REG_BOT] == false || SettingManager->bBotsSameNick == false)) {
                                                if(((curUser->ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == false) {
                                                    UserSendCharDelayed(curUser, SettingManager->sPreTexts[SetMan::SETPRETXT_OP_CHAT_HELLO],
                                                        SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_OP_CHAT_HELLO]);
                                                }

                                                UserSendCharDelayed(curUser, SettingManager->sPreTexts[SetMan::SETPRETXT_OP_CHAT_MYINFO],
                                                    SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_OP_CHAT_MYINFO]);
                                                int imsgLen = sprintf(msg, "$OpList %s$$|", SettingManager->sTexts[SETTXT_OP_CHAT_NICK]);
                                                if(CheckSprintf(imsgLen, 1024, "cDcCommands::PreProcessData::MyPass->RegUser3") == true) {
                                                    UserSendCharDelayed(curUser, msg, imsgLen);
                                                }
                                            }
                                        }
                                    }
                                }

                                return;
                            }
                        }
                        break;
                    case 'G': {
                        if(iLen == 13 && memcmp(sData+2, "etNickList", 10) == 0) {
                            iStatCmdGetNickList++;
                            if(GetNickList(curUser, sData, iLen, bCheck) == true) {
                                curUser->ui32BoolBits |= User::BIT_GETNICKLIST;
                            }
                            return;
                        } else if(memcmp(sData+2, "etINFO ", 7) == 0) {
							iStatCmdGetInfo++;
                            GetINFO(curUser, sData, iLen);
                            return;
                        }
                        break;
                    }
                    case 'T':
                        if(memcmp(sData+2, "o: ", 3) == 0) {
                            iStatCmdTo++;
                            To(curUser, sData, iLen, bCheck);
                            return;
                        }
                    case 'K':
                        if(*((uint32_t *)(sData+2)) == ui32ick) {
							iStatCmdKick++;
                            Kick(curUser, sData, iLen);
                            return;
                        }
                        break;
                    case 'O':
                        if(memcmp(sData+2, "pForceMove $Who:", 16) == 0) {
                            iStatCmdOpForceMove++;
                            OpForceMove(curUser, sData, iLen);
                            return;
                        }
                    default:
                        break;
                }
            } else if(sData[0] == '<') {
                iStatChat++;
                if(ChatDeflood(curUser, sData, iLen, bCheck) == true) {
                    Chat(curUser, sData, iLen, bCheck);
                }
                
                return;
            }
            break;
        }
        case User::STATE_CLOSING:
        case User::STATE_REMME:
            return;
    }
    
    // PPK ... fallback to full command identification and disconnect on bad state or unknown command not handled by script
    switch(sData[0]) {
        case '$':
            switch(sData[1]) {
                case 'B': {
					if(memcmp(sData+2, "otINFO", 6) == 0) {
						iStatBotINFO++;
                        BotINFO(curUser, sData, iLen);
                        return;
                    }
                    break;
                }
                case 'C':
                    if(memcmp(sData+2, "onnectToMe ", 11) == 0) {
                        iStatCmdConnectToMe++;
                        #ifdef _DBG
                            int iret sprintf(msg, "%s (%s) bad state in case $ConnectToMe: %d", curUser->Nick, curUser->IP, curUser->iState);
                            if(CheckSprintf(iret, 1024, "cDcCommands::PreProcessData18") == true) {
                                Memo(msg);
                            }
                        #endif
                        int imsgLen = sprintf(msg, "[SYS] Bad state (%d) in $ConnectToMe %s (%s) - user closed.", (int)curUser->ui8State, curUser->sNick, curUser->sIP);
                        if(CheckSprintf(imsgLen, 1024, "cDcCommands::PreProcessData19") == true) {
                            UdpDebug->Broadcast(msg, imsgLen);
                        }
                        UserClose(curUser);
                        return;
                    } else if(memcmp(sData+2, "lose ", 5) == 0) {
                        iStatCmdClose++;
                        #ifdef _DBG
                            int iret = sprintf(msg, "%s (%s) bad state in case $Close: %d", curUser->Nick, curUser->IP, curUser->iState);
                            if(CheckSprintf(iret, 1024, "cDcCommands::PreProcessData20") == true) {
                                Memo(msg);
                            }
                        #endif
                        int imsgLen = sprintf(msg, "[SYS] Bad state (%d) in $Close %s (%s) - user closed.", (int)curUser->ui8State, curUser->sNick, curUser->sIP);
                        if(CheckSprintf(imsgLen, 1024, "cDcCommands::PreProcessData21") == true) {
                            UdpDebug->Broadcast(msg, imsgLen);
                        }
                        UserClose(curUser);
                        return;
                    }
                    break;
                case 'G': {
                    if(*((uint16_t *)(sData+2)) == *((uint16_t *)"et")) {
                        if(memcmp(sData+4, "INFO ", 5) == 0) {
                            iStatCmdGetInfo++;
                            #ifdef _DBG
                                int iret = sprintf(msg, "%s (%s) bad state in case $GetINFO: %d", curUser->Nick, curUser->IP, curUser->iState);
                                if(CheckSprintf(iret, 1024, "cDcCommands::PreProcessData22") == true) {
                                    Memo(msg);
                                }
                            #endif
                            int imsgLen = sprintf(msg, "[SYS] Bad state (%d) in $GetINFO %s (%s) - user closed.", (int)curUser->ui8State, curUser->sNick, curUser->sIP);
                            if(CheckSprintf(imsgLen, 1024, "cDcCommands::PreProcessData23") == true) {
                                UdpDebug->Broadcast(msg, imsgLen);
                            }
                            UserClose(curUser);
                            return;
                        } else if(iLen == 13 && *((uint64_t *)(sData+4)) == *((uint64_t *)"NickList")) {
                            iStatCmdGetNickList++;
                            #ifdef _DBG
                                int iret = sprintf(msg, "%s (%s) bad state in case $GetNickList: %d", curUser->Nick, curUser->IP, curUser->iState);
                                if(CheckSprintf(iret, 1024, "cDcCommands::PreProcessData24") == true) {
                                    Memo(msg);
                                }
                            #endif
                            int imsgLen = sprintf(msg, "[SYS] Bad state (%d) in $GetNickList %s (%s) - user closed.", (int)curUser->ui8State, curUser->sNick, curUser->sIP);
                            if(CheckSprintf(imsgLen, 1024, "cDcCommands::PreProcessData25") == true) {
                                UdpDebug->Broadcast(msg, imsgLen);
                            }
                            UserClose(curUser);
                            return;
                        }
                    }
                    break;
                }
                case 'K':
                    if(memcmp(sData+2, "ey ", 3) == 0) {
                        iStatCmdKey++;
                        #ifdef _DBG
                            int iret = sprintf(msg, "%s (%s) bad state in case $Key: %d", curUser->Nick, curUser->IP, curUser->iState);
                            if(CheckSprintf(iret, 1024, "cDcCommands::PreProcessData26") == true) {
                                Memo(msg);
                            }
                        #endif
                        int imsgLen = sprintf(msg, "[SYS] Bad state (%d) in $Key %s (%s) - user closed.", (int)curUser->ui8State, curUser->sNick, curUser->sIP);
                        if(CheckSprintf(imsgLen, 1024, "cDcCommands::PreProcessData27") == true) {
                            UdpDebug->Broadcast(msg, imsgLen);
                        }
                        UserClose(curUser);
                        return;
                    } else if(*((uint32_t *)(sData+2)) == ui32ick) {
                        iStatCmdKick++;
                        #ifdef _DBG
                            int iret = sprintf(msg, "%s (%s) bad state in case $Kick: %d", curUser->Nick, curUser->IP, curUser->iState);
                            if(CheckSprintf(iret, 1024, "cDcCommands::PreProcessData28") == true) {
                                Memo(msg);
                            }
                        #endif
                        int imsgLen = sprintf(msg, "[SYS] Bad state (%d) in $Kick %s (%s) - user closed.", (int)curUser->ui8State, curUser->sNick, curUser->sIP);
                        if(CheckSprintf(imsgLen, 1024, "cDcCommands::PreProcessData29") == true) {
                            UdpDebug->Broadcast(msg, imsgLen);
                        }
                        UserClose(curUser);
                        return;
                    }
                    break;
                case 'M':
                    if(*((uint32_t *)(sData+2)) == ui32ulti) {
                        if(memcmp(sData+6, "ConnectToMe ", 12) == 0) {
                            iStatCmdMultiConnectToMe++;
                            #ifdef _DBG
                                int iret = sprintf(msg, "%s (%s) bad state in case $MultiConnectToMe: %d", curUser->Nick, curUser->IP, curUser->iState);
                                if(CheckSprintf(iret, 1024, "cDcCommands::PreProcessData30") == true) {
                                    Memo(msg);
                                }
                            #endif
                            int imsgLen = sprintf(msg, "[SYS] Bad state (%d) in $MultiConnectToMe %s (%s) - user closed.", (int)curUser->ui8State, curUser->sNick, curUser->sIP);
                            if(CheckSprintf(imsgLen, 1024, "cDcCommands::PreProcessData31") == true) {
                                UdpDebug->Broadcast(msg, imsgLen);
                            }
                            UserClose(curUser);
                            return;
                        } else if(memcmp(sData+6, "Search ", 7) == 0) {
                            iStatCmdMultiSearch++;
                            #ifdef _DBG
                                int iret = sprintf(msg, "%s (%s) bad state in case $MultiSearch: %d", curUser->Nick, curUser->IP, curUser->iState);
                                if(CheckSprintf(iret, 1024, "cDcCommands::PreProcessData32") == true) {
                                    Memo(msg);
                                }
                            #endif
                            int imsgLen = sprintf(msg, "[SYS] Bad state (%d) in $MultiSearch %s (%s) - user closed.", (int)curUser->ui8State, curUser->sNick, curUser->sIP);
                            if(CheckSprintf(imsgLen, 1024, "cDcCommands::PreProcessData33") == true) {
                                UdpDebug->Broadcast(msg, imsgLen);
                            }
                            UserClose(curUser);
                            return;
                        }
                    } else if(sData[2] == 'y') {
                        if(memcmp(sData+3, "INFO $ALL ", 10) == 0) {
                            iStatCmdMyInfo++;
                            #ifdef _DBG
                                int iret sprintf(msg, "%s (%s) bad state in case $MyINFO: %d", curUser->Nick, curUser->IP, curUser->iState);
                                if(CheckSprintf(iret, 1024, "cDcCommands::PreProcessData34") == true) {
                                    Memo(msg);
                                }
                            #endif
                            int imsgLen = sprintf(msg, "[SYS] Bad state (%d) in $MyINFO %s (%s) - user closed.", (int)curUser->ui8State, curUser->sNick, curUser->sIP);
                            if(CheckSprintf(imsgLen, 1024, "cDcCommands::PreProcessData35") == true) {
                                UdpDebug->Broadcast(msg, imsgLen);
                            }
                            UserClose(curUser);
                            return;
                        } else if(memcmp(sData+3, "Pass ", 5) == 0) {
                            iStatCmdMyPass++;
                            #ifdef _DBG
                                int iret = sprintf(msg, "%s (%s) bad state in case $MyPass: %d", curUser->Nick, curUser->IP, curUser->iState);
                                if(CheckSprintf(iret, 1024, "cDcCommands::PreProcessData36") == true) {
                                    Memo(msg);
                                }
                            #endif
                            int imsgLen = sprintf(msg, "[SYS] Bad state (%d) in $MyPass %s (%s) - user closed.", (int)curUser->ui8State, curUser->sNick, curUser->sIP);
                            if(CheckSprintf(imsgLen, 1024, "cDcCommands::PreProcessData37") == true) {
                                UdpDebug->Broadcast(msg, imsgLen);
                            }
                            UserClose(curUser);
                            return;
                        }
                    }
                    break;
                case 'O':
                    if(memcmp(sData+2, "pForceMove $Who:", 16) == 0) {
                        iStatCmdOpForceMove++;
                        #ifdef _DBG
                            int iret = sprintf(msg, "%s (%s) bad state in case $OpForceMove: %d", curUser->Nick, curUser->IP, curUser->iState);
                            if(CheckSprintf(iret, 1024, "cDcCommands::PreProcessData38") == true) {
                                Memo(msg);
                            }
                        #endif
                        int imsgLen = sprintf(msg, "[SYS] Bad state (%d) in $OpForceMove %s (%s) - user closed.", (int)curUser->ui8State, curUser->sNick, curUser->sIP);
                        if(CheckSprintf(imsgLen, 1024, "cDcCommands::PreProcessData39") == true) {
                            UdpDebug->Broadcast(msg, imsgLen);
                        }
                        UserClose(curUser);
                        return;
                    }
                    break;
                case 'R':
                    if(memcmp(sData+2, "evConnectToMe ", 14) == 0) {
                        iStatCmdRevCTM++;
                        #ifdef _DBG
                            int iret = sprintf(msg, "%s (%s) bad state in case $RevConnectToMe: %d", curUser->Nick, curUser->IP, curUser->iState);
                            if(CheckSprintf(iret, 1024, "cDcCommands::PreProcessData40") == true) {
                                Memo(msg);
                            }
                        #endif
                        int imsgLen = sprintf(msg, "[SYS] Bad state (%d) in $RevConnectToMe %s (%s) - user closed.", (int)curUser->ui8State, curUser->sNick, curUser->sIP);
                        if(CheckSprintf(imsgLen, 1024, "cDcCommands::PreProcessData41") == true) {
                            UdpDebug->Broadcast(msg, imsgLen);
                        }
						UserClose(curUser);
                        return;
                    }
                    break;
                case 'S':
                    switch(sData[2]) {
                        case 'e': {
                            if(memcmp(sData+3, "arch ", 5) == 0) {
                                iStatCmdSearch++;
                                #ifdef _DBG
                                    int iret = sprintf(msg, "%s (%s) bad state in case $Search: %d", curUser->Nick, curUser->IP, curUser->iState);
                                    if(CheckSprintf(iret, 1024, "cDcCommands::PreProcessData42") == true) {
                                        Memo(msg);
                                    }
                                #endif
                                int imsgLen = sprintf(msg, "[SYS] Bad state (%d) in $Search %s (%s) - user closed.", (int)curUser->ui8State, curUser->sNick, curUser->sIP);
                                if(CheckSprintf(imsgLen, 1024, "cDcCommands::PreProcessData43") == true) {
                                    UdpDebug->Broadcast(msg, imsgLen);
                                }
                                UserClose(curUser);
                                return;
                            }
                            break;
                        }
                        case 'R':
                            if(sData[3] == ' ') {
                                iStatCmdSR++;
                                #ifdef _DBG
                                    int iret sprintf(msg, "%s (%s) bad state in case $SR: %d", curUser->Nick, curUser->IP, curUser->iState);
                                    if(CheckSprintf(iret, 1024, "cDcCommands::PreProcessData44") == true) {
                                        Memo(msg);
                                    }
                                #endif
                                int imsgLen = sprintf(msg, "[SYS] Bad state (%d) in $SR %s (%s) - user closed.", (int)curUser->ui8State, curUser->sNick, curUser->sIP);
                                if(CheckSprintf(imsgLen, 1024, "cDcCommands::PreProcessData45") == true) {
                                    UdpDebug->Broadcast(msg, imsgLen);
                                }
                                UserClose(curUser);
                                return;
                            }
                            break;
                        case 'u': {
                            if(memcmp(sData+3, "pports ", 7) == 0) {
                                iStatCmdSupports++;
                                #ifdef _DBG
                                    int iret = sprintf(msg, "%s (%s) bad state in case $Supports: %d", curUser->Nick, curUser->IP, curUser->iState);
                                    if(CheckSprintf(iret, 1024, "cDcCommands::PreProcessData46") == true) {
                                        Memo(msg);
                                    }
                                #endif
                                int imsgLen = sprintf(msg, "[SYS] Bad state (%d) in $Supports %s (%s) - user closed.", (int)curUser->ui8State, curUser->sNick, curUser->sIP);
                                if(CheckSprintf(imsgLen, 1024, "cDcCommands::PreProcessData47") == true) {
                                    UdpDebug->Broadcast(msg, imsgLen);
                                }
                                UserClose(curUser);
                                return;
                            }
                            break;
                        }
                        default:
                            break;
                    }
                    break;
                case 'T':
                    if(memcmp(sData+2, "o: ", 3) == 0) {
                        iStatCmdTo++;
                        #ifdef _DBG
                            int iret sprintf(msg, "%s (%s) bad state in case $To: %d", curUser->Nick, curUser->IP, curUser->iState);
                            if(CheckSprintf(iret, 1024, "cDcCommands::PreProcessData48") == true) {
                                Memo(msg);
                            }
                        #endif
                        int imsgLen = sprintf(msg, "[SYS] Bad state (%d) in $To %s (%s) - user closed.", (int)curUser->ui8State, curUser->sNick, curUser->sIP);
                        if(CheckSprintf(imsgLen, 1024, "cDcCommands::PreProcessData49") == true) {
                            UdpDebug->Broadcast(msg, imsgLen);
                        }
                        UserClose(curUser);
                        return;
                    }
                    break;
                case 'V':
                    if(memcmp(sData+2, "alidateNick ", 12) == 0) {
                        iStatCmdValidate++;
                        #ifdef _DBG
                            int iret = sprintf(msg, "%s (%s) bad state in case $ValidateNick: %d", curUser->Nick, curUser->IP, curUser->iState);
                            if(CheckSprintf(iret, 1024, "cDcCommands::PreProcessData50") == true) {
                                Memo(msg);
                            }
                        #endif
                        int imsgLen = sprintf(msg, "[SYS] Bad state (%d) in $ValidateNick %s (%s) - user closed.", (int)curUser->ui8State, curUser->sNick, curUser->sIP);
                        if(CheckSprintf(imsgLen, 1024, "cDcCommands::PreProcessData51") == true) {
                            UdpDebug->Broadcast(msg, imsgLen);
                        }
                        UserClose(curUser);
                        return;
                    } else if(memcmp(sData+2, "ersion ", 7) == 0) {
                        iStatCmdVersion++;
                        #ifdef _DBG
                            int iret = sprintf(msg, "%s (%s) bad state in case $Version: %d", curUser->Nick, curUser->IP, curUser->iState);
                            if(CheckSprintf(iret, 1024, "cDcCommands::PreProcessData52") == true) {
                                Memo(msg);
                            }
                        #endif
                        int imsgLen = sprintf(msg, "[SYS] Bad state (%d) in $Version %s (%s) - user closed.", (int)curUser->ui8State, curUser->sNick, curUser->sIP);
                        if(CheckSprintf(imsgLen, 1024, "cDcCommands::PreProcessData53") == true) {
                            UdpDebug->Broadcast(msg, imsgLen);
                        }
                        UserClose(curUser);
                        return;
                    }
                    break;
                default:
                    break;
            }
            break;
        case '<': {
            iStatChat++;
            #ifdef _DBG
                int iret = sprintf(msg, "%s (%s) bad state in case Chat: %d", curUser->Nick, curUser->IP, curUser->iState);
                if(CheckSprintf(iret, 1024, "cDcCommands::PreProcessData54") == true) {
                    Memo(msg);
                }
            #endif
            int imsgLen = sprintf(msg, "[SYS] Bad state (%d) in Chat %s (%s) - user closed.", (int)curUser->ui8State, curUser->sNick, curUser->sIP);
            if(CheckSprintf(imsgLen, 1024, "cDcCommands::PreProcessData55") == true) {
                UdpDebug->Broadcast(msg, imsgLen);
            }
            UserClose(curUser);
            return;
        }
        default:
            break;
    }


    Unknown(curUser, sData, iLen);
}
//---------------------------------------------------------------------------

// $BotINFO pinger identification|
void cDcCommands::BotINFO(User * curUser, char * sData, const uint32_t &iLen) {
	if(((curUser->ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == false || ((curUser->ui32BoolBits & User::BIT_HAVE_BOTINFO) == User::BIT_HAVE_BOTINFO) == true) {
        #ifdef _DBG
            int iret = sprintf(msg, "%s (%s) send $BotINFO and not detected as pinger.", curUser->Nick, curUser->IP);
            if(CheckSprintf(iret, 1024, "cDcCommands::BotINFO1") == true) {
                Memo(msg);
            }
        #endif
        int imsgLen = sprintf(msg, "[SYS] Not pinger $BotINFO or $BotINFO flood from %s (%s)", curUser->sNick, curUser->sIP);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::BotINFO2") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(curUser);
        return;
    }

    if(iLen < 9) {
        int imsgLen = sprintf(msg, "[SYS] Bad $BotINFO (%s) from %s (%s) - user closed.", sData, curUser->sNick, curUser->sIP);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::BotINFO3") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(curUser);
        return;
    }

    curUser->ui32BoolBits |= User::BIT_HAVE_BOTINFO;

	if(ScriptManager->Arrival(curUser, sData, iLen, ScriptMan::BOTINFO_ARRIVAL) == true ||
		curUser->ui8State >= User::STATE_CLOSING) {
		return;
	}

	int imsgLen = sprintf(msg, "$HubINFO %s$%s:%hu$%s.px.$%hd$%" PRIu64 "$%hd$%hd$PtokaX$%s|", SettingManager->sTexts[SETTXT_HUB_NAME], SettingManager->sTexts[SETTXT_HUB_ADDRESS],
        SettingManager->iPortNumbers[0], SettingManager->sTexts[SETTXT_HUB_DESCRIPTION] == NULL ? "" : SettingManager->sTexts[SETTXT_HUB_DESCRIPTION],
        SettingManager->iShorts[SETSHORT_MAX_USERS], SettingManager->ui64MinShare, SettingManager->iShorts[SETSHORT_MIN_SLOTS_LIMIT], SettingManager->iShorts[SETSHORT_MAX_HUBS_LIMIT],
		SettingManager->sTexts[SETTXT_HUB_OWNER_EMAIL] == NULL ? "" : SettingManager->sTexts[SETTXT_HUB_OWNER_EMAIL]);
	if(CheckSprintf(imsgLen, 1024, "cDcCommands::BotINFO4") == true) {
        UserSendCharDelayed(curUser, msg, imsgLen);
    }

	if(((curUser->ui32BoolBits & User::BIT_HAVE_GETNICKLIST) == User::BIT_HAVE_GETNICKLIST) == true) {
		UserClose(curUser);
    }
}
//---------------------------------------------------------------------------

// $ConnectToMe <nickname> <ownip>:<ownlistenport>
// $MultiConnectToMe <nick> <ownip:port> <hub[:port]>
void cDcCommands::ConnectToMe(User * curUser, char * sData, const uint32_t &iLen, const bool &bCheck, const bool &bMulti) {
    if((bMulti == false && iLen < 25) || (bMulti == true && iLen < 30)) {
        int imsgLen = sprintf(msg, "[SYS] Bad $%sConnectToMe (%s) from %s (%s) - user closed.", bMulti == false ? "" : "Multi", sData, curUser->sNick, curUser->sIP);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::ConnectToMe1") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(curUser);
        return;
    }

    // PPK ... check flood ...
    if(bCheck == true && ProfileMan->IsAllowed(curUser, ProfileManager::NODEFLOODCTM) == false) { 
        if(SettingManager->iShorts[SETSHORT_CTM_ACTION] != 0) {
            if(DeFloodCheckForFlood(curUser, DEFLOOD_CTM, SettingManager->iShorts[SETSHORT_CTM_ACTION], 
              curUser->ui16CTMs, curUser->ui64CTMsTick, SettingManager->iShorts[SETSHORT_CTM_MESSAGES], 
              (uint32_t)SettingManager->iShorts[SETSHORT_CTM_TIME]) == true) {
				return;
            }
        }

        if(SettingManager->iShorts[SETSHORT_CTM_ACTION2] != 0) {
            if(DeFloodCheckForFlood(curUser, DEFLOOD_CTM, SettingManager->iShorts[SETSHORT_CTM_ACTION2], 
              curUser->ui16CTMs2, curUser->ui64CTMsTick2, SettingManager->iShorts[SETSHORT_CTM_MESSAGES2], 
			  (uint32_t)SettingManager->iShorts[SETSHORT_CTM_TIME2]) == true) {
                return;
            }
        }
    }

    if(iLen > (uint32_t)SettingManager->iShorts[SETSHORT_MAX_CTM_LEN]) {
        int imsgLen = sprintf(msg, "<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_CTM_TOO_LONG]);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::ConnectToMe2") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }

        if(iLen > 65000) {
            sData[65000] = '\0';
        }

        imsgLen = sprintf(g_sBuffer, "[SYS] Long $ConnectToMe from %s (%s) - user closed. (%s)", curUser->sNick, curUser->sIP, sData);
        if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::ConnectToMe3") == true) {
            UdpDebug->Broadcast(g_sBuffer, imsgLen);
        }

        UserClose(curUser);
		return;
    }

    // PPK ... $CTM means user is active ?!? Probably yes, let set it active and use on another places ;)
    if(curUser->sTag == NULL) {
        curUser->ui32BoolBits |= User::BIT_IPV4_ACTIVE;
    }

    // full data only and allow blocking
	if(ScriptManager->Arrival(curUser, sData, iLen, (uint8_t)(bMulti == false ? ScriptMan::CONNECTTOME_ARRIVAL : ScriptMan::MULTICONNECTTOME_ARRIVAL)) == true ||
		curUser->ui8State >= User::STATE_CLOSING) {
		return;
	}

    char *towho = strchr(sData+(bMulti == false ? 13 : 18), ' ');
    if(towho == NULL) {
        return;
    }

    towho[0] = '\0';

    User * pOtherUser = hashManager->FindUser(sData+(bMulti == false ? 13 : 18), towho-(sData+(bMulti == false ? 13 : 18)));
    // PPK ... no connection to yourself !!!
    if(pOtherUser == NULL || pOtherUser == curUser || pOtherUser->ui8State != User::STATE_ADDED) {
        return;
    }

    towho[0] = ' ';

    // IP check
    if(bCheck == true && SettingManager->bBools[SETBOOL_CHECK_IP_IN_COMMANDS] == true && ProfileMan->IsAllowed(curUser, ProfileManager::NOIPCHECK) == false) {
        if(CheckIP(curUser, towho+1) == false) {
            size_t szPortLen = 0;
            char * sPort = GetPort(towho+1, '|', szPortLen);
            if(sPort != NULL) {
                if((curUser->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6 && (pOtherUser->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
                    int imsgLen = sprintf(g_sBuffer, "$ConnectToMe %s [%s]:%s|", pOtherUser->sNick, curUser->sIP, sPort);
                    if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::ConnectToMe4") == true) {
                        UserAddPrcsdCmd(curUser, PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO, g_sBuffer, imsgLen, pOtherUser);
                    }

                    char * sBadIP = towho+1;
                    if(sBadIP[0] == '[') {
                        sBadIP[strlen(sBadIP)-1] = '\0';
                        sBadIP++;
                    } else if(strchr(sBadIP, '.') == NULL) {
                        *(sPort-1) = ':';
                    }

                    SendIPFixedMsg(curUser, sBadIP, curUser->sIP);
                    return;
                } else if((curUser->ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4 && (pOtherUser->ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4) {
                    char * sIP = curUser->ui8IPv4Len == 0 ? curUser->sIP : curUser->sIPv4;

                    int imsgLen = sprintf(g_sBuffer, "$ConnectToMe %s %s:%s|", pOtherUser->sNick, sIP, sPort);
                    if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::ConnectToMe5") == true) {
                        UserAddPrcsdCmd(curUser, PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO, g_sBuffer, imsgLen, pOtherUser);
                    }

                    char * sBadIP = towho+1;
                    if(sBadIP[0] == '[') {
                        sBadIP[strlen(sBadIP)-1] = '\0';
                        sBadIP++;
                    } else if(strchr(sBadIP, '.') == NULL) {
                        *(sPort-1) = ':';
                    }

                    SendIPFixedMsg(curUser, sBadIP, sIP);
                    return;
                }

                *(sPort-1) = ':';
                *(sPort+szPortLen) = '|';
            }

            SendIncorrectIPMsg(curUser, towho+1, true);

            if(iLen > 65000) {
                sData[65000] = '\0';
            }

            int imsgLen = sprintf(g_sBuffer, "[SYS] Bad IP in %sCTM from %s (%s). (%s)", bMulti == false ? "" : "M", curUser->sNick, curUser->sIP, sData);
            if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::ConnectToMe6") == true) {
                UdpDebug->Broadcast(g_sBuffer, imsgLen);
            }

            UserClose(curUser);
            return;
        }
    }

    if(bMulti == true) {
        sData[5] = '$';
    }
    UserAddPrcsdCmd(curUser, PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO, bMulti == false ? sData : sData+5, bMulti == false ? iLen : iLen-5, pOtherUser);
}
//---------------------------------------------------------------------------

// $GetINFO <nickname> <ownnickname>
void cDcCommands::GetINFO(User * curUser, char * sData, const uint32_t &iLen) {
	if(((curUser->ui32SupportBits & User::SUPPORTBIT_NOGETINFO) == User::SUPPORTBIT_NOGETINFO) == true ||
        ((curUser->ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == true ||
        ((curUser->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == true) {
        int imsgLen = sprintf(msg, "[SYS] Not allowed user %s (%s) send $GetINFO - user closed.", curUser->sNick, curUser->sIP);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::GetINFO1") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(curUser);
        return;
    }
    
    // PPK ... code change, added own nick and space on right place check
    if(iLen < (12u+curUser->ui8NickLen) || iLen > (75u+curUser->ui8NickLen) || sData[iLen-curUser->ui8NickLen-2] != ' ' ||
        memcmp(sData+(iLen-curUser->ui8NickLen-1), curUser->sNick, curUser->ui8NickLen) != 0) {
        if(iLen > 65000) {
            sData[65000] = '\0';
        }

        int imsgLen = sprintf(g_sBuffer, "[SYS] Bad $GetINFO from %s (%s) - user closed. (%s)", curUser->sNick, curUser->sIP, sData);
        if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::GetINFO2") == true) {
            UdpDebug->Broadcast(g_sBuffer, imsgLen);
        }

        UserClose(curUser);
        return;
    }

	if(ScriptManager->Arrival(curUser, sData, iLen, ScriptMan::GETINFO_ARRIVAL) == true ||
		curUser->ui8State >= User::STATE_CLOSING) {
		return;
	}

	// PPK ... for now this is disabled, used by flooders... change later
/*    sData[iLen-curUser->NickLen-2] = '\0';
    sData += 9;
    User *OtherUser = hashManager->FindUser(sData);
    if(OtherUser == NULL) {
        // if the user is not here then send $Quit user| !!!
        // so the client can remove him from nicklist :)
        if(ReservedNicks->IndexOf(sData) == -1) {
            int imsgLen = sprintf(msg, "$Quit %s|", OtherUser->Nick);
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
    }*/
}
//---------------------------------------------------------------------------

// $GetNickList
bool cDcCommands::GetNickList(User * curUser, char * sData, const uint32_t &iLen, const bool &bCheck) {
    if(((curUser->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == true &&
        ((curUser->ui32BoolBits & User::BIT_HAVE_GETNICKLIST) == User::BIT_HAVE_GETNICKLIST) == true) {
        // PPK ... refresh not allowed !
        #ifdef _DBG
           	int iret = sprintf(msg, "%s (%s) bad $GetNickList request.", curUser->Nick, curUser->IP);
           	if(CheckSprintf(iret, 1024, "cDcCommands::GetNickList1") == true) {
               	Memo(msg);
            }
        #endif
        int imsgLen = sprintf(msg, "[SYS] Bad $GetNickList request %s (%s) - user closed.", curUser->sNick, curUser->sIP);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::GetNickList2") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(curUser);
        return false;
    } else if(((curUser->ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == true) {
		if(((curUser->ui32BoolBits & User::BIT_HAVE_GETNICKLIST) == User::BIT_HAVE_GETNICKLIST) == false) {
            curUser->ui32BoolBits |= User::BIT_BIG_SEND_BUFFER;
            if(((curUser->ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == false && colUsers->nickListLen > 11) {
                if(((curUser->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false) {
                    UserSendCharDelayed(curUser, colUsers->nickList, colUsers->nickListLen);
                } else {
                    if(colUsers->iZNickListLen == 0) {
                        colUsers->sZNickList = ZlibUtility->CreateZPipe(colUsers->nickList, colUsers->nickListLen, colUsers->sZNickList,
                            colUsers->iZNickListLen, colUsers->iZNickListSize, Allign16K);
                        if(colUsers->iZNickListLen == 0) {
                            UserSendCharDelayed(curUser, colUsers->nickList, colUsers->nickListLen);
                        } else {
                            UserPutInSendBuf(curUser, colUsers->sZNickList, colUsers->iZNickListLen);
                            ui64BytesSentSaved += colUsers->nickListLen-colUsers->iZNickListLen;
                        }
                    } else {
                        UserPutInSendBuf(curUser, colUsers->sZNickList, colUsers->iZNickListLen);
                        ui64BytesSentSaved += colUsers->nickListLen-colUsers->iZNickListLen;
                    }
                }
            }
            
            if(SettingManager->ui8FullMyINFOOption == 2) {
                if(colUsers->myInfosLen != 0) {
                    if(((curUser->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false) {
                        UserSendCharDelayed(curUser, colUsers->myInfos, colUsers->myInfosLen);
                    } else {
                        if(colUsers->iZMyInfosLen == 0) {
                            colUsers->sZMyInfos = ZlibUtility->CreateZPipe(colUsers->myInfos, colUsers->myInfosLen, colUsers->sZMyInfos,
                                colUsers->iZMyInfosLen, colUsers->iZMyInfosSize, Allign128K);
                            if(colUsers->iZMyInfosLen == 0) {
                                UserSendCharDelayed(curUser, colUsers->myInfos, colUsers->myInfosLen);
                            } else {
                                UserPutInSendBuf(curUser, colUsers->sZMyInfos, colUsers->iZMyInfosLen);
                                ui64BytesSentSaved += colUsers->myInfosLen-colUsers->iZMyInfosLen;
                            }
                        } else {
                            UserPutInSendBuf(curUser, colUsers->sZMyInfos, colUsers->iZMyInfosLen);
                            ui64BytesSentSaved += colUsers->myInfosLen-colUsers->iZMyInfosLen;
                        }
                    }
                }
            } else if(colUsers->myInfosTagLen != 0) {
                if(((curUser->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false) {
                    UserSendCharDelayed(curUser, colUsers->myInfosTag, colUsers->myInfosTagLen);
                } else {
                    if(colUsers->iZMyInfosTagLen == 0) {
                        colUsers->sZMyInfosTag = ZlibUtility->CreateZPipe(colUsers->myInfosTag, colUsers->myInfosTagLen, colUsers->sZMyInfosTag,
                            colUsers->iZMyInfosTagLen, colUsers->iZMyInfosTagSize, Allign128K);
                        if(colUsers->iZMyInfosTagLen == 0) {
                            UserSendCharDelayed(curUser, colUsers->myInfosTag, colUsers->myInfosTagLen);
                        } else {
                            UserPutInSendBuf(curUser, colUsers->sZMyInfosTag, colUsers->iZMyInfosTagLen);
                            ui64BytesSentSaved += colUsers->myInfosTagLen-colUsers->iZMyInfosTagLen;
                        }
                    } else {
                        UserPutInSendBuf(curUser, colUsers->sZMyInfosTag, colUsers->iZMyInfosTagLen);
                        ui64BytesSentSaved += colUsers->myInfosTagLen-colUsers->iZMyInfosTagLen;
                    }  
                }
            }
            
 			if(colUsers->opListLen > 9) {
                if(((curUser->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false) {
                    UserSendCharDelayed(curUser, colUsers->opList, colUsers->opListLen);
                } else {
                    if(colUsers->iZOpListLen == 0) {
                        colUsers->sZOpList = ZlibUtility->CreateZPipe(colUsers->opList, colUsers->opListLen, colUsers->sZOpList,
                            colUsers->iZOpListLen, colUsers->iZOpListSize, Allign16K);
                        if(colUsers->iZOpListLen == 0) {
                            UserSendCharDelayed(curUser, colUsers->opList, colUsers->opListLen);
                        } else {
                            UserPutInSendBuf(curUser, colUsers->sZOpList, colUsers->iZOpListLen);
                            ui64BytesSentSaved += colUsers->opListLen-colUsers->iZOpListLen;
                        }
                    } else {
                        UserPutInSendBuf(curUser, colUsers->sZOpList, colUsers->iZOpListLen);
                        ui64BytesSentSaved += colUsers->opListLen-colUsers->iZOpListLen;
                    }
                }
            }
            
 			if(curUser->sbdatalen != 0) {
                UserTry2Send(curUser);
            }
 			
  			curUser->ui32BoolBits |= User::BIT_HAVE_GETNICKLIST;
  			
   			if(SettingManager->bBools[SETBOOL_REPORT_PINGERS] == true) {
                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                    int imsgLen = sprintf(msg, "%s $<%s> *** %s: %s %s: %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        LanguageManager->sTexts[LAN_PINGER_FROM_IP], curUser->sIP, LanguageManager->sTexts[LAN_WITH_NICK], curUser->sNick, LanguageManager->sTexts[LAN_DETECTED_LWR]);
                    if(CheckSprintf(imsgLen, 1024, "cDcCommands::GetNickList3") == true) {
						g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                    }
                    imsgLen = sprintf(msg, "<%s> *** Pinger from IP: %s with nick: %s detected.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sIP, curUser->sNick);
                    if(CheckSprintf(imsgLen, 1024, "cDcCommands::GetNickList4") == true) {
                        UdpDebug->Broadcast(msg, imsgLen);
                    }
                } else {
                    int imsgLen = sprintf(msg, "<%s> *** %s: %s %s: %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_PINGER_FROM_IP], curUser->sIP,
                        LanguageManager->sTexts[LAN_WITH_NICK], curUser->sNick, LanguageManager->sTexts[LAN_DETECTED_LWR]);
                    if(CheckSprintf(imsgLen, 1024, "cDcCommands::GetNickList5") == true) {
                        UdpDebug->Broadcast(msg, imsgLen);
						g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                    }
                }
			}
			if(((curUser->ui32BoolBits & User::BIT_HAVE_BOTINFO) == User::BIT_HAVE_BOTINFO) == true) {
                UserClose(curUser);
            }
			return false;
		} else {
			int imsgLen = sprintf(msg, "[SYS] $GetNickList flood from pinger %s (%s) - user closed.", curUser->sNick, curUser->sIP);
			if(CheckSprintf(imsgLen, 1024, "cDcCommands::GetNickList6") == true) {
                UdpDebug->Broadcast(msg, imsgLen);
            }
			UserClose(curUser);
			return false;
		}
	}

    curUser->ui32BoolBits |= User::BIT_HAVE_GETNICKLIST;
    
     // PPK ... check flood...
    if(bCheck == true && ProfileMan->IsAllowed(curUser, ProfileManager::NODEFLOODGETNICKLIST) == false && 
      SettingManager->iShorts[SETSHORT_GETNICKLIST_ACTION] != 0) {
        if(DeFloodCheckForFlood(curUser, DEFLOOD_GETNICKLIST, SettingManager->iShorts[SETSHORT_GETNICKLIST_ACTION], 
          curUser->ui16GetNickLists, curUser->ui64GetNickListsTick, SettingManager->iShorts[SETSHORT_GETNICKLIST_MESSAGES], 
          ((uint32_t)SettingManager->iShorts[SETSHORT_GETNICKLIST_TIME])*60) == true) {
            return false;
        }
    }

	if(ScriptManager->Arrival(curUser, sData, iLen, ScriptMan::GETNICKLIST_ARRIVAL) == true ||
		curUser->ui8State >= User::STATE_CLOSING ||
		((curUser->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == true) {
		return false;
	}

	return true;
}
//---------------------------------------------------------------------------

// $Key
void cDcCommands::Key(User * curUser, char * sData, const uint32_t &iLen) {
    if(((curUser->ui32BoolBits & User::BIT_HAVE_KEY) == User::BIT_HAVE_KEY) == true) {
        int imsgLen = sprintf(msg, "[SYS] $Key flood from %s (%s) - user closed.", curUser->sNick, curUser->sIP);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::Key1") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(curUser);
        return;
    }
    
    curUser->ui32BoolBits |= User::BIT_HAVE_KEY;
    
    if(bCmdNoKeyCheck == true) {
        return;
    }
    
    sData[iLen-1] = '\0'; // cutoff pipe

    if(iLen < 6 || strcmp(Lock2Key(curUser->uLogInOut->sLockUsrConn), sData+5) != 0) {
        if(iLen > 65000) {
            sData[65000] = '\0';
        }

        int imsgLen = sprintf(g_sBuffer, "[SYS] Bad $Key from %s (%s) - user closed. (%s)", curUser->sNick, curUser->sIP, sData);
        if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::Key2") == true) {
            UdpDebug->Broadcast(g_sBuffer, imsgLen);
        }

        UserClose(curUser);
        return;
    }
    
    sData[iLen-1] = '|'; // add back pipe

	ScriptManager->Arrival(curUser, sData, iLen, ScriptMan::KEY_ARRIVAL);

	if(curUser->ui8State >= User::STATE_CLOSING) {
		return;
	}

    curUser->ui8State = User::STATE_VALIDATE;
}
//---------------------------------------------------------------------------

// $Kick <name>
void cDcCommands::Kick(User * curUser, char * sData, const uint32_t &iLen) {
    if(ProfileMan->IsAllowed(curUser, ProfileManager::KICK) == false) {
        int iLen = sprintf(msg, "<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_ARE_NOT_ALWD_TO_USE_THIS_CMD]);
        if(CheckSprintf(iLen, 1024, "cDcCommands::Kick1") == true) {
            UserSendCharDelayed(curUser, msg, iLen);
        }
        return;
    } 

    if(iLen < 8) {
        int imsgLen = sprintf(msg, "[SYS] Bad $Kick (%s) from %s (%s) - user closed.", sData, curUser->sNick, curUser->sIP);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::Kick2") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(curUser);
        return;
    }

	if(ScriptManager->Arrival(curUser, sData, iLen, ScriptMan::KICK_ARRIVAL) == true ||
		curUser->ui8State >= User::STATE_CLOSING) {
		return;
	}

    sData[iLen-1] = '\0'; // cutoff pipe

    User *OtherUser = hashManager->FindUser(sData+6, iLen-7);
    if(OtherUser != NULL) {
        // Self-kick
        if(OtherUser == curUser) {
        	int imsgLen = sprintf(msg, "<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_CANT_KICK_YOURSELF]);
        	if(CheckSprintf(imsgLen, 1024, "cDcCommands::Kick3") == true) {
                UserSendCharDelayed(curUser, msg, imsgLen);
            }
            return;
    	}
    	
        if(OtherUser->iProfile != -1 && curUser->iProfile > OtherUser->iProfile) {
        	int imsgLen = sprintf(msg, "<%s> %s %s|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_ARE_NOT_ALLOWED_TO_KICK], OtherUser->sNick);
        	if(CheckSprintf(imsgLen, 1024, "cDcCommands::Kick4") == true) {
                UserSendCharDelayed(curUser, msg, imsgLen);
            }
        	return;
        }

        if(curUser->cmdToUserStrt != NULL) {
            PrcsdToUsrCmd *prev = NULL, 
                *next = curUser->cmdToUserStrt;

            while(next != NULL) {
                PrcsdToUsrCmd *cur = next;
                next = cur->next;
                                       
                if(OtherUser == cur->To) {
                    UserSendChar(cur->To, cur->sCommand, cur->iLen);

                    if(prev == NULL) {
                        if(cur->next == NULL) {
                            curUser->cmdToUserStrt = NULL;
                            curUser->cmdToUserEnd = NULL;
                        } else {
                            curUser->cmdToUserStrt = cur->next;
                        }
                    } else if(cur->next == NULL) {
                        prev->next = NULL;
                        curUser->cmdToUserEnd = prev;
                    } else {
                        prev->next = cur->next;
                    }

#ifdef _WIN32
					if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->sCommand) == 0) {
						AppendDebugLog("%s - [MEM] Cannot deallocate cur->sCommand in cDcCommands::Kick\n", 0);
                    }
#else
					free(cur->sCommand);
#endif
                    cur->sCommand = NULL;

#ifdef _WIN32
                    if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->ToNick) == 0) {
						AppendDebugLog("%s - [MEM] Cannot deallocate cur->ToNick in cDcCommands::Kick\n", 0);
                    }
#else
					free(cur->ToNick);
#endif
                    cur->ToNick = NULL;

					delete cur;
                    break;
                }
                prev = cur;
            }
        }

        char * sBanTime;
		if(OtherUser->uLogInOut != NULL && OtherUser->uLogInOut->sKickMsg != NULL &&
			(sBanTime = stristr(OtherUser->uLogInOut->sKickMsg, "_BAN_")) != NULL) {
			sBanTime[0] = '\0';

			if(sBanTime[5] == '\0' || sBanTime[5] == ' ') { // permban
				hashBanManager->Ban(OtherUser, OtherUser->uLogInOut->sKickMsg, curUser->sNick, false);
    
                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                    int imsgLen = 0;
                    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                        imsgLen = sprintf(msg, "%s $", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
                        CheckSprintf(imsgLen, 1024, "cDcCommands::Kick5");
                    }
                    
                    int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], OtherUser->sNick,
                        LanguageManager->sTexts[LAN_WITH_IP], OtherUser->sIP, LanguageManager->sTexts[LAN_HAS_BEEN], LanguageManager->sTexts[LAN_BANNED_LWR],
                        LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick);
					imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "cDcCommands::Kick6") == true) {
                        if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
        					g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                        } else {
							g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                        }
                    }
                }

                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    int imsgLen = sprintf(msg, "<%s> *** %s %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], OtherUser->sNick, LanguageManager->sTexts[LAN_WITH_IP],
                        OtherUser->sIP, LanguageManager->sTexts[LAN_HAS_BEEN], LanguageManager->sTexts[LAN_BANNED_LWR]);
					if(CheckSprintf(imsgLen, 1024, "cDcCommands::Kick7") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                }

        		// disconnect the user
        		int imsgLen = sprintf(msg, "[SYS] User %s (%s) kicked by %s", OtherUser->sNick, OtherUser->sIP, curUser->sNick);
                	if(CheckSprintf(imsgLen, 1024, "cDcCommands::Kick8") == true) {
					UdpDebug->Broadcast(msg, imsgLen);
				}
				UserClose(OtherUser);

                return;
			} else if(isdigit(sBanTime[5]) != 0) { // tempban
				uint32_t i = 6;
				while(sBanTime[i] != '\0' && isdigit(sBanTime[i]) != 0) {
                	i++;
                }

                char cTime = sBanTime[i];
                sBanTime[i] = '\0';
                int iTime = atoi(sBanTime+5);
				time_t acc_time, ban_time;

				if(cTime != '\0' && iTime > 0 && GenerateTempBanTime(cTime, iTime, acc_time, ban_time) == true) {
					hashBanManager->TempBan(OtherUser, OtherUser->uLogInOut->sKickMsg, curUser->sNick, 0, ban_time, false);

                    static char sTime[256];
                    strcpy(sTime, formatTime((ban_time-acc_time)/60));

					if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                        int imsgLen = 0;
                        if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                            imsgLen = sprintf(msg, "%s $", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
							CheckSprintf(imsgLen, 1024, "cDcCommands::Kick9");
                        }
                        
                        int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s %s %s %s %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], OtherUser->sNick,
                            LanguageManager->sTexts[LAN_WITH_IP], OtherUser->sIP, LanguageManager->sTexts[LAN_HAS_BEEN], LanguageManager->sTexts[LAN_TEMP_BANNED],
                            LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick, LanguageManager->sTexts[LAN_TO_LWR], sTime);
						imsgLen += iret;
                        if(CheckSprintf1(iret, imsgLen, 1024, "cDcCommands::Kick10") == true) {
							if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
            					g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                            } else {
								g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                            }
                        }
                    }
                
                    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                        int imsgLen = sprintf(msg, "<%s> *** %s %s %s %s %s %s: %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], OtherUser->sNick,
                            LanguageManager->sTexts[LAN_WITH_IP], OtherUser->sIP, LanguageManager->sTexts[LAN_HAS_BEEN], LanguageManager->sTexts[LAN_TEMP_BANNED],
                            LanguageManager->sTexts[LAN_TO_LWR], sTime);
                        if(CheckSprintf(imsgLen, 1024, "cDcCommands::Kick11") == true) {
                            UserSendCharDelayed(curUser, msg, imsgLen);
                        }
                	}
    
                    // disconnect the user
                    int imsgLen = sprintf(msg, "[SYS] User %s (%s) kicked by %s", OtherUser->sNick, OtherUser->sIP, curUser->sNick);
                    if(CheckSprintf(imsgLen, 1024, "cDcCommands::Kick12") == true) {
                        UdpDebug->Broadcast(msg, imsgLen);
                    }
					UserClose(OtherUser);

					return;
                }
            }
		}

        hashBanManager->TempBan(OtherUser, OtherUser->uLogInOut != NULL ? OtherUser->uLogInOut->sKickMsg : NULL, curUser->sNick, 0, 0, false);

        if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
            int imsgLen = 0;
            if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                imsgLen = sprintf(msg, "%s $", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
                CheckSprintf(imsgLen, 1024, "cDcCommands::Kick13");
            }

            int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], OtherUser->sNick, LanguageManager->sTexts[LAN_WITH_IP],
                OtherUser->sIP, LanguageManager->sTexts[LAN_WAS_KICKED_BY], curUser->sNick);
            imsgLen += iret;
            if(CheckSprintf1(iret, imsgLen, 1024, "cDcCommands::Kick14") == true) {
                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
    				g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                } else {
					g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                }
            }
        }

        if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
            int imsgLen = sprintf(msg, "<%s> *** %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], OtherUser->sNick, LanguageManager->sTexts[LAN_WITH_IP], OtherUser->sIP,
                LanguageManager->sTexts[LAN_WAS_KICKED]);
            if(CheckSprintf(imsgLen, 1024, "cDcCommands::Kick15") == true) {
                UserSendCharDelayed(curUser, msg, imsgLen);
            }
        }

        // disconnect the user
        int imsgLen = sprintf(msg, "[SYS] User %s (%s) kicked by %s", OtherUser->sNick, OtherUser->sIP, curUser->sNick);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::Kick16") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(OtherUser);
    }
}
//---------------------------------------------------------------------------

// $Search $MultiSearch
bool cDcCommands::SearchDeflood(User *curUser, char * sData, const uint32_t &iLen, const bool &bCheck, const bool &bMulti) {
    // search flood protection ... modified by PPK ;-)
    if(bCheck == true && ProfileMan->IsAllowed(curUser, ProfileManager::NODEFLOODSEARCH) == false) {
        if(SettingManager->iShorts[SETSHORT_SEARCH_ACTION] != 0) {  
            if(DeFloodCheckForFlood(curUser, DEFLOOD_SEARCH, SettingManager->iShorts[SETSHORT_SEARCH_ACTION], 
              curUser->ui16Searchs, curUser->ui64SearchsTick, SettingManager->iShorts[SETSHORT_SEARCH_MESSAGES], 
              (uint32_t)SettingManager->iShorts[SETSHORT_SEARCH_TIME]) == true) {
                return false;
            }
        }

        if(SettingManager->iShorts[SETSHORT_SEARCH_ACTION2] != 0) {  
            if(DeFloodCheckForFlood(curUser, DEFLOOD_SEARCH, SettingManager->iShorts[SETSHORT_SEARCH_ACTION2], 
              curUser->ui16Searchs2, curUser->ui64SearchsTick2, SettingManager->iShorts[SETSHORT_SEARCH_MESSAGES2], 
              (uint32_t)SettingManager->iShorts[SETSHORT_SEARCH_TIME2]) == true) {
                return false;
            }
        }

        // 2nd check for same search flood
		if(SettingManager->iShorts[SETSHORT_SAME_SEARCH_ACTION] != 0) {
			bool bNewData = false;
            if(DeFloodCheckForSameFlood(curUser, DEFLOOD_SAME_SEARCH, SettingManager->iShorts[SETSHORT_SAME_SEARCH_ACTION], 
              curUser->ui16SameSearchs, curUser->ui64SameSearchsTick, 
              SettingManager->iShorts[SETSHORT_SAME_SEARCH_MESSAGES], SettingManager->iShorts[SETSHORT_SAME_SEARCH_TIME], 
			  sData+(bMulti == true ? 13 : 8), iLen-(bMulti == true ? 13 : 8),
			  curUser->sLastSearch, curUser->ui16LastSearchLen, bNewData) == true) {
                return false;
            }

			if(bNewData == true) {
				UserSetLastSearch(curUser, sData+(bMulti == true ? 13 : 8), iLen-(bMulti == true ? 13 : 8));
			}
        }
    }
    
    return true;
}
//---------------------------------------------------------------------------

// $Search $MultiSearch
void cDcCommands::Search(User *curUser, char * sData, uint32_t iLen, const bool &bCheck, const bool &bMulti) {
    uint32_t iAfterCmd;
    if(bMulti == false) {
        if(iLen < 10) {
            int imsgLen = sprintf(msg, "[SYS] Bad $Search (%s) from %s (%s) - user closed.", sData, curUser->sNick, curUser->sIP);
            if(CheckSprintf(imsgLen, 1024, "cDcCommands::Search1") == true) {
                UdpDebug->Broadcast(msg, imsgLen);
            }
            UserClose(curUser);
			return;
        }
        iAfterCmd = 8;
    } else {
        if(iLen < 15) {
            int imsgLen = sprintf(msg, "[SYS] Bad $MultiSearch (%s) from %s (%s) - user closed.", sData, curUser->sNick, curUser->sIP);
            if(CheckSprintf(imsgLen, 1024, "cDcCommands::Search2") == true) {
                UdpDebug->Broadcast(msg, imsgLen);
            }
            UserClose(curUser);
            return;
        }
        iAfterCmd = 13;
    }

    if(bCheck == true && ProfileMan->IsAllowed(curUser, ProfileManager::NOSEARCHINTERVAL) == false) {
        if(DeFloodCheckInterval(curUser, INTERVAL_SEARCH, curUser->ui16SearchsInt, 
            curUser->ui64SearchsIntTick, SettingManager->iShorts[SETSHORT_SEARCH_INTERVAL_MESSAGES], 
            (uint32_t)SettingManager->iShorts[SETSHORT_SEARCH_INTERVAL_TIME]) == true) {
            return;
        }
    }

	if(ScriptManager->Arrival(curUser, sData, iLen, ScriptMan::SEARCH_ARRIVAL) == true ||
		curUser->ui8State >= User::STATE_CLOSING) {
		return;
	}

    // send search from actives to all, from passives to actives only
    // PPK ... optimization ;o)
    if(bMulti == false && *((uint32_t *)(sData+iAfterCmd)) == *((uint32_t *)"Hub:")) {
        if(curUser->sTag == NULL) {
            curUser->ui32BoolBits &= ~User::BIT_IPV4_ACTIVE;
        }

        // PPK ... check nick !!!
        if((sData[iAfterCmd+4+curUser->ui8NickLen] != ' ') || (memcmp(sData+iAfterCmd+4, curUser->sNick, curUser->ui8NickLen) != 0)) {
            if(iLen > 65000) {
                sData[65000] = '\0';
            }

            int imsgLen = sprintf(g_sBuffer, "[SYS] Nick spoofing in search from %s (%s) - user closed. (%s)", curUser->sNick, curUser->sIP, sData);
            if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::Search3") == true) {
                UdpDebug->Broadcast(g_sBuffer, imsgLen);
            }

            UserClose(curUser);
            return;
        }

        if(bCheck == true && ProfileMan->IsAllowed(curUser, ProfileManager::NOSEARCHLIMITS) == false &&
            (SettingManager->iShorts[SETSHORT_MIN_SEARCH_LEN] != 0 || SettingManager->iShorts[SETSHORT_MAX_SEARCH_LEN] != 0)) {
            // PPK ... search string len check
            // $Search Hub:PPK F?T?0?2?test|
            uint32_t iChar = iAfterCmd+8+curUser->ui8NickLen+1;
            uint32_t iCount = 0;
            for(; iChar < iLen; iChar++) {
                if(sData[iChar] == '?') {
                    iCount++;
                    if(iCount == 2)
                        break;
                }
            }
            iCount = iLen-2-iChar;

            if(iCount < (uint32_t)SettingManager->iShorts[SETSHORT_MIN_SEARCH_LEN]) {
                int imsgLen = sprintf(msg, "<%s> %s %hd.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SORRY_MIN_SEARCH_LEN_IS],
                    SettingManager->iShorts[SETSHORT_MIN_SEARCH_LEN]);
                if(CheckSprintf(imsgLen, 1024, "cDcCommands::Search5") == true) {
                    UserSendCharDelayed(curUser, msg, imsgLen);
                }
                return;
            }
            if(SettingManager->iShorts[SETSHORT_MAX_SEARCH_LEN] != 0 && iCount > (uint32_t)SettingManager->iShorts[SETSHORT_MAX_SEARCH_LEN]) {
                int imsgLen = sprintf(msg, "<%s> %s %hd.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SORRY_MAX_SEARCH_LEN_IS],
                    SettingManager->iShorts[SETSHORT_MAX_SEARCH_LEN]);
                if(CheckSprintf(imsgLen, 1024, "cDcCommands::Search6") == true) {
                    UserSendCharDelayed(curUser, msg, imsgLen);
                }
                return;
            }
        }

        curUser->iSR = 0;

        curUser->cmdPassiveSearch = AddSearch(curUser, curUser->cmdPassiveSearch, sData, iLen, false);
    } else {
        if(curUser->sTag == NULL) {
            curUser->ui32BoolBits |= User::BIT_IPV4_ACTIVE;
        }

        if(bCheck == true && ProfileMan->IsAllowed(curUser, ProfileManager::NOSEARCHLIMITS) == false &&
            (SettingManager->iShorts[SETSHORT_MIN_SEARCH_LEN] != 0 || SettingManager->iShorts[SETSHORT_MAX_SEARCH_LEN] != 0)) {
            // PPK ... search string len check
            // $Search 1.2.3.4:1 F?F?0?2?test|
            uint32_t iChar = iAfterCmd+11;
            uint32_t iCount = 0;

            for(; iChar < iLen; iChar++) {
                if(sData[iChar] == '?') {
                    iCount++;
                    if(iCount == 4)
                        break;
                }
            }

            iCount = iLen-2-iChar;

            if(iCount < (uint32_t)SettingManager->iShorts[SETSHORT_MIN_SEARCH_LEN]) {
                int imsgLen = sprintf(msg, "<%s> %s %hd.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SORRY_MIN_SEARCH_LEN_IS],
                    SettingManager->iShorts[SETSHORT_MIN_SEARCH_LEN]);
                if(CheckSprintf(imsgLen, 1024, "cDcCommands::Search10") == true) {
                    UserSendCharDelayed(curUser, msg, imsgLen);
                }
                return;
            }
            if(SettingManager->iShorts[SETSHORT_MAX_SEARCH_LEN] != 0 && iCount > (uint32_t)SettingManager->iShorts[SETSHORT_MAX_SEARCH_LEN]) {
                int imsgLen = sprintf(msg, "<%s> %s %hd.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SORRY_MAX_SEARCH_LEN_IS],
                    SettingManager->iShorts[SETSHORT_MAX_SEARCH_LEN]);
                if(CheckSprintf(imsgLen, 1024, "cDcCommands::Search11") == true) {
                    UserSendCharDelayed(curUser, msg, imsgLen);
                }
                return;
            }
        }

        // IP check
        if(bCheck == true && SettingManager->bBools[SETBOOL_CHECK_IP_IN_COMMANDS] == true && ProfileMan->IsAllowed(curUser, ProfileManager::NOIPCHECK) == false) {
            if(CheckIP(curUser, sData+iAfterCmd) == false) {
                size_t szPortLen = 0;
                char * sPort = GetPort(sData+iAfterCmd, ' ', szPortLen);
                if(sPort != NULL) {
                    if((curUser->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
                        if((curUser->ui32BoolBits & User::BIT_IPV6_ACTIVE) == User::BIT_IPV6_ACTIVE) {
                            int imsgLen = sprintf(g_sBuffer, "$Search [%s]:%s %s", curUser->sIP, sPort, sPort+szPortLen+1);
                            if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::Search12") == true) {
							 curUser->cmdActive6Search = AddSearch(curUser, curUser->cmdActive6Search, g_sBuffer, imsgLen, true);
                            }
                        } else {
                            int imsgLen = sprintf(g_sBuffer, "$Search Hub:%s %s", curUser->sNick, sPort+szPortLen+1);
                            if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::Search12-1") == true) {
                                curUser->cmdPassiveSearch = AddSearch(curUser, curUser->cmdPassiveSearch, g_sBuffer, imsgLen, false);
                            }
                        }

						if((curUser->ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4) {
                            if((curUser->ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE) {
                                int imsgLen = sprintf(g_sBuffer, "$Search %s:%s %s", curUser->sIPv4, sPort, sPort+szPortLen+1);
                                if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::Search12-2") == true) {
                                    curUser->cmdActive4Search = AddSearch(curUser, curUser->cmdActive4Search, g_sBuffer, imsgLen, true);
                                }
                            } else {
                                int imsgLen = sprintf(g_sBuffer, "$Search Hub:%s %s", curUser->sNick, sPort+szPortLen+1);
                                if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::Search12-3") == true) {
                                    curUser->cmdPassiveSearch = AddSearch(curUser, curUser->cmdPassiveSearch, g_sBuffer, imsgLen, false);
                                }
                            }
						}

                        char * sBadIP = sData+iAfterCmd;
                        if(sBadIP[0] == '[') {
                            sBadIP[strlen(sBadIP)-1] = '\0';
                            sBadIP++;
                        } else if(strchr(sBadIP, '.') == NULL) {
                            *(sPort-1) = ':';
                        }

                        SendIPFixedMsg(curUser, sBadIP, curUser->sIP);
                        return;
                    } else if((curUser->ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4) {
                        char * sIP = curUser->ui8IPv4Len == 0 ? curUser->sIP : curUser->sIPv4;

                        int imsgLen = sprintf(g_sBuffer, "$Search %s:%s %s", sIP, sPort, sPort+szPortLen+1);
                        if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::Search13") == true) {
							curUser->cmdActive4Search = AddSearch(curUser, curUser->cmdActive4Search, g_sBuffer, imsgLen, true);
                        }

                        char * sBadIP = sData+iAfterCmd;
                        if(sBadIP[0] == '[') {
                            sBadIP[strlen(sBadIP)-1] = '\0';
                            sBadIP++;
                        } else if(strchr(sBadIP, '.') == NULL) {
                            *(sPort-1) = ':';
                        }

                        SendIPFixedMsg(curUser, sBadIP, sIP);
                        return;
                    }

                    *(sPort-1) = ':';
                    *(sPort+szPortLen) = ' ';
                }

                SendIncorrectIPMsg(curUser, sData+iAfterCmd, false);

                if(iLen > 65000) {
                    sData[65000] = '\0';
                }

                int imsgLen = sprintf(g_sBuffer, "[SYS] Bad IP in Search from %s (%s). (%s)", curUser->sNick, curUser->sIP, sData);
                if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::Search14") == true) {
                    UdpDebug->Broadcast(g_sBuffer, imsgLen);
                }

                UserClose(curUser);
                return;
            }
        }

        if(bMulti == true) {
            sData[5] = '$';
            sData += 5;
            iLen -= 5;
        }

		if((curUser->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
			if(sData[8] == '[') {
				curUser->cmdActive6Search = AddSearch(curUser, curUser->cmdActive6Search, sData, iLen, true);

				if((curUser->ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4) {
					size_t szPortLen = 0;
					char * sPort = GetPort(sData+8, ' ', szPortLen);
					if(sPort != NULL) {
                        if((curUser->ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE) {
                            int imsgLen = sprintf(g_sBuffer, "$Search %s:%s %s", curUser->sIPv4, sPort, sPort+szPortLen+1);
                            if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::Search15") == true) {
                                curUser->cmdActive4Search = AddSearch(curUser, curUser->cmdActive4Search, g_sBuffer, imsgLen, true);
                            }
						} else {
                            int imsgLen = sprintf(g_sBuffer, "$Search Hub:%s %s", curUser->sNick, sPort+szPortLen+1);
                            if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::Search16") == true) {
                                curUser->cmdPassiveSearch = AddSearch(curUser, curUser->cmdPassiveSearch, g_sBuffer, imsgLen, false);
                            }
                        }
					}
				}
			} else {
                curUser->cmdActive4Search = AddSearch(curUser, curUser->cmdActive4Search, sData, iLen, true);

                if(((curUser->ui32BoolBits & User::BIT_IPV6_ACTIVE) == User::BIT_IPV6_ACTIVE) == false) {
					size_t szPortLen = 0;
					char * sPort = GetPort(sData+8, ' ', szPortLen);
					if(sPort != NULL) {
                        int imsgLen = sprintf(g_sBuffer, "$Search Hub:%s %s", curUser->sNick, sPort+szPortLen+1);
                        if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::Search17") == true) {
                            curUser->cmdPassiveSearch = AddSearch(curUser, curUser->cmdPassiveSearch, g_sBuffer, imsgLen, false);
                        }
                    }
                }
			}
		} else {
			curUser->cmdActive4Search = AddSearch(curUser, curUser->cmdActive4Search, sData, iLen, true);
		}
    }
}
//---------------------------------------------------------------------------

// $MyINFO $ALL  $ $$$$|
bool cDcCommands::MyINFODeflood(User * curUser, char * sData, const uint32_t &iLen, const bool &bCheck) {
    if(iLen < (22u+curUser->ui8NickLen)) {
        int imsgLen = sprintf(msg, "[SYS] Bad $MyINFO (%s) from %s (%s) - user closed.", sData, curUser->sNick, curUser->sIP);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::MyINFODeflood1") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(curUser);
        return false;
    }

    // PPK ... check flood ...
    if(bCheck == true && ProfileMan->IsAllowed(curUser, ProfileManager::NODEFLOODMYINFO) == false) { 
        if(SettingManager->iShorts[SETSHORT_MYINFO_ACTION] != 0) {
            if(DeFloodCheckForFlood(curUser, DEFLOOD_MYINFO, SettingManager->iShorts[SETSHORT_MYINFO_ACTION], 
              curUser->ui16MyINFOs, curUser->ui64MyINFOsTick, SettingManager->iShorts[SETSHORT_MYINFO_MESSAGES], 
              (uint32_t)SettingManager->iShorts[SETSHORT_MYINFO_TIME]) == true) {
                return false;
            }
        }

        if(SettingManager->iShorts[SETSHORT_MYINFO_ACTION2] != 0) {
            if(DeFloodCheckForFlood(curUser, DEFLOOD_MYINFO, SettingManager->iShorts[SETSHORT_MYINFO_ACTION2], 
              curUser->ui16MyINFOs2, curUser->ui64MyINFOsTick2, SettingManager->iShorts[SETSHORT_MYINFO_MESSAGES2], 
              (uint32_t)SettingManager->iShorts[SETSHORT_MYINFO_TIME2]) == true) {
                return false;
            }
        }
    }

    if(iLen > (uint32_t)SettingManager->iShorts[SETSHORT_MAX_MYINFO_LEN]) {
        int imsgLen = sprintf(msg, "<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_MYINFO_TOO_LONG]);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::MyINFODeflood2") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }

        if(iLen > 65000) {
            sData[65000] = '\0';
        }

        imsgLen = sprintf(g_sBuffer, "[SYS] Bad $MyINFO from %s (%s) - user closed. (%s)", curUser->sNick, curUser->sIP, sData);
        if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::MyINFODeflood3") == true) {
            UdpDebug->Broadcast(g_sBuffer, imsgLen);
        }

        UserClose(curUser);
		return false;
    }
    
    return true;
}
//---------------------------------------------------------------------------

// $MyINFO $ALL  $ $$$$|
bool cDcCommands::MyINFO(User * curUser, char * sData, const uint32_t &iLen) {
    // if no change, just return
    // else store MyINFO and perform all checks again
    if(curUser->sMyInfoOriginal != NULL) { // PPK ... optimizations
       	if(iLen == curUser->ui16MyInfoOriginalLen && memcmp(curUser->sMyInfoOriginal+14+curUser->ui8NickLen, sData+14+curUser->ui8NickLen, curUser->ui16MyInfoOriginalLen-14-curUser->ui8NickLen) == 0) {
           return false;
        }
    }

    UserSetMyInfoOriginal(curUser, sData, (uint16_t)iLen);

    if(curUser->ui8State >= User::STATE_CLOSING) {
        return false;
    }

    if(UserProcessRules(curUser) == false) {
        UserClose(curUser);
        return false;
    }
    
    // TODO 3 -oPTA -ccheckers: Slots fetching for no tag users
    //search command for slots fetch for users without tag
    //if(curUser->Tag == NULL)
    //{
    //    curUser->SendText("$Search "+HubAddress->Text+":411 F?F?0?1?.|");
    //}

    // SEND myinfo to others (including me) only if this is
    // a refresh MyINFO event. Else dispatch it in addMe condition
    // of service loop

    // PPK ... moved lua here -> another "optimization" ;o)
	ScriptManager->Arrival(curUser, sData, iLen, ScriptMan::MYINFO_ARRIVAL);

	if(curUser->ui8State >= User::STATE_CLOSING) {
		return false;
	}
    
    return true;
}
//---------------------------------------------------------------------------

// $MyPass
void cDcCommands::MyPass(User * curUser, char * sData, const uint32_t &iLen) {
    if(curUser->uLogInOut->sPassword == NULL) {
        // no password required
        int imsgLen = sprintf(msg, "[SYS] $MyPass without request from %s (%s) - user closed.", curUser->sNick, curUser->sIP);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::MyPass1") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(curUser);
        return;
    }

    if(iLen < 10|| iLen > 73) {
        if(iLen > 65000) {
            sData[65000] = '\0';
        }

        int imsgLen = sprintf(g_sBuffer, "[SYS] Bad $MyPass from %s (%s) - user closed. (%s)", curUser->sNick, curUser->sIP, sData);
        if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::MyPass2") == true) {
            UdpDebug->Broadcast(g_sBuffer, imsgLen);
        }

        UserClose(curUser);
        return;
    }

    sData[iLen-1] = '\0'; // cutoff pipe

    // if password is wrong, close the connection
    if(strcmp(curUser->uLogInOut->sPassword, sData+8) != 0) {
        if(SettingManager->bBools[SETBOOL_ADVANCED_PASS_PROTECTION] == true) {
			RegUser *Reg = hashRegManager->Find(curUser);
            if(Reg != NULL) {
                time(&Reg->tLastBadPass);
                if(Reg->iBadPassCount < 255)
                    Reg->iBadPassCount++;
            }
        }
    
        if(SettingManager->iShorts[SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE] != 0) {
            // brute force password protection
			PassBf * PassBfItem = Find(curUser->ui128IpHash);
            if(PassBfItem == NULL) {
                PassBfItem = new PassBf(curUser->ui128IpHash);
                if(PassBfItem == NULL) {
					AppendDebugLog("%s - [MEM] Cannot allocate new PassBfItem in cDcCommands::MyPass\n", 0);
                	return;
                }

                if(PasswdBfCheck != NULL) {
                    PasswdBfCheck->prev = PassBfItem;
                    PassBfItem->next = PasswdBfCheck;
                    PasswdBfCheck = PassBfItem;
                }
                PasswdBfCheck = PassBfItem;
            } else {
                if(PassBfItem->iCount == 2) {
                    BanItem *Ban = hashBanManager->FindFull(curUser->ui128IpHash);
                    if(Ban == NULL || ((Ban->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == false) {
                        int iret = sprintf(msg, "3x bad password for nick %s", curUser->sNick);
                        if(CheckSprintf(iret, 1024, "cDcCommands::MyPass4") == false) {
                            msg[0] = '\0';
                        }
                        if(SettingManager->iShorts[SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE] == 1) {
                            hashBanManager->BanIp(curUser, NULL, msg, NULL, true);
                        } else {
                            hashBanManager->TempBanIp(curUser, NULL, msg, NULL, SettingManager->iShorts[SETSHORT_BRUTE_FORCE_PASS_PROTECT_TEMP_BAN_TIME]*60, 0, true);
                        }
                        Remove(PassBfItem);
                        int imsgLen = sprintf(msg, "<%s> %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOUR_IP_BANNED_BRUTE_FORCE_ATTACK]);
                        if(CheckSprintf(imsgLen, 1024, "cDcCommands::MyPass5") == true) {
                            UserSendChar(curUser, msg, imsgLen);
                        }
                    } else {
                        int imsgLen = sprintf(msg, "<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOUR_IS_BANNED]);
                        if(CheckSprintf(imsgLen, 1024, "cDcCommands::MyPass6") == true) {
                            UserSendChar(curUser, msg, imsgLen);
                        }
                    }
                    if(SettingManager->bBools[SETBOOL_REPORT_3X_BAD_PASS] == true) {
                        if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                            int imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s %s|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                                LanguageManager->sTexts[LAN_IP], curUser->sIP, LanguageManager->sTexts[LAN_BANNED_BECAUSE_3X_BAD_PASS_FOR_NICK], curUser->sNick);
                            if(CheckSprintf(imsgLen, 1024, "cDcCommands::MyPass7") == true) {
								g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                            }
                        } else {
                            int imsgLen = sprintf(msg, "<%s> *** %s %s %s %s|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_IP], curUser->sIP,
                                LanguageManager->sTexts[LAN_BANNED_BECAUSE_3X_BAD_PASS_FOR_NICK], curUser->sNick);
                            if(CheckSprintf(imsgLen, 1024, "cDcCommands::MyPass8") == true) {
								g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                            }
                        }
                    }

                    if(iLen > 65000) {
                        sData[65000] = '\0';
                    }

                    int imsgLen = sprintf(g_sBuffer, "[SYS] Bad 3x password from %s (%s) - user banned. (%s)", curUser->sNick, curUser->sIP, sData);
                    if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::MyPass10") == true) {
                        UdpDebug->Broadcast(g_sBuffer, imsgLen);
                    }

                    UserClose(curUser);
                    return;
                } else {
                    PassBfItem->iCount++;
                }
            }
        }

        int imsgLen = sprintf(msg, "$BadPass|<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_INCORRECT_PASSWORD]);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::MyPass12") == true) {
            UserSendChar(curUser, msg, imsgLen);
        }

        if(iLen > 65000) {
            sData[65000] = '\0';
        }

        imsgLen = sprintf(g_sBuffer, "[SYS] Bad password from %s (%s) - user closed. (%s)", curUser->sNick, curUser->sIP, sData);
        if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::MyPass13") == true) {
            UdpDebug->Broadcast(g_sBuffer, imsgLen);
        }

        UserClose(curUser);
        return;
    } else {
		RegUser *Reg = hashRegManager->Find(curUser);
        if(Reg != NULL) {
            Reg->iBadPassCount = 0;
        }
        PassBf * PassBfItem = Find(curUser->ui128IpHash);
        if(PassBfItem != NULL) {
            Remove(PassBfItem);
        }

        // PPK ... memory clean up, is not needed anymore
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)curUser->uLogInOut->sPassword) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate curUser->uLogInOut->Password in cDcCommands::MyPass\n", 0);
        }
#else
		free(curUser->uLogInOut->sPassword);
#endif
        curUser->uLogInOut->sPassword = NULL;

        sData[iLen-1] = '|'; // add back pipe

        // PPK ... Lua DataArrival only if pass is ok
		ScriptManager->Arrival(curUser, sData, iLen, ScriptMan::PASSWORD_ARRIVAL);

		if(curUser->ui8State >= User::STATE_CLOSING) {
    		return;
    	}

        if(ProfileMan->IsAllowed(curUser, ProfileManager::HASKEYICON) == true) {
            curUser->ui32BoolBits |= User::BIT_OPERATOR;
        } else {
            curUser->ui32BoolBits &= ~User::BIT_OPERATOR;
        }
        
        // PPK ... addition for registered users, kill your own ghost >:-]
        if(((curUser->ui32BoolBits & User::BIT_HASHED) == User::BIT_HASHED) == false) {
            User *OtherUser = hashManager->FindUser(curUser->sNick, curUser->ui8NickLen);
            if(OtherUser != NULL) {
                int imsgLen = sprintf(msg, "[SYS] Ghost %s (%s) closed.", OtherUser->sNick, OtherUser->sIP);
                if(CheckSprintf(imsgLen, 1024, "cDcCommands::MyPass12") == true) {
                    UdpDebug->Broadcast(msg, imsgLen);
                }
                if(((curUser->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == false) {
           	        UserClose(OtherUser);
                } else {
                    UserClose(OtherUser, true);
                }
            }
            if(hashManager->Add(curUser) == false) {
                return;
            }
            curUser->ui32BoolBits |= User::BIT_HASHED;
        }
        if(((curUser->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == false) {
            // welcome the new user
            // PPK ... fixed bad DC protocol implementation, $LogedIn is only for OPs !!!
            // registered DC1 users have enabled OP menu :)))))))))
            int imsgLen;
            if(((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                imsgLen = sprintf(msg, "$Hello %s|$LogedIn %s|", curUser->sNick, curUser->sNick);
                if(CheckSprintf(imsgLen, 1024, "cDcCommands::MyPass15") == false) {
                    return;
                }
            } else {
                imsgLen = sprintf(msg, "$Hello %s|", curUser->sNick);
                if(CheckSprintf(imsgLen, 1024, "cDcCommands::MyPass16") == false) {
                    return;
                }
            }
            UserSendCharDelayed(curUser, msg, imsgLen);
            return;
        } else {
            if(((curUser->ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == false) {
                UserAddMeOrIPv4Check(curUser);
            }
        }
    }     
}
//---------------------------------------------------------------------------

// $OpForceMove $Who:<nickname>$Where:<iptoredirect>$Msg:<a message>
void cDcCommands::OpForceMove(User * curUser, char * sData, const uint32_t &iLen) {
    if(ProfileMan->IsAllowed(curUser, ProfileManager::REDIRECT) == false) {
        int iLen = sprintf(msg, "<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_ARE_NOT_ALWD_TO_USE_THIS_CMD]);
        if(CheckSprintf(iLen, 1024, "cDcCommands::OpForceMove1") == true) {
            UserSendCharDelayed(curUser, msg, iLen);
        }
        return;
    }

    if(iLen < 31) {
        int imsgLen = sprintf(msg, "[SYS] Bad $OpForceMove (%s) from %s (%s) - user closed.", sData, curUser->sNick, curUser->sIP);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::OpForceMove2") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(curUser);
        return;
    }

	if(ScriptManager->Arrival(curUser, sData, iLen, ScriptMan::OPFORCEMOVE_ARRIVAL) == true ||
		curUser->ui8State >= User::STATE_CLOSING) {
		return;
	}

    sData[iLen-1] = '\0'; // cutoff pipe

    char *sCmdParts[] = { NULL, NULL, NULL };
    uint16_t iCmdPartsLen[] = { 0, 0, 0 };
                
    uint8_t cPart = 0;
                
    sCmdParts[cPart] = sData+18; // nick start
                
    for(uint32_t ui32i = 18; ui32i < iLen; ui32i++) {
        if(sData[ui32i] == '$') {
            sData[ui32i] = '\0';
            iCmdPartsLen[cPart] = (uint16_t)((sData+ui32i)-sCmdParts[cPart]);
                    
            // are we on last $ ???
            if(cPart == 1) {
                sCmdParts[2] = sData+ui32i+1;
                iCmdPartsLen[2] = (uint16_t)(iLen-ui32i-1);
                break;
            }
                    
            cPart++;
            sCmdParts[cPart] = sData+ui32i+1;
        }
    }

    if(iCmdPartsLen[0] == 0 || iCmdPartsLen[1] < 7 || iCmdPartsLen[2] < 5 || iCmdPartsLen[1] > 4096 || iCmdPartsLen[2] > 16384) {
        return;
    }

    User *OtherUser = hashManager->FindUser(sCmdParts[0], iCmdPartsLen[0]);
    if(OtherUser) {
   	    if(OtherUser->iProfile != -1 && curUser->iProfile > OtherUser->iProfile) {
            int imsgLen = sprintf(msg, "<%s> %s %s|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_ARE_NOT_ALLOWED_TO_REDIRECT], OtherUser->sNick);
            if(CheckSprintf(imsgLen, 1024, "cDcCommands::OpForceMove3") == true) {
                UserSendCharDelayed(curUser, msg, imsgLen);
            }
            return;
        } else {
            int imsgLen = sprintf(g_sBuffer, "<%s> %s %s %s %s. %s: %s|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_ARE_REDIRECTED_TO],
                sCmdParts[1]+6, LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick, LanguageManager->sTexts[LAN_MESSAGE], sCmdParts[2]+4);
            if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::OpForceMove4") == true) {
                UserSendCharDelayed(OtherUser, g_sBuffer, imsgLen);
            }

            imsgLen = sprintf(g_sBuffer, "$ForceMove %s|", sCmdParts[1]+6);
            if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::OpForceMove6") == true) {
                UserSendChar(OtherUser, g_sBuffer, imsgLen);
            }

            // PPK ... close user !!!
            imsgLen = sprintf(msg, "[SYS] User %s (%s) redirected by %s", OtherUser->sNick, OtherUser->sIP, curUser->sNick);
            if(CheckSprintf(imsgLen, 1024, "cDcCommands::OpForceMove13") == true) {
                UdpDebug->Broadcast(msg, imsgLen);
            }
            UserClose(OtherUser);

            if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                    imsgLen = sprintf(g_sBuffer, "%s $<%s> *** %s %s %s %s %s. %s: %s|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                        SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], OtherUser->sNick, LanguageManager->sTexts[LAN_IS_REDIRECTED_TO], sCmdParts[1]+6,
                        LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick, LanguageManager->sTexts[LAN_MESSAGE], sCmdParts[2]+4);
                    if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::OpForceMove7") == true) {
						g_GlobalDataQueue->SingleItemStore(g_sBuffer, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                    }
                } else {
                    imsgLen = sprintf(g_sBuffer, "<%s> *** %s %s %s %s %s. %s: %s|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], OtherUser->sNick,
                        LanguageManager->sTexts[LAN_IS_REDIRECTED_TO], sCmdParts[1]+6, LanguageManager->sTexts[LAN_BY_LWR], curUser->sNick, LanguageManager->sTexts[LAN_MESSAGE], sCmdParts[2]+4);
                    if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::OpForceMove9") == true) {
						g_GlobalDataQueue->AddQueueItem(g_sBuffer, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                    }
                }
            }

            if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                imsgLen = sprintf(g_sBuffer, "<%s> *** %s %s %s. %s: %s|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], OtherUser->sNick, LanguageManager->sTexts[LAN_IS_REDIRECTED_TO],
                    sCmdParts[1]+6, LanguageManager->sTexts[LAN_MESSAGE], sCmdParts[2]+4);
                if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::OpForceMove11") == true) {
                    UserSendCharDelayed(curUser, g_sBuffer, imsgLen);
                }
            }
        }
    }
}
//---------------------------------------------------------------------------

// $RevConnectToMe <ownnick> <nickname>
void cDcCommands::RevConnectToMe(User * curUser, char * sData, const uint32_t &iLen, const bool &bCheck) {
    if(iLen < 19) {
        int imsgLen = sprintf(msg, "[SYS] Bad $RevConnectToMe (%s) from %s (%s) - user closed.", sData, curUser->sNick, curUser->sIP);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::RevConnectToMe1") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(curUser);
        return;
    }

    // PPK ... optimizations
    if((sData[16+curUser->ui8NickLen] != ' ') || (memcmp(sData+16, curUser->sNick, curUser->ui8NickLen) != 0)) {
        if(iLen > 65000) {
            sData[65000] = '\0';
        }

        int imsgLen = sprintf(g_sBuffer, "[SYS] Nick spoofing in RCTM from %s (%s) - user closed. (%s)", curUser->sNick, curUser->sIP, sData);
        if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::RevConnectToMe5") == true) {
            UdpDebug->Broadcast(g_sBuffer, imsgLen);
        }

        UserClose(curUser);
        return;
    }

    // PPK ... check flood ...
    if(bCheck == true && ProfileMan->IsAllowed(curUser, ProfileManager::NODEFLOODRCTM) == false) { 
        if(SettingManager->iShorts[SETSHORT_RCTM_ACTION] != 0) {
            if(DeFloodCheckForFlood(curUser, DEFLOOD_RCTM, SettingManager->iShorts[SETSHORT_RCTM_ACTION], 
              curUser->ui16RCTMs, curUser->ui64RCTMsTick, SettingManager->iShorts[SETSHORT_RCTM_MESSAGES], 
              (uint32_t)SettingManager->iShorts[SETSHORT_RCTM_TIME]) == true) {
				return;
            }
        }

        if(SettingManager->iShorts[SETSHORT_RCTM_ACTION2] != 0) {
            if(DeFloodCheckForFlood(curUser, DEFLOOD_RCTM, SettingManager->iShorts[SETSHORT_RCTM_ACTION2], 
              curUser->ui16RCTMs2, curUser->ui64RCTMsTick2, SettingManager->iShorts[SETSHORT_RCTM_MESSAGES2], 
			  (uint32_t)SettingManager->iShorts[SETSHORT_RCTM_TIME2]) == true) {
                return;
            }
        }
    }

    if(iLen > (uint32_t)SettingManager->iShorts[SETSHORT_MAX_RCTM_LEN]) {
        int imsgLen = sprintf(msg, "<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_RCTM_TOO_LONG]);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::RevConnectToMe2") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }

        if(iLen > 65000) {
            sData[65000] = '\0';
        }

        imsgLen = sprintf(g_sBuffer, "[SYS] Long $RevConnectToMe from %s (%s) - user closed. (%s)", curUser->sNick, curUser->sIP, sData);
        if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::RevConnectToMe3") == true) {
            UdpDebug->Broadcast(g_sBuffer, imsgLen);
        }

        UserClose(curUser);
        return;
    }

    // PPK ... $RCTM means user is pasive ?!? Probably yes, let set it not active and use on another places ;)
    if(curUser->sTag == NULL) {
        curUser->ui32BoolBits &= ~User::BIT_IPV4_ACTIVE;
    }

	if(ScriptManager->Arrival(curUser, sData, iLen, ScriptMan::REVCONNECTTOME_ARRIVAL) == true ||
		curUser->ui8State >= User::STATE_CLOSING) {
		return;
	}

    sData[iLen-1] = '\0'; // cutoff pipe
       
    User *OtherUser = hashManager->FindUser(sData+17+curUser->ui8NickLen, iLen-(18+curUser->ui8NickLen));
    // PPK ... no connection to yourself !!!
    if(OtherUser != NULL && OtherUser != curUser && OtherUser->ui8State == User::STATE_ADDED) {
        sData[iLen-1] = '|'; // add back pipe
        UserAddPrcsdCmd(curUser, PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO, sData, iLen, OtherUser);
    }   
}
//---------------------------------------------------------------------------

// $SR <nickname> - Search Respond for passive users
void cDcCommands::SR(User * curUser, char * sData, const uint32_t &iLen, const bool &bCheck) {
    if(iLen < 6u+curUser->ui8NickLen) {
        int imsgLen = sprintf(msg, "[SYS] Bad $SR (%s) from %s (%s) - user closed.", sData, curUser->sNick, curUser->sIP);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::SR1") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(curUser);
        return;
    }

    // PPK ... check flood ...
    if(bCheck == true && ProfileMan->IsAllowed(curUser, ProfileManager::NODEFLOODSR) == false) { 
        if(SettingManager->iShorts[SETSHORT_SR_ACTION] != 0) {
            if(DeFloodCheckForFlood(curUser, DEFLOOD_SR, SettingManager->iShorts[SETSHORT_SR_ACTION], 
              curUser->ui16SRs, curUser->ui64SRsTick, SettingManager->iShorts[SETSHORT_SR_MESSAGES], 
              (uint32_t)SettingManager->iShorts[SETSHORT_SR_TIME]) == true) {
				return;
            }
        }

        if(SettingManager->iShorts[SETSHORT_SR_ACTION2] != 0) {
            if(DeFloodCheckForFlood(curUser, DEFLOOD_SR, SettingManager->iShorts[SETSHORT_SR_ACTION2], 
              curUser->ui16SRs2, curUser->ui64SRsTick2, SettingManager->iShorts[SETSHORT_SR_MESSAGES2], 
			  (uint32_t)SettingManager->iShorts[SETSHORT_SR_TIME2]) == true) {
                return;
            }
        }
    }

    if(iLen > (uint32_t)SettingManager->iShorts[SETSHORT_MAX_SR_LEN]) {
        int imsgLen = sprintf(msg, "<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SR_TOO_LONG]);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::SR2") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }

        if(iLen > 65000) {
            sData[65000] = '\0';
        }

        imsgLen = sprintf(g_sBuffer, "[SYS] Long $SR from %s (%s) - user closed. (%s)", curUser->sNick, curUser->sIP, sData);
        if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::SR3") == true) {
            UdpDebug->Broadcast(g_sBuffer, imsgLen);
        }

        UserClose(curUser);
		return;
    }

    // check $SR spoofing (thanx Fusbar)
    // PPK... added checking for empty space after nick
    if(sData[4+curUser->ui8NickLen] != ' ' || memcmp(sData+4, curUser->sNick, curUser->ui8NickLen) != 0) {
        if(iLen > 65000) {
            sData[65000] = '\0';
        }

        int imsgLen = sprintf(g_sBuffer, "[SYS] Nick spoofing in SR from %s (%s) - user closed. (%s)", curUser->sNick, curUser->sIP, sData);
        if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::SR5") == true) {
            UdpDebug->Broadcast(g_sBuffer, imsgLen);
        }

        UserClose(curUser);
        return;
    }

    // is this response on SlotFetch request ?
    /*if(strcmp(towho, "SlotFetch") == 0) {
        int s,c=0;
        char *slash;
        slash = strchr(sData, '/');
        if(slash == NULL) return;

        s = atoi(slash);
        curUser->Slots = s;
        if(s < hubForm->UpDownMinSlots->Position) {
            if(ProfileMan->IsAllowed(curUser, ProfileMan::NOSLOTCHECK) == true) return; // Applies for OPs ?
            int imsgLen = sprintf(msg, "$To: %s From: %s $<%s> %s|", curUser->Nick, HubBotName->Text.c_str(), HubBotName->Text.c_str(), MinSlotsCheckMessage->Text.c_str());
            UserSendChar(curUser, msg, imsgLen);
            if(hubForm->MinShareRedir->Checked) {
                imsgLen = sprintf(msg, "<%s> %s|$ForceMove %s|", hubForm->HubBotName->Text.c_str(), hubForm->ShareLimitMessage->Text.c_str(), hubForm->MinShareRedirAddr->Text.c_str());
               	UserSendChar(curUser, msg, imsgLen);
            }
            int imsgLen = sprintf(msg, "[SYS] SlotFetch %s (%s) - user closed.", curUser->Nick, curUser->IP);
            UdpDebug->Broadcast(msg, imsgLen);
            UserClose(curUser);
        }
        return;
    }*/

    // past SR to script only if it's not a data for SlotFetcher
	if(ScriptManager->Arrival(curUser, sData, iLen, ScriptMan::SR_ARRIVAL) == true ||
		curUser->ui8State >= User::STATE_CLOSING) {
		return;
	}

    sData[iLen-1] = '\0'; // cutoff pipe

    char *towho = strrchr(sData, '\5');
    if(towho == NULL) return;

    User *OtherUser = hashManager->FindUser(towho+1, iLen-2-(towho-sData));
    // PPK ... no $SR to yourself !!!
    if(OtherUser != NULL && OtherUser != curUser && OtherUser->ui8State == User::STATE_ADDED) {
        // PPK ... search replies limiting
        if(SettingManager->iShorts[SETSHORT_MAX_PASIVE_SR] != 0) {
			if(OtherUser->iSR >= (uint32_t)SettingManager->iShorts[SETSHORT_MAX_PASIVE_SR])
                return;
                        
            OtherUser->iSR++;
        }

        // cutoff the last part // PPK ... and do it fast ;)
        towho[0] = '|';
        towho[1] = '\0';
        UserAddPrcsdCmd(curUser, PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO, sData, iLen-OtherUser->ui8NickLen-1, OtherUser);
    }   
}

//---------------------------------------------------------------------------

// $SR <nickname> - Search Respond for active users from UDP
void cDcCommands::SRFromUDP(User * curUser, char * sData, const size_t &szLen) {
	if(ScriptManager->Arrival(curUser, sData, szLen, ScriptMan::UDP_SR_ARRIVAL) == true ||
		curUser->ui8State >= User::STATE_CLOSING) {
		return;
	}
}
//---------------------------------------------------------------------------

// $Supports item item item... PPK $Supports UserCommand NoGetINFO NoHello UserIP2 QuickList|
void cDcCommands::Supports(User * curUser, char * sData, const uint32_t &iLen) {
    if(((curUser->ui32BoolBits & User::BIT_HAVE_SUPPORTS) == User::BIT_HAVE_SUPPORTS) == true) {
        int imsgLen = sprintf(msg, "[SYS] $Supports flood from %s (%s) - user closed.", curUser->sNick, curUser->sIP);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::Supports1") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(curUser);
        return;
    }

    curUser->ui32BoolBits |= User::BIT_HAVE_SUPPORTS;
    
    if(iLen < 13) {
        int imsgLen = sprintf(msg, "[SYS] Bad $Supports (%s) from %s (%s) - user closed.", sData, curUser->sNick, curUser->sIP);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::Supports2") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(curUser);
        return;
    }

    if(sData[iLen-2] == ' ') {
        if(SettingManager->bBools[SETBOOL_NO_QUACK_SUPPORTS] == false) {
            curUser->ui32BoolBits |= User::BIT_QUACK_SUPPORTS;
        } else {
            if(iLen > 65000) {
                sData[65000] = '\0';
            }

            int imsgLen = sprintf(g_sBuffer, "[SYS] Quack $Supports from %s (%s) - user closed. (%s)", curUser->sNick, curUser->sIP, sData);
            if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::Supports3") == true) {
                UdpDebug->Broadcast(g_sBuffer, imsgLen);
            }

       		imsgLen = sprintf(msg, "<%s> %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_QUACK_SUPPORTS]);
			if(CheckSprintf(imsgLen, 1024, "cDcCommands::Supports5") == true) {
				UserSendCharDelayed(curUser, msg, imsgLen);
			}

			UserClose(curUser);
			return;
		}
    }

	ScriptManager->Arrival(curUser, sData, iLen, ScriptMan::SUPPORTS_ARRIVAL);

	if(curUser->ui8State >= User::STATE_CLOSING) {
    	return;
    }

    char *sSupport = sData+10;
	size_t szDataLen;
                    
    for(uint32_t ui32i = 10; ui32i < iLen-1; ui32i++) {
        if(sData[ui32i] == ' ') {
            sData[ui32i] = '\0';
        } else if(ui32i != iLen-2) {
            continue;
        } else {
            ui32i++;
        }

        szDataLen = (sData+ui32i)-sSupport;

        switch(sSupport[0]) {
            case 'N':
                if(sSupport[1] == 'o') {
                    if(((curUser->ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == false && szDataLen == 7 && memcmp(sSupport+2, "Hello", 5) == 0) {
                        curUser->ui32SupportBits |= User::SUPPORTBIT_NOHELLO;
                    } else if(((curUser->ui32SupportBits & User::SUPPORTBIT_NOGETINFO) == User::SUPPORTBIT_NOGETINFO) == false && szDataLen == 9 && memcmp(sSupport+2, "GetINFO", 7) == 0) {
                        curUser->ui32SupportBits |= User::SUPPORTBIT_NOGETINFO;
                    }
                }
                break;
            case 'Q': {
                if(((curUser->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == false && szDataLen == 9 && *((uint64_t *)(sSupport+1)) == *((uint64_t *)"uickList")) {
                    curUser->ui32SupportBits |= User::SUPPORTBIT_QUICKLIST;
                    // PPK ... in fact NoHello is only not fully implemented Quicklist (without diferent login sequency)
                    // That's why i overide NoHello here and use bQuicklist only for login, on other places is same as NoHello ;)
                    curUser->ui32SupportBits |= User::SUPPORTBIT_NOHELLO;
                }
                break;
            }
            case 'U': {
                if(((curUser->ui32SupportBits & User::SUPPORTBIT_USERCOMMAND) == User::SUPPORTBIT_USERCOMMAND) == false && szDataLen == 11 && memcmp(sSupport+1, "serCommand", 10) == 0) {
                    curUser->ui32SupportBits |= User::SUPPORTBIT_USERCOMMAND;
                } else if(((curUser->ui32SupportBits & User::SUPPORTBIT_USERIP2) == User::SUPPORTBIT_USERIP2) == false && szDataLen == 7 && memcmp(sSupport+1, "serIP2", 6) == 0) {
                    curUser->ui32SupportBits |= User::SUPPORTBIT_USERIP2;
                }
                break;
            }
            case 'B': {
                if(((curUser->ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == false && szDataLen == 7 && memcmp(sSupport+1, "otINFO", 6) == 0) {
                    if(SettingManager->bBools[SETBOOL_DONT_ALLOW_PINGERS] == true) {
                        int imsgLen = sprintf(msg, "<%s> %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SORRY_THIS_HUB_NOT_ALLOW_PINGERS]);
                        if(CheckSprintf(imsgLen, 1024, "cDcCommands::Supports6") == true) {
                            UserSendChar(curUser, msg, imsgLen);
                        }
                        UserClose(curUser);
                        return;
                    } else if(SettingManager->bBools[SETBOOL_CHECK_IP_IN_COMMANDS] == false) {
                        int imsgLen = sprintf(msg, "<%s> Sorry, this hub banned yourself from hublist because allow CTM exploit.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
                        if(CheckSprintf(imsgLen, 1024, "cDcCommands::Supports7") == true) {
                            UserSendChar(curUser, msg, imsgLen);
                        }
                        UserClose(curUser);
                        return;
                    } else {
                        curUser->ui32BoolBits |= User::BIT_PINGER;
                        int imsgLen = GetWlcmMsg(msg);
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                }
                break;
            }
            case 'Z': {
                if(((curUser->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false && ((szDataLen == 5 && *((uint32_t *)(sSupport+1)) == *((uint32_t *)"Pipe")) ||
                    (szDataLen == 6 && memcmp(sSupport+1, "Pipe0", 5) == 0))) {
                    curUser->ui32SupportBits |= User::SUPPORTBIT_ZPIPE;
                    iStatZPipe++;
                }
                break;
            }
            case 'I': {
                if(szDataLen == 4) {
                    if(((curUser->ui32SupportBits & User::SUPPORTBIT_IP64) == User::SUPPORTBIT_IP64) == false && *((uint32_t *)sSupport) == *((uint32_t *)"IP64")) {
                        curUser->ui32SupportBits |= User::SUPPORTBIT_IP64;
                    } else if(((curUser->ui32SupportBits & User::SUPPORTBIT_IPV4) == User::SUPPORTBIT_IPV4) == false && *((uint32_t *)sSupport) == *((uint32_t *)"IPv4")) {
                        curUser->ui32SupportBits |= User::SUPPORTBIT_IPV4;
                    }
                }
                break;
            }
            case '\0': {
                // PPK ... corrupted $Supports ???
                int imsgLen = sprintf(msg, "[SYS] Bad $Supports from %s (%s) - user closed.", curUser->sNick, curUser->sIP);
                if(CheckSprintf(imsgLen, 1024, "cDcCommands::Supports8") == true) {
                    UdpDebug->Broadcast(msg, imsgLen);
                }
                UserClose(curUser);
                return;
            }
            default:
                // PPK ... unknown supports
                break;
        }
                
        sSupport = sData+ui32i+1;
    }
    
    curUser->ui8State = User::STATE_VALIDATE;
    
    UserAddPrcsdCmd(curUser, PrcsdUsrCmd::SUPPORTS, NULL, 0, NULL);
}
//---------------------------------------------------------------------------

// $To: nickname From: ownnickname $<ownnickname> <message>
void cDcCommands::To(User * curUser, char * sData, const uint32_t &iLen, const bool &bCheck) {
    char *cTemp = strchr(sData+5, ' ');

    if(iLen < 19 || cTemp == NULL) {
        if(iLen > 65000) {
            sData[65000] = '\0';
        }

        int imsgLen = sprintf(g_sBuffer, "[SYS] Bad To from %s (%s) - user closed. (%s)", curUser->sNick, curUser->sIP, sData);
        if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::To1") == true) {
            UdpDebug->Broadcast(g_sBuffer, imsgLen);
        }

        UserClose(curUser);
        return;
    }
    
    size_t szNickLen = cTemp-(sData+5);

    if(szNickLen > 64) {
        int imsgLen = sprintf(msg, "<%s> *** %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::To3") == true) {
            UserSendCharDelayed(curUser, msg, imsgLen);
        }
        return;
    }

    // is the mesg really from us ?
    // PPK ... replaced by better and faster code ;)
    int imsgLen = sprintf(msg, "From: %s $<%s> ", curUser->sNick, curUser->sNick);
    if(CheckSprintf(imsgLen, 1024, "cDcCommands::To4") == true) {
        if(strncmp(cTemp+1, msg, imsgLen) != 0) {
            if(iLen > 65000) {
                sData[65000] = '\0';
            }

            int imsgLen = sprintf(g_sBuffer, "[SYS] Nick spoofing in To from %s (%s) - user closed. (%s)", curUser->sNick, curUser->sIP, sData);
            if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::To5") == true) {
                UdpDebug->Broadcast(g_sBuffer, imsgLen);
            }

            UserClose(curUser);
            return;
        }
    }

    //FloodCheck
    if(bCheck == true && ProfileMan->IsAllowed(curUser, ProfileManager::NODEFLOODPM) == false) {
        // PPK ... pm antiflood
        if(SettingManager->iShorts[SETSHORT_PM_ACTION] != 0) {
            cTemp[0] = '\0';
            if(DeFloodCheckForFlood(curUser, DEFLOOD_PM, SettingManager->iShorts[SETSHORT_PM_ACTION], 
                curUser->ui16PMs, curUser->ui64PMsTick, SettingManager->iShorts[SETSHORT_PM_MESSAGES], 
                (uint32_t)SettingManager->iShorts[SETSHORT_PM_TIME], sData+5) == true) {
                return;
            }
            cTemp[0] = ' ';
        }

        if(SettingManager->iShorts[SETSHORT_PM_ACTION2] != 0) {
            cTemp[0] = '\0';
            if(DeFloodCheckForFlood(curUser, DEFLOOD_PM, SettingManager->iShorts[SETSHORT_PM_ACTION2], 
                curUser->ui16PMs2, curUser->ui64PMsTick2, SettingManager->iShorts[SETSHORT_PM_MESSAGES2], 
                (uint32_t)SettingManager->iShorts[SETSHORT_PM_TIME2], sData+5) == true) {
                return;
            }
            cTemp[0] = ' ';
        }

        // 2nd check for PM flooding
		if(SettingManager->iShorts[SETSHORT_SAME_PM_ACTION] != 0) {
			bool bNewData = false;
			cTemp[0] = '\0';
			if(DeFloodCheckForSameFlood(curUser, DEFLOOD_SAME_PM, SettingManager->iShorts[SETSHORT_SAME_PM_ACTION],
                curUser->ui16SamePMs, curUser->ui64SamePMsTick, 
                SettingManager->iShorts[SETSHORT_SAME_PM_MESSAGES], SettingManager->iShorts[SETSHORT_SAME_PM_TIME], 
                cTemp+(12+(2*curUser->ui8NickLen)), (iLen-(cTemp-sData))-(12+(2*curUser->ui8NickLen)), 
				curUser->sLastPM, curUser->ui16LastPMLen, bNewData, sData+5) == true) {
                return;
            }
            cTemp[0] = ' ';

			if(bNewData == true) {
				UserSetLastPM(curUser, cTemp+(12+(2*curUser->ui8NickLen)), (iLen-(cTemp-sData))-(12+(2*curUser->ui8NickLen)));
			}
        }
    }

    if(ProfileMan->IsAllowed(curUser, ProfileManager::NOCHATLIMITS) == false) {
        // 1st check for length limit for PM message
        size_t szMessLen = iLen-(2*curUser->ui8NickLen)-(cTemp-sData)-13;
        if(SettingManager->iShorts[SETSHORT_MAX_PM_LEN] != 0 && szMessLen > (size_t)SettingManager->iShorts[SETSHORT_MAX_PM_LEN]) {
       	    // PPK ... hubsec alias
       	    cTemp[0] = '\0';
       	   	int imsgLen = sprintf(msg, "$To: %s From: %s $<%s> %s!|", curUser->sNick, sData+5, SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                LanguageManager->sTexts[LAN_THE_MESSAGE_WAS_TOO_LONG]);
            if(CheckSprintf(imsgLen, 1024, "cDcCommands::To14") == true) {
               	UserSendCharDelayed(curUser, msg, imsgLen);
            }
            return;
        }

        // PPK ... check for message lines limit
        if(SettingManager->iShorts[SETSHORT_MAX_PM_LINES] != 0 || SettingManager->iShorts[SETSHORT_SAME_MULTI_PM_ACTION] != 0) {
			if(curUser->ui16SamePMs < 2) {
				uint16_t iLines = 1;
                for(uint32_t ui32i = 9+curUser->ui8NickLen; ui32i < iLen-(cTemp-sData)-1; ui32i++) {
                    if(cTemp[ui32i] == '\n') {
                        iLines++;
                    }
                }
                curUser->ui16LastPmLines = iLines;
                if(curUser->ui16LastPmLines > 1) {
                    curUser->ui16SameMultiPms++;
                }
            } else if(curUser->ui16LastPmLines > 1) {
                curUser->ui16SameMultiPms++;
            }

			if(ProfileMan->IsAllowed(curUser, ProfileManager::NODEFLOODPM) == false && SettingManager->iShorts[SETSHORT_SAME_MULTI_PM_ACTION] != 0) {
				if(curUser->ui16SameMultiPms > SettingManager->iShorts[SETSHORT_SAME_MULTI_PM_MESSAGES] &&
                    curUser->ui16LastPmLines >= SettingManager->iShorts[SETSHORT_SAME_MULTI_PM_LINES]) {
					cTemp[0] = '\0';
					uint16_t lines = 0;
					DeFloodDoAction(curUser, DEFLOOD_SAME_MULTI_PM, SettingManager->iShorts[SETSHORT_SAME_MULTI_PM_ACTION], lines, sData+5);
                    return;
                }
            }

            if(SettingManager->iShorts[SETSHORT_MAX_PM_LINES] != 0 && curUser->ui16LastPmLines > SettingManager->iShorts[SETSHORT_MAX_PM_LINES]) {
                cTemp[0] = '\0';
                int imsgLen = sprintf(msg, "$To: %s From: %s $<%s> %s!|", curUser->sNick, sData+5, SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                    LanguageManager->sTexts[LAN_THE_MESSAGE_WAS_TOO_LONG]);
                if(CheckSprintf(imsgLen, 1024, "cDcCommands::To15") == true) {
                    UserSendCharDelayed(curUser, msg, imsgLen);
                }
                return;
            }
        }
    }

    if(bCheck == true && ProfileMan->IsAllowed(curUser, ProfileManager::NOPMINTERVAL) == false) {
        cTemp[0] = '\0';
        if(DeFloodCheckInterval(curUser, INTERVAL_PM, curUser->ui16PMsInt, 
            curUser->ui64PMsIntTick, SettingManager->iShorts[SETSHORT_PM_INTERVAL_MESSAGES], 
            (uint32_t)SettingManager->iShorts[SETSHORT_PM_INTERVAL_TIME], sData+5) == true) {
            return;
        }
        cTemp[0] = ' ';
    }

	if(ScriptManager->Arrival(curUser, sData, iLen, ScriptMan::TO_ARRIVAL) == true ||
		curUser->ui8State >= User::STATE_CLOSING) {
		return;
	}

    cTemp[0] = '\0';

    // ignore the silly debug messages !!!
    if(memcmp(sData+5, "Vandel\\Debug", 12) == 0) {
        return;
    }

    // Everything's ok lets chat
    // if this is a PM to OpChat or Hub bot, process the message
    if(SettingManager->bBools[SETBOOL_REG_BOT] == true && strcmp(sData+5, SettingManager->sTexts[SETTXT_BOT_NICK]) == 0) {
        cTemp += 9+curUser->ui8NickLen;
        // PPK ... check message length, return if no mess found
        uint32_t iLen1 = (uint32_t)((iLen-(cTemp-sData))+1);
        if(iLen1 <= curUser->ui8NickLen+4u)
            return;

        // find chat message data
        char *sBuff = cTemp+curUser->ui8NickLen+3;

        // non-command chat msg
        for(uint8_t ui8i = 0; ui8i < (uint8_t)SettingManager->ui16TextsLens[SETTXT_CHAT_COMMANDS_PREFIXES]; ui8i++) {
            if(sBuff[0] == SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][ui8i]) {
                sData[iLen-1] = '\0'; // cutoff pipe
                // built-in commands
                if(SettingManager->bBools[SETBOOL_ENABLE_TEXT_FILES] == true && 
                    TextFileManager->ProcessTextFilesCmd(curUser, sBuff+1, true)) {
                    return;
                }
           
                // HubCommands
                if(iLen1-curUser->ui8NickLen >= 10) {
                    if(HubCmds->DoCommand(curUser, sBuff, iLen1, true)) return;
                }
                        
                sData[iLen-1] = '|'; // add back pipe
                break;
            }
        }
        // PPK ... if i am here is not textfile request or hub command, try opchat
        if(SettingManager->bBools[SETBOOL_REG_OP_CHAT] == true && ProfileMan->IsAllowed(curUser, ProfileManager::ALLOWEDOPCHAT) == true && 
            SettingManager->bBotsSameNick == true) {
            uint32_t iOpChatLen = SettingManager->ui16TextsLens[SETTXT_OP_CHAT_NICK];
            memcpy(cTemp-iOpChatLen-2, SettingManager->sTexts[SETTXT_OP_CHAT_NICK], iOpChatLen);
            UserAddPrcsdCmd(curUser, PrcsdUsrCmd::TO_OP_CHAT, cTemp-iOpChatLen-2, iLen-((cTemp-iOpChatLen-2)-sData), NULL);
        }
    } else if(SettingManager->bBools[SETBOOL_REG_OP_CHAT] == true && 
        ProfileMan->IsAllowed(curUser, ProfileManager::ALLOWEDOPCHAT) == true && 
        strcmp(sData+5, SettingManager->sTexts[SETTXT_OP_CHAT_NICK]) == 0) {
        cTemp += 9+curUser->ui8NickLen;
        uint32_t iOpChatLen = SettingManager->ui16TextsLens[SETTXT_OP_CHAT_NICK];
        memcpy(cTemp-iOpChatLen-2, SettingManager->sTexts[SETTXT_OP_CHAT_NICK], iOpChatLen);
        UserAddPrcsdCmd(curUser, PrcsdUsrCmd::TO_OP_CHAT, cTemp-iOpChatLen-2, (iLen-(cTemp-sData))+iOpChatLen+2, NULL);
    } else {       
        User *OtherUser = hashManager->FindUser(sData+5, szNickLen);
        // PPK ... pm to yourself ?!? NO!
        if(OtherUser != NULL && OtherUser != curUser && OtherUser->ui8State == User::STATE_ADDED) {
            cTemp[0] = ' ';
            UserAddPrcsdCmd(curUser, PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO, sData, iLen, OtherUser, true);
        }
    }
}
//---------------------------------------------------------------------------

// $ValidateNick
void cDcCommands::ValidateNick(User * curUser, char * sData, const uint32_t &iLen) {
    if(((curUser->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == true) {
        int imsgLen = sprintf(msg, "[SYS] $ValidateNick with QuickList support from %s (%s) - user closed.", curUser->sNick, curUser->sIP);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::ValidateNick1") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(curUser);
        return;
    }

    if(iLen < 16) {
        int imsgLen = sprintf(msg, "[SYS] Attempt to Validate empty nick (%s) from %s (%s) - user closed.", sData, curUser->sNick, curUser->sIP);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::ValidateNick3") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(curUser);
        return;
    }

    sData[iLen-1] = '\0'; // cutoff pipe
    if(ValidateUserNick(curUser, sData+14, iLen-15, true) == false) return;

	sData[iLen-1] = '|'; // add back pipe

	ScriptManager->Arrival(curUser, sData, iLen, ScriptMan::VALIDATENICK_ARRIVAL);
}
//---------------------------------------------------------------------------

// $Version
void cDcCommands::Version(User * curUser, char * sData, const uint32_t &iLen) {
    if(iLen < 11) {
        int imsgLen = sprintf(msg, "[SYS] Bad $Version (%s) from %s (%s) - user closed.", sData, curUser->sNick, curUser->sIP);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::Version3") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(curUser);
        return;
    }

	curUser->ui8State = User::STATE_GETNICKLIST_OR_MYINFO;

	ScriptManager->Arrival(curUser, sData, iLen, ScriptMan::VERSION_ARRIVAL);

	if(curUser->ui8State >= User::STATE_CLOSING) {
    	return;
    }
    
    sData[iLen-1] = '\0'; // cutoff pipe
    UserSetVersion(curUser, sData+9);
}
//---------------------------------------------------------------------------

// Chat message
bool cDcCommands::ChatDeflood(User * curUser, char * sData, const uint32_t &iLen, const bool &bCheck) {
#ifdef _BUILD_GUI
    if(::SendMessage(pMainWindowPageUsersChat->hWndPageItems[MainWindowPageUsersChat::BTN_SHOW_CHAT], BM_GETCHECK, 0, 0) == BST_CHECKED) {
        sData[iLen - 1] = '\0';
        RichEditAppendText(pMainWindowPageUsersChat->hWndPageItems[MainWindowPageUsersChat::REDT_CHAT], sData);
        sData[iLen - 1] = '|';
    }
#endif
    
	// if the user is sending chat as other user, kick him
	if(sData[1+curUser->ui8NickLen] != '>' || sData[2+curUser->ui8NickLen] != ' ' || memcmp(curUser->sNick, sData+1, curUser->ui8NickLen) != 0) {
        if(iLen > 65000) {
            sData[65000] = '\0';
        }

        int imsgLen = sprintf(g_sBuffer, "[SYS] Nick spoofing in chat from %s (%s) - user closed. (%s)", curUser->sNick, curUser->sIP, sData);
        if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::ChatDeflood1") == true) {
            UdpDebug->Broadcast(g_sBuffer, imsgLen);
        }

		UserClose(curUser);
		return false;
	}
        
	// PPK ... check flood...
	if(bCheck == true && ProfileMan->IsAllowed(curUser, ProfileManager::NODEFLOODMAINCHAT) == false) {
		if(SettingManager->iShorts[SETSHORT_MAIN_CHAT_ACTION] != 0) {
            if(DeFloodCheckForFlood(curUser, DEFLOOD_CHAT, SettingManager->iShorts[SETSHORT_MAIN_CHAT_ACTION], 
                curUser->ui16ChatMsgs, curUser->ui64ChatMsgsTick, SettingManager->iShorts[SETSHORT_MAIN_CHAT_MESSAGES], 
                (uint32_t)SettingManager->iShorts[SETSHORT_MAIN_CHAT_TIME]) == true) {
                return false;
            }
		}

		if(SettingManager->iShorts[SETSHORT_MAIN_CHAT_ACTION2] != 0) {
            if(DeFloodCheckForFlood(curUser, DEFLOOD_CHAT, SettingManager->iShorts[SETSHORT_MAIN_CHAT_ACTION2], 
                curUser->ui16ChatMsgs2, curUser->ui64ChatMsgsTick2, SettingManager->iShorts[SETSHORT_MAIN_CHAT_MESSAGES2], 
                (uint32_t)SettingManager->iShorts[SETSHORT_MAIN_CHAT_TIME2]) == true) {
                return false;
            }
		}

		// 2nd check for chatmessage flood
		if(SettingManager->iShorts[SETSHORT_SAME_MAIN_CHAT_ACTION] != 0) {
			bool bNewData = false;
			if(DeFloodCheckForSameFlood(curUser, DEFLOOD_SAME_CHAT, SettingManager->iShorts[SETSHORT_SAME_MAIN_CHAT_ACTION],
			  curUser->ui16SameChatMsgs, curUser->ui64SameChatsTick,
              SettingManager->iShorts[SETSHORT_SAME_MAIN_CHAT_MESSAGES], SettingManager->iShorts[SETSHORT_SAME_MAIN_CHAT_TIME], 
			  sData+curUser->ui8NickLen+3, iLen-(curUser->ui8NickLen+3), curUser->sLastChat, curUser->ui16LastChatLen, bNewData) == true) {
				return false;
			}

			if(bNewData == true) {
				UserSetLastChat(curUser, sData+curUser->ui8NickLen+3, iLen-(curUser->ui8NickLen+3));
			}
        }
    }

    // PPK ... ignore empty chat ;)
    if(iLen < curUser->ui8NickLen+5u) {
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------

// Chat message
void cDcCommands::Chat(User * curUser, char * sData, const uint32_t &iLen, const bool &bCheck) {   
    if(bCheck == true && ProfileMan->IsAllowed(curUser, ProfileManager::NOCHATLIMITS) == false) {
    	// PPK ... check for message limit length
 	    if(SettingManager->iShorts[SETSHORT_MAX_CHAT_LEN] != 0 && (iLen-curUser->ui8NickLen-4) > (uint32_t)SettingManager->iShorts[SETSHORT_MAX_CHAT_LEN]) {
     		// PPK ... hubsec alias
	   		int imsgLen = sprintf(msg, "<%s> %s !|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_THE_MESSAGE_WAS_TOO_LONG]);
	   		if(CheckSprintf(imsgLen, 1024, "cDcCommands::Chat1") == true) {
                UserSendCharDelayed(curUser, msg, imsgLen);
            }
	        return;
 	    }

        // PPK ... check for message lines limit
        if(SettingManager->iShorts[SETSHORT_MAX_CHAT_LINES] != 0 || SettingManager->iShorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION] != 0) {
            if(curUser->ui16SameChatMsgs < 2) {
                uint16_t iLines = 1;

                for(uint32_t ui32i = curUser->ui8NickLen+3; ui32i < iLen-1; ui32i++) {
                    if(sData[ui32i] == '\n') {
                        iLines++;
                    }
                }

                curUser->ui16LastChatLines = iLines;

                if(curUser->ui16LastChatLines > 1) {
                    curUser->ui16SameMultiChats++;
                }
			} else if(curUser->ui16LastChatLines > 1) {
                curUser->ui16SameMultiChats++;
            }

			if(ProfileMan->IsAllowed(curUser, ProfileManager::NODEFLOODMAINCHAT) == false && SettingManager->iShorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION] != 0) {
                if(curUser->ui16SameMultiChats > SettingManager->iShorts[SETSHORT_SAME_MULTI_MAIN_CHAT_MESSAGES] && 
                    curUser->ui16LastChatLines >= SettingManager->iShorts[SETSHORT_SAME_MULTI_MAIN_CHAT_LINES]) {
                    uint16_t lines = 0;
					DeFloodDoAction(curUser, DEFLOOD_SAME_MULTI_CHAT, SettingManager->iShorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION], lines, NULL);
					return;
				}
			}

			if(SettingManager->iShorts[SETSHORT_MAX_CHAT_LINES] != 0 && curUser->ui16LastChatLines > SettingManager->iShorts[SETSHORT_MAX_CHAT_LINES]) {
                int imsgLen = sprintf(msg, "<%s> %s !|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_THE_MESSAGE_WAS_TOO_LONG]);
                if(CheckSprintf(imsgLen, 1024, "cDcCommands::Chat2") == true) {
                    UserSendCharDelayed(curUser, msg, imsgLen);
                }
                return;
			}
		}
	}

    if(bCheck == true && ProfileMan->IsAllowed(curUser, ProfileManager::NOCHATINTERVAL) == false) {
        if(DeFloodCheckInterval(curUser, INTERVAL_CHAT, curUser->ui16ChatIntMsgs, 
            curUser->ui64ChatIntMsgsTick, SettingManager->iShorts[SETSHORT_CHAT_INTERVAL_MESSAGES], 
            (uint32_t)SettingManager->iShorts[SETSHORT_CHAT_INTERVAL_TIME]) == true) {
            return;
        }
    }

    if(((curUser->ui32BoolBits & User::BIT_GAGGED) == User::BIT_GAGGED) == true)
        return;

    void * pQueueItem1 = g_GlobalDataQueue->GetLastQueueItem();

	if(ScriptManager->Arrival(curUser, sData, iLen, ScriptMan::CHAT_ARRIVAL) == true ||
		curUser->ui8State >= User::STATE_CLOSING) {
		return;
	}

    User * pToUser = NULL;
    void * pQueueItem2 = g_GlobalDataQueue->GetLastQueueItem();

    if(pQueueItem1 != pQueueItem2) {
        if(pQueueItem1 == NULL) {
            pToUser = (User *)g_GlobalDataQueue->GetFirstQueueItem();
        } else {
            pToUser = (User *)pQueueItem1;
        }
    }

	// PPK ... filtering kick messages
	if(ProfileMan->IsAllowed(curUser, ProfileManager::KICK) == true) {
    	if(iLen > curUser->ui8NickLen+21u) {
            char * cTemp = strchr(sData+curUser->ui8NickLen+3, '\n');
            if(cTemp != NULL) {
            	cTemp[0] = '\0';
            }

            char *temp, *temp1;
            if((temp = stristr(sData+curUser->ui8NickLen+3, "is kicking ")) != NULL &&
            	(temp1 = stristr(temp+12, " because: ")) != NULL) {
                // PPK ... catch kick message and store for later use in $Kick for tempban reason
                temp1[0] = '\0';               
                User * KickedUser = hashManager->FindUser(temp+11, temp1-(temp+11));
                temp1[0] = ' ';
                if(KickedUser != NULL) {
                    // PPK ... valid kick messages for existing user, remove this message from deflood ;)
                    if(curUser->ui16ChatMsgs != 0) {
                        curUser->ui16ChatMsgs--;
                        curUser->ui16ChatMsgs2--;
                    }
                    if(temp1[10] != '|') {
                        sData[iLen-1] = '\0'; // get rid of the pipe
                        UserSetKickMsg(KickedUser, temp1+10, iLen-(temp1-sData)-11);
                        sData[iLen-1] = '|'; // add back pipe
                    }
                }

            	if(cTemp != NULL) {
                    cTemp[0] = '\n';
                }

                // PPK ... kick messages filtering
                if(SettingManager->bBools[SETBOOL_FILTER_KICK_MESSAGES] == true) {
                	if(SettingManager->bBools[SETBOOL_SEND_KICK_MESSAGES_TO_OPS] == true) {
               			if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                            if(iLen > 65000) {
                                sData[65000] = '\0';
                            }

                            int imsgLen = sprintf(g_sBuffer, "%s $%s", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], sData);
                            if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::Chat3") == true) {
								g_GlobalDataQueue->SingleItemStore(g_sBuffer, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                            }
                        } else {
							g_GlobalDataQueue->AddQueueItem(sData, iLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                        }
		    		} else {
                        UserSendCharDelayed(curUser, sData, iLen);
                    }
                    return;
                }
            }

            if(cTemp != NULL) {
                cTemp[0] = '\n';
            }
        }        
	}

    UserAddPrcsdCmd(curUser, PrcsdUsrCmd::CHAT, sData, iLen, pToUser);
}
//---------------------------------------------------------------------------

// $Close nick|
void cDcCommands::Close(User * curUser, char * sData, const uint32_t &iLen) {
    if(ProfileMan->IsAllowed(curUser, ProfileManager::CLOSE) == false) {
        int iLen = sprintf(msg, "<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_ARE_NOT_ALWD_TO_USE_THIS_CMD]);
        if(CheckSprintf(iLen, 1024, "cDcCommands::Close1") == true) {
            UserSendCharDelayed(curUser, msg, iLen);
        }
        return;
    } 

    if(iLen < 9) {
        int imsgLen = sprintf(msg, "[SYS] Bad $Close (%s) from %s (%s) - user closed.", sData, curUser->sNick, curUser->sIP);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::Close2") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(curUser);
        return;
    }

	if(ScriptManager->Arrival(curUser, sData, iLen, ScriptMan::CLOSE_ARRIVAL) == true ||
		curUser->ui8State >= User::STATE_CLOSING) {
		return;
	}

    sData[iLen-1] = '\0'; // cutoff pipe

    User *OtherUser = hashManager->FindUser(sData+7, iLen-8);
    if(OtherUser != NULL) {
        // Self-kick
        if(OtherUser == curUser) {
        	int imsgLen = sprintf(msg, "<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_CANT_CLOSE_YOURSELF]);
        	if(CheckSprintf(imsgLen, 1024, "cDcCommands::Close3") == true) {
                UserSendCharDelayed(curUser, msg, imsgLen);
            }
            return;
    	}
    	
        if(OtherUser->iProfile != -1 && curUser->iProfile > OtherUser->iProfile) {
        	int imsgLen = sprintf(msg, "<%s> %s %s|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_ARE_NOT_ALLOWED_TO_CLOSE], OtherUser->sNick);
        	if(CheckSprintf(imsgLen, 1024, "cDcCommands::Close4") == true) {
                UserSendCharDelayed(curUser, msg, imsgLen);
            }
        	return;
        }

        // disconnect the user
        int imsgLen = sprintf(msg, "[SYS] User %s (%s) closed by %s", OtherUser->sNick, OtherUser->sIP, curUser->sNick);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::Close5") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(OtherUser);
        
        if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
            if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                int imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                    OtherUser->sNick, LanguageManager->sTexts[LAN_WITH_IP], OtherUser->sIP, LanguageManager->sTexts[LAN_WAS_CLOSED_BY], curUser->sNick);
                if(CheckSprintf(imsgLen, 1024, "cDcCommands::Close6") == true) {
					g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                }
            } else {
                int imsgLen = sprintf(msg, "<%s> *** %s %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], OtherUser->sNick, LanguageManager->sTexts[LAN_WITH_IP],
                    OtherUser->sIP, LanguageManager->sTexts[LAN_WAS_CLOSED_BY], curUser->sNick);
                if(CheckSprintf(imsgLen, 1024, "cDcCommands::Close7") == true) {
					g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                }
            }
        }
        
        if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        	int imsgLen = sprintf(msg, "<%s> *** %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], OtherUser->sNick, LanguageManager->sTexts[LAN_WITH_IP], OtherUser->sIP,
                LanguageManager->sTexts[LAN_WAS_CLOSED]);
            if(CheckSprintf(imsgLen, 1024, "cDcCommands::Close8") == true) {
                UserSendCharDelayed(curUser, msg, imsgLen);
            }
        }
    }
}
//---------------------------------------------------------------------------

void cDcCommands::Unknown(User * curUser, char * sData, const uint32_t &iLen) {
    iStatCmdUnknown++;

    #ifdef _DBG
        Memo(">>> Unimplemented Cmd "+curUser->Nick+" [" + curUser->IP + "]: " + sData);
    #endif

    // if we got unknown command sooner than full login finished
    // PPK ... fixed posibility to send (or flood !!!) hub with unknown command before full login
    // Give him chance with script...
    // if this is unkncwn command and script dont clarify that it's ok, disconnect the user
    if(ScriptManager->Arrival(curUser, sData, iLen, ScriptMan::UNKNOWN_ARRIVAL) == false) {
        if(iLen > 65000) {
            sData[65000] = '\0';
        }

        int imsgLen = sprintf(g_sBuffer, "[SYS] Unknown command from %s (%s) - user closed. (%s)", curUser->sNick, curUser->sIP, sData);
        if(CheckSprintf(imsgLen, g_szBufferSize, "cDcCommands::Unknown1") == true) {
            UdpDebug->Broadcast(g_sBuffer, imsgLen);
        }

        UserClose(curUser);
    }
}
//---------------------------------------------------------------------------

bool cDcCommands::ValidateUserNick(User * curUser, char * Nick, const size_t &szNickLen, const bool &ValidateNick) {
    // illegal characters in nick?
    for(uint32_t ui32i = 0; ui32i < szNickLen; ui32i++) {
        switch(Nick[ui32i]) {
            case ' ':
            case '$':
            case '|': {
           	    int imsgLen = sprintf(msg, "<%s> %s '%c' ! %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOUR_NICK_CONTAINS_ILLEGAL_CHARACTER],
                    Nick[ui32i], LanguageManager->sTexts[LAN_PLS_CORRECT_IT_AND_GET_BACK_AGAIN]);
                if(CheckSprintf(imsgLen, 1024, "cDcCommands::ValidateUserNick1") == true) {
                    UserSendChar(curUser, msg, imsgLen);
                }
        //        int imsgLen = sprintf(msg, "[SYS] Nick with bad chars (%s) from %s (%s) - user closed.", Nick, curUser->Nick, curUser->IP);
        //        UdpDebug->Broadcast(msg, imsgLen);
                UserClose(curUser);
                return false;
            }
            default:
                continue;
        }
    }

    UserSetNick(curUser, Nick, (uint8_t)szNickLen);
    
    // check for reserved nicks
    if(ResNickManager->CheckReserved(curUser->sNick, curUser->ui32NickHash) == true) {
   	    int imsgLen = sprintf(msg, "<%s> %s. %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_THE_NICK_IS_RESERVED_FOR_SOMEONE_OTHER],
            LanguageManager->sTexts[LAN_CHANGE_YOUR_NICK_AND_GET_BACK_AGAIN]);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::ValidateUserNick2") == true) {
       	    UserSendChar(curUser, msg, imsgLen);
        }
//       	int imsgLen = sprintf(msg, "[SYS] Reserved nick (%s) from %s (%s) - user closed.", Nick, curUser->Nick, curUser->IP);
//       	UdpDebug->Broadcast(msg, imsgLen);
        UserClose(curUser);
        return false;
    }

    time_t acc_time;
    time(&acc_time);

    // PPK ... check if we already have ban for this user
    if(curUser->uLogInOut->uBan != NULL && curUser->ui32NickHash == curUser->uLogInOut->uBan->ui32NickHash) {
        UserSendChar(curUser, curUser->uLogInOut->uBan->sMessage, curUser->uLogInOut->uBan->iLen);
        int imsgLen = sprintf(msg, "[SYS] Banned user %s (%s) - user closed.", curUser->sNick, curUser->sIP);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::ValidateUserNick3") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(curUser);
        return false;
    }
    
    // check for banned nicks
    BanItem *UserBan = hashBanManager->FindNick(curUser);
    if(UserBan != NULL) {
        int imsgLen;
        char *messg = GenerateBanMessage(UserBan, imsgLen, acc_time);
        UserSendChar(curUser, messg, imsgLen);
        imsgLen = sprintf(msg, "[SYS] Banned user %s (%s) - user closed.", curUser->sNick, curUser->sIP);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::ValidateUserNick4") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(curUser);
        return false;
    }

    // Nick is ok, check for registered nick
    RegUser *Reg = hashRegManager->Find(curUser);
    if(Reg != NULL) {
        if(SettingManager->bBools[SETBOOL_ADVANCED_PASS_PROTECTION] == true && Reg->iBadPassCount != 0) {
            time_t acc_time;
            time(&acc_time);

#ifdef _WIN32
			uint32_t iMinutes2Wait = (uint32_t)pow((float)2, Reg->iBadPassCount-1);
#else
			uint32_t iMinutes2Wait = (uint32_t)pow(2, Reg->iBadPassCount-1);
#endif
            if(acc_time < (time_t)(Reg->tLastBadPass+(60*iMinutes2Wait))) {
                int imsgLen = sprintf(msg, "<%s> %s %s %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_LAST_PASS_WAS_WRONG_YOU_NEED_WAIT], 
                    formatSecTime((Reg->tLastBadPass+(60*iMinutes2Wait))-acc_time), LanguageManager->sTexts[LAN_BEFORE_YOU_TRY_AGAIN]);
                if(CheckSprintf(imsgLen, 1024, "cDcCommands::ValidateUserNick5") == true) {
                    UserSendChar(curUser, msg, imsgLen);
                }

				imsgLen = sprintf(msg, "[SYS] User %s (%s) not allowed to send password (%" PRIu64 ") - user closed.", curUser->sNick, curUser->sIP, 
                    (uint64_t)((Reg->tLastBadPass+(60*iMinutes2Wait))-acc_time));
                if(CheckSprintf(imsgLen, 1024, "cDcCommands::ValidateUserNick6") == true) {
                    UdpDebug->Broadcast(msg, imsgLen);
                }
                UserClose(curUser);
                return false;
            }
        }
            
        curUser->iProfile = (int32_t)Reg->iProfile;
        UserSetPasswd(curUser, Reg->sPass);
    }

    // PPK ... moved IP ban check here, we need to allow registered users on shared IP to log in if not have banned nick, but only IP.
    if(ProfileMan->IsAllowed(curUser, ProfileManager::ENTERIFIPBAN) == false) {
        // PPK ... check if we already have ban for this user
        if(curUser->uLogInOut->uBan != NULL) {
            UserSendChar(curUser, curUser->uLogInOut->uBan->sMessage, curUser->uLogInOut->uBan->iLen);
            int imsgLen = sprintf(msg, "[SYS] uBanned user %s (%s) - user closed.", curUser->sNick, curUser->sIP);
            if(CheckSprintf(imsgLen, 1024, "cDcCommands::ValidateUserNick7") == true) {
                UdpDebug->Broadcast(msg, imsgLen);
            }
            UserClose(curUser);
            return false;
        }
    }
    
    // PPK ... delete user ban if we have it
    if(curUser->uLogInOut->uBan != NULL) {
        delete curUser->uLogInOut->uBan;
        curUser->uLogInOut->uBan = NULL;
    }
    
    // first check for user limit ! PPK ... allow hublist pinger to check hub any time ;)
    if(ProfileMan->IsAllowed(curUser, ProfileManager::ENTERFULLHUB) == false && ((curUser->ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == false) {
        // user is NOT allowed enter full hub, check for maxClients
        if(ui32Joins-ui32Parts > (uint32_t)SettingManager->iShorts[SETSHORT_MAX_USERS]) {
			int imsgLen = sprintf(msg, "$HubIsFull|<%s> %s. %u %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_THIS_HUB_IS_FULL], ui32Logged,
                LanguageManager->sTexts[LAN_USERS_ONLINE_LWR]);
            if(CheckSprintf(imsgLen, 1024, "cDcCommands::ValidateUserNick8") == false) {
                return false;
            }
            if(SettingManager->bBools[SETBOOL_REDIRECT_WHEN_HUB_FULL] == true &&
                SettingManager->sPreTexts[SetMan::SETPRETXT_REDIRECT_ADDRESS] != NULL) {
                memcpy(msg+imsgLen, SettingManager->sPreTexts[SetMan::SETPRETXT_REDIRECT_ADDRESS],
                    (size_t)SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_REDIRECT_ADDRESS]);
                imsgLen += SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_REDIRECT_ADDRESS];
                msg[imsgLen] = '\0';
            }
            UserSendChar(curUser, msg, imsgLen);
//            int imsgLen = sprintf(msg, "[SYS] Hub full for %s (%s) - user closed.", curUser->Nick, curUser->IP);
//            UdpDebug->Broadcast(msg, imsgLen);
            UserClose(curUser);
            return false;
        }
    }

    // Check for maximum connections from same IP
    if(ProfileMan->IsAllowed(curUser, ProfileManager::NOUSRSAMEIP) == false) {
        uint32_t ui32Count = hashManager->GetUserIpCount(curUser);
        if(ui32Count >= (uint32_t)SettingManager->iShorts[SETSHORT_MAX_CONN_SAME_IP]) {
            int imsgLen = sprintf(msg, "<%s> %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SORRY_ALREADY_MAX_IP_CONNS]);
            if(CheckSprintf(imsgLen, 1024, "cDcCommands::ValidateUserNick9") == false) {
                return false;
            }
            if(SettingManager->bBools[SETBOOL_REDIRECT_WHEN_HUB_FULL] == true &&
                SettingManager->sPreTexts[SetMan::SETPRETXT_REDIRECT_ADDRESS] != NULL) {
                   memcpy(msg+imsgLen, SettingManager->sPreTexts[SetMan::SETPRETXT_REDIRECT_ADDRESS],
                    (size_t)SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_REDIRECT_ADDRESS]);
                imsgLen += SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_REDIRECT_ADDRESS];
                msg[imsgLen] = '\0';
            }
            UserSendChar(curUser, msg, imsgLen);
			imsgLen = sprintf(msg, "[SYS] Max connections from same IP (%u) for %s (%s) - user closed. ", ui32Count, curUser->sNick, curUser->sIP);
            string tmp;
            if(CheckSprintf(imsgLen, 1024, "cDcCommands::ValidateUserNick10") == true) {
                //UdpDebug->Broadcast(msg, imsgLen);
                tmp = msg;
            }

            User *nxt = hashManager->FindUser(curUser->ui128IpHash);

            while(nxt != NULL) {
        		User *cur = nxt;
                nxt = cur->hashiptablenext;

                tmp += " "+string(cur->sNick, cur->ui8NickLen);
            }

            UdpDebug->Broadcast(tmp);

            UserClose(curUser);
            return false;
        }
    }

    // Check for reconnect time
    if(ProfileMan->IsAllowed(curUser, ProfileManager::NORECONNTIME) == false &&
        colUsers->CheckRecTime(curUser) == true) {
        int imsgLen = sprintf(msg, "[SYS] Fast reconnect from %s (%s) - user closed.", curUser->sNick, curUser->sIP);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::ValidateUserNick11") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(curUser);
        return false;
    }

    curUser->ui8Country = IP2Country->Find(curUser->ui128IpHash);

    // check for nick in userlist. If taken, check for dupe's socket state
    // if still active, send $ValidateDenide and close()
    User *OtherUser = hashManager->FindUser(curUser);

    if(OtherUser != NULL) {
   	    if(OtherUser->ui8State < User::STATE_CLOSING) {
            // check for socket error, or if user closed connection
            int iRet = recv(OtherUser->Sck, msg, 16, MSG_PEEK);

            // if socket error or user closed connection then allow new user to log in
#ifdef _WIN32
            if((iRet == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) || iRet == 0) {
#else
			if((iRet == -1 && errno != EAGAIN) || iRet == 0) {
#endif
                OtherUser->ui32BoolBits |= User::BIT_ERROR;
                int imsgLen = sprintf(msg, "[SYS] Ghost in validate nick %s (%s) - user closed.", OtherUser->sNick, OtherUser->sIP);
                if(CheckSprintf(imsgLen, 1024, "cDcCommands::ValidateUserNick13") == true) {
                    UdpDebug->Broadcast(msg, imsgLen);
                }
                UserClose(OtherUser);
                return false;
            } else {
                if(curUser->iProfile == -1) {
           	        int imsgLen = sprintf(msg, "$ValidateDenide %s|", Nick);
           	        if(CheckSprintf(imsgLen, 1024, "cDcCommands::ValidateUserNick14") == true) {
                        UserSendChar(curUser, msg, imsgLen);
                    }

                    if(strcmp(OtherUser->sIP, curUser->sIP) != 0 || strcmp(OtherUser->sNick, curUser->sNick) != 0) {
                        imsgLen = sprintf(msg, "[SYS] Nick taken [%s (%s)] %s (%s) - user closed.", OtherUser->sNick, OtherUser->sIP, curUser->sNick, curUser->sIP);
                        if(CheckSprintf(imsgLen, 1024, "cDcCommands::ValidateUserNick15") == true) {
                            UdpDebug->Broadcast(msg, imsgLen);
                        }
                    }

                    UserClose(curUser);
                    return false;
                } else {
                    // PPK ... addition for registered users, kill your own ghost >:-]
                    curUser->ui8State = User::STATE_VERSION_OR_MYPASS;
                    UserAddPrcsdCmd(curUser, PrcsdUsrCmd::GETPASS, NULL, 0, NULL);
                    return true;
                }
            }
        }
    }
        
    if(curUser->iProfile == -1) {
        // user is NOT registered
        
        // nick length check
        if((SettingManager->iShorts[SETSHORT_MIN_NICK_LEN] != 0 && szNickLen < (uint32_t)SettingManager->iShorts[SETSHORT_MIN_NICK_LEN]) ||
            (SettingManager->iShorts[SETSHORT_MAX_NICK_LEN] != 0 && szNickLen > (uint32_t)SettingManager->iShorts[SETSHORT_MAX_NICK_LEN])) {
            UserSendChar(curUser, SettingManager->sPreTexts[SetMan::SETPRETXT_NICK_LIMIT_MSG],
                SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_NICK_LIMIT_MSG]);
            int imsgLen = sprintf(msg, "[SYS] Bad nick length (%d) from %s (%s) - user closed.", (int)szNickLen, curUser->sNick, curUser->sIP);
            if(CheckSprintf(imsgLen, 1024, "cDcCommands::ValidateUserNick16") == true) {
                UdpDebug->Broadcast(msg, imsgLen);
            }
            UserClose(curUser);
            return false;
        }

        if(SettingManager->bBools[SETBOOL_REG_ONLY] == true && ((curUser->ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == false) {
            UserSendChar(curUser, SettingManager->sPreTexts[SetMan::SETPRETXT_REG_ONLY_MSG],
                SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_REG_ONLY_MSG]);
//            imsgLen = sprintf(msg, "[SYS] Hub for reg only %s (%s) - user closed.", curUser->Nick, curUser->IP);
//            UdpDebug->Broadcast(msg, imsgLen);
            UserClose(curUser);
            return false;
        } else {
       	    // hub is public, proceed to Hello
            if(hashManager->Add(curUser) == false) {
                return false;
            }

            curUser->ui32BoolBits |= User::BIT_HASHED;

            if(ValidateNick == true) {
                curUser->ui8State = User::STATE_VERSION_OR_MYPASS; // waiting for $Version
                UserAddPrcsdCmd(curUser, PrcsdUsrCmd::LOGINHELLO, NULL, 0, NULL);
            }
            return true;
        }
    } else {
        // user is registered, wait for password
        if(hashManager->Add(curUser) == false) {
            return false;
        }

        curUser->ui32BoolBits |= User::BIT_HASHED;
        curUser->ui8State = User::STATE_VERSION_OR_MYPASS;
        UserAddPrcsdCmd(curUser, PrcsdUsrCmd::GETPASS, NULL, 0, NULL);
        return true;
    }
}
//---------------------------------------------------------------------------

PassBf * cDcCommands::Find(const uint8_t * ui128IpHash) {
	PassBf *next = PasswdBfCheck;

    while(next != NULL) {
        PassBf *cur = next;
        next = cur->next;
		if(memcmp(cur->ui128IpHash, ui128IpHash, 16) == 0) {
            return cur;
        }
    }
    return NULL;
}
//---------------------------------------------------------------------------

void cDcCommands::Remove(PassBf * PassBfItem) {
    if(PassBfItem->prev == NULL) {
        if(PassBfItem->next == NULL) {
            PasswdBfCheck = NULL;
        } else {
            PassBfItem->next->prev = NULL;
            PasswdBfCheck = PassBfItem->next;
        }
    } else if(PassBfItem->next == NULL) {
        PassBfItem->prev->next = NULL;
    } else {
        PassBfItem->prev->next = PassBfItem->next;
        PassBfItem->next->prev = PassBfItem->prev;
    }
	delete PassBfItem;
}
//---------------------------------------------------------------------------

void cDcCommands::ProcessCmds(User * curUser) {
    PrcsdUsrCmd *next = curUser->cmdStrt;
    
    while(next != NULL) {
        PrcsdUsrCmd *cur = next;
        next = cur->next;
        switch(cur->cType) {
            case PrcsdUsrCmd::SUPPORTS: {
                memcpy(msg, "$Supports", 9);
                uint32_t iSupportsLen = 9;
                
                // PPK ... why to send it if client don't need it =)
                /*if(((curUser->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == true) {
                    memcpy(msg+iSupportsLen, " ZPipe", 6);
                    iSupportsLen += 6;
                }*/
                
                // PPK ... yes yes yes finally QuickList support in PtokaX !!! ;))
                if((curUser->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) {
                    memcpy(msg+iSupportsLen, " QuickList", 10);
                    iSupportsLen += 10;
                } else if((curUser->ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) {
                    // PPK ... Hmmm Client not really need it, but for now send it ;-)
                    memcpy(msg+iSupportsLen, " NoHello", 8);
                    iSupportsLen += 8;
                } else if((curUser->ui32SupportBits & User::SUPPORTBIT_NOGETINFO) == User::SUPPORTBIT_NOGETINFO) {
                    // PPK ... if client support NoHello automatically supports NoGetINFO another badwith wasting !
                    memcpy(msg+iSupportsLen, " NoGetINFO", 10);
                    iSupportsLen += 10;
                }
            
                if((curUser->ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) {
                    memcpy(msg+iSupportsLen, " BotINFO HubINFO", 16);
                    iSupportsLen += 16;
                }

                if((curUser->ui32SupportBits & User::SUPPORTBIT_IP64) == User::SUPPORTBIT_IP64) {
                    memcpy(msg+iSupportsLen, " IP64", 5);
                    iSupportsLen += 5;
                }

                if(((curUser->ui32SupportBits & User::SUPPORTBIT_IPV4) == User::SUPPORTBIT_IPV4) && ((curUser->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6)) {
                    // Only client connected with IPv6 sending this, so only that client is getting reply
                    memcpy(msg+iSupportsLen, " IPv4", 5);
                    iSupportsLen += 5;
                }

                memcpy(msg+iSupportsLen, "|\0", 2);
                UserSendCharDelayed(curUser, msg, iSupportsLen+1);
                break;
            }
            case PrcsdUsrCmd::LOGINHELLO: {
                int imsgLen = sprintf(msg, "$Hello %s|", curUser->sNick);
                if(CheckSprintf(imsgLen, 1024, "cDcCommands::ProcessCmds1") == true) {
                    UserSendCharDelayed(curUser, msg, imsgLen);
                }
                break;
            }
            case PrcsdUsrCmd::GETPASS: {
                uint32_t iLen = 9;
                UserSendCharDelayed(curUser, "$GetPass|", iLen); // query user for password
                break;
            }
            case PrcsdUsrCmd::CHAT: {
            	// find chat message data
                char *sBuff = cur->sCommand+curUser->ui8NickLen+3;

            	// non-command chat msg
                bool bNonChat = false;
            	for(uint8_t ui8i = 0; ui8i < (uint8_t)SettingManager->ui16TextsLens[SETTXT_CHAT_COMMANDS_PREFIXES]; ui8i++) {
            	    if(sBuff[0] == SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][ui8i] ) {
                        bNonChat = true;
                	    break;
                    }
            	}

                if(bNonChat == true) {
                    // text files...
                    if(SettingManager->bBools[SETBOOL_ENABLE_TEXT_FILES] == true) {
                        cur->sCommand[cur->iLen-1] = '\0'; // get rid of the pipe
                        
                        if(TextFileManager->ProcessTextFilesCmd(curUser, sBuff+1)) {
                            break;
                        }
    
                        cur->sCommand[cur->iLen-1] = '|'; // add back pipe
                    }
    
                    // built-in commands
                    if(cur->iLen-curUser->ui8NickLen >= 9) {
                        if(HubCmds->DoCommand(curUser, sBuff-(curUser->ui8NickLen-1), cur->iLen)) break;
                        
                        cur->sCommand[cur->iLen-1] = '|'; // add back pipe
                    }
                }
           
            	// everything's ok, let's chat
            	colUsers->SendChat2All(curUser, cur->sCommand, cur->iLen, cur->ptr);
            
                break;
            }
            case PrcsdUsrCmd::TO_OP_CHAT: {
                g_GlobalDataQueue->SingleItemStore(cur->sCommand, cur->iLen, curUser, 0, GlobalDataQueue::SI_OPCHAT);
                break;
            }
        }

#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->sCommand) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate cur->sCommand in cDcCommands::ProcessCmds\n", 0);
        }
#else
		free(cur->sCommand);
#endif
        cur->sCommand = NULL;

        delete cur;
    }
    
    curUser->cmdStrt = NULL;
    curUser->cmdEnd = NULL;

    if((curUser->ui32BoolBits & User::BIT_PRCSD_MYINFO) == User::BIT_PRCSD_MYINFO) {
        curUser->ui32BoolBits &= ~User::BIT_PRCSD_MYINFO;

        if(((curUser->ui32BoolBits & User::BIT_HAVE_BADTAG) == User::BIT_HAVE_BADTAG) == true) {
            UserHasSuspiciousTag(curUser);
        }

        if(SettingManager->ui8FullMyINFOOption == 0) {
            if(UserGenerateMyInfoLong(curUser) == false) {
                return;
            }

            colUsers->Add2MyInfosTag(curUser);

            if(SettingManager->iShorts[SETSHORT_MYINFO_DELAY] == 0 || ui64ActualTick > ((60*SettingManager->iShorts[SETSHORT_MYINFO_DELAY])+curUser->iLastMyINFOSendTick)) {
				g_GlobalDataQueue->AddQueueItem(curUser->sMyInfoLong, curUser->ui16MyInfoLongLen, NULL, 0, GlobalDataQueue::CMD_MYINFO);
                curUser->iLastMyINFOSendTick = ui64ActualTick;
            } else {
				g_GlobalDataQueue->AddQueueItem(curUser->sMyInfoLong, curUser->ui16MyInfoLongLen, NULL, 0, GlobalDataQueue::CMD_OPS);
            }

            return;
        } else if(SettingManager->ui8FullMyINFOOption == 2) {
            if(UserGenerateMyInfoShort(curUser) == false) {
                return;
            }

            colUsers->Add2MyInfos(curUser);

            if(SettingManager->iShorts[SETSHORT_MYINFO_DELAY] == 0 || ui64ActualTick > ((60*SettingManager->iShorts[SETSHORT_MYINFO_DELAY])+curUser->iLastMyINFOSendTick)) {
				g_GlobalDataQueue->AddQueueItem(curUser->sMyInfoShort, curUser->ui16MyInfoShortLen, NULL, 0, GlobalDataQueue::CMD_MYINFO);
                curUser->iLastMyINFOSendTick = ui64ActualTick;
            } else {
				g_GlobalDataQueue->AddQueueItem(curUser->sMyInfoShort, curUser->ui16MyInfoShortLen, NULL, 0, GlobalDataQueue::CMD_OPS);
            }

            return;
		}

		if(UserGenerateMyInfoLong(curUser) == false) {
			return;
		}

		colUsers->Add2MyInfosTag(curUser);

		char * sShortMyINFO = NULL;
		size_t szShortMyINFOLen = 0;

		if(UserGenerateMyInfoShort(curUser) == true) {
			colUsers->Add2MyInfos(curUser);

			if(SettingManager->iShorts[SETSHORT_MYINFO_DELAY] == 0 || ui64ActualTick > ((60*SettingManager->iShorts[SETSHORT_MYINFO_DELAY])+curUser->iLastMyINFOSendTick)) {
				sShortMyINFO = curUser->sMyInfoShort;
				szShortMyINFOLen = curUser->ui16MyInfoShortLen;
				curUser->iLastMyINFOSendTick = ui64ActualTick;
			}
		}

		g_GlobalDataQueue->AddQueueItem(sShortMyINFO, szShortMyINFOLen, curUser->sMyInfoLong, curUser->ui16MyInfoLongLen, GlobalDataQueue::CMD_MYINFO);
    }
}
//---------------------------------------------------------------------------

bool cDcCommands::CheckIP(const User * curUser, const char * sIP) {
    if((curUser->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
        if(sIP[0] == '[' && sIP[1+curUser->ui8IpLen] == ']' && sIP[2+curUser->ui8IpLen] == ':' && strncmp(sIP+1, curUser->sIP, curUser->ui8IpLen) == 0) {
            return true;
        } else if(((curUser->ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4) && sIP[curUser->ui8IPv4Len] == ':' && strncmp(sIP, curUser->sIPv4, curUser->ui8IPv4Len) == 0) {
            return true;
        }
    } else if(sIP[curUser->ui8IpLen] == ':' && strncmp(sIP, curUser->sIP, curUser->ui8IpLen) == 0) {
        return true;
    }

    return false;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

char * cDcCommands::GetPort(char * sData, char cPortEnd, size_t &szPortLen) {
    char * sPortEnd = strchr(sData, cPortEnd);
    if(sPortEnd == NULL) {
        return NULL;
    }

    sPortEnd[0] = '\0';

    char * sPortStart = strrchr(sData, ':');
    if(sPortStart == NULL || sPortStart[1] == '\0') {
        sPortEnd[0] = cPortEnd;
        return NULL;
    }

    sPortStart[0] = '\0';
    sPortStart++;
    szPortLen = (sPortEnd-sPortStart);

    int iPort = atoi(sPortStart);
    if(iPort < 1 || iPort > 65535) {
        *(sPortStart-1) = ':';
        sPortEnd[0] = cPortEnd;
        return NULL;
    }

    return sPortStart;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void cDcCommands::SendIncorrectIPMsg(User * curUser, char * sBadIP, const bool &bCTM) {
	int imsgLen = sprintf(msg, "<%s> %s ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOUR_CLIENT_SEND_INCORRECT_IP]);
	if(CheckSprintf(imsgLen, 1024, "SendIncorrectIPMsg1") == false) {
		return;
	}

    if((curUser->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
        uint8_t ui8i = 1;
        while(sBadIP[ui8i] != '\0') {
            if(isxdigit(sBadIP[ui8i]) == false && sBadIP[ui8i] != ':') {
                if(ui8i == 0) {
                    imsgLen--;
                }

                break;
            }

            msg[imsgLen] = sBadIP[ui8i];
            imsgLen++;

            ui8i++;
        }
    } else {
        uint8_t ui8i = 0;
        while(sBadIP[ui8i] != '\0') {
            if(isdigit(sBadIP[ui8i]) == false && sBadIP[ui8i] != '.') {
                if(ui8i == 0) {
                    imsgLen--;
                }

                break;
            }

            msg[imsgLen] = sBadIP[ui8i];
            imsgLen++;

            ui8i++;
        }
    }

	int iret = sprintf(msg+imsgLen, " %s %s.|", bCTM == true ? LanguageManager->sTexts[LAN_IN_CTM_REQ_REAL_IP_IS] : LanguageManager->sTexts[LAN_IN_SEARCH_REQ_REAL_IP_IS], curUser->sIP);
	imsgLen += iret;
	if(CheckSprintf1(iret, imsgLen, 1024, "SendIncorrectIPMsg2") == true) {
		UserSendCharDelayed(curUser, msg, imsgLen);
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void cDcCommands::SendIPFixedMsg(User * pUser, char * sBadIP, char * sRealIP) {
    if((pUser->ui32BoolBits & User::BIT_WARNED_WRONG_IP) == User::BIT_WARNED_WRONG_IP) {
        return;
    }

    int imsgLen = sprintf(g_sBuffer, "<%s> %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOUR_CLIENT_SEND_INCORRECT_IP], sBadIP,
        LanguageManager->sTexts[LAN_IN_COMMAND_HUB_REPLACED_IT_WITH_YOUR_REAL_IP], sRealIP);
    if(CheckSprintf(imsgLen, g_szBufferSize, "SendIncorrectIPMsg1") == true) {
        UserSendCharDelayed(pUser, g_sBuffer, imsgLen);
    }

    pUser->ui32BoolBits |= User::BIT_WARNED_WRONG_IP;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void cDcCommands::MyNick(User * pUser, char * sData, const uint32_t &ui32Len) {
    if((pUser->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
        int imsgLen = sprintf(msg, "[SYS] IPv6 $MyNick (%s) from %s (%s) - user closed.", sData, pUser->sNick, pUser->sIP);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::MyNick") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }

        Unknown(pUser, sData, ui32Len);
        return;
    }

    if(ui32Len < 10) {
        int imsgLen = sprintf(msg, "[SYS] Short $MyNick (%s) from %s (%s) - user closed.", sData, pUser->sNick, pUser->sIP);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::MyNick1") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }

        Unknown(pUser, sData, ui32Len);
        return;
    }

    sData[ui32Len-1] = '\0'; // cutoff pipe

    User * pOtherUser = hashManager->FindUser(sData+8, ui32Len-9);

    if(pOtherUser == NULL || pOtherUser->ui8State != User::STATE_IPV4_CHECK) {
        int imsgLen = sprintf(msg, "[SYS] Bad $MyNick (%s) from %s (%s) - user closed.", sData, pUser->sNick, pUser->sIP);
        if(CheckSprintf(imsgLen, 1024, "cDcCommands::MyNick2") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }

        Unknown(pUser, sData, ui32Len);
        return;
    }

    strcpy(pOtherUser->sIPv4, pUser->sIP);
    pOtherUser->ui8IPv4Len = pUser->ui8IpLen;
    pOtherUser->ui32BoolBits |= User::BIT_IPV4;

    pOtherUser->ui8State = User::STATE_ADDME;

    UserClose(pUser);
/*
	int imsgLen = sprintf(msg, "<%s> Found IPv4: %s =)|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], pOtherUser->sIPv4);
	if(CheckSprintf(imsgLen, 1024, "cDcCommands::MyNick3") == true) {
        UserSendCharDelayed(pOtherUser, msg, imsgLen);
	}
*/
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

PrcsdUsrCmd * cDcCommands::AddSearch(User * pUser, PrcsdUsrCmd * cmdSearch, char * sSearch, const size_t &szLen, const bool &bActive) const {
    if(cmdSearch != NULL) {
        char * pOldBuf = cmdSearch->sCommand;
#ifdef _WIN32
        cmdSearch->sCommand = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, cmdSearch->iLen+szLen+1);
#else
		cmdSearch->sCommand = (char *)realloc(pOldBuf, cmdSearch->iLen+szLen+1);
#endif
        if(cmdSearch->sCommand == NULL) {
            cmdSearch->sCommand = pOldBuf;
            pUser->ui32BoolBits |= User::BIT_ERROR;
            UserClose(pUser);

			AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes for cDcCommands::AddSearch1\n", (uint64_t)(cmdSearch->iLen+szLen+1));

            return cmdSearch;
        }
        memcpy(cmdSearch->sCommand+cmdSearch->iLen, sSearch, szLen);
        cmdSearch->iLen += (uint32_t)szLen;
        cmdSearch->sCommand[cmdSearch->iLen] = '\0';

        if(bActive == true) {
            colUsers->ui16ActSearchs++;
        } else {
            colUsers->ui16PasSearchs++;
        }
    } else {
        cmdSearch = new PrcsdUsrCmd();
        if(cmdSearch == NULL) {
            pUser->ui32BoolBits |= User::BIT_ERROR;
            UserClose(pUser);

			AppendDebugLog("%s - [MEM] Cannot allocate new cmdSearch in cDcCommands::AddSearch1\n", 0);
            return cmdSearch;
        }

#ifdef _WIN32
        cmdSearch->sCommand = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szLen+1);
#else
		cmdSearch->sCommand = (char *)malloc(szLen+1);
#endif
		if(cmdSearch->sCommand == NULL) {
            delete cmdSearch;
            cmdSearch = NULL;

            pUser->ui32BoolBits |= User::BIT_ERROR;
            UserClose(pUser);

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for DcCommands::Search5\n", (uint64_t)(szLen+1));

            return cmdSearch;
        }

        memcpy(cmdSearch->sCommand, sSearch, szLen);
        cmdSearch->sCommand[szLen] = '\0';

        cmdSearch->iLen = (uint32_t)szLen;

        if(bActive == true) {
            colUsers->ui16ActSearchs++;
        } else {
            colUsers->ui16PasSearchs++;
        }
    }

    return cmdSearch;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
