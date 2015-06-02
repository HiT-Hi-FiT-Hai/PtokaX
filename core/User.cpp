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
#include "User.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "DcCommands.h"
#include "GlobalDataQueue.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "utility.h"
#include "UdpDebug.h"
#include "ZlibUtility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#ifdef _WITH_SQLITE
	#include "DB-SQLite.h"
#elif _WITH_POSTGRES
	#include "DB-PostgreSQL.h"
#elif _WITH_MYSQL
	#include "DB-MySQL.h"
#endif
#include "DeFlood.h"
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
	#include "../gui.win/GuiUtil.h"
    #include "../gui.win/MainWindowPageUsersChat.h"
#endif
//---------------------------------------------------------------------------
static const size_t ZMINDATALEN = 128;
static const char * sBadTag = "BAD TAG!"; // 8
static const char * sOtherNoTag = "OTHER (NO TAG)"; // 14
static const char * sUnknownTag = "UNKNOWN TAG"; // 11
static const char * sDefaultNick = "<unknown>"; // 9
//---------------------------------------------------------------------------
static char msg[1024];
//---------------------------------------------------------------------------

static bool UserProcessLines(User * u, const uint32_t &iStrtLen) {
	// nothing to process?
	if(u->pRecvBuf[0] == '\0')
        return false;

    char c = 0;
    
    char * buffer = u->pRecvBuf;

    for(uint32_t ui32i = iStrtLen; ui32i < u->ui32RecvBufDataLen; ui32i++) {
        if(u->pRecvBuf[ui32i] == '|') {
            // look for pipes in the data - process lines one by one
            c = u->pRecvBuf[ui32i+1];
            u->pRecvBuf[ui32i+1] = '\0';
            uint32_t ui32iCommandLen = (uint32_t)(((u->pRecvBuf+ui32i)-buffer)+1);
            if(buffer[0] == '|') {
                //UdpDebug->Broadcast("[SYS] heartbeat from " + string(u->Nick, u->NickLen));
                //send(Sck, "|", 1, 0);
            } else if(ui32iCommandLen < (u->ui8State < User::STATE_ADDME ? 1024U : 65536U)) {
        		clsDcCommands::mPtr->PreProcessData(u, buffer, true, ui32iCommandLen);
        	} else {
                int imsgLen = sprintf(msg, "<%s> %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_CMD_TOO_LONG]);
                if(CheckSprintf(imsgLen, 1024, "UserProcessLines1") == true) {
                    u->SendCharDelayed(msg, imsgLen);
                }
				u->Close();
				clsUdpDebug::mPtr->Broadcast("[SYS] " + string(u->sNick, u->ui8NickLen) + " (" + string(u->sIP, u->ui8IpLen) + ") Received command too long. User disconnected.");
                return false;
            }
        	u->pRecvBuf[ui32i+1] = c;
        	buffer += ui32iCommandLen;
            if(u->ui8State >= User::STATE_CLOSING) {
                return true;
            }
        } else if(u->pRecvBuf[ui32i] == '\0') {
            // look for NULL character and replace with zero
            u->pRecvBuf[ui32i] = '0';
            continue;
        }
	}

	u->ui32RecvBufDataLen -= (uint32_t)(buffer-u->pRecvBuf);

	if(u->ui32RecvBufDataLen == 0) {
        clsDcCommands::mPtr->ProcessCmds(u);
        u->pRecvBuf[0] = '\0';
        return false;
    } else if(u->ui32RecvBufDataLen != 1) {
        if(u->ui32RecvBufDataLen > unsigned (u->ui8State < User::STATE_ADDME ? 1024 : 65536)) {
            // PPK ... we don't want commands longer than 64 kB, drop this user !
            int imsgLen = sprintf(msg, "<%s> %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_CMD_TOO_LONG]);
            if(CheckSprintf(imsgLen, 1024, "UserProcessLines2") == true) {
                u->SendCharDelayed(msg, imsgLen);
            }
            u->Close();
			clsUdpDebug::mPtr->Broadcast("[SYS] " + string(u->sNick, u->ui8NickLen) + " (" + string(u->sIP, u->ui8IpLen) +") RecvBuffer overflow. User disconnected.");
        	return false;
        }
        clsDcCommands::mPtr->ProcessCmds(u);
        memmove(u->pRecvBuf, buffer, u->ui32RecvBufDataLen);
        u->pRecvBuf[u->ui32RecvBufDataLen] = '\0';
        return true;
    } else {
        clsDcCommands::mPtr->ProcessCmds(u);
        if(buffer[0] == '|') {
            u->pRecvBuf[0] = '\0';
            u->ui32RecvBufDataLen = 0;
            return false;
        } else {
            u->pRecvBuf[0] = buffer[0];
            u->pRecvBuf[1] = '\0';
            return true;
        }
    }
}
//------------------------------------------------------------------------------

static void UserSetBadTag(User * u, char * Descr, uint8_t DescrLen) {
    // PPK ... clear all tag related things
    u->sTagVersion = NULL;
    u->ui8TagVersionLen = 0;

    u->sModes[0] = '\0';
    u->Hubs = u->Slots = u->OLimit = u->LLimit = u->DLimit = u->iNormalHubs = u->iRegHubs = u->iOpHubs = 0;
    u->ui32BoolBits |= User::BIT_OLDHUBSTAG;
    u->ui32BoolBits |= User::BIT_HAVE_BADTAG;
    
    u->sDescription = Descr;
    u->ui8DescriptionLen = (uint8_t)DescrLen;

    // PPK ... clear (fake) tag
    u->sTag = NULL;
    u->ui8TagLen = 0;

    // PPK ... set bad tag
    u->sClient = (char *)sBadTag;
    u->ui8ClientLen = 8;

    // PPK ... send report to udp debug
    int imsgLen = sprintf(msg, "[SYS] User %s (%s) have bad TAG (%s) ?!?.", u->sNick, u->sIP, u->sMyInfoOriginal);
    if(CheckSprintf(imsgLen, 1024, "SetBadTag") == true) {
        clsUdpDebug::mPtr->Broadcast(msg, imsgLen);
    }
}
//---------------------------------------------------------------------------

static void UserParseMyInfo(User * u) {
    memcpy(msg, u->sMyInfoOriginal, u->ui16MyInfoOriginalLen);

    char *sMyINFOParts[] = { NULL, NULL, NULL, NULL, NULL };
    uint16_t iMyINFOPartsLen[] = { 0, 0, 0, 0, 0 };

    unsigned char cPart = 0;

    sMyINFOParts[cPart] = msg+14+u->ui8NickLen; // desription start


    for(uint32_t ui32i = 14+u->ui8NickLen; ui32i < u->ui16MyInfoOriginalLen-1u; ui32i++) {
        if(msg[ui32i] == '$') {
            msg[ui32i] = '\0';
            iMyINFOPartsLen[cPart] = (uint16_t)((msg+ui32i)-sMyINFOParts[cPart]);

            // are we on end of myinfo ???
            if(cPart == 4)
                break;

            cPart++;
            sMyINFOParts[cPart] = msg+ui32i+1;
        }
    }

    // check if we have all myinfo parts, connection and sharesize must have length more than 0 !
    if(sMyINFOParts[0] == NULL || sMyINFOParts[1] == NULL || iMyINFOPartsLen[1] != 1 || sMyINFOParts[2] == NULL || iMyINFOPartsLen[2] == 0 ||
        sMyINFOParts[3] == NULL || sMyINFOParts[4] == NULL || iMyINFOPartsLen[4] == 0) {
        int imsgLen = sprintf(msg, "<%s> %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_MyINFO_IS_CORRUPTED]);
        if(CheckSprintf(imsgLen, 1024, "UserParseMyInfo1") == true) {
            u->SendChar(msg, imsgLen);
        }
        imsgLen = sprintf(msg, "[SYS] User %s (%s) with bad MyINFO (%s) disconnected.", u->sNick, u->sIP, u->sMyInfoOriginal);
        if(CheckSprintf(imsgLen, 1024, "UserParseMyInfo2") == true) {
            clsUdpDebug::mPtr->Broadcast(msg, imsgLen);
        }
        u->Close();
        return;
    }

    // connection
    u->ui8MagicByte = sMyINFOParts[2][iMyINFOPartsLen[2]-1];
    u->sConnection = u->sMyInfoOriginal+(sMyINFOParts[2]-msg);
    u->ui8ConnectionLen = (uint8_t)(iMyINFOPartsLen[2]-1);

    // email
    if(iMyINFOPartsLen[3] != 0) {
        u->sEmail = u->sMyInfoOriginal+(sMyINFOParts[3]-msg);
        u->ui8EmailLen = (uint8_t)iMyINFOPartsLen[3];
    }
    
    // share
    // PPK ... check for valid numeric share, kill fakers !
    if(HaveOnlyNumbers(sMyINFOParts[4], iMyINFOPartsLen[4]) == false) {
        //clsUdpDebug::mPtr->Broadcast("[SYS] User " + string(u->Nick, u->NickLen) + " (" + string(u->IP, u->ui8IpLen) + ") with non-numeric sharesize disconnected.");
        u->Close();
        return;
    }
            
    if(((u->ui32BoolBits & User::BIT_HAVE_SHARECOUNTED) == User::BIT_HAVE_SHARECOUNTED) == true) {
        clsServerManager::ui64TotalShare -= u->ui64SharedSize;
#ifdef _WIN32
        u->ui64SharedSize = _strtoui64(sMyINFOParts[4], NULL, 10);
#else
		u->ui64SharedSize = strtoull(sMyINFOParts[4], NULL, 10);
#endif
        clsServerManager::ui64TotalShare += u->ui64SharedSize;
    } else {
#ifdef _WIN32
        u->ui64SharedSize = _strtoui64(sMyINFOParts[4], NULL, 10);
#else
		u->ui64SharedSize = strtoull(sMyINFOParts[4], NULL, 10);
#endif
    }

    // Reset all tag infos...
    u->sModes[0] = '\0';
    u->Hubs = 0;
    u->iNormalHubs = 0;
    u->iRegHubs = 0;
    u->iOpHubs =0;
    u->Slots = 0;
    u->OLimit = 0;
    u->LLimit = 0;
    u->DLimit = 0;
    
    // description
    if(iMyINFOPartsLen[0] != 0) {
        if(sMyINFOParts[0][iMyINFOPartsLen[0]-1] == '>') {
            char *DCTag = strrchr(sMyINFOParts[0], '<');
            if(DCTag == NULL) {               
                u->sDescription = u->sMyInfoOriginal+(sMyINFOParts[0]-msg);
                u->ui8DescriptionLen = (uint8_t)iMyINFOPartsLen[0];

                u->sClient = (char*)sOtherNoTag;
                u->ui8ClientLen = 14;
                return;
            }

            u->sTag = u->sMyInfoOriginal+(DCTag-msg);
            u->ui8TagLen = (uint8_t)(iMyINFOPartsLen[0]-(DCTag-sMyINFOParts[0]));

            static const uint16_t ui16plusplus = *((uint16_t *)"++");
            if(DCTag[3] == ' ' && *((uint16_t *)(DCTag+1)) == ui16plusplus) {
                u->ui32SupportBits |= User::SUPPORTBIT_NOHELLO;
            }

            static const uint16_t ui16V = *((uint16_t *)"V:");

            char * sTemp = strchr(DCTag, ' ');

            if(sTemp != NULL && *((uint16_t *)(sTemp+1)) == ui16V) {
                sTemp[0] = '\0';
                u->sClient = u->sMyInfoOriginal+((DCTag+1)-msg);
                u->ui8ClientLen = (uint8_t)((sTemp-DCTag)-1);
            } else {
                u->sClient = (char *)sUnknownTag;
                u->ui8ClientLen = 11;
                u->sTag = NULL;
                u->ui8TagLen = 0;
                sMyINFOParts[0][iMyINFOPartsLen[0]-1] = '>'; // not valid DC Tag, add back > tag ending
                u->sDescription = u->sMyInfoOriginal+(sMyINFOParts[0]-msg);
                u->ui8DescriptionLen = (uint8_t)iMyINFOPartsLen[0];
                return;
            }

            size_t szTagPattLen = ((sTemp-DCTag)+1);

            sMyINFOParts[0][iMyINFOPartsLen[0]-1] = ','; // terminate tag end with ',' for easy tag parsing

            uint32_t reqVals = 0;
            char * sTagPart = DCTag+szTagPattLen;

            for(size_t szi = szTagPattLen; szi < (size_t)(iMyINFOPartsLen[0]-(DCTag-sMyINFOParts[0])); szi++) {
                if(DCTag[szi] == ',') {
                    DCTag[szi] = '\0';
                    if(sTagPart[1] != ':') {
                        UserSetBadTag(u, u->sMyInfoOriginal+(sMyINFOParts[0]-msg), (uint8_t)iMyINFOPartsLen[0]);
                        return;
                    }

                    switch(sTagPart[0]) {
                        case 'V':
                            // PPK ... fix for potencial memory leak with fake tag
                            if(sTagPart[2] == '\0' || u->sTagVersion) {
                                UserSetBadTag(u, u->sMyInfoOriginal+(sMyINFOParts[0]-msg), (uint8_t)iMyINFOPartsLen[0]);
                                return;
                            }
                            u->sTagVersion = u->sMyInfoOriginal+((sTagPart+2)-msg);
                            u->ui8TagVersionLen = (uint8_t)((DCTag+szi)-(sTagPart+2));
                            reqVals++;
                            break;
                        case 'M':
                            if((u->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6 && (u->ui32SupportBits & User::SUPPORTBIT_IP64) == User::SUPPORTBIT_IP64) {
                                if(sTagPart[2] == '\0' || sTagPart[3] == '\0' || sTagPart[4] != '\0') {
                                    UserSetBadTag(u, u->sMyInfoOriginal+(sMyINFOParts[0]-msg), (uint8_t)iMyINFOPartsLen[0]);
                                    return;
                                }
                                u->sModes[0] = sTagPart[2];
                                u->sModes[1] = sTagPart[3];
                                u->sModes[2] = '\0';

                                if(toupper(sTagPart[3]) == 'A') {
                                    u->ui32BoolBits |= User::BIT_IPV6_ACTIVE;
                                } else {
                                    u->ui32BoolBits &= ~User::BIT_IPV6_ACTIVE;
                                }
                            } else {
                                if(sTagPart[2] == '\0' || sTagPart[3] != '\0') {
                                    UserSetBadTag(u, u->sMyInfoOriginal+(sMyINFOParts[0]-msg), (uint8_t)iMyINFOPartsLen[0]);
                                    return;
                                }
                                u->sModes[0] = sTagPart[2];
                                u->sModes[1] = '\0';
                            }

                            if(toupper(sTagPart[2]) == 'A') {
                                u->ui32BoolBits |= User::BIT_IPV4_ACTIVE;
                            } else {
                                u->ui32BoolBits &= ~User::BIT_IPV4_ACTIVE;
                            }

                            reqVals++;
                            break;
                        case 'H': {
                            if(sTagPart[2] == '\0') {
                                UserSetBadTag(u, u->sMyInfoOriginal+(sMyINFOParts[0]-msg), (uint8_t)iMyINFOPartsLen[0]);
                                return;
                            }

                            DCTag[szi] = '/';

                            char *sHubsParts[] = { NULL, NULL, NULL };
                            uint16_t iHubsPartsLen[] = { 0, 0, 0 };

                            uint8_t cPart = 0;

                            sHubsParts[cPart] = sTagPart+2;


                            for(uint32_t ui32j = 3; ui32j < (uint32_t)((DCTag+szi+1)-sTagPart); ui32j++) {
                                if(sTagPart[ui32j] == '/') {
                                    sTagPart[ui32j] = '\0';
                                    iHubsPartsLen[cPart] = (uint16_t)((sTagPart+ui32j)-sHubsParts[cPart]);

                                    // are we on end of hubs tag part ???
                                    if(cPart == 2)
                                        break;

                                    cPart++;
                                    sHubsParts[cPart] = sTagPart+ui32j+1;
                                }
                            }

                            if(sHubsParts[0] != NULL && sHubsParts[1] != NULL && sHubsParts[2] != NULL) {
                                if(iHubsPartsLen[0] != 0 && iHubsPartsLen[1] != 0 && iHubsPartsLen[2] != 0) {
                                    if(HaveOnlyNumbers(sHubsParts[0], iHubsPartsLen[0]) == false ||
                                        HaveOnlyNumbers(sHubsParts[1], iHubsPartsLen[1]) == false ||
                                        HaveOnlyNumbers(sHubsParts[2], iHubsPartsLen[2]) == false) {
                                        UserSetBadTag(u, u->sMyInfoOriginal+(sMyINFOParts[0]-msg), (uint8_t)iMyINFOPartsLen[0]);
                                        return;
                                    }
                                    u->iNormalHubs = atoi(sHubsParts[0]);
                                    u->iRegHubs = atoi(sHubsParts[1]);
                                    u->iOpHubs = atoi(sHubsParts[2]);
                                    u->Hubs = u->iNormalHubs+u->iRegHubs+u->iOpHubs;
                                    // PPK ... kill LAM3R with fake hubs
                                    if(u->Hubs != 0) {
                                        u->ui32BoolBits &= ~User::BIT_OLDHUBSTAG;
                                        reqVals++;
                                        break;
                                    }
                                }
                            } else if(sHubsParts[1] == DCTag+szi+1 && sHubsParts[2] == NULL) {
                                DCTag[szi] = '\0';
                                u->Hubs = atoi(sHubsParts[0]);
                                reqVals++;
                                u->ui32BoolBits |= User::BIT_OLDHUBSTAG;
                                break;
                            }
                            int imsgLen = sprintf(msg, "<%s> %s!|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_FAKE_TAG]);
                            if(CheckSprintf(imsgLen, 1024, "UserParseMyInfo3") == true) {
                                u->SendChar(msg, imsgLen);
                            }
                            imsgLen = sprintf(msg, "[SYS] User %s (%s) with fake Tag disconnected: ", u->sNick, u->sIP);
                            if(CheckSprintf(imsgLen, 1024, "UserParseMyInfo4") == true) {
                                memcpy(msg+imsgLen, u->sTag, u->ui8TagLen);
                                clsUdpDebug::mPtr->Broadcast(msg, imsgLen+(int)u->ui8TagLen);
                            }
                            u->Close();
                            return;
                        }
                        case 'S':
                            if(sTagPart[2] == '\0') {
                                UserSetBadTag(u, u->sMyInfoOriginal+(sMyINFOParts[0]-msg), (uint8_t)iMyINFOPartsLen[0]);
                                return;
                            }
                            if(HaveOnlyNumbers(sTagPart+2, (uint16_t)strlen(sTagPart+2)) == false) {
                                UserSetBadTag(u, u->sMyInfoOriginal+(sMyINFOParts[0]-msg), (uint8_t)iMyINFOPartsLen[0]);
                                return;
                            }
                            u->Slots = atoi(sTagPart+2);
                            reqVals++;
                            break;
                        case 'O':
                            if(sTagPart[2] == '\0') {
                                UserSetBadTag(u, u->sMyInfoOriginal+(sMyINFOParts[0]-msg), (uint8_t)iMyINFOPartsLen[0]);
                                return;
                            }
                            u->OLimit = atoi(sTagPart+2);
                            break;
                        case 'B':
                            if(sTagPart[2] == '\0') {
                                UserSetBadTag(u, u->sMyInfoOriginal+(sMyINFOParts[0]-msg), (uint8_t)iMyINFOPartsLen[0]);
                                return;
                            }
                            u->LLimit = atoi(sTagPart+2);
                            break;
                        case 'L':
                            if(sTagPart[2] == '\0') {
                                UserSetBadTag(u, u->sMyInfoOriginal+(sMyINFOParts[0]-msg), (uint8_t)iMyINFOPartsLen[0]);
                                return;
                            }
                            u->LLimit = atoi(sTagPart+2);
                            break;
                        case 'D':
                            if(sTagPart[2] == '\0') {
                                UserSetBadTag(u, u->sMyInfoOriginal+(sMyINFOParts[0]-msg), (uint8_t)iMyINFOPartsLen[0]);
                                return;
                            }
                            u->DLimit = atoi(sTagPart+2);
                            break;
                        default:
                            //clsUdpDebug::mPtr->Broadcast("[SYS] "+string(u->Nick, u->NickLen)+": Extra info in DC tag: "+string(sTag));
                            break;
                    }
                    sTagPart = DCTag+szi+1;
                }
            }
                
            if(reqVals < 4) {
                UserSetBadTag(u, u->sMyInfoOriginal+(sMyINFOParts[0]-msg), (uint8_t)iMyINFOPartsLen[0]);
                return;
            } else {
                u->sDescription = u->sMyInfoOriginal+(sMyINFOParts[0]-msg);
                u->ui8DescriptionLen = (uint8_t)(DCTag-sMyINFOParts[0]);
                return;
            }
        } else {
            u->sDescription = u->sMyInfoOriginal+(sMyINFOParts[0]-msg);
            u->ui8DescriptionLen = (uint8_t)iMyINFOPartsLen[0];
        }
    }

    u->sClient = (char *)sOtherNoTag;
    u->ui8ClientLen = 14;

    u->sTag = NULL;
    u->ui8TagLen = 0;

    u->sTagVersion = NULL;
    u->ui8TagVersionLen = 0;

    u->sModes[0] = '\0';
    u->Hubs = 0;
    u->iNormalHubs = 0;
    u->iRegHubs = 0;
    u->iOpHubs =0;
    u->Slots = 0;
    u->OLimit = 0;
    u->LLimit = 0;
    u->DLimit = 0;
}
//---------------------------------------------------------------------------

UserBan::UserBan() : sMessage(NULL), ui32Len(0), ui32NickHash(0) {
    // ...
}
//---------------------------------------------------------------------------

UserBan::~UserBan() {
#ifdef _WIN32
    if(sMessage != NULL && HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMessage) == 0) {
        AppendDebugLog("%s - [MEM] Cannot deallocate sMessage in UserBan::~UserBan\n", 0);
    }
#else
	free(sMessage);
#endif
    sMessage = NULL;
}
//---------------------------------------------------------------------------

UserBan * UserBan::CreateUserBan(char * sMess, const uint32_t &iMessLen, const uint32_t &ui32Hash) {
    UserBan * pUserBan = new (std::nothrow) UserBan();

    if(pUserBan == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate new pUserBan in UserBan::CreateUserBan\n", 0);

        return NULL;
    }

#ifdef _WIN32
    pUserBan->sMessage = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, iMessLen+1);
#else
	pUserBan->sMessage = (char *)malloc(iMessLen+1);
#endif
    if(pUserBan->sMessage == NULL) {
        AppendDebugLog("%s - [MEM] UserBan::CreateUserBan cannot allocate %" PRIu64 " bytes for sMessage\n", (uint64_t)(iMessLen+1));

        delete pUserBan;
        return NULL;
    }

    memcpy(pUserBan->sMessage, sMess, iMessLen);
    pUserBan->sMessage[iMessLen] = '\0';

    pUserBan->ui32Len = iMessLen;
    pUserBan->ui32NickHash = ui32Hash;

    return pUserBan;
}
//---------------------------------------------------------------------------

LoginLogout::LoginLogout() : ui64LogonTick(0), ui64IPv4CheckTick(0), ui32ToCloseLoops(0), ui32UserConnectedLen(0), pBuffer(NULL), pBan(NULL) {
    // ...
}
//---------------------------------------------------------------------------

LoginLogout::~LoginLogout() {
    delete pBan;

#ifdef _WIN32
    if(pBuffer != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pBuffer) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate pBuffer in LoginLogout::~LoginLogout\n", 0);
        }
    }
#else
	free(pBuffer);
#endif
}
//---------------------------------------------------------------------------

User::User() : ui64SharedSize(0), ui64ChangedSharedSizeShort(0), ui64ChangedSharedSizeLong(0), ui64GetNickListsTick(0), ui64MyINFOsTick(0), ui64SearchsTick(0),
	ui64ChatMsgsTick(0), ui64PMsTick(0), ui64SameSearchsTick(0), ui64SamePMsTick(0), ui64SameChatsTick(0), iLastMyINFOSendTick(0), iLastNicklist(0), iReceivedPmTick(0),
	ui64ChatMsgsTick2(0), ui64PMsTick2(0), ui64SearchsTick2(0), ui64MyINFOsTick2(0), ui64CTMsTick(0), ui64CTMsTick2(0), ui64RCTMsTick(0), ui64RCTMsTick2(0),
	ui64SRsTick(0), ui64SRsTick2(0), ui64RecvsTick(0), ui64RecvsTick2(0), ui64ChatIntMsgsTick(0), ui64PMsIntTick(0), ui64SearchsIntTick(0), 
	ui32Recvs(0), ui32Recvs2(0),
	Hubs(0), Slots(0), OLimit(0), LLimit(0), DLimit(0), iNormalHubs(0), iRegHubs(0), iOpHubs(0), 
	iSendCalled(0), iRecvCalled(0), iReceivedPmCount(0), iSR(0), iDefloodWarnings(0),
	ui32BoolBits(0), ui32InfoBits(0), ui32SupportBits(0), 
#ifdef _WIN32
	Sck(INVALID_SOCKET),
#else
	Sck(-1),
#endif
	tLoginTime(0), ui32SendBufLen(0), ui32RecvBufLen(0), ui32SendBufDataLen(0), ui32RecvBufDataLen(0),
	ui32NickHash(0), i32Profile(-1), 
	sNick((char *)sDefaultNick), sVersion(NULL), sMyInfoOriginal(NULL), sMyInfoShort(NULL), sMyInfoLong(NULL), 
	sDescription(NULL), sTag(NULL), sConnection(NULL), sEmail(NULL), sClient((char *)sOtherNoTag), sTagVersion(NULL), 
	sLastChat(NULL), sLastPM(NULL), sLastSearch(NULL), pSendBuf(NULL), pRecvBuf(NULL), pSendBufHead(NULL),
	sChangedDescriptionShort(NULL), sChangedDescriptionLong(NULL), sChangedTagShort(NULL), sChangedTagLong(NULL),
	sChangedConnectionShort(NULL), sChangedConnectionLong(NULL), sChangedEmailShort(NULL), sChangedEmailLong(NULL),
	ui8MagicByte(0), pLogInOut(NULL),
	pCmdToUserStrt(NULL), pCmdToUserEnd(NULL), pCmdStrt(NULL), pCmdEnd(NULL), pCmdActive4Search(NULL), pCmdActive6Search(NULL), pCmdPassiveSearch(NULL),
	pPrev(NULL), pNext(NULL), pHashTablePrev(NULL), pHashTableNext(NULL), pHashIpTablePrev(NULL), pHashIpTableNext(NULL),
	ui16MyInfoOriginalLen(0), ui16MyInfoShortLen(0), ui16MyInfoLongLen(0), ui16GetNickLists(0), ui16MyINFOs(0), ui16Searchs(0),
	ui16ChatMsgs(0), ui16PMs(0), ui16SameSearchs(0), ui16LastSearchLen(0), ui16SamePMs(0), ui16LastPMLen(0), ui16SameChatMsgs(0),
	ui16LastChatLen(0), ui16LastPmLines(0), ui16SameMultiPms(0), ui16LastChatLines(0), ui16SameMultiChats(0), ui16ChatMsgs2(0), ui16PMs2(0),
	ui16Searchs2(0), ui16MyINFOs2(0), ui16CTMs(0), ui16CTMs2(0), ui16RCTMs(0), ui16RCTMs2(0), ui16SRs(0), ui16SRs2(0), ui16ChatIntMsgs(0),
	ui16PMsInt(0), ui16SearchsInt(0), ui16IpTableIdx(0),
	ui8NickLen(9), ui8IpLen(0), ui8ConnectionLen(0), ui8DescriptionLen(0), ui8EmailLen(0), ui8TagLen(0), ui8ClientLen(14), ui8TagVersionLen(0),
	ui8Country(246), ui8State(User::STATE_SOCKET_ACCEPTED), ui8IPv4Len(0),
	ui8ChangedDescriptionShortLen(0), ui8ChangedDescriptionLongLen(0), ui8ChangedTagShortLen(0), ui8ChangedTagLongLen(0),
    ui8ChangedConnectionShortLen(0), ui8ChangedConnectionLongLen(0), ui8ChangedEmailShortLen(0), ui8ChangedEmailLongLen(0) {
	ui32BoolBits |= User::BIT_IPV4_ACTIVE;
	ui32BoolBits |= User::BIT_OLDHUBSTAG;

	time(&tLoginTime);

	memset(&ui128IpHash, 0, 16);

	sIP[0] = '\0';
	sIPv4[0] = '\0';
	sModes[0] = '\0';
}
//---------------------------------------------------------------------------

User::~User() {
#ifdef _WIN32
	if(pRecvBuf != NULL) {
		if(HeapFree(clsServerManager::hRecvHeap, HEAP_NO_SERIALIZE, (void *)pRecvBuf) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate pRecvBuf in User::~User\n", 0);
        }
    }
#else
	free(pRecvBuf);
#endif

#ifdef _WIN32
	if(pSendBuf != NULL) {
		if(HeapFree(clsServerManager::hSendHeap, HEAP_NO_SERIALIZE, (void *)pSendBuf) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate pSendBuf in User::~User\n", 0);
        }
    }
#else
	free(pSendBuf);
#endif

#ifdef _WIN32
	if(sLastChat != NULL) {
		if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sLastChat) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sLastChat in User::~User\n", 0);
        }
    }
#else
	free(sLastChat);
#endif

#ifdef _WIN32
	if(sLastPM != NULL) {
		if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sLastPM) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sLastPM in User::~User\n", 0);
        }
    }
#else
	free(sLastPM);
#endif

#ifdef _WIN32
	if(sLastSearch != NULL) {
		if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sLastSearch) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sLastSearch in User::~User\n", 0);
        }
    }
#else
	free(sLastSearch);
#endif

#ifdef _WIN32
	if(sMyInfoShort != NULL) {
		if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMyInfoShort) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sMyInfoShort in User::~User\n", 0);
        }
    }
#else
	free(sMyInfoShort);
#endif

#ifdef _WIN32
	if(sMyInfoLong != NULL) {
		if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMyInfoLong) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sMyInfoLong in User::~User\n", 0);
        }
    }
#else
	free(sMyInfoLong);
#endif

#ifdef _WIN32
	if(sMyInfoOriginal != NULL) {
		if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMyInfoOriginal) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sMyInfoOriginal in User::~User\n", 0);
        }
    }
#else
	free(sMyInfoOriginal);
#endif

#ifdef _WIN32
	if(sVersion != NULL) {
		if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sVersion) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sVersion in User::~User\n", 0);
        }
    }
#else
	free(sVersion);
#endif

#ifdef _WIN32
	if(sChangedDescriptionShort != NULL) {
		if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sChangedDescriptionShort) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sChangedDescriptionShort in User::~User\n", 0);
        }
    }
#else
	free(sChangedDescriptionShort);
#endif

#ifdef _WIN32
	if(sChangedDescriptionLong != NULL) {
		if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sChangedDescriptionLong) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sChangedDescriptionLong in User::~User\n", 0);
        }
    }
#else
	free(sChangedDescriptionLong);
#endif

#ifdef _WIN32
	if(sChangedTagShort != NULL) {
		if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sChangedTagShort) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sChangedTagShort in User::~User\n", 0);
        }
    }
#else
	free(sChangedTagShort);
#endif

#ifdef _WIN32
	if(sChangedTagLong != NULL) {
		if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sChangedTagLong) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sChangedTagLong in User::~User\n", 0);
        }
    }
#else
	free(sChangedTagLong);
#endif

#ifdef _WIN32
	if(sChangedConnectionShort != NULL) {
		if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sChangedConnectionShort) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sChangedConnectionShort in User::~User\n", 0);
        }
    }
#else
	free(sChangedConnectionShort);
#endif

#ifdef _WIN32
	if(sChangedConnectionLong != NULL) {
		if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sChangedConnectionLong) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sChangedConnectionLong in User::~User\n", 0);
        }
    }
#else
	free(sChangedConnectionLong);
#endif

#ifdef _WIN32
	if(sChangedEmailShort != NULL) {
		if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sChangedEmailShort) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sChangedEmailShort in User::~User\n", 0);
        }
    }
#else
	free(sChangedEmailShort);
#endif

#ifdef _WIN32
	if(sChangedEmailLong != NULL) {
		if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sChangedEmailLong) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sChangedEmailLong in User::~User\n", 0);
        }
    }
#else
	free(sChangedEmailLong);
#endif

	if(((ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == true)
        clsDcCommands::mPtr->iStatZPipe--;

	clsServerManager::ui32Parts++;

#ifdef _BUILD_GUI
    if(::SendMessage(clsMainWindowPageUsersChat::mPtr->hWndPageItems[clsMainWindowPageUsersChat::BTN_SHOW_COMMANDS], BM_GETCHECK, 0, 0) == BST_CHECKED) {
        RichEditAppendText(clsMainWindowPageUsersChat::mPtr->hWndPageItems[clsMainWindowPageUsersChat::REDT_CHAT], ("x User removed: " + string(sNick, ui8NickLen) + " (Socket " + string(Sck) + ")").c_str());
    }
#endif

	if(sNick != sDefaultNick) {
#ifdef _WIN32
		if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sNick) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sNick in User::~User\n", 0);
		}
#else
		free(sNick);
#endif
	}
        
	delete pLogInOut;
    
	if(pCmdActive4Search != NULL) {
        User::DeletePrcsdUsrCmd(pCmdActive4Search);
		pCmdActive4Search = NULL;
    }

	if(pCmdActive6Search != NULL) {
        User::DeletePrcsdUsrCmd(pCmdActive6Search);
		pCmdActive6Search = NULL;
    }

	if(pCmdPassiveSearch != NULL) {
        User::DeletePrcsdUsrCmd(pCmdPassiveSearch);
		pCmdPassiveSearch = NULL;
    }
                
	PrcsdUsrCmd * cur = NULL,
        * next = pCmdStrt;
        
    while(next != NULL) {
        cur = next;
        next = cur->pNext;

#ifdef _WIN32
		if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->sCommand) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate cur->sCommand in User::~User\n", 0);
        }
#else
		free(cur->sCommand);
#endif
		cur->sCommand = NULL;

        delete cur;
	}

	pCmdStrt = NULL;
	pCmdEnd = NULL;

	PrcsdToUsrCmd * curto = NULL,
        * nextto = pCmdToUserStrt;
                    
    while(nextto != NULL) {
        curto = nextto;
        nextto = curto->pNext;

#ifdef _WIN32
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)curto->sCommand) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate curto->sCommand in User::~User\n", 0);
        }
#else
		free(curto->sCommand);
#endif
        curto->sCommand = NULL;

#ifdef _WIN32
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)curto->sToNick) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate curto->ToNick in User::~User\n", 0);
        }
#else
		free(curto->sToNick);
#endif
        curto->sToNick = NULL;

        delete curto;
	}
    

	pCmdToUserStrt = NULL;
	pCmdToUserEnd = NULL;
}
//---------------------------------------------------------------------------

bool User::MakeLock() {
    size_t szAllignLen = Allign1024(ui32SendBufDataLen+63);

	char * pOldBuf = pSendBuf;
#ifdef _WIN32
    if(pSendBuf == NULL) {
        pSendBuf = (char *)HeapAlloc(clsServerManager::hSendHeap, HEAP_NO_SERIALIZE, szAllignLen);
    } else {
    	pSendBuf = (char *)HeapReAlloc(clsServerManager::hSendHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, szAllignLen);
	}
#else
	pSendBuf = (char *)realloc(pOldBuf, szAllignLen);
#endif
    if(pSendBuf == NULL) {
    	pSendBuf = pOldBuf;
		ui32BoolBits |= BIT_ERROR;

		AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes in User::MakeLock\n", (uint64_t)szAllignLen);

        return false;
    }
    ui32SendBufLen = (uint32_t)(szAllignLen-1);
	pSendBufHead = pSendBuf;

    // This code computes the valid Lock string including the Pk= string
    // For maximum speed we just find two random numbers - start and step
    // Step is added each cycle to the start and the ascii 122 boundary is
    // checked. If overflow occurs then the overflowed value is added to the
    // ascii 48 value ("0") and continues.
	// The lock has fixed length 63 bytes

#ifdef _WIN32
	#ifdef _BUILD_GUI
	    #ifndef _M_X64
	        static const char * sLock = "$Lock EXTENDEDPROTOCOL                           win Pk=PtokaX|"; // 63
	    #else
	        static const char * sLock = "$Lock EXTENDEDPROTOCOL                           wg6 Pk=PtokaX|"; // 63
	    #endif
	#else
	    #ifndef _M_X64
	        static const char * sLock = "$Lock EXTENDEDPROTOCOL                           wis Pk=PtokaX|"; // 63
	    #else
	        static const char * sLock = "$Lock EXTENDEDPROTOCOL                           ws6 Pk=PtokaX|"; // 63
	    #endif
	#endif
#else
	static const char * sLock = "$Lock EXTENDEDPROTOCOL                           nix Pk=PtokaX|"; // 63
#endif

    // append data to the buffer
    memcpy(pSendBuf, sLock, 63);
    ui32SendBufDataLen += 63;
    pSendBuf[ui32SendBufDataLen] = '\0';

	for(uint8_t ui8i = 22; ui8i < 49; ui8i++) {
#ifdef _WIN32
        pSendBuf[ui8i] = (char)((rand() % 74) + 48);
#else
        pSendBuf[ui8i] = (char)((random() % 74) + 48);
#endif
	}

//	Memo(string(pSendBuf, ui32SendBufDataLen));

#ifdef _WIN32
    pLogInOut->pBuffer = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, 64);
#else
	pLogInOut->pBuffer = (char *)malloc(64);
#endif
    if(pLogInOut->pBuffer == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate 64 bytes for pBuffer in User::MakeLock\n", 0);
		return false;
    }
    
    memcpy(pLogInOut->pBuffer, pSendBuf, 63);
	pLogInOut->pBuffer[63] = '\0';

    return true;
}
//---------------------------------------------------------------------------

bool User::DoRecv() {
    if((ui32BoolBits & BIT_ERROR) == BIT_ERROR || ui8State >= STATE_CLOSING)
        return false;

#ifdef _WIN32
	u_long iAvailBytes = 0;
	if(ioctlsocket(Sck, FIONREAD, &iAvailBytes) == SOCKET_ERROR) {
		int iError = WSAGetLastError();
#else
	int iAvailBytes = 0;
	if(ioctl(Sck, FIONREAD, &iAvailBytes) == -1) {
#endif
		clsUdpDebug::mPtr->Broadcast("[ERR] " + string(sNick, ui8NickLen) + " (" + string(sIP, ui8IpLen) +
#ifdef _WIN32
			" ): ioctlsocket(FIONREAD) error " + string(WSErrorStr(iError)) + " (" + string(iError) + "). User is being closed.");
#else
			" ): ioctlsocket(FIONREAD) error " + string(ErrnoStr(errno)) + " (" + string(errno) + "). User is being closed.");
#endif
        ui32BoolBits |= BIT_ERROR;
		Close();
        return false;
    }

    // PPK ... check flood ...
	if(iAvailBytes != 0 && clsProfileManager::mPtr->IsAllowed(this, clsProfileManager::NODEFLOODRECV) == false) {
        if(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_DOWN_ACTION] != 0) {
    		if(ui32Recvs == 0) {
    			ui64RecvsTick = clsServerManager::ui64ActualTick;
            }

            ui32Recvs += iAvailBytes;

			if(DeFloodCheckForDataFlood(this, DEFLOOD_MAX_DOWN, clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_DOWN_ACTION],
			  ui32Recvs, ui64RecvsTick, clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_DOWN_KB],
              (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_DOWN_TIME]) == true) {
				return false;
            }

    		if(ui32Recvs != 0) {
                ui32Recvs -= iAvailBytes;
            }
        }

        if(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_DOWN_ACTION2] != 0) {
    		if(ui32Recvs2 == 0) {
    			ui64RecvsTick2 = clsServerManager::ui64ActualTick;
            }

            ui32Recvs2 += iAvailBytes;

			if(DeFloodCheckForDataFlood(this, DEFLOOD_MAX_DOWN, clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_DOWN_ACTION2],
			  ui32Recvs2, ui64RecvsTick2, clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_DOWN_KB2],
			  (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_DOWN_TIME2]) == true) {
                return false;
            }

    		if(ui32Recvs2 != 0) {
                ui32Recvs2 -= iAvailBytes;
            }
        }
    }

	if(iAvailBytes == 0) {
		// we need to try recv to catch connection error or closed connection
        iAvailBytes = 16;
    } else if(iAvailBytes > 16384) {
        // receive max. 16384 bytes to receive buffer
        iAvailBytes = 16384;
    }

    size_t szAllignLen = 0;

    if(ui32RecvBufLen < ui32RecvBufDataLen+iAvailBytes) {
        szAllignLen = Allign512(ui32RecvBufDataLen+iAvailBytes);
    } else if(iRecvCalled > 60) {
        szAllignLen = Allign512(ui32RecvBufDataLen+iAvailBytes);
        if(ui32RecvBufLen <= szAllignLen) {
            szAllignLen = 0;
        }

        iRecvCalled = 0;
    }

    if(szAllignLen != 0) {
        char * pOldBuf = pRecvBuf;

#ifdef _WIN32
        if(pRecvBuf == NULL) {
            pRecvBuf = (char *)HeapAlloc(clsServerManager::hRecvHeap, HEAP_NO_SERIALIZE, szAllignLen);
        } else {
            pRecvBuf = (char *)HeapReAlloc(clsServerManager::hRecvHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, szAllignLen);
        }
#else
        pRecvBuf = (char *)realloc(pOldBuf, szAllignLen);
#endif
		if(pRecvBuf == NULL) {
            pRecvBuf = pOldBuf;
            ui32BoolBits |= BIT_ERROR;
            Close();

			AppendDebugLog("%s - [MEM] Cannot (re)allocate %" PRIu64 " bytes for pRecvBuf in User::DoRecv\n", (uint64_t)szAllignLen);

			return false;
		}

		ui32RecvBufLen = (uint32_t)(szAllignLen-1);
	}
    
    // receive new data to pRecvBuf
	int recvlen = recv(Sck, pRecvBuf+ui32RecvBufDataLen, ui32RecvBufLen-ui32RecvBufDataLen, 0);
	iRecvCalled++;

#ifdef _WIN32
    if(recvlen == SOCKET_ERROR) {
		int iError = WSAGetLastError();
        if(iError != WSAEWOULDBLOCK) {
#else
    if(recvlen == -1) {
        if(errno != EAGAIN) {
#endif
			clsUdpDebug::mPtr->Broadcast("[ERR] " + string(sNick, ui8NickLen) + " (" + string(sIP, ui8IpLen) + "): recv() error " + 
#ifdef _WIN32
                string(WSErrorStr(iError)) + ". User is being closed.");
#else
				string(ErrnoStr(errno)) + " (" + string(errno) + "). User is being closed.");
#endif
			ui32BoolBits |= BIT_ERROR;
            Close();
            return false;
        } else {
            return false;
        }
    } else if(recvlen == 0) { // regular close
#ifdef _WIN32
        int iError = WSAGetLastError();
	    if(iError != 0) {
			Memo("[SYS] recvlen == 0 and iError == "+string(iError)+"! User: "+string(sNick, ui8NickLen)+" ("+string(sIP, ui8IpLen)+")");
			clsUdpDebug::mPtr->Broadcast("[SYS] recvlen == 0 and iError == " + string(iError) + " ! User: " + string(sNick, ui8NickLen) +
				" (" + string(sIP, ui8IpLen) + ").");
		}

	#ifdef _BUILD_GUI
        if(::SendMessage(clsMainWindowPageUsersChat::mPtr->hWndPageItems[clsMainWindowPageUsersChat::BTN_SHOW_COMMANDS], BM_GETCHECK, 0, 0) == BST_CHECKED) {
			int iret = sprintf(msg, "- User has closed the connection: %s (%s)", sNick, sIP);
			if(CheckSprintf(iret, 1024, "User::DoRecv") == true) {
				RichEditAppendText(clsMainWindowPageUsersChat::mPtr->hWndPageItems[clsMainWindowPageUsersChat::REDT_CHAT], msg);
			}
        }
    #endif
#endif

        ui32BoolBits |= BIT_ERROR;
        Close();
	    return false;
    }

    ui32Recvs += recvlen;
    ui32Recvs2 += recvlen;
	clsServerManager::ui64BytesRead += recvlen;
	ui32RecvBufDataLen += recvlen;
	pRecvBuf[ui32RecvBufDataLen] = '\0';
    if(UserProcessLines(this, ui32RecvBufDataLen-recvlen) == true) {
        return true;
    }
        
    return false;
}
//---------------------------------------------------------------------------

void User::SendChar(const char * cText, const size_t &szTextLen) {
	if(ui8State >= STATE_CLOSING || szTextLen == 0)
        return;

    if(((ui32SupportBits & SUPPORTBIT_ZPIPE) == SUPPORTBIT_ZPIPE) == false || szTextLen < ZMINDATALEN) {
        if(PutInSendBuf(cText, szTextLen)) {
            Try2Send();
        }
    } else {
        uint32_t iLen = 0;
        char *sData = clsZlibUtility::mPtr->CreateZPipe(cText, szTextLen, iLen);
            
        if(iLen == 0) {
            if(PutInSendBuf(cText, szTextLen)) {
                Try2Send();
            }
        } else {
            clsServerManager::ui64BytesSentSaved += szTextLen-iLen;
            if(PutInSendBuf(sData, iLen)) {
                Try2Send();
            }
        }
    }
}
//---------------------------------------------------------------------------

void User::SendCharDelayed(const char * cText, const size_t &szTextLen) {
	if(ui8State >= STATE_CLOSING || szTextLen == 0) {
        return;
    }
        
    if(((ui32SupportBits & SUPPORTBIT_ZPIPE) == SUPPORTBIT_ZPIPE) == false || szTextLen < ZMINDATALEN) {
        PutInSendBuf(cText, szTextLen);
    } else {
        uint32_t iLen = 0;
        char *sPipeData = clsZlibUtility::mPtr->CreateZPipe(cText, szTextLen, iLen);
        
        if(iLen == 0) {
            PutInSendBuf(cText, szTextLen);
        } else {
            PutInSendBuf(sPipeData, iLen);
            clsServerManager::ui64BytesSentSaved += szTextLen-iLen;
        }
    }
}
//---------------------------------------------------------------------------

void User::SendTextDelayed(const string &sText) {
	if(ui8State >= STATE_CLOSING || sText.size() == 0) {
        return;
    }
      
    if(((ui32SupportBits & SUPPORTBIT_ZPIPE) == SUPPORTBIT_ZPIPE) == false || sText.size() < ZMINDATALEN) {
        PutInSendBuf(sText.c_str(), sText.size());
    } else {
        uint32_t iLen = 0;
        char *sData = clsZlibUtility::mPtr->CreateZPipe(sText.c_str(), sText.size(), iLen);
            
        if(iLen == 0) {
            PutInSendBuf(sText.c_str(), sText.size());
        } else {
            PutInSendBuf(sData, iLen);
            clsServerManager::ui64BytesSentSaved += sText.size()-iLen;
        }
    }
}
//---------------------------------------------------------------------------

bool User::PutInSendBuf(const char * Text, const size_t &szTxtLen) {
	iSendCalled++;

    size_t szAllignLen = 0;

    if(ui32SendBufLen < ui32SendBufDataLen+szTxtLen) {
        if(pSendBuf == NULL) {
            szAllignLen = Allign1024(ui32SendBufDataLen+szTxtLen);
        } else {
            if((size_t)(pSendBufHead-pSendBuf) > szTxtLen) {
                uint32_t offset = (uint32_t)(pSendBufHead-pSendBuf);
                memmove(pSendBuf, pSendBufHead, (ui32SendBufDataLen-offset));
                pSendBufHead = pSendBuf;
                ui32SendBufDataLen = ui32SendBufDataLen-offset;
            } else {
                szAllignLen = Allign1024(ui32SendBufDataLen+szTxtLen);
                size_t szMaxBufLen = (size_t)(((ui32BoolBits & BIT_BIG_SEND_BUFFER) == BIT_BIG_SEND_BUFFER) == true ?
                    ((clsUsers::mPtr->ui32MyInfosTagLen > clsUsers::mPtr->ui32MyInfosLen ? clsUsers::mPtr->ui32MyInfosTagLen : clsUsers::mPtr->ui32MyInfosLen)*2) :
                    (clsUsers::mPtr->ui32MyInfosTagLen > clsUsers::mPtr->ui32MyInfosLen ? clsUsers::mPtr->ui32MyInfosTagLen : clsUsers::mPtr->ui32MyInfosLen));
                szMaxBufLen = szMaxBufLen < 262144 ? 262144 :szMaxBufLen;
                if(szAllignLen > szMaxBufLen) {
                    // does the buffer size reached the maximum
                    if(clsSettingManager::mPtr->bBools[SETBOOL_KEEP_SLOW_USERS] == false || (ui32SupportBits & SUPPORTBIT_ZPIPE) == SUPPORTBIT_ZPIPE) {
                        // we want to drop the slow user
                        ui32BoolBits |= BIT_ERROR;
                        Close();

                        clsUdpDebug::mPtr->Broadcast("[SYS] " + string(sNick, ui8NickLen) + " (" + string(sIP, ui8IpLen) +") SendBuffer overflow (AL:"+string((uint64_t)szAllignLen)+
                            "[SL:"+string(ui32SendBufDataLen)+"|NL:"+string((uint64_t)szTxtLen)+"|FL:"+string((uint64_t)(pSendBufHead-pSendBuf))+"]/ML:"+string((uint64_t)szMaxBufLen)+"). User disconnected.");
                        return false;
                    } else {
    				    clsUdpDebug::mPtr->Broadcast("[SYS] " + string(sNick, ui8NickLen) + " (" + string(sIP, ui8IpLen) +") SendBuffer overflow (AL:"+string((uint64_t)szAllignLen)+
                            "[SL:"+string(ui32SendBufDataLen)+"|NL:"+string((uint64_t)szTxtLen)+"|FL:"+string((uint64_t)(pSendBufHead-pSendBuf))+"]/ML:"+string((uint64_t)szMaxBufLen)+
                            "). Buffer cleared - user stays online.");
                    }

                    // we want to keep the slow user online
                    // PPK ... i don't want to corrupt last command, get rest of it and add to new buffer ;)
                    char *sTemp = (char *)memchr(pSendBufHead, '|', ui32SendBufDataLen-(pSendBufHead-pSendBuf));
                    if(sTemp != NULL) {
                        uint32_t iOldSBDataLen = ui32SendBufDataLen;

                        uint32_t iRestCommandLen = (uint32_t)((sTemp-pSendBufHead)+1);
                        if(pSendBuf != pSendBufHead) {
                            memmove(pSendBuf, pSendBufHead, iRestCommandLen);
                        }
                        ui32SendBufDataLen = iRestCommandLen;

                        // If is not needed then don't lost all data, try to find some space with removing only few oldest commands
                        if(szTxtLen < szMaxBufLen && iOldSBDataLen > (uint32_t)((sTemp+1)-pSendBuf) && (iOldSBDataLen-((sTemp+1)-pSendBuf)) > (uint32_t)szTxtLen) {
                            char *sTemp1;
                            // try to remove min half of send bufer
                            if(iOldSBDataLen > (ui32SendBufLen/2) && (uint32_t)((sTemp+1+szTxtLen)-pSendBuf) < (ui32SendBufLen/2)) {
                                sTemp1 = (char *)memchr(pSendBuf+(ui32SendBufLen/2), '|', iOldSBDataLen-(ui32SendBufLen/2));
                            } else {
                                sTemp1 = (char *)memchr(sTemp+1+szTxtLen, '|', iOldSBDataLen-((sTemp+1+szTxtLen)-pSendBuf));
                            }

                            if(sTemp1 != NULL) {
                                iRestCommandLen = (uint32_t)(iOldSBDataLen-((sTemp1+1)-pSendBuf));
                                memmove(pSendBuf+ui32SendBufDataLen, sTemp1+1, iRestCommandLen);
                                ui32SendBufDataLen += iRestCommandLen;
                            }
                        }
                    } else {
                        pSendBuf[0] = '|';
                        pSendBuf[1] = '\0';
                        ui32SendBufDataLen = 1;
                    }

                    size_t szAllignTxtLen = Allign1024(szTxtLen+ui32SendBufDataLen);

                    char * pOldBuf = pSendBuf;
#ifdef _WIN32
                    pSendBuf = (char *)HeapReAlloc(clsServerManager::hSendHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, szAllignTxtLen);
#else
				    pSendBuf = (char *)realloc(pOldBuf, szAllignTxtLen);
#endif
                    if(pSendBuf == NULL) {
                        pSendBuf = pOldBuf;
                        ui32BoolBits |= BIT_ERROR;
                        Close();

                        AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in User::PutInSendBuf-keepslow\n", (uint64_t)szAllignLen);

                        return false;
                    }
                    ui32SendBufLen = (uint32_t)(szAllignTxtLen-1);
                    pSendBufHead = pSendBuf;

                    szAllignLen = 0;
                } else {
                    szAllignLen = Allign1024(ui32SendBufDataLen+szTxtLen);
                }
        	}
        }
    } else if(iSendCalled > 100) {
        szAllignLen = Allign1024(ui32SendBufDataLen+szTxtLen);
        if(ui32SendBufLen <= szAllignLen) {
            szAllignLen = 0;
        }

        iSendCalled = 0;
    }

    if(szAllignLen != 0) {
        uint32_t offset = (pSendBuf == NULL ? 0 : (uint32_t)(pSendBufHead-pSendBuf));

        char * pOldBuf = pSendBuf;
#ifdef _WIN32
        if(pSendBuf == NULL) {
            pSendBuf = (char *)HeapAlloc(clsServerManager::hSendHeap, HEAP_NO_SERIALIZE, szAllignLen);
        } else {
            pSendBuf = (char *)HeapReAlloc(clsServerManager::hSendHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, szAllignLen);
        }
#else
		pSendBuf = (char *)realloc(pOldBuf, szAllignLen);
#endif
        if(pSendBuf == NULL) {
            pSendBuf = pOldBuf;
            ui32BoolBits |= BIT_ERROR;
            Close();

			AppendDebugLog("%s - [MEM] Cannot (re)allocate %" PRIu64 " bytes for new pSendBuf in User::PutInSendBuf\n", (uint64_t)szAllignLen);

        	return false;
        }

        ui32SendBufLen = (uint32_t)(szAllignLen-1);
        pSendBufHead = pSendBuf+offset;
    }

    // append data to the buffer
    memcpy(pSendBuf+ui32SendBufDataLen, Text, szTxtLen);
    ui32SendBufDataLen += (uint32_t)szTxtLen;
    pSendBuf[ui32SendBufDataLen] = '\0';

    return true;
}
//---------------------------------------------------------------------------

bool User::Try2Send() {
    if((ui32BoolBits & BIT_ERROR) == BIT_ERROR || ui32SendBufDataLen == 0) {
        return false;
    }

    // compute length of unsent data
    int32_t offset = (int32_t)(pSendBufHead - pSendBuf);
	int32_t len = ui32SendBufDataLen - offset;

	if(offset < 0 || len < 0) {
		int imsgLen = sprintf(msg, "%s - [ERR] Negative send values!\nSendBuf: %p\nPlayHead: %p\nDataLen: %u\n", "%s", pSendBuf, pSendBufHead, ui32SendBufDataLen);
        if(CheckSprintf(imsgLen, 1024, "UserTry2Send") == true) {
    		AppendDebugLog(msg, 0);
        }

        ui32BoolBits |= BIT_ERROR;
        Close();

        return false;
    }

    int n = send(Sck, pSendBufHead, len < 32768 ? len : 32768, 0);

#ifdef _WIN32
    if(n == SOCKET_ERROR) {
    	int iError = WSAGetLastError();
        if(iError != WSAEWOULDBLOCK) {
#else
	if(n == -1) {
        if(errno != EAGAIN) {
#endif
			clsUdpDebug::mPtr->Broadcast("[ERR] " + string(sNick, ui8NickLen) + " (" + string(sIP, ui8IpLen) +
#ifdef _WIN32
				"): send() error " + string(WSErrorStr(iError)) + ". User is being closed.");
#else
				"): send() error " + string(ErrnoStr(errno)) + " (" + string(errno) + "). User is being closed.");
#endif
			ui32BoolBits |= BIT_ERROR;
            Close();
            return false;
        } else {
            return true;
        }
    }

	clsServerManager::ui64BytesSent += n;

	// if buffer is sent then mark it as empty (first byte = 0)
	// else move remaining data on new place and free old buffer
	if(n < len) {
        pSendBufHead += n;
		return true;
	} else {
        // PPK ... we need to free memory allocated for big buffer on login (userlist, motd...)
        if(((ui32BoolBits & BIT_BIG_SEND_BUFFER) == BIT_BIG_SEND_BUFFER) == true) {
            if(pSendBuf != NULL) {
#ifdef _WIN32
               if(HeapFree(clsServerManager::hSendHeap, HEAP_NO_SERIALIZE, (void *)pSendBuf) == 0) {
					AppendDebugLog("%s - [MEM] Cannot deallocate pSendBuf in User::Try2Send\n", 0);
                }
#else
				free(pSendBuf);
#endif
                pSendBuf = NULL;
                pSendBufHead = pSendBuf;
                ui32SendBufLen = 0;
                ui32SendBufDataLen = 0;
            }
            ui32BoolBits &= ~BIT_BIG_SEND_BUFFER;
        } else {
    		pSendBuf[0] = '\0';
            pSendBufHead = pSendBuf;
            ui32SendBufDataLen = 0;
        }
		return false;
	}
}
//---------------------------------------------------------------------------

void User::SetIP(char * sNewIP) {
    strcpy(sIP, sNewIP);
    ui8IpLen = (uint8_t)strlen(sIP);
}
//------------------------------------------------------------------------------

void User::SetNick(char * sNewNick, const uint8_t &ui8NewNickLen) {
	if(sNick != sDefaultNick && sNick != NULL) {
#ifdef _WIN32
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sNick) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sNick in User::SetNick\n", 0);
        }
#else
		free(sNick);
#endif
        sNick = NULL;
    }

#ifdef _WIN32
    sNick = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, ui8NewNickLen+1);
#else
	sNick = (char *)malloc(ui8NewNickLen+1);
#endif
    if(sNick == NULL) {
        sNick = (char *)sDefaultNick;
        ui32BoolBits |= BIT_ERROR;
        Close();

		AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sNick in User::SetNick\n", (uint64_t)(ui8NewNickLen+1));

        return;
    }   
    memcpy(sNick, sNewNick, ui8NewNickLen);
    sNick[ui8NewNickLen] = '\0';
    ui8NickLen = ui8NewNickLen;
    ui32NickHash = HashNick(sNick, ui8NickLen);
}
//------------------------------------------------------------------------------

void User::SetMyInfoOriginal(char * sNewMyInfo, const uint16_t &ui16NewMyInfoLen) {
    char * sOldMyInfo = sMyInfoOriginal;

    char * sOldDescription = sDescription;
    uint8_t ui8OldDescriptionLen = ui8DescriptionLen;

    char * sOldTag = sTag;
    uint8_t ui8OldTagLen = ui8TagLen;

    char * sOldConnection = sConnection;
    uint8_t ui8OldConnectionLen = ui8ConnectionLen;

    char * sOldEmail = sEmail;
    uint8_t ui8OldEmailLen = ui8EmailLen;

    uint64_t ui64OldShareSize = ui64SharedSize;

	if(sMyInfoOriginal != NULL) {
        sConnection = NULL;
        ui8ConnectionLen = 0;

        sDescription = NULL;
        ui8DescriptionLen = 0;

        sEmail = NULL;
        ui8EmailLen = 0;

        sTag = NULL;
        ui8TagLen = 0;

        sClient = NULL;
        ui8ClientLen = 0;

        sTagVersion = NULL;
        ui8TagVersionLen = 0;

        sMyInfoOriginal = NULL;
    }

#ifdef _WIN32
    sMyInfoOriginal = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, ui16NewMyInfoLen+1);
#else
	sMyInfoOriginal = (char *)malloc(ui16NewMyInfoLen+1);
#endif
    if(sMyInfoOriginal == NULL) {
        ui32BoolBits |= BIT_ERROR;
        Close();

		AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sMyInfoOriginal in UserSetMyInfoOriginal\n", (uint64_t)(ui16NewMyInfoLen+1));

        return;
    }
    memcpy(sMyInfoOriginal, sNewMyInfo, ui16NewMyInfoLen);
    sMyInfoOriginal[ui16NewMyInfoLen] = '\0';
    ui16MyInfoOriginalLen = ui16NewMyInfoLen;

    UserParseMyInfo(this);

    if(ui8OldDescriptionLen != ui8DescriptionLen || (ui8DescriptionLen > 0 && memcmp(sOldDescription, sDescription, ui8DescriptionLen) != 0)) {
        ui32InfoBits |= INFOBIT_DESCRIPTION_CHANGED;
    } else {
        ui32InfoBits &= ~INFOBIT_DESCRIPTION_CHANGED;
    }

    if(ui8OldTagLen != ui8TagLen || (ui8TagLen > 0 && memcmp(sOldTag, sTag, ui8TagLen) != 0)) {
        ui32InfoBits |= INFOBIT_TAG_CHANGED;
    } else {
        ui32InfoBits &= ~INFOBIT_TAG_CHANGED;
    }

    if(ui8OldConnectionLen != ui8ConnectionLen || (ui8ConnectionLen > 0 && memcmp(sOldConnection, sConnection, ui8ConnectionLen) != 0)) {
        ui32InfoBits |= INFOBIT_CONNECTION_CHANGED;
    } else {
        ui32InfoBits &= ~INFOBIT_CONNECTION_CHANGED;
    }

    if(ui8OldEmailLen != ui8EmailLen || (ui8EmailLen > 0 && memcmp(sOldEmail, sEmail, ui8EmailLen) != 0)) {
        ui32InfoBits |= INFOBIT_EMAIL_CHANGED;
    } else {
        ui32InfoBits &= ~INFOBIT_EMAIL_CHANGED;
    }

    if(ui64OldShareSize != ui64SharedSize) {
        ui32InfoBits |= INFOBIT_SHARE_CHANGED;
    } else {
        ui32InfoBits &= ~INFOBIT_SHARE_CHANGED;
    }

    if(sOldMyInfo != NULL) {
#ifdef _WIN32
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldMyInfo) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate sOldMyInfo in UserSetMyInfoOriginal\n", 0);
        }
#else
        free(sOldMyInfo);
#endif
    }

    if(((ui32InfoBits & INFOBIT_SHARE_SHORT_PERM) == INFOBIT_SHARE_SHORT_PERM) == false) {
        ui64ChangedSharedSizeShort = ui64SharedSize;
    }

    if(((ui32InfoBits & INFOBIT_SHARE_LONG_PERM) == INFOBIT_SHARE_LONG_PERM) == false) {
        ui64ChangedSharedSizeLong = ui64SharedSize;
    }

}
//------------------------------------------------------------------------------

static void UserSetMyInfoLong(User * u, char * sNewMyInfoLong, const uint16_t &ui16NewMyInfoLongLen) {
	if(u->sMyInfoLong != NULL) {
        if(clsSettingManager::mPtr->ui8FullMyINFOOption != 2) {
    	    clsUsers::mPtr->DelFromMyInfosTag(u);
        }

#ifdef _WIN32
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)u->sMyInfoLong) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate u->sMyInfoLong in UserSetMyInfoLong\n", 0);
        }
#else
        free(u->sMyInfoLong);
#endif
        u->sMyInfoLong = NULL;
    }

#ifdef _WIN32
    u->sMyInfoLong = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, ui16NewMyInfoLongLen+1);
#else
	u->sMyInfoLong = (char *)malloc(ui16NewMyInfoLongLen+1);
#endif
    if(u->sMyInfoLong == NULL) {
        u->ui32BoolBits |= User::BIT_ERROR;
        u->Close();

		AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sMyInfoLong in UserSetMyInfoLong\n", (uint64_t)(ui16NewMyInfoLongLen+1));

        return;
    }   
    memcpy(u->sMyInfoLong, sNewMyInfoLong, ui16NewMyInfoLongLen);
    u->sMyInfoLong[ui16NewMyInfoLongLen] = '\0';
    u->ui16MyInfoLongLen = ui16NewMyInfoLongLen;
}
//------------------------------------------------------------------------------

static void UserSetMyInfoShort(User * u, char * sNewMyInfoShort, const uint16_t &ui16NewMyInfoShortLen) {
	if(u->sMyInfoShort != NULL) {
        if(clsSettingManager::mPtr->ui8FullMyINFOOption != 0) {
    	    clsUsers::mPtr->DelFromMyInfos(u);
        }

#ifdef _WIN32    	    
    	if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)u->sMyInfoShort) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate u->sMyInfoShort in UserSetMyInfoShort\n", 0);
        }
#else
		free(u->sMyInfoShort);
#endif
        u->sMyInfoShort = NULL;
    }

#ifdef _WIN32
    u->sMyInfoShort = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, ui16NewMyInfoShortLen+1);
#else
	u->sMyInfoShort = (char *)malloc(ui16NewMyInfoShortLen+1);
#endif
    if(u->sMyInfoShort == NULL) {
        u->ui32BoolBits |= User::BIT_ERROR;
        u->Close();

		AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for MyInfoShort in UserSetMyInfoShort\n", (uint64_t)(ui16NewMyInfoShortLen+1));

        return;
    }   
    memcpy(u->sMyInfoShort, sNewMyInfoShort, ui16NewMyInfoShortLen);
    u->sMyInfoShort[ui16NewMyInfoShortLen] = '\0';
    u->ui16MyInfoShortLen = ui16NewMyInfoShortLen;
}
//------------------------------------------------------------------------------

void User::SetVersion(char * sNewVer) {
#ifdef _WIN32
	if(sVersion) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sVersion) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sVersion in User::SetVersion\n", 0);
        }
    }
#else
	free(sVersion);
#endif

    size_t szLen = strlen(sNewVer);
#ifdef _WIN32
    sVersion = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szLen+1);
#else
	sVersion = (char *)malloc(szLen+1);
#endif
    if(sVersion == NULL) {
        ui32BoolBits |= BIT_ERROR;
        Close();

		AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for Version in User::SetVersion\n", (uint64_t)(szLen+1));

        return;
    }   
    memcpy(sVersion, sNewVer, szLen);
    sVersion[szLen] = '\0';
}
//------------------------------------------------------------------------------

void User::SetLastChat(char * sNewData, const size_t &szLen) {
#ifdef _WIN32
    if(sLastChat != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sLastChat) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sLastChat in User::SetLastChat\n", 0);
        }
    }
#else
	free(sLastChat);
#endif

#ifdef _WIN32
    sLastChat = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szLen+1);
#else
	sLastChat = (char *)malloc(szLen+1);
#endif
    if(sLastChat == NULL) {
        ui32BoolBits |= BIT_ERROR;
        Close();

		AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sLastChat in User::SetLastChat\n", (uint64_t)(szLen+1));

        return;
    }   
    memcpy(sLastChat, sNewData, szLen);
    sLastChat[szLen] = '\0';
    ui16SameChatMsgs = 1;
    ui64SameChatsTick = clsServerManager::ui64ActualTick;
    ui16LastChatLen = (uint16_t)szLen;
    ui16SameMultiChats = 0;
    ui16LastChatLines = 0;
}
//------------------------------------------------------------------------------

void User::SetLastPM(char * sNewData, const size_t &szLen) {
#ifdef _WIN32
    if(sLastPM != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sLastPM) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sLastPM in User::SetLastPM\n", 0);
        }
    }
#else
	free(sLastPM);
#endif

#ifdef _WIN32
    sLastPM = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szLen+1);
#else
	sLastPM = (char *)malloc(szLen+1);
#endif
    if(sLastPM == NULL) {
        ui32BoolBits |= BIT_ERROR;
        Close();

		AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sLastPM in User::SetLastPM\n", (uint64_t)(szLen+1));

        return;
    }

    memcpy(sLastPM, sNewData, szLen);
    sLastPM[szLen] = '\0';
    ui16SamePMs = 1;
    ui64SamePMsTick = clsServerManager::ui64ActualTick;
    ui16LastPMLen = (uint16_t)szLen;
    ui16SameMultiPms = 0;
    ui16LastPmLines = 0;
}
//------------------------------------------------------------------------------

void User::SetLastSearch(char * sNewData, const size_t &szLen) {
#ifdef _WIN32
    if(sLastSearch != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sLastSearch) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sLastSearch in User::SetLastSearch\n", 0);
        }
    }
#else
	free(sLastSearch);
#endif

#ifdef _WIN32
    sLastSearch = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szLen+1);
#else
	sLastSearch = (char *)malloc(szLen+1);
#endif
    if(sLastSearch == NULL) {
        ui32BoolBits |= BIT_ERROR;
        Close();

        AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sLastSearch in User::SetLastSearch\n", (uint64_t)(szLen+1));

        return;
    }   
    memcpy(sLastSearch, sNewData, szLen);
    sLastSearch[szLen] = '\0';
    ui16SameSearchs = 1;
    ui64SameSearchsTick = clsServerManager::ui64ActualTick;
    ui16LastSearchLen = (uint16_t)szLen;
}
//------------------------------------------------------------------------------

void User::SetBuffer(char * sKickMsg, size_t szLen/* = 0*/) {
    if(szLen == 0) {
        szLen = strlen(sKickMsg);
    }

    if(pLogInOut == NULL) {
        pLogInOut = new (std::nothrow) LoginLogout();
        if(pLogInOut == NULL) {
    		ui32BoolBits |= BIT_ERROR;
    		Close();

    		AppendDebugLog("%s - [MEM] Cannot allocate new pLogInOut in User::SetBuffer\n", 0);
    		return;
        }
    }

	void * pOldBuf = pLogInOut->pBuffer;

    if(szLen < 512) {
#ifdef _WIN32
		if(pLogInOut->pBuffer == NULL) {
			pLogInOut->pBuffer = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szLen+1);
		} else {
			pLogInOut->pBuffer = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, pOldBuf, szLen+1);
		}
#else
		pLogInOut->pBuffer = (char *)realloc(pOldBuf, szLen+1);
#endif
        if(pLogInOut->pBuffer == NULL) {
            ui32BoolBits |= BIT_ERROR;
            Close();

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for pBuffer in User::SetBuffer\n", (uint64_t)(szLen+1));

            return;
        }
        memcpy(pLogInOut->pBuffer, sKickMsg, szLen);
        pLogInOut->pBuffer[szLen] = '\0';
    } else {
#ifdef _WIN32
		if(pLogInOut->pBuffer == NULL) {
			pLogInOut->pBuffer = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, 512);
		} else {
			pLogInOut->pBuffer = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, pOldBuf, 512);
		}
#else
		pLogInOut->pBuffer = (char *)realloc(pOldBuf, 512);
#endif
        if(pLogInOut->pBuffer == NULL) {
            ui32BoolBits |= BIT_ERROR;
            Close();

			AppendDebugLog("%s - [MEM] Cannot allocate 256 bytes for pBuffer in User::SetBuffer\n", 0);

            return;
        }
        memcpy(pLogInOut->pBuffer, sKickMsg, 508);
        pLogInOut->pBuffer[511] = '\0';
        pLogInOut->pBuffer[510] = '.';
        pLogInOut->pBuffer[509] = '.';
        pLogInOut->pBuffer[508] = '.';
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void User::FreeBuffer() {
    if(pLogInOut->pBuffer != NULL) {
#ifdef _WIN32
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pLogInOut->pBuffer) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate pLogInOut->pBuffer in User::FreeBuffer\n", 0);
        }
#else
        free(pLogInOut->pBuffer);
#endif
        pLogInOut->pBuffer = NULL;
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void User::Close(bool bNoQuit/* = false*/) {
    if(ui8State >= STATE_CLOSING) {
        return;
    }
    
	// nick in hash table ?
	if((ui32BoolBits & BIT_HASHED) == BIT_HASHED) {
    	clsHashManager::mPtr->Remove(this);
    }

    // nick in nick/op list ?
    if(ui8State >= STATE_ADDME_2LOOP) {  
		clsUsers::mPtr->DelFromNickList(sNick, (ui32BoolBits & BIT_OPERATOR) == BIT_OPERATOR);
		clsUsers::mPtr->DelFromUserIP(this);

        // PPK ... fix for QuickList nad ghost...
        // and fixing redirect all too ;)
        // and fix disconnect on send error too =)
        if(bNoQuit == false) {         
            int imsgLen = sprintf(msg, "$Quit %s|", sNick); 
            if(CheckSprintf(imsgLen, 1024, "User::Close") == true) {
                clsGlobalDataQueue::mPtr->AddQueueItem(msg, imsgLen, NULL, 0, clsGlobalDataQueue::CMD_QUIT);
            }

			clsUsers::mPtr->Add2RecTimes(this);
        }

#ifdef _BUILD_GUI
        if(::SendMessage(clsMainWindowPageUsersChat::mPtr->hWndPageItems[clsMainWindowPageUsersChat::BTN_AUTO_UPDATE_USERLIST], BM_GETCHECK, 0, 0) == BST_CHECKED) {
            clsMainWindowPageUsersChat::mPtr->RemoveUser(this);
        }
#endif

        //sqldb->FinalizeVisit(u);
#ifdef _WITH_SQLITE
		DBSQLite::mPtr->UpdateRecord(this);
#elif _WITH_POSTGRES
		DBPostgreSQL::mPtr->UpdateRecord(this);
#elif _WITH_MYSQL
		DBMySQL::mPtr->UpdateRecord(this);
#endif

		if(((ui32BoolBits & BIT_HAVE_SHARECOUNTED) == BIT_HAVE_SHARECOUNTED) == true) {
            clsServerManager::ui64TotalShare -= ui64SharedSize;
            ui32BoolBits &= ~BIT_HAVE_SHARECOUNTED;
		}

		clsScriptManager::mPtr->UserDisconnected(this);
	}

    if(ui8State > STATE_ADDME_2LOOP) {
        clsServerManager::ui32Logged--;
    }

	ui8State = STATE_CLOSING;
	
    if(pCmdActive4Search != NULL) {
        User::DeletePrcsdUsrCmd(pCmdActive4Search);
        pCmdActive4Search = NULL;
    }

    if(pCmdActive6Search != NULL) {
        User::DeletePrcsdUsrCmd(pCmdActive6Search);
        pCmdActive6Search = NULL;
    }

    if(pCmdPassiveSearch != NULL) {
        User::DeletePrcsdUsrCmd(pCmdPassiveSearch);
        pCmdPassiveSearch = NULL;
    }
                        
    PrcsdUsrCmd * cur = NULL,
        * next = pCmdStrt;
                        
    while(next != NULL) {
        cur = next;
        next = cur->pNext;

#ifdef _WIN32
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->sCommand) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate cur->sCommand in User::Close\n", 0);
        }
#else
		free(cur->sCommand);
#endif
        cur->sCommand = NULL;

        delete cur;
	}
    
    pCmdStrt = NULL;
    pCmdEnd = NULL;
    
    PrcsdToUsrCmd * curto = NULL,
        * nextto = pCmdToUserStrt;
                        
    while(nextto != NULL) {
        curto = nextto;
        nextto = curto->pNext;

#ifdef _WIN32
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)curto->sCommand) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate curto->sCommand in User::Close\n", 0);
        }
#else
		free(curto->sCommand);
#endif
        curto->sCommand = NULL;

#ifdef _WIN32
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)curto->sToNick) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate curto->ToNick in User::Close\n", 0);
        }
#else
		free(curto->sToNick);
#endif
        curto->sToNick = NULL;

        delete curto;
	}


    pCmdToUserStrt = NULL;
    pCmdToUserEnd = NULL;

    if(sMyInfoLong) {
    	if(clsSettingManager::mPtr->ui8FullMyINFOOption != 2) {
    		clsUsers::mPtr->DelFromMyInfosTag(this);
        }
    }
    
    if(sMyInfoShort) {
    	if(clsSettingManager::mPtr->ui8FullMyINFOOption != 0) {
    		clsUsers::mPtr->DelFromMyInfos(this);
        }
    }

    if(ui32SendBufDataLen == 0 || (ui32BoolBits & BIT_ERROR) == BIT_ERROR) {
        ui8State = STATE_REMME;
    } else {
        if(pLogInOut == NULL) {
            pLogInOut = new (std::nothrow) LoginLogout();
            if(pLogInOut == NULL) {
                ui8State = STATE_REMME;
        		AppendDebugLog("%s - [MEM] Cannot allocate new pLogInOut in User::Close\n", 0);
        		return;
            }
        }

        pLogInOut->ui32ToCloseLoops = 100;
    }
}
//---------------------------------------------------------------------------

void User::Add2Userlist() {
    clsUsers::mPtr->Add2NickList(this);
    clsUsers::mPtr->Add2UserIP(this);
    
    switch(clsSettingManager::mPtr->ui8FullMyINFOOption) {
        case 0: {
            GenerateMyInfoLong();
            clsUsers::mPtr->Add2MyInfosTag(this);
            return;
        }
        case 1: {
            GenerateMyInfoLong();
            clsUsers::mPtr->Add2MyInfosTag(this);
            GenerateMyInfoShort();
            clsUsers::mPtr->Add2MyInfos(this);
            return;
        }
        case 2: {
            GenerateMyInfoShort();
            clsUsers::mPtr->Add2MyInfos(this);
            return;
        }
        default:
            break;
    }
}
//------------------------------------------------------------------------------

void User::AddUserList() {
    ui32BoolBits |= BIT_BIG_SEND_BUFFER;
	iLastNicklist = clsServerManager::ui64ActualTick;

	if(((ui32SupportBits & SUPPORTBIT_NOHELLO) == SUPPORTBIT_NOHELLO) == false) {
    	if(clsProfileManager::mPtr->IsAllowed(this, clsProfileManager::ALLOWEDOPCHAT) == false || (clsSettingManager::mPtr->bBools[SETBOOL_REG_OP_CHAT] == false ||
            (clsSettingManager::mPtr->bBools[SETBOOL_REG_BOT] == true && clsSettingManager::mPtr->bBotsSameNick == true))) {
            if(((ui32SupportBits & SUPPORTBIT_ZPIPE) == SUPPORTBIT_ZPIPE) == false) {
                SendCharDelayed(clsUsers::mPtr->pNickList, clsUsers::mPtr->ui32NickListLen);
            } else {
                if(clsUsers::mPtr->ui32ZNickListLen == 0) {
                    clsUsers::mPtr->pZNickList = clsZlibUtility::mPtr->CreateZPipe(clsUsers::mPtr->pNickList, clsUsers::mPtr->ui32NickListLen, clsUsers::mPtr->pZNickList,
                        clsUsers::mPtr->ui32ZNickListLen, clsUsers::mPtr->ui32ZNickListSize, Allign16K);
                    if(clsUsers::mPtr->ui32ZNickListLen == 0) {
                        SendCharDelayed(clsUsers::mPtr->pNickList, clsUsers::mPtr->ui32NickListLen);
                    } else {
                        PutInSendBuf(clsUsers::mPtr->pZNickList, clsUsers::mPtr->ui32ZNickListLen);
                        clsServerManager::ui64BytesSentSaved += clsUsers::mPtr->ui32NickListLen-clsUsers::mPtr->ui32ZNickListLen;
                    }
                } else {
                    PutInSendBuf(clsUsers::mPtr->pZNickList, clsUsers::mPtr->ui32ZNickListLen);
                    clsServerManager::ui64BytesSentSaved += clsUsers::mPtr->ui32NickListLen-clsUsers::mPtr->ui32ZNickListLen;
                }
            }
        } else {
            // PPK ... OpChat bot is now visible only for OPs ;)
            int iLen = sprintf(msg, "%s$$|", clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_NICK]);
            if(CheckSprintf(iLen, 1024, "User::AddUserList") == true) {
                if(clsUsers::mPtr->ui32NickListSize < clsUsers::mPtr->ui32NickListLen+iLen) {
                    char * pOldBuf = clsUsers::mPtr->pNickList;
#ifdef _WIN32
                    clsUsers::mPtr->pNickList = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, clsUsers::mPtr->ui32NickListSize+NICKLISTSIZE+1);
#else
					clsUsers::mPtr->pNickList = (char *)realloc(pOldBuf, clsUsers::mPtr->ui32NickListSize+NICKLISTSIZE+1);
#endif
                    if(clsUsers::mPtr->pNickList == NULL) {
                        clsUsers::mPtr->pNickList = pOldBuf;
                        ui32BoolBits |= BIT_ERROR;
                        Close();

						AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes for nickList in User::AddUserList\n", (uint64_t)(clsUsers::mPtr->ui32NickListSize+NICKLISTSIZE+1));

                        return;
                    }
                    clsUsers::mPtr->ui32NickListSize += NICKLISTSIZE;
                }
    
                memcpy(clsUsers::mPtr->pNickList+clsUsers::mPtr->ui32NickListLen-1, msg, iLen);
                clsUsers::mPtr->pNickList[clsUsers::mPtr->ui32NickListLen+(iLen-1)] = '\0';
                SendCharDelayed(clsUsers::mPtr->pNickList, clsUsers::mPtr->ui32NickListLen+(iLen-1));
                clsUsers::mPtr->pNickList[clsUsers::mPtr->ui32NickListLen-1] = '|';
                clsUsers::mPtr->pNickList[clsUsers::mPtr->ui32NickListLen] = '\0';
            }
        }
	}
	
	switch(clsSettingManager::mPtr->ui8FullMyINFOOption) {
    	case 0: {
            if(clsUsers::mPtr->ui32MyInfosTagLen == 0) {
                break;
            }

            if(((ui32SupportBits & SUPPORTBIT_ZPIPE) == SUPPORTBIT_ZPIPE) == false) {
                SendCharDelayed(clsUsers::mPtr->pMyInfosTag, clsUsers::mPtr->ui32MyInfosTagLen);
            } else {
                if(clsUsers::mPtr->ui32ZMyInfosTagLen == 0) {
                    clsUsers::mPtr->pZMyInfosTag = clsZlibUtility::mPtr->CreateZPipe(clsUsers::mPtr->pMyInfosTag, clsUsers::mPtr->ui32MyInfosTagLen, clsUsers::mPtr->pZMyInfosTag,
                        clsUsers::mPtr->ui32ZMyInfosTagLen, clsUsers::mPtr->ui32ZMyInfosTagSize, Allign128K);
                    if(clsUsers::mPtr->ui32ZMyInfosTagLen == 0) {
                        SendCharDelayed(clsUsers::mPtr->pMyInfosTag, clsUsers::mPtr->ui32MyInfosTagLen);
                    } else {
                        PutInSendBuf(clsUsers::mPtr->pZMyInfosTag, clsUsers::mPtr->ui32ZMyInfosTagLen);
                        clsServerManager::ui64BytesSentSaved += clsUsers::mPtr->ui32MyInfosTagLen-clsUsers::mPtr->ui32ZMyInfosTagLen;
                    }
                } else {
                    PutInSendBuf(clsUsers::mPtr->pZMyInfosTag, clsUsers::mPtr->ui32ZMyInfosTagLen);
                    clsServerManager::ui64BytesSentSaved += clsUsers::mPtr->ui32MyInfosTagLen-clsUsers::mPtr->ui32ZMyInfosTagLen;
                }
            }
            break;
    	}
    	case 1: {
    		if(clsProfileManager::mPtr->IsAllowed(this, clsProfileManager::SENDFULLMYINFOS) == false) {
                if(clsUsers::mPtr->ui32MyInfosLen == 0) {
                    break;
                }

                if(((ui32SupportBits & SUPPORTBIT_ZPIPE) == SUPPORTBIT_ZPIPE) == false) {
                    SendCharDelayed(clsUsers::mPtr->pMyInfos, clsUsers::mPtr->ui32MyInfosLen);
                } else {
                    if(clsUsers::mPtr->ui32ZMyInfosLen == 0) {
                        clsUsers::mPtr->pZMyInfos = clsZlibUtility::mPtr->CreateZPipe(clsUsers::mPtr->pMyInfos, clsUsers::mPtr->ui32MyInfosLen, clsUsers::mPtr->pZMyInfos,
                            clsUsers::mPtr->ui32ZMyInfosLen, clsUsers::mPtr->ui32ZMyInfosSize, Allign128K);
                        if(clsUsers::mPtr->ui32ZMyInfosLen == 0) {
                            SendCharDelayed(clsUsers::mPtr->pMyInfos, clsUsers::mPtr->ui32MyInfosLen);
                        } else {
                            PutInSendBuf(clsUsers::mPtr->pZMyInfos, clsUsers::mPtr->ui32ZMyInfosLen);
                            clsServerManager::ui64BytesSentSaved += clsUsers::mPtr->ui32MyInfosLen-clsUsers::mPtr->ui32ZMyInfosLen;
                        }
                    } else {
                        PutInSendBuf(clsUsers::mPtr->pZMyInfos, clsUsers::mPtr->ui32ZMyInfosLen);
                        clsServerManager::ui64BytesSentSaved += clsUsers::mPtr->ui32MyInfosLen-clsUsers::mPtr->ui32ZMyInfosLen;
                    }
                }
    		} else {
                if(clsUsers::mPtr->ui32MyInfosTagLen == 0) {
                    break;
                }

                if(((ui32SupportBits & SUPPORTBIT_ZPIPE) == SUPPORTBIT_ZPIPE) == false) {
                    SendCharDelayed(clsUsers::mPtr->pMyInfosTag, clsUsers::mPtr->ui32MyInfosTagLen);
                } else {
                    if(clsUsers::mPtr->ui32ZMyInfosTagLen == 0) {
                        clsUsers::mPtr->pZMyInfosTag = clsZlibUtility::mPtr->CreateZPipe(clsUsers::mPtr->pMyInfosTag, clsUsers::mPtr->ui32MyInfosTagLen, clsUsers::mPtr->pZMyInfosTag,
                            clsUsers::mPtr->ui32ZMyInfosTagLen, clsUsers::mPtr->ui32ZMyInfosTagSize, Allign128K);
                        if(clsUsers::mPtr->ui32ZMyInfosTagLen == 0) {
                            SendCharDelayed(clsUsers::mPtr->pMyInfosTag, clsUsers::mPtr->ui32MyInfosTagLen);
                        } else {
                            PutInSendBuf(clsUsers::mPtr->pZMyInfosTag, clsUsers::mPtr->ui32ZMyInfosTagLen);
                            clsServerManager::ui64BytesSentSaved += clsUsers::mPtr->ui32MyInfosTagLen-clsUsers::mPtr->ui32ZMyInfosTagLen;
                        }
                    } else {
                        PutInSendBuf(clsUsers::mPtr->pZMyInfosTag, clsUsers::mPtr->ui32ZMyInfosTagLen);
                        clsServerManager::ui64BytesSentSaved += clsUsers::mPtr->ui32MyInfosTagLen-clsUsers::mPtr->ui32ZMyInfosTagLen;
                    }
                }
    		}
    		break;
    	}
        case 2: {
            if(clsUsers::mPtr->ui32MyInfosLen == 0) {
                break;
            }

            if(((ui32SupportBits & SUPPORTBIT_ZPIPE) == SUPPORTBIT_ZPIPE) == false) {
                SendCharDelayed(clsUsers::mPtr->pMyInfos, clsUsers::mPtr->ui32MyInfosLen);
            } else {
                if(clsUsers::mPtr->ui32ZMyInfosLen == 0) {
                    clsUsers::mPtr->pZMyInfos = clsZlibUtility::mPtr->CreateZPipe(clsUsers::mPtr->pMyInfos, clsUsers::mPtr->ui32MyInfosLen, clsUsers::mPtr->pZMyInfos,
                        clsUsers::mPtr->ui32ZMyInfosLen, clsUsers::mPtr->ui32ZMyInfosSize, Allign128K);
                    if(clsUsers::mPtr->ui32ZMyInfosLen == 0) {
                        SendCharDelayed(clsUsers::mPtr->pMyInfos, clsUsers::mPtr->ui32MyInfosLen);
                    } else {
                        PutInSendBuf(clsUsers::mPtr->pZMyInfos, clsUsers::mPtr->ui32ZMyInfosLen);
                        clsServerManager::ui64BytesSentSaved += clsUsers::mPtr->ui32MyInfosLen-clsUsers::mPtr->ui32ZMyInfosLen;
                    }
                } else {
                    PutInSendBuf(clsUsers::mPtr->pZMyInfos, clsUsers::mPtr->ui32ZMyInfosLen);
                    clsServerManager::ui64BytesSentSaved += clsUsers::mPtr->ui32MyInfosLen-clsUsers::mPtr->ui32ZMyInfosLen;
                }
            }
    	}
    	default:
            break;
    }
	
	if(clsProfileManager::mPtr->IsAllowed(this, clsProfileManager::ALLOWEDOPCHAT) == false || (clsSettingManager::mPtr->bBools[SETBOOL_REG_OP_CHAT] == false ||
        (clsSettingManager::mPtr->bBools[SETBOOL_REG_BOT] == true && clsSettingManager::mPtr->bBotsSameNick == true))) {
        if(clsUsers::mPtr->ui32OpListLen > 9) {
            if(((ui32SupportBits & SUPPORTBIT_ZPIPE) == SUPPORTBIT_ZPIPE) == false) {
                SendCharDelayed(clsUsers::mPtr->pOpList, clsUsers::mPtr->ui32OpListLen);
            } else {
                if(clsUsers::mPtr->ui32ZOpListLen == 0) {
                    clsUsers::mPtr->pZOpList = clsZlibUtility::mPtr->CreateZPipe(clsUsers::mPtr->pOpList, clsUsers::mPtr->ui32OpListLen, clsUsers::mPtr->pZOpList,
                        clsUsers::mPtr->ui32ZOpListLen, clsUsers::mPtr->ui32ZOpListSize, Allign16K);
                    if(clsUsers::mPtr->ui32ZOpListLen == 0) {
                        SendCharDelayed(clsUsers::mPtr->pOpList, clsUsers::mPtr->ui32OpListLen);
                    } else {
                        PutInSendBuf(clsUsers::mPtr->pZOpList, clsUsers::mPtr->ui32ZOpListLen);
                        clsServerManager::ui64BytesSentSaved += clsUsers::mPtr->ui32OpListLen-clsUsers::mPtr->ui32ZOpListLen;
                    }
                } else {
                    PutInSendBuf(clsUsers::mPtr->pZOpList, clsUsers::mPtr->ui32ZOpListLen);
                    clsServerManager::ui64BytesSentSaved += clsUsers::mPtr->ui32OpListLen-clsUsers::mPtr->ui32ZOpListLen;
                }  
            }
        }
    } else {
        // PPK ... OpChat bot is now visible only for OPs ;)
        SendCharDelayed(clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_OP_CHAT_MYINFO],
            clsSettingManager::mPtr->ui16PreTextsLens[clsSettingManager::SETPRETXT_OP_CHAT_MYINFO]);
        int iLen = sprintf(msg, "%s$$|", clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_NICK]);
        if(CheckSprintf(iLen, 1024, "User::AddUserList1") == true) {
            if(clsUsers::mPtr->ui32OpListSize < clsUsers::mPtr->ui32OpListLen+iLen) {
                char * pOldBuf = clsUsers::mPtr->pOpList;
#ifdef _WIN32
                clsUsers::mPtr->pOpList = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, clsUsers::mPtr->ui32OpListSize+OPLISTSIZE+1);
#else
				clsUsers::mPtr->pOpList = (char *)realloc(pOldBuf, clsUsers::mPtr->ui32OpListSize+OPLISTSIZE+1);
#endif
                if(clsUsers::mPtr->pOpList == NULL) {
                    clsUsers::mPtr->pOpList = pOldBuf;
                    ui32BoolBits |= BIT_ERROR;
                    Close();

                    AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes for opList in User::AddUserList\n", (uint64_t)(clsUsers::mPtr->ui32OpListSize+OPLISTSIZE+1));

                    return;
                }
                clsUsers::mPtr->ui32OpListSize += OPLISTSIZE;
            }
    
            memcpy(clsUsers::mPtr->pOpList+clsUsers::mPtr->ui32OpListLen-1, msg, iLen);
            clsUsers::mPtr->pOpList[clsUsers::mPtr->ui32OpListLen+(iLen-1)] = '\0';
            SendCharDelayed(clsUsers::mPtr->pOpList, clsUsers::mPtr->ui32OpListLen+(iLen-1));
            clsUsers::mPtr->pOpList[clsUsers::mPtr->ui32OpListLen-1] = '|';
            clsUsers::mPtr->pOpList[clsUsers::mPtr->ui32OpListLen] = '\0';
        }
    }

    if(clsProfileManager::mPtr->IsAllowed(this, clsProfileManager::SENDALLUSERIP) == true && ((ui32SupportBits & SUPPORTBIT_USERIP2) == SUPPORTBIT_USERIP2) == true) {
        if(clsUsers::mPtr->ui32UserIPListLen > 9) {
            if(((ui32SupportBits & SUPPORTBIT_ZPIPE) == SUPPORTBIT_ZPIPE) == false) {
                SendCharDelayed(clsUsers::mPtr->pUserIPList, clsUsers::mPtr->ui32UserIPListLen);
            } else {
                if(clsUsers::mPtr->ui32ZUserIPListLen == 0) {
                    clsUsers::mPtr->pZUserIPList = clsZlibUtility::mPtr->CreateZPipe(clsUsers::mPtr->pUserIPList, clsUsers::mPtr->ui32UserIPListLen, clsUsers::mPtr->pZUserIPList,
                        clsUsers::mPtr->ui32ZUserIPListLen, clsUsers::mPtr->ui32ZUserIPListSize, Allign16K);
                    if(clsUsers::mPtr->ui32ZUserIPListLen == 0) {
                        SendCharDelayed(clsUsers::mPtr->pUserIPList, clsUsers::mPtr->ui32UserIPListLen);
                    } else {
                        PutInSendBuf(clsUsers::mPtr->pZUserIPList, clsUsers::mPtr->ui32ZUserIPListLen);
                        clsServerManager::ui64BytesSentSaved += clsUsers::mPtr->ui32UserIPListLen-clsUsers::mPtr->ui32ZUserIPListLen;
                    }
                } else {
                    PutInSendBuf(clsUsers::mPtr->pZUserIPList, clsUsers::mPtr->ui32ZUserIPListLen);
                    clsServerManager::ui64BytesSentSaved += clsUsers::mPtr->ui32UserIPListLen-clsUsers::mPtr->ui32ZUserIPListLen;
                }  
            }
        }
    }
}
//---------------------------------------------------------------------------

bool User::GenerateMyInfoLong() { // true == changed
    // Prepare myinfo with nick
    int iLen = sprintf(msg, "$MyINFO $ALL %s ", sNick);
    if(CheckSprintf(iLen, 1024, "User::GenerateMyInfoLong") == false) {
        return false;
    }

    // Add description
    if(ui8ChangedDescriptionLongLen != 0) {
        if(sChangedDescriptionLong != NULL) {
            memcpy(msg+iLen, sChangedDescriptionLong, ui8ChangedDescriptionLongLen);
            iLen += ui8ChangedDescriptionLongLen;
        }

        if(((ui32InfoBits & INFOBIT_DESCRIPTION_LONG_PERM) == INFOBIT_DESCRIPTION_LONG_PERM) == false) {
            if(sChangedDescriptionLong != NULL) {
                User::FreeInfo(sChangedDescriptionLong, "sChangedDescriptionLong");
                sChangedDescriptionLong = NULL;
            }
            ui8ChangedDescriptionLongLen = 0;
        }
    } else if(sDescription != NULL) {
        memcpy(msg+iLen, sDescription, (size_t)ui8DescriptionLen);
        iLen += ui8DescriptionLen;
    }

    // Add tag
    if(ui8ChangedTagLongLen != 0) {
        if(sChangedTagLong != NULL) {
            memcpy(msg+iLen, sChangedTagLong, ui8ChangedTagLongLen);
            iLen += ui8ChangedTagLongLen;
        }

        if(((ui32InfoBits & INFOBIT_TAG_LONG_PERM) == INFOBIT_TAG_LONG_PERM) == false) {
            if(sChangedTagLong != NULL) {
                User::FreeInfo(sChangedTagLong, "sChangedTagLong");
                sChangedTagLong = NULL;
            }
            ui8ChangedTagLongLen = 0;
        }
    } else if(sTag != NULL) {
        memcpy(msg+iLen, sTag, (size_t)ui8TagLen);
        iLen += (int)ui8TagLen;
    }

    memcpy(msg+iLen, "$ $", 3);
    iLen += 3;

    // Add connection
    if(ui8ChangedConnectionLongLen != 0) {
        if(sChangedConnectionLong != NULL) {
            memcpy(msg+iLen, sChangedConnectionLong, ui8ChangedConnectionLongLen);
            iLen += ui8ChangedConnectionLongLen;
        }

        if(((ui32InfoBits & INFOBIT_CONNECTION_LONG_PERM) == INFOBIT_CONNECTION_LONG_PERM) == false) {
            if(sChangedConnectionLong != NULL) {
                User::FreeInfo(sChangedConnectionLong, "sChangedConnectionLong");
                sChangedConnectionLong = NULL;
            }
            ui8ChangedConnectionLongLen = 0;
        }
    } else if(sConnection != NULL) {
        memcpy(msg+iLen, sConnection, ui8ConnectionLen);
        iLen += ui8ConnectionLen;
    }

    // add magicbyte
    uint8_t ui8Magic = ui8MagicByte;

    if(((ui32BoolBits & User::SUPPORTBIT_TLS2) == User::SUPPORTBIT_TLS2) == false) {
    	// should not be set if user not have TLS2 support
        ui8Magic &= ~0x10;
        ui8Magic &= ~0x20;
    }

    if((ui32BoolBits & BIT_IPV4) == BIT_IPV4) {
        ui8Magic |= 0x40; // IPv4 support
    } else {
        ui8Magic &= ~0x40; // IPv4 support
    }

    if((ui32BoolBits & BIT_IPV6) == BIT_IPV6) {
        ui8Magic |= 0x80; // IPv6 support
    } else {
        ui8Magic &= ~0x80; // IPv6 support
    }

    msg[iLen] = ui8Magic;
    msg[iLen+1] = '$';
    iLen += 2;

    // Add email
    if(ui8ChangedEmailLongLen != 0) {
        if(sChangedEmailLong != NULL) {
            memcpy(msg+iLen, sChangedEmailLong, ui8ChangedEmailLongLen);
            iLen += ui8ChangedEmailLongLen;
        }

        if(((ui32InfoBits & INFOBIT_EMAIL_LONG_PERM) == INFOBIT_EMAIL_LONG_PERM) == false) {
            if(sChangedEmailLong != NULL) {
                User::FreeInfo(sChangedEmailLong, "sChangedEmailLong");
                sChangedEmailLong = NULL;
            }
            ui8ChangedEmailLongLen = 0;
        }
    } else if(sEmail != NULL) {
        memcpy(msg+iLen, sEmail, (size_t)ui8EmailLen);
        iLen += (int)ui8EmailLen;
    }

    // Add share and end of myinfo
	int iRet = sprintf(msg+iLen, "$%" PRIu64 "$|", ui64ChangedSharedSizeLong);
    iLen += iRet;

    if(((ui32InfoBits & INFOBIT_SHARE_LONG_PERM) == INFOBIT_SHARE_LONG_PERM) == false) {
        ui64ChangedSharedSizeLong = ui64SharedSize;
    }

    if(CheckSprintf1(iRet, iLen, 1024, "User::GenerateMyInfoLong2") == false) {
        return false;
    }

    if(sMyInfoLong != NULL) {
        if(ui16MyInfoLongLen == (uint16_t)iLen && memcmp(sMyInfoLong+14+ui8NickLen, msg+14+ui8NickLen, ui16MyInfoLongLen-14-ui8NickLen) == 0) {
            return false;
        }
    }

    UserSetMyInfoLong(this, msg, (uint16_t)iLen);

    return true;
}
//---------------------------------------------------------------------------

bool User::GenerateMyInfoShort() { // true == changed
    // Prepare myinfo with nick
    int iLen = sprintf(msg, "$MyINFO $ALL %s ", sNick);
    if(CheckSprintf(iLen, 1024, "User::GenerateMyInfoShort") == false) {
        return false;
    }

    // Add mode to start of description if is enabled
    if(clsSettingManager::mPtr->bBools[SETBOOL_MODE_TO_DESCRIPTION] == true && sModes[0] != 0) {
        char * sActualDescription = NULL;

        if(ui8ChangedDescriptionShortLen != 0) {
            sActualDescription = sChangedDescriptionShort;
        } else if(clsSettingManager::mPtr->bBools[SETBOOL_STRIP_DESCRIPTION] == true) {
            sActualDescription = sDescription;
        }

        if(sActualDescription == NULL) {
            msg[iLen] = sModes[0];
            iLen++;
        } else if(sActualDescription[0] != sModes[0] && sActualDescription[1] != ' ') {
            msg[iLen] = sModes[0];
            msg[iLen+1] = ' ';
            iLen += 2;
        }
    }

    // Add description
    if(ui8ChangedDescriptionShortLen != 0) {
        if(sChangedDescriptionShort != NULL) {
            memcpy(msg+iLen, sChangedDescriptionShort, ui8ChangedDescriptionShortLen);
            iLen += ui8ChangedDescriptionShortLen;
        }

        if(((ui32InfoBits & INFOBIT_DESCRIPTION_SHORT_PERM) == INFOBIT_DESCRIPTION_SHORT_PERM) == false) {
            if(sChangedDescriptionShort != NULL) {
                User::FreeInfo(sChangedDescriptionShort, "sChangedDescriptionShort");
                sChangedDescriptionShort = NULL;
            }
            ui8ChangedDescriptionShortLen = 0;
        }
    } else if(clsSettingManager::mPtr->bBools[SETBOOL_STRIP_DESCRIPTION] == false && sDescription != NULL) {
        memcpy(msg+iLen, sDescription, ui8DescriptionLen);
        iLen += ui8DescriptionLen;
    }

    // Add tag
    if(ui8ChangedTagShortLen != 0) {
        if(sChangedTagShort != NULL) {
            memcpy(msg+iLen, sChangedTagShort, ui8ChangedTagShortLen);
            iLen += ui8ChangedTagShortLen;
        }

        if(((ui32InfoBits & INFOBIT_TAG_SHORT_PERM) == INFOBIT_TAG_SHORT_PERM) == false) {
            if(sChangedTagShort != NULL) {
                User::FreeInfo(sChangedTagShort, "sChangedTagShort");
                sChangedTagShort = NULL;
            }
            ui8ChangedTagShortLen = 0;
        }
    } else if(clsSettingManager::mPtr->bBools[SETBOOL_STRIP_TAG] == false && sTag != NULL) {
        memcpy(msg+iLen, sTag, (size_t)ui8TagLen);
        iLen += (int)ui8TagLen;
    }

    // Add mode to myinfo if is enabled
    if(clsSettingManager::mPtr->bBools[SETBOOL_MODE_TO_MYINFO] == true && sModes[0] != 0) {
        int iRet = sprintf(msg+iLen, "$%c$", sModes[0]);
        iLen += iRet;
        if(CheckSprintf1(iRet, iLen, 1024, "GenerateMyInfoShort1") == false) {
            return false;
        }
    } else {
        memcpy(msg+iLen, "$ $", 3);
        iLen += 3;
    }

    // Add connection
    if(ui8ChangedConnectionShortLen != 0) {
        if(sChangedConnectionShort != NULL) {
            memcpy(msg+iLen, sChangedConnectionShort, ui8ChangedConnectionShortLen);
            iLen += ui8ChangedConnectionShortLen;
        }

        if(((ui32InfoBits & INFOBIT_CONNECTION_SHORT_PERM) == INFOBIT_CONNECTION_SHORT_PERM) == false) {
            if(sChangedConnectionShort != NULL) {
                User::FreeInfo(sChangedConnectionShort, "sChangedConnectionShort");
                sChangedConnectionShort = NULL;
            }
            ui8ChangedConnectionShortLen = 0;
        }
    } else if(clsSettingManager::mPtr->bBools[SETBOOL_STRIP_CONNECTION] == false && sConnection != NULL) {
        memcpy(msg+iLen, sConnection, ui8ConnectionLen);
        iLen += ui8ConnectionLen;
    }

    // add magicbyte
    uint8_t ui8Magic = ui8MagicByte;

    if(((ui32BoolBits & User::SUPPORTBIT_TLS2) == User::SUPPORTBIT_TLS2) == false) {
    	// should not be set if user not have TLS2 support
        ui8Magic &= ~0x10;
        ui8Magic &= ~0x20;
    }

    if((ui32BoolBits & BIT_IPV4) == BIT_IPV4) {
        ui8Magic |= 0x40; // IPv4 support
    } else {
        ui8Magic &= ~0x40; // IPv4 support
    }

    if((ui32BoolBits & BIT_IPV6) == BIT_IPV6) {
        ui8Magic |= 0x80; // IPv6 support
    } else {
        ui8Magic &= ~0x80; // IPv6 support
    }

    msg[iLen] = ui8Magic;
    msg[iLen+1] = '$';
    iLen += 2;

    // Add email
    if(ui8ChangedEmailShortLen != 0) {
        if(sChangedEmailShort != NULL) {
            memcpy(msg+iLen, sChangedEmailShort, ui8ChangedEmailShortLen);
            iLen += ui8ChangedEmailShortLen;
        }

        if(((ui32InfoBits & INFOBIT_EMAIL_SHORT_PERM) == INFOBIT_EMAIL_SHORT_PERM) == false) {
            if(sChangedEmailShort != NULL) {
                User::FreeInfo(sChangedEmailShort, "sChangedEmailShort");
                sChangedEmailShort = NULL;
            }
            ui8ChangedEmailShortLen = 0;
        }
    } else if(clsSettingManager::mPtr->bBools[SETBOOL_STRIP_EMAIL] == false && sEmail != NULL) {
        memcpy(msg+iLen, sEmail, (size_t)ui8EmailLen);
        iLen += (int)ui8EmailLen;
    }

    // Add share and end of myinfo
	int iRet = sprintf(msg+iLen, "$%" PRIu64 "$|", ui64ChangedSharedSizeShort);
    iLen += iRet;

    if(((ui32InfoBits & INFOBIT_SHARE_SHORT_PERM) == INFOBIT_SHARE_SHORT_PERM) == false) {
        ui64ChangedSharedSizeShort = ui64SharedSize;
    }

    if(CheckSprintf1(iRet, iLen, 1024, "User::GenerateMyInfoShort2") == false) {
        return false;
    }

    if(sMyInfoShort != NULL) {
        if(ui16MyInfoShortLen == (uint16_t)iLen && memcmp(sMyInfoShort+14+ui8NickLen, msg+14+ui8NickLen, ui16MyInfoShortLen-14-ui8NickLen) == 0) {
            return false;
        }
    }

    UserSetMyInfoShort(this, msg, (uint16_t)iLen);

    return true;
}
//---------------------------------------------------------------------------

#ifdef _WIN32
void User::FreeInfo(char * sInfo, const char * sName) {
	if(sInfo != NULL) {
		if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sInfo) == 0) {
			AppendDebugLog(("%s - [MEM] Cannot deallocate "+string(sName)+" in User::FreeInfo\n").c_str(), 0);
        }
    }
#else
void User::FreeInfo(char * sInfo, const char */* sName*/) {
	free(sInfo);
#endif
}
//------------------------------------------------------------------------------

void User::HasSuspiciousTag() {
	if(clsSettingManager::mPtr->bBools[SETBOOL_REPORT_SUSPICIOUS_TAG] == true && clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
		if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
			int imsgLen = sprintf(msg, "%s $<%s> *** %s (%s) %s. %s: ", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC],
                sNick, sIP, clsLanguageManager::mPtr->sTexts[LAN_HAS_SUSPICIOUS_TAG_CHECK_HIM], clsLanguageManager::mPtr->sTexts[LAN_FULL_DESCRIPTION]);
            if(CheckSprintf(imsgLen, 1024, "UserHasSuspiciousTag1") == true) {
                memcpy(msg+imsgLen, sDescription, (size_t)ui8DescriptionLen);
                imsgLen += (int)ui8DescriptionLen;
                msg[imsgLen] = '|';
                clsGlobalDataQueue::mPtr->SingleItemStore(msg, imsgLen+1, NULL, 0, clsGlobalDataQueue::SI_PM2OPS);
            }
		} else {
			int imsgLen = sprintf(msg, "<%s> *** %s (%s) %s. %s: ", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], sNick, sIP,
                clsLanguageManager::mPtr->sTexts[LAN_HAS_SUSPICIOUS_TAG_CHECK_HIM], clsLanguageManager::mPtr->sTexts[LAN_FULL_DESCRIPTION]);
            if(CheckSprintf(imsgLen, 1024, "UserHasSuspiciousTag2") == true) {
                memcpy(msg+imsgLen, sDescription, (size_t)ui8DescriptionLen);
                imsgLen += (int)ui8DescriptionLen;
                msg[imsgLen] = '|';
                clsGlobalDataQueue::mPtr->AddQueueItem(msg, imsgLen+1, NULL, 0, clsGlobalDataQueue::CMD_OPS);
            }
	   }
    }
    ui32BoolBits &= ~BIT_HAVE_BADTAG;
}
//---------------------------------------------------------------------------

bool User::ProcessRules() {
    // if share limit enabled, check it
    if(clsProfileManager::mPtr->IsAllowed(this, clsProfileManager::NOSHARELIMIT) == false) {      
        if((clsSettingManager::mPtr->ui64MinShare != 0 && ui64SharedSize < clsSettingManager::mPtr->ui64MinShare) ||
            (clsSettingManager::mPtr->ui64MaxShare != 0 && ui64SharedSize > clsSettingManager::mPtr->ui64MaxShare)) {
            SendChar(clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_SHARE_LIMIT_MSG], clsSettingManager::mPtr->ui16PreTextsLens[clsSettingManager::SETPRETXT_SHARE_LIMIT_MSG]);
            //clsUdpDebug::mPtr->Broadcast("[SYS] User with low or high share " + Nick + " (" + IP + ") disconnected.");
            return false;
        }
    }
    
    // no Tag? Apply rule
    if(sTag == NULL) {
        if(clsProfileManager::mPtr->IsAllowed(this, clsProfileManager::NOTAGCHECK) == false) {
            if(clsSettingManager::mPtr->i16Shorts[SETSHORT_NO_TAG_OPTION] != 0) {
                SendChar(clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_NO_TAG_MSG], clsSettingManager::mPtr->ui16PreTextsLens[clsSettingManager::SETPRETXT_NO_TAG_MSG]);
                //clsUdpDebug::mPtr->Broadcast("[SYS] User without Tag " + Nick + (" + IP + ") redirected.");
                return false;
            }
        }
    } else {
        // min and max slot check
        if(clsProfileManager::mPtr->IsAllowed(this, clsProfileManager::NOSLOTCHECK) == false) {
            // TODO 2 -oPTA -ccheckers: $SR based slots fetching for no_tag users
        
			if((clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_SLOTS_LIMIT] != 0 && Slots < (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_SLOTS_LIMIT]) ||
				(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_SLOTS_LIMIT] != 0 && Slots > (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_SLOTS_LIMIT])) {
                SendChar(clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_SLOTS_LIMIT_MSG], clsSettingManager::mPtr->ui16PreTextsLens[clsSettingManager::SETPRETXT_SLOTS_LIMIT_MSG]);
                //clsUdpDebug::mPtr->Broadcast("[SYS] User with bad slots " + Nick + " (" + IP + ") disconnected.");
                return false;
            }
        }
    
        // slots/hub ration check
        if(clsProfileManager::mPtr->IsAllowed(this, clsProfileManager::NOSLOTHUBRATIO) == false && 
            clsSettingManager::mPtr->i16Shorts[SETSHORT_HUB_SLOT_RATIO_HUBS] != 0 && clsSettingManager::mPtr->i16Shorts[SETSHORT_HUB_SLOT_RATIO_SLOTS] != 0) {
            uint32_t slots = Slots;
            uint32_t hubs = Hubs > 0 ? Hubs : 1;
        	if(((double)slots / hubs) < ((double)clsSettingManager::mPtr->i16Shorts[SETSHORT_HUB_SLOT_RATIO_SLOTS] / clsSettingManager::mPtr->i16Shorts[SETSHORT_HUB_SLOT_RATIO_HUBS])) {
        	    SendChar(clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SLOT_RATIO_MSG], clsSettingManager::mPtr->ui16PreTextsLens[clsSettingManager::SETPRETXT_HUB_SLOT_RATIO_MSG]);
                //clsUdpDebug::mPtr->Broadcast("[SYS] User with bad hub/slot ratio " + Nick + " (" + IP + ") disconnected.");
                return false;
            }
        }
    
        // hub checker
        if(clsProfileManager::mPtr->IsAllowed(this, clsProfileManager::NOMAXHUBCHECK) == false && clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_HUBS_LIMIT] != 0) {
            if(Hubs > (uint32_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_HUBS_LIMIT]) {
                SendChar(clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_MAX_HUBS_LIMIT_MSG], clsSettingManager::mPtr->ui16PreTextsLens[clsSettingManager::SETPRETXT_MAX_HUBS_LIMIT_MSG]);
                //clsUdpDebug::mPtr->Broadcast("[SYS] User with bad hubs count " + Nick + " (" + IP + ") disconnected.");
                return false;
            }
        }
    }
    
    return true;
}

//------------------------------------------------------------------------------

void User::AddPrcsdCmd(const uint8_t &ui8Type, char * sCommand, const size_t &szCommandLen, User * to, const bool &bIsPm/* = false*/) {
    if(ui8Type == PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO) {
        PrcsdToUsrCmd * cur = NULL,
            * next = pCmdToUserStrt;

        while(next != NULL) {
            cur = next;
            next = cur->pNext;

            if(cur->pTo == to) {
                char * pOldBuf = cur->sCommand;
#ifdef _WIN32
                cur->sCommand = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, cur->ui32Len+szCommandLen+1);
#else
				cur->sCommand = (char *)realloc(pOldBuf, cur->ui32Len+szCommandLen+1);
#endif
                if(cur->sCommand == NULL) {
                    cur->sCommand = pOldBuf;
                    ui32BoolBits |= BIT_ERROR;
                    Close();

					AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in User::AddPrcsdCmd\n", (uint64_t)(cur->ui32Len+szCommandLen+1));

                    return;
                }
                memcpy(cur->sCommand+cur->ui32Len, sCommand, szCommandLen);
                cur->sCommand[cur->ui32Len+szCommandLen] = '\0';
                cur->ui32Len += (uint32_t)szCommandLen;
                cur->ui32PmCount += bIsPm == true ? 1 : 0;
                return;
            }
        }

        PrcsdToUsrCmd * pNewToCmd = new (std::nothrow) PrcsdToUsrCmd();
        if(pNewToCmd == NULL) {
            ui32BoolBits |= BIT_ERROR;
            Close();

			AppendDebugLog("%s - [MEM] User::AddPrcsdCmd cannot allocate new pNewToCmd\n", 0);

        	return;
        }

#ifdef _WIN32
        pNewToCmd->sCommand = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szCommandLen+1);
#else
		pNewToCmd->sCommand = (char *)malloc(szCommandLen+1);
#endif
        if(pNewToCmd->sCommand == NULL) {
            ui32BoolBits |= BIT_ERROR;
            Close();

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sCommand in User::AddPrcsdCmd\n", (uint64_t)(szCommandLen+1));

            delete pNewToCmd;

            return;
        }

        memcpy(pNewToCmd->sCommand, sCommand, szCommandLen);
        pNewToCmd->sCommand[szCommandLen] = '\0';

        pNewToCmd->ui32Len = (uint32_t)szCommandLen;
        pNewToCmd->ui32PmCount = bIsPm == true ? 1 : 0;
        pNewToCmd->ui32Loops = 0;
        pNewToCmd->pTo = to;
        
#ifdef _WIN32
        pNewToCmd->sToNick = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, to->ui8NickLen+1);
#else
		pNewToCmd->sToNick = (char *)malloc(to->ui8NickLen+1);
#endif
        if(pNewToCmd->sToNick == NULL) {
            ui32BoolBits |= BIT_ERROR;
            Close();

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for ToNick in User::AddPrcsdCmd\n", (uint64_t)(to->ui8NickLen+1));

#ifdef _WIN32
            if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pNewToCmd->sCommand) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate pNewToCmd->sCommand in User::AddPrcsdCmd\n", 0);
			}
#else
            free(pNewToCmd->sCommand);
#endif

            delete pNewToCmd;

            return;
        }   

        memcpy(pNewToCmd->sToNick, to->sNick, to->ui8NickLen);
        pNewToCmd->sToNick[to->ui8NickLen] = '\0';
        
        pNewToCmd->ui32ToNickLen = to->ui8NickLen;
        pNewToCmd->pNext = NULL;
               
        if(pCmdToUserStrt == NULL) {
            pCmdToUserStrt = pNewToCmd;
            pCmdToUserEnd = pNewToCmd;
        } else {
            pCmdToUserEnd->pNext = pNewToCmd;
            pCmdToUserEnd = pNewToCmd;
        }

        return;
    }
    
    PrcsdUsrCmd * pNewcmd = new (std::nothrow) PrcsdUsrCmd();
    if(pNewcmd == NULL) {
        ui32BoolBits |= BIT_ERROR;
        Close();

		AppendDebugLog("%s - [MEM] User::AddPrcsdCmd cannot allocate new pNewcmd1\n", 0);

    	return;
    }

#ifdef _WIN32
    pNewcmd->sCommand = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szCommandLen+1);
#else
	pNewcmd->sCommand = (char *)malloc(szCommandLen+1);
#endif
    if(pNewcmd->sCommand == NULL) {
        ui32BoolBits |= BIT_ERROR;
        Close();

		AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sCommand in User::AddPrcsdCmd1\n", (uint64_t)(szCommandLen+1));

        delete pNewcmd;

        return;
    }

    memcpy(pNewcmd->sCommand, sCommand, szCommandLen);
    pNewcmd->sCommand[szCommandLen] = '\0';

    pNewcmd->ui32Len = (uint32_t)szCommandLen;
    pNewcmd->ui8Type = ui8Type;
    pNewcmd->pNext = NULL;
    pNewcmd->pPtr = (void *)to;

    if(pCmdStrt == NULL) {
        pCmdStrt = pNewcmd;
        pCmdEnd = pNewcmd;
    } else {
        pCmdEnd->pNext = pNewcmd;
        pCmdEnd = pNewcmd;
    }
}
//---------------------------------------------------------------------------

void User::AddMeOrIPv4Check() {
    if(((ui32BoolBits & BIT_IPV6) == BIT_IPV6) && ((ui32SupportBits & SUPPORTBIT_IPV4) == SUPPORTBIT_IPV4) && clsServerManager::sHubIP[0] != '\0' && clsServerManager::bUseIPv4 == true) {
        ui8State = STATE_IPV4_CHECK;

        int imsgLen = sprintf(msg, "$ConnectToMe %s %s:%s|", sNick, clsServerManager::sHubIP, string(clsSettingManager::mPtr->ui16PortNumbers[0]).c_str());
        if(CheckSprintf(imsgLen, 1024, "User::AddMeOrIPv4Check") == false) {
            return;
        }

        pLogInOut->ui64IPv4CheckTick = clsServerManager::ui64ActualTick;

        SendCharDelayed(msg, imsgLen);
    } else {
        ui8State = STATE_ADDME;
    }
}
//---------------------------------------------------------------------------

char * User::SetUserInfo(char * sOldData, uint8_t &ui8OldDataLen, char * sNewData, size_t &sz8NewDataLen, const char * sDataName) {
    if(sOldData != NULL) {
        User::FreeInfo(sOldData, sDataName);
        sOldData = NULL;
        ui8OldDataLen = 0;
    }

    if(sz8NewDataLen > 0) {
#ifdef _WIN32
        sOldData = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, sz8NewDataLen+1);
#else
        sOldData = (char *)malloc(sz8NewDataLen+1);
#endif
        if(sOldData == NULL) {
            AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes in User::SetUserInfo\n", (uint64_t)(sz8NewDataLen+1));
            return sOldData;
        }

        memcpy(sOldData, sNewData, sz8NewDataLen);
        sOldData[sz8NewDataLen] = '\0';
        ui8OldDataLen = (uint8_t)sz8NewDataLen;
    } else {
        ui8OldDataLen = 1;
    }

    return sOldData;
}
//---------------------------------------------------------------------------

void User::RemFromSendBuf(const char * sData, const uint32_t &iLen, const uint32_t &iSbLen) {
	char *match = strstr(pSendBuf+iSbLen, sData);
    if(match != NULL) {
        memmove(match, match+iLen, ui32SendBufDataLen-((match+(iLen))-pSendBuf));
        ui32SendBufDataLen -= iLen;
        pSendBuf[ui32SendBufDataLen] = '\0';
    }
}
//------------------------------------------------------------------------------

void User::DeletePrcsdUsrCmd(PrcsdUsrCmd * pCommand) {
#ifdef _WIN32
    if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pCommand->sCommand) == 0) {
        AppendDebugLog("%s - [MEM] Cannot deallocate pCommand->sCommand in User::DeletePrcsdUsrCmd\n", 0);
    }
#else
    free(pCommand->sCommand);
#endif
    delete pCommand;
}
