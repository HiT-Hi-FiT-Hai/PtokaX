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
clsDcCommands * clsDcCommands::mPtr = NULL;
//---------------------------------------------------------------------------

clsDcCommands::PassBf::PassBf(const uint8_t * ui128Hash) : iCount(1), pPrev (NULL), pNext(NULL) {
	memcpy(ui128IpHash, ui128Hash, 16);
}
//---------------------------------------------------------------------------

clsDcCommands::clsDcCommands() : PasswdBfCheck(NULL), iStatChat(0), iStatCmdUnknown(0), iStatCmdTo(0), iStatCmdMyInfo(0), iStatCmdSearch(0), iStatCmdSR(0), iStatCmdRevCTM(0), iStatCmdOpForceMove(0), iStatCmdMyPass(0), iStatCmdValidate(0), iStatCmdKey(0), iStatCmdGetInfo(0), iStatCmdGetNickList(0), iStatCmdConnectToMe(0), 
	iStatCmdVersion(0), iStatCmdKick(0), iStatCmdSupports(0), iStatBotINFO(0), iStatZPipe(0), iStatCmdMultiSearch(0), iStatCmdMultiConnectToMe(0), iStatCmdClose(0) {
	// ...
}
//---------------------------------------------------------------------------

clsDcCommands::~clsDcCommands() {
	PassBf * cur = NULL,
        * next = PasswdBfCheck;

    while(next != NULL) {
        cur = next;
		next = cur->pNext;

		delete cur;
    }

	PasswdBfCheck = NULL;
}
//---------------------------------------------------------------------------

// Process DC data form User
void clsDcCommands::PreProcessData(User * pUser, char * sData, const bool &bCheck, const uint32_t &ui32Len) {
#ifdef _BUILD_GUI
    // Full raw data trace for better logging
    if(::SendMessage(clsMainWindowPageUsersChat::mPtr->hWndPageItems[clsMainWindowPageUsersChat::BTN_SHOW_COMMANDS], BM_GETCHECK, 0, 0) == BST_CHECKED) {
		int iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "%s (%s) > ", pUser->sNick, pUser->sIP);
		if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsDcCommands::PreProcessData") == true) {
			RichEditAppendText(clsMainWindowPageUsersChat::mPtr->hWndPageItems[clsMainWindowPageUsersChat::REDT_CHAT], (clsServerManager::pGlobalBuffer+string(sData, ui32Len)).c_str());
		}
    }
#endif

    // micro spam
    if(ui32Len < 5 && bCheck) {
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Garbage DATA from %s (%s) -> %s", pUser->sNick, pUser->sIP, sData);

        pUser->Close();
        return;
    }

    static const uint32_t ui32ulti = *((uint32_t *)"ulti");
    static const uint32_t ui32ick = *((uint32_t *)"ick ");

    switch(pUser->ui8State) {
    	case User::STATE_SOCKET_ACCEPTED:
            if(sData[0] == '$') {
                if(memcmp(sData+1, "MyNick ", 7) == 0) {
                    MyNick(pUser, sData, ui32Len);
                    return;
                }
            }
            break;
        case User::STATE_KEY_OR_SUP:
            if(sData[0] == '$') {
                if(memcmp(sData+1, "Supports ", 9) == 0) {
                    iStatCmdSupports++;
                    Supports(pUser, sData, ui32Len);
                    return;
                } else if(*((uint32_t *)(sData+1)) == *((uint32_t *)"Key ")) {
					iStatCmdKey++;
                    Key(pUser, sData, ui32Len);
                    return;
                } else if(memcmp(sData+1, "MyNick ", 7) == 0) {
                    MyNick(pUser, sData, ui32Len);
                    return;
                }
            }
            break;
        case User::STATE_VALIDATE: {
            if(sData[0] == '$') {
                switch(sData[1]) {
                    case 'K':
                        if(memcmp(sData+2, "ey ", 3) == 0) {
                            iStatCmdKey++;
                            if(((pUser->ui32BoolBits & User::BIT_HAVE_SUPPORTS) == User::BIT_HAVE_SUPPORTS) == false) {
                                Key(pUser, sData, ui32Len);
                            } else {
                                pUser->FreeBuffer();
                            }

                            return;
                        }
                        break;
                    case 'V':
                        if(memcmp(sData+2, "alidateNick ", 12) == 0) {
                            iStatCmdValidate++;
                            ValidateNick(pUser, sData, ui32Len);
                            return;
                        }
                        break;
                    case 'M':
                        if(memcmp(sData+2, "yINFO $ALL ", 11) == 0) {
                            iStatCmdMyInfo++;
                            if(((pUser->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == false) {
                                // bad state
								clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad state (%d) in $MyINFO %s (%s) - user closed.", (int)pUser->ui8State, pUser->sNick, pUser->sIP);

                                pUser->Close();
                                return;
                            }
                            
                            if(MyINFODeflood(pUser, sData, ui32Len, bCheck) == false) {
                                return;
                            }
                            
                            // PPK [ Strikes back ;) ] ... get nick from MyINFO
                            char *cTemp;
                            if((cTemp = strchr(sData+13, ' ')) == NULL) {
                                if(ui32Len > 65000) {
                                    sData[65000] = '\0';
                                }

								clsUdpDebug::mPtr->BroadcastFormat("[SYS] Attempt to validate empty nick  from %s (%s) - user closed. (QuickList -> %s)", pUser->sNick, pUser->sIP, sData);

                                pUser->Close();
                                return;
                            }
                            // PPK ... one null please :)
                            cTemp[0] = '\0';
                            
                            if(ValidateUserNick(pUser, sData+13, (cTemp-sData)-13, false) == false) return;
                            
                            cTemp[0] = ' ';

                            // 1st time MyINFO, user is being added to nicklist
                            if(MyINFO(pUser, sData, ui32Len) == false || (pUser->ui32BoolBits & User::BIT_WAITING_FOR_PASS) == User::BIT_WAITING_FOR_PASS ||
                                ((pUser->ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == true)
                                return;

                            pUser->AddMeOrIPv4Check();

                            return;
                        }
                        break;
                    case 'G':
                        if(ui32Len == 13 && memcmp(sData+2, "etNickList", 10) == 0) {
                            iStatCmdGetNickList++;
                            if(((pUser->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == false &&
                                ((pUser->ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == false) {
                                // bad state
								clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad state (%d) in $GetNickList %s (%s) - user closed.", (int)pUser->ui8State, pUser->sNick, pUser->sIP);

								pUser->Close();
                                return;
                            }
                            GetNickList(pUser, sData, ui32Len, bCheck);
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
                            Version(pUser, sData, ui32Len);
                            return;
                        }
                        break;
                    case 'G':
                        if(ui32Len == 13 && memcmp(sData+2, "etNickList", 10) == 0) {
                            iStatCmdGetNickList++;
                            if(GetNickList(pUser, sData, ui32Len, bCheck) == true && ((pUser->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == false) {
                        	   pUser->ui32BoolBits |= User::BIT_GETNICKLIST;
                            }
                            return;
                        }
                        break;
                    case 'M':
                        if(sData[2] == 'y') {
                            if(memcmp(sData+3, "INFO $ALL ", 10) == 0) {
                                iStatCmdMyInfo++;
                                if(MyINFODeflood(pUser, sData, ui32Len, bCheck) == false) {
                                    return;
                                }
                                
                                // Am I sending MyINFO of someone other ?
                                // OR i try to fuck up hub with some chars after my nick ??? ... PPK
                                if((sData[13+pUser->ui8NickLen] != ' ') || (memcmp(pUser->sNick, sData+13, pUser->ui8NickLen) != 0)) {
                                    if(ui32Len > 65000) {
                                        sData[65000] = '\0';
                                    }

									clsUdpDebug::mPtr->BroadcastFormat("[SYS] Nick spoofing in myinfo from %s (%s) - user closed. (%s)", pUser->sNick, pUser->sIP, sData);

                                    pUser->Close();
                                    return;
                                }
        
                                if(MyINFO(pUser, sData, ui32Len) == false || (pUser->ui32BoolBits & User::BIT_WAITING_FOR_PASS) == User::BIT_WAITING_FOR_PASS || ((pUser->ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == true) {
                                    return;
								}
                                
                                pUser->AddMeOrIPv4Check();

                                return;
                            } else if(memcmp(sData+3, "Pass ", 5) == 0) {
                                iStatCmdMyPass++;
                                MyPass(pUser, sData, ui32Len);
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
                if(ui32Len == 13 && memcmp(sData+1, "GetNickList", 11) == 0) {
                    iStatCmdGetNickList++;
                    if(GetNickList(pUser, sData, ui32Len, bCheck) == true) {
                        pUser->ui32BoolBits |= User::BIT_GETNICKLIST;
                    }
                    return;
                } else if(memcmp(sData+1, "MyINFO $ALL ", 12) == 0) {
                    iStatCmdMyInfo++;
                    if(MyINFODeflood(pUser, sData, ui32Len, bCheck) == false) {
                        return;
                    }
                                
                    // Am I sending MyINFO of someone other ?
                    // OR i try to fuck up hub with some chars after my nick ??? ... PPK
                    if((sData[13+pUser->ui8NickLen] != ' ') || (memcmp(pUser->sNick, sData+13, pUser->ui8NickLen) != 0)) {
                        if(ui32Len > 65000) {
                            sData[65000] = '\0';
                        }

						clsUdpDebug::mPtr->BroadcastFormat("[SYS] Nick spoofing in myinfo from %s (%s) - user closed. (%s)", pUser->sNick, pUser->sIP, sData);

                        pUser->Close();
                        return;
                    }
        
                    if(MyINFO(pUser, sData, ui32Len) == false || (pUser->ui32BoolBits & User::BIT_WAITING_FOR_PASS) == User::BIT_WAITING_FOR_PASS ||
                        ((pUser->ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == true)
                        return;
                    
                    pUser->AddMeOrIPv4Check();

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
                        if(ui32Len == 13 && memcmp(sData+2, "etNickList", 10) == 0) {
                            iStatCmdGetNickList++;
                            if(GetNickList(pUser, sData, ui32Len, bCheck) == true) {
                                pUser->ui32BoolBits |= User::BIT_GETNICKLIST;
                            }
                            return;
                        }
                        break;
                    case 'M': {
                        if(memcmp(sData+2, "yINFO $ALL ", 11) == 0) {
                            iStatCmdMyInfo++;
                            if(MyINFODeflood(pUser, sData, ui32Len, bCheck) == false) {
                                return;
                            }

                            // Am I sending MyINFO of someone other ?
                            // OR i try to fuck up hub with some chars after my nick ??? ... PPK
                            if((sData[13+pUser->ui8NickLen] != ' ') || (memcmp(pUser->sNick, sData+13, pUser->ui8NickLen) != 0)) {
                                if(ui32Len > 65000) {
                                    sData[65000] = '\0';
                                }

								clsUdpDebug::mPtr->BroadcastFormat("[SYS] Nick spoofing in myinfo from %s (%s) - user closed. (%s)", pUser->sNick, pUser->sIP, sData);

                                pUser->Close();
                                return;
                            }

                            MyINFO(pUser, sData, ui32Len);
                            
                            return;
                        } else if(memcmp(sData+2, "ultiSearch ", 11) == 0) {
                            iStatCmdMultiSearch++;
                            SearchDeflood(pUser, sData, ui32Len, bCheck, true);
                            return;
                        }
                        break;
                    }
                    case 'S':
                        if(memcmp(sData+2, "earch ", 6) == 0) {
                            iStatCmdSearch++;
                            SearchDeflood(pUser, sData, ui32Len, bCheck, false);
                            return;
                        }
                        break;
                    default:
                        break;
                }
            } else if(sData[0] == '<') {
                iStatChat++;
                ChatDeflood(pUser, sData, ui32Len, bCheck);
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
                            if(SearchDeflood(pUser, sData, ui32Len, bCheck, false) == true) {
                                Search(pUser, sData, ui32Len, bCheck, false);
                            }
                            return;
                        } else if(*((uint16_t *)(sData+2)) == *((uint16_t *)"R ")) {
                            iStatCmdSR++;
                            SR(pUser, sData, ui32Len, bCheck);
                            return;
                        }
                        break;
                    }
                    case 'C':
                        if(memcmp(sData+2, "onnectToMe ", 11) == 0) {
                            iStatCmdConnectToMe++;
                            ConnectToMe(pUser, sData, ui32Len, bCheck, false);
                            return;
                        } else if(memcmp(sData+2, "lose ", 5) == 0) {
                            iStatCmdClose++;
                            Close(pUser, sData, ui32Len);
                            return;
                        }
                        break;
                    case 'R':
                        if(memcmp(sData+2, "evConnectToMe ", 14) == 0) {
                            iStatCmdRevCTM++;
                            RevConnectToMe(pUser, sData, ui32Len, bCheck);
                            return;
                        }
                        break;
                    case 'M':
                        if(memcmp(sData+2, "yINFO $ALL ", 11) == 0) {
                            iStatCmdMyInfo++;
                            if(MyINFODeflood(pUser, sData, ui32Len, bCheck) == false) {
                                return;
                            }
                                    
                            // Am I sending MyINFO of someone other ?
                            // OR i try to fuck up hub with some chars after my nick ??? ... PPK
                            if((sData[13+pUser->ui8NickLen] != ' ') || (memcmp(pUser->sNick, sData+13, pUser->ui8NickLen) != 0)) {
                                if(ui32Len > 65000) {
                                    sData[65000] = '\0';
                                }

								clsUdpDebug::mPtr->BroadcastFormat("[SYS] Nick spoofing in myinfo from %s (%s) - user closed. (%s)", pUser->sNick, pUser->sIP, sData);

                                pUser->Close();
                                return;
                            }
                                                  
                            if(MyINFO(pUser, sData, ui32Len) == true) {
                                pUser->ui32BoolBits |= User::BIT_PRCSD_MYINFO;
                            }
                            return;
                        } else if(*((uint32_t *)(sData+2)) == ui32ulti) {
                            if(memcmp(sData+6, "Search ", 7) == 0) {
                                iStatCmdMultiSearch++;
                                if(SearchDeflood(pUser, sData, ui32Len, bCheck, true) == true) {
                                    Search(pUser, sData, ui32Len, bCheck, true);
                                }
                                return;
                            } else if(memcmp(sData+6, "ConnectToMe ", 12) == 0) {
                                iStatCmdMultiConnectToMe++;
                                ConnectToMe(pUser, sData, ui32Len, bCheck, true);
                                return;
                            }
                        } else if(memcmp(sData+2, "yPass ", 6) == 0) {
                            iStatCmdMyPass++;
                            //MyPass(pUser, sData, ui32Len);
                            if((pUser->ui32BoolBits & User::BIT_WAITING_FOR_PASS) == User::BIT_WAITING_FOR_PASS) {
                                pUser->ui32BoolBits &= ~User::BIT_WAITING_FOR_PASS;

                                if(pUser->pLogInOut != NULL && pUser->pLogInOut->pBuffer != NULL) {
                                    int iProfile = clsProfileManager::mPtr->GetProfileIndex(pUser->pLogInOut->pBuffer);
                                    if(iProfile == -1) {
                                        pUser->SendFormat("clsDcCommands::PreProcessData::MyPass->RegUser1", true, "<%s> %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_ERR_NO_PROFILE_GIVEN_NAME_EXIST]);

                                        delete pUser->pLogInOut;
                                        pUser->pLogInOut = NULL;

                                        return;
                                    }
                                    
                                    if(ui32Len > 73) {
                                        pUser->SendFormat("clsDcCommands::PreProcessData::MyPass->RegUser2", true, "<%s> %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_MAX_ALWD_PASS_LEN_64_CHARS]);

                                        delete pUser->pLogInOut;
                                        pUser->pLogInOut = NULL;

                                        return;
                                    }

                                    sData[ui32Len-1] = '\0'; // cutoff pipe

                                    if(clsRegManager::mPtr->AddNew(pUser->sNick, sData+8, (uint16_t)iProfile) == false) {
                                        pUser->SendFormat("clsDcCommands::PreProcessData::MyPass->RegUser3", true, "<%s> %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SORRY_YOU_ARE_ALREADY_REGISTERED]);
                                    } else {
                                        pUser->SendFormat("clsDcCommands::PreProcessData::MyPass->RegUser4", true, "<%s> %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_THANK_YOU_FOR_PASSWORD_YOU_ARE_NOW_REGISTERED_AS], pUser->pLogInOut->pBuffer);
                                    }

                                    delete pUser->pLogInOut;
                                    pUser->pLogInOut = NULL;

                                    pUser->i32Profile = iProfile;

                                    if(((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                                        if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::HASKEYICON) == false) {
                                            return;
                                        }
                                        
                                        pUser->ui32BoolBits |= User::BIT_OPERATOR;

                                        clsUsers::mPtr->Add2OpList(pUser);
                                        clsGlobalDataQueue::mPtr->OpListStore(pUser->sNick);

                                        if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::ALLOWEDOPCHAT) == true) {
                                            if(clsSettingManager::mPtr->bBools[SETBOOL_REG_OP_CHAT] == true &&
                                                (clsSettingManager::mPtr->bBools[SETBOOL_REG_BOT] == false || clsSettingManager::mPtr->bBotsSameNick == false)) {
                                                if(((pUser->ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == false) {
                                                    pUser->SendCharDelayed(clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_OP_CHAT_HELLO],
                                                        clsSettingManager::mPtr->ui16PreTextsLens[clsSettingManager::SETPRETXT_OP_CHAT_HELLO]);
                                                }

                                                pUser->SendCharDelayed(clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_OP_CHAT_MYINFO], clsSettingManager::mPtr->ui16PreTextsLens[clsSettingManager::SETPRETXT_OP_CHAT_MYINFO]);
                                                pUser->SendFormat("clsDcCommands::PreProcessData::MyPass->RegUser5", true, "$OpList %s$$|", clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_NICK]);
                                            }
                                        }
                                    }
                                }

                                return;
                            }
                        }
                        break;
                    case 'G': {
                        if(ui32Len == 13 && memcmp(sData+2, "etNickList", 10) == 0) {
                            iStatCmdGetNickList++;
                            if(GetNickList(pUser, sData, ui32Len, bCheck) == true) {
                                pUser->ui32BoolBits |= User::BIT_GETNICKLIST;
                            }
                            return;
                        } else if(memcmp(sData+2, "etINFO ", 7) == 0) {
							iStatCmdGetInfo++;
                            GetINFO(pUser, sData, ui32Len);
                            return;
                        }
                        break;
                    }
                    case 'T':
                        if(memcmp(sData+2, "o: ", 3) == 0) {
                            iStatCmdTo++;
                            To(pUser, sData, ui32Len, bCheck);
                            return;
                        }
                    case 'K':
                        if(*((uint32_t *)(sData+2)) == ui32ick) {
							iStatCmdKick++;
                            Kick(pUser, sData, ui32Len);
                            return;
                        }
                        break;
                    case 'O':
                        if(memcmp(sData+2, "pForceMove $Who:", 16) == 0) {
                            iStatCmdOpForceMove++;
                            OpForceMove(pUser, sData, ui32Len);
                            return;
                        }
                    default:
                        break;
                }
            } else if(sData[0] == '<') {
                iStatChat++;
                if(ChatDeflood(pUser, sData, ui32Len, bCheck) == true) {
                    Chat(pUser, sData, ui32Len, bCheck);
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
                        BotINFO(pUser, sData, ui32Len);
                        return;
                    }
                    break;
                }
                case 'C':
                    if(memcmp(sData+2, "onnectToMe ", 11) == 0) {
                        iStatCmdConnectToMe++;

						clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad state (%d) in $ConnectToMe %s (%s) - user closed.", (int)pUser->ui8State, pUser->sNick, pUser->sIP);

                        pUser->Close();
                        return;
                    } else if(memcmp(sData+2, "lose ", 5) == 0) {
                        iStatCmdClose++;

						clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad state (%d) in $Close %s (%s) - user closed.", (int)pUser->ui8State, pUser->sNick, pUser->sIP);

                        pUser->Close();
                        return;
                    }
                    break;
                case 'G': {
                    if(*((uint16_t *)(sData+2)) == *((uint16_t *)"et")) {
                        if(memcmp(sData+4, "INFO ", 5) == 0) {
                            iStatCmdGetInfo++;

							clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad state (%d) in $GetINFO %s (%s) - user closed.", (int)pUser->ui8State, pUser->sNick, pUser->sIP);

                            pUser->Close();
                            return;
                        } else if(ui32Len == 13 && *((uint64_t *)(sData+4)) == *((uint64_t *)"NickList")) {
                            iStatCmdGetNickList++;

							clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad state (%d) in $GetNickList %s (%s) - user closed.", (int)pUser->ui8State, pUser->sNick, pUser->sIP);

                            pUser->Close();
                            return;
                        }
                    }
                    break;
                }
                case 'K':
                    if(memcmp(sData+2, "ey ", 3) == 0) {
                        iStatCmdKey++;

						clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad state (%d) in $Key %s (%s) - user closed.", (int)pUser->ui8State, pUser->sNick, pUser->sIP);

                        pUser->Close();
                        return;
                    } else if(*((uint32_t *)(sData+2)) == ui32ick) {
                        iStatCmdKick++;

						clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad state (%d) in $Kick %s (%s) - user closed.", (int)pUser->ui8State, pUser->sNick, pUser->sIP);

                        pUser->Close();
                        return;
                    }
                    break;
                case 'M':
                    if(*((uint32_t *)(sData+2)) == ui32ulti) {
                        if(memcmp(sData+6, "ConnectToMe ", 12) == 0) {
                            iStatCmdMultiConnectToMe++;

							clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad state (%d) in $MultiConnectToMe %s (%s) - user closed.", (int)pUser->ui8State, pUser->sNick, pUser->sIP);

                            pUser->Close();
                            return;
                        } else if(memcmp(sData+6, "Search ", 7) == 0) {
                            iStatCmdMultiSearch++;

							clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad state (%d) in $MultiSearch %s (%s) - user closed.", (int)pUser->ui8State, pUser->sNick, pUser->sIP);

                            pUser->Close();
                            return;
                        }
                    } else if(sData[2] == 'y') {
                        if(memcmp(sData+3, "INFO $ALL ", 10) == 0) {
                            iStatCmdMyInfo++;

							clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad state (%d) in $MyINFO %s (%s) - user closed.", (int)pUser->ui8State, pUser->sNick, pUser->sIP);

                            pUser->Close();
                            return;
                        } else if(memcmp(sData+3, "Pass ", 5) == 0) {
                            iStatCmdMyPass++;

							clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad state (%d) in $MyPass %s (%s) - user closed.", (int)pUser->ui8State, pUser->sNick, pUser->sIP);

                            pUser->Close();
                            return;
                        }
                    }
                    break;
                case 'O':
                    if(memcmp(sData+2, "pForceMove $Who:", 16) == 0) {
                        iStatCmdOpForceMove++;

						clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad state (%d) in $OpForceMove %s (%s) - user closed.", (int)pUser->ui8State, pUser->sNick, pUser->sIP);

                        pUser->Close();
                        return;
                    }
                    break;
                case 'R':
                    if(memcmp(sData+2, "evConnectToMe ", 14) == 0) {
                        iStatCmdRevCTM++;

						clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad state (%d) in $RevConnectToMe %s (%s) - user closed.", (int)pUser->ui8State, pUser->sNick, pUser->sIP);

						pUser->Close();
                        return;
                    }
                    break;
                case 'S':
                    switch(sData[2]) {
                        case 'e': {
                            if(memcmp(sData+3, "arch ", 5) == 0) {
                                iStatCmdSearch++;

								clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad state (%d) in $Search %s (%s) - user closed.", (int)pUser->ui8State, pUser->sNick, pUser->sIP);

                                pUser->Close();
                                return;
                            }
                            break;
                        }
                        case 'R':
                            if(sData[3] == ' ') {
                                iStatCmdSR++;

								clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad state (%d) in $SR %s (%s) - user closed.", (int)pUser->ui8State, pUser->sNick, pUser->sIP);

								pUser->Close();
                                return;
                            }
                            break;
                        case 'u': {
                            if(memcmp(sData+3, "pports ", 7) == 0) {
                                iStatCmdSupports++;

								clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad state (%d) in $Supports %s (%s) - user closed.", (int)pUser->ui8State, pUser->sNick, pUser->sIP);

                                pUser->Close();
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

						clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad state (%d) in $To %s (%s) - user closed.", (int)pUser->ui8State, pUser->sNick, pUser->sIP);

                        pUser->Close();
                        return;
                    }
                    break;
                case 'V':
                    if(memcmp(sData+2, "alidateNick ", 12) == 0) {
                        iStatCmdValidate++;

						clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad state (%d) in $ValidateNick %s (%s) - user closed.", (int)pUser->ui8State, pUser->sNick, pUser->sIP);

                        pUser->Close();
                        return;
                    } else if(memcmp(sData+2, "ersion ", 7) == 0) {
                        iStatCmdVersion++;

						clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad state (%d) in $Version %s (%s) - user closed.", (int)pUser->ui8State, pUser->sNick, pUser->sIP);

						pUser->Close();
                        return;
                    }
                    break;
                default:
                    break;
            }
            break;
        case '<': {
            iStatChat++;

			clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad state (%d) in Chat %s (%s) - user closed.", (int)pUser->ui8State, pUser->sNick, pUser->sIP);

            pUser->Close();
            return;
        }
        default:
            break;
    }


    Unknown(pUser, sData, ui32Len);
}
//---------------------------------------------------------------------------

// $BotINFO pinger identification|
void clsDcCommands::BotINFO(User * pUser, char * sData, const uint32_t &ui32Len) {
	if(((pUser->ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == false || ((pUser->ui32BoolBits & User::BIT_HAVE_BOTINFO) == User::BIT_HAVE_BOTINFO) == true) {
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Not pinger $BotINFO or $BotINFO flood from %s (%s)", pUser->sNick, pUser->sIP);

        pUser->Close();
        return;
    }

    if(ui32Len < 9) {
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad $BotINFO (%s) from %s (%s) - user closed.", sData, pUser->sNick, pUser->sIP);

        pUser->Close();
        return;
    }

    pUser->ui32BoolBits |= User::BIT_HAVE_BOTINFO;

	if(clsScriptManager::mPtr->Arrival(pUser, sData, ui32Len, clsScriptManager::BOTINFO_ARRIVAL) == true ||
		pUser->ui8State >= User::STATE_CLOSING) {
		return;
	}

    pUser->SendFormat("clsDcCommands::BotINFO", true, "$HubINFO %s$%s:%hu$%s.px.$%hd$%" PRIu64 "$%hd$%hd$PtokaX$%s|", clsSettingManager::mPtr->sTexts[SETTXT_HUB_NAME], clsSettingManager::mPtr->sTexts[SETTXT_HUB_ADDRESS], clsSettingManager::mPtr->ui16PortNumbers[0], 
		clsSettingManager::mPtr->sTexts[SETTXT_HUB_DESCRIPTION] == NULL ? "" : clsSettingManager::mPtr->sTexts[SETTXT_HUB_DESCRIPTION], clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_USERS], clsSettingManager::mPtr->ui64MinShare, clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_SLOTS_LIMIT], 
		clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_HUBS_LIMIT], clsSettingManager::mPtr->sTexts[SETTXT_HUB_OWNER_EMAIL] == NULL ? "" : clsSettingManager::mPtr->sTexts[SETTXT_HUB_OWNER_EMAIL]);

	if(((pUser->ui32BoolBits & User::BIT_HAVE_GETNICKLIST) == User::BIT_HAVE_GETNICKLIST) == true) {
		pUser->Close();
    }
}
//---------------------------------------------------------------------------

// $ConnectToMe <nickname> <ownip>:<ownlistenport>
// $MultiConnectToMe <nick> <ownip:port> <hub[:port]>
void clsDcCommands::ConnectToMe(User * pUser, char * sData, const uint32_t &ui32Len, const bool &bCheck, const bool &bMulti) {
    if((bMulti == false && ui32Len < 23) || (bMulti == true && ui32Len < 28)) {
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad $%sConnectToMe (%s) from %s (%s) - user closed.", bMulti == false ? "" : "Multi", sData, pUser->sNick, pUser->sIP);

        pUser->Close();
        return;
    }

    // PPK ... check flood ...
    if(bCheck == true && clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::NODEFLOODCTM) == false) { 
        if(clsSettingManager::mPtr->i16Shorts[SETSHORT_CTM_ACTION] != 0) {
            if(DeFloodCheckForFlood(pUser, DEFLOOD_CTM, clsSettingManager::mPtr->i16Shorts[SETSHORT_CTM_ACTION],
              pUser->ui16CTMs, pUser->ui64CTMsTick, clsSettingManager::mPtr->i16Shorts[SETSHORT_CTM_MESSAGES],
              (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_CTM_TIME]) == true) {
				return;
            }
        }

        if(clsSettingManager::mPtr->i16Shorts[SETSHORT_CTM_ACTION2] != 0) {
            if(DeFloodCheckForFlood(pUser, DEFLOOD_CTM, clsSettingManager::mPtr->i16Shorts[SETSHORT_CTM_ACTION2],
              pUser->ui16CTMs2, pUser->ui64CTMsTick2, clsSettingManager::mPtr->i16Shorts[SETSHORT_CTM_MESSAGES2],
			  (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_CTM_TIME2]) == true) {
                return;
            }
        }
    }

    if(ui32Len > (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_CTM_LEN]) {
        pUser->SendFormat("clsDcCommands::ConnectToMe", true, "<%s> %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_CTM_TOO_LONG]);

        if(ui32Len > 65000) {
            sData[65000] = '\0';
        }

		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Long $ConnectToMe from %s (%s) - user closed. (%s)", pUser->sNick, pUser->sIP, sData);

        pUser->Close();
		return;
    }

    // PPK ... $CTM means user is active ?!? Probably yes, let set it active and use on another places ;)
    if(pUser->sTag == NULL) {
        pUser->ui32BoolBits |= User::BIT_IPV4_ACTIVE;
    }

    // full data only and allow blocking
	if(clsScriptManager::mPtr->Arrival(pUser, sData, ui32Len, (uint8_t)(bMulti == false ? clsScriptManager::CONNECTTOME_ARRIVAL : clsScriptManager::MULTICONNECTTOME_ARRIVAL)) == true ||
		pUser->ui8State >= User::STATE_CLOSING) {
		return;
	}

    char *towho = strchr(sData+(bMulti == false ? 13 : 18), ' ');
    if(towho == NULL) {
        return;
    }

    towho[0] = '\0';

    User * pOtherUser = clsHashManager::mPtr->FindUser(sData+(bMulti == false ? 13 : 18), towho-(sData+(bMulti == false ? 13 : 18)));
    // PPK ... no connection to yourself !!!
    if(pOtherUser == NULL || pOtherUser == pUser || pOtherUser->ui8State != User::STATE_ADDED) {
        return;
    }

    towho[0] = ' ';

    uint8_t ui8AfterPortLen = 0;
    uint16_t ui16Port = 0;
    bool bWrongPort = false;
    bool bCorrectIP = CheckIPPort(pUser, towho+1, bWrongPort, ui16Port, ui8AfterPortLen, '|');

	if(bWrongPort == true) {
        SendIncorrectPortMsg(pUser, true);

        if(ui32Len > 65000) {
            sData[65000] = '\0';
        }

		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad Port in %sCTM from %s (%s). (%s)", bMulti == false ? "" : "M", pUser->sNick, pUser->sIP, sData);

        pUser->Close();
        return;
	}

    // IP check
    if(bCheck == true && clsSettingManager::mPtr->bBools[SETBOOL_CHECK_IP_IN_COMMANDS] == true && clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::NOIPCHECK) == false && bCorrectIP == false) {
        if((pUser->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6 && (pOtherUser->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
            int iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "$ConnectToMe %s [%s]:%hu|", pOtherUser->sNick, pUser->sIP, ui16Port);
            if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsDcCommands::ConnectToMe1") == true) {
                pUser->AddPrcsdCmd(PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO, clsServerManager::pGlobalBuffer, iMsgLen, pOtherUser);
            }

            char * sBadIP = towho+1;
            if(sBadIP[0] == '[') {
                sBadIP[strlen(sBadIP)-1] = '\0';
                sBadIP++;
            }

            SendIPFixedMsg(pUser, sBadIP, pUser->sIP);
            return;
        } else if((pUser->ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4 && (pOtherUser->ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4) {
            char * sIP = pUser->ui8IPv4Len == 0 ? pUser->sIP : pUser->sIPv4;

            int iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "$ConnectToMe %s %s:%hu|", pOtherUser->sNick, sIP, ui16Port);
            if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsDcCommands::ConnectToMe2") == true) {
                pUser->AddPrcsdCmd(PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO, clsServerManager::pGlobalBuffer, iMsgLen, pOtherUser);
            }

            char * sBadIP = towho+1;
            if(sBadIP[0] == '[') {
                sBadIP[strlen(sBadIP)-1] = '\0';
                sBadIP++;
            }

            SendIPFixedMsg(pUser, sBadIP, sIP);
            return;
        }

        SendIncorrectIPMsg(pUser, towho+1, true);

        if(ui32Len > 65000) {
            sData[65000] = '\0';
        }

		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad IP in %sCTM from %s (%s). (%s)", bMulti == false ? "" : "M", pUser->sNick, pUser->sIP, sData);

        pUser->Close();
        return;
    }

    if(bMulti == true) {
        sData[5] = '$';
    }

    pUser->AddPrcsdCmd(PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO, bMulti == false ? sData : sData+5, bMulti == false ? ui32Len : ui32Len-5, pOtherUser);
}
//---------------------------------------------------------------------------

// $GetINFO <nickname> <ownnickname>
void clsDcCommands::GetINFO(User * pUser, char * sData, const uint32_t &ui32Len) {
	if(((pUser->ui32SupportBits & User::SUPPORTBIT_NOGETINFO) == User::SUPPORTBIT_NOGETINFO) == true || ((pUser->ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == true || ((pUser->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == true) {
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Not allowed user %s (%s) send $GetINFO - user closed.", pUser->sNick, pUser->sIP);

        pUser->Close();
        return;
    }
    
    // PPK ... code change, added own nick and space on right place check
    if(ui32Len < (12u+pUser->ui8NickLen) || ui32Len > (75u+pUser->ui8NickLen) || sData[ui32Len-pUser->ui8NickLen-2] != ' ' ||
        memcmp(sData+(ui32Len-pUser->ui8NickLen-1), pUser->sNick, pUser->ui8NickLen) != 0) {
        if(ui32Len > 65000) {
            sData[65000] = '\0';
        }

		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad $GetINFO from %s (%s) - user closed. (%s)", pUser->sNick, pUser->sIP, sData);

        pUser->Close();
        return;
    }

	if(clsScriptManager::mPtr->Arrival(pUser, sData, ui32Len, clsScriptManager::GETINFO_ARRIVAL) == true ||
		pUser->ui8State >= User::STATE_CLOSING) {
		return;
	}
}
//---------------------------------------------------------------------------

// $GetNickList
bool clsDcCommands::GetNickList(User * pUser, char * sData, const uint32_t &ui32Len, const bool &bCheck) {
    if(((pUser->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == true &&
        ((pUser->ui32BoolBits & User::BIT_HAVE_GETNICKLIST) == User::BIT_HAVE_GETNICKLIST) == true) {
        // PPK ... refresh not allowed !
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad $GetNickList request %s (%s) - user closed.", pUser->sNick, pUser->sIP);

        pUser->Close();
        return false;
    } else if(((pUser->ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == true) {
		if(((pUser->ui32BoolBits & User::BIT_HAVE_GETNICKLIST) == User::BIT_HAVE_GETNICKLIST) == false) {
            pUser->ui32BoolBits |= User::BIT_BIG_SEND_BUFFER;
            if(((pUser->ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == false && clsUsers::mPtr->ui32NickListLen > 11) {
                if(((pUser->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false) {
                    pUser->SendCharDelayed(clsUsers::mPtr->pNickList, clsUsers::mPtr->ui32NickListLen);
                } else {
                    if(clsUsers::mPtr->ui32ZNickListLen == 0) {
                        clsUsers::mPtr->pZNickList = clsZlibUtility::mPtr->CreateZPipe(clsUsers::mPtr->pNickList, clsUsers::mPtr->ui32NickListLen, clsUsers::mPtr->pZNickList,
                            clsUsers::mPtr->ui32ZNickListLen, clsUsers::mPtr->ui32ZNickListSize, Allign16K);
                        if(clsUsers::mPtr->ui32ZNickListLen == 0) {
                            pUser->SendCharDelayed(clsUsers::mPtr->pNickList, clsUsers::mPtr->ui32NickListLen);
                        } else {
                            pUser->PutInSendBuf(clsUsers::mPtr->pZNickList, clsUsers::mPtr->ui32ZNickListLen);
                            clsServerManager::ui64BytesSentSaved += clsUsers::mPtr->ui32NickListLen-clsUsers::mPtr->ui32ZNickListLen;
                        }
                    } else {
                        pUser->PutInSendBuf(clsUsers::mPtr->pZNickList, clsUsers::mPtr->ui32ZNickListLen);
                        clsServerManager::ui64BytesSentSaved += clsUsers::mPtr->ui32NickListLen-clsUsers::mPtr->ui32ZNickListLen;
                    }
                }
            }
            
            if(clsSettingManager::mPtr->ui8FullMyINFOOption == 2) {
                if(clsUsers::mPtr->ui32MyInfosLen != 0) {
                    if(((pUser->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false) {
                        pUser->SendCharDelayed(clsUsers::mPtr->pMyInfos, clsUsers::mPtr->ui32MyInfosLen);
                    } else {
                        if(clsUsers::mPtr->ui32ZMyInfosLen == 0) {
                            clsUsers::mPtr->pZMyInfos = clsZlibUtility::mPtr->CreateZPipe(clsUsers::mPtr->pMyInfos, clsUsers::mPtr->ui32MyInfosLen, clsUsers::mPtr->pZMyInfos,
                                clsUsers::mPtr->ui32ZMyInfosLen, clsUsers::mPtr->ui32ZMyInfosSize, Allign128K);
                            if(clsUsers::mPtr->ui32ZMyInfosLen == 0) {
                                pUser->SendCharDelayed(clsUsers::mPtr->pMyInfos, clsUsers::mPtr->ui32MyInfosLen);
                            } else {
                                pUser->PutInSendBuf(clsUsers::mPtr->pZMyInfos, clsUsers::mPtr->ui32ZMyInfosLen);
                                clsServerManager::ui64BytesSentSaved += clsUsers::mPtr->ui32MyInfosLen-clsUsers::mPtr->ui32ZMyInfosLen;
                            }
                        } else {
                            pUser->PutInSendBuf(clsUsers::mPtr->pZMyInfos, clsUsers::mPtr->ui32ZMyInfosLen);
                            clsServerManager::ui64BytesSentSaved += clsUsers::mPtr->ui32MyInfosLen-clsUsers::mPtr->ui32ZMyInfosLen;
                        }
                    }
                }
            } else if(clsUsers::mPtr->ui32MyInfosTagLen != 0) {
                if(((pUser->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false) {
                    pUser->SendCharDelayed(clsUsers::mPtr->pMyInfosTag, clsUsers::mPtr->ui32MyInfosTagLen);
                } else {
                    if(clsUsers::mPtr->ui32ZMyInfosTagLen == 0) {
                        clsUsers::mPtr->pZMyInfosTag = clsZlibUtility::mPtr->CreateZPipe(clsUsers::mPtr->pMyInfosTag, clsUsers::mPtr->ui32MyInfosTagLen, clsUsers::mPtr->pZMyInfosTag,
                            clsUsers::mPtr->ui32ZMyInfosTagLen, clsUsers::mPtr->ui32ZMyInfosTagSize, Allign128K);
                        if(clsUsers::mPtr->ui32ZMyInfosTagLen == 0) {
                            pUser->SendCharDelayed(clsUsers::mPtr->pMyInfosTag, clsUsers::mPtr->ui32MyInfosTagLen);
                        } else {
                            pUser->PutInSendBuf(clsUsers::mPtr->pZMyInfosTag, clsUsers::mPtr->ui32ZMyInfosTagLen);
                            clsServerManager::ui64BytesSentSaved += clsUsers::mPtr->ui32MyInfosTagLen-clsUsers::mPtr->ui32ZMyInfosTagLen;
                        }
                    } else {
                        pUser->PutInSendBuf(clsUsers::mPtr->pZMyInfosTag, clsUsers::mPtr->ui32ZMyInfosTagLen);
                        clsServerManager::ui64BytesSentSaved += clsUsers::mPtr->ui32MyInfosTagLen-clsUsers::mPtr->ui32ZMyInfosTagLen;
                    }  
                }
            }
            
 			if(clsUsers::mPtr->ui32OpListLen > 9) {
                if(((pUser->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false) {
                    pUser->SendCharDelayed(clsUsers::mPtr->pOpList, clsUsers::mPtr->ui32OpListLen);
                } else {
                    if(clsUsers::mPtr->ui32ZOpListLen == 0) {
                        clsUsers::mPtr->pZOpList = clsZlibUtility::mPtr->CreateZPipe(clsUsers::mPtr->pOpList, clsUsers::mPtr->ui32OpListLen, clsUsers::mPtr->pZOpList,
                            clsUsers::mPtr->ui32ZOpListLen, clsUsers::mPtr->ui32ZOpListSize, Allign16K);
                        if(clsUsers::mPtr->ui32ZOpListLen == 0) {
                            pUser->SendCharDelayed(clsUsers::mPtr->pOpList, clsUsers::mPtr->ui32OpListLen);
                        } else {
                            pUser->PutInSendBuf(clsUsers::mPtr->pZOpList, clsUsers::mPtr->ui32ZOpListLen);
                            clsServerManager::ui64BytesSentSaved += clsUsers::mPtr->ui32OpListLen-clsUsers::mPtr->ui32ZOpListLen;
                        }
                    } else {
                        pUser->PutInSendBuf(clsUsers::mPtr->pZOpList, clsUsers::mPtr->ui32ZOpListLen);
                        clsServerManager::ui64BytesSentSaved += clsUsers::mPtr->ui32OpListLen-clsUsers::mPtr->ui32ZOpListLen;
                    }
                }
            }
            
 			if(pUser->ui32SendBufDataLen != 0) {
                pUser->Try2Send();
            }
 			
  			pUser->ui32BoolBits |= User::BIT_HAVE_GETNICKLIST;
  			
   			if(clsSettingManager::mPtr->bBools[SETBOOL_REPORT_PINGERS] == true) {
                clsGlobalDataQueue::mPtr->StatusMessageFormat("clsDcCommands::GetNickList", "<%s> *** %s: %s %s: %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_PINGER_FROM_IP], pUser->sIP, clsLanguageManager::mPtr->sTexts[LAN_WITH_NICK], pUser->sNick, 
					clsLanguageManager::mPtr->sTexts[LAN_DETECTED_LWR]);
			}

			if(((pUser->ui32BoolBits & User::BIT_HAVE_BOTINFO) == User::BIT_HAVE_BOTINFO) == true) {
                pUser->Close();
            }
			return false;
		} else {
			clsUdpDebug::mPtr->BroadcastFormat("[SYS] $GetNickList flood from pinger %s (%s) - user closed.", pUser->sNick, pUser->sIP);

			pUser->Close();
			return false;
		}
	}

    pUser->ui32BoolBits |= User::BIT_HAVE_GETNICKLIST;
    
     // PPK ... check flood...
    if(bCheck == true && clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::NODEFLOODGETNICKLIST) == false && 
      clsSettingManager::mPtr->i16Shorts[SETSHORT_GETNICKLIST_ACTION] != 0) {
        if(DeFloodCheckForFlood(pUser, DEFLOOD_GETNICKLIST, clsSettingManager::mPtr->i16Shorts[SETSHORT_GETNICKLIST_ACTION],
          pUser->ui16GetNickLists, pUser->ui64GetNickListsTick, clsSettingManager::mPtr->i16Shorts[SETSHORT_GETNICKLIST_MESSAGES],
          ((uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_GETNICKLIST_TIME])*60) == true) {
            return false;
        }
    }

	if(clsScriptManager::mPtr->Arrival(pUser, sData, ui32Len, clsScriptManager::GETNICKLIST_ARRIVAL) == true ||
		pUser->ui8State >= User::STATE_CLOSING ||
		((pUser->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == true) {
		return false;
	}

	return true;
}
//---------------------------------------------------------------------------

// $Key
void clsDcCommands::Key(User * pUser, char * sData, const uint32_t &ui32Len) {
    if(((pUser->ui32BoolBits & User::BIT_HAVE_KEY) == User::BIT_HAVE_KEY) == true) {
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] $Key flood from %s (%s) - user closed.", pUser->sNick, pUser->sIP);

        pUser->Close();
        return;
    }
    
    pUser->ui32BoolBits |= User::BIT_HAVE_KEY;

    sData[ui32Len-1] = '\0'; // cutoff pipe

    if(ui32Len < 6 || strcmp(Lock2Key(pUser->pLogInOut->pBuffer), sData+5) != 0) {
        if(ui32Len > 65000) {
            sData[65000] = '\0';
        }

		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad $Key from %s (%s) - user closed. (%s)", pUser->sNick, pUser->sIP, sData);

        pUser->Close();
        return;
    }

    pUser->FreeBuffer();

    sData[ui32Len-1] = '|'; // add back pipe

	clsScriptManager::mPtr->Arrival(pUser, sData, ui32Len, clsScriptManager::KEY_ARRIVAL);

	if(pUser->ui8State >= User::STATE_CLOSING) {
		return;
	}

    pUser->ui8State = User::STATE_VALIDATE;
}
//---------------------------------------------------------------------------

// $Kick <name>
void clsDcCommands::Kick(User * pUser, char * sData, const uint32_t &ui32Len) {
    if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::KICK) == false) {
        pUser->SendFormat("clsDcCommands::Kick1", true, "<%s> %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_ARE_NOT_ALWD_TO_USE_THIS_CMD]);
        return;
    } 

    if(ui32Len < 8) {
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad $Kick (%s) from %s (%s) - user closed.", sData, pUser->sNick, pUser->sIP);

        pUser->Close();
        return;
    }

	if(clsScriptManager::mPtr->Arrival(pUser, sData, ui32Len, clsScriptManager::KICK_ARRIVAL) == true ||
		pUser->ui8State >= User::STATE_CLOSING) {
		return;
	}

    sData[ui32Len-1] = '\0'; // cutoff pipe

    User *OtherUser = clsHashManager::mPtr->FindUser(sData+6, ui32Len-7);
    if(OtherUser != NULL) {
        // Self-kick
        if(OtherUser == pUser) {
            pUser->SendFormat("clsDcCommands::Kick2", true, "<%s> %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_CANT_KICK_YOURSELF]);
            return;
    	}
    	
        if(OtherUser->i32Profile != -1 && pUser->i32Profile > OtherUser->i32Profile) {
            pUser->SendFormat("clsDcCommands::Kick3", true, "<%s> %s %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_ARE_NOT_ALLOWED_TO_KICK], OtherUser->sNick);
        	return;
        }

        if(pUser->pCmdToUserStrt != NULL) {
            PrcsdToUsrCmd * cur = NULL, * prev = NULL,
                * next = pUser->pCmdToUserStrt;

            while(next != NULL) {
                cur = next;
                next = cur->pNext;
                                       
                if(OtherUser == cur->pTo) {
                    cur->pTo->SendChar(cur->sCommand, cur->ui32Len);

                    if(prev == NULL) {
                        if(cur->pNext == NULL) {
                            pUser->pCmdToUserStrt = NULL;
                            pUser->pCmdToUserEnd = NULL;
                        } else {
                            pUser->pCmdToUserStrt = cur->pNext;
                        }
                    } else if(cur->pNext == NULL) {
                        prev->pNext = NULL;
                        pUser->pCmdToUserEnd = prev;
                    } else {
                        prev->pNext = cur->pNext;
                    }

#ifdef _WIN32
					if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->sCommand) == 0) {
						AppendDebugLog("%s - [MEM] Cannot deallocate cur->sCommand in clsDcCommands::Kick\n");
                    }
#else
					free(cur->sCommand);
#endif
                    cur->sCommand = NULL;

#ifdef _WIN32
                    if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->sToNick) == 0) {
						AppendDebugLog("%s - [MEM] Cannot deallocate cur->ToNick in clsDcCommands::Kick\n");
                    }
#else
					free(cur->sToNick);
#endif
                    cur->sToNick = NULL;

					delete cur;
                    break;
                }
                prev = cur;
            }
        }

        char * sBanTime;
		if(OtherUser->pLogInOut != NULL && OtherUser->pLogInOut->pBuffer != NULL &&
			(sBanTime = stristr(OtherUser->pLogInOut->pBuffer, "_BAN_")) != NULL) {
			sBanTime[0] = '\0';

			if(sBanTime[5] == '\0' || sBanTime[5] == ' ') { // permban
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::BAN) == false) {
					pUser->SendFormat("clsDcCommands::Kick4", true, "<%s> %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_ARE_NOT_ALWD_TO_USE_THIS_CMD]);
                    return;
                }

				clsBanManager::mPtr->Ban(OtherUser, OtherUser->pLogInOut->pBuffer, pUser->sNick, false);
    
                clsGlobalDataQueue::mPtr->StatusMessageFormat("clsDcCommands::Kick1", "<%s> *** %s %s %s %s %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], OtherUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_WITH_IP], OtherUser->sIP, clsLanguageManager::mPtr->sTexts[LAN_HAS_BEEN], 
					clsLanguageManager::mPtr->sTexts[LAN_BANNED_LWR], clsLanguageManager::mPtr->sTexts[LAN_BY_LWR], pUser->sNick);

                if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    pUser->SendFormat("clsDcCommands::Kick5", true, "<%s> *** %s %s %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], OtherUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_WITH_IP], OtherUser->sIP, clsLanguageManager::mPtr->sTexts[LAN_HAS_BEEN], 
						clsLanguageManager::mPtr->sTexts[LAN_BANNED_LWR]);
                }

        		// disconnect the user
				clsUdpDebug::mPtr->BroadcastFormat("[SYS] User %s (%s) kicked by %s", OtherUser->sNick, OtherUser->sIP, pUser->sNick);

				OtherUser->Close();

                return;
			} else if(isdigit(sBanTime[5]) != 0) { // tempban
                if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::TEMP_BAN) == false) {
                    pUser->SendFormat("clsDcCommands::Kick6", true, "<%s> %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_ARE_NOT_ALWD_TO_USE_THIS_CMD]);
                    return;
                }

				uint32_t i = 6;
				while(sBanTime[i] != '\0' && isdigit(sBanTime[i]) != 0) {
                	i++;
                }

                char cTime = sBanTime[i];
                sBanTime[i] = '\0';
                int iTime = atoi(sBanTime+5);
				time_t acc_time, ban_time;

				if(cTime != '\0' && iTime > 0 && GenerateTempBanTime(cTime, iTime, acc_time, ban_time) == true) {
					clsBanManager::mPtr->TempBan(OtherUser, OtherUser->pLogInOut->pBuffer, pUser->sNick, 0, ban_time, false);

                    static char sTime[256];
                    strcpy(sTime, formatTime((ban_time-acc_time)/60));

                    clsGlobalDataQueue::mPtr->StatusMessageFormat("clsDcCommands::Kick2", "<%s> *** %s %s %s %s %s %s %s %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], OtherUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_WITH_IP], OtherUser->sIP, clsLanguageManager::mPtr->sTexts[LAN_HAS_BEEN], 
						clsLanguageManager::mPtr->sTexts[LAN_TEMP_BANNED], clsLanguageManager::mPtr->sTexts[LAN_BY_LWR], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_TO_LWR], sTime);
                
                    if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                        pUser->SendFormat("clsDcCommands::Kick7", true, "<%s> *** %s %s %s %s %s %s: %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], OtherUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_WITH_IP], OtherUser->sIP, clsLanguageManager::mPtr->sTexts[LAN_HAS_BEEN], 
							clsLanguageManager::mPtr->sTexts[LAN_TEMP_BANNED], clsLanguageManager::mPtr->sTexts[LAN_TO_LWR], sTime);
                	}
    
                    // disconnect the user
					clsUdpDebug::mPtr->BroadcastFormat("[SYS] User %s (%s) kicked by %s", OtherUser->sNick, OtherUser->sIP, pUser->sNick);

					OtherUser->Close();

					return;
                }
            }
		}

        clsBanManager::mPtr->TempBan(OtherUser, OtherUser->pLogInOut != NULL ? OtherUser->pLogInOut->pBuffer : NULL, pUser->sNick, 0, 0, false);

        clsGlobalDataQueue::mPtr->StatusMessageFormat("clsDcCommands::Kick3", "<%s> *** %s %s %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], OtherUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_WITH_IP], OtherUser->sIP, clsLanguageManager::mPtr->sTexts[LAN_WAS_KICKED_BY], pUser->sNick);

        if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
            pUser->SendFormat("clsDcCommands::Kick8", true, "<%s> *** %s %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], OtherUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_WITH_IP], OtherUser->sIP, clsLanguageManager::mPtr->sTexts[LAN_WAS_KICKED]);
        }

        // disconnect the user
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] User %s (%s) kicked by %s", OtherUser->sNick, OtherUser->sIP, pUser->sNick);

        OtherUser->Close();
    }
}
//---------------------------------------------------------------------------

// $Search $MultiSearch
bool clsDcCommands::SearchDeflood(User *pUser, char * sData, const uint32_t &ui32Len, const bool &bCheck, const bool &bMulti) {
    // search flood protection ... modified by PPK ;-)
    if(bCheck == true && clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::NODEFLOODSEARCH) == false) {
        if(clsSettingManager::mPtr->i16Shorts[SETSHORT_SEARCH_ACTION] != 0) {
            if(DeFloodCheckForFlood(pUser, DEFLOOD_SEARCH, clsSettingManager::mPtr->i16Shorts[SETSHORT_SEARCH_ACTION],
              pUser->ui16Searchs, pUser->ui64SearchsTick, clsSettingManager::mPtr->i16Shorts[SETSHORT_SEARCH_MESSAGES],
              (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_SEARCH_TIME]) == true) {
                return false;
            }
        }

        if(clsSettingManager::mPtr->i16Shorts[SETSHORT_SEARCH_ACTION2] != 0) {
            if(DeFloodCheckForFlood(pUser, DEFLOOD_SEARCH, clsSettingManager::mPtr->i16Shorts[SETSHORT_SEARCH_ACTION2],
              pUser->ui16Searchs2, pUser->ui64SearchsTick2, clsSettingManager::mPtr->i16Shorts[SETSHORT_SEARCH_MESSAGES2],
              (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_SEARCH_TIME2]) == true) {
                return false;
            }
        }

        // 2nd check for same search flood
		if(clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_SEARCH_ACTION] != 0) {
			bool bNewData = false;
            if(DeFloodCheckForSameFlood(pUser, DEFLOOD_SAME_SEARCH, clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_SEARCH_ACTION],
              pUser->ui16SameSearchs, pUser->ui64SameSearchsTick, 
              clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_SEARCH_MESSAGES], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_SEARCH_TIME],
			  sData+(bMulti == true ? 13 : 8), ui32Len-(bMulti == true ? 13 : 8),
			  pUser->sLastSearch, pUser->ui16LastSearchLen, bNewData) == true) {
                return false;
            }

			if(bNewData == true) {
				pUser->SetLastSearch(sData+(bMulti == true ? 13 : 8), ui32Len-(bMulti == true ? 13 : 8));
			}
        }
    }
    
    return true;
}
//---------------------------------------------------------------------------

// $Search $MultiSearch
void clsDcCommands::Search(User *pUser, char * sData, uint32_t ui32Len, const bool &bCheck, const bool &bMulti) {
    uint32_t iAfterCmd;
    if(bMulti == false) {
        if(ui32Len < 10) {
			clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad $Search (%s) from %s (%s) - user closed.", sData, pUser->sNick, pUser->sIP);

            pUser->Close();
			return;
        }
        iAfterCmd = 8;
    } else {
        if(ui32Len < 15) {
			clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad $MultiSearch (%s) from %s (%s) - user closed.", sData, pUser->sNick, pUser->sIP);

            pUser->Close();
            return;
        }
        iAfterCmd = 13;
    }

    if(bCheck == true && clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::NOSEARCHINTERVAL) == false) {
        if(DeFloodCheckInterval(pUser, INTERVAL_SEARCH, pUser->ui16SearchsInt, 
            pUser->ui64SearchsIntTick, clsSettingManager::mPtr->i16Shorts[SETSHORT_SEARCH_INTERVAL_MESSAGES],
            (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_SEARCH_INTERVAL_TIME]) == true) {
            return;
        }
    }

	if(clsScriptManager::mPtr->Arrival(pUser, sData, ui32Len, clsScriptManager::SEARCH_ARRIVAL) == true ||
		pUser->ui8State >= User::STATE_CLOSING) {
		return;
	}

    // send search from actives to all, from passives to actives only
    // PPK ... optimization ;o)
    if(bMulti == false && *((uint32_t *)(sData+iAfterCmd)) == *((uint32_t *)"Hub:")) {
        if(pUser->sTag == NULL) {
            pUser->ui32BoolBits &= ~User::BIT_IPV4_ACTIVE;
        }

        // PPK ... check nick !!!
        if((sData[iAfterCmd+4+pUser->ui8NickLen] != ' ') || (memcmp(sData+iAfterCmd+4, pUser->sNick, pUser->ui8NickLen) != 0)) {
            if(ui32Len > 65000) {
                sData[65000] = '\0';
            }

			clsUdpDebug::mPtr->BroadcastFormat("[SYS] Nick spoofing in search from %s (%s) - user closed. (%s)", pUser->sNick, pUser->sIP, sData);

            pUser->Close();
            return;
        }

        if(bCheck == true && clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::NOSEARCHLIMITS) == false &&
            (clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_SEARCH_LEN] != 0 || clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_SEARCH_LEN] != 0)) {
            // PPK ... search string len check
            // $Search Hub:PPK F?T?0?2?test|
            uint32_t iChar = iAfterCmd+8+pUser->ui8NickLen+1;
            uint32_t iCount = 0;
            for(; iChar < ui32Len; iChar++) {
                if(sData[iChar] == '?') {
                    iCount++;
                    if(iCount == 2)
                        break;
                }
            }

            iCount = ui32Len-2-iChar;

            if(iCount < (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_SEARCH_LEN]) {
                pUser->SendFormat("clsDcCommands::Search1", true, "<%s> %s %hd.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SORRY_MIN_SEARCH_LEN_IS], clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_SEARCH_LEN]);
                return;
            }
            if(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_SEARCH_LEN] != 0 && iCount > (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_SEARCH_LEN]) {
                pUser->SendFormat("clsDcCommands::Search2", true, "<%s> %s %hd.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SORRY_MAX_SEARCH_LEN_IS], clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_SEARCH_LEN]);
                return;
            }
        }

        pUser->iSR = 0;

        pUser->pCmdPassiveSearch = AddSearch(pUser, pUser->pCmdPassiveSearch, sData, ui32Len, false);
    } else {
        if(pUser->sTag == NULL) {
            pUser->ui32BoolBits |= User::BIT_IPV4_ACTIVE;
        }

        if(bCheck == true && clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::NOSEARCHLIMITS) == false &&
            (clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_SEARCH_LEN] != 0 || clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_SEARCH_LEN] != 0)) {
            // PPK ... search string len check
            // $Search 1.2.3.4:1 F?F?0?2?test| / $Search [::1]:1 F?F?0?2?test|
            uint32_t iChar = iAfterCmd+9;
            uint32_t iCount = 0;

            for(; iChar < ui32Len; iChar++) {
                if(sData[iChar] == '?') {
                    iCount++;
                    if(iCount == 4)
                        break;
                }
            }

            iCount = ui32Len-2-iChar;

            if(iCount < (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_SEARCH_LEN]) {
                pUser->SendFormat("clsDcCommands::Search3", true, "<%s> %s %hd.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SORRY_MIN_SEARCH_LEN_IS], clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_SEARCH_LEN]);
                return;
            }
            if(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_SEARCH_LEN] != 0 && iCount > (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_SEARCH_LEN]) {
                pUser->SendFormat("clsDcCommands::Search4", true, "<%s> %s %hd.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SORRY_MAX_SEARCH_LEN_IS], clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_SEARCH_LEN]);
                return;
            }
        }

        uint8_t ui8AfterPortLen = 0;
    	uint16_t ui16Port = 0;
    	bool bWrongPort = false;
    	bool bCorrectIP = CheckIPPort(pUser, sData+iAfterCmd, bWrongPort, ui16Port, ui8AfterPortLen, ' ');

		if(bWrongPort == true) {
            SendIncorrectPortMsg(pUser, false);

            if(ui32Len > 65000) {
                sData[65000] = '\0';
            }

			clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad Port in Search from %s (%s). (%s)", pUser->sNick, pUser->sIP, sData);

            pUser->Close();
            return;
		}
    
        // IP check
        if(bCheck == true && clsSettingManager::mPtr->bBools[SETBOOL_CHECK_IP_IN_COMMANDS] == true && clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::NOIPCHECK) == false && bCorrectIP == false) {
            if((pUser->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
                if((pUser->ui32BoolBits & User::BIT_IPV6_ACTIVE) == User::BIT_IPV6_ACTIVE) {
                    int iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "$Search [%s]:%hu %s", pUser->sIP, ui16Port, sData+iAfterCmd+ui8AfterPortLen);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsDcCommands::Search12-1") == true) {
						pUser->pCmdActive6Search = AddSearch(pUser, pUser->pCmdActive6Search, clsServerManager::pGlobalBuffer, iMsgLen, true);
                    }
                } else {
                    int iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "$Search Hub:%s %s", pUser->sNick, sData+iAfterCmd+ui8AfterPortLen);
                    if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsDcCommands::Search12-1") == true) {
                        pUser->pCmdPassiveSearch = AddSearch(pUser, pUser->pCmdPassiveSearch, clsServerManager::pGlobalBuffer, iMsgLen, false);
                    }
                }

				if((pUser->ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4) {
                    if((pUser->ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE) {
                        int iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "$Search %s:%hu %s", pUser->sIPv4, ui16Port, sData+iAfterCmd+ui8AfterPortLen);
                        if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsDcCommands::Search12-2") == true) {
                            pUser->pCmdActive4Search = AddSearch(pUser, pUser->pCmdActive4Search, clsServerManager::pGlobalBuffer, iMsgLen, true);
                        }
                    } else {
                        int iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "$Search Hub:%s %s", pUser->sNick, sData+iAfterCmd+ui8AfterPortLen);
                        if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsDcCommands::Search12-3") == true) {
                            pUser->pCmdPassiveSearch = AddSearch(pUser, pUser->pCmdPassiveSearch, clsServerManager::pGlobalBuffer, iMsgLen, false);
                        }
                    }
				}

                char * sBadIP = sData+iAfterCmd;
                if(sBadIP[0] == '[') {
                    sBadIP[strlen(sBadIP)-1] = '\0';
                    sBadIP++;
                }

                SendIPFixedMsg(pUser, sBadIP, pUser->sIP);
                return;
            } else if((pUser->ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4) {
                char * sIP = pUser->ui8IPv4Len == 0 ? pUser->sIP : pUser->sIPv4;

                int iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "$Search %s:%hu %s", sIP, ui16Port, sData+iAfterCmd+ui8AfterPortLen);
                if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsDcCommands::Search13") == true) {
					pUser->pCmdActive4Search = AddSearch(pUser, pUser->pCmdActive4Search, clsServerManager::pGlobalBuffer, iMsgLen, true);
                }

                char * sBadIP = sData+iAfterCmd;
                if(sBadIP[0] == '[') {
                    sBadIP[strlen(sBadIP)-1] = '\0';
                    sBadIP++;
                }

                SendIPFixedMsg(pUser, sBadIP, sIP);
                return;
            }
        }

        if(bMulti == true) {
            sData[5] = '$';
            sData += 5;
            ui32Len -= 5;
        }

		if((pUser->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
			if(sData[8] == '[') {
				pUser->pCmdActive6Search = AddSearch(pUser, pUser->pCmdActive6Search, sData, ui32Len, true);

				if((pUser->ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4) {
					if(GetPort(sData+8, ui16Port, ui8AfterPortLen, ' ') == false) {
                    	if((pUser->ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE) {
                        	int iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "$Search %s:%hu %s", pUser->sIPv4, ui16Port, sData+8+ui8AfterPortLen);
                        	if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsDcCommands::Search15") == true) {
                            	pUser->pCmdActive4Search = AddSearch(pUser, pUser->pCmdActive4Search, clsServerManager::pGlobalBuffer, iMsgLen, true);
                        	}
						} else {
                        	int iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "$Search Hub:%s %s", pUser->sNick, sData+8+ui8AfterPortLen);
                        	if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsDcCommands::Search16") == true) {
                            	pUser->pCmdPassiveSearch = AddSearch(pUser, pUser->pCmdPassiveSearch, clsServerManager::pGlobalBuffer, iMsgLen, false);
                        	}
                        }
					}
				}
			} else {
                pUser->pCmdActive4Search = AddSearch(pUser, pUser->pCmdActive4Search, sData, ui32Len, true);

                if(((pUser->ui32BoolBits & User::BIT_IPV6_ACTIVE) == User::BIT_IPV6_ACTIVE) == false) {
                	if(GetPort(sData+8, ui16Port, ui8AfterPortLen, ' ') == false) {
                    	int iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "$Search Hub:%s %s", pUser->sNick, sData+8+ui8AfterPortLen);
                    	if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsDcCommands::Search17") == true) {
                        	pUser->pCmdPassiveSearch = AddSearch(pUser, pUser->pCmdPassiveSearch, clsServerManager::pGlobalBuffer, iMsgLen, false);
                        }
                    }
                }
			}
		} else {
			pUser->pCmdActive4Search = AddSearch(pUser, pUser->pCmdActive4Search, sData, ui32Len, true);
		}
    }
}
//---------------------------------------------------------------------------

// $MyINFO $ALL  $ $$$$|
bool clsDcCommands::MyINFODeflood(User * pUser, char * sData, const uint32_t &ui32Len, const bool &bCheck) {
    if(ui32Len < (22u+pUser->ui8NickLen)) {
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad $MyINFO (%s) from %s (%s) - user closed.", sData, pUser->sNick, pUser->sIP);

        pUser->Close();
        return false;
    }

    // PPK ... check flood ...
    if(bCheck == true && clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::NODEFLOODMYINFO) == false) { 
        if(clsSettingManager::mPtr->i16Shorts[SETSHORT_MYINFO_ACTION] != 0) {
            if(DeFloodCheckForFlood(pUser, DEFLOOD_MYINFO, clsSettingManager::mPtr->i16Shorts[SETSHORT_MYINFO_ACTION],
              pUser->ui16MyINFOs, pUser->ui64MyINFOsTick, clsSettingManager::mPtr->i16Shorts[SETSHORT_MYINFO_MESSAGES],
              (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_MYINFO_TIME]) == true) {
                return false;
            }
        }

        if(clsSettingManager::mPtr->i16Shorts[SETSHORT_MYINFO_ACTION2] != 0) {
            if(DeFloodCheckForFlood(pUser, DEFLOOD_MYINFO, clsSettingManager::mPtr->i16Shorts[SETSHORT_MYINFO_ACTION2],
              pUser->ui16MyINFOs2, pUser->ui64MyINFOsTick2, clsSettingManager::mPtr->i16Shorts[SETSHORT_MYINFO_MESSAGES2],
              (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_MYINFO_TIME2]) == true) {
                return false;
            }
        }
    }

    if(ui32Len > (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_MYINFO_LEN]) {
        pUser->SendFormat("clsDcCommands::MyINFODeflood", true, "<%s> %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_MYINFO_TOO_LONG]);

        if(ui32Len > 65000) {
            sData[65000] = '\0';
        }

		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad $MyINFO from %s (%s) - user closed. (%s)", pUser->sNick, pUser->sIP, sData);

        pUser->Close();
		return false;
    }
    
    return true;
}
//---------------------------------------------------------------------------

// $MyINFO $ALL  $ $$$$|
bool clsDcCommands::MyINFO(User * pUser, char * sData, const uint32_t &ui32Len) {
    // if no change, just return
    // else store MyINFO and perform all checks again
    if(pUser->sMyInfoOriginal != NULL) { // PPK ... optimizations
       	if(ui32Len == pUser->ui16MyInfoOriginalLen && memcmp(pUser->sMyInfoOriginal+14+pUser->ui8NickLen, sData+14+pUser->ui8NickLen, pUser->ui16MyInfoOriginalLen-14-pUser->ui8NickLen) == 0) {
           return false;
        }
    }

    pUser->SetMyInfoOriginal(sData, (uint16_t)ui32Len);

    if(pUser->ui8State >= User::STATE_CLOSING) {
        return false;
    }

    if(pUser->ProcessRules() == false) {
        pUser->Close();
        return false;
    }
    
    // TODO 3 -oPTA -ccheckers: Slots fetching for no tag users
    //search command for slots fetch for users without tag
    //if(pUser->Tag == NULL)
    //{
    //    pUser->SendText("$Search "+HubAddress->Text+":411 F?F?0?1?.|");
    //}

    // SEND myinfo to others (including me) only if this is
    // a refresh MyINFO event. Else dispatch it in addMe condition
    // of service loop

    // PPK ... moved lua here -> another "optimization" ;o)
	clsScriptManager::mPtr->Arrival(pUser, sData, ui32Len, clsScriptManager::MYINFO_ARRIVAL);

	if(pUser->ui8State >= User::STATE_CLOSING) {
		return false;
	}
    
    return true;
}
//---------------------------------------------------------------------------

// $MyPass
void clsDcCommands::MyPass(User * pUser, char * sData, const uint32_t &ui32Len) {
    RegUser * pReg = clsRegManager::mPtr->Find(pUser);
    if(pReg != NULL && (pUser->ui32BoolBits & User::BIT_WAITING_FOR_PASS) == User::BIT_WAITING_FOR_PASS) {
        pUser->ui32BoolBits &= ~User::BIT_WAITING_FOR_PASS;
    } else {
        // We don't send $GetPass!
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] $MyPass without request from %s (%s) - user closed.", pUser->sNick, pUser->sIP);

        pUser->Close();
        return;
    }

    if(ui32Len < 10|| ui32Len > 73) {
        if(ui32Len > 65000) {
            sData[65000] = '\0';
        }

		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad $MyPass from %s (%s) - user closed. (%s)", pUser->sNick, pUser->sIP, sData);

        pUser->Close();
        return;
    }

    sData[ui32Len-1] = '\0'; // cutoff pipe

    bool bBadPass = false;

	if(pReg->bPassHash == true) {
		uint8_t ui8Hash[64];

		size_t szLen = ui32Len-9;

		if(HashPassword(sData+8, szLen, ui8Hash) == false || memcmp(pReg->ui8PassHash, ui8Hash, 64) != 0) {
			bBadPass = true;
		}
	} else {
		if(strcmp(pReg->sPass, sData+8) != 0) {
			bBadPass = true;
		}
	}

    // if password is wrong, close the connection
    if(bBadPass == true) {
        if(clsSettingManager::mPtr->bBools[SETBOOL_ADVANCED_PASS_PROTECTION] == true) {
            time(&pReg->tLastBadPass);
            if(pReg->ui8BadPassCount < 255)
                pReg->ui8BadPassCount++;
        }
    
        if(clsSettingManager::mPtr->i16Shorts[SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE] != 0) {
            // brute force password protection
			PassBf * PassBfItem = Find(pUser->ui128IpHash);
            if(PassBfItem == NULL) {
                PassBfItem = new (std::nothrow) PassBf(pUser->ui128IpHash);
                if(PassBfItem == NULL) {
					AppendDebugLog("%s - [MEM] Cannot allocate new PassBfItem in clsDcCommands::MyPass\n");
                	return;
                }

                if(PasswdBfCheck != NULL) {
                    PasswdBfCheck->pPrev = PassBfItem;
                    PassBfItem->pNext = PasswdBfCheck;
                    PasswdBfCheck = PassBfItem;
                }
                PasswdBfCheck = PassBfItem;
            } else {
                if(PassBfItem->iCount == 2) {
                    BanItem *Ban = clsBanManager::mPtr->FindFull(pUser->ui128IpHash);
                    if(Ban == NULL || ((Ban->ui8Bits & clsBanManager::FULL) == clsBanManager::FULL) == false) {
                        int iRet = sprintf(clsServerManager::pGlobalBuffer, "3x bad password for nick %s", pUser->sNick);
                        if(CheckSprintf(iRet, clsServerManager::szGlobalBufferSize, "clsDcCommands::MyPass4") == false) {
                            clsServerManager::pGlobalBuffer[0] = '\0';
                        }
                        if(clsSettingManager::mPtr->i16Shorts[SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE] == 1) {
                            clsBanManager::mPtr->BanIp(pUser, NULL, clsServerManager::pGlobalBuffer, NULL, true);
                        } else {
                            clsBanManager::mPtr->TempBanIp(pUser, NULL, clsServerManager::pGlobalBuffer, NULL, clsSettingManager::mPtr->i16Shorts[SETSHORT_BRUTE_FORCE_PASS_PROTECT_TEMP_BAN_TIME]*60, 0, true);
                        }
                        Remove(PassBfItem);

                        pUser->SendFormat("clsDcCommands::MyPass1", false, "<%s> %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOUR_IP_BANNED_BRUTE_FORCE_ATTACK]);
                    } else {
                        pUser->SendFormat("clsDcCommands::MyPass2", false, "<%s> %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOUR_IS_BANNED]);
                    }
                    if(clsSettingManager::mPtr->bBools[SETBOOL_REPORT_3X_BAD_PASS] == true) {
                        clsGlobalDataQueue::mPtr->StatusMessageFormat("clsDcCommands::MyPass", "<%s> *** %s %s %s %s|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_IP], pUser->sIP, clsLanguageManager::mPtr->sTexts[LAN_BANNED_BECAUSE_3X_BAD_PASS_FOR_NICK], pUser->sNick);
                    }

                    if(ui32Len > 65000) {
                        sData[65000] = '\0';
                    }

					clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad 3x password from %s (%s) - user banned. (%s)", pUser->sNick, pUser->sIP, sData);

                    pUser->Close();
                    return;
                } else {
                    PassBfItem->iCount++;
                }
            }
        }

        pUser->SendFormat("clsDcCommands::MyPass3", false, "$BadPass|<%s> %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_INCORRECT_PASSWORD]);

        if(ui32Len > 65000) {
            sData[65000] = '\0';
        }

		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad password from %s (%s) - user closed. (%s)", pUser->sNick, pUser->sIP, sData);

        pUser->Close();
        return;
    } else {
        pUser->i32Profile = (int32_t)pReg->ui16Profile;

        pReg->ui8BadPassCount = 0;

        PassBf * PassBfItem = Find(pUser->ui128IpHash);
        if(PassBfItem != NULL) {
            Remove(PassBfItem);
        }

        sData[ui32Len-1] = '|'; // add back pipe

        // PPK ... Lua DataArrival only if pass is ok
		clsScriptManager::mPtr->Arrival(pUser, sData, ui32Len, clsScriptManager::PASSWORD_ARRIVAL);

		if(pUser->ui8State >= User::STATE_CLOSING) {
    		return;
    	}

        if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::HASKEYICON) == true) {
            pUser->ui32BoolBits |= User::BIT_OPERATOR;
        } else {
            pUser->ui32BoolBits &= ~User::BIT_OPERATOR;
        }
        
        // PPK ... addition for registered users, kill your own ghost >:-]
        if(((pUser->ui32BoolBits & User::BIT_HASHED) == User::BIT_HASHED) == false) {
            User *OtherUser = clsHashManager::mPtr->FindUser(pUser->sNick, pUser->ui8NickLen);
            if(OtherUser != NULL) {
				clsUdpDebug::mPtr->BroadcastFormat("[SYS] Ghost %s (%s) closed.", OtherUser->sNick, OtherUser->sIP);

                if(((pUser->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == false) {
           	        OtherUser->Close();
                } else {
                    OtherUser->Close(true);
                }
            }
            if(clsHashManager::mPtr->Add(pUser) == false) {
                return;
            }
            pUser->ui32BoolBits |= User::BIT_HASHED;
        }
        if(((pUser->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == false) {
            // welcome the new user
            // PPK ... fixed bad DC protocol implementation, $LogedIn is only for OPs !!!
            // registered DC1 users have enabled OP menu :)))))))))
            if(((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                pUser->SendFormat("clsDcCommands::MyPass4", true, "$Hello %s|$LogedIn %s|", pUser->sNick, pUser->sNick);
            } else {
                pUser->SendFormat("clsDcCommands::MyPass5", true, "$Hello %s|", pUser->sNick);
            }

            return;
        } else {
            if(((pUser->ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == false) {
                pUser->AddMeOrIPv4Check();
            }
        }
    }     
}
//---------------------------------------------------------------------------

// $OpForceMove $Who:<nickname>$Where:<iptoredirect>$Msg:<a message>
void clsDcCommands::OpForceMove(User * pUser, char * sData, const uint32_t &ui32Len) {
    if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::REDIRECT) == false) {
        pUser->SendFormat("clsDcCommands::OpForceMove1", true, "<%s> %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_ARE_NOT_ALWD_TO_USE_THIS_CMD]);
        return;
    }

    if(ui32Len < 31) {
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad $OpForceMove (%s) from %s (%s) - user closed.", sData, pUser->sNick, pUser->sIP);

        pUser->Close();
        return;
    }

	if(clsScriptManager::mPtr->Arrival(pUser, sData, ui32Len, clsScriptManager::OPFORCEMOVE_ARRIVAL) == true ||
		pUser->ui8State >= User::STATE_CLOSING) {
		return;
	}

    sData[ui32Len-1] = '\0'; // cutoff pipe

    char *sCmdParts[] = { NULL, NULL, NULL };
    uint16_t iCmdPartsLen[] = { 0, 0, 0 };
                
    uint8_t cPart = 0;
                
    sCmdParts[cPart] = sData+18; // nick start
                
    for(uint32_t ui32i = 18; ui32i < ui32Len; ui32i++) {
        if(sData[ui32i] == '$') {
            sData[ui32i] = '\0';
            iCmdPartsLen[cPart] = (uint16_t)((sData+ui32i)-sCmdParts[cPart]);
                    
            // are we on last $ ???
            if(cPart == 1) {
                sCmdParts[2] = sData+ui32i+1;
                iCmdPartsLen[2] = (uint16_t)(ui32Len-ui32i-1);
                break;
            }
                    
            cPart++;
            sCmdParts[cPart] = sData+ui32i+1;
        }
    }

    if(iCmdPartsLen[0] == 0 || iCmdPartsLen[1] < 7 || iCmdPartsLen[2] < 5 || iCmdPartsLen[1] > 4096 || iCmdPartsLen[2] > 16384) {
        return;
    }

    User *OtherUser = clsHashManager::mPtr->FindUser(sCmdParts[0], iCmdPartsLen[0]);
    if(OtherUser) {
   	    if(OtherUser->i32Profile != -1 && pUser->i32Profile > OtherUser->i32Profile) {
            pUser->SendFormat("clsDcCommands::OpForceMove2", true, "<%s> %s %s|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_ARE_NOT_ALLOWED_TO_REDIRECT], OtherUser->sNick);
            return;
        } else {
            OtherUser->SendFormat("clsDcCommands::OpForceMove3", false, "<%s> %s %s %s %s. %s: %s|$ForceMove %s|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_ARE_REDIRECTED_TO], sCmdParts[1]+6, clsLanguageManager::mPtr->sTexts[LAN_BY_LWR], pUser->sNick, 
				clsLanguageManager::mPtr->sTexts[LAN_MESSAGE], sCmdParts[2]+4, sCmdParts[1]+6);

            // PPK ... close user !!!
			clsUdpDebug::mPtr->BroadcastFormat("[SYS] User %s (%s) redirected by %s", OtherUser->sNick, OtherUser->sIP, pUser->sNick);

            OtherUser->Close();

            if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                clsGlobalDataQueue::mPtr->StatusMessageFormat("clsDcCommands::OpForceMove", "<%s> *** %s %s %s %s %s. %s: %s|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], OtherUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_IS_REDIRECTED_TO], sCmdParts[1]+6, 
					clsLanguageManager::mPtr->sTexts[LAN_BY_LWR], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_MESSAGE], sCmdParts[2]+4);
            }

            if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                pUser->SendFormat("clsDcCommands::OpForceMove4", true, "<%s> *** %s %s %s. %s: %s|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], OtherUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_IS_REDIRECTED_TO], sCmdParts[1]+6, clsLanguageManager::mPtr->sTexts[LAN_MESSAGE], sCmdParts[2]+4);
            }
        }
    }
}
//---------------------------------------------------------------------------

// $RevConnectToMe <ownnick> <nickname>
void clsDcCommands::RevConnectToMe(User * pUser, char * sData, const uint32_t &ui32Len, const bool &bCheck) {
    if(ui32Len < 19) {
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad $RevConnectToMe (%s) from %s (%s) - user closed.", sData, pUser->sNick, pUser->sIP);

        pUser->Close();
        return;
    }

    // PPK ... optimizations
    if((sData[16+pUser->ui8NickLen] != ' ') || (memcmp(sData+16, pUser->sNick, pUser->ui8NickLen) != 0)) {
        if(ui32Len > 65000) {
            sData[65000] = '\0';
        }

		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Nick spoofing in RCTM from %s (%s) - user closed. (%s)", pUser->sNick, pUser->sIP, sData);

        pUser->Close();
        return;
    }

    // PPK ... check flood ...
    if(bCheck == true && clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::NODEFLOODRCTM) == false) { 
        if(clsSettingManager::mPtr->i16Shorts[SETSHORT_RCTM_ACTION] != 0) {
            if(DeFloodCheckForFlood(pUser, DEFLOOD_RCTM, clsSettingManager::mPtr->i16Shorts[SETSHORT_RCTM_ACTION],
              pUser->ui16RCTMs, pUser->ui64RCTMsTick, clsSettingManager::mPtr->i16Shorts[SETSHORT_RCTM_MESSAGES],
              (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_RCTM_TIME]) == true) {
				return;
            }
        }

        if(clsSettingManager::mPtr->i16Shorts[SETSHORT_RCTM_ACTION2] != 0) {
            if(DeFloodCheckForFlood(pUser, DEFLOOD_RCTM, clsSettingManager::mPtr->i16Shorts[SETSHORT_RCTM_ACTION2],
              pUser->ui16RCTMs2, pUser->ui64RCTMsTick2, clsSettingManager::mPtr->i16Shorts[SETSHORT_RCTM_MESSAGES2],
			  (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_RCTM_TIME2]) == true) {
                return;
            }
        }
    }

    if(ui32Len > (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_RCTM_LEN]) {
        pUser->SendFormat("clsDcCommands::RevConnectToMe", true, "<%s> %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_RCTM_TOO_LONG]);

        if(ui32Len > 65000) {
            sData[65000] = '\0';
        }

		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Long $RevConnectToMe from %s (%s) - user closed. (%s)", pUser->sNick, pUser->sIP, sData);

        pUser->Close();
        return;
    }

    // PPK ... $RCTM means user is pasive ?!? Probably yes, let set it not active and use on another places ;)
    if(pUser->sTag == NULL) {
        pUser->ui32BoolBits &= ~User::BIT_IPV4_ACTIVE;
    }

	if(clsScriptManager::mPtr->Arrival(pUser, sData, ui32Len, clsScriptManager::REVCONNECTTOME_ARRIVAL) == true ||
		pUser->ui8State >= User::STATE_CLOSING) {
		return;
	}

    sData[ui32Len-1] = '\0'; // cutoff pipe
       
    User *OtherUser = clsHashManager::mPtr->FindUser(sData+17+pUser->ui8NickLen, ui32Len-(18+pUser->ui8NickLen));
    // PPK ... no connection to yourself !!!
    if(OtherUser != NULL && OtherUser != pUser && OtherUser->ui8State == User::STATE_ADDED) {
        sData[ui32Len-1] = '|'; // add back pipe
        pUser->AddPrcsdCmd(PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO, sData, ui32Len, OtherUser);
    }   
}
//---------------------------------------------------------------------------

// $SR <nickname> - Search Respond for passive users
void clsDcCommands::SR(User * pUser, char * sData, const uint32_t &ui32Len, const bool &bCheck) {
    if(ui32Len < 6u+pUser->ui8NickLen) {
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad $SR (%s) from %s (%s) - user closed.", sData, pUser->sNick, pUser->sIP);

        pUser->Close();
        return;
    }

    // PPK ... check flood ...
    if(bCheck == true && clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::NODEFLOODSR) == false) { 
        if(clsSettingManager::mPtr->i16Shorts[SETSHORT_SR_ACTION] != 0) {
            if(DeFloodCheckForFlood(pUser, DEFLOOD_SR, clsSettingManager::mPtr->i16Shorts[SETSHORT_SR_ACTION],
              pUser->ui16SRs, pUser->ui64SRsTick, clsSettingManager::mPtr->i16Shorts[SETSHORT_SR_MESSAGES],
              (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_SR_TIME]) == true) {
				return;
            }
        }

        if(clsSettingManager::mPtr->i16Shorts[SETSHORT_SR_ACTION2] != 0) {
            if(DeFloodCheckForFlood(pUser, DEFLOOD_SR, clsSettingManager::mPtr->i16Shorts[SETSHORT_SR_ACTION2],
              pUser->ui16SRs2, pUser->ui64SRsTick2, clsSettingManager::mPtr->i16Shorts[SETSHORT_SR_MESSAGES2],
			  (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_SR_TIME2]) == true) {
                return;
            }
        }
    }

    if(ui32Len > (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_SR_LEN]) {
        pUser->SendFormat("clsDcCommands::SR", true, "<%s> %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SR_TOO_LONG]);

        if(ui32Len > 65000) {
            sData[65000] = '\0';
        }

		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Long $SR from %s (%s) - user closed. (%s)", pUser->sNick, pUser->sIP, sData);


        pUser->Close();
		return;
    }

    // check $SR spoofing (thanx Fusbar)
    // PPK... added checking for empty space after nick
    if(sData[4+pUser->ui8NickLen] != ' ' || memcmp(sData+4, pUser->sNick, pUser->ui8NickLen) != 0) {
        if(ui32Len > 65000) {
            sData[65000] = '\0';
        }

		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Nick spoofing in SR from %s (%s) - user closed. (%s)", pUser->sNick, pUser->sIP, sData);

        pUser->Close();
        return;
    }

    // past SR to script only if it's not a data for SlotFetcher
	if(clsScriptManager::mPtr->Arrival(pUser, sData, ui32Len, clsScriptManager::SR_ARRIVAL) == true ||
		pUser->ui8State >= User::STATE_CLOSING) {
		return;
	}

    sData[ui32Len-1] = '\0'; // cutoff pipe

    char *towho = strrchr(sData, '\5');
    if(towho == NULL) return;

    User *OtherUser = clsHashManager::mPtr->FindUser(towho+1, ui32Len-2-(towho-sData));
    // PPK ... no $SR to yourself !!!
    if(OtherUser != NULL && OtherUser != pUser && OtherUser->ui8State == User::STATE_ADDED) {
        // PPK ... search replies limiting
        if(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_PASIVE_SR] != 0) {
			if(OtherUser->iSR >= (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_PASIVE_SR])
                return;
                        
            OtherUser->iSR++;
        }

        // cutoff the last part // PPK ... and do it fast ;)
        towho[0] = '|';
        towho[1] = '\0';
        pUser->AddPrcsdCmd(PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO, sData, ui32Len-OtherUser->ui8NickLen-1, OtherUser);
    }   
}

//---------------------------------------------------------------------------

// $SR <nickname> - Search Respond for active users from UDP
void clsDcCommands::SRFromUDP(User * pUser, char * sData, const size_t &szLen) {
	if(clsScriptManager::mPtr->Arrival(pUser, sData, szLen, clsScriptManager::UDP_SR_ARRIVAL) == true ||
		pUser->ui8State >= User::STATE_CLOSING) {
		return;
	}
}
//---------------------------------------------------------------------------

// $Supports item item item... PPK $Supports UserCommand NoGetINFO NoHello UserIP2 QuickList|
void clsDcCommands::Supports(User * pUser, char * sData, const uint32_t &ui32Len) {
    if(((pUser->ui32BoolBits & User::BIT_HAVE_SUPPORTS) == User::BIT_HAVE_SUPPORTS) == true) {
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] $Supports flood from %s (%s) - user closed.", pUser->sNick, pUser->sIP);

        pUser->Close();
        return;
    }

    pUser->ui32BoolBits |= User::BIT_HAVE_SUPPORTS;
    
    if(ui32Len < 13) {
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad $Supports (%s) from %s (%s) - user closed.", sData, pUser->sNick, pUser->sIP);

        pUser->Close();
        return;
    }

    if(sData[ui32Len-2] == ' ') {
        if(clsSettingManager::mPtr->bBools[SETBOOL_NO_QUACK_SUPPORTS] == false) {
            pUser->ui32BoolBits |= User::BIT_QUACK_SUPPORTS;
        } else {
            if(ui32Len > 65000) {
                sData[65000] = '\0';
            }

			clsUdpDebug::mPtr->BroadcastFormat("[SYS] Quack $Supports from %s (%s) - user closed. (%s)", pUser->sNick, pUser->sIP, sData);

			pUser->SendFormat("clsDcCommands::Supports1", false, "<%s> %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_QUACK_SUPPORTS]);

			pUser->Close();
			return;
		}
    }

	clsScriptManager::mPtr->Arrival(pUser, sData, ui32Len, clsScriptManager::SUPPORTS_ARRIVAL);

	if(pUser->ui8State >= User::STATE_CLOSING) {
    	return;
    }

    char *sSupport = sData+10;
	size_t szDataLen;
                    
    for(uint32_t ui32i = 10; ui32i < ui32Len-1; ui32i++) {
        if(sData[ui32i] == ' ') {
            sData[ui32i] = '\0';
        } else if(ui32i != ui32Len-2) {
            continue;
        } else {
            ui32i++;
        }

        szDataLen = (sData+ui32i)-sSupport;

        switch(sSupport[0]) {
            case 'N':
                if(sSupport[1] == 'o') {
                    if(((pUser->ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == false && szDataLen == 7 && memcmp(sSupport+2, "Hello", 5) == 0) {
                        pUser->ui32SupportBits |= User::SUPPORTBIT_NOHELLO;
                    } else if(((pUser->ui32SupportBits & User::SUPPORTBIT_NOGETINFO) == User::SUPPORTBIT_NOGETINFO) == false && szDataLen == 9 && memcmp(sSupport+2, "GetINFO", 7) == 0) {
                        pUser->ui32SupportBits |= User::SUPPORTBIT_NOGETINFO;
                    }
                }
                break;
            case 'Q': {
                if(((pUser->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == false && szDataLen == 9 && *((uint64_t *)(sSupport+1)) == *((uint64_t *)"uickList")) {
                    pUser->ui32SupportBits |= User::SUPPORTBIT_QUICKLIST;
                    // PPK ... in fact NoHello is only not fully implemented Quicklist (without diferent login sequency)
                    // That's why i overide NoHello here and use bQuicklist only for login, on other places is same as NoHello ;)
                    pUser->ui32SupportBits |= User::SUPPORTBIT_NOHELLO;
                }
                break;
            }
            case 'U': {
                if(((pUser->ui32SupportBits & User::SUPPORTBIT_USERCOMMAND) == User::SUPPORTBIT_USERCOMMAND) == false && szDataLen == 11 && memcmp(sSupport+1, "serCommand", 10) == 0) {
                    pUser->ui32SupportBits |= User::SUPPORTBIT_USERCOMMAND;
                } else if(((pUser->ui32SupportBits & User::SUPPORTBIT_USERIP2) == User::SUPPORTBIT_USERIP2) == false && szDataLen == 7 && memcmp(sSupport+1, "serIP2", 6) == 0) {
                    pUser->ui32SupportBits |= User::SUPPORTBIT_USERIP2;
                }
                break;
            }
            case 'B': {
                if(((pUser->ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == false && szDataLen == 7 && memcmp(sSupport+1, "otINFO", 6) == 0) {
                    if(clsSettingManager::mPtr->bBools[SETBOOL_DONT_ALLOW_PINGERS] == true) {
                        pUser->SendFormat("clsDcCommands::Supports2", false, "<%s> %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SORRY_THIS_HUB_NOT_ALLOW_PINGERS]);
                        pUser->Close();
                        return;
                    } else if(clsSettingManager::mPtr->bBools[SETBOOL_CHECK_IP_IN_COMMANDS] == false) {
                        pUser->SendFormat("clsDcCommands::Supports3", false, "<%s> Sorry, this hub don't want to be in hublist because allow CTM exploit.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC]);
                        pUser->Close();
                        return;
                    } else {
                        pUser->ui32BoolBits |= User::BIT_PINGER;
						pUser->SendFormat("clsDcCommands::Supports4", true, "%s%" PRIu64 " %s, %" PRIu64 " %s, %" PRIu64 " %s / %s: %u)|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_NAME_WLCM], clsServerManager::ui64Days, clsLanguageManager::mPtr->sTexts[LAN_DAYS_LWR], 
							clsServerManager::ui64Hours, clsLanguageManager::mPtr->sTexts[LAN_HOURS_LWR], clsServerManager::ui64Mins, clsLanguageManager::mPtr->sTexts[LAN_MINUTES_LWR], clsLanguageManager::mPtr->sTexts[LAN_USERS], clsServerManager::ui32Logged);
                    }
                }
                break;
            }
            case 'Z': {
                if(((pUser->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false) {
                	if(szDataLen == 6 && memcmp(sSupport+1, "Pipe0", 5) == 0) {
                		pUser->ui32SupportBits |= User::SUPPORTBIT_ZPIPE0;
                		pUser->ui32SupportBits |= User::SUPPORTBIT_ZPIPE;
                    	iStatZPipe++;
					} else if(szDataLen == 5 && *((uint32_t *)(sSupport+1)) == *((uint32_t *)"Pipe")) {
                    	pUser->ui32SupportBits |= User::SUPPORTBIT_ZPIPE;
                    	iStatZPipe++;
                    }
                }
                break;
            }
            case 'I': {
                if(szDataLen == 4) {
                    if(((pUser->ui32SupportBits & User::SUPPORTBIT_IP64) == User::SUPPORTBIT_IP64) == false && *((uint32_t *)sSupport) == *((uint32_t *)"IP64")) {
                        pUser->ui32SupportBits |= User::SUPPORTBIT_IP64;
                    } else if(((pUser->ui32SupportBits & User::SUPPORTBIT_IPV4) == User::SUPPORTBIT_IPV4) == false && *((uint32_t *)sSupport) == *((uint32_t *)"IPv4")) {
                        pUser->ui32SupportBits |= User::SUPPORTBIT_IPV4;
                    }
                }
                break;
            }
            case 'T': {
            	if(szDataLen == 4) {
                    if(((pUser->ui32SupportBits & User::SUPPORTBIT_TLS2) == User::SUPPORTBIT_TLS2) == false && *((uint32_t *)sSupport) == *((uint32_t *)"TLS2")) {
                        pUser->ui32SupportBits |= User::SUPPORTBIT_TLS2;
                    }
            	}
				break;
			}
            case '\0': {
                // PPK ... corrupted $Supports ???
				clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad $Supports from %s (%s) - user closed.", pUser->sNick, pUser->sIP);

                pUser->Close();
                return;
            }
            default:
                // PPK ... unknown supports
                break;
        }
                
        sSupport = sData+ui32i+1;
    }
    
    pUser->ui8State = User::STATE_VALIDATE;
    
    pUser->AddPrcsdCmd(PrcsdUsrCmd::SUPPORTS, NULL, 0, NULL);
}
//---------------------------------------------------------------------------

// $To: nickname From: ownnickname $<ownnickname> <message>
void clsDcCommands::To(User * pUser, char * sData, const uint32_t &ui32Len, const bool &bCheck) {
    char *cTemp = strchr(sData+5, ' ');

    if(ui32Len < 19 || cTemp == NULL) {
        if(ui32Len > 65000) {
            sData[65000] = '\0';
        }

		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad To from %s (%s) - user closed. (%s)", pUser->sNick, pUser->sIP, sData);

        pUser->Close();
        return;
    }
    
    size_t szNickLen = cTemp-(sData+5);

    if(szNickLen > 64) {
        pUser->SendFormat("clsDcCommands::To1", true, "<%s> *** %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
        return;
    }

    // is the mesg really from us ?
    // PPK ... replaced by better and faster code ;)
    int iRet = sprintf(clsServerManager::pGlobalBuffer, "From: %s $<%s> ", pUser->sNick, pUser->sNick);
    if(CheckSprintf(iRet, clsServerManager::szGlobalBufferSize, "clsDcCommands::To4") == true) {
        if(strncmp(cTemp+1, clsServerManager::pGlobalBuffer, iRet) != 0) {
			clsUdpDebug::mPtr->BroadcastFormat("[SYS] Nick spoofing in To from %s (%s) - user closed. (%s)", pUser->sNick, pUser->sIP, sData);

            pUser->Close();
            return;
        }
    }

    //FloodCheck
    if(bCheck == true && clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::NODEFLOODPM) == false) {
        // PPK ... pm antiflood
        if(clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_ACTION] != 0) {
            cTemp[0] = '\0';
            if(DeFloodCheckForFlood(pUser, DEFLOOD_PM, clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_ACTION],
                pUser->ui16PMs, pUser->ui64PMsTick, clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_MESSAGES],
                (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_TIME], sData+5) == true) {
                return;
            }
            cTemp[0] = ' ';
        }

        if(clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_ACTION2] != 0) {
            cTemp[0] = '\0';
            if(DeFloodCheckForFlood(pUser, DEFLOOD_PM, clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_ACTION2],
                pUser->ui16PMs2, pUser->ui64PMsTick2, clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_MESSAGES2],
                (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_TIME2], sData+5) == true) {
                return;
            }
            cTemp[0] = ' ';
        }

        // 2nd check for PM flooding
		if(clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_PM_ACTION] != 0) {
			bool bNewData = false;
			cTemp[0] = '\0';
			if(DeFloodCheckForSameFlood(pUser, DEFLOOD_SAME_PM, clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_PM_ACTION],
                pUser->ui16SamePMs, pUser->ui64SamePMsTick, 
                clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_PM_MESSAGES], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_PM_TIME],
                cTemp+(12+(2*pUser->ui8NickLen)), (ui32Len-(cTemp-sData))-(12+(2*pUser->ui8NickLen)), 
				pUser->sLastPM, pUser->ui16LastPMLen, bNewData, sData+5) == true) {
                return;
            }
            cTemp[0] = ' ';

			if(bNewData == true) {
				pUser->SetLastPM(cTemp+(12+(2*pUser->ui8NickLen)), (ui32Len-(cTemp-sData))-(12+(2*pUser->ui8NickLen)));
			}
        }
    }

    if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::NOCHATLIMITS) == false) {
        // 1st check for length limit for PM message
        size_t szMessLen = ui32Len-(2*pUser->ui8NickLen)-(cTemp-sData)-13;
        if(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_PM_LEN] != 0 && szMessLen > (size_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_PM_LEN]) {
       	    // PPK ... hubsec alias
       	    cTemp[0] = '\0';
            pUser->SendFormat("clsDcCommands::To2", true, "$To: %s From: %s $<%s> %s!|", pUser->sNick, sData+5, clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_THE_MESSAGE_WAS_TOO_LONG]);
            return;
        }

        // PPK ... check for message lines limit
        if(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_PM_LINES] != 0 || clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_PM_ACTION] != 0) {
			if(pUser->ui16SamePMs < 2) {
				uint16_t iLines = 1;
                for(uint32_t ui32i = 9+pUser->ui8NickLen; ui32i < ui32Len-(cTemp-sData)-1; ui32i++) {
                    if(cTemp[ui32i] == '\n') {
                        iLines++;
                    }
                }
                pUser->ui16LastPmLines = iLines;
                if(pUser->ui16LastPmLines > 1) {
                    pUser->ui16SameMultiPms++;
                }
            } else if(pUser->ui16LastPmLines > 1) {
                pUser->ui16SameMultiPms++;
            }

			if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::NODEFLOODPM) == false && clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_PM_ACTION] != 0) {
				if(pUser->ui16SameMultiPms > clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_PM_MESSAGES] &&
                    pUser->ui16LastPmLines >= clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_PM_LINES]) {
					cTemp[0] = '\0';
					uint16_t lines = 0;
					DeFloodDoAction(pUser, DEFLOOD_SAME_MULTI_PM, clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_PM_ACTION], lines, sData+5);
                    return;
                }
            }

            if(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_PM_LINES] != 0 && pUser->ui16LastPmLines > clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_PM_LINES]) {
                cTemp[0] = '\0';
                pUser->SendFormat("clsDcCommands::To3", true, "$To: %s From: %s $<%s> %s!|", pUser->sNick, sData+5, clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_THE_MESSAGE_WAS_TOO_LONG]);
                return;
            }
        }
    }

    if(bCheck == true && clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::NOPMINTERVAL) == false) {
        cTemp[0] = '\0';
        if(DeFloodCheckInterval(pUser, INTERVAL_PM, pUser->ui16PMsInt, 
            pUser->ui64PMsIntTick, clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_INTERVAL_MESSAGES],
            (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_INTERVAL_TIME], sData+5) == true) {
            return;
        }
        cTemp[0] = ' ';
    }

	if(clsScriptManager::mPtr->Arrival(pUser, sData, ui32Len, clsScriptManager::TO_ARRIVAL) == true ||
		pUser->ui8State >= User::STATE_CLOSING) {
		return;
	}

    cTemp[0] = '\0';

    // ignore the silly debug messages !!!
    if(memcmp(sData+5, "Vandel\\Debug", 12) == 0) {
        return;
    }

    // Everything's ok lets chat
    // if this is a PM to OpChat or Hub bot, process the message
    if(clsSettingManager::mPtr->bBools[SETBOOL_REG_BOT] == true && strcmp(sData+5, clsSettingManager::mPtr->sTexts[SETTXT_BOT_NICK]) == 0) {
        cTemp += 9+pUser->ui8NickLen;
        // PPK ... check message length, return if no mess found
        uint32_t ui32Len1 = (uint32_t)((ui32Len-(cTemp-sData))+1);
        if(ui32Len1 <= pUser->ui8NickLen+4u)
            return;

        // find chat message data
        char *sBuff = cTemp+pUser->ui8NickLen+3;

        // non-command chat msg
        for(uint8_t ui8i = 0; ui8i < (uint8_t)clsSettingManager::mPtr->ui16TextsLens[SETTXT_CHAT_COMMANDS_PREFIXES]; ui8i++) {
            if(sBuff[0] == clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][ui8i]) {
                sData[ui32Len-1] = '\0'; // cutoff pipe
                // built-in commands
                if(clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_TEXT_FILES] == true && 
                    clsTextFilesManager::mPtr->ProcessTextFilesCmd(pUser, sBuff+1, true)) {
                    return;
                }
           
                // HubCommands
                if(ui32Len1-pUser->ui8NickLen >= 10) {
                    if(clsHubCommands::DoCommand(pUser, sBuff, ui32Len1, true)) return;
                }
                        
                sData[ui32Len-1] = '|'; // add back pipe
                break;
            }
        }
        // PPK ... if i am here is not textfile request or hub command, try opchat
        if(clsSettingManager::mPtr->bBools[SETBOOL_REG_OP_CHAT] == true && clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::ALLOWEDOPCHAT) == true && 
            clsSettingManager::mPtr->bBotsSameNick == true) {
            uint32_t iOpChatLen = clsSettingManager::mPtr->ui16TextsLens[SETTXT_OP_CHAT_NICK];
            memcpy(cTemp-iOpChatLen-2, clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_NICK], iOpChatLen);
            pUser->AddPrcsdCmd(PrcsdUsrCmd::TO_OP_CHAT, cTemp-iOpChatLen-2, ui32Len-((cTemp-iOpChatLen-2)-sData), NULL);
        }
    } else if(clsSettingManager::mPtr->bBools[SETBOOL_REG_OP_CHAT] == true && 
        clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::ALLOWEDOPCHAT) == true && 
        strcmp(sData+5, clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_NICK]) == 0) {
        cTemp += 9+pUser->ui8NickLen;
        uint32_t iOpChatLen = clsSettingManager::mPtr->ui16TextsLens[SETTXT_OP_CHAT_NICK];
        memcpy(cTemp-iOpChatLen-2, clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_NICK], iOpChatLen);
        pUser->AddPrcsdCmd(PrcsdUsrCmd::TO_OP_CHAT, cTemp-iOpChatLen-2, (ui32Len-(cTemp-sData))+iOpChatLen+2, NULL);
    } else {       
        User *OtherUser = clsHashManager::mPtr->FindUser(sData+5, szNickLen);
        // PPK ... pm to yourself ?!? NO!
        if(OtherUser != NULL && OtherUser != pUser && OtherUser->ui8State == User::STATE_ADDED) {
            cTemp[0] = ' ';
            pUser->AddPrcsdCmd(PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO, sData, ui32Len, OtherUser, true);
        }
    }
}
//---------------------------------------------------------------------------

// $ValidateNick
void clsDcCommands::ValidateNick(User * pUser, char * sData, const uint32_t &ui32Len) {
    if(((pUser->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == true) {
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] $ValidateNick with QuickList support from %s (%s) - user closed.", pUser->sNick, pUser->sIP);

        pUser->Close();
        return;
    }

    if(ui32Len < 16) {
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Attempt to Validate empty nick (%s) from %s (%s) - user closed.", sData, pUser->sNick, pUser->sIP);

        pUser->Close();
        return;
    }

    sData[ui32Len-1] = '\0'; // cutoff pipe
    if(ValidateUserNick(pUser, sData+14, ui32Len-15, true) == false) return;

	sData[ui32Len-1] = '|'; // add back pipe

	clsScriptManager::mPtr->Arrival(pUser, sData, ui32Len, clsScriptManager::VALIDATENICK_ARRIVAL);
}
//---------------------------------------------------------------------------

// $Version
void clsDcCommands::Version(User * pUser, char * sData, const uint32_t &ui32Len) {
    if(ui32Len < 11) {
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad $Version (%s) from %s (%s) - user closed.", sData, pUser->sNick, pUser->sIP);

        pUser->Close();
        return;
    }

	pUser->ui8State = User::STATE_GETNICKLIST_OR_MYINFO;

	clsScriptManager::mPtr->Arrival(pUser, sData, ui32Len, clsScriptManager::VERSION_ARRIVAL);

	if(pUser->ui8State >= User::STATE_CLOSING) {
    	return;
    }
    
    sData[ui32Len-1] = '\0'; // cutoff pipe
    pUser->SetVersion(sData+9);
}
//---------------------------------------------------------------------------

// Chat message
bool clsDcCommands::ChatDeflood(User * pUser, char * sData, const uint32_t &ui32Len, const bool &bCheck) {
#ifdef _BUILD_GUI
    if(::SendMessage(clsMainWindowPageUsersChat::mPtr->hWndPageItems[clsMainWindowPageUsersChat::BTN_SHOW_CHAT], BM_GETCHECK, 0, 0) == BST_CHECKED) {
        sData[ui32Len - 1] = '\0';
        RichEditAppendText(clsMainWindowPageUsersChat::mPtr->hWndPageItems[clsMainWindowPageUsersChat::REDT_CHAT], sData);
        sData[ui32Len - 1] = '|';
    }
#endif
    
	// if the user is sending chat as other user, kick him
	if(sData[1+pUser->ui8NickLen] != '>' || sData[2+pUser->ui8NickLen] != ' ' || memcmp(pUser->sNick, sData+1, pUser->ui8NickLen) != 0) {
        if(ui32Len > 65000) {
            sData[65000] = '\0';
        }

		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Nick spoofing in chat from %s (%s) - user closed. (%s)", pUser->sNick, pUser->sIP, sData);

		pUser->Close();
		return false;
	}
        
	// PPK ... check flood...
	if(bCheck == true && clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::NODEFLOODMAINCHAT) == false) {
		if(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAIN_CHAT_ACTION] != 0) {
            if(DeFloodCheckForFlood(pUser, DEFLOOD_CHAT, clsSettingManager::mPtr->i16Shorts[SETSHORT_MAIN_CHAT_ACTION],
                pUser->ui16ChatMsgs, pUser->ui64ChatMsgsTick, clsSettingManager::mPtr->i16Shorts[SETSHORT_MAIN_CHAT_MESSAGES],
                (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_MAIN_CHAT_TIME]) == true) {
                return false;
            }
		}

		if(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAIN_CHAT_ACTION2] != 0) {
            if(DeFloodCheckForFlood(pUser, DEFLOOD_CHAT, clsSettingManager::mPtr->i16Shorts[SETSHORT_MAIN_CHAT_ACTION2],
                pUser->ui16ChatMsgs2, pUser->ui64ChatMsgsTick2, clsSettingManager::mPtr->i16Shorts[SETSHORT_MAIN_CHAT_MESSAGES2],
                (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_MAIN_CHAT_TIME2]) == true) {
                return false;
            }
		}

		// 2nd check for chatmessage flood
		if(clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MAIN_CHAT_ACTION] != 0) {
			bool bNewData = false;
			if(DeFloodCheckForSameFlood(pUser, DEFLOOD_SAME_CHAT, clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MAIN_CHAT_ACTION],
			  pUser->ui16SameChatMsgs, pUser->ui64SameChatsTick,
              clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MAIN_CHAT_MESSAGES], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MAIN_CHAT_TIME],
			  sData+pUser->ui8NickLen+3, ui32Len-(pUser->ui8NickLen+3), pUser->sLastChat, pUser->ui16LastChatLen, bNewData) == true) {
				return false;
			}

			if(bNewData == true) {
				pUser->SetLastChat(sData+pUser->ui8NickLen+3, ui32Len-(pUser->ui8NickLen+3));
			}
        }
    }

    // PPK ... ignore empty chat ;)
    if(ui32Len < pUser->ui8NickLen+5u) {
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------

// Chat message
void clsDcCommands::Chat(User * pUser, char * sData, const uint32_t &ui32Len, const bool &bCheck) {
    if(bCheck == true && clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::NOCHATLIMITS) == false) {
    	// PPK ... check for message limit length
 	    if(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_CHAT_LEN] != 0 && (ui32Len-pUser->ui8NickLen-4) > (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_CHAT_LEN]) {
            pUser->SendFormat("clsDcCommands::Chat1", true, "<%s> %s !|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_THE_MESSAGE_WAS_TOO_LONG]);
	        return;
 	    }

        // PPK ... check for message lines limit
        if(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_CHAT_LINES] != 0 || clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION] != 0) {
            if(pUser->ui16SameChatMsgs < 2) {
                uint16_t iLines = 1;

                for(uint32_t ui32i = pUser->ui8NickLen+3; ui32i < ui32Len-1; ui32i++) {
                    if(sData[ui32i] == '\n') {
                        iLines++;
                    }
                }

                pUser->ui16LastChatLines = iLines;

                if(pUser->ui16LastChatLines > 1) {
                    pUser->ui16SameMultiChats++;
                }
			} else if(pUser->ui16LastChatLines > 1) {
                pUser->ui16SameMultiChats++;
            }

			if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::NODEFLOODMAINCHAT) == false && clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION] != 0) {
                if(pUser->ui16SameMultiChats > clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_MESSAGES] &&
                    pUser->ui16LastChatLines >= clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_LINES]) {
                    uint16_t lines = 0;
					DeFloodDoAction(pUser, DEFLOOD_SAME_MULTI_CHAT, clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION], lines, NULL);
					return;
				}
			}

			if(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_CHAT_LINES] != 0 && pUser->ui16LastChatLines > clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_CHAT_LINES]) {
                pUser->SendFormat("clsDcCommands::Chat2", true, "<%s> %s !|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_THE_MESSAGE_WAS_TOO_LONG]);
                return;
			}
		}
	}

    if(bCheck == true && clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::NOCHATINTERVAL) == false) {
        if(DeFloodCheckInterval(pUser, INTERVAL_CHAT, pUser->ui16ChatIntMsgs, 
            pUser->ui64ChatIntMsgsTick, clsSettingManager::mPtr->i16Shorts[SETSHORT_CHAT_INTERVAL_MESSAGES],
            (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_CHAT_INTERVAL_TIME]) == true) {
            return;
        }
    }

    if(((pUser->ui32BoolBits & User::BIT_GAGGED) == User::BIT_GAGGED) == true)
        return;

    void * pQueueItem1 = clsGlobalDataQueue::mPtr->GetLastQueueItem();

	if(clsScriptManager::mPtr->Arrival(pUser, sData, ui32Len, clsScriptManager::CHAT_ARRIVAL) == true ||
		pUser->ui8State >= User::STATE_CLOSING) {
		return;
	}

    User * pToUser = NULL;
    void * pQueueItem2 = clsGlobalDataQueue::mPtr->GetLastQueueItem();

    if(pQueueItem1 != pQueueItem2) {
        if(pQueueItem1 == NULL) {
            pToUser = (User *)clsGlobalDataQueue::mPtr->InsertBlankQueueItem(clsGlobalDataQueue::mPtr->GetFirstQueueItem(), clsGlobalDataQueue::CMD_CHAT);
        } else {
            pToUser = (User *)clsGlobalDataQueue::mPtr->InsertBlankQueueItem(pQueueItem1, clsGlobalDataQueue::CMD_CHAT);
        }

        if(pToUser != NULL) {
            pUser->ui32BoolBits |= User::BIT_CHAT_INSERT;
        }
    } else if((pUser->ui32BoolBits & User::BIT_CHAT_INSERT) == User::BIT_CHAT_INSERT) {
        pToUser = (User *)clsGlobalDataQueue::mPtr->InsertBlankQueueItem(pQueueItem1, clsGlobalDataQueue::CMD_CHAT);
    }

	// PPK ... filtering kick messages
	if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::KICK) == true) {
    	if(ui32Len > pUser->ui8NickLen+21u) {
            char * cTemp = strchr(sData+pUser->ui8NickLen+3, '\n');
            if(cTemp != NULL) {
            	cTemp[0] = '\0';
            }

            char *temp, *temp1;
            if((temp = stristr(sData+pUser->ui8NickLen+3, "is kicking ")) != NULL &&
            	(temp1 = stristr(temp+12, " because: ")) != NULL) {
                // PPK ... catch kick message and store for later use in $Kick for tempban reason
                temp1[0] = '\0';               
                User * KickedUser = clsHashManager::mPtr->FindUser(temp+11, temp1-(temp+11));
                temp1[0] = ' ';
                if(KickedUser != NULL) {
                    // PPK ... valid kick messages for existing user, remove this message from deflood ;)
                    if(pUser->ui16ChatMsgs != 0) {
                        pUser->ui16ChatMsgs--;
                        pUser->ui16ChatMsgs2--;
                    }
                    if(temp1[10] != '|') {
                        sData[ui32Len-1] = '\0'; // get rid of the pipe
                        KickedUser->SetBuffer(temp1+10, ui32Len-(temp1-sData)-11);
                        sData[ui32Len-1] = '|'; // add back pipe
                    }
                }

            	if(cTemp != NULL) {
                    cTemp[0] = '\n';
                }

                // PPK ... kick messages filtering
                if(clsSettingManager::mPtr->bBools[SETBOOL_FILTER_KICK_MESSAGES] == true) {
                	if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_KICK_MESSAGES_TO_OPS] == true) {
               			if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                            int iRet = sprintf(clsServerManager::pGlobalBuffer, "%s $%s", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sData);
                            if(CheckSprintf(iRet, clsServerManager::szGlobalBufferSize, "clsDcCommands::Chat3") == true) {
								clsGlobalDataQueue::mPtr->SingleItemStore(clsServerManager::pGlobalBuffer, iRet, NULL, 0, clsGlobalDataQueue::SI_PM2OPS);
                            }
                        } else {
							clsGlobalDataQueue::mPtr->AddQueueItem(sData, ui32Len, NULL, 0, clsGlobalDataQueue::CMD_OPS);
                        }
		    		} else {
                        pUser->SendCharDelayed(sData, ui32Len);
                    }
                    return;
                }
            }

            if(cTemp != NULL) {
                cTemp[0] = '\n';
            }
        }        
	}

    pUser->AddPrcsdCmd(PrcsdUsrCmd::CHAT, sData, ui32Len, pToUser);
}
//---------------------------------------------------------------------------

// $Close nick|
void clsDcCommands::Close(User * pUser, char * sData, const uint32_t &ui32Len) {
    if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::CLOSE) == false) {
        pUser->SendFormat("clsDcCommands::Close1", true, "<%s> %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_ARE_NOT_ALWD_TO_USE_THIS_CMD]);
        return;
    } 

    if(ui32Len < 9) {
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad $Close (%s) from %s (%s) - user closed.", sData, pUser->sNick, pUser->sIP);

        pUser->Close();
        return;
    }

	if(clsScriptManager::mPtr->Arrival(pUser, sData, ui32Len, clsScriptManager::CLOSE_ARRIVAL) == true ||
		pUser->ui8State >= User::STATE_CLOSING) {
		return;
	}

    sData[ui32Len-1] = '\0'; // cutoff pipe

    User *OtherUser = clsHashManager::mPtr->FindUser(sData+7, ui32Len-8);
    if(OtherUser != NULL) {
        // Self-kick
        if(OtherUser == pUser) {
            pUser->SendFormat("clsDcCommands::Close2", true, "<%s> %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_CANT_CLOSE_YOURSELF]);
            return;
    	}
    	
        if(OtherUser->i32Profile != -1 && pUser->i32Profile > OtherUser->i32Profile) {
            pUser->SendFormat("clsDcCommands::Close3", true, "<%s> %s %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_ARE_NOT_ALLOWED_TO_CLOSE], OtherUser->sNick);
        	return;
        }

        // disconnect the user
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] User %s (%s) closed by %s", OtherUser->sNick, OtherUser->sIP, pUser->sNick);

        OtherUser->Close();
        
        if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
            clsGlobalDataQueue::mPtr->StatusMessageFormat("clsDcCommands::Close", "<%s> *** %s %s %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], OtherUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_WITH_IP], OtherUser->sIP, clsLanguageManager::mPtr->sTexts[LAN_WAS_CLOSED_BY], pUser->sNick);
        }
        
        if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
            pUser->SendFormat("clsDcCommands::Close4", true, "<%s> *** %s %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], OtherUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_WITH_IP], OtherUser->sIP, clsLanguageManager::mPtr->sTexts[LAN_WAS_CLOSED]);
        }
    }
}
//---------------------------------------------------------------------------

void clsDcCommands::Unknown(User * pUser, char * sData, const uint32_t &ui32Len, const bool &bMyNick/* = false*/) {
    iStatCmdUnknown++;

    #ifdef _DBG
        Memo(">>> Unimplemented Cmd "+pUser->Nick+" [" + pUser->IP + "]: " + sData);
    #endif

    // if we got unknown command sooner than full login finished
    // PPK ... fixed posibility to send (or flood !!!) hub with unknown command before full login
    // Give him chance with script...
    // if this is unkncwn command and script dont clarify that it's ok, disconnect the user
    if(clsScriptManager::mPtr->Arrival(pUser, sData, ui32Len, clsScriptManager::UNKNOWN_ARRIVAL) == false) {
        if(ui32Len > 65000) {
            sData[65000] = '\0';
        }

		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Unknown command from %s (%s) - user closed. (%s)", pUser->sNick, pUser->sIP, sData);

		if(bMyNick == true) {
			pUser->SendCharDelayed("$Error CTM2HUB|", 15);
		}

        pUser->Close();
    }
}
//---------------------------------------------------------------------------

bool clsDcCommands::ValidateUserNick(User * pUser, char * sNick, const size_t &szNickLen, const bool &ValidateNick) {
    // illegal characters in nick?
    for(uint32_t ui32i = 0; ui32i < szNickLen; ui32i++) {
        switch(sNick[ui32i]) {
            case ' ':
            case '$':
            case '|': {
                pUser->SendFormat("clsDcCommands::ValidateUserNick1", false, "<%s> %s '%c' ! %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOUR_NICK_CONTAINS_ILLEGAL_CHARACTER], sNick[ui32i], clsLanguageManager::mPtr->sTexts[LAN_PLS_CORRECT_IT_AND_GET_BACK_AGAIN]);

//				clsUdpDebug::mPtr->BroadcastFormat("[SYS] Nick with bad chars (%s) from %s (%s) - user closed.", Nick, pUser->sNick, pUser->sIP);

                pUser->Close();
                return false;
            }
            default:
                if((unsigned char)sNick[ui32i] < 32) {
                    pUser->SendFormat("clsDcCommands::ValidateUserNick2", false, "<%s> %s! %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOUR_NICK_CONTAINS_ILLEGAL_WHITE_CHARACTER], clsLanguageManager::mPtr->sTexts[LAN_PLS_CORRECT_IT_AND_GET_BACK_AGAIN]);

//					clsUdpDebug::mPtr->BroadcastFormat("[SYS] Nick with white chars (%s) from %s (%s) - user closed.", Nick, pUser->sNick, pUser->sIP);

                    pUser->Close();
                    return false;
                }

                continue;
        }
    }

    pUser->SetNick(sNick, (uint8_t)szNickLen);
    
    // check for reserved nicks
    if(clsReservedNicksManager::mPtr->CheckReserved(pUser->sNick, pUser->ui32NickHash) == true) {
        pUser->SendFormat("clsDcCommands::ValidateUserNick3", false, "<%s> %s. %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_THE_NICK_IS_RESERVED_FOR_SOMEONE_OTHER], clsLanguageManager::mPtr->sTexts[LAN_CHANGE_YOUR_NICK_AND_GET_BACK_AGAIN]);

//		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Reserved nick (%s) from %s (%s) - user closed.", Nick, pUser->sNick, pUser->sIP);

        pUser->Close();
        return false;
    }

    time_t acc_time;
    time(&acc_time);

    // PPK ... check if we already have ban for this user
    if(pUser->pLogInOut->pBan != NULL && pUser->ui32NickHash == pUser->pLogInOut->pBan->ui32NickHash) {
        pUser->SendChar(pUser->pLogInOut->pBan->sMessage, pUser->pLogInOut->pBan->ui32Len);
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Banned user %s (%s) - user closed.", pUser->sNick, pUser->sIP);

        pUser->Close();
        return false;
    }
    
    // check for banned nicks
    BanItem * pBan = clsBanManager::mPtr->FindNick(pUser);
    if(pBan != NULL) {
        int iMsgLen = GenerateBanMessage(pBan, acc_time);
        if(iMsgLen != 0) {
        	pUser->SendChar(clsServerManager::pGlobalBuffer, iMsgLen);
        }

		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Banned user %s (%s) - user closed.", pUser->sNick, pUser->sIP);

        pUser->Close();
        return false;
    }

    int32_t i32Profile = -1;

    // Nick is ok, check for registered nick
    RegUser *Reg = clsRegManager::mPtr->Find(pUser);
    if(Reg != NULL) {
        if(clsSettingManager::mPtr->bBools[SETBOOL_ADVANCED_PASS_PROTECTION] == true && Reg->ui8BadPassCount != 0) {
            time_t acc_time;
            time(&acc_time);

			uint32_t iMinutes2Wait = (uint32_t)pow(2.0, (double)Reg->ui8BadPassCount-1);

            if(acc_time < (time_t)(Reg->tLastBadPass+(60*iMinutes2Wait))) {
                pUser->SendFormat("clsDcCommands::ValidateUserNick4", false, "<%s> %s %s %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_LAST_PASS_WAS_WRONG_YOU_NEED_WAIT], formatSecTime((Reg->tLastBadPass+(60*iMinutes2Wait))-acc_time), 
					clsLanguageManager::mPtr->sTexts[LAN_BEFORE_YOU_TRY_AGAIN]);

				clsUdpDebug::mPtr->BroadcastFormat("[SYS] User %s (%s) not allowed to send password (%" PRIu64 ") - user closed.", pUser->sNick, pUser->sIP, (uint64_t)((Reg->tLastBadPass+(60*iMinutes2Wait))-acc_time));

                pUser->Close();
                return false;
            }
        }

        i32Profile = (int32_t)Reg->ui16Profile;
    }

    // PPK ... moved IP ban check here, we need to allow registered users on shared IP to log in if not have banned nick, but only IP.
    if(clsProfileManager::mPtr->IsProfileAllowed(i32Profile, clsProfileManager::ENTERIFIPBAN) == false) {
        // PPK ... check if we already have ban for this user
        if(pUser->pLogInOut->pBan != NULL) {
            pUser->SendChar(pUser->pLogInOut->pBan->sMessage, pUser->pLogInOut->pBan->ui32Len);

			clsUdpDebug::mPtr->BroadcastFormat("[SYS] uBanned user %s (%s) - user closed.", pUser->sNick, pUser->sIP);

            pUser->Close();
            return false;
        }
    }

    // PPK ... delete user ban if we have it
    if(pUser->pLogInOut->pBan != NULL) {
        delete pUser->pLogInOut->pBan;
        pUser->pLogInOut->pBan = NULL;
    }

    // first check for user limit ! PPK ... allow hublist pinger to check hub any time ;)
    if(clsProfileManager::mPtr->IsProfileAllowed(i32Profile, clsProfileManager::ENTERFULLHUB) == false && ((pUser->ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == false) {
        // user is NOT allowed enter full hub, check for maxClients
        if(clsServerManager::ui32Joins-clsServerManager::ui32Parts > (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_USERS]) {
            pUser->SendFormat("clsDcCommands::ValidateUserNick5", false, "$HubIsFull|<%s> %s. %u %s.|%s", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_THIS_HUB_IS_FULL], clsServerManager::ui32Logged, clsLanguageManager::mPtr->sTexts[LAN_USERS_ONLINE_LWR],
				(clsSettingManager::mPtr->bBools[SETBOOL_REDIRECT_WHEN_HUB_FULL] == true && clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_REDIRECT_ADDRESS] != NULL) ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_REDIRECT_ADDRESS] : "");
//			clsUdpDebug::mPtr->BroadcastFormat("[SYS] Hub full for %s (%s) - user closed.", pUser->sNick, pUser->sIP);

            pUser->Close();
            return false;
        }
    }

    // Check for maximum connections from same IP
    if(clsProfileManager::mPtr->IsProfileAllowed(i32Profile, clsProfileManager::NOUSRSAMEIP) == false) {
        uint32_t ui32Count = clsHashManager::mPtr->GetUserIpCount(pUser);
        if(ui32Count >= (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_CONN_SAME_IP]) {
            pUser->SendFormat("clsDcCommands::ValidateUserNick6", false, "<%s> %s.|%s", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_SORRY_ALREADY_MAX_IP_CONNS],
				(clsSettingManager::mPtr->bBools[SETBOOL_REDIRECT_WHEN_HUB_FULL] == true && clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_REDIRECT_ADDRESS] != NULL) ? clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_REDIRECT_ADDRESS] : "");

			int iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "[SYS] Max connections from same IP (%u) for %s (%s) - user closed. ", ui32Count, pUser->sNick, pUser->sIP);
            string tmp;
            if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsDcCommands::ValidateUserNick10") == true) {
                //clsUdpDebug::mPtr->Broadcast(clsServerManager::pGlobalBuffer, iMsgLen);
                tmp = clsServerManager::pGlobalBuffer;
            }

            User * cur = NULL,
                * nxt = clsHashManager::mPtr->FindUser(pUser->ui128IpHash);

            while(nxt != NULL) {
        		cur = nxt;
                nxt = cur->pHashIpTableNext;

                tmp += " "+string(cur->sNick, cur->ui8NickLen);
            }

            clsUdpDebug::mPtr->Broadcast(tmp.c_str(), tmp.size());

            pUser->Close();
            return false;
        }
    }

    // Check for reconnect time
    if(clsProfileManager::mPtr->IsProfileAllowed(i32Profile, clsProfileManager::NORECONNTIME) == false && clsUsers::mPtr->CheckRecTime(pUser) == true) {
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Fast reconnect from %s (%s) - user closed.", pUser->sNick, pUser->sIP);

        pUser->Close();
        return false;
    }

    pUser->ui8Country = clsIpP2Country::mPtr->Find(pUser->ui128IpHash);

    // check for nick in userlist. If taken, check for dupe's socket state
    // if still active, send $ValidateDenide and close()
    User *OtherUser = clsHashManager::mPtr->FindUser(pUser);

    if(OtherUser != NULL) {
   	    if(OtherUser->ui8State < User::STATE_CLOSING) {
            // check for socket error, or if user closed connection
            int iRet = recv(OtherUser->Sck, clsServerManager::pGlobalBuffer, 16, MSG_PEEK);

            // if socket error or user closed connection then allow new user to log in
#ifdef _WIN32
            if((iRet == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) || iRet == 0) {
#else
			if((iRet == -1 && errno != EAGAIN) || iRet == 0) {
#endif
                OtherUser->ui32BoolBits |= User::BIT_ERROR;

				clsUdpDebug::mPtr->BroadcastFormat("[SYS] Ghost in validate nick %s (%s) - user closed.", OtherUser->sNick, OtherUser->sIP);

                OtherUser->Close();
                return false;
            } else {
                if(Reg == NULL) {
                    pUser->SendFormat("clsDcCommands::ValidateUserNick7", false, "$ValidateDenide %s|", sNick);

                    if(strcmp(OtherUser->sIP, pUser->sIP) != 0 || strcmp(OtherUser->sNick, pUser->sNick) != 0) {
						clsUdpDebug::mPtr->BroadcastFormat("[SYS] Nick taken [%s (%s)] %s (%s) - user closed.", OtherUser->sNick, OtherUser->sIP, pUser->sNick, pUser->sIP);
                    }

                    pUser->Close();
                    return false;
                } else {
                    // PPK ... addition for registered users, kill your own ghost >:-]
                    pUser->ui8State = User::STATE_VERSION_OR_MYPASS;
                    pUser->ui32BoolBits |= User::BIT_WAITING_FOR_PASS;
                    pUser->AddPrcsdCmd(PrcsdUsrCmd::GETPASS, NULL, 0, NULL);
                    return true;
                }
            }
        }
    }
        
    if(Reg == NULL) {
        // user is NOT registered
        
        // nick length check
        if((clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_NICK_LEN] != 0 && szNickLen < (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_NICK_LEN]) || 
			(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_NICK_LEN] != 0 && szNickLen > (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_NICK_LEN]))
		{
            pUser->SendChar(clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_NICK_LIMIT_MSG], clsSettingManager::mPtr->ui16PreTextsLens[clsSettingManager::SETPRETXT_NICK_LIMIT_MSG]);
			clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad nick length (%d) from %s (%s) - user closed.", (int)szNickLen, pUser->sNick, pUser->sIP);

            pUser->Close();
            return false;
        }

        if(clsSettingManager::mPtr->bBools[SETBOOL_REG_ONLY] == true && ((pUser->ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == false) {
            pUser->SendChar(clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_REG_ONLY_MSG], clsSettingManager::mPtr->ui16PreTextsLens[clsSettingManager::SETPRETXT_REG_ONLY_MSG]);

//			clsUdpDebug::mPtr->BroadcastFormat("[SYS] Hub for reg only %s (%s) - user closed.", pUser->sNick, pUser->sIP);

            pUser->Close();
            return false;
        } else {
       	    // hub is public, proceed to Hello
            if(clsHashManager::mPtr->Add(pUser) == false) {
                return false;
            }

            pUser->ui32BoolBits |= User::BIT_HASHED;

            if(ValidateNick == true) {
                pUser->ui8State = User::STATE_VERSION_OR_MYPASS; // waiting for $Version
                pUser->AddPrcsdCmd(PrcsdUsrCmd::LOGINHELLO, NULL, 0, NULL);
            }
            return true;
        }
    } else {
        // user is registered, wait for password
        if(clsHashManager::mPtr->Add(pUser) == false) {
            return false;
        }

        pUser->ui32BoolBits |= User::BIT_HASHED;
        pUser->ui8State = User::STATE_VERSION_OR_MYPASS;
        pUser->ui32BoolBits |= User::BIT_WAITING_FOR_PASS;
        pUser->AddPrcsdCmd(PrcsdUsrCmd::GETPASS, NULL, 0, NULL);
        return true;
    }
}
//---------------------------------------------------------------------------

clsDcCommands::PassBf * clsDcCommands::Find(const uint8_t * ui128IpHash) {
	PassBf * cur = NULL,
        * next = PasswdBfCheck;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

		if(memcmp(cur->ui128IpHash, ui128IpHash, 16) == 0) {
            return cur;
        }
    }
    return NULL;
}
//---------------------------------------------------------------------------

void clsDcCommands::Remove(PassBf * PassBfItem) {
    if(PassBfItem->pPrev == NULL) {
        if(PassBfItem->pNext == NULL) {
            PasswdBfCheck = NULL;
        } else {
            PassBfItem->pNext->pPrev = NULL;
            PasswdBfCheck = PassBfItem->pNext;
        }
    } else if(PassBfItem->pNext == NULL) {
        PassBfItem->pPrev->pNext = NULL;
    } else {
        PassBfItem->pPrev->pNext = PassBfItem->pNext;
        PassBfItem->pNext->pPrev = PassBfItem->pPrev;
    }
	delete PassBfItem;
}
//---------------------------------------------------------------------------

void clsDcCommands::ProcessCmds(User * pUser) {
    pUser->ui32BoolBits &= ~User::BIT_CHAT_INSERT;

    PrcsdUsrCmd * cur = NULL,
        * next = pUser->pCmdStrt;
    
    while(next != NULL) {
        cur = next;
        next = cur->pNext;

        switch(cur->ui8Type) {
            case PrcsdUsrCmd::SUPPORTS: {
                memcpy(clsServerManager::pGlobalBuffer, "$Supports", 9);
                uint32_t iSupportsLen = 9;
                
                // PPK ... why dc++ have that 0 on end ? that was not in original documentation.. stupid duck...
                if(((pUser->ui32SupportBits & User::SUPPORTBIT_ZPIPE0) == User::SUPPORTBIT_ZPIPE0) == true) {
                    memcpy(clsServerManager::pGlobalBuffer+iSupportsLen, " ZPipe0", 7);
                    iSupportsLen += 7;
                } else if(((pUser->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == true) {
                    memcpy(clsServerManager::pGlobalBuffer+iSupportsLen, " ZPipe", 6);
                    iSupportsLen += 6;
                }
                
                // PPK ... yes yes yes finally QuickList support in PtokaX !!! ;))
                if((pUser->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) {
                    memcpy(clsServerManager::pGlobalBuffer+iSupportsLen, " QuickList", 10);
                    iSupportsLen += 10;
                } else if((pUser->ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) {
                    // PPK ... Hmmm Client not really need it, but for now send it ;-)
                    memcpy(clsServerManager::pGlobalBuffer+iSupportsLen, " NoHello", 8);
                    iSupportsLen += 8;
                } else if((pUser->ui32SupportBits & User::SUPPORTBIT_NOGETINFO) == User::SUPPORTBIT_NOGETINFO) {
                    // PPK ... if client support NoHello automatically supports NoGetINFO another badwith wasting !
                    memcpy(clsServerManager::pGlobalBuffer+iSupportsLen, " NoGetINFO", 10);
                    iSupportsLen += 10;
                }

                if((pUser->ui32SupportBits & User::SUPPORTBIT_IP64) == User::SUPPORTBIT_IP64) {
                    memcpy(clsServerManager::pGlobalBuffer+iSupportsLen, " IP64", 5);
                    iSupportsLen += 5;
                }

                if(((pUser->ui32SupportBits & User::SUPPORTBIT_IPV4) == User::SUPPORTBIT_IPV4) && ((pUser->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6)) {
                    // Only client connected with IPv6 sending this, so only that client is getting reply
                    memcpy(clsServerManager::pGlobalBuffer+iSupportsLen, " IPv4", 5);
                    iSupportsLen += 5;
                }

                if((pUser->ui32SupportBits & User::SUPPORTBIT_USERCOMMAND) == User::SUPPORTBIT_USERCOMMAND) {
                    memcpy(clsServerManager::pGlobalBuffer+iSupportsLen, " UserCommand", 12);
                    iSupportsLen += 12;
                }

                if((pUser->ui32SupportBits & User::SUPPORTBIT_USERIP2) == User::SUPPORTBIT_USERIP2 && ((pUser->ui32BoolBits & User::BIT_QUACK_SUPPORTS) == User::BIT_QUACK_SUPPORTS) == false) {
                    memcpy(clsServerManager::pGlobalBuffer+iSupportsLen, " UserIP2", 8);
                    iSupportsLen += 8;
                }

                if((pUser->ui32SupportBits & User::SUPPORTBIT_TLS2) == User::SUPPORTBIT_TLS2) {
                    memcpy(clsServerManager::pGlobalBuffer+iSupportsLen, " TLS2", 5);
                    iSupportsLen += 5;
                }

                if((pUser->ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) {
                    memcpy(clsServerManager::pGlobalBuffer+iSupportsLen, " BotINFO HubINFO", 16);
                    iSupportsLen += 16;
                }

                memcpy(clsServerManager::pGlobalBuffer+iSupportsLen, "|\0", 2);
                pUser->SendCharDelayed(clsServerManager::pGlobalBuffer, iSupportsLen+1);
                break;
            }
            case PrcsdUsrCmd::LOGINHELLO: {
                pUser->SendFormat("clsDcCommands::ProcessCmds", true, "$Hello %s|", pUser->sNick);
                break;
            }
            case PrcsdUsrCmd::GETPASS: {
                uint32_t ui32Len = 9;
                pUser->SendCharDelayed("$GetPass|", ui32Len); // query user for password
                break;
            }
            case PrcsdUsrCmd::CHAT: {
            	// find chat message data
                char *sBuff = cur->sCommand+pUser->ui8NickLen+3;

            	// non-command chat msg
                bool bNonChat = false;
            	for(uint8_t ui8i = 0; ui8i < (uint8_t)clsSettingManager::mPtr->ui16TextsLens[SETTXT_CHAT_COMMANDS_PREFIXES]; ui8i++) {
            	    if(sBuff[0] == clsSettingManager::mPtr->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][ui8i] ) {
                        bNonChat = true;
                	    break;
                    }
            	}

                if(bNonChat == true) {
                    // text files...
                    if(clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_TEXT_FILES] == true) {
                        cur->sCommand[cur->ui32Len-1] = '\0'; // get rid of the pipe
                        
                        if(clsTextFilesManager::mPtr->ProcessTextFilesCmd(pUser, sBuff+1)) {
                            break;
                        }
    
                        cur->sCommand[cur->ui32Len-1] = '|'; // add back pipe
                    }
    
                    // built-in commands
                    if(cur->ui32Len-pUser->ui8NickLen >= 9) {
                        if(clsHubCommands::DoCommand(pUser, sBuff-(pUser->ui8NickLen-1), cur->ui32Len)) break;
                        
                        cur->sCommand[cur->ui32Len-1] = '|'; // add back pipe
                    }
                }
           
            	// everything's ok, let's chat
            	clsUsers::mPtr->SendChat2All(pUser, cur->sCommand, cur->ui32Len, cur->pPtr);
            
                break;
            }
            case PrcsdUsrCmd::TO_OP_CHAT: {
                clsGlobalDataQueue::mPtr->SingleItemStore(cur->sCommand, cur->ui32Len, pUser, 0, clsGlobalDataQueue::SI_OPCHAT);
                break;
            }
        }

#ifdef _WIN32
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->sCommand) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate cur->sCommand in clsDcCommands::ProcessCmds\n");
        }
#else
		free(cur->sCommand);
#endif
        cur->sCommand = NULL;

        delete cur;
    }
    
    pUser->pCmdStrt = NULL;
    pUser->pCmdEnd = NULL;

    if((pUser->ui32BoolBits & User::BIT_PRCSD_MYINFO) == User::BIT_PRCSD_MYINFO) {
        pUser->ui32BoolBits &= ~User::BIT_PRCSD_MYINFO;

        if(((pUser->ui32BoolBits & User::BIT_HAVE_BADTAG) == User::BIT_HAVE_BADTAG) == true) {
            pUser->HasSuspiciousTag();
        }

        if(clsSettingManager::mPtr->ui8FullMyINFOOption == 0) {
            if(pUser->GenerateMyInfoLong() == false) {
                return;
            }

            clsUsers::mPtr->Add2MyInfosTag(pUser);

            if(clsSettingManager::mPtr->i16Shorts[SETSHORT_MYINFO_DELAY] == 0 || clsServerManager::ui64ActualTick > ((60*clsSettingManager::mPtr->i16Shorts[SETSHORT_MYINFO_DELAY])+pUser->iLastMyINFOSendTick)) {
				clsGlobalDataQueue::mPtr->AddQueueItem(pUser->sMyInfoLong, pUser->ui16MyInfoLongLen, NULL, 0, clsGlobalDataQueue::CMD_MYINFO);
                pUser->iLastMyINFOSendTick = clsServerManager::ui64ActualTick;
            } else {
				clsGlobalDataQueue::mPtr->AddQueueItem(pUser->sMyInfoLong, pUser->ui16MyInfoLongLen, NULL, 0, clsGlobalDataQueue::CMD_OPS);
            }

            return;
        } else if(clsSettingManager::mPtr->ui8FullMyINFOOption == 2) {
            if(pUser->GenerateMyInfoShort() == false) {
                return;
            }

            clsUsers::mPtr->Add2MyInfos(pUser);

            if(clsSettingManager::mPtr->i16Shorts[SETSHORT_MYINFO_DELAY] == 0 || clsServerManager::ui64ActualTick > ((60*clsSettingManager::mPtr->i16Shorts[SETSHORT_MYINFO_DELAY])+pUser->iLastMyINFOSendTick)) {
				clsGlobalDataQueue::mPtr->AddQueueItem(pUser->sMyInfoShort, pUser->ui16MyInfoShortLen, NULL, 0, clsGlobalDataQueue::CMD_MYINFO);
                pUser->iLastMyINFOSendTick = clsServerManager::ui64ActualTick;
            } else {
				clsGlobalDataQueue::mPtr->AddQueueItem(pUser->sMyInfoShort, pUser->ui16MyInfoShortLen, NULL, 0, clsGlobalDataQueue::CMD_OPS);
            }

            return;
		}

		if(pUser->GenerateMyInfoLong() == false) {
			return;
		}

		clsUsers::mPtr->Add2MyInfosTag(pUser);

		char * sShortMyINFO = NULL;
		size_t szShortMyINFOLen = 0;

		if(pUser->GenerateMyInfoShort() == true) {
			clsUsers::mPtr->Add2MyInfos(pUser);

			if(clsSettingManager::mPtr->i16Shorts[SETSHORT_MYINFO_DELAY] == 0 || clsServerManager::ui64ActualTick > ((60*clsSettingManager::mPtr->i16Shorts[SETSHORT_MYINFO_DELAY])+pUser->iLastMyINFOSendTick)) {
				sShortMyINFO = pUser->sMyInfoShort;
				szShortMyINFOLen = pUser->ui16MyInfoShortLen;
				pUser->iLastMyINFOSendTick = clsServerManager::ui64ActualTick;
			}
		}

		clsGlobalDataQueue::mPtr->AddQueueItem(sShortMyINFO, szShortMyINFOLen, pUser->sMyInfoLong, pUser->ui16MyInfoLongLen, clsGlobalDataQueue::CMD_MYINFO);
    }
}
//---------------------------------------------------------------------------

bool CheckPort(char * sData, char cPortEnd) {
	for(uint8_t ui8i = 0; ui8i < 6; ui8i++) {
		if(isdigit(sData[ui8i]) != 0) {
			continue;
		} else if(sData[ui8i] == cPortEnd || (cPortEnd == '|' && sData[ui8i] == 'S' && sData[ui8i+1] == cPortEnd)) {
			char cEnd = sData[ui8i];
			sData[ui8i] = '\0';

    		int iPort = atoi(sData);
    		if(ui8i != 0 && iPort > 0 && iPort < 65536) {
				sData[ui8i] = cEnd;

				return false;
    		}
		}

		break;
	}

	return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool clsDcCommands::CheckIPPort(const User * pUser, char * sIP, bool &bWrongPort, uint16_t &ui16Port, uint8_t &ui8AfterPortLen, char cPortEnd) {
    if((pUser->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
        if(sIP[0] == '[' && sIP[1+pUser->ui8IpLen] == ']' && sIP[2+pUser->ui8IpLen] == ':' && strncmp(sIP+1, pUser->sIP, pUser->ui8IpLen) == 0) {
        	bWrongPort = CheckPort(sIP+3+pUser->ui8IpLen, cPortEnd);
            return true;
        } else if(((pUser->ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4) && sIP[pUser->ui8IPv4Len] == ':' && strncmp(sIP, pUser->sIPv4, pUser->ui8IPv4Len) == 0) {
        	bWrongPort = CheckPort(sIP+pUser->ui8IPv4Len+1, cPortEnd);
            return true;
        }
    } else if(sIP[pUser->ui8IpLen] == ':' && strncmp(sIP, pUser->sIP, pUser->ui8IpLen) == 0) {
    	bWrongPort = CheckPort(sIP+pUser->ui8IpLen+1, cPortEnd);
        return true;
    }

	bWrongPort = GetPort(sIP, ui16Port, ui8AfterPortLen, cPortEnd);

	return false;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool clsDcCommands::GetPort(char * sData, uint16_t &ui16Port, uint8_t &ui8AfterPortLen, char cPortEnd) {
    char * sPortEnd = strchr(sData, cPortEnd);
    if(sPortEnd == NULL) {
        return true;
    }

    sPortEnd[0] = '\0';

    char * sPortStart = strrchr(sData, ':');
    if(sPortStart == NULL || sPortStart[1] == '\0') {
        return true;
    }

    sPortStart[0] = '\0';

    int iPort = atoi(sPortStart+1);
    if(iPort < 1 || iPort > 65535) {
        return true;
    }

    ui16Port = (uint16_t)iPort;

	ui8AfterPortLen = (uint8_t)(sPortEnd-sData)+1;

    return false;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsDcCommands::SendIncorrectPortMsg(User * pUser, const bool &bCTM) {
	pUser->SendFormat("clsDcCommands::SendIncorrectPortMsg", true, "<%s> %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], bCTM == true ? clsLanguageManager::mPtr->sTexts[LAN_YOUR_CLIENT_SEND_INCORRECT_PORT_IN_CTM] : clsLanguageManager::mPtr->sTexts[LAN_YOUR_CLIENT_SEND_INCORRECT_PORT_IN_SEARCH]);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsDcCommands::SendIncorrectIPMsg(User * pUser, char * sBadIP, const bool &bCTM) {
	int iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "<%s> %s ", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOUR_CLIENT_SEND_INCORRECT_IP]);
	if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "SendIncorrectIPMsg1") == false) {
		return;
	}

    if((pUser->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6 && sBadIP[0] == '[') {
        uint8_t ui8i = 1;
        while(sBadIP[ui8i] != '\0') {
            if(isxdigit(sBadIP[ui8i]) == false && sBadIP[ui8i] != ':') {
                if(ui8i == 0) {
                    iMsgLen--;
                }

                break;
            }

            clsServerManager::pGlobalBuffer[iMsgLen] = sBadIP[ui8i];
            iMsgLen++;

            ui8i++;
        }
    } else {
        uint8_t ui8i = 0;
        while(sBadIP[ui8i] != '\0') {
            if(isdigit(sBadIP[ui8i]) == false && sBadIP[ui8i] != '.') {
                if(ui8i == 0) {
                    iMsgLen--;
                }

                break;
            }

            clsServerManager::pGlobalBuffer[iMsgLen] = sBadIP[ui8i];
            iMsgLen++;

            ui8i++;
        }
    }

	int iret = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, " %s %s !|", bCTM == true ? clsLanguageManager::mPtr->sTexts[LAN_IN_CTM_REQ_REAL_IP_IS] : clsLanguageManager::mPtr->sTexts[LAN_IN_SEARCH_REQ_REAL_IP_IS], pUser->sIP);
	iMsgLen += iret;
	if(CheckSprintf1(iret, iMsgLen, clsServerManager::szGlobalBufferSize, "SendIncorrectIPMsg2") == true) {
		pUser->SendCharDelayed(clsServerManager::pGlobalBuffer, iMsgLen);
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsDcCommands::SendIPFixedMsg(User * pUser, char * sBadIP, char * sRealIP) {
    if((pUser->ui32BoolBits & User::BIT_WARNED_WRONG_IP) == User::BIT_WARNED_WRONG_IP) {
        return;
    }

    pUser->SendFormat("clsDcCommands::SendIPFixedMsg", true, "<%s> %s %s %s %s !|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOUR_CLIENT_SEND_INCORRECT_IP], sBadIP,
        clsLanguageManager::mPtr->sTexts[LAN_IN_COMMAND_HUB_REPLACED_IT_WITH_YOUR_REAL_IP], sRealIP);

    pUser->ui32BoolBits |= User::BIT_WARNED_WRONG_IP;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsDcCommands::MyNick(User * pUser, char * sData, const uint32_t &ui32Len) {
    if((pUser->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] IPv6 $MyNick (%s) from %s (%s) - user closed.", sData, pUser->sNick, pUser->sIP);

        Unknown(pUser, sData, ui32Len, true);
        return;
    }

    if(ui32Len < 10) {
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Short $MyNick (%s) from %s (%s) - user closed.", sData, pUser->sNick, pUser->sIP);

        Unknown(pUser, sData, ui32Len, true);
        return;
    }

    sData[ui32Len-1] = '\0'; // cutoff pipe

    User * pOtherUser = clsHashManager::mPtr->FindUser(sData+8, ui32Len-9);

    if(pOtherUser == NULL || pOtherUser->ui8State != User::STATE_IPV4_CHECK) {
		clsUdpDebug::mPtr->BroadcastFormat("[SYS] Bad $MyNick (%s) from %s (%s) - user closed.", sData, pUser->sNick, pUser->sIP);

        Unknown(pUser, sData, ui32Len, true);
        return;
    }

    strcpy(pOtherUser->sIPv4, pUser->sIP);
    pOtherUser->ui8IPv4Len = pUser->ui8IpLen;
    pOtherUser->ui32BoolBits |= User::BIT_IPV4;

    pOtherUser->ui8State = User::STATE_ADDME;

    pUser->Close();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

PrcsdUsrCmd * clsDcCommands::AddSearch(User * pUser, PrcsdUsrCmd * cmdSearch, char * sSearch, const size_t &szLen, const bool &bActive) const {
    if(cmdSearch != NULL) {
        char * pOldBuf = cmdSearch->sCommand;
#ifdef _WIN32
        cmdSearch->sCommand = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, cmdSearch->ui32Len+szLen+1);
#else
		cmdSearch->sCommand = (char *)realloc(pOldBuf, cmdSearch->ui32Len+szLen+1);
#endif
        if(cmdSearch->sCommand == NULL) {
            cmdSearch->sCommand = pOldBuf;
            pUser->ui32BoolBits |= User::BIT_ERROR;
            pUser->Close();

			AppendDebugLogFormat("[MEM] Cannot reallocate %" PRIu64 " bytes for clsDcCommands::AddSearch1\n", (uint64_t)(cmdSearch->ui32Len+szLen+1));

            return cmdSearch;
        }
        memcpy(cmdSearch->sCommand+cmdSearch->ui32Len, sSearch, szLen);
        cmdSearch->ui32Len += (uint32_t)szLen;
        cmdSearch->sCommand[cmdSearch->ui32Len] = '\0';

        if(bActive == true) {
            clsUsers::mPtr->ui16ActSearchs++;
        } else {
            clsUsers::mPtr->ui16PasSearchs++;
        }
    } else {
        cmdSearch = new (std::nothrow) PrcsdUsrCmd();
        if(cmdSearch == NULL) {
            pUser->ui32BoolBits |= User::BIT_ERROR;
            pUser->Close();

			AppendDebugLog("%s - [MEM] Cannot allocate new cmdSearch in clsDcCommands::AddSearch1\n");
            return cmdSearch;
        }

#ifdef _WIN32
        cmdSearch->sCommand = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szLen+1);
#else
		cmdSearch->sCommand = (char *)malloc(szLen+1);
#endif
		if(cmdSearch->sCommand == NULL) {
            delete cmdSearch;

            pUser->ui32BoolBits |= User::BIT_ERROR;
            pUser->Close();

			AppendDebugLogFormat("[MEM] Cannot allocate %" PRIu64 " bytes for DcCommands::Search5\n", (uint64_t)(szLen+1));

            return NULL;
        }

        memcpy(cmdSearch->sCommand, sSearch, szLen);
        cmdSearch->sCommand[szLen] = '\0';

        cmdSearch->ui32Len = (uint32_t)szLen;

        if(bActive == true) {
            clsUsers::mPtr->ui16ActSearchs++;
        } else {
            clsUsers::mPtr->ui16PasSearchs++;
        }
    }

    return cmdSearch;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
