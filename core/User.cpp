/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
 * Copyright (C) 2004-2011  Petr Kozelka, PPK at PtokaX dot org

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
#include "globalQueue.h"
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
#include "ClientTagManager.h"
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

    for(uint32_t i = iStrtLen; i < u->rbdatalen; i++) {
        if(u->recvbuf[i] == '|') {
            // look for pipes in the data - process lines one by one
            c = u->recvbuf[i+1];
            u->recvbuf[i+1] = '\0';
            uint32_t iCommandLen = (uint32_t)(((u->recvbuf+i)-buffer)+1);
            if(buffer[0] == '|') {
                //UdpDebug->Broadcast("[SYS] heartbeat from " + string(u->Nick, u->NickLen));
                //send(Sck, "|", 1, 0);
            } else if(iCommandLen < (u->iState < User::STATE_ADDME ? 1024U : 65536U)) {
        		DcCommands->PreProcessData(u, buffer, true, iCommandLen);
        	} else {
                int imsgLen = sprintf(msg, "<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                    LanguageManager->sTexts[LAN_CMD_TOO_LONG]);
                if(CheckSprintf(imsgLen, 1024, "UserProcessLines1") == true) {
                    UserSendCharDelayed(u, msg, imsgLen);
                }
				UserClose(u);
				UdpDebug->Broadcast("[BUF] " + string(u->Nick, u->NickLen) + " (" + string(u->IP, u->ui8IpLen) + ") Received command too long. User disconnected.");
                return false;
            }
        	u->recvbuf[i+1] = c;
        	buffer += iCommandLen;
            if(u->iState >= User::STATE_CLOSING) {
                return true;
            }
        } else if(u->recvbuf[i] == '\0') {
            // look for NULL character and replace with zero
            u->recvbuf[i] = '0';
            continue;
        }
	}

	u->rbdatalen -= (uint32_t)(buffer-u->recvbuf);

	if(u->rbdatalen == 0) {
        DcCommands->ProcessCmds(u);
        u->recvbuf[0] = '\0';
        return false;
    } else if(u->rbdatalen != 1) {
        if(u->rbdatalen > unsigned (u->iState < User::STATE_ADDME ? 1024 : 65536)) {
            // PPK ... we don't want commands longer than 64 kB, drop this user !
            int imsgLen = sprintf(msg, "<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_CMD_TOO_LONG]);
            if(CheckSprintf(imsgLen, 1024, "UserProcessLines2") == true) {
                UserSendCharDelayed(u, msg, imsgLen);
            }
            UserClose(u);
			UdpDebug->Broadcast("[BUF] " + string(u->Nick, u->NickLen) + " (" + string(u->IP, u->ui8IpLen) +
				") RecvBuffer overflow. User disconnected.");
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

static void UserRemFromSendBuf(User * u, const char * sData, const uint32_t &iLen, const uint32_t &iSbLen) {
	char *match = strstr(u->sendbuf+iSbLen, sData);
    if(match != NULL) {
        memmove(match, match+iLen, u->sbdatalen-((match+(iLen))-u->sendbuf));
        u->sbdatalen -= iLen;
        u->sendbuf[u->sbdatalen] = '\0';
    }
}
//------------------------------------------------------------------------------

static void UserSetBadTag(User * u, char * Descr, uint8_t DescrLen) {
    // PPK ... clear all tag related things
    u->Ver = NULL;
    u->ui8VerLen = 0;

    u->Mode = '\0';
    u->Hubs = u->Slots = u->OLimit = u->LLimit = u->DLimit = u->iNormalHubs = u->iRegHubs = u->iOpHubs = 0;
    u->ui32BoolBits |= User::BIT_OLDHUBSTAG;
    u->ui32BoolBits |= User::BIT_HAVE_BADTAG;
    
    u->Description = Descr;
    u->ui8DescrLen = (uint8_t)DescrLen;

    // PPK ... clear (fake) tag
    u->Tag = NULL;
    u->ui8TagLen = 0;

    // PPK ... set bad tag
    u->Client = (char *)sBadTag;
    u->ui8ClientLen = 8;

    // PPK ... send report to udp debug
    int imsgLen = sprintf(msg, "[SYS] User %s (%s) have bad TAG (%s) ?!?.", u->Nick, u->IP, u->MyInfoTag);
    if(CheckSprintf(imsgLen, 1024, "SetBadTag") == true) {
        UdpDebug->Broadcast(msg, imsgLen);
    }
}
//---------------------------------------------------------------------------

static void UserParseMyInfo(User * u) {
    memcpy(msg, u->MyInfoTag, u->iMyInfoTagLen);

    char *sMyINFOParts[] = { NULL, NULL, NULL, NULL, NULL };
    uint16_t iMyINFOPartsLen[] = { 0, 0, 0, 0, 0 };

    unsigned char cPart = 0;

    sMyINFOParts[cPart] = msg+14+u->NickLen; // desription start


    for(uint32_t i = 14+u->NickLen; i < u->iMyInfoTagLen-1; i++) {
        if(msg[i] == '$') {
            msg[i] = '\0';
            iMyINFOPartsLen[cPart] = (uint16_t)((msg+i)-sMyINFOParts[cPart]);

            // are we on end of myinfo ???
            if(cPart == 4)
                break;

            cPart++;
            sMyINFOParts[cPart] = msg+i+1;
        }
    }

    // check if we have all myinfo parts, connection and sharesize must have length more than 0 !
    if(sMyINFOParts[0] == NULL || sMyINFOParts[1] == NULL || iMyINFOPartsLen[1] != 1 || sMyINFOParts[2] == NULL || iMyINFOPartsLen[2] == 0 ||
        sMyINFOParts[3] == NULL || sMyINFOParts[4] == NULL || iMyINFOPartsLen[4] == 0) {
        int imsgLen = sprintf(msg, "<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
            LanguageManager->sTexts[LAN_YOU_MyINFO_IS_CORRUPTED]);
        if(CheckSprintf(imsgLen, 1024, "UserParseMyInfo1") == true) {
            UserSendChar(u, msg, imsgLen);
        }
        imsgLen = sprintf(msg, "[SYS] User %s (%s) with bad MyINFO (%s) disconnected.", u->Nick, u->IP, u->MyInfoTag);
        if(CheckSprintf(imsgLen, 1024, "UserParseMyInfo2") == true) {
            UdpDebug->Broadcast(msg, imsgLen);
        }
        UserClose(u);
        return;
    }

    // connection
    u->MagicByte = sMyINFOParts[2][iMyINFOPartsLen[2]-1];
    u->Connection = u->MyInfoTag+(sMyINFOParts[2]-msg);
    u->ui8ConnLen = (uint8_t)(iMyINFOPartsLen[2]-1);

    // email
    if(iMyINFOPartsLen[3] != 0) {
        u->Email = u->MyInfoTag+(sMyINFOParts[3]-msg);
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
        ui64TotalShare -= u->sharedSize;
#ifdef _WIN32
        u->sharedSize = _strtoui64(sMyINFOParts[4], NULL, 10);
#else
		u->sharedSize = strtoull(sMyINFOParts[4], NULL, 10);
#endif
        ui64TotalShare += u->sharedSize;
    } else {
#ifdef _WIN32
        u->sharedSize = _strtoui64(sMyINFOParts[4], NULL, 10);
#else
		u->sharedSize = strtoull(sMyINFOParts[4], NULL, 10);
#endif
    }

    // Reset all tag infos...
    u->Mode = '\0';
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
                u->Description = u->MyInfoTag+(sMyINFOParts[0]-msg);
                u->ui8DescrLen = (uint8_t)iMyINFOPartsLen[0];

                u->Client = (char*)sOtherNoTag;
                u->ui8ClientLen = 14;
                return;
            }

            u->Tag = u->MyInfoTag+(DCTag-msg);
            u->ui8TagLen = (uint8_t)(iMyINFOPartsLen[0]-(DCTag-sMyINFOParts[0]));

            static const uint16_t ui16plusplus = ((uint16_t *)"++")[0];
            if(DCTag[3] == ' ' && ((uint16_t *)(DCTag+1))[0] == ui16plusplus) {
                u->ui32BoolBits |= User::BIT_SUPPORT_NOHELLO;
            }
                
            // PPK ... new tag code with customizable tag file support... but only god knows how slow is :(
            // PPK's code replaced with a faster one by Pta.
            size_t iTagPattLen = 0;
            uint32_t k = 0;
            while(ClientTagManager->cliTags[k].PattLen) { // PattLen == 0 means end of tags list
                static const uint16_t ui16V = ((uint16_t *)" V")[0];
                if(((uint16_t *)(DCTag+ClientTagManager->cliTags[k].PattLen+1))[0] == ui16V &&
#ifdef _WIN32
                    strnicmp(DCTag+1, ClientTagManager->cliTags[k].TagPatt, ClientTagManager->cliTags[k].PattLen) == 0) {
#else
					strncasecmp(DCTag+1, ClientTagManager->cliTags[k].TagPatt, ClientTagManager->cliTags[k].PattLen) == 0) {
#endif
                    u->Client = ClientTagManager->cliTags[k].CliName;
                    u->ui8ClientLen = (uint8_t)strlen(ClientTagManager->cliTags[k].CliName);
                    iTagPattLen = ClientTagManager->cliTags[k].PattLen+2;
                    break;
                }
                k++;
            }

            // no match ? set NoDCTag and return
            if(ClientTagManager->cliTags[k].PattLen == 0) {
                char * sTemp;
                static const uint16_t ui16V = ((uint16_t *)"V:")[0];
                if(SettingManager->bBools[SETBOOL_ACCEPT_UNKNOWN_TAG] == true && (sTemp = strchr(DCTag, ' ')) != NULL &&
                    ((uint16_t *)(sTemp+1))[0] == ui16V) {
                    sTemp[0] = '\0';
                    u->Client = u->MyInfoTag+((DCTag+1)-msg);
                    u->ui8ClientLen = (uint8_t)((sTemp-DCTag)-1);
                    iTagPattLen = (uint32_t)((sTemp-DCTag)+1);
                } else {
                    u->Client = (char *)sUnknownTag;
                    u->ui8ClientLen = 11;
                    u->Tag = NULL;
                    u->ui8TagLen = 0;
                    sMyINFOParts[0][iMyINFOPartsLen[0]-1] = '>'; // not valid DC Tag, add back > tag ending
                    u->Description = u->MyInfoTag+(sMyINFOParts[0]-msg);
                    u->ui8DescrLen = (uint8_t)iMyINFOPartsLen[0];
                    return;
                }
            }

            sMyINFOParts[0][iMyINFOPartsLen[0]-1] = ','; // terminate tag end with ',' for easy tag parsing

            uint32_t reqVals = 0;
            char *sTagPart = DCTag+iTagPattLen;
            for(size_t i = iTagPattLen; i < (size_t)(iMyINFOPartsLen[0]-(DCTag-sMyINFOParts[0])); i++) {
                if(DCTag[i] == ',') {
                    DCTag[i] = '\0';

                    if(sTagPart[1] != ':') {
                        UserSetBadTag(u, u->MyInfoTag+(sMyINFOParts[0]-msg), (uint8_t)iMyINFOPartsLen[0]);
                        return;
                    }
                       
                    switch(sTagPart[0]) {
                        case 'V':
                            // PPK ... fix for potencial memory leak with fake tag
                            if(sTagPart[2] == '\0' || u->Ver) {
                                UserSetBadTag(u, u->MyInfoTag+(sMyINFOParts[0]-msg), (uint8_t)iMyINFOPartsLen[0]);
                                return;
                            }
                            u->Ver = u->MyInfoTag+((sTagPart+2)-msg);
                            u->ui8VerLen = (uint8_t)((DCTag+i)-(sTagPart+2));
                            reqVals++;
                            break;
                        case 'M':
                            if(sTagPart[2] == '\0' || sTagPart[3] != '\0') {
                                UserSetBadTag(u, u->MyInfoTag+(sMyINFOParts[0]-msg), (uint8_t)iMyINFOPartsLen[0]);
                                return;
                            }
                            if(toupper(sTagPart[2]) == 'A') {
                                u->ui32BoolBits |= User::BIT_ACTIVE;
                            } else {
                                u->ui32BoolBits &= ~User::BIT_ACTIVE;
                            }
                            u->Mode = sTagPart[2];
                            reqVals++;
                            break;
                        case 'H': {
                            if(sTagPart[2] == '\0') {
                                UserSetBadTag(u, u->MyInfoTag+(sMyINFOParts[0]-msg), (uint8_t)iMyINFOPartsLen[0]);
                                return;
                            }

                            DCTag[i] = '/';

                            char *sHubsParts[] = { NULL, NULL, NULL };
                            uint16_t iHubsPartsLen[] = { 0, 0, 0 };

                            uint8_t cPart = 0;

                            sHubsParts[cPart] = sTagPart+2;


                            for(uint32_t j = 3; j < (uint32_t)((DCTag+i+1)-sTagPart); j++) {
                                if(sTagPart[j] == '/') {
                                    sTagPart[j] = '\0';
                                    iHubsPartsLen[cPart] = (uint16_t)((sTagPart+j)-sHubsParts[cPart]);

                                    // are we on end of hubs tag part ???
                                    if(cPart == 2)
                                        break;

                                    cPart++;
                                    sHubsParts[cPart] = sTagPart+j+1;
                                }
                            }

                            if(sHubsParts[0] != NULL && sHubsParts[1] != NULL && sHubsParts[2] != NULL) {
                                if(iHubsPartsLen[0] != 0 && iHubsPartsLen[1] != 0 && iHubsPartsLen[2] != 0) {
                                    if(HaveOnlyNumbers(sHubsParts[0], iHubsPartsLen[0]) == false ||
                                        HaveOnlyNumbers(sHubsParts[1], iHubsPartsLen[1]) == false ||
                                        HaveOnlyNumbers(sHubsParts[2], iHubsPartsLen[2]) == false) {
                                        UserSetBadTag(u, u->MyInfoTag+(sMyINFOParts[0]-msg), (uint8_t)iMyINFOPartsLen[0]);
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
                            } else if(sHubsParts[1] == DCTag+i+1 && sHubsParts[2] == NULL) {
                                DCTag[i] = '\0';
                                u->Hubs = atoi(sHubsParts[0]);
                                reqVals++;
                                u->ui32BoolBits |= User::BIT_OLDHUBSTAG;
                                break;
                            }
                            int imsgLen = sprintf(msg, "<%s> %s!|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                                LanguageManager->sTexts[LAN_FAKE_TAG]);
                            if(CheckSprintf(imsgLen, 1024, "UserParseMyInfo3") == true) {
                                UserSendChar(u, msg, imsgLen);
                            }
                            imsgLen = sprintf(msg, "[SYS] User %s (%s) with fake Tag disconnected: ", u->Nick, u->IP);
                            if(CheckSprintf(imsgLen, 1024, "UserParseMyInfo4") == true) {
                                memcpy(msg+imsgLen, u->Tag, u->ui8TagLen);
                                UdpDebug->Broadcast(msg, imsgLen+(int)u->ui8TagLen);
                            }
                            UserClose(u);
                            return;
                        }
                        case 'S':
                            if(sTagPart[2] == '\0') {
                                UserSetBadTag(u, u->MyInfoTag+(sMyINFOParts[0]-msg), (uint8_t)iMyINFOPartsLen[0]);
                                return;
                            }
                            if(HaveOnlyNumbers(sTagPart+2, (uint16_t)strlen(sTagPart+2)) == false) {
                                UserSetBadTag(u, u->MyInfoTag+(sMyINFOParts[0]-msg), (uint8_t)iMyINFOPartsLen[0]);
                                return;
                            }
                            u->Slots = atoi(sTagPart+2);
                            reqVals++;
                            break;
                        case 'O':
                            if(sTagPart[2] == '\0') {
                                UserSetBadTag(u, u->MyInfoTag+(sMyINFOParts[0]-msg), (uint8_t)iMyINFOPartsLen[0]);
                                return;
                            }
                            u->OLimit = atoi(sTagPart+2);
                            break;
                        case 'B':
                            if(sTagPart[2] == '\0') {
                                UserSetBadTag(u, u->MyInfoTag+(sMyINFOParts[0]-msg), (uint8_t)iMyINFOPartsLen[0]);
                                return;
                            }
                            u->LLimit = atoi(sTagPart+2);
                            break;
                        case 'L':
                            if(sTagPart[2] == '\0') {
                                UserSetBadTag(u, u->MyInfoTag+(sMyINFOParts[0]-msg), (uint8_t)iMyINFOPartsLen[0]);
                                return;
                            }
                            u->LLimit = atoi(sTagPart+2);
                            break;
                        case 'D':
                            if(sTagPart[2] == '\0') {
                                UserSetBadTag(u, u->MyInfoTag+(sMyINFOParts[0]-msg), (uint8_t)iMyINFOPartsLen[0]);
                                return;
                            }
                            u->DLimit = atoi(sTagPart+2);
                            break;
                        default:
                            //UdpDebug->Broadcast("[SYS] "+string(u->Nick, u->NickLen)+": Extra info in DC tag: "+string(sTag));
                            break;
                    }
                    sTagPart = DCTag+i+1;
                }
            }
                
            if(reqVals < 4) {
                UserSetBadTag(u, u->MyInfoTag+(sMyINFOParts[0]-msg), (uint8_t)iMyINFOPartsLen[0]);
                return;
            } else {
                u->Description = u->MyInfoTag+(sMyINFOParts[0]-msg);
                u->ui8DescrLen = (uint8_t)(DCTag-sMyINFOParts[0]);
                return;
            }
        } else {
            u->Description = u->MyInfoTag+(sMyINFOParts[0]-msg);
            u->ui8DescrLen = (uint8_t)iMyINFOPartsLen[0];
        }
    }

    u->Client = (char *)sOtherNoTag;
    u->ui8ClientLen = 14;

    u->Tag = NULL;
    u->ui8TagLen = 0;

    u->Ver = NULL;
    u->ui8VerLen = 0;

    u->Mode = '\0';
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
    sMessage = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iMessLen+1);
#else
	sMessage = (char *) malloc(iMessLen+1);
#endif
    if(sMessage == NULL) {
		string sDbgstr = "[BUF] UserBan::UserBann cannot allocate "+string(iMessLen+1)+" bytes of memory for sMessage! "+
			string(sMess, iMessLen);
#ifdef _WIN32
            sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
        AppendSpecialLog(sDbgstr);
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
    if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMessage) == 0) {
		string sDbgstr = "[BUF] Cannot deallocate sMessage in ~UserBan! "+string((uint32_t)GetLastError())+" "+
            string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
        AppendSpecialLog(sDbgstr);
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
    Password = NULL;
    sKickMsg = NULL;

    iToCloseLoops = 0;
    iUserConnectedLen = 0;

    logonClk = 0;
}
//---------------------------------------------------------------------------

LoginLogout::~LoginLogout() {
    delete uBan;

#ifdef _WIN32
    if(sLockUsrConn != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sLockUsrConn) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate sLockUsrConn in ~LoginLogout! "+string((uint32_t)GetLastError())+" "+
                string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
            AppendSpecialLog(sDbgstr);
        }
    }
#else
	free(sLockUsrConn);
#endif

#ifdef _WIN32
    if(Password != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)Password) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate Password in ~LoginLogout! "+string((uint32_t)GetLastError())+" "+
                string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
            AppendSpecialLog(sDbgstr);
        }
    }
#else
	free(Password);
#endif

#ifdef _WIN32
    if(sKickMsg != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sKickMsg) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate sKickMsg in ~LoginLogout! "+string((uint32_t)GetLastError())+" "+
                string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
            AppendSpecialLog(sDbgstr);
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

	sLastChat = NULL;
	sLastPM = NULL;
	sendbuf = NULL;
	recvbuf = NULL;
	sbplayhead = NULL;
	sLastSearch = NULL;
	Nick = (char *)sDefaultNick;
	IP[0] = '\0';
	Version = NULL;
	MyInfo = NULL;
	MyInfoOld = NULL;
	MyInfoTag = NULL;
	Client = (char *)sOtherNoTag;
	Tag = NULL;
	Description = NULL;
	Connection = NULL;
	MagicByte = '\0';
	Email = NULL;
	Ver = NULL;
	Mode = '\0';

	ui8IpLen = 0;

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

	NickLen = 9;
	iMyInfoTagLen = 0;
	iMyInfoLen = 0;
	iMyInfoOldLen = 0;
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
	iState = User::STATE_SOCKET_ACCEPTED;
//    PORT = 0;
	iProfile = -1;
	sendbuflen = 0;
	recvbuflen = 0;
	sbdatalen = 0;
	rbdatalen = 0;
//    sin_len = 0;
	iReceivedPmCount = 0;

	time(&LoginTime);

	ui32NickHash = 0;
	ui32IpHash = 0;

	ui32BoolBits = 0;
	ui32BoolBits |= User::BIT_ACTIVE;
	ui32BoolBits |= User::BIT_OLDHUBSTAG;

	ui8ConnLen = 0;
	ui8DescrLen = 0;
	ui8EmailLen = 0;
	ui8TagLen = 0;
	ui8ClientLen = 14;
	ui8VerLen = 0;

    ui8Country = 246;

	sharedSize = 0;

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

	uLogInOut = new LoginLogout();

    if(uLogInOut == NULL) {
		string sDbgstr = "[BUF] Cannot allocate new uLogInOut!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
		exit(EXIT_FAILURE);
    }

	cmdStrt = NULL;
	cmdEnd = NULL;
	cmdASearch = NULL;
	cmdPSearch = NULL;
	cmdToUserStrt = NULL;
	cmdToUserEnd = NULL;
}
//---------------------------------------------------------------------------

User::~User() {
#ifdef _WIN32
	if(recvbuf != NULL) {
		if(HeapFree(hRecvHeap, HEAP_NO_SERIALIZE, (void *)recvbuf) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate recvbuf in ~User! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hRecvHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
    }
#else
	free(recvbuf);
#endif

#ifdef _WIN32
	if(sendbuf != NULL) {
		if(HeapFree(hSendHeap, HEAP_NO_SERIALIZE, (void *)sendbuf) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate sendbuf in ~User! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hSendHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
    }
#else
	free(sendbuf);
#endif

#ifdef _WIN32
	if(sLastChat != NULL) {
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sLastChat) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate sLastChat in ~User! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
    }
#else
	free(sLastChat);
#endif

#ifdef _WIN32
	if(sLastPM != NULL) {
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sLastPM) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate sLastPM in ~User! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
    }
#else
	free(sLastPM);
#endif

#ifdef _WIN32
	if(sLastSearch != NULL) {
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sLastSearch) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate sLastSearch in ~User! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
    }
#else
	free(sLastSearch);
#endif

#ifdef _WIN32
	if(MyInfo != NULL) {
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)MyInfo) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate MyInfo in ~User! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
    }
#else
	free(MyInfo);
#endif

#ifdef _WIN32
	if(MyInfoOld != NULL) {
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)MyInfoOld) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate MyInfoOld in ~User! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
    }
#else
	free(MyInfoOld);
#endif
        
	if(MyInfoTag != NULL) {
		UserClearMyINFOTag(this);
    }

#ifdef _WIN32
	if(Version != NULL) {
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)Version) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate Version in ~User! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
    }
#else
	free(Version);
#endif
        
	if(((ui32BoolBits & User::BIT_SUPPORT_ZPIPE) == User::BIT_SUPPORT_ZPIPE) == true)
        DcCommands->iStatZPipe--;

	ui32Parts++;

#ifdef _BUILD_GUI
    if(::SendMessage(pMainWindowPageUsersChat->hWndPageItems[MainWindowPageUsersChat::BTN_SHOW_COMMANDS], BM_GETCHECK, 0, 0) == BST_CHECKED) {
        RichEditAppendText(pMainWindowPageUsersChat->hWndPageItems[MainWindowPageUsersChat::REDT_CHAT], ("x User removed: " + string(Nick, NickLen) + " (Socket " + string(Sck) + ")").c_str());
    }
#endif

	if(Nick != sDefaultNick) {
#ifdef _WIN32
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)Nick) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate Nick in ~User! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
		}
#else
		free(Nick);
#endif
	}
        
	delete uLogInOut;
    
	if(cmdASearch != NULL) {
#ifdef _WIN32
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cmdASearch->sCommand) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate cmdASearch->sCommand in ~User! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(cmdASearch->sCommand);
#endif
		cmdASearch->sCommand = NULL;

		delete cmdASearch;
		cmdASearch = NULL;
    }
       
	if(cmdPSearch != NULL) {
#ifdef _WIN32
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cmdPSearch->sCommand) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate cmdPSearch->sCommand in ~User! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
            AppendSpecialLog(sDbgstr);
        }
#else
		free(cmdPSearch->sCommand);
#endif
		cmdPSearch->sCommand = NULL;

		delete cmdPSearch;
		cmdPSearch = NULL;
    }
                
	PrcsdUsrCmd *next = cmdStrt;
        
    while(next != NULL) {
        PrcsdUsrCmd *cur = next;
        next = cur->next;

#ifdef _WIN32
		if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->sCommand) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate cur->sCommand in ~User! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
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
			string sDbgstr = "[BUF] Cannot deallocate curto->sCommand in ~User! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(curto->sCommand);
#endif
        curto->sCommand = NULL;

#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)curto->ToNick) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate curto->ToNick in !User! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
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

void UserMakeLock(User * u) {
    size_t iAllignLen = Allign1024(63);
#ifdef _WIN32
    u->sendbuf = (char *) HeapAlloc(hSendHeap, HEAP_NO_SERIALIZE, iAllignLen);
#else
	u->sendbuf = (char *) malloc(iAllignLen);
#endif
    if(u->sendbuf == NULL) {              
		string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") Cannot allocate 63/"+string((uint64_t)iAllignLen)+
			" bytes of memory for new sendbuf in UserPutInSendBuf!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hSendHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        u->sbdatalen = 0;
        return;
    }
    u->sendbuflen = (uint32_t)(iAllignLen-1);
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

	for(uint8_t i = 22; i < 49; i++) {
#ifdef _WIN32
	#ifndef _MSC_VER
			u->sendbuf[i] = (char)((random(1000) % 74) + 48);
	#else
			u->sendbuf[i] = (char)((rand() % 74) + 48);
	#endif
#else
	u->sendbuf[i] = (char)((random() % 74) + 48);
#endif
	}

//	Memo(string(u->sendbuf, u->sbdatalen));

#ifdef _WIN32
    u->uLogInOut->sLockUsrConn = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, 64);
#else
	u->uLogInOut->sLockUsrConn = (char *) malloc(64);
#endif
    if(u->uLogInOut->sLockUsrConn == NULL) {
		string sDbgstr = "[BUF] Cannot allocate 64 bytes of memory for sLockUsrConn in UserMakeLock!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
		return;
    }
    
    memcpy(u->uLogInOut->sLockUsrConn, u->sendbuf, 63);
    u->uLogInOut->sLockUsrConn[63] = '\0';
}
//---------------------------------------------------------------------------

bool UserDoRecv(User * u) {
    if((u->ui32BoolBits & User::BIT_ERROR) == User::BIT_ERROR || u->iState >= User::STATE_CLOSING)
        return false;

#ifdef _WIN32
	uint32_t ui32bytes = 0;
	if(ioctlsocket(u->Sck, FIONREAD, (unsigned long *)&ui32bytes) == SOCKET_ERROR) {
		int iError = WSAGetLastError();
#else
	int32_t i32bytes = 0;
	if(ioctl(u->Sck, FIONREAD, &i32bytes) == -1) {
#endif
		UdpDebug->Broadcast("[ERR] " + string(u->Nick, u->NickLen) + " (" + string(u->IP, u->ui8IpLen) +
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
#ifdef _WIN32
	if(ui32bytes != 0 && ProfileMan->IsAllowed(u, ProfileManager::NODEFLOODRECV) == false) {
#else
	if(i32bytes != 0 && ProfileMan->IsAllowed(u, ProfileManager::NODEFLOODRECV) == false) {
#endif
        if(SettingManager->iShorts[SETSHORT_MAX_DOWN_ACTION] != 0) {
    		if(u->ui32Recvs == 0) {
    			u->ui64RecvsTick = ui64ActualTick;
            }

#ifdef _WIN32
            u->ui32Recvs += ui32bytes;
#else
			u->ui32Recvs += i32bytes;
#endif

			if(DeFloodCheckForDataFlood(u, DEFLOOD_MAX_DOWN, SettingManager->iShorts[SETSHORT_MAX_DOWN_ACTION],
			  u->ui32Recvs, u->ui64RecvsTick, SettingManager->iShorts[SETSHORT_MAX_DOWN_KB],
              (uint32_t)SettingManager->iShorts[SETSHORT_MAX_DOWN_TIME]) == true) {
				return false;
            }

    		if(u->ui32Recvs != 0) {
#ifdef _WIN32
                u->ui32Recvs -= ui32bytes;
#else
				u->ui32Recvs -= i32bytes;
#endif
            }
        }

        if(SettingManager->iShorts[SETSHORT_MAX_DOWN_ACTION2] != 0) {
    		if(u->ui32Recvs2 == 0) {
    			u->ui64RecvsTick2 = ui64ActualTick;
            }

#ifdef _WIN32
            u->ui32Recvs2 += ui32bytes;
#else
			u->ui32Recvs2 += i32bytes;
#endif

			if(DeFloodCheckForDataFlood(u, DEFLOOD_MAX_DOWN, SettingManager->iShorts[SETSHORT_MAX_DOWN_ACTION2],
			  u->ui32Recvs2, u->ui64RecvsTick2, SettingManager->iShorts[SETSHORT_MAX_DOWN_KB2],
			  (uint32_t)SettingManager->iShorts[SETSHORT_MAX_DOWN_TIME2]) == true) {
                return false;
            }

    		if(u->ui32Recvs2 != 0) {
#ifdef _WIN32
                u->ui32Recvs2 -= ui32bytes;
#else
				u->ui32Recvs2 -= i32bytes;
#endif
            }
        }
    }

#ifdef _WIN32
	if(ui32bytes == 0) {
#else
	if(i32bytes == 0) {
#endif
		// we need to try recv to catch connection error or if user closed connection
#ifdef _WIN32
        ui32bytes = 16;
    } else if(ui32bytes > 16384) {
#else
        i32bytes = 16;
    } else if(i32bytes > 16384) {
#endif
        // receive max. 16384 bytes to receive buffer
#ifdef _WIN32
        ui32bytes = 16384;
#else
		i32bytes = 16384;
#endif
    }

    if(u->recvbuf == NULL) {
#ifdef _WIN32
        size_t iAllignLen = Allign512(ui32bytes);
    	u->recvbuf = (char *) HeapAlloc(hRecvHeap, HEAP_NO_SERIALIZE, iAllignLen);
#else
        size_t iAllignLen = Allign512(i32bytes);
    	u->recvbuf = (char *) malloc(iAllignLen);
#endif
        if(u->recvbuf == NULL) {
#ifdef _WIN32
			string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") Cannot allocate "+string(ui32bytes)+"/"+string((uint64_t)iAllignLen)+
				" bytes of memory for new recvbuf in UserDoRecv! "+string(HeapValidate(hRecvHeap, HEAP_NO_SERIALIZE, 0))+" "+GetMemStat();
#else
			string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") Cannot allocate "+string(i32bytes)+"/"+string((uint64_t)iAllignLen)+
				" bytes of memory for new recvbuf in UserDoRecv!";
#endif
			AppendSpecialLog(sDbgstr);
			u->recvbuflen = 0;
            return false;
        }
        u->recvbuflen = (uint32_t)(iAllignLen-1);
        u->recvbuf[0] = '\0';
#ifdef _WIN32
    } else if(u->recvbuflen < u->rbdatalen+ui32bytes) {
#else
	} else if(u->recvbuflen < u->rbdatalen+i32bytes) {
#endif
        char * oldbuf = u->recvbuf;
        // take care of the recvbuf size
#ifdef _WIN32
        size_t iAllignLen = Allign512(u->rbdatalen+ui32bytes);
        u->recvbuf = (char *) HeapReAlloc(hRecvHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, iAllignLen);
#else
        size_t iAllignLen = Allign512(u->rbdatalen+i32bytes);
        u->recvbuf = (char *) realloc(oldbuf, iAllignLen);
#endif
		if(u->recvbuf == NULL) {
			string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") Cannot reallocate "+
#ifdef _WIN32
                string(ui32bytes)+"/"+string(u->rbdatalen)+"/"+string((uint64_t)iAllignLen)+" bytes of memory for new recvbuf in UserDoRecv1! "+string(HeapValidate(hRecvHeap, HEAP_NO_SERIALIZE, 0))+" "+GetMemStat();
            HeapFree(hRecvHeap, HEAP_NO_SERIALIZE, (void *)oldbuf);
#else
				string(i32bytes)+"/"+string(u->rbdatalen)+"/"+string((uint64_t)iAllignLen)+" bytes of memory for new recvbuf in UserDoRecv1!";
            free(oldbuf);
#endif
			AppendSpecialLog(sDbgstr);
            u->recvbuflen = 0;
			return false;
		}
		u->recvbuflen = (uint32_t)(iAllignLen-1);
	} else if(u->iRecvCalled > 50) {
#ifdef _WIN32
        if(u->recvbuflen > u->rbdatalen+ui32bytes) {
            size_t iAllignLen = Allign512(u->rbdatalen+ui32bytes);
#else
        if(u->recvbuflen > u->rbdatalen+i32bytes) {
            size_t iAllignLen = Allign512(u->rbdatalen+i32bytes);
#endif
            if(u->recvbuflen > iAllignLen-1) {
                char * oldbuf = u->recvbuf;
#ifdef _WIN32
                u->recvbuf = (char *) HeapReAlloc(hRecvHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, iAllignLen);
#else
				u->recvbuf = (char *) realloc(oldbuf, iAllignLen);
#endif
        		if(u->recvbuf == NULL) {
					string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") Cannot reallocate "+
#ifdef _WIN32
                        string(ui32bytes)+"/"+string(u->rbdatalen)+"/"+string((uint64_t)iAllignLen)+" bytes of memory for new recvbuf in UserDoRecv2! "+string(HeapValidate(hRecvHeap, HEAP_NO_SERIALIZE, 0))+" "+GetMemStat();
                    HeapFree(hRecvHeap, HEAP_NO_SERIALIZE, (void *)oldbuf);
#else
						string(i32bytes)+"/"+string(u->rbdatalen)+"/"+string((uint64_t)iAllignLen)+" bytes of memory for new recvbuf in UserDoRecv2!";
                    free(oldbuf);
#endif
					AppendSpecialLog(sDbgstr);
                    u->recvbuflen = 0;
        	        return false;
        	    }
        		u->recvbuflen = (uint32_t)(iAllignLen-1);
                //UdpDebug->Broadcast( "[SYS] "+string(u->Nick, u->NickLen)+" recv buffer shrinked to "+string(u->recvbuflen+1)+" bytes" );
            }
        }
        u->iRecvCalled = 0;
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
			UdpDebug->Broadcast("[ERR] " + string(u->Nick, u->NickLen) + " (" + string(u->IP, u->ui8IpLen) + "): recv() error " + 
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
			Memo("[SYS] recvlen == 0 and iError == "+string(iError)+"! User: "+string(u->Nick, u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+")");
			UdpDebug->Broadcast("[SYS] recvlen == 0 and iError == " + string(iError) + " ! User: " + string(u->Nick, u->NickLen) +
				" (" + string(u->IP, u->ui8IpLen) + ").");
		}

	#ifdef _BUILD_GUI
        if(::SendMessage(pMainWindowPageUsersChat->hWndPageItems[MainWindowPageUsersChat::BTN_SHOW_COMMANDS], BM_GETCHECK, 0, 0) == BST_CHECKED) {
			int iret = sprintf(msg, "- User has closed the connection: %s (%s)", u->Nick, u->IP);
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

void UserSendChar(User * u, const char * cText, const size_t &iTextLen) {
	if(u->iState >= User::STATE_CLOSING)
        return;

    if(((u->ui32BoolBits & User::BIT_SUPPORT_ZPIPE) == User::BIT_SUPPORT_ZPIPE) == false || iTextLen < ZMINDATALEN) {
        if(UserPutInSendBuf(u, cText, iTextLen)) {
            UserTry2Send(u);
        }
    } else {
        uint32_t iLen = 0;
        char *sData = ZlibUtility->CreateZPipe(cText, iTextLen, iLen);
            
        if(iLen == 0) {
            if(UserPutInSendBuf(u, cText, iTextLen)) {
                UserTry2Send(u);
            }
        } else {
            ui64BytesSentSaved += iTextLen-iLen;
            if(UserPutInSendBuf(u, sData, iLen)) {
                UserTry2Send(u);
            }
        }
    }
}
//---------------------------------------------------------------------------

void UserSendCharDelayed(User * u, const char * cText) {
	if(u->iState >= User::STATE_CLOSING) {
        return;
    }
    
    size_t iTxtLen = strlen(cText);

    if(((u->ui32BoolBits & User::BIT_SUPPORT_ZPIPE) == User::BIT_SUPPORT_ZPIPE) == false || iTxtLen < ZMINDATALEN) {
        UserPutInSendBuf(u, cText, iTxtLen);
    } else {
        uint32_t iLen = 0;
        char *sData = ZlibUtility->CreateZPipe(cText, iTxtLen, iLen);
            
        if(iLen == 0) {
            UserPutInSendBuf(u, cText, iTxtLen);
        } else {
            UserPutInSendBuf(u, sData, iLen);
            ui64BytesSentSaved += iTxtLen-iLen;
        }
    }
}
//---------------------------------------------------------------------------

void UserSendCharDelayed(User * u, const char * cText, const size_t &iTextLen) {
	if(u->iState >= User::STATE_CLOSING) {
        return;
    }
        
    if(((u->ui32BoolBits & User::BIT_SUPPORT_ZPIPE) == User::BIT_SUPPORT_ZPIPE) == false || iTextLen < ZMINDATALEN) {
        UserPutInSendBuf(u, cText, iTextLen);
    } else {
        uint32_t iLen = 0;
        char *sPipeData = ZlibUtility->CreateZPipe(cText, iTextLen, iLen);
        
        if(iLen == 0) {
            UserPutInSendBuf(u, cText, iTextLen);
        } else {
            UserPutInSendBuf(u, sPipeData, iLen);
            ui64BytesSentSaved += iTextLen-iLen;
        }
    }
}
//---------------------------------------------------------------------------

void UserSendText(User * u, const string &sText) {
	if(u->iState >= User::STATE_CLOSING) {
        return;
    }
    
	size_t iTxtLen = sText.size();

    if(((u->ui32BoolBits & User::BIT_SUPPORT_ZPIPE) == User::BIT_SUPPORT_ZPIPE) == false || iTxtLen < ZMINDATALEN) {
        if(UserPutInSendBuf(u, sText.c_str(), iTxtLen)) {
            UserTry2Send(u);
        }
    } else {
        uint32_t iLen = 0;
        char *sData = ZlibUtility->CreateZPipe(sText.c_str(), iTxtLen, iLen);
            
        if(iLen == 0) {
            if(UserPutInSendBuf(u, sText.c_str(), iTxtLen)) {
                UserTry2Send(u);
            }
        } else {
            ui64BytesSentSaved += iTxtLen-iLen;
            if(UserPutInSendBuf(u, sData, iLen)) {
                UserTry2Send(u);
            }
        }
    }
}
//---------------------------------------------------------------------------

void UserSendTextDelayed(User * u, const string &sText) {
	if(u->iState >= User::STATE_CLOSING) {
        return;
    }
    
    size_t iTxtLen = sText.size();  
      
    if(((u->ui32BoolBits & User::BIT_SUPPORT_ZPIPE) == User::BIT_SUPPORT_ZPIPE) == false || iTxtLen < ZMINDATALEN) {
        UserPutInSendBuf(u, sText.c_str(), iTxtLen);
    } else {
        uint32_t iLen = 0;
        char *sData = ZlibUtility->CreateZPipe(sText.c_str(), iTxtLen, iLen);
            
        if(iLen == 0) {
            UserPutInSendBuf(u, sText.c_str(), iTxtLen);
        } else {
            UserPutInSendBuf(u, sData, iLen);
            ui64BytesSentSaved += iTxtLen-iLen;
        }
    }
}
//---------------------------------------------------------------------------

void UserSendQueue(User * u, QzBuf * Queue, bool bChckActSr/* = true*/) {
    if(ui8SrCntr == 0) {
        if(bChckActSr == true) {
            if(((u->ui32BoolBits & User::BIT_SUPPORT_ZPIPE) == User::BIT_SUPPORT_ZPIPE) == false) {
                uint32_t iSbLen = u->sbdatalen;
                UserPutInSendBuf(u, Queue->buffer, Queue->len);
                                                                  
                if(u->cmdASearch != NULL) {
                    // PPK ... check if adding of searchs not cause buffer overflow !
                    if(u->sbdatalen <= iSbLen) {
                        iSbLen = 0;
                    }

                    UserRemFromSendBuf(u, u->cmdASearch->sCommand, u->cmdASearch->iLen, iSbLen);

#ifdef _WIN32
                    if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)u->cmdASearch->sCommand) == 0) {
                        string sDbgstr = "[BUF] Cannot deallocate u->cmdASearch->sCommand in UserSendQueue! "+string((uint32_t)GetLastError())+" "+
							string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
						AppendSpecialLog(sDbgstr); 
					}
#else
					free(u->cmdASearch->sCommand);
#endif
                    u->cmdASearch->sCommand = NULL;

                    delete u->cmdASearch;
                    u->cmdASearch = NULL;
                }
            } else {
                if(Queue->zlined == false) {
                    Queue->zlined = true;
                    Queue->zbuffer = ZlibUtility->CreateZPipe(Queue->buffer, Queue->len, Queue->zbuffer,
                        Queue->zlen, Queue->zsize);
                }
                
                if(Queue->zlen != 0 && (u->cmdASearch == NULL || Queue->zlen <= (Queue->len-u->cmdASearch->iLen))) {
                    UserPutInSendBuf(u, Queue->zbuffer, Queue->zlen);
                    ui64BytesSentSaved += Queue->len-Queue->zlen;
                    if(u->cmdASearch != NULL) {
#ifdef _WIN32
                        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)u->cmdASearch->sCommand) == 0) {
                            string sDbgstr = "[BUF] Cannot deallocate u->cmdASearch->sCommand1 in UserSendQueue! "+string((uint32_t)GetLastError())+" "+
								string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
							AppendSpecialLog(sDbgstr);
                        }
#else
						free(u->cmdASearch->sCommand);
#endif
                        u->cmdASearch->sCommand = NULL;

                        delete u->cmdASearch;
                        u->cmdASearch = NULL;
                    }
                } else {
                    uint32_t iSbLen = u->sbdatalen;
                    UserPutInSendBuf(u, Queue->buffer, Queue->len);
                                    
                    if(u->cmdASearch != NULL) {
                        // PPK ... check if adding of searchs not cause buffer overflow !
                        if(u->sbdatalen <= iSbLen) {
                            iSbLen = 0;
                        }

                        UserRemFromSendBuf(u, u->cmdASearch->sCommand, u->cmdASearch->iLen, iSbLen);
#ifdef _WIN32
                        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)u->cmdASearch->sCommand) == 0) {
                            string sDbgstr = "[BUF] Cannot deallocate u->cmdASearch->sCommand2 in UserSendQueue! "+string((uint32_t)GetLastError())+" "+
								string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
							AppendSpecialLog(sDbgstr);
                        }
#else
						free(u->cmdASearch->sCommand);
#endif
                        u->cmdASearch->sCommand = NULL;

                        delete u->cmdASearch;
                        u->cmdASearch = NULL;
                    }
                }
            }
            return;
        } else {
            if(u->cmdASearch != NULL) {
#ifdef _WIN32
                if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)u->cmdASearch->sCommand) == 0) {
                    string sDbgstr = "[BUF] Cannot deallocate u->cmdASearch->sCommand3 in UserSendQueue! "+string((uint32_t)GetLastError())+" "+
						string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
					AppendSpecialLog(sDbgstr);
                }
#else
				free(u->cmdASearch->sCommand);
#endif
                u->cmdASearch->sCommand = NULL;

                delete u->cmdASearch;
                u->cmdASearch = NULL;
            }
        }
    }
    
    if(((u->ui32BoolBits & User::BIT_SUPPORT_ZPIPE) == User::BIT_SUPPORT_ZPIPE) == false) {
        UserPutInSendBuf(u, Queue->buffer, Queue->len);
    } else {
        if(Queue->zlined == false) {
            Queue->zlined = true;
            Queue->zbuffer = ZlibUtility->CreateZPipe(Queue->buffer, Queue->len, Queue->zbuffer,
                Queue->zlen, Queue->zsize);
        }
            
        if(Queue->zlen == 0) {
            UserPutInSendBuf(u, Queue->buffer, Queue->len);
        } else {
            ui64BytesSentSaved += Queue->len-Queue->zlen;
            UserPutInSendBuf(u, Queue->zbuffer, Queue->zlen);
        }
    }
}
//---------------------------------------------------------------------------

bool UserPutInSendBuf(User * u, const char * Text, const size_t &iTxtLen) {
	u->iSendCalled++;

	// if no sendbuf, initialize one with double length of new data
	if(u->sendbuf == NULL) {
        size_t iAllignLen = Allign1024(iTxtLen);
#ifdef _WIN32
    	u->sendbuf = (char *) HeapAlloc(hSendHeap, HEAP_NO_SERIALIZE, iAllignLen);
#else
		u->sendbuf = (char *) malloc(iAllignLen);
#endif
        if(u->sendbuf == NULL) {              
			string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") Cannot allocate "+string((uint64_t)iTxtLen)+"/"+string((uint64_t)iAllignLen)+
				" bytes of memory for new sendbuf in UserPutInSendBuf! "+string(Text, iTxtLen);
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hSendHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            u->sbdatalen = 0;
        	return false;
        }
        u->sendbuflen = (uint32_t)(iAllignLen-1);
	    u->sbplayhead = u->sendbuf;
    } else if(u->sendbuflen < u->sbdatalen+iTxtLen) {
        // resize buffer if needed
		if((size_t)(u->sbplayhead-u->sendbuf) > iTxtLen) {
            uint32_t offset = (uint32_t)(u->sbplayhead-u->sendbuf);
            memmove(u->sendbuf, u->sbplayhead, (u->sbdatalen-offset));
            u->sbplayhead = u->sendbuf;
            u->sbdatalen = u->sbdatalen-offset;
        } else {
            size_t iAllignLen = Allign1024(u->sbdatalen+iTxtLen);
			size_t iMaxBufLen = (size_t)(((u->ui32BoolBits & User::BIT_BIG_SEND_BUFFER) == User::BIT_BIG_SEND_BUFFER) == true ?
                ((colUsers->myInfosTagLen > colUsers->myInfosLen ? colUsers->myInfosTagLen : colUsers->myInfosLen)*2) :
                (colUsers->myInfosTagLen > colUsers->myInfosLen ? colUsers->myInfosTagLen : colUsers->myInfosLen));
            iMaxBufLen = iMaxBufLen < 131072 ? 131072 : iMaxBufLen;
			if(iAllignLen > iMaxBufLen) {
                // does the buffer size reached the maximum
                // PPK ... if we are on maximum but in buffer is empty place for data then use it ! ;o)
                if(SettingManager->bBools[SETBOOL_KEEP_SLOW_USERS] == false) {
                    // we want to drop the slow user
                    u->ui32BoolBits |= User::BIT_ERROR;
                    UserClose(u);

					UdpDebug->Broadcast("[BUF] " + string(u->Nick, u->NickLen) + " (" + string(u->IP, u->ui8IpLen) +
						") SendBuffer overflow (AL:"+string((uint64_t)iAllignLen)+"[SL:"+string(u->sbdatalen)+"|NL:"+
#ifdef _WIN32
						string((uint64_t)iTxtLen)+"|FL:"+string(u->sbplayhead-u->sendbuf)+"]/ML:"+string((uint64_t)iMaxBufLen)+"). User disconnected.");
#else
						string((uint64_t)iTxtLen)+"|FL:"+string((uint64_t)(u->sbplayhead-u->sendbuf))+"]/ML:"+string((uint64_t)iMaxBufLen)+"). User disconnected.");
#endif
            		return false;
                } else {
    				UdpDebug->Broadcast("[BUF] " + string(u->Nick, u->NickLen) + " (" + string(u->IP, u->ui8IpLen) +
    					") SendBuffer overflow (AL:"+string((uint64_t)iAllignLen)+"[SL:"+string(u->sbdatalen)+"|NL:"+
#ifdef _WIN32
                        string((uint64_t)iTxtLen)+"|FL:"+string(u->sbplayhead-u->sendbuf)+"]/ML:"+string((uint64_t)iMaxBufLen)+
#else
						string((uint64_t)iTxtLen)+"|FL:"+string((uint64_t)(u->sbplayhead-u->sendbuf))+"]/ML:"+string((uint64_t)iMaxBufLen)+
#endif
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
					if(iTxtLen < iMaxBufLen && iOldSBDataLen > (uint32_t)((sTemp+1)-u->sendbuf) && (iOldSBDataLen-((sTemp+1)-u->sendbuf)) > (uint32_t)iTxtLen) {
						char *sTemp1;
                        // try to remove min half of send bufer
                        if(iOldSBDataLen > (u->sendbuflen/2) && (uint32_t)((sTemp+1+iTxtLen)-u->sendbuf) < (u->sendbuflen/2)) {
                            sTemp1 = (char *)memchr(u->sendbuf+(u->sendbuflen/2), '|', iOldSBDataLen-(u->sendbuflen/2));
                        } else {
                            sTemp1 = (char *)memchr(sTemp+1+iTxtLen, '|', iOldSBDataLen-((sTemp+1+iTxtLen)-u->sendbuf));
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

                size_t iAllignTxtLen = Allign1024(iTxtLen+u->sbdatalen);

                char * oldbuf = u->sendbuf;
#ifdef _WIN32
                u->sendbuf = (char *) HeapReAlloc(hSendHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, iAllignTxtLen);
#else
				u->sendbuf = (char *) realloc(oldbuf, iAllignTxtLen);
#endif
                if(u->sendbuf == NULL) {
#ifdef _WIN32
					string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") Cannot reallocate "+string(iTxtLen)+"/"+string(u->sbdatalen)+"/"+string(iAllignLen)+
						" bytes of memory in UserPutInSendBuf-keepslow! "+string(HeapValidate(hSendHeap, HEAP_NO_SERIALIZE, 0))+" "+string(Text, iTxtLen)+GetMemStat();
                    HeapFree(hSendHeap, HEAP_NO_SERIALIZE, (void *)oldbuf);
#else
					string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") Cannot reallocate "+
                        string((uint64_t)iTxtLen)+"/"+string(u->sbdatalen)+"/"+string((uint64_t)iAllignLen)+" bytes of memory in UserPutInSendBuf-keepslow! "+string(Text, iTxtLen);
                    free(oldbuf);
#endif
					AppendSpecialLog(sDbgstr);
                    u->sbdatalen = 0;
                    return false;
                }
                u->sendbuflen = (uint32_t)(iAllignTxtLen-1);
                u->sbplayhead = u->sendbuf;
        	} else {
        		uint32_t iOffSet = (uint32_t)(u->sbplayhead-u->sendbuf);

        		char * oldbuf = u->sendbuf;
#ifdef _WIN32
                u->sendbuf = (char *) HeapReAlloc(hSendHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, iAllignLen);
#else
				u->sendbuf = (char *) realloc(oldbuf, iAllignLen);
#endif
        		if(u->sendbuf == NULL) {
					string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") Cannot reallocate "+
						string((uint64_t)iTxtLen)+"/"+string((uint64_t)iAllignLen)+" bytes of memory in UserPutInSendBuf! "+string(Text, iTxtLen);
#ifdef _WIN32
					sDbgstr += " "+string(HeapValidate(hSendHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
					HeapFree(hSendHeap, HEAP_NO_SERIALIZE, (void *)oldbuf);
#else
                    free(oldbuf);
#endif
					AppendSpecialLog(sDbgstr);
                    u->sbdatalen = 0;
        	        return false;
        	    }
        		u->sendbuflen = (uint32_t)(iAllignLen-1);
                u->sbplayhead = u->sendbuf+iOffSet;
            }
        }
    } else if(u->iSendCalled > 100) {
        if(u->sendbuflen > u->sbdatalen+iTxtLen) {
        	size_t iAllignLen = Allign1024(u->sbdatalen+iTxtLen);
            if(u->sendbuflen > iAllignLen-1) {
            	uint32_t offset = (uint32_t)(u->sbplayhead-u->sendbuf);

            	char * oldbuf = u->sendbuf;
#ifdef _WIN32
                u->sendbuf = (char *) HeapReAlloc(hSendHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, iAllignLen);
#else
				u->sendbuf = (char *) realloc(oldbuf, iAllignLen);
#endif
        		if(u->sendbuf == NULL) {
					string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") Cannot reallocate "+
						string((uint64_t)iTxtLen)+"/"+string((uint64_t)iAllignLen)+" bytes of memory for new sendbuf in UserPutInSendBuf1! "+string(Text, iTxtLen);
#ifdef _WIN32
					sDbgstr += " "+string(HeapValidate(hSendHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
					HeapFree(hSendHeap, HEAP_NO_SERIALIZE, (void *)oldbuf);
#else
                    free(oldbuf);
#endif
					AppendSpecialLog(sDbgstr);
                    u->sbdatalen = 0;
        	        return false;
        	    }
        		u->sendbuflen = (uint32_t)(iAllignLen-1);
                u->sbplayhead = u->sendbuf+offset;
                //UdpDebug->Broadcast( "[SYS] "+string(u->Nick, u->NickLen)+" send buffer shrinked to "+string(u->sendbuflen+1)+" bytes" );
            }
        }
        u->iSendCalled = 0;
    }

    // append data to the buffer
    memcpy(u->sendbuf+u->sbdatalen, Text, iTxtLen);
    u->sbdatalen += (uint32_t)iTxtLen;
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
        char msg[128];
#ifdef _WIN32
        int imsgLen = sprintf(msg, "[ERR] Negative send values!\nSendBuf: %p\nPlayHead: %p\nDataLen: %I32d", u->sendbuf, u->sbplayhead, u->sbdatalen);
#else
		int imsgLen = sprintf(msg, "[ERR] Negative send values!\nSendBuf: %p\nPlayHead: %p\nDataLen: %u", u->sendbuf, u->sbplayhead, u->sbdatalen);
#endif
        if(CheckSprintf(imsgLen, 128, "UserTry2Send") == true) {
    		Memo(msg);
    		AppendSpecialLog(msg);
        }
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
			UdpDebug->Broadcast("[ERR] " + string(u->Nick, u->NickLen) + " (" + string(u->IP, u->ui8IpLen) +
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
					string sDbgstr = "[BUF] Cannot deallocate u->sendbuf in UserTry2Send! "+string((uint32_t)GetLastError())+" "+
						string(HeapValidate(hSendHeap, HEAP_NO_SERIALIZE, 0));
					AppendSpecialLog(sDbgstr);
                }
#else
				free(u->sendbuf);
#endif
                u->sendbuf = NULL;
                u->sbplayhead = u->sendbuf;
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

void UserSetIP(User * u, char * newIP) {
    strcpy(u->IP, newIP);
    u->ui8IpLen = (uint8_t)strlen(u->IP);
}
//------------------------------------------------------------------------------

void UserSetNick(User * u, char * newNick, const size_t &iNewNickLen) {
	if(u->Nick != sDefaultNick && u->Nick != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)u->Nick) == 0) {
            string sDbgstr = "[BUF] Cannot deallocate u->Nick in UserSetNick! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(u->Nick);
#endif
        u->Nick = NULL;
    }

#ifdef _WIN32
    u->Nick = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iNewNickLen+1);
#else
	u->Nick = (char *) malloc(iNewNickLen+1);
#endif
    if(u->Nick == NULL) {
		string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") UserSetNick cannot allocate "+string((uint64_t)(iNewNickLen+1))+
			" bytes of memory for Nick! "+string(newNick, iNewNickLen);
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        return;
    }   
    memcpy(u->Nick, newNick, iNewNickLen);
    u->Nick[iNewNickLen] = '\0';
    u->NickLen = (uint32_t)iNewNickLen;
    u->ui32NickHash = HashNick(u->Nick, u->NickLen);
}
//------------------------------------------------------------------------------

void UserSetMyInfoTag(User * u, char * newInfoTag, const size_t &MyInfoTagLen) {
	if(u->MyInfoTag != NULL) {
        if(SettingManager->ui8FullMyINFOOption != 2) {
    	    colUsers->DelFromMyInfosTag(u);
        }
    	
    	UserClearMyINFOTag(u);
    }

#ifdef _WIN32
    u->MyInfoTag = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, MyInfoTagLen+1);
#else
	u->MyInfoTag = (char *) malloc(MyInfoTagLen+1);
#endif
    if(u->MyInfoTag == NULL) {
		string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") UserSetMyInfoTag cannot allocate "+string((uint64_t)(MyInfoTagLen+1))+
			" bytes of memory for MyInfoTag! "+string(newInfoTag, MyInfoTagLen);
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        return;
    }   
    memcpy(u->MyInfoTag, newInfoTag, MyInfoTagLen);
    u->MyInfoTag[MyInfoTagLen] = '\0';
    u->iMyInfoTagLen = (uint32_t)MyInfoTagLen;
    UserParseMyInfo(u);
}
//------------------------------------------------------------------------------

static void UserSetMyInfo(User * u, char * newInfo, const size_t &MyInfoLen) {
	if(u->MyInfo != NULL) {
        if(SettingManager->ui8FullMyINFOOption != 0) {
    	    colUsers->DelFromMyInfos(u);
        }

#ifdef _WIN32    	    
    	if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)u->MyInfo) == 0) {
            string sDbgstr = "[BUF] Cannot deallocate u->MyInfo in UserSetMyInfo! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(u->MyInfo);
#endif
        u->MyInfo = NULL;
    }

#ifdef _WIN32
    u->MyInfo = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, MyInfoLen+1);
#else
	u->MyInfo = (char *) malloc(MyInfoLen+1);
#endif
    if(u->MyInfo == NULL) {
		string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") UserSetMyInfo cannot allocate "+string((uint64_t)(MyInfoLen+1))+
			" bytes of memory for MyInfo! "+string(newInfo, MyInfoLen);
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        return;
    }   
    memcpy(u->MyInfo, newInfo, MyInfoLen);
    u->MyInfo[MyInfoLen] = '\0';
    u->iMyInfoLen = (uint32_t)MyInfoLen;
}
//------------------------------------------------------------------------------

void UserSetVersion(User * u, char * newVer) {
#ifdef _WIN32
	if(u->Version) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)u->Version) == 0) {
            string sDbgstr = "[BUF] Cannot deallocate u->Version in UserSetVersion! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
    }
#else
	free(u->Version);
#endif

    size_t iLen = strlen(newVer);
#ifdef _WIN32
    u->Version = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iLen+1);
#else
	u->Version = (char *) malloc(iLen+1);
#endif
    if(u->Version == NULL) {
		string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") UserSetVersion cannot allocate "+string((uint64_t)(iLen+1))+
			" bytes of memory for Version! "+string(newVer, iLen);
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        return;
    }   
    memcpy(u->Version, newVer, iLen);
    u->Version[iLen] = '\0';
}
//------------------------------------------------------------------------------

void UserSetPasswd(User * u, char * newPass) {
	if(u->uLogInOut->Password != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)u->uLogInOut->Password) == 0) {
            string sDbgstr = "[BUF] Cannot deallocate u->uLogInOut->Password in UserSetPasswd! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(u->uLogInOut->Password);
#endif
        u->uLogInOut->Password = NULL;
    }
        
    size_t iLen = strlen(newPass);
#ifdef _WIN32
    u->uLogInOut->Password = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iLen+1);
#else
	u->uLogInOut->Password = (char *) malloc(iLen+1);
#endif
    if(u->uLogInOut->Password == NULL) {
		string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") UserSetPasswd cannot allocate "+string((uint64_t)(iLen+1))+
			" bytes of memory for Password! "+string(newPass, iLen);
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        return;
    }   
    memcpy(u->uLogInOut->Password, newPass, iLen);
    u->uLogInOut->Password[iLen] = '\0';
}
//------------------------------------------------------------------------------

void UserSetLastChat(User * u, char * newData, const size_t &iLen) {
#ifdef _WIN32
    if(u->sLastChat != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)u->sLastChat) == 0) {
            string sDbgstr = "[BUF] Cannot deallocate u->sLastChat in UserSetLastChat! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
    }
#else
	free(u->sLastChat);
#endif

#ifdef _WIN32
    u->sLastChat = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iLen+1);
#else
	u->sLastChat = (char *) malloc(iLen+1);
#endif
    if(u->sLastChat == NULL) {
		string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") UserSetLastChat cannot allocate "+string((uint64_t)(iLen+1))+
			" bytes of memory for sLastChat! "+string(newData, iLen);
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        return;
    }   
    memcpy(u->sLastChat, newData, iLen);
    u->sLastChat[iLen] = '\0';
    u->ui16SameChatMsgs = 1;
    u->ui64SameChatsTick = ui64ActualTick;
    u->ui16LastChatLen = (uint16_t)iLen;
    u->ui16SameMultiChats = 0;
    u->ui16LastChatLines = 0;
}
//------------------------------------------------------------------------------

void UserSetLastPM(User * u, char * newData, const size_t &iLen) {
#ifdef _WIN32
    if(u->sLastPM != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)u->sLastPM) == 0) {
            string sDbgstr = "[BUF] Cannot deallocate u->sLastPM in UserSetLastPM! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
    }
#else
	free(u->sLastPM);
#endif

#ifdef _WIN32
    u->sLastPM = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iLen+1);
#else
	u->sLastPM = (char *) malloc(iLen+1);
#endif
    if(u->sLastPM == NULL) {
		string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") UserSetLastPM cannot allocate "+string((uint64_t)(iLen+1))+
			" bytes of memory for sLastPM! "+string(newData, iLen);
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        return;
    }

    memcpy(u->sLastPM, newData, iLen);
    u->sLastPM[iLen] = '\0';
    u->ui16SamePMs = 1;
    u->ui64SamePMsTick = ui64ActualTick;
    u->ui16LastPMLen = (uint16_t)iLen;
    u->ui16SameMultiPms = 0;
    u->ui16LastPmLines = 0;
}
//------------------------------------------------------------------------------

void UserSetLastSearch(User * u, char * newData, const size_t &iLen) {
#ifdef _WIN32
    if(u->sLastSearch != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)u->sLastSearch) == 0) {
            string sDbgstr = "[BUF] Cannot deallocate u->sLastSearch in UserSetLastSearch! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
    }
#else
	free(u->sLastSearch);
#endif

#ifdef _WIN32
    u->sLastSearch = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iLen+1);
#else
	u->sLastSearch = (char *) malloc(iLen+1);
#endif
    if(u->sLastSearch == NULL) {
		string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") UserSetLastSearch cannot allocate "+string((uint64_t)(iLen+1))+
			" bytes of memory for sLastSearch! "+string(newData, iLen);
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
        AppendSpecialLog(sDbgstr);
        return;
    }   
    memcpy(u->sLastSearch, newData, iLen);
    u->sLastSearch[iLen] = '\0';
    u->ui16SameSearchs = 1;
    u->ui64SameSearchsTick = ui64ActualTick;
    u->ui16LastSearchLen = (uint16_t)iLen;
}
//------------------------------------------------------------------------------

void UserSetKickMsg(User * u, char * kickmsg, size_t iLen/* = 0*/) {
    if(iLen == 0) {
        iLen = strlen(kickmsg);
    }

    if(u->uLogInOut != NULL) {
        if(u->uLogInOut->sKickMsg != NULL) {
#ifdef _WIN32
            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)u->uLogInOut->sKickMsg) == 0) {
                string sDbgstr = "[BUF] Cannot deallocate u->uLogInOut->sKickMsg in UserSetKickMsg! "+string((uint32_t)GetLastError())+" "+
					string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
				AppendSpecialLog(sDbgstr);
            }
#else
			free(u->uLogInOut->sKickMsg);
#endif
            u->uLogInOut->sKickMsg = NULL;
        }
    } else {
        u->uLogInOut = new LoginLogout();
        if(u->uLogInOut == NULL) {
			string sDbgstr = "[BUF] Cannot allocate new u->uLogInOut!";
#ifdef _WIN32
    		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
    		AppendSpecialLog(sDbgstr);
    		exit(EXIT_FAILURE);
        }
    }

    if(iLen < 256) {
#ifdef _WIN32
        u->uLogInOut->sKickMsg = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iLen+1);
#else
		u->uLogInOut->sKickMsg = (char *) malloc(iLen+1);
#endif
        if(u->uLogInOut->sKickMsg == NULL) {
			string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") UserSetKickMsg cannot allocate "+string((uint64_t)(iLen+1))+
				" bytes of memory for sKickMsg! "+string(kickmsg, iLen);
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            return;
        }
        memcpy(u->uLogInOut->sKickMsg, kickmsg, iLen);
        u->uLogInOut->sKickMsg[iLen] = '\0';
    } else {
#ifdef _WIN32
        u->uLogInOut->sKickMsg = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, 256);
#else
		u->uLogInOut->sKickMsg = (char *) malloc(256);
#endif
        if(u->uLogInOut->sKickMsg == NULL) {
			string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") UserSetKickMsg cannot allocate 256 bytes of memory for sKickMsg1! "+
				string(kickmsg, iLen);
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            return;
        }
        memcpy(u->uLogInOut->sKickMsg, kickmsg, 252);
        u->uLogInOut->sKickMsg[255] = '\0';
        u->uLogInOut->sKickMsg[254] = '.';
        u->uLogInOut->sKickMsg[253] = '.';
        u->uLogInOut->sKickMsg[252] = '.';
    }
}
//------------------------------------------------------------------------------

void UserClose(User * u, bool bNoQuit/* = false*/) {
    if(u->iState >= User::STATE_CLOSING) {
        return;
    }
    
	// nick in hash table ?
	if((u->ui32BoolBits & User::BIT_HASHED) == User::BIT_HASHED) {
    	hashManager->Remove(u);
    }

    // nick in nick/op list ?
    if(u->iState >= User::STATE_ADDME_2LOOP) {  
		colUsers->DelFromNickList(u->Nick, (u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR);
		colUsers->DelFromUserIP(u);

        // PPK ... fix for QuickList nad ghost...
        // and fixing redirect all too ;)
        // and fix disconnect on send error too =)
        if(bNoQuit == false) {         
            int imsgLen = sprintf(msg, "$Quit %s|", u->Nick); 
            if(CheckSprintf(imsgLen, 1024, "UserClose") == true) {
                globalQ->InfoStore(msg, imsgLen);
            }

			colUsers->Add2RecTimes(u);
        }

#ifdef _BUILD_GUI
        if(::SendMessage(pMainWindowPageUsersChat->hWndPageItems[MainWindowPageUsersChat::BTN_AUTO_UPDATE_USERLIST], BM_GETCHECK, 0, 0) == BST_CHECKED) {
            pMainWindowPageUsersChat->RemoveUser(u);
        }
#endif

        //sqldb->FinalizeVisit(u);
		ui32Logged--;

		if(((u->ui32BoolBits & User::BIT_HAVE_SHARECOUNTED) == User::BIT_HAVE_SHARECOUNTED) == true) {
            ui64TotalShare -= u->sharedSize;
            u->ui32BoolBits &= ~User::BIT_HAVE_SHARECOUNTED;
		}

		ScriptManager->UserDisconnected(u);
	}
	
	u->iState = User::STATE_CLOSING;
	
    if(u->cmdASearch != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)u->cmdASearch->sCommand) == 0) {
            string sDbgstr = "[BUF] Cannot deallocate u->cmdASearch->sCommand in UserClose! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(u->cmdASearch->sCommand);
#endif
        u->cmdASearch->sCommand = NULL;

        delete u->cmdASearch;
        u->cmdASearch = NULL;
    }
                        
    if(u->cmdPSearch != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)u->cmdPSearch->sCommand) == 0) {
            string sDbgstr = "[BUF] Cannot deallocate u->cmdPSearch->sCommand in UserClose! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(u->cmdPSearch->sCommand);
#endif
        u->cmdPSearch->sCommand = NULL;

        delete u->cmdPSearch;
        u->cmdPSearch = NULL;
    }
                        
    PrcsdUsrCmd *next = u->cmdStrt;
                        
    while(next != NULL) {
        PrcsdUsrCmd *cur = next;
        next = cur->next;

#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->sCommand) == 0) {
            string sDbgstr = "[BUF] Cannot deallocate cur->sCommand in UserClose! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
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
            string sDbgstr = "[BUF] Cannot deallocate curto->sCommand in UserClose! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(curto->sCommand);
#endif
        curto->sCommand = NULL;

#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)curto->ToNick) == 0) {
            string sDbgstr = "[BUF] Cannot deallocate curto->ToNick in UserClose! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(curto->ToNick);
#endif
        curto->ToNick = NULL;

        delete curto;
	}


    u->cmdToUserStrt = NULL;
    u->cmdToUserEnd = NULL;

    if(u->MyInfoTag) {
    	if(SettingManager->ui8FullMyINFOOption != 2) {
    		colUsers->DelFromMyInfosTag(u);
        }
    }
    
    if(u->MyInfo) {
    	if(SettingManager->ui8FullMyINFOOption != 0) {
    		colUsers->DelFromMyInfos(u);
        }
    }

    if(u->sbdatalen == 0 || (u->ui32BoolBits & User::BIT_ERROR) == User::BIT_ERROR) {
        u->iState = User::STATE_REMME;
    } else {
        if(u->uLogInOut == NULL) {
            u->uLogInOut = new LoginLogout();
            if(u->uLogInOut == NULL) {
				string sDbgstr = "[BUF] Cannot allocate new u->uLogInOut!";
#ifdef _WIN32
        		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
        		AppendSpecialLog(sDbgstr);
        		exit(EXIT_FAILURE);
            }
        }

        u->uLogInOut->iToCloseLoops = 100;
    }
}
//---------------------------------------------------------------------------

void UserAdd2Userlist(User * u) {
    colUsers->Add2NickList(u->Nick, (size_t)u->NickLen, (u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR);
    colUsers->Add2UserIP(u);
    
    switch(SettingManager->ui8FullMyINFOOption) {
        case 0: {
            colUsers->Add2MyInfosTag(u);
            return;
        }
        case 1: {
            colUsers->Add2MyInfosTag(u);
            UserGenerateMyINFO(u);
            colUsers->Add2MyInfos(u);
            return;
        }
        case 2: {
            UserGenerateMyINFO(u);
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

	if(((u->ui32BoolBits & User::BIT_SUPPORT_NOHELLO) == User::BIT_SUPPORT_NOHELLO) == false) {
    	if(ProfileMan->IsAllowed(u, ProfileManager::ALLOWEDOPCHAT) == false || (SettingManager->bBools[SETBOOL_REG_OP_CHAT] == false ||
            (SettingManager->bBools[SETBOOL_REG_BOT] == true && SettingManager->bBotsSameNick == true))) {
            if(((u->ui32BoolBits & User::BIT_SUPPORT_ZPIPE) == User::BIT_SUPPORT_ZPIPE) == false) {
                UserSendCharDelayed(u, colUsers->nickList, colUsers->nickListLen);
            } else {
                if(colUsers->iZNickListLen == 0) {
                    colUsers->sZNickList = ZlibUtility->CreateZPipe(colUsers->nickList, colUsers->nickListLen, colUsers->sZNickList,
                        colUsers->iZNickListLen, colUsers->iZNickListSize, ZLISTSIZE);
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
                    char * oldbuf = colUsers->nickList;
#ifdef _WIN32
                    colUsers->nickList = (char *) HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, colUsers->nickListSize+NICKLISTSIZE+1);
#else
					colUsers->nickList = (char *) realloc(oldbuf, colUsers->nickListSize+NICKLISTSIZE+1);
#endif
                    if(colUsers->nickList == NULL) {
						string sDbgstr = "[BUF] Cannot reallocate "+string(iLen)+"/"+string(colUsers->nickListSize)+"/"+string(colUsers->nickListSize+NICKLISTSIZE+1)+
							" bytes of memory for nickList in UserAddUserList!";
#ifdef _WIN32
						sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
                        HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf);
#else
                        free(oldbuf);
#endif
						AppendSpecialLog(sDbgstr);
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
            if(((u->ui32BoolBits & User::BIT_SUPPORT_ZPIPE) == User::BIT_SUPPORT_ZPIPE) == false) {
                UserSendCharDelayed(u, colUsers->myInfosTag, colUsers->myInfosTagLen);
            } else {
                if(colUsers->iZMyInfosTagLen == 0) {
                    colUsers->sZMyInfosTag = ZlibUtility->CreateZPipe(colUsers->myInfosTag, colUsers->myInfosTagLen, colUsers->sZMyInfosTag,
                        colUsers->iZMyInfosTagLen, colUsers->iZMyInfosTagSize, ZMYINFOLISTSIZE);
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
                if(((u->ui32BoolBits & User::BIT_SUPPORT_ZPIPE) == User::BIT_SUPPORT_ZPIPE) == false) {
                    UserSendCharDelayed(u, colUsers->myInfos, colUsers->myInfosLen);
                } else {
                    if(colUsers->iZMyInfosLen == 0) {
                        colUsers->sZMyInfos = ZlibUtility->CreateZPipe(colUsers->myInfos, colUsers->myInfosLen, colUsers->sZMyInfos,
                            colUsers->iZMyInfosLen, colUsers->iZMyInfosSize, ZMYINFOLISTSIZE);
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
                if(((u->ui32BoolBits & User::BIT_SUPPORT_ZPIPE) == User::BIT_SUPPORT_ZPIPE) == false) {
                    UserSendCharDelayed(u, colUsers->myInfosTag, colUsers->myInfosTagLen);
                } else {
                    if(colUsers->iZMyInfosTagLen == 0) {
                        colUsers->sZMyInfosTag = ZlibUtility->CreateZPipe(colUsers->myInfosTag, colUsers->myInfosTagLen, colUsers->sZMyInfosTag,
                            colUsers->iZMyInfosTagLen, colUsers->iZMyInfosTagSize, ZMYINFOLISTSIZE);
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
            if(((u->ui32BoolBits & User::BIT_SUPPORT_ZPIPE) == User::BIT_SUPPORT_ZPIPE) == false) {
                UserSendCharDelayed(u, colUsers->myInfos, colUsers->myInfosLen);
            } else {
                if(colUsers->iZMyInfosLen == 0) {
                    colUsers->sZMyInfos = ZlibUtility->CreateZPipe(colUsers->myInfos, colUsers->myInfosLen, colUsers->sZMyInfos,
                        colUsers->iZMyInfosLen, colUsers->iZMyInfosSize, ZMYINFOLISTSIZE);
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
            if(((u->ui32BoolBits & User::BIT_SUPPORT_ZPIPE) == User::BIT_SUPPORT_ZPIPE) == false) {
                UserSendCharDelayed(u, colUsers->opList, colUsers->opListLen);
            } else {
                if(colUsers->iZOpListLen == 0) {
                    colUsers->sZOpList = ZlibUtility->CreateZPipe(colUsers->opList, colUsers->opListLen, colUsers->sZOpList,
                        colUsers->iZOpListLen, colUsers->iZOpListSize, ZLISTSIZE);
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
                char * oldbuf = colUsers->opList;
#ifdef _WIN32
                colUsers->opList = (char *) HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, colUsers->opListSize+OPLISTSIZE+1);
#else
				colUsers->opList = (char *) realloc(oldbuf, colUsers->opListSize+OPLISTSIZE+1);
#endif
                if(colUsers->opList == NULL) {
					string sDbgstr = "[BUF] Cannot reallocate "+string(iLen)+"/"+string(colUsers->opListSize)+"/"+string(colUsers->opListSize+OPLISTSIZE+1)+
						" bytes of memory for opList in UserAddUserList1!";
#ifdef _WIN32
					sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
                    HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf);
#else
                    free(oldbuf);
#endif
                    AppendSpecialLog(sDbgstr);
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

    if(ProfileMan->IsAllowed(u, ProfileManager::SENDALLUSERIP) == true && ((u->ui32BoolBits & User::BIT_SUPPORT_USERIP2) == User::BIT_SUPPORT_USERIP2) == true) {
        if(colUsers->userIPListLen > 9) {
            if(((u->ui32BoolBits & User::BIT_SUPPORT_ZPIPE) == User::BIT_SUPPORT_ZPIPE) == false) {
                UserSendCharDelayed(u, colUsers->userIPList, colUsers->userIPListLen);
            } else {
                if(colUsers->iZUserIPListLen == 0) {
                    colUsers->sZUserIPList = ZlibUtility->CreateZPipe(colUsers->userIPList, colUsers->userIPListLen, colUsers->sZUserIPList,
                        colUsers->iZUserIPListLen, colUsers->iZUserIPListSize, ZLISTSIZE);
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

void UserClearMyINFOTag(User * u) {
    u->Connection = NULL;
    u->ui8ConnLen = 0;
        
    u->Description = NULL;
    u->ui8DescrLen = 0;

    u->Email = NULL;
    u->ui8EmailLen = 0;

    u->Tag = NULL;
    u->ui8TagLen = 0;

    u->Client = NULL;
    u->ui8ClientLen = 0;

    u->Ver = NULL;
    u->ui8VerLen = 0;

#ifdef _WIN32
    if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)u->MyInfoTag) == 0) {
        string sDbgstr = "[BUF] Cannot deallocate u->MyInfoTag in UserClearMyINFOTag! "+string((uint32_t)GetLastError())+" "+
			string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
		AppendSpecialLog(sDbgstr);
    }
#else
	free(u->MyInfoTag);
#endif
    u->MyInfoTag = NULL;
}
//---------------------------------------------------------------------------

void UserGenerateMyINFO(User *u) {
    // Prepare myinfo with nick
    int iLen = sprintf(msg, "$MyINFO $ALL %s ", u->Nick);
    if(CheckSprintf(iLen, 1024, "UserGenerateMyINFO") == false) {
        return;
    }

    // Add mode to start of description if is enabled
    if(SettingManager->bBools[SETBOOL_MODE_TO_DESCRIPTION] == true && u->Mode != 0) {
        if(SettingManager->bBools[SETBOOL_STRIP_DESCRIPTION] == true || u->Description == NULL || u->Description[0] != u->Mode || u->Description[1] != ' ') {
            msg[iLen] = u->Mode;
            iLen++;
        }

        if(SettingManager->bBools[SETBOOL_STRIP_DESCRIPTION] == false && u->Description != NULL && u->Description[0] != u->Mode && u->Description[1] != ' ') {
            msg[iLen] = ' ';
            iLen++;
        }
    }

    // Add description if is enabled
    if(SettingManager->bBools[SETBOOL_STRIP_DESCRIPTION] == false && u->Description != NULL) {
        memcpy(msg+iLen, u->Description, (size_t)u->ui8DescrLen);
        iLen += (int)u->ui8DescrLen;
    }

    // Add tag if is enabled
    if(SettingManager->bBools[SETBOOL_STRIP_TAG] == false && u->Tag != NULL) {
        memcpy(msg+iLen, u->Tag, (size_t)u->ui8TagLen);
        iLen += (int)u->ui8TagLen;
    }

    // Add mode to myinfo if is enabled
    if(SettingManager->bBools[SETBOOL_MODE_TO_MYINFO] == true && u->Mode != 0) {
        int iRet = sprintf(msg+iLen, "$%c$", u->Mode);
        iLen += iRet;
        if(CheckSprintf1(iRet, iLen, 1024, "UserGenerateMyINFO1") == false) {
            return;
        }
    } else {
        memcpy(msg+iLen, "$ $", 3);
        iLen += 3;
    }

    // Add connection if is enabled
    if(SettingManager->bBools[SETBOOL_STRIP_CONNECTION] == false && u->Connection != NULL) {
        memcpy(msg+iLen, u->Connection, (size_t)u->ui8ConnLen);
        iLen += (int)u->ui8ConnLen;
    }

    // add magicbyte and $
    if(u->MagicByte < 16 || ((u->ui32BoolBits & User::BIT_QUACK_SUPPORTS) == User::BIT_QUACK_SUPPORTS) == false) {
        msg[iLen] = u->MagicByte;
    } else {
        char cMagic = u->MagicByte;

        cMagic &= ~0x10; // TLSv1 support
        cMagic &= ~0x20; // TLSv1 support
        cMagic &= ~0x40; // IPv4 support
        cMagic &= ~0x80; // IPv6 support

        msg[iLen] = cMagic;
    }

    msg[iLen+1] = '$';
    iLen += 2;

    // Add email if is enabled
    if(SettingManager->bBools[SETBOOL_STRIP_EMAIL] == false && u->Email != NULL) {
        memcpy(msg+iLen, u->Email, (size_t)u->ui8EmailLen);
        iLen += (int)u->ui8EmailLen;
    }

    // Add share and end of myinfo
#ifdef _WIN32
    int iRet = sprintf(msg+iLen, "$%I64d$|", u->sharedSize);
#else
	int iRet = sprintf(msg+iLen, "$%" PRIu64 "$|", u->sharedSize);
#endif
    iLen += iRet;
    if(CheckSprintf1(iRet, iLen, 1024, "UserGenerateMyINFO2") == false) {
        return;
    }

    UserSetMyInfo(u, msg, iLen);
}
//---------------------------------------------------------------------------

void UserHasSuspiciousTag(User *curUser) {
	if(SettingManager->bBools[SETBOOL_REPORT_SUSPICIOUS_TAG] == true && SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
		if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
			int imsgLen = sprintf(msg, "%s $<%s> *** %s (%s) %s. %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->Nick, curUser->IP, 
                LanguageManager->sTexts[LAN_HAS_SUSPICIOUS_TAG_CHECK_HIM], LanguageManager->sTexts[LAN_FULL_DESCRIPTION]);
            if(CheckSprintf(imsgLen, 1024, "UserHasSuspiciousTag1") == true) {
                memcpy(msg+imsgLen, curUser->Description, (size_t)curUser->ui8DescrLen);
                imsgLen += (int)curUser->ui8DescrLen;
                msg[imsgLen] = '|';
                QueueDataItem *newItem = globalQ->CreateQueueDataItem(msg, imsgLen+1, NULL, 0, globalqueue::PM2OPS);
                globalQ->SingleItemsStore(newItem);
            }
		} else {
			int imsgLen = sprintf(msg, "<%s> *** %s (%s) %s. %s: ", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->Nick, curUser->IP, 
                LanguageManager->sTexts[LAN_HAS_SUSPICIOUS_TAG_CHECK_HIM], LanguageManager->sTexts[LAN_FULL_DESCRIPTION]);
            if(CheckSprintf(imsgLen, 1024, "UserHasSuspiciousTag2") == true) {
                memcpy(msg+imsgLen, curUser->Description, (size_t)curUser->ui8DescrLen);
                imsgLen += (int)curUser->ui8DescrLen;
                msg[imsgLen] = '|';
                globalQ->OPStore(msg, imsgLen+1);
            }
	   }
    }
    curUser->ui32BoolBits &= ~User::BIT_HAVE_BADTAG;
}
//---------------------------------------------------------------------------

bool UserProcessRules(User * u) {
    // if share limit enabled, check it
    if(ProfileMan->IsAllowed(u, ProfileManager::NOSHARELIMIT) == false) {      
        if((SettingManager->ui64MinShare != 0 && u->sharedSize < SettingManager->ui64MinShare) ||
            (SettingManager->ui64MaxShare != 0 && u->sharedSize > SettingManager->ui64MaxShare)) {
            UserSendChar(u, SettingManager->sPreTexts[SetMan::SETPRETXT_SHARE_LIMIT_MSG],
                SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_SHARE_LIMIT_MSG]);
            //UdpDebug->Broadcast("[SYS] User with low or high share " + u->Nick + " (" + u->IP + ") disconnected.");
            return false;
        }
    }
    
    // no Tag? Apply rule
    if(u->Tag == NULL) {
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

void UserAddPrcsdCmd(User * u, unsigned char cType, char *sCommand, const size_t &iCommandLen, User * to, bool bIsPm/* = false*/) {
    if(cType == PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO) {
        PrcsdToUsrCmd *next = u->cmdToUserStrt;
        while(next != NULL) {
            PrcsdToUsrCmd *cur = next;
            next = cur->next;
            if(cur->To == to) {
                char * oldbuf = cur->sCommand;
#ifdef _WIN32
                cur->sCommand = (char *) HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, cur->iLen+iCommandLen+1);
#else
				cur->sCommand = (char *) realloc(oldbuf, cur->iLen+iCommandLen+1);
#endif
                if(cur->sCommand == NULL) {
					string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") UserAddPrcsdCmd cannot reallocate "+
						string((uint64_t)iCommandLen)+"/"+string(cur->iLen)+"/"+string((uint64_t)(cur->iLen+iCommandLen+1))+" bytes of memory! "+string(sCommand, iCommandLen);
#ifdef _WIN32
					sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
                    HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf);
#else
                    free(oldbuf);
#endif
					AppendSpecialLog(sDbgstr);
                    return;
                }
                memcpy(cur->sCommand+cur->iLen, sCommand, iCommandLen);
                cur->sCommand[cur->iLen+iCommandLen] = '\0';
                cur->iLen += (uint32_t)iCommandLen;
                cur->iPmCount += bIsPm == true ? 1 : 0;
                return;
            }
        }

        PrcsdToUsrCmd *newcmd = new PrcsdToUsrCmd();
        if(newcmd == NULL) {
			string sDbgstr = "[BUF] UserAddPrcsdCmd cannot allocate new newcmd!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
        	return;
        }

#ifdef _WIN32
        newcmd->sCommand = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iCommandLen+1);
#else
		newcmd->sCommand = (char *) malloc(iCommandLen+1);
#endif
        if(newcmd->sCommand == NULL) {
			string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") UserAddPrcsdCmd cannot allocate "+string((uint64_t)(iCommandLen+1))+
				" bytes of memory! "+string(sCommand, iCommandLen);
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            return;
        }

        memcpy(newcmd->sCommand, sCommand, iCommandLen);
        newcmd->sCommand[iCommandLen] = '\0';

        newcmd->iLen = (uint32_t)iCommandLen;
        newcmd->iPmCount = bIsPm == true ? 1 : 0;
        newcmd->iLoops = 0;
        newcmd->To = to;
        
#ifdef _WIN32
        newcmd->ToNick = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, to->NickLen+1);
#else
		newcmd->ToNick = (char *) malloc(to->NickLen+1);
#endif
        if(newcmd->ToNick == NULL) {
			string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") UserAddPrcsdCmd cannot allocate "+string(to->NickLen+1)+
				" bytes of memory! "+string(to->Nick, to->NickLen);
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            return;
        }   

        memcpy(newcmd->ToNick, to->Nick, to->NickLen);
        newcmd->ToNick[to->NickLen] = '\0';
        
        newcmd->iToNickLen = to->NickLen;
        newcmd->next = NULL;
               
        if(u->cmdToUserStrt == NULL) {
            u->cmdToUserStrt = newcmd;
            u->cmdToUserEnd = newcmd;
        } else {
            u->cmdToUserEnd->next = newcmd;
            u->cmdToUserEnd = newcmd;
        }
        return;
    }
    
    PrcsdUsrCmd *newcmd = new PrcsdUsrCmd();
    if(newcmd == NULL) {
		string sDbgstr = "[BUF] UserAddPrcsdCmd cannot allocate new newcmd1!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
    	return;
    }

#ifdef _WIN32
    newcmd->sCommand = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iCommandLen+1);
#else
	newcmd->sCommand = (char *) malloc(iCommandLen+1);
#endif
    if(newcmd->sCommand == NULL) {
		string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") UserAddPrcsdCmd cannot allocate "+string((uint64_t)(iCommandLen+1))+
			" bytes of memory! "+string(sCommand, iCommandLen);
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        return;
    }

    memcpy(newcmd->sCommand, sCommand, iCommandLen);
    newcmd->sCommand[iCommandLen] = '\0';

    newcmd->iLen = (uint32_t)iCommandLen;
    newcmd->cType = cType;
    newcmd->next = NULL;

    if(u->cmdStrt == NULL) {
        u->cmdStrt = newcmd;
        u->cmdEnd = newcmd;
    } else {
        u->cmdEnd->next = newcmd;
        u->cmdEnd = newcmd;
    }
}
//---------------------------------------------------------------------------
