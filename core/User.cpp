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
	if(u->recvbuf[0] == '\0')
        return false;

    char c;
    
    char *buffer = u->recvbuf;

    for(uint32_t ui32i = iStrtLen; ui32i < u->rbdatalen; ui32i++) {
        if(u->recvbuf[ui32i] == '|') {
            // look for pipes in the data - process lines one by one
            c = u->recvbuf[ui32i+1];
            u->recvbuf[ui32i+1] = '\0';
            uint32_t ui32iCommandLen = (uint32_t)(((u->recvbuf+ui32i)-buffer)+1);
            if(buffer[0] == '|') {
                //UdpDebug->Broadcast("[SYS] heartbeat from " + string(u->Nick, u->NickLen));
                //send(Sck, "|", 1, 0);
            } else if(ui32iCommandLen < (u->ui8State < User::STATE_ADDME ? 1024U : 65536U)) {
        		DcCommands->PreProcessData(u, buffer, true, ui32iCommandLen);
        	} else {
                int imsgLen = sprintf(msg, "<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_CMD_TOO_LONG]);
                if(CheckSprintf(imsgLen, 1024, "UserProcessLines1") == true) {
                    UserSendCharDelayed(u, msg, imsgLen);
                }
				UserClose(u);
				UdpDebug->Broadcast("[SYS] " + string(u->sNick, u->ui8NickLen) + " (" + string(u->sIP, u->ui8IpLen) + ") Received command too long. User disconnected.");
                return false;
            }
        	u->recvbuf[ui32i+1] = c;
        	buffer += ui32iCommandLen;
            if(u->ui8State >= User::STATE_CLOSING) {
                return true;
            }
        } else if(u->recvbuf[ui32i] == '\0') {
            // look for NULL character and replace with zero
            u->recvbuf[ui32i] = '0';
            continue;
        }
	}

	u->rbdatalen -= (uint32_t)(buffer-u->recvbuf);

	if(u->rbdatalen == 0) {
        DcCommands->ProcessCmds(u);
        u->recvbuf[0] = '\0';
        return false;
    } else if(u->rbdatalen != 1) {
        if(u->rbdatalen > unsigned (u->ui8State < User::STATE_ADDME ? 1024 : 65536)) {
            // PPK ... we don't want commands longer than 64 kB, drop this user !
            int imsgLen = sprintf(msg, "<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_CMD_TOO_LONG]);
            if(CheckSprintf(imsgLen, 1024, "UserProcessLines2") == true) {
                UserSendCharDelayed(u, msg, imsgLen);
            }
            UserClose(u);
			UdpDebug->Broadcast("[SYS] " + string(u->sNick, u->ui8NickLen) + " (" + string(u->sIP, u->ui8IpLen) +") RecvBuffer overflow. User disconnected.");
        	return false;
        }
        DcCommands->ProcessCmds(u);
        memmove(u->recvbuf, buffer, u->rbdatalen);
        u->recvbuf[u->rbdatalen] = '\0';
        return true;
    } else {
        DcCommands->ProcessCmds(u);
        if(buffer[0] == '|') {
            u->recvbuf[0] = '\0';
            u->rbdatalen = 0;
            return false;
        } else {
            u->recvbuf[0] = buffer[0];
            u->recvbuf[1] = '\0';
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
        UdpDebug->Broadcast(msg, imsgLen);
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
        int imsgLen = sprintf(msg, "<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_MyINFO_IS_CORRUPTED]);
        if(CheckSprintf(imsgLen, 1024, "UserParseMyInfo1") == true) {
            UserSendChar(u, msg, imsgLen);
        }
        imsgLen = sprintf(msg, "[SYS] User %s (%s) with bad MyINFO (%s) disconnected.", u->sNick, u->sIP, u->sMyInfoOriginal);
        if(CheckSprintf(imsgLen, 1024, "UserParseMyInfo2") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(u);
        return;
    }

    // connection
    u->MagicByte = sMyINFOParts[2][iMyINFOPartsLen[2]-1];
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
        //UdpDebug->Broadcast("[SYS] User " + string(u->Nick, u->NickLen) + " (" + string(u->IP, u->ui8IpLen) + ") with non-numeric sharesize disconnected.");
        UserClose(u);
        return;
    }
            
    if(((u->ui32BoolBits & User::BIT_HAVE_SHARECOUNTED) == User::BIT_HAVE_SHARECOUNTED) == true) {
        ui64TotalShare -= u->ui64SharedSize;
#ifdef _WIN32
        u->ui64SharedSize = _strtoui64(sMyINFOParts[4], NULL, 10);
#else
		u->ui64SharedSize = strtoull(sMyINFOParts[4], NULL, 10);
#endif
        ui64TotalShare += u->ui64SharedSize;
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
                            int imsgLen = sprintf(msg, "<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_FAKE_TAG]);
                            if(CheckSprintf(imsgLen, 1024, "UserParseMyInfo3") == true) {
                                UserSendChar(u, msg, imsgLen);
                            }
                            imsgLen = sprintf(msg, "[SYS] User %s (%s) with fake Tag disconnected: ", u->sNick, u->sIP);
                            if(CheckSprintf(imsgLen, 1024, "UserParseMyInfo4") == true) {
                                memcpy(msg+imsgLen, u->sTag, u->ui8TagLen);
                                UdpDebug->Broadcast(msg, imsgLen+(int)u->ui8TagLen);
                            }
                            UserClose(u);
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
                            //UdpDebug->Broadcast("[SYS] "+string(u->Nick, u->NickLen)+": Extra info in DC tag: "+string(sTag));
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

UserBan::UserBan(char * sMess, const uint32_t &iMessLen, const uint32_t &ui32Hash) {
#ifdef _WIN32
    sMessage = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iMessLen+1);
#else
	sMessage = (char *)malloc(iMessLen+1);
#endif
    if(sMessage == NULL) {
        AppendDebugLog("%s - [MEM] UserBan::UserBann cannot allocate %" PRIu64 " bytes for sMessage\n", (uint64_t)(iMessLen+1));
        return;
    }

    memcpy(sMessage, sMess, iMessLen);
    sMessage[iMessLen] = '\0';

    iLen = iMessLen;
    ui32NickHash = ui32Hash;
}
//---------------------------------------------------------------------------

UserBan::~UserBan() {
#ifdef _WIN32
    if(sMessage != NULL && HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMessage) == 0) {
        AppendDebugLog("%s - [MEM] Cannot deallocate sMessage in UserBan::~UserBan\n", 0);
    }
#else
	free(sMessage);
#endif
    sMessage = NULL;
}
//---------------------------------------------------------------------------


LoginLogout::LoginLogout() {
    uBan = NULL;

    sLockUsrConn = NULL;
    sPassword = NULL;
    sKickMsg = NULL;

    iToCloseLoops = 0;
    iUserConnectedLen = 0;

    logonClk = 0;
    ui64IPv4CheckTick = 0;
}
//---------------------------------------------------------------------------

LoginLogout::~LoginLogout() {
    delete uBan;

#ifdef _WIN32
    if(sLockUsrConn != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sLockUsrConn) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate sLockUsrConn in LoginLogout::~LoginLogout\n", 0);
        }
    }
#else
	free(sLockUsrConn);
#endif

#ifdef _WIN32
    if(sPassword != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPassword) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate sPassword in LoginLogout::~LoginLogout\n", 0);
        }
    }
#else
	free(sPassword);
#endif

#ifdef _WIN32
    if(sKickMsg != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sKickMsg) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate sKickMsg in LoginLogout::~LoginLogout\n", 0);
        }
    }
#else
	free(sKickMsg);
#endif
}
//---------------------------------------------------------------------------

User::User() {
	iSendCalled = 0;
	iRecvCalled = 0;

#ifdef _WIN32
	Sck = INVALID_SOCKET;
#else
	Sck = -1;
#endif

	prev = NULL;
	next = NULL;
	hashtableprev = NULL;
	hashtablenext = NULL;
	hashiptableprev = NULL;
	hashiptablenext = NULL;

    sMyInfoOriginal = NULL;
    sMyInfoShort = NULL;
    sMyInfoLong = NULL;
	sLastChat = NULL;
	sLastPM = NULL;
	sendbuf = NULL;
	recvbuf = NULL;
	sbplayhead = NULL;
	sLastSearch = NULL;
	sNick = (char *)sDefaultNick;
	sIP[0] = '\0';
	sIPv4[0] = '\0';
	sVersion = NULL;
	sClient = (char *)sOtherNoTag;
	sTag = NULL;
	sDescription = NULL;
	sConnection = NULL;
	MagicByte = '\0';
	sEmail = NULL;
	sTagVersion = NULL;
	sModes[0] = '\0';

    sChangedDescriptionShort = NULL;
    sChangedDescriptionLong = NULL;
    sChangedTagShort = NULL;
    sChangedTagLong = NULL;
    sChangedConnectionShort = NULL;
    sChangedConnectionLong = NULL;
    sChangedEmailShort = NULL;
    sChangedEmailLong = NULL;

	ui8IpLen = 0;
	ui8IPv4Len = 0;

    ui16MyInfoOriginalLen = 0;
    ui16MyInfoShortLen = 0;
    ui16MyInfoLongLen = 0;
	ui16GetNickLists = 0;
	ui16MyINFOs = 0;
	ui16Searchs = 0;
	ui16ChatMsgs = 0;
	ui16PMs = 0;
	ui16SameSearchs = 0;
	ui16LastSearchLen = 0;
	ui16SamePMs = 0;
	ui16LastPMLen = 0;
	ui16SameChatMsgs = 0;
	ui16LastChatLen = 0;
	ui16LastPmLines = 0;
	ui16SameMultiPms = 0;
	ui16LastChatLines = 0;
	ui16SameMultiChats = 0;
	ui16ChatMsgs2 = 0;
	ui16PMs2 = 0;
	ui16Searchs2 = 0;
	ui16MyINFOs2 = 0;
	ui16CTMs = 0;
	ui16CTMs2 = 0;
	ui16RCTMs = 0;
	ui16RCTMs2 = 0;
	ui16SRs = 0;
	ui16SRs2 = 0;
	ui16ChatIntMsgs = 0;
	ui16PMsInt = 0;
	ui16SearchsInt = 0;

    ui32Recvs = 0;
    ui32Recvs2 = 0;

	ui8NickLen = 9;
	iSR = 0;
	iDefloodWarnings = 0;
	Hubs = 0;
	Slots = 0;
	OLimit = 0;
	LLimit = 0;
	DLimit = 0;
	iNormalHubs = 0;
	iRegHubs = 0;
	iOpHubs = 0;
	ui8State = User::STATE_SOCKET_ACCEPTED;

	iProfile = -1;
	sendbuflen = 0;
	recvbuflen = 0;
	sbdatalen = 0;
	rbdatalen = 0;

	iReceivedPmCount = 0;

	time(&LoginTime);

	ui32NickHash = 0;

    memset(&ui128IpHash, 0, 16);
    ui16IpTableIdx = 0;

	ui32BoolBits = 0;
	ui32BoolBits |= User::BIT_IPV4_ACTIVE;
	ui32BoolBits |= User::BIT_OLDHUBSTAG;

    ui32InfoBits = 0;
    ui32SupportBits = 0;

	ui8ConnectionLen = 0;
	ui8DescriptionLen = 0;
	ui8EmailLen = 0;
	ui8TagLen = 0;
	ui8ClientLen = 14;
	ui8TagVersionLen = 0;

    ui8ChangedDescriptionShortLen = 0;
    ui8ChangedDescriptionLongLen = 0;
    ui8ChangedTagShortLen = 0;
    ui8ChangedTagLongLen = 0;
    ui8ChangedConnectionShortLen = 0;
    ui8ChangedConnectionLongLen = 0;
    ui8ChangedEmailShortLen = 0;
    ui8ChangedEmailLongLen = 0;

    ui8Country = 246;

	ui64SharedSize = 0;
	ui64ChangedSharedSizeShort = 0;
    ui64ChangedSharedSizeLong = 0;

	ui64GetNickListsTick = 0;
	ui64MyINFOsTick = 0;
	ui64SearchsTick = 0;
	ui64ChatMsgsTick = 0;
	ui64PMsTick = 0;
	ui64SameSearchsTick = 0;
	ui64SamePMsTick = 0;
	ui64SameChatsTick = 0;
	ui64ChatMsgsTick2 = 0;
	ui64PMsTick2 = 0;
	ui64SearchsTick2 = 0;
	ui64MyINFOsTick2 = 0;
	ui64CTMsTick = 0;
	ui64CTMsTick2 = 0;
	ui64RCTMsTick = 0;
	ui64RCTMsTick2 = 0;
	ui64SRsTick = 0;
	ui64SRsTick2 = 0;
	ui64RecvsTick = 0;
	ui64RecvsTick2 = 0;
	ui64ChatIntMsgsTick = 0;
	ui64PMsIntTick = 0;
	ui64SearchsIntTick = 0;

	iLastMyINFOSendTick = 0;
    iLastNicklist = 0;
	iReceivedPmTick = 0;

	cmdStrt = NULL;
	cmdEnd = NULL;

	cmdActive4Search = NULL;
	cmdActive6Search = NULL;
	cmdPassiveSearch = NULL;

	cmdToUserStrt = NULL;
	cmdToUserEnd = NULL;

	uLogInOut = NULL;
}
//---------------------------------------------------------------------------

User::~User() {
#ifdef _WIN32
	if(recvbuf != NULL) {
		if(HeapFree(hRecvHeap, HEAP_NO_SERIALIZE, (void *)recvbuf) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate recvbuf in User::~User\n", 0);
        }
    }
#else
	free(recvbuf);
#endif

#ifdef _WIN32
	if(sendbuf != NULL) {
		if(HeapFree(hSendHeap, HEAP_NO_SERIALIZE, (void *)sendbuf) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sendbuf in User::~User\n", 0);
        }
    }
#else
	free(sendbuf);
#endif

#ifdef _WIN32
	if(sLastChat != NULL) {
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sLastChat) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sLastChat in User::~User\n", 0);
        }
    }
#else
	free(sLastChat);
#endif

#ifdef _WIN32
	if(sLastPM != NULL) {
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sLastPM) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sLastPM in User::~User\n", 0);
        }
    }
#else
	free(sLastPM);
#endif

#ifdef _WIN32
	if(sLastSearch != NULL) {
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sLastSearch) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sLastSearch in User::~User\n", 0);
        }
    }
#else
	free(sLastSearch);
#endif

#ifdef _WIN32
	if(sMyInfoShort != NULL) {
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMyInfoShort) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sMyInfoShort in User::~User\n", 0);
        }
    }
#else
	free(sMyInfoShort);
#endif

#ifdef _WIN32
	if(sMyInfoLong != NULL) {
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMyInfoLong) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sMyInfoLong in User::~User\n", 0);
        }
    }
#else
	free(sMyInfoLong);
#endif

#ifdef _WIN32
	if(sMyInfoOriginal != NULL) {
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMyInfoOriginal) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sMyInfoOriginal in User::~User\n", 0);
        }
    }
#else
	free(sMyInfoOriginal);
#endif

#ifdef _WIN32
	if(sVersion != NULL) {
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sVersion) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sVersion in User::~User\n", 0);
        }
    }
#else
	free(sVersion);
#endif

#ifdef _WIN32
	if(sChangedDescriptionShort != NULL) {
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sChangedDescriptionShort) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sChangedDescriptionShort in User::~User\n", 0);
        }
    }
#else
	free(sChangedDescriptionShort);
#endif

#ifdef _WIN32
	if(sChangedDescriptionLong != NULL) {
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sChangedDescriptionLong) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sChangedDescriptionLong in User::~User\n", 0);
        }
    }
#else
	free(sChangedDescriptionLong);
#endif

#ifdef _WIN32
	if(sChangedTagShort != NULL) {
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sChangedTagShort) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sChangedTagShort in User::~User\n", 0);
        }
    }
#else
	free(sChangedTagShort);
#endif

#ifdef _WIN32
	if(sChangedTagLong != NULL) {
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sChangedTagLong) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sChangedTagLong in User::~User\n", 0);
        }
    }
#else
	free(sChangedTagLong);
#endif

#ifdef _WIN32
	if(sChangedConnectionShort != NULL) {
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sChangedConnectionShort) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sChangedConnectionShort in User::~User\n", 0);
        }
    }
#else
	free(sChangedConnectionShort);
#endif

#ifdef _WIN32
	if(sChangedConnectionLong != NULL) {
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sChangedConnectionLong) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sChangedConnectionLong in User::~User\n", 0);
        }
    }
#else
	free(sChangedConnectionLong);
#endif

#ifdef _WIN32
	if(sChangedEmailShort != NULL) {
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sChangedEmailShort) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sChangedEmailShort in User::~User\n", 0);
        }
    }
#else
	free(sChangedEmailShort);
#endif

#ifdef _WIN32
	if(sChangedEmailLong != NULL) {
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sChangedEmailLong) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sChangedEmailLong in User::~User\n", 0);
        }
    }
#else
	free(sChangedEmailLong);
#endif

	if(((ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == true)
        DcCommands->iStatZPipe--;

	ui32Parts++;

#ifdef _BUILD_GUI
    if(::SendMessage(pMainWindowPageUsersChat->hWndPageItems[MainWindowPageUsersChat::BTN_SHOW_COMMANDS], BM_GETCHECK, 0, 0) == BST_CHECKED) {
        RichEditAppendText(pMainWindowPageUsersChat->hWndPageItems[MainWindowPageUsersChat::REDT_CHAT], ("x User removed: " + string(sNick, ui8NickLen) + " (Socket " + string(Sck) + ")").c_str());
    }
#endif

	if(sNick != sDefaultNick) {
#ifdef _WIN32
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sNick) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sNick in User::~User\n", 0);
		}
#else
		free(sNick);
#endif
	}
        
	delete uLogInOut;
    
	if(cmdActive4Search != NULL) {
        UserDeletePrcsdUsrCmd(cmdActive4Search);
		cmdActive4Search = NULL;
    }

	if(cmdActive6Search != NULL) {
        UserDeletePrcsdUsrCmd(cmdActive6Search);
		cmdActive6Search = NULL;
    }

	if(cmdPassiveSearch != NULL) {
        UserDeletePrcsdUsrCmd(cmdPassiveSearch);
		cmdPassiveSearch = NULL;
    }
                
	PrcsdUsrCmd *next = cmdStrt;
        
    while(next != NULL) {
        PrcsdUsrCmd *cur = next;
        next = cur->next;

#ifdef _WIN32
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->sCommand) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate cur->sCommand in User::~User\n", 0);
        }
#else
		free(cur->sCommand);
#endif
		cur->sCommand = NULL;

        delete cur;
	}

	cmdStrt = NULL;
	cmdEnd = NULL;

	PrcsdToUsrCmd *nextto = cmdToUserStrt;
                    
    while(nextto != NULL) {
        PrcsdToUsrCmd *curto = nextto;
        nextto = curto->next;

#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)curto->sCommand) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate curto->sCommand in User::~User\n", 0);
        }
#else
		free(curto->sCommand);
#endif
        curto->sCommand = NULL;

#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)curto->ToNick) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate curto->ToNick in User::~User\n", 0);
        }
#else
		free(curto->ToNick);
#endif
        curto->ToNick = NULL;

        delete curto;
	}
    

	cmdToUserStrt = NULL;
	cmdToUserEnd = NULL;
}
//---------------------------------------------------------------------------

bool UserMakeLock(User * u) {
    size_t szAllignLen = Allign1024(63);
#ifdef _WIN32
    u->sendbuf = (char *)HeapAlloc(hSendHeap, HEAP_NO_SERIALIZE, szAllignLen);
#else
	u->sendbuf = (char *)malloc(szAllignLen);
#endif
    if(u->sendbuf == NULL) {
        u->sbdatalen = 0;

		AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes in UserMakeLock\n", (uint64_t)szAllignLen);

        return false;
    }
    u->sendbuflen = (uint32_t)(szAllignLen-1);
	u->sbplayhead = u->sendbuf;

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
    memcpy(u->sendbuf, sLock, 63);
    u->sbdatalen += 63;
    u->sendbuf[u->sbdatalen] = '\0';

	for(uint8_t ui8i = 22; ui8i < 49; ui8i++) {
#ifdef _WIN32
        u->sendbuf[ui8i] = (char)((rand() % 74) + 48);
#else
        u->sendbuf[ui8i] = (char)((random() % 74) + 48);
#endif
	}

//	Memo(string(u->sendbuf, u->sbdatalen));

#ifdef _WIN32
    u->uLogInOut->sLockUsrConn = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, 64);
#else
	u->uLogInOut->sLockUsrConn = (char *)malloc(64);
#endif
    if(u->uLogInOut->sLockUsrConn == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate 64 bytes for sLockUsrConn in UserMakeLock\n", 0);
		return false;
    }
    
    memcpy(u->uLogInOut->sLockUsrConn, u->sendbuf, 63);
    u->uLogInOut->sLockUsrConn[63] = '\0';

    return true;
}
//---------------------------------------------------------------------------

bool UserDoRecv(User * u) {
    if((u->ui32BoolBits & User::BIT_ERROR) == User::BIT_ERROR || u->ui8State >= User::STATE_CLOSING)
        return false;

#ifdef _WIN32
	u_long iAvailBytes = 0;
	if(ioctlsocket(u->Sck, FIONREAD, &iAvailBytes) == SOCKET_ERROR) {
		int iError = WSAGetLastError();
#else
	int iAvailBytes = 0;
	if(ioctl(u->Sck, FIONREAD, &iAvailBytes) == -1) {
#endif
		UdpDebug->Broadcast("[ERR] " + string(u->sNick, u->ui8NickLen) + " (" + string(u->sIP, u->ui8IpLen) +
#ifdef _WIN32
			" ): ioctlsocket(FIONREAD) error " + string(WSErrorStr(iError)) + " (" + string(iError) + "). User is being closed.");
#else
			" ): ioctlsocket(FIONREAD) error " + string(ErrnoStr(errno)) + " (" + string(errno) + "). User is being closed.");
#endif
        u->ui32BoolBits |= User::BIT_ERROR;
		UserClose(u);
        return false;
    }

    // PPK ... check flood ...
	if(iAvailBytes != 0 && ProfileMan->IsAllowed(u, ProfileManager::NODEFLOODRECV) == false) {
        if(SettingManager->iShorts[SETSHORT_MAX_DOWN_ACTION] != 0) {
    		if(u->ui32Recvs == 0) {
    			u->ui64RecvsTick = ui64ActualTick;
            }

            u->ui32Recvs += iAvailBytes;

			if(DeFloodCheckForDataFlood(u, DEFLOOD_MAX_DOWN, SettingManager->iShorts[SETSHORT_MAX_DOWN_ACTION],
			  u->ui32Recvs, u->ui64RecvsTick, SettingManager->iShorts[SETSHORT_MAX_DOWN_KB],
              (uint32_t)SettingManager->iShorts[SETSHORT_MAX_DOWN_TIME]) == true) {
				return false;
            }

    		if(u->ui32Recvs != 0) {
                u->ui32Recvs -= iAvailBytes;
            }
        }

        if(SettingManager->iShorts[SETSHORT_MAX_DOWN_ACTION2] != 0) {
    		if(u->ui32Recvs2 == 0) {
    			u->ui64RecvsTick2 = ui64ActualTick;
            }

            u->ui32Recvs2 += iAvailBytes;

			if(DeFloodCheckForDataFlood(u, DEFLOOD_MAX_DOWN, SettingManager->iShorts[SETSHORT_MAX_DOWN_ACTION2],
			  u->ui32Recvs2, u->ui64RecvsTick2, SettingManager->iShorts[SETSHORT_MAX_DOWN_KB2],
			  (uint32_t)SettingManager->iShorts[SETSHORT_MAX_DOWN_TIME2]) == true) {
                return false;
            }

    		if(u->ui32Recvs2 != 0) {
                u->ui32Recvs2 -= iAvailBytes;
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

    if(u->recvbuflen < u->rbdatalen+iAvailBytes) {
        szAllignLen = Allign512(u->rbdatalen+iAvailBytes);
    } else if(u->iRecvCalled > 60) {
        szAllignLen = Allign512(u->rbdatalen+iAvailBytes);
        if(u->recvbuflen <= szAllignLen) {
            szAllignLen = 0;
        }

        u->iRecvCalled = 0;
    }

    if(szAllignLen != 0) {
        char * pOldBuf = u->recvbuf;

#ifdef _WIN32
        if(u->recvbuf == NULL) {
            u->recvbuf = (char *)HeapAlloc(hRecvHeap, HEAP_NO_SERIALIZE, szAllignLen);
        } else {
            u->recvbuf = (char *)HeapReAlloc(hRecvHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, szAllignLen);
        }
#else
        u->recvbuf = (char *)realloc(pOldBuf, szAllignLen);
#endif
		if(u->recvbuf == NULL) {
            u->recvbuf = pOldBuf;
            u->ui32BoolBits |= User::BIT_ERROR;
            UserClose(u);

			AppendDebugLog("%s - [MEM] Cannot (re)allocate %" PRIu64 " bytes for recvbuf in UserDoRecv\n", (uint64_t)szAllignLen);

			return false;
		}

		u->recvbuflen = (uint32_t)(szAllignLen-1);
	}
    
    // receive new data to recvbuf
	int recvlen = recv(u->Sck, u->recvbuf+u->rbdatalen, u->recvbuflen-u->rbdatalen, 0);
	u->iRecvCalled++;

#ifdef _WIN32
    if(recvlen == SOCKET_ERROR) {
		int iError = WSAGetLastError();
        if(iError != WSAEWOULDBLOCK) {
#else
    if(recvlen == -1) {
        if(errno != EAGAIN) {
#endif
			UdpDebug->Broadcast("[ERR] " + string(u->sNick, u->ui8NickLen) + " (" + string(u->sIP, u->ui8IpLen) + "): recv() error " + 
#ifdef _WIN32
                string(WSErrorStr(iError)) + ". User is being closed.");
#else
				string(ErrnoStr(errno)) + " (" + string(errno) + "). User is being closed.");
#endif
			u->ui32BoolBits |= User::BIT_ERROR;
            UserClose(u);
            return false;
        } else {
            return false;
        }
    } else if(recvlen == 0) { // regular close
#ifdef _WIN32
        int iError = WSAGetLastError();
	    if(iError != 0) {
			Memo("[SYS] recvlen == 0 and iError == "+string(iError)+"! User: "+string(u->sNick, u->ui8NickLen)+" ("+string(u->sIP, u->ui8IpLen)+")");
			UdpDebug->Broadcast("[SYS] recvlen == 0 and iError == " + string(iError) + " ! User: " + string(u->sNick, u->ui8NickLen) +
				" (" + string(u->sIP, u->ui8IpLen) + ").");
		}

	#ifdef _BUILD_GUI
        if(::SendMessage(pMainWindowPageUsersChat->hWndPageItems[MainWindowPageUsersChat::BTN_SHOW_COMMANDS], BM_GETCHECK, 0, 0) == BST_CHECKED) {
			int iret = sprintf(msg, "- User has closed the connection: %s (%s)", u->sNick, u->sIP);
			if(CheckSprintf(iret, 1024, "UserDoRecv") == true) {
				RichEditAppendText(pMainWindowPageUsersChat->hWndPageItems[MainWindowPageUsersChat::REDT_CHAT], msg);
			}
        }
    #endif
#endif

        u->ui32BoolBits |= User::BIT_ERROR;
        UserClose(u);
	    return false;
    }

    u->ui32Recvs += recvlen;
    u->ui32Recvs2 += recvlen;
	ui64BytesRead += recvlen;
	u->rbdatalen += recvlen;
	u->recvbuf[u->rbdatalen] = '\0';
    if(UserProcessLines(u, u->rbdatalen-recvlen) == true) {
        return true;
    }
        
    return false;
}
//---------------------------------------------------------------------------

void UserSendChar(User * u, const char * cText, const size_t &szTextLen) {
	if(u->ui8State >= User::STATE_CLOSING || szTextLen == 0)
        return;

    if(((u->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false || szTextLen < ZMINDATALEN) {
        if(UserPutInSendBuf(u, cText, szTextLen)) {
            UserTry2Send(u);
        }
    } else {
        uint32_t iLen = 0;
        char *sData = ZlibUtility->CreateZPipe(cText, szTextLen, iLen);
            
        if(iLen == 0) {
            if(UserPutInSendBuf(u, cText, szTextLen)) {
                UserTry2Send(u);
            }
        } else {
            ui64BytesSentSaved += szTextLen-iLen;
            if(UserPutInSendBuf(u, sData, iLen)) {
                UserTry2Send(u);
            }
        }
    }
}
//---------------------------------------------------------------------------

void UserSendCharDelayed(User * u, const char * cText) {
	if(u->ui8State >= User::STATE_CLOSING) {
        return;
    }
    
    size_t szTxtLen = strlen(cText);

	if(szTxtLen == 0) {
        return;
    }

    if(((u->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false || szTxtLen < ZMINDATALEN) {
        UserPutInSendBuf(u, cText, szTxtLen);
    } else {
        uint32_t iLen = 0;
        char *sData = ZlibUtility->CreateZPipe(cText, szTxtLen, iLen);
            
        if(iLen == 0) {
            UserPutInSendBuf(u, cText, szTxtLen);
        } else {
            UserPutInSendBuf(u, sData, iLen);
            ui64BytesSentSaved += szTxtLen-iLen;
        }
    }
}
//---------------------------------------------------------------------------

void UserSendCharDelayed(User * u, const char * cText, const size_t &szTextLen) {
	if(u->ui8State >= User::STATE_CLOSING || szTextLen == 0) {
        return;
    }
        
    if(((u->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false || szTextLen < ZMINDATALEN) {
        UserPutInSendBuf(u, cText, szTextLen);
    } else {
        uint32_t iLen = 0;
        char *sPipeData = ZlibUtility->CreateZPipe(cText, szTextLen, iLen);
        
        if(iLen == 0) {
            UserPutInSendBuf(u, cText, szTextLen);
        } else {
            UserPutInSendBuf(u, sPipeData, iLen);
            ui64BytesSentSaved += szTextLen-iLen;
        }
    }
}
//---------------------------------------------------------------------------

void UserSendText(User * u, const string &sText) {
	if(u->ui8State >= User::STATE_CLOSING || sText.size() == 0) {
        return;
    }

    if(((u->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false || sText.size() < ZMINDATALEN) {
        if(UserPutInSendBuf(u, sText.c_str(), sText.size())) {
            UserTry2Send(u);
        }
    } else {
        uint32_t iLen = 0;
        char *sData = ZlibUtility->CreateZPipe(sText.c_str(), sText.size(), iLen);
            
        if(iLen == 0) {
            if(UserPutInSendBuf(u, sText.c_str(), sText.size())) {
                UserTry2Send(u);
            }
        } else {
            ui64BytesSentSaved += sText.size()-iLen;
            if(UserPutInSendBuf(u, sData, iLen)) {
                UserTry2Send(u);
            }
        }
    }
}
//---------------------------------------------------------------------------

void UserSendTextDelayed(User * u, const string &sText) {
	if(u->ui8State >= User::STATE_CLOSING || sText.size() == 0) {
        return;
    }
      
    if(((u->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false || sText.size() < ZMINDATALEN) {
        UserPutInSendBuf(u, sText.c_str(), sText.size());
    } else {
        uint32_t iLen = 0;
        char *sData = ZlibUtility->CreateZPipe(sText.c_str(), sText.size(), iLen);
            
        if(iLen == 0) {
            UserPutInSendBuf(u, sText.c_str(), sText.size());
        } else {
            UserPutInSendBuf(u, sData, iLen);
            ui64BytesSentSaved += sText.size()-iLen;
        }
    }
}
//---------------------------------------------------------------------------

bool UserPutInSendBuf(User * u, const char * Text, const size_t &szTxtLen) {
	u->iSendCalled++;

    size_t szAllignLen = 0;

    if(u->sendbuflen < u->sbdatalen+szTxtLen) {
        if(u->sendbuf == NULL) {
            szAllignLen = Allign1024(u->sbdatalen+szTxtLen);
        } else {
            if((size_t)(u->sbplayhead-u->sendbuf) > szTxtLen) {
                uint32_t offset = (uint32_t)(u->sbplayhead-u->sendbuf);
                memmove(u->sendbuf, u->sbplayhead, (u->sbdatalen-offset));
                u->sbplayhead = u->sendbuf;
                u->sbdatalen = u->sbdatalen-offset;
            } else {
                szAllignLen = Allign1024(u->sbdatalen+szTxtLen);
                size_t szMaxBufLen = (size_t)(((u->ui32BoolBits & User::BIT_BIG_SEND_BUFFER) == User::BIT_BIG_SEND_BUFFER) == true ?
                    ((colUsers->myInfosTagLen > colUsers->myInfosLen ? colUsers->myInfosTagLen : colUsers->myInfosLen)*2) :
                    (colUsers->myInfosTagLen > colUsers->myInfosLen ? colUsers->myInfosTagLen : colUsers->myInfosLen));
                szMaxBufLen = szMaxBufLen < 262144 ? 262144 :szMaxBufLen;
                if(szAllignLen > szMaxBufLen) {
                    // does the buffer size reached the maximum
                    if(SettingManager->bBools[SETBOOL_KEEP_SLOW_USERS] == false || (u->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) {
                        // we want to drop the slow user
                        u->ui32BoolBits |= User::BIT_ERROR;
                        UserClose(u);

                        UdpDebug->Broadcast("[SYS] " + string(u->sNick, u->ui8NickLen) + " (" + string(u->sIP, u->ui8IpLen) +") SendBuffer overflow (AL:"+string((uint64_t)szAllignLen)+
                            "[SL:"+string(u->sbdatalen)+"|NL:"+string((uint64_t)szTxtLen)+"|FL:"+string((uint64_t)(u->sbplayhead-u->sendbuf))+"]/ML:"+string((uint64_t)szMaxBufLen)+"). User disconnected.");
                        return false;
                    } else {
    				    UdpDebug->Broadcast("[SYS] " + string(u->sNick, u->ui8NickLen) + " (" + string(u->sIP, u->ui8IpLen) +") SendBuffer overflow (AL:"+string((uint64_t)szAllignLen)+
                            "[SL:"+string(u->sbdatalen)+"|NL:"+string((uint64_t)szTxtLen)+"|FL:"+string((uint64_t)(u->sbplayhead-u->sendbuf))+"]/ML:"+string((uint64_t)szMaxBufLen)+
                            "). Buffer cleared - user stays online.");
                    }

                    // we want to keep the slow user online
                    // PPK ... i don't want to corrupt last command, get rest of it and add to new buffer ;)
                    char *sTemp = (char *)memchr(u->sbplayhead, '|', u->sbdatalen-(u->sbplayhead-u->sendbuf));
                    if(sTemp != NULL) {
                        uint32_t iOldSBDataLen = u->sbdatalen;

                        uint32_t iRestCommandLen = (uint32_t)((sTemp-u->sbplayhead)+1);
                        if(u->sendbuf != u->sbplayhead) {
                            memmove(u->sendbuf, u->sbplayhead, iRestCommandLen);
                        }
                        u->sbdatalen = iRestCommandLen;

                        // If is not needed then don't lost all data, try to find some space with removing only few oldest commands
                        if(szTxtLen < szMaxBufLen && iOldSBDataLen > (uint32_t)((sTemp+1)-u->sendbuf) && (iOldSBDataLen-((sTemp+1)-u->sendbuf)) > (uint32_t)szTxtLen) {
                            char *sTemp1;
                            // try to remove min half of send bufer
                            if(iOldSBDataLen > (u->sendbuflen/2) && (uint32_t)((sTemp+1+szTxtLen)-u->sendbuf) < (u->sendbuflen/2)) {
                                sTemp1 = (char *)memchr(u->sendbuf+(u->sendbuflen/2), '|', iOldSBDataLen-(u->sendbuflen/2));
                            } else {
                                sTemp1 = (char *)memchr(sTemp+1+szTxtLen, '|', iOldSBDataLen-((sTemp+1+szTxtLen)-u->sendbuf));
                            }

                            if(sTemp1 != NULL) {
                                iRestCommandLen = (uint32_t)(iOldSBDataLen-((sTemp1+1)-u->sendbuf));
                                memmove(u->sendbuf+u->sbdatalen, sTemp1+1, iRestCommandLen);
                                u->sbdatalen += iRestCommandLen;
                            }
                        }
                    } else {
                        u->sendbuf[0] = '|';
                        u->sendbuf[1] = '\0';
                        u->sbdatalen = 1;
                    }

                    size_t szAllignTxtLen = Allign1024(szTxtLen+u->sbdatalen);

                    char * pOldBuf = u->sendbuf;
#ifdef _WIN32
                    u->sendbuf = (char *)HeapReAlloc(hSendHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, szAllignTxtLen);
#else
				    u->sendbuf = (char *)realloc(pOldBuf, szAllignTxtLen);
#endif
                    if(u->sendbuf == NULL) {
                        u->sendbuf = pOldBuf;
                        u->ui32BoolBits |= User::BIT_ERROR;
                        UserClose(u);

                        AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in UserPutInSendBuf-keepslow\n", (uint64_t)szAllignLen);

                        return false;
                    }
                    u->sendbuflen = (uint32_t)(szAllignTxtLen-1);
                    u->sbplayhead = u->sendbuf;

                    szAllignLen = 0;
                } else {
                    szAllignLen = Allign1024(u->sbdatalen+szTxtLen);
                }
        	}
        }
    } else if(u->iSendCalled > 100) {
        szAllignLen = Allign1024(u->sbdatalen+szTxtLen);
        if(u->sendbuflen <= szAllignLen) {
            szAllignLen = 0;
        }

        u->iSendCalled = 0;
    }

    if(szAllignLen != 0) {
        uint32_t offset = (u->sendbuf == NULL ? 0 : (uint32_t)(u->sbplayhead-u->sendbuf));

        char * pOldBuf = u->sendbuf;
#ifdef _WIN32
        if(u->sendbuf == NULL) {
            u->sendbuf = (char *)HeapAlloc(hSendHeap, HEAP_NO_SERIALIZE, szAllignLen);
        } else {
            u->sendbuf = (char *)HeapReAlloc(hSendHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, szAllignLen);
        }
#else
		u->sendbuf = (char *)realloc(pOldBuf, szAllignLen);
#endif
        if(u->sendbuf == NULL) {
            u->sendbuf = pOldBuf;
            u->ui32BoolBits |= User::BIT_ERROR;
            UserClose(u);

			AppendDebugLog("%s - [MEM] Cannot (re)allocate %" PRIu64 " bytes for new sendbuf in UserPutInSendBuf\n", (uint64_t)szAllignLen);

        	return false;
        }

        u->sendbuflen = (uint32_t)(szAllignLen-1);
        u->sbplayhead = u->sendbuf+offset;
    }

    // append data to the buffer
    memcpy(u->sendbuf+u->sbdatalen, Text, szTxtLen);
    u->sbdatalen += (uint32_t)szTxtLen;
    u->sendbuf[u->sbdatalen] = '\0';

    return true;
}
//---------------------------------------------------------------------------

bool UserTry2Send(User * u) {
    if((u->ui32BoolBits & User::BIT_ERROR) == User::BIT_ERROR || u->sbdatalen == 0) {
        return false;
    }

    // compute length of unsent data
    int32_t offset = (int32_t)(u->sbplayhead - u->sendbuf);
	int32_t len = u->sbdatalen - offset;

	if(offset < 0 || len < 0) {
		int imsgLen = sprintf(msg, "%s - [ERR] Negative send values!\nSendBuf: %p\nPlayHead: %p\nDataLen: %u\n", "%s", u->sendbuf, u->sbplayhead, u->sbdatalen);
        if(CheckSprintf(imsgLen, 1024, "UserTry2Send") == true) {
    		AppendDebugLog(msg, 0);
        }

        u->ui32BoolBits |= User::BIT_ERROR;
        UserClose(u);

        return false;
    }

    int n = send(u->Sck, u->sbplayhead, len < 32768 ? len : 32768, 0);

#ifdef _WIN32
    if(n == SOCKET_ERROR) {
    	int iError = WSAGetLastError();
        if(iError != WSAEWOULDBLOCK) {
#else
	if(n == -1) {
        if(errno != EAGAIN) {
#endif
			UdpDebug->Broadcast("[ERR] " + string(u->sNick, u->ui8NickLen) + " (" + string(u->sIP, u->ui8IpLen) +
#ifdef _WIN32
				"): send() error " + string(WSErrorStr(iError)) + ". User is being closed.");
#else
				"): send() error " + string(ErrnoStr(errno)) + " (" + string(errno) + "). User is being closed.");
#endif
			u->ui32BoolBits |= User::BIT_ERROR;
            UserClose(u);
            return false;
        } else {
            return true;
        }
    }

	ui64BytesSent += n;

	// if buffer is sent then mark it as empty (first byte = 0)
	// else move remaining data on new place and free old buffer
	if(n < len) {
        u->sbplayhead += n;
		return true;
	} else {
        // PPK ... we need to free memory allocated for big buffer on login (userlist, motd...)
        if(((u->ui32BoolBits & User::BIT_BIG_SEND_BUFFER) == User::BIT_BIG_SEND_BUFFER) == true) {
            if(u->sendbuf != NULL) {
#ifdef _WIN32
               if(HeapFree(hSendHeap, HEAP_NO_SERIALIZE, (void *)u->sendbuf) == 0) {
					AppendDebugLog("%s - [MEM] Cannot deallocate u->sendbuf in UserTry2Send\n", 0);
                }
#else
				free(u->sendbuf);
#endif
                u->sendbuf = NULL;
                u->sbplayhead = u->sendbuf;
                u->sendbuflen = 0;
                u->sbdatalen = 0;
            }
            u->ui32BoolBits &= ~User::BIT_BIG_SEND_BUFFER;
        } else {
    		u->sendbuf[0] = '\0';
            u->sbplayhead = u->sendbuf;
            u->sbdatalen = 0;
        }
		return false;
	}
}
//---------------------------------------------------------------------------

void UserSetIP(User * u, char * sNewIP) {
    strcpy(u->sIP, sNewIP);
    u->ui8IpLen = (uint8_t)strlen(u->sIP);
}
//------------------------------------------------------------------------------

void UserSetNick(User * u, char * sNewNick, const uint8_t &ui8NewNickLen) {
	if(u->sNick != sDefaultNick && u->sNick != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)u->sNick) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate u->sNick in UserSetNick\n", 0);
        }
#else
		free(u->sNick);
#endif
        u->sNick = NULL;
    }

#ifdef _WIN32
    u->sNick = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, ui8NewNickLen+1);
#else
	u->sNick = (char *)malloc(ui8NewNickLen+1);
#endif
    if(u->sNick == NULL) {
        u->sNick = (char *)sDefaultNick;
        u->ui32BoolBits |= User::BIT_ERROR;
        UserClose(u);

		AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for Nick in UserSetNick\n", (uint64_t)(ui8NewNickLen+1));

        return;
    }   
    memcpy(u->sNick, sNewNick, ui8NewNickLen);
    u->sNick[ui8NewNickLen] = '\0';
    u->ui8NickLen = ui8NewNickLen;
    u->ui32NickHash = HashNick(u->sNick, u->ui8NickLen);
}
//------------------------------------------------------------------------------

void UserSetMyInfoOriginal(User * u, char * sNewMyInfo, const uint16_t &ui16NewMyInfoLen) {
    char * sOldMyInfo = u->sMyInfoOriginal;

    char * sOldDescription = u->sDescription;
    uint8_t ui8OldDescriptionLen = u->ui8DescriptionLen;

    char * sOldTag = u->sTag;
    uint8_t ui8OldTagLen = u->ui8TagLen;

    char * sOldConnection = u->sConnection;
    uint8_t ui8OldConnectionLen = u->ui8ConnectionLen;

    char * sOldEmail = u->sEmail;
    uint8_t ui8OldEmailLen = u->ui8EmailLen;

    uint64_t ui64OldShareSize = u->ui64SharedSize;

	if(u->sMyInfoOriginal != NULL) {
        u->sConnection = NULL;
        u->ui8ConnectionLen = 0;

        u->sDescription = NULL;
        u->ui8DescriptionLen = 0;

        u->sEmail = NULL;
        u->ui8EmailLen = 0;

        u->sTag = NULL;
        u->ui8TagLen = 0;

        u->sClient = NULL;
        u->ui8ClientLen = 0;

        u->sTagVersion = NULL;
        u->ui8TagVersionLen = 0;

        u->sMyInfoOriginal = NULL;
    }

#ifdef _WIN32
    u->sMyInfoOriginal = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, ui16NewMyInfoLen+1);
#else
	u->sMyInfoOriginal = (char *)malloc(ui16NewMyInfoLen+1);
#endif
    if(u->sMyInfoOriginal == NULL) {
        u->ui32BoolBits |= User::BIT_ERROR;
        UserClose(u);

		AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sMyInfoOriginal in UserSetMyInfoOriginal\n", (uint64_t)(ui16NewMyInfoLen+1));

        return;
    }
    memcpy(u->sMyInfoOriginal, sNewMyInfo, ui16NewMyInfoLen);
    u->sMyInfoOriginal[ui16NewMyInfoLen] = '\0';
    u->ui16MyInfoOriginalLen = ui16NewMyInfoLen;

    UserParseMyInfo(u);

    if(ui8OldDescriptionLen != u->ui8DescriptionLen || (u->ui8DescriptionLen > 0 && memcmp(sOldDescription, u->sDescription, u->ui8DescriptionLen) != 0)) {
        u->ui32InfoBits |= User::INFOBIT_DESCRIPTION_CHANGED;
    } else {
        u->ui32InfoBits &= ~User::INFOBIT_DESCRIPTION_CHANGED;
    }

    if(ui8OldTagLen != u->ui8TagLen || (u->ui8TagLen > 0 && memcmp(sOldTag, u->sTag, u->ui8TagLen) != 0)) {
        u->ui32InfoBits |= User::INFOBIT_TAG_CHANGED;
    } else {
        u->ui32InfoBits &= ~User::INFOBIT_TAG_CHANGED;
    }

    if(ui8OldConnectionLen != u->ui8ConnectionLen || (u->ui8ConnectionLen > 0 && memcmp(sOldConnection, u->sConnection, u->ui8ConnectionLen) != 0)) {
        u->ui32InfoBits |= User::INFOBIT_CONNECTION_CHANGED;
    } else {
        u->ui32InfoBits &= ~User::INFOBIT_CONNECTION_CHANGED;
    }

    if(ui8OldEmailLen != u->ui8EmailLen || (u->ui8EmailLen > 0 && memcmp(sOldEmail, u->sEmail, u->ui8EmailLen) != 0)) {
        u->ui32InfoBits |= User::INFOBIT_EMAIL_CHANGED;
    } else {
        u->ui32InfoBits &= ~User::INFOBIT_EMAIL_CHANGED;
    }

    if(ui64OldShareSize != u->ui64SharedSize) {
        u->ui32InfoBits |= User::INFOBIT_SHARE_CHANGED;
    } else {
        u->ui32InfoBits &= ~User::INFOBIT_SHARE_CHANGED;
    }

    if(sOldMyInfo != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldMyInfo) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate sOldMyInfo in UserSetMyInfoOriginal\n", 0);
        }
#else
        free(sOldMyInfo);
#endif
    }

    if(((u->ui32InfoBits & User::INFOBIT_SHARE_SHORT_PERM) == User::INFOBIT_SHARE_SHORT_PERM) == false) {
        u->ui64ChangedSharedSizeShort = u->ui64SharedSize;
    }

    if(((u->ui32InfoBits & User::INFOBIT_SHARE_LONG_PERM) == User::INFOBIT_SHARE_LONG_PERM) == false) {
        u->ui64ChangedSharedSizeLong = u->ui64SharedSize;
    }

}
//------------------------------------------------------------------------------

static void UserSetMyInfoLong(User * u, char * sNewMyInfoLong, const uint16_t &ui16NewMyInfoLongLen) {
	if(u->sMyInfoLong != NULL) {
        if(SettingManager->ui8FullMyINFOOption != 2) {
    	    colUsers->DelFromMyInfosTag(u);
        }

#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)u->sMyInfoLong) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate u->sMyInfoLong in UserSetMyInfoLong\n", 0);
        }
#else
        free(u->sMyInfoLong);
#endif
        u->sMyInfoLong = NULL;
    }

#ifdef _WIN32
    u->sMyInfoLong = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, ui16NewMyInfoLongLen+1);
#else
	u->sMyInfoLong = (char *)malloc(ui16NewMyInfoLongLen+1);
#endif
    if(u->sMyInfoLong == NULL) {
        u->ui32BoolBits |= User::BIT_ERROR;
        UserClose(u);

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
        if(SettingManager->ui8FullMyINFOOption != 0) {
    	    colUsers->DelFromMyInfos(u);
        }

#ifdef _WIN32    	    
    	if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)u->sMyInfoShort) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate u->sMyInfoShort in UserSetMyInfoShort\n", 0);
        }
#else
		free(u->sMyInfoShort);
#endif
        u->sMyInfoShort = NULL;
    }

#ifdef _WIN32
    u->sMyInfoShort = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, ui16NewMyInfoShortLen+1);
#else
	u->sMyInfoShort = (char *)malloc(ui16NewMyInfoShortLen+1);
#endif
    if(u->sMyInfoShort == NULL) {
        u->ui32BoolBits |= User::BIT_ERROR;
        UserClose(u);

		AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for MyInfoShort in UserSetMyInfoShort\n", (uint64_t)(ui16NewMyInfoShortLen+1));

        return;
    }   
    memcpy(u->sMyInfoShort, sNewMyInfoShort, ui16NewMyInfoShortLen);
    u->sMyInfoShort[ui16NewMyInfoShortLen] = '\0';
    u->ui16MyInfoShortLen = ui16NewMyInfoShortLen;
}
//------------------------------------------------------------------------------

void UserSetVersion(User * u, char * sNewVer) {
#ifdef _WIN32
	if(u->sVersion) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)u->sVersion) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate u->sVersion in UserSetVersion\n", 0);
        }
    }
#else
	free(u->sVersion);
#endif

    size_t szLen = strlen(sNewVer);
#ifdef _WIN32
    u->sVersion = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szLen+1);
#else
	u->sVersion = (char *)malloc(szLen+1);
#endif
    if(u->sVersion == NULL) {
        u->ui32BoolBits |= User::BIT_ERROR;
        UserClose(u);

		AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for Version in UserSetVersion\n", (uint64_t)(szLen+1));

        return;
    }   
    memcpy(u->sVersion, sNewVer, szLen);
    u->sVersion[szLen] = '\0';
}
//------------------------------------------------------------------------------

void UserSetPasswd(User * u, char * sNewPass) {
	if(u->uLogInOut->sPassword != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)u->uLogInOut->sPassword) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate u->uLogInOut->Password in UserSetPasswd\n", 0);
        }
#else
		free(u->uLogInOut->sPassword);
#endif
        u->uLogInOut->sPassword = NULL;
    }
        
    size_t szLen = strlen(sNewPass);
#ifdef _WIN32
    u->uLogInOut->sPassword = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szLen+1);
#else
	u->uLogInOut->sPassword = (char *)malloc(szLen+1);
#endif
    if(u->uLogInOut->sPassword == NULL) {
        u->ui32BoolBits |= User::BIT_ERROR;
        UserClose(u);

		AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for Password in UserSetPasswd\n", (uint64_t)(szLen+1));

        return;
    }   
    memcpy(u->uLogInOut->sPassword, sNewPass, szLen);
    u->uLogInOut->sPassword[szLen] = '\0';
}
//------------------------------------------------------------------------------

void UserSetLastChat(User * u, char * sNewData, const size_t &szLen) {
#ifdef _WIN32
    if(u->sLastChat != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)u->sLastChat) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate u->sLastChat in UserSetLastChat\n", 0);
        }
    }
#else
	free(u->sLastChat);
#endif

#ifdef _WIN32
    u->sLastChat = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szLen+1);
#else
	u->sLastChat = (char *)malloc(szLen+1);
#endif
    if(u->sLastChat == NULL) {
        u->ui32BoolBits |= User::BIT_ERROR;
        UserClose(u);

		AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sLastChat in UserSetLastChat\n", (uint64_t)(szLen+1));

        return;
    }   
    memcpy(u->sLastChat, sNewData, szLen);
    u->sLastChat[szLen] = '\0';
    u->ui16SameChatMsgs = 1;
    u->ui64SameChatsTick = ui64ActualTick;
    u->ui16LastChatLen = (uint16_t)szLen;
    u->ui16SameMultiChats = 0;
    u->ui16LastChatLines = 0;
}
//------------------------------------------------------------------------------

void UserSetLastPM(User * u, char * sNewData, const size_t &szLen) {
#ifdef _WIN32
    if(u->sLastPM != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)u->sLastPM) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate u->sLastPM in UserSetLastPM\n", 0);
        }
    }
#else
	free(u->sLastPM);
#endif

#ifdef _WIN32
    u->sLastPM = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szLen+1);
#else
	u->sLastPM = (char *)malloc(szLen+1);
#endif
    if(u->sLastPM == NULL) {
        u->ui32BoolBits |= User::BIT_ERROR;
        UserClose(u);

		AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sLastPM in UserSetLastPM\n", (uint64_t)(szLen+1));

        return;
    }

    memcpy(u->sLastPM, sNewData, szLen);
    u->sLastPM[szLen] = '\0';
    u->ui16SamePMs = 1;
    u->ui64SamePMsTick = ui64ActualTick;
    u->ui16LastPMLen = (uint16_t)szLen;
    u->ui16SameMultiPms = 0;
    u->ui16LastPmLines = 0;
}
//------------------------------------------------------------------------------

void UserSetLastSearch(User * u, char * sNewData, const size_t &szLen) {
#ifdef _WIN32
    if(u->sLastSearch != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)u->sLastSearch) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate u->sLastSearch in UserSetLastSearch\n", 0);
        }
    }
#else
	free(u->sLastSearch);
#endif

#ifdef _WIN32
    u->sLastSearch = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szLen+1);
#else
	u->sLastSearch = (char *)malloc(szLen+1);
#endif
    if(u->sLastSearch == NULL) {
        u->ui32BoolBits |= User::BIT_ERROR;
        UserClose(u);

        AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sLastSearch in UserSetLastSearch\n", (uint64_t)(szLen+1));

        return;
    }   
    memcpy(u->sLastSearch, sNewData, szLen);
    u->sLastSearch[szLen] = '\0';
    u->ui16SameSearchs = 1;
    u->ui64SameSearchsTick = ui64ActualTick;
    u->ui16LastSearchLen = (uint16_t)szLen;
}
//------------------------------------------------------------------------------

void UserSetKickMsg(User * u, char * sKickMsg, size_t szLen/* = 0*/) {
    if(szLen == 0) {
        szLen = strlen(sKickMsg);
    }

    if(u->uLogInOut != NULL) {
        if(u->uLogInOut->sKickMsg != NULL) {
#ifdef _WIN32
            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)u->uLogInOut->sKickMsg) == 0) {
				AppendDebugLog("%s - [MEM] Cannot deallocate u->uLogInOut->sKickMsg in UserSetKickMsg\n", 0);
            }
#else
			free(u->uLogInOut->sKickMsg);
#endif
            u->uLogInOut->sKickMsg = NULL;
        }
    } else {
        u->uLogInOut = new LoginLogout();
        if(u->uLogInOut == NULL) {
    		u->ui32BoolBits |= User::BIT_ERROR;
    		UserClose(u);

    		AppendDebugLog("%s - [MEM] Cannot allocate new u->uLogInOut in UserSetKickMsg\n", 0);
    		return;
        }
    }

    if(szLen < 256) {
#ifdef _WIN32
        u->uLogInOut->sKickMsg = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szLen+1);
#else
		u->uLogInOut->sKickMsg = (char *)malloc(szLen+1);
#endif
        if(u->uLogInOut->sKickMsg == NULL) {
            u->ui32BoolBits |= User::BIT_ERROR;
            UserClose(u);

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sKickMsg in UserSetKickMsg\n", (uint64_t)(szLen+1));

            return;
        }
        memcpy(u->uLogInOut->sKickMsg, sKickMsg, szLen);
        u->uLogInOut->sKickMsg[szLen] = '\0';
    } else {
#ifdef _WIN32
        u->uLogInOut->sKickMsg = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, 256);
#else
		u->uLogInOut->sKickMsg = (char *)malloc(256);
#endif
        if(u->uLogInOut->sKickMsg == NULL) {
            u->ui32BoolBits |= User::BIT_ERROR;
            UserClose(u);

			AppendDebugLog("%s - [MEM] Cannot allocate 256 bytes for sKickMsg in UserSetKickMsg1\n", 0);

            return;
        }
        memcpy(u->uLogInOut->sKickMsg, sKickMsg, 252);
        u->uLogInOut->sKickMsg[255] = '\0';
        u->uLogInOut->sKickMsg[254] = '.';
        u->uLogInOut->sKickMsg[253] = '.';
        u->uLogInOut->sKickMsg[252] = '.';
    }
}
//------------------------------------------------------------------------------

void UserClose(User * u, bool bNoQuit/* = false*/) {
    if(u->ui8State >= User::STATE_CLOSING) {
        return;
    }
    
	// nick in hash table ?
	if((u->ui32BoolBits & User::BIT_HASHED) == User::BIT_HASHED) {
    	hashManager->Remove(u);
    }

    // nick in nick/op list ?
    if(u->ui8State >= User::STATE_ADDME_2LOOP) {  
		colUsers->DelFromNickList(u->sNick, (u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR);
		colUsers->DelFromUserIP(u);

        // PPK ... fix for QuickList nad ghost...
        // and fixing redirect all too ;)
        // and fix disconnect on send error too =)
        if(bNoQuit == false) {         
            int imsgLen = sprintf(msg, "$Quit %s|", u->sNick); 
            if(CheckSprintf(imsgLen, 1024, "UserClose") == true) {
                g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_QUIT);
            }

			colUsers->Add2RecTimes(u);
        }

#ifdef _BUILD_GUI
        if(::SendMessage(pMainWindowPageUsersChat->hWndPageItems[MainWindowPageUsersChat::BTN_AUTO_UPDATE_USERLIST], BM_GETCHECK, 0, 0) == BST_CHECKED) {
            pMainWindowPageUsersChat->RemoveUser(u);
        }
#endif

        //sqldb->FinalizeVisit(u);

		if(((u->ui32BoolBits & User::BIT_HAVE_SHARECOUNTED) == User::BIT_HAVE_SHARECOUNTED) == true) {
            ui64TotalShare -= u->ui64SharedSize;
            u->ui32BoolBits &= ~User::BIT_HAVE_SHARECOUNTED;
		}

		ScriptManager->UserDisconnected(u);
	}

    if(u->ui8State > User::STATE_ADDME_2LOOP) {
        ui32Logged--;
    }

	u->ui8State = User::STATE_CLOSING;
	
    if(u->cmdActive4Search != NULL) {
        UserDeletePrcsdUsrCmd(u->cmdActive4Search);
        u->cmdActive4Search = NULL;
    }

    if(u->cmdActive6Search != NULL) {
        UserDeletePrcsdUsrCmd(u->cmdActive6Search);
        u->cmdActive6Search = NULL;
    }

    if(u->cmdPassiveSearch != NULL) {
        UserDeletePrcsdUsrCmd(u->cmdPassiveSearch);
        u->cmdPassiveSearch = NULL;
    }
                        
    PrcsdUsrCmd *next = u->cmdStrt;
                        
    while(next != NULL) {
        PrcsdUsrCmd *cur = next;
        next = cur->next;

#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->sCommand) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate cur->sCommand in UserClose\n", 0);
        }
#else
		free(cur->sCommand);
#endif
        cur->sCommand = NULL;

        delete cur;
	}
    
    u->cmdStrt = NULL;
    u->cmdEnd = NULL;
    
    PrcsdToUsrCmd *nextto = u->cmdToUserStrt;
                        
    while(nextto != NULL) {
        PrcsdToUsrCmd *curto = nextto;
        nextto = curto->next;

#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)curto->sCommand) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate curto->sCommand in UserClose\n", 0);
        }
#else
		free(curto->sCommand);
#endif
        curto->sCommand = NULL;

#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)curto->ToNick) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate curto->ToNick in UserClose\n", 0);
        }
#else
		free(curto->ToNick);
#endif
        curto->ToNick = NULL;

        delete curto;
	}


    u->cmdToUserStrt = NULL;
    u->cmdToUserEnd = NULL;

    if(u->sMyInfoLong) {
    	if(SettingManager->ui8FullMyINFOOption != 2) {
    		colUsers->DelFromMyInfosTag(u);
        }
    }
    
    if(u->sMyInfoShort) {
    	if(SettingManager->ui8FullMyINFOOption != 0) {
    		colUsers->DelFromMyInfos(u);
        }
    }

    if(u->sbdatalen == 0 || (u->ui32BoolBits & User::BIT_ERROR) == User::BIT_ERROR) {
        u->ui8State = User::STATE_REMME;
    } else {
        if(u->uLogInOut == NULL) {
            u->uLogInOut = new LoginLogout();
            if(u->uLogInOut == NULL) {
                u->ui8State = User::STATE_REMME;
        		AppendDebugLog("%s - [MEM] Cannot allocate new u->uLogInOut in UserClose\n", 0);
        		return;
            }
        }

        u->uLogInOut->iToCloseLoops = 100;
    }
}
//---------------------------------------------------------------------------

void UserAdd2Userlist(User * u) {
    colUsers->Add2NickList(u);
    colUsers->Add2UserIP(u);
    
    switch(SettingManager->ui8FullMyINFOOption) {
        case 0: {
            UserGenerateMyInfoLong(u);
            colUsers->Add2MyInfosTag(u);
            return;
        }
        case 1: {
            UserGenerateMyInfoLong(u);
            colUsers->Add2MyInfosTag(u);
            UserGenerateMyInfoShort(u);
            colUsers->Add2MyInfos(u);
            return;
        }
        case 2: {
            UserGenerateMyInfoShort(u);
            colUsers->Add2MyInfos(u);
            return;
        }
        default:
            break;
    }
}
//------------------------------------------------------------------------------

void UserAddUserList(User * u) {
    u->ui32BoolBits |= User::BIT_BIG_SEND_BUFFER;
	u->iLastNicklist = ui64ActualTick;

	if(((u->ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == false) {
    	if(ProfileMan->IsAllowed(u, ProfileManager::ALLOWEDOPCHAT) == false || (SettingManager->bBools[SETBOOL_REG_OP_CHAT] == false ||
            (SettingManager->bBools[SETBOOL_REG_BOT] == true && SettingManager->bBotsSameNick == true))) {
            if(((u->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false) {
                UserSendCharDelayed(u, colUsers->nickList, colUsers->nickListLen);
            } else {
                if(colUsers->iZNickListLen == 0) {
                    colUsers->sZNickList = ZlibUtility->CreateZPipe(colUsers->nickList, colUsers->nickListLen, colUsers->sZNickList,
                        colUsers->iZNickListLen, colUsers->iZNickListSize, Allign16K);
                    if(colUsers->iZNickListLen == 0) {
                        UserSendCharDelayed(u, colUsers->nickList, colUsers->nickListLen);
                    } else {
                        UserPutInSendBuf(u, colUsers->sZNickList, colUsers->iZNickListLen);
                        ui64BytesSentSaved += colUsers->nickListLen-colUsers->iZNickListLen;
                    }
                } else {
                    UserPutInSendBuf(u, colUsers->sZNickList, colUsers->iZNickListLen);
                    ui64BytesSentSaved += colUsers->nickListLen-colUsers->iZNickListLen;
                }
            }
        } else {
            // PPK ... OpChat bot is now visible only for OPs ;)
            int iLen = sprintf(msg, "%s$$|", SettingManager->sTexts[SETTXT_OP_CHAT_NICK]);
            if(CheckSprintf(iLen, 1024, "UserAddUserList") == true) {
                if(colUsers->nickListSize < colUsers->nickListLen+iLen) {
                    char * pOldBuf = colUsers->nickList;
#ifdef _WIN32
                    colUsers->nickList = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, colUsers->nickListSize+NICKLISTSIZE+1);
#else
					colUsers->nickList = (char *)realloc(pOldBuf, colUsers->nickListSize+NICKLISTSIZE+1);
#endif
                    if(colUsers->nickList == NULL) {
                        colUsers->nickList = pOldBuf;
                        u->ui32BoolBits |= User::BIT_ERROR;
                        UserClose(u);

						AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes for nickList in UserAddUserList\n", (uint64_t)(colUsers->nickListSize+NICKLISTSIZE+1));

                        return;
                    }
                    colUsers->nickListSize += NICKLISTSIZE;
                }
    
                memcpy(colUsers->nickList+colUsers->nickListLen-1, msg, iLen);
                colUsers->nickList[colUsers->nickListLen+(iLen-1)] = '\0';
                UserSendCharDelayed(u, colUsers->nickList, colUsers->nickListLen+(iLen-1));
                colUsers->nickList[colUsers->nickListLen-1] = '|';
                colUsers->nickList[colUsers->nickListLen] = '\0';
            }
        }
	}
	
	switch(SettingManager->ui8FullMyINFOOption) {
    	case 0: {
            if(colUsers->myInfosTagLen == 0) {
                break;
            }

            if(((u->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false) {
                UserSendCharDelayed(u, colUsers->myInfosTag, colUsers->myInfosTagLen);
            } else {
                if(colUsers->iZMyInfosTagLen == 0) {
                    colUsers->sZMyInfosTag = ZlibUtility->CreateZPipe(colUsers->myInfosTag, colUsers->myInfosTagLen, colUsers->sZMyInfosTag,
                        colUsers->iZMyInfosTagLen, colUsers->iZMyInfosTagSize, Allign128K);
                    if(colUsers->iZMyInfosTagLen == 0) {
                        UserSendCharDelayed(u, colUsers->myInfosTag, colUsers->myInfosTagLen);
                    } else {
                        UserPutInSendBuf(u, colUsers->sZMyInfosTag, colUsers->iZMyInfosTagLen);
                        ui64BytesSentSaved += colUsers->myInfosTagLen-colUsers->iZMyInfosTagLen;
                    }
                } else {
                    UserPutInSendBuf(u, colUsers->sZMyInfosTag, colUsers->iZMyInfosTagLen);
                    ui64BytesSentSaved += colUsers->myInfosTagLen-colUsers->iZMyInfosTagLen;
                }
            }
            break;
    	}
    	case 1: {
    		if(ProfileMan->IsAllowed(u, ProfileManager::SENDFULLMYINFOS) == false) {
                if(colUsers->myInfosLen == 0) {
                    break;
                }

                if(((u->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false) {
                    UserSendCharDelayed(u, colUsers->myInfos, colUsers->myInfosLen);
                } else {
                    if(colUsers->iZMyInfosLen == 0) {
                        colUsers->sZMyInfos = ZlibUtility->CreateZPipe(colUsers->myInfos, colUsers->myInfosLen, colUsers->sZMyInfos,
                            colUsers->iZMyInfosLen, colUsers->iZMyInfosSize, Allign128K);
                        if(colUsers->iZMyInfosLen == 0) {
                            UserSendCharDelayed(u, colUsers->myInfos, colUsers->myInfosLen);
                        } else {
                            UserPutInSendBuf(u, colUsers->sZMyInfos, colUsers->iZMyInfosLen);
                            ui64BytesSentSaved += colUsers->myInfosLen-colUsers->iZMyInfosLen;
                        }
                    } else {
                        UserPutInSendBuf(u, colUsers->sZMyInfos, colUsers->iZMyInfosLen);
                        ui64BytesSentSaved += colUsers->myInfosLen-colUsers->iZMyInfosLen;
                    }
                }
    		} else {
                if(colUsers->myInfosTagLen == 0) {
                    break;
                }

                if(((u->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false) {
                    UserSendCharDelayed(u, colUsers->myInfosTag, colUsers->myInfosTagLen);
                } else {
                    if(colUsers->iZMyInfosTagLen == 0) {
                        colUsers->sZMyInfosTag = ZlibUtility->CreateZPipe(colUsers->myInfosTag, colUsers->myInfosTagLen, colUsers->sZMyInfosTag,
                            colUsers->iZMyInfosTagLen, colUsers->iZMyInfosTagSize, Allign128K);
                        if(colUsers->iZMyInfosTagLen == 0) {
                            UserSendCharDelayed(u, colUsers->myInfosTag, colUsers->myInfosTagLen);
                        } else {
                            UserPutInSendBuf(u, colUsers->sZMyInfosTag, colUsers->iZMyInfosTagLen);
                            ui64BytesSentSaved += colUsers->myInfosTagLen-colUsers->iZMyInfosTagLen;
                        }
                    } else {
                        UserPutInSendBuf(u, colUsers->sZMyInfosTag, colUsers->iZMyInfosTagLen);
                        ui64BytesSentSaved += colUsers->myInfosTagLen-colUsers->iZMyInfosTagLen;
                    }
                }
    		}
    		break;
    	}
        case 2: {
            if(colUsers->myInfosLen == 0) {
                break;
            }

            if(((u->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false) {
                UserSendCharDelayed(u, colUsers->myInfos, colUsers->myInfosLen);
            } else {
                if(colUsers->iZMyInfosLen == 0) {
                    colUsers->sZMyInfos = ZlibUtility->CreateZPipe(colUsers->myInfos, colUsers->myInfosLen, colUsers->sZMyInfos,
                        colUsers->iZMyInfosLen, colUsers->iZMyInfosSize, Allign128K);
                    if(colUsers->iZMyInfosLen == 0) {
                        UserSendCharDelayed(u, colUsers->myInfos, colUsers->myInfosLen);
                    } else {
                        UserPutInSendBuf(u, colUsers->sZMyInfos, colUsers->iZMyInfosLen);
                        ui64BytesSentSaved += colUsers->myInfosLen-colUsers->iZMyInfosLen;
                    }
                } else {
                    UserPutInSendBuf(u, colUsers->sZMyInfos, colUsers->iZMyInfosLen);
                    ui64BytesSentSaved += colUsers->myInfosLen-colUsers->iZMyInfosLen;
                }
            }
    	}
    	default:
            break;
    }
	
	if(ProfileMan->IsAllowed(u, ProfileManager::ALLOWEDOPCHAT) == false || (SettingManager->bBools[SETBOOL_REG_OP_CHAT] == false ||
        (SettingManager->bBools[SETBOOL_REG_BOT] == true && SettingManager->bBotsSameNick == true))) {
        if(colUsers->opListLen > 9) {
            if(((u->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false) {
                UserSendCharDelayed(u, colUsers->opList, colUsers->opListLen);
            } else {
                if(colUsers->iZOpListLen == 0) {
                    colUsers->sZOpList = ZlibUtility->CreateZPipe(colUsers->opList, colUsers->opListLen, colUsers->sZOpList,
                        colUsers->iZOpListLen, colUsers->iZOpListSize, Allign16K);
                    if(colUsers->iZOpListLen == 0) {
                        UserSendCharDelayed(u, colUsers->opList, colUsers->opListLen);
                    } else {
                        UserPutInSendBuf(u, colUsers->sZOpList, colUsers->iZOpListLen);
                        ui64BytesSentSaved += colUsers->opListLen-colUsers->iZOpListLen;
                    }
                } else {
                    UserPutInSendBuf(u, colUsers->sZOpList, colUsers->iZOpListLen);
                    ui64BytesSentSaved += colUsers->opListLen-colUsers->iZOpListLen;
                }  
            }
        }
    } else {
        // PPK ... OpChat bot is now visible only for OPs ;)
        UserSendCharDelayed(u, SettingManager->sPreTexts[SetMan::SETPRETXT_OP_CHAT_MYINFO],
            SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_OP_CHAT_MYINFO]);
        int iLen = sprintf(msg, "%s$$|", SettingManager->sTexts[SETTXT_OP_CHAT_NICK]);
        if(CheckSprintf(iLen, 1024, "UserAddUserList1") == true) {
            if(colUsers->opListSize < colUsers->opListLen+iLen) {
                char * pOldBuf = colUsers->opList;
#ifdef _WIN32
                colUsers->opList = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, colUsers->opListSize+OPLISTSIZE+1);
#else
				colUsers->opList = (char *)realloc(pOldBuf, colUsers->opListSize+OPLISTSIZE+1);
#endif
                if(colUsers->opList == NULL) {
                    colUsers->opList = pOldBuf;
                    u->ui32BoolBits |= User::BIT_ERROR;
                    UserClose(u);

                    AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes for opList in UserAddUserList\n", (uint64_t)(colUsers->opListSize+OPLISTSIZE+1));

                    return;
                }
                colUsers->opListSize += OPLISTSIZE;
            }
    
            memcpy(colUsers->opList+colUsers->opListLen-1, msg, iLen);
            colUsers->opList[colUsers->opListLen+(iLen-1)] = '\0';
            UserSendCharDelayed(u, colUsers->opList, colUsers->opListLen+(iLen-1));
            colUsers->opList[colUsers->opListLen-1] = '|';
            colUsers->opList[colUsers->opListLen] = '\0';
        }
    }

    if(ProfileMan->IsAllowed(u, ProfileManager::SENDALLUSERIP) == true && ((u->ui32SupportBits & User::SUPPORTBIT_USERIP2) == User::SUPPORTBIT_USERIP2) == true) {
        if(colUsers->userIPListLen > 9) {
            if(((u->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false) {
                UserSendCharDelayed(u, colUsers->userIPList, colUsers->userIPListLen);
            } else {
                if(colUsers->iZUserIPListLen == 0) {
                    colUsers->sZUserIPList = ZlibUtility->CreateZPipe(colUsers->userIPList, colUsers->userIPListLen, colUsers->sZUserIPList,
                        colUsers->iZUserIPListLen, colUsers->iZUserIPListSize, Allign16K);
                    if(colUsers->iZUserIPListLen == 0) {
                        UserSendCharDelayed(u, colUsers->userIPList, colUsers->userIPListLen);
                    } else {
                        UserPutInSendBuf(u, colUsers->sZUserIPList, colUsers->iZUserIPListLen);
                        ui64BytesSentSaved += colUsers->userIPListLen-colUsers->iZUserIPListLen;
                    }
                } else {
                    UserPutInSendBuf(u, colUsers->sZUserIPList, colUsers->iZUserIPListLen);
                    ui64BytesSentSaved += colUsers->userIPListLen-colUsers->iZUserIPListLen;
                }  
            }
        }
    }
}
//---------------------------------------------------------------------------

bool UserGenerateMyInfoLong(User *u) { // true == changed
    // Prepare myinfo with nick
    int iLen = sprintf(msg, "$MyINFO $ALL %s ", u->sNick);
    if(CheckSprintf(iLen, 1024, "UserGenerateMyInfoLong") == false) {
        return false;
    }

    // Add description
    if(u->ui8ChangedDescriptionLongLen != 0) {
        if(u->sChangedDescriptionLong != NULL) {
            memcpy(msg+iLen, u->sChangedDescriptionLong, u->ui8ChangedDescriptionLongLen);
            iLen += u->ui8ChangedDescriptionLongLen;
        }

        if(((u->ui32InfoBits & User::INFOBIT_DESCRIPTION_LONG_PERM) == User::INFOBIT_DESCRIPTION_LONG_PERM) == false) {
            if(u->sChangedDescriptionLong != NULL) {
                UserFreeInfo(u->sChangedDescriptionLong, "sChangedDescriptionLong");
                u->sChangedDescriptionLong = NULL;
            }
            u->ui8ChangedDescriptionLongLen = 0;
        }
    } else if(u->sDescription != NULL) {
        memcpy(msg+iLen, u->sDescription, (size_t)u->ui8DescriptionLen);
        iLen += u->ui8DescriptionLen;
    }

    // Add tag
    if(u->ui8ChangedTagLongLen != 0) {
        if(u->sChangedTagLong != NULL) {
            memcpy(msg+iLen, u->sChangedTagLong, u->ui8ChangedTagLongLen);
            iLen += u->ui8ChangedTagLongLen;
        }

        if(((u->ui32InfoBits & User::INFOBIT_TAG_LONG_PERM) == User::INFOBIT_TAG_LONG_PERM) == false) {
            if(u->sChangedTagLong != NULL) {
                UserFreeInfo(u->sChangedTagLong, "sChangedTagLong");
                u->sChangedTagLong = NULL;
            }
            u->ui8ChangedTagLongLen = 0;
        }
    } else if(u->sTag != NULL) {
        memcpy(msg+iLen, u->sTag, (size_t)u->ui8TagLen);
        iLen += (int)u->ui8TagLen;
    }

    memcpy(msg+iLen, "$ $", 3);
    iLen += 3;

    // Add connection
    if(u->ui8ChangedConnectionLongLen != 0) {
        if(u->sChangedConnectionLong != NULL) {
            memcpy(msg+iLen, u->sChangedConnectionLong, u->ui8ChangedConnectionLongLen);
            iLen += u->ui8ChangedConnectionLongLen;
        }

        if(((u->ui32InfoBits & User::INFOBIT_CONNECTION_LONG_PERM) == User::INFOBIT_CONNECTION_LONG_PERM) == false) {
            if(u->sChangedConnectionLong != NULL) {
                UserFreeInfo(u->sChangedConnectionLong, "sChangedConnectionLong");
                u->sChangedConnectionLong = NULL;
            }
            u->ui8ChangedConnectionLongLen = 0;
        }
    } else if(u->sConnection != NULL) {
        memcpy(msg+iLen, u->sConnection, u->ui8ConnectionLen);
        iLen += u->ui8ConnectionLen;
    }

    // add magicbyte
    char cMagic = u->MagicByte;

    if((u->ui32BoolBits & User::BIT_QUACK_SUPPORTS) == User::BIT_QUACK_SUPPORTS) {
        cMagic &= ~0x10; // TLSv1 support
        cMagic &= ~0x20; // TLSv1 support
    }

    if((u->ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4) {
        cMagic |= 0x40; // IPv4 support
    } else {
        cMagic &= ~0x40; // IPv4 support
    }

    if((u->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
        cMagic |= 0x80; // IPv6 support
    } else {
        cMagic &= ~0x80; // IPv6 support
    }

    msg[iLen] = cMagic;
    msg[iLen+1] = '$';
    iLen += 2;

    // Add email
    if(u->ui8ChangedEmailLongLen != 0) {
        if(u->sChangedEmailLong != NULL) {
            memcpy(msg+iLen, u->sChangedEmailLong, u->ui8ChangedEmailLongLen);
            iLen += u->ui8ChangedEmailLongLen;
        }

        if(((u->ui32InfoBits & User::INFOBIT_EMAIL_LONG_PERM) == User::INFOBIT_EMAIL_LONG_PERM) == false) {
            if(u->sChangedEmailLong != NULL) {
                UserFreeInfo(u->sChangedEmailLong, "sChangedEmailLong");
                u->sChangedEmailLong = NULL;
            }
            u->ui8ChangedEmailLongLen = 0;
        }
    } else if(u->sEmail != NULL) {
        memcpy(msg+iLen, u->sEmail, (size_t)u->ui8EmailLen);
        iLen += (int)u->ui8EmailLen;
    }

    // Add share and end of myinfo
	int iRet = sprintf(msg+iLen, "$%" PRIu64 "$|", u->ui64ChangedSharedSizeLong);
    iLen += iRet;

    if(((u->ui32InfoBits & User::INFOBIT_SHARE_LONG_PERM) == User::INFOBIT_SHARE_LONG_PERM) == false) {
        u->ui64ChangedSharedSizeLong = u->ui64SharedSize;
    }

    if(CheckSprintf1(iRet, iLen, 1024, "UserGenerateMyInfoLong2") == false) {
        return false;
    }

    if(u->sMyInfoLong != NULL) {
        if(u->ui16MyInfoLongLen == (uint16_t)iLen && memcmp(u->sMyInfoLong+14+u->ui8NickLen, msg+14+u->ui8NickLen, u->ui16MyInfoLongLen-14-u->ui8NickLen) == 0) {
            return false;
        }
    }

    UserSetMyInfoLong(u, msg, (uint16_t)iLen);

    return true;
}
//---------------------------------------------------------------------------

bool UserGenerateMyInfoShort(User *u) { // true == changed
    // Prepare myinfo with nick
    int iLen = sprintf(msg, "$MyINFO $ALL %s ", u->sNick);
    if(CheckSprintf(iLen, 1024, "UserGenerateMyInfoShort") == false) {
        return false;
    }

    // Add mode to start of description if is enabled
    if(SettingManager->bBools[SETBOOL_MODE_TO_DESCRIPTION] == true && u->sModes[0] != 0) {
        char * sDescription = NULL;

        if(u->ui8ChangedDescriptionShortLen != 0) {
            sDescription = u->sChangedDescriptionShort;
        } else if(SettingManager->bBools[SETBOOL_STRIP_DESCRIPTION] == true) {
            sDescription = u->sDescription;
        }

        if(sDescription == NULL) {
            msg[iLen] = u->sModes[0];
            iLen++;
        } else if(sDescription[0] != u->sModes[0] && sDescription[1] != ' ') {
            msg[iLen] = u->sModes[0];
            msg[iLen+1] = ' ';
            iLen += 2;
        }
    }

    // Add description
    if(u->ui8ChangedDescriptionShortLen != 0) {
        if(u->sChangedDescriptionShort != NULL) {
            memcpy(msg+iLen, u->sChangedDescriptionShort, u->ui8ChangedDescriptionShortLen);
            iLen += u->ui8ChangedDescriptionShortLen;
        }

        if(((u->ui32InfoBits & User::INFOBIT_DESCRIPTION_SHORT_PERM) == User::INFOBIT_DESCRIPTION_SHORT_PERM) == false) {
            if(u->sChangedDescriptionShort != NULL) {
                UserFreeInfo(u->sChangedDescriptionShort, "sChangedDescriptionShort");
                u->sChangedDescriptionShort = NULL;
            }
            u->ui8ChangedDescriptionShortLen = 0;
        }
    } else if(SettingManager->bBools[SETBOOL_STRIP_DESCRIPTION] == false && u->sDescription != NULL) {
        memcpy(msg+iLen, u->sDescription, u->ui8DescriptionLen);
        iLen += u->ui8DescriptionLen;
    }

    // Add tag
    if(u->ui8ChangedTagShortLen != 0) {
        if(u->sChangedTagShort != NULL) {
            memcpy(msg+iLen, u->sChangedTagShort, u->ui8ChangedTagShortLen);
            iLen += u->ui8ChangedTagShortLen;
        }

        if(((u->ui32InfoBits & User::INFOBIT_TAG_SHORT_PERM) == User::INFOBIT_TAG_SHORT_PERM) == false) {
            if(u->sChangedTagShort != NULL) {
                UserFreeInfo(u->sChangedTagShort, "sChangedTagShort");
                u->sChangedTagShort = NULL;
            }
            u->ui8ChangedTagShortLen = 0;
        }
    } else if(SettingManager->bBools[SETBOOL_STRIP_TAG] == false && u->sTag != NULL) {
        memcpy(msg+iLen, u->sTag, (size_t)u->ui8TagLen);
        iLen += (int)u->ui8TagLen;
    }

    // Add mode to myinfo if is enabled
    if(SettingManager->bBools[SETBOOL_MODE_TO_MYINFO] == true && u->sModes[0] != 0) {
        int iRet = sprintf(msg+iLen, "$%c$", u->sModes[0]);
        iLen += iRet;
        if(CheckSprintf1(iRet, iLen, 1024, "UserGenerateMyInfoShort1") == false) {
            return false;
        }
    } else {
        memcpy(msg+iLen, "$ $", 3);
        iLen += 3;
    }

    // Add connection
    if(u->ui8ChangedConnectionShortLen != 0) {
        if(u->sChangedConnectionShort != NULL) {
            memcpy(msg+iLen, u->sChangedConnectionShort, u->ui8ChangedConnectionShortLen);
            iLen += u->ui8ChangedConnectionShortLen;
        }

        if(((u->ui32InfoBits & User::INFOBIT_CONNECTION_SHORT_PERM) == User::INFOBIT_CONNECTION_SHORT_PERM) == false) {
            if(u->sChangedConnectionShort != NULL) {
                UserFreeInfo(u->sChangedConnectionShort, "sChangedConnectionShort");
                u->sChangedConnectionShort = NULL;
            }
            u->ui8ChangedConnectionShortLen = 0;
        }
    } else if(SettingManager->bBools[SETBOOL_STRIP_CONNECTION] == false && u->sConnection != NULL) {
        memcpy(msg+iLen, u->sConnection, u->ui8ConnectionLen);
        iLen += u->ui8ConnectionLen;
    }

    // add magicbyte
    char cMagic = u->MagicByte;

    if((u->ui32BoolBits & User::BIT_QUACK_SUPPORTS) == User::BIT_QUACK_SUPPORTS) {
        cMagic &= ~0x10; // TLSv1 support
        cMagic &= ~0x20; // TLSv1 support
    }

    if((u->ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4) {
        cMagic |= 0x40; // IPv4 support
    } else {
        cMagic &= ~0x40; // IPv4 support
    }

    if((u->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
        cMagic |= 0x80; // IPv6 support
    } else {
        cMagic &= ~0x80; // IPv6 support
    }

    msg[iLen] = cMagic;
    msg[iLen+1] = '$';
    iLen += 2;

    // Add email
    if(u->ui8ChangedEmailShortLen != 0) {
        if(u->sChangedEmailShort != NULL) {
            memcpy(msg+iLen, u->sChangedEmailShort, u->ui8ChangedEmailShortLen);
            iLen += u->ui8ChangedEmailShortLen;
        }

        if(((u->ui32InfoBits & User::INFOBIT_EMAIL_SHORT_PERM) == User::INFOBIT_EMAIL_SHORT_PERM) == false) {
            if(u->sChangedEmailShort != NULL) {
                UserFreeInfo(u->sChangedEmailShort, "sChangedEmailShort");
                u->sChangedEmailShort = NULL;
            }
            u->ui8ChangedEmailShortLen = 0;
        }
    } else if(SettingManager->bBools[SETBOOL_STRIP_EMAIL] == false && u->sEmail != NULL) {
        memcpy(msg+iLen, u->sEmail, (size_t)u->ui8EmailLen);
        iLen += (int)u->ui8EmailLen;
    }

    // Add share and end of myinfo
	int iRet = sprintf(msg+iLen, "$%" PRIu64 "$|", u->ui64ChangedSharedSizeShort);
    iLen += iRet;

    if(((u->ui32InfoBits & User::INFOBIT_SHARE_SHORT_PERM) == User::INFOBIT_SHARE_SHORT_PERM) == false) {
        u->ui64ChangedSharedSizeShort = u->ui64SharedSize;
    }

    if(CheckSprintf1(iRet, iLen, 1024, "UserGenerateMyInfoShort2") == false) {
        return false;
    }

    if(u->sMyInfoShort != NULL) {
        if(u->ui16MyInfoShortLen == (uint16_t)iLen && memcmp(u->sMyInfoShort+14+u->ui8NickLen, msg+14+u->ui8NickLen, u->ui16MyInfoShortLen-14-u->ui8NickLen) == 0) {
            return false;
        }
    }

    UserSetMyInfoShort(u, msg, (uint16_t)iLen);

    return true;
}
//---------------------------------------------------------------------------

#ifdef _WIN32
void UserFreeInfo(char * sInfo, const char * sName) {
	if(sInfo != NULL) {
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sInfo) == 0) {
			AppendDebugLog(("%s - [MEM] Cannot deallocate "+string(sName)+" in UserFreeInfo\n").c_str(), 0);
        }
    }
#else
void UserFreeInfo(char * sInfo, const char */* sName*/) {
	free(sInfo);
#endif
}
//------------------------------------------------------------------------------

void UserHasSuspiciousTag(User *curUser) {
	if(SettingManager->bBools[SETBOOL_REPORT_SUSPICIOUS_TAG] == true && SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
		if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
			int imsgLen = sprintf(msg, "%s $<%s> *** %s (%s) %s. %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                curUser->sNick, curUser->sIP, LanguageManager->sTexts[LAN_HAS_SUSPICIOUS_TAG_CHECK_HIM], LanguageManager->sTexts[LAN_FULL_DESCRIPTION]);
            if(CheckSprintf(imsgLen, 1024, "UserHasSuspiciousTag1") == true) {
                memcpy(msg+imsgLen, curUser->sDescription, (size_t)curUser->ui8DescriptionLen);
                imsgLen += (int)curUser->ui8DescriptionLen;
                msg[imsgLen] = '|';
                g_GlobalDataQueue->SingleItemStore(msg, imsgLen+1, NULL, 0, GlobalDataQueue::SI_PM2OPS);
            }
		} else {
			int imsgLen = sprintf(msg, "<%s> *** %s (%s) %s. %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->sNick, curUser->sIP, 
                LanguageManager->sTexts[LAN_HAS_SUSPICIOUS_TAG_CHECK_HIM], LanguageManager->sTexts[LAN_FULL_DESCRIPTION]);
            if(CheckSprintf(imsgLen, 1024, "UserHasSuspiciousTag2") == true) {
                memcpy(msg+imsgLen, curUser->sDescription, (size_t)curUser->ui8DescriptionLen);
                imsgLen += (int)curUser->ui8DescriptionLen;
                msg[imsgLen] = '|';
                g_GlobalDataQueue->AddQueueItem(msg, imsgLen+1, NULL, 0, GlobalDataQueue::CMD_OPS);
            }
	   }
    }
    curUser->ui32BoolBits &= ~User::BIT_HAVE_BADTAG;
}
//---------------------------------------------------------------------------

bool UserProcessRules(User * u) {
    // if share limit enabled, check it
    if(ProfileMan->IsAllowed(u, ProfileManager::NOSHARELIMIT) == false) {      
        if((SettingManager->ui64MinShare != 0 && u->ui64SharedSize < SettingManager->ui64MinShare) ||
            (SettingManager->ui64MaxShare != 0 && u->ui64SharedSize > SettingManager->ui64MaxShare)) {
            UserSendChar(u, SettingManager->sPreTexts[SetMan::SETPRETXT_SHARE_LIMIT_MSG],
                SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_SHARE_LIMIT_MSG]);
            //UdpDebug->Broadcast("[SYS] User with low or high share " + u->Nick + " (" + u->IP + ") disconnected.");
            return false;
        }
    }
    
    // no Tag? Apply rule
    if(u->sTag == NULL) {
        if(ProfileMan->IsAllowed(u, ProfileManager::NOTAGCHECK) == false) {
            if(SettingManager->iShorts[SETSHORT_NO_TAG_OPTION] != 0) {
                UserSendChar(u, SettingManager->sPreTexts[SetMan::SETPRETXT_NO_TAG_MSG],
                    SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_NO_TAG_MSG]);
                //UdpDebug->Broadcast("[SYS] User without Tag " + u->Nick + (" + u->IP + ") redirected.");
                return false;
            }
        }
    } else {
        // min and max slot check
        if(ProfileMan->IsAllowed(u, ProfileManager::NOSLOTCHECK) == false) {
            // TODO 2 -oPTA -ccheckers: $SR based slots fetching for no_tag users
        
			if((SettingManager->iShorts[SETSHORT_MIN_SLOTS_LIMIT] != 0 && u->Slots < (uint32_t)SettingManager->iShorts[SETSHORT_MIN_SLOTS_LIMIT]) ||
				(SettingManager->iShorts[SETSHORT_MAX_SLOTS_LIMIT] != 0 && u->Slots > (uint32_t)SettingManager->iShorts[SETSHORT_MAX_SLOTS_LIMIT])) {
                UserSendChar(u, SettingManager->sPreTexts[SetMan::SETPRETXT_SLOTS_LIMIT_MSG],
                    SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_SLOTS_LIMIT_MSG]);
                //UdpDebug->Broadcast("[SYS] User with bad slots " + u->Nick + " (" + u->IP + ") disconnected.");
                return false;
            }
        }
    
        // slots/hub ration check
        if(ProfileMan->IsAllowed(u, ProfileManager::NOSLOTHUBRATIO) == false && 
            SettingManager->iShorts[SETSHORT_HUB_SLOT_RATIO_HUBS] != 0 && SettingManager->iShorts[SETSHORT_HUB_SLOT_RATIO_SLOTS] != 0) {
            uint32_t slots = u->Slots;
            uint32_t hubs = u->Hubs > 0 ? u->Hubs : 1;
        	if(((double)slots / hubs) < ((double)SettingManager->iShorts[SETSHORT_HUB_SLOT_RATIO_SLOTS] / SettingManager->iShorts[SETSHORT_HUB_SLOT_RATIO_HUBS])) {
        	    UserSendChar(u, SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SLOT_RATIO_MSG],
                    SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_HUB_SLOT_RATIO_MSG]);
                //UdpDebug->Broadcast("[SYS] User with bad hub/slot ratio " + u->Nick + " (" + u->IP + ") disconnected.");
                return false;
            }
        }
    
        // hub checker
        if(ProfileMan->IsAllowed(u, ProfileManager::NOMAXHUBCHECK) == false && SettingManager->iShorts[SETSHORT_MAX_HUBS_LIMIT] != 0) {
            if(u->Hubs > (uint32_t)SettingManager->iShorts[SETSHORT_MAX_HUBS_LIMIT]) {
                UserSendChar(u, SettingManager->sPreTexts[SetMan::SETPRETXT_MAX_HUBS_LIMIT_MSG],
                    SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_MAX_HUBS_LIMIT_MSG]);
                //UdpDebug->Broadcast("[SYS] User with bad hubs count " + u->Nick + " (" + u->IP + ") disconnected.");
                return false;
            }
        }
    }
    
    return true;
}

//------------------------------------------------------------------------------

void UserAddPrcsdCmd(User * u, const unsigned char &cType, char * sCommand, const size_t &szCommandLen, User * to, const bool &bIsPm/* = false*/) {
    if(cType == PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO) {
        PrcsdToUsrCmd *next = u->cmdToUserStrt;
        while(next != NULL) {
            PrcsdToUsrCmd *cur = next;
            next = cur->next;
            if(cur->To == to) {
                char * pOldBuf = cur->sCommand;
#ifdef _WIN32
                cur->sCommand = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, cur->iLen+szCommandLen+1);
#else
				cur->sCommand = (char *)realloc(pOldBuf, cur->iLen+szCommandLen+1);
#endif
                if(cur->sCommand == NULL) {
                    cur->sCommand = pOldBuf;
                    u->ui32BoolBits |= User::BIT_ERROR;
                    UserClose(u);

					AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in UserAddPrcsdCmd\n", (uint64_t)(cur->iLen+szCommandLen+1));

                    return;
                }
                memcpy(cur->sCommand+cur->iLen, sCommand, szCommandLen);
                cur->sCommand[cur->iLen+szCommandLen] = '\0';
                cur->iLen += (uint32_t)szCommandLen;
                cur->iPmCount += bIsPm == true ? 1 : 0;
                return;
            }
        }

        PrcsdToUsrCmd * pNewcmd = new PrcsdToUsrCmd();
        if(pNewcmd == NULL) {
            u->ui32BoolBits |= User::BIT_ERROR;
            UserClose(u);

			AppendDebugLog("%s - [MEM] UserAddPrcsdCmd cannot allocate new pNewcmd\n", 0);

        	return;
        }

#ifdef _WIN32
        pNewcmd->sCommand = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szCommandLen+1);
#else
		pNewcmd->sCommand = (char *)malloc(szCommandLen+1);
#endif
        if(pNewcmd->sCommand == NULL) {
            u->ui32BoolBits |= User::BIT_ERROR;
            UserClose(u);

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sCommand in UserAddPrcsdCmd\n", (uint64_t)(szCommandLen+1));

            return;
        }

        memcpy(pNewcmd->sCommand, sCommand, szCommandLen);
        pNewcmd->sCommand[szCommandLen] = '\0';

        pNewcmd->iLen = (uint32_t)szCommandLen;
        pNewcmd->iPmCount = bIsPm == true ? 1 : 0;
        pNewcmd->iLoops = 0;
        pNewcmd->To = to;
        
#ifdef _WIN32
        pNewcmd->ToNick = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, to->ui8NickLen+1);
#else
		pNewcmd->ToNick = (char *)malloc(to->ui8NickLen+1);
#endif
        if(pNewcmd->ToNick == NULL) {
            u->ui32BoolBits |= User::BIT_ERROR;
            UserClose(u);

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for ToNick in UserAddPrcsdCmd\n", (uint64_t)(to->ui8NickLen+1));

            return;
        }   

        memcpy(pNewcmd->ToNick, to->sNick, to->ui8NickLen);
        pNewcmd->ToNick[to->ui8NickLen] = '\0';
        
        pNewcmd->iToNickLen = to->ui8NickLen;
        pNewcmd->next = NULL;
               
        if(u->cmdToUserStrt == NULL) {
            u->cmdToUserStrt = pNewcmd;
            u->cmdToUserEnd = pNewcmd;
        } else {
            u->cmdToUserEnd->next = pNewcmd;
            u->cmdToUserEnd = pNewcmd;
        }
        return;
    }
    
    PrcsdUsrCmd * pNewcmd = new PrcsdUsrCmd();
    if(pNewcmd == NULL) {
        u->ui32BoolBits |= User::BIT_ERROR;
        UserClose(u);

		AppendDebugLog("%s - [MEM] UserAddPrcsdCmd cannot allocate new pNewcmd1\n", 0);

    	return;
    }

#ifdef _WIN32
    pNewcmd->sCommand = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szCommandLen+1);
#else
	pNewcmd->sCommand = (char *)malloc(szCommandLen+1);
#endif
    if(pNewcmd->sCommand == NULL) {
        u->ui32BoolBits |= User::BIT_ERROR;
        UserClose(u);

		AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sCommand in UserAddPrcsdCmd1\n", (uint64_t)(szCommandLen+1));

        return;
    }

    memcpy(pNewcmd->sCommand, sCommand, szCommandLen);
    pNewcmd->sCommand[szCommandLen] = '\0';

    pNewcmd->iLen = (uint32_t)szCommandLen;
    pNewcmd->cType = cType;
    pNewcmd->next = NULL;
    pNewcmd->ptr = (void *)to;

    if(u->cmdStrt == NULL) {
        u->cmdStrt = pNewcmd;
        u->cmdEnd = pNewcmd;
    } else {
        u->cmdEnd->next = pNewcmd;
        u->cmdEnd = pNewcmd;
    }
}
//---------------------------------------------------------------------------

void UserAddMeOrIPv4Check(User * pUser) {
    if(((pUser->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) && ((pUser->ui32SupportBits & User::SUPPORTBIT_IPV4) == User::SUPPORTBIT_IPV4) && sHubIP[0] != '\0' && bUseIPv4 == true) {
        pUser->ui8State = User::STATE_IPV4_CHECK;

        int imsgLen = sprintf(msg, "$ConnectToMe %s %s:%s|", pUser->sNick, sHubIP, string(SettingManager->iPortNumbers[0]).c_str());
        if(CheckSprintf(imsgLen, 1024, "UserAddMeOrIPv4Check") == false) {
            return;
        }

        pUser->uLogInOut->ui64IPv4CheckTick = ui64ActualTick;

        UserSendCharDelayed(pUser, msg, imsgLen);
    } else {
        pUser->ui8State = User::STATE_ADDME;
    }
}
//---------------------------------------------------------------------------

char * UserSetUserInfo(char * sOldData, uint8_t &ui8OldDataLen, char * sNewData, size_t &sz8NewDataLen, const char * sDataName) {
    if(sOldData != NULL) {
        UserFreeInfo(sOldData, sDataName);
        sOldData = NULL;
        ui8OldDataLen = 0;
    }

    if(sz8NewDataLen > 0) {
#ifdef _WIN32
        sOldData = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, sz8NewDataLen+1);
#else
        sOldData = (char *)malloc(sz8NewDataLen+1);
#endif
        if(sOldData == NULL) {
            AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes in UserSetUserInfo\n", (uint64_t)(sz8NewDataLen+1));
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

void UserRemFromSendBuf(User * u, const char * sData, const uint32_t &iLen, const uint32_t &iSbLen) {
	char *match = strstr(u->sendbuf+iSbLen, sData);
    if(match != NULL) {
        memmove(match, match+iLen, u->sbdatalen-((match+(iLen))-u->sendbuf));
        u->sbdatalen -= iLen;
        u->sendbuf[u->sbdatalen] = '\0';
    }
}
//------------------------------------------------------------------------------

void UserDeletePrcsdUsrCmd(PrcsdUsrCmd * pCommand) {
#ifdef _WIN32
    if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pCommand->sCommand) == 0) {
        AppendDebugLog("%s - [MEM] Cannot deallocate pCommand->sCommand in UserDeletePrcsdUsrCmd\n", 0);
    }
#else
    free(pCommand->sCommand);
#endif
    delete pCommand;
}
