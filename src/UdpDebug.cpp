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
#include "UdpDebug.h"
//---------------------------------------------------------------------------
#include "LanguageManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
clsUdpDebug *UdpDebug = NULL;
//---------------------------------------------------------------------------

clsUdpDebug::UdpDbgItem::~UdpDbgItem() {
    if(Nick != NULL) {
        free(Nick);
        Nick = NULL;
    }

	close(s);
}
//---------------------------------------------------------------------------

clsUdpDebug::clsUdpDebug() {
	llist = NULL;
	ScriptList = NULL;
}
//---------------------------------------------------------------------------

clsUdpDebug::~clsUdpDebug() {
    UdpDbgItem *next = llist;
	while(next != NULL) {
        UdpDbgItem *cur = next;
        next = cur->next;

    	delete cur;
    }
    
    llist = NULL;

    next = ScriptList;
	while(next != NULL) {
        UdpDbgItem *cur = next;
        next = cur->next;

    	delete cur;
    }

    ScriptList = NULL;
}
//---------------------------------------------------------------------------

void clsUdpDebug::Broadcast(const char * msg) {
    Broadcast(msg, strlen(msg));
}
//---------------------------------------------------------------------------

void clsUdpDebug::Broadcast(const char * msg, const size_t &iLen) {
    if(llist == NULL)
        return;

    size_t pktLen = 4+SettingManager->ui16TextsLens[SETTXT_HUB_NAME]+iLen;

    char *full = (char *) malloc(pktLen);
    if(full == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)pktLen)+
			" bytes of memory in clsUdpDebug::Broadcast!";
        AppendSpecialLog(sDbgstr);
		return;
    }

    // create packet
    ((uint16_t *)full)[0] = (uint16_t)SettingManager->ui16TextsLens[SETTXT_HUB_NAME];
    ((uint16_t *)full)[1] = (uint16_t)iLen;
    memcpy(full+4, SettingManager->sTexts[SETTXT_HUB_NAME], (size_t)SettingManager->ui16TextsLens[SETTXT_HUB_NAME]);
    memcpy(full+4+SettingManager->ui16TextsLens[SETTXT_HUB_NAME], msg, iLen);
     
    UdpDbgItem *next = llist;
	while(next != NULL) {
        UdpDbgItem *cur = next;
        next = cur->next;
    	sendto(cur->s, full, pktLen, 0, (sockaddr *)&cur->to, sizeof(cur->to));
        ui64BytesSent += pktLen;
    }
    
    free(full);
}
//---------------------------------------------------------------------------

void clsUdpDebug::Broadcast(const string & msg) {
    Broadcast(msg.c_str(), msg.size());
}
//---------------------------------------------------------------------------

bool clsUdpDebug::New(User * u, const int32_t &port) {
	UdpDbgItem *NewDbg = new UdpDbgItem();
    if(NewDbg == NULL) {
		string sDbgstr = "[BUF] Cannot allocate NewDbg in clsUdpDebug::New!";
		AppendSpecialLog(sDbgstr);
    	return false;
    }

    // initialize dbg item       
    NewDbg->Nick = (char *) malloc(u->NickLen+1);
    if(NewDbg->Nick == NULL) {
		string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") Cannot allocate "+string(u->NickLen+1)+
			" bytes of memory for Nick in clsUdpDebug::New!";
		AppendSpecialLog(sDbgstr);
        return false;
    }
        
    memcpy(NewDbg->Nick, u->Nick, u->NickLen);
    NewDbg->Nick[u->NickLen] = '\0';
        
    NewDbg->ui32Hash = u->ui32NickHash;
        
    NewDbg->to.sin_family = AF_INET;
    NewDbg->to.sin_port = htons((unsigned short)port);
    NewDbg->to.sin_addr.s_addr = inet_addr(u->IP);
        
    NewDbg->s = socket(AF_INET, SOCK_DGRAM, 0);
        
    if(NewDbg->s == -1) {
		string Txt = "*** [ERR] "+string(LanguageManager->sTexts[LAN_UDP_SCK_CREATE_ERR], (size_t)LanguageManager->ui16TextsLens[LAN_UDP_SCK_CREATE_ERR])+
			": "+string(errno);
        UserSendTextDelayed(u, Txt);
        delete NewDbg;
        return false;
    }
        
    // set non-blocking
    int oldFlag = fcntl(NewDbg->s, F_GETFL, 0);
    if(fcntl(NewDbg->s, F_SETFL, oldFlag | O_NONBLOCK) == -1) {
		string Txt = "*** [ERR] "+string(LanguageManager->sTexts[LAN_UDP_NON_BLOCK_FAIL], (size_t)LanguageManager->ui16TextsLens[LAN_UDP_NON_BLOCK_FAIL])+
			": "+string(errno);
		UserSendTextDelayed(u, Txt);
        delete NewDbg;
        return false;
    }
        
    NewDbg->prev = NULL;
    NewDbg->next = NULL;

    if(llist == NULL){
        llist = NewDbg;
    } else {
        llist->prev = NewDbg;
        NewDbg->next = llist;
        llist = NewDbg;
    }

    char msg[128];
    int iLen = sprintf(msg, "[HUB] Subscribed, users online: %u", ui32Logged);
    if(CheckSprintf(iLen, 128, "clsUdpDebug::New") == true) {
	    size_t pktLen = 4+SettingManager->ui16TextsLens[SETTXT_HUB_NAME]+iLen;
	
	    char *full = (char *) malloc(pktLen);
	    if(full == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)pktLen)+
				" bytes of memory in clsUdpDebug::New1!";
			AppendSpecialLog(sDbgstr);
			return false;
	    }
	
	    // create packet
	    ((uint16_t *)full)[0] = (uint16_t)SettingManager->ui16TextsLens[SETTXT_HUB_NAME];
	    ((uint16_t *)full)[1] = (uint16_t)iLen;
	    memcpy(full+4, SettingManager->sTexts[SETTXT_HUB_NAME], SettingManager->ui16TextsLens[SETTXT_HUB_NAME]);
	    memcpy(full+4+SettingManager->ui16TextsLens[SETTXT_HUB_NAME], msg, iLen);
	
		sendto(NewDbg->s, full, pktLen, 0, (sockaddr *)&NewDbg->to, sizeof(NewDbg->to));
        ui64BytesSent += iLen;
		
	    free(full);
    }

    NewDbg->bIsScript = false;

    return true;
}
//---------------------------------------------------------------------------

bool clsUdpDebug::New(char * sIP, const uint16_t &usPort, const bool &bAllData, char * sScriptName) {
	UdpDbgItem *NewDbg = new UdpDbgItem();
    if(NewDbg == NULL) {
		string sDbgstr = "[BUF] Cannot allocate NewDbg in clsUdpDebug::New!";
		AppendSpecialLog(sDbgstr);
    	return false;
    }

    // initialize dbg item
    size_t iScriptLen = strlen(sScriptName);
    NewDbg->Nick = (char *) malloc(iScriptLen+1);
    if(NewDbg->Nick == NULL) {
		string sDbgstr = "[BUF] "+string(sScriptName)+" Cannot allocate "+string((uint64_t)(iScriptLen+1))+
            " bytes of memory for Nick in clsUdpDebug::New!";
		AppendSpecialLog(sDbgstr);
        delete NewDbg;
        return false;
    }
        
	memcpy(NewDbg->Nick, sScriptName, iScriptLen);
    NewDbg->Nick[iScriptLen] = '\0';
        
    NewDbg->ui32Hash = 0;
        
    NewDbg->to.sin_family = AF_INET;
    NewDbg->to.sin_port = htons(usPort);
    NewDbg->to.sin_addr.s_addr = inet_addr(sIP);
        
    NewDbg->s = socket(AF_INET, SOCK_DGRAM, 0);
        
    if(NewDbg->s == -1) {
		string sDbgstr = "*** [ERR] "+string(LanguageManager->sTexts[LAN_UDP_SCK_CREATE_ERR], (size_t)LanguageManager->ui16TextsLens[LAN_UDP_SCK_CREATE_ERR])+
			": "+string(errno);
        AppendSpecialLog(sDbgstr);
        delete NewDbg;
        return false;
    }
        
    // set non-blocking
    int oldFlag = fcntl(NewDbg->s, F_GETFL, 0);
    if(fcntl(NewDbg->s, F_SETFL, oldFlag | O_NONBLOCK) == -1) {
		string sDbgstr = "*** [ERR] "+string(LanguageManager->sTexts[LAN_UDP_NON_BLOCK_FAIL], (size_t)LanguageManager->ui16TextsLens[LAN_UDP_NON_BLOCK_FAIL])+
			": "+string(errno);
		AppendSpecialLog(sDbgstr);
        delete NewDbg;
        return false;
    }
        
    NewDbg->prev = NULL;
    NewDbg->next = NULL;

    if(bAllData == true) {
        if(llist == NULL){
            llist = NewDbg;
        } else {
            llist->prev = NewDbg;
            NewDbg->next = llist;
            llist = NewDbg;
        }
    } else {
        if(ScriptList == NULL){
            ScriptList = NewDbg;
        } else {
            ScriptList->prev = NewDbg;
            NewDbg->next = ScriptList;
            ScriptList = NewDbg;
        }
    }

    char msg[128];
    int iLen = sprintf(msg, "[HUB] Subscribed, users online: %u", ui32Logged);
    if(CheckSprintf(iLen, 128, "clsUdpDebug::New") == true) {
	    size_t pktLen = 4+SettingManager->ui16TextsLens[SETTXT_HUB_NAME]+iLen;
	    if(bAllData == false) {
            pktLen += iScriptLen+2;
        }
	
	    char *full = (char *) malloc(pktLen);
	    if(full == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)pktLen)+
				" bytes of memory in clsUdpDebug::New1!";
			AppendSpecialLog(sDbgstr);
			return false;
	    }
	
	    // create packet
	    if(bAllData == true) {
	       ((uint16_t *)full)[0] = (uint16_t)SettingManager->ui16TextsLens[SETTXT_HUB_NAME];
        } else {
            ((uint16_t *)full)[0] = (uint16_t)(SettingManager->ui16TextsLens[SETTXT_HUB_NAME]+iScriptLen+2);
        }

	    ((uint16_t *)full)[1] = (uint16_t)iLen;

	    memcpy(full+4, SettingManager->sTexts[SETTXT_HUB_NAME], SettingManager->ui16TextsLens[SETTXT_HUB_NAME]);
	    if(bAllData == false) {
			full[4+SettingManager->ui16TextsLens[SETTXT_HUB_NAME]] = '[';
            memcpy(full+4+SettingManager->ui16TextsLens[SETTXT_HUB_NAME]+1, sScriptName, iScriptLen);
			full[4+SettingManager->ui16TextsLens[SETTXT_HUB_NAME]+1+iScriptLen] = ']';
            memcpy(full+4+SettingManager->ui16TextsLens[SETTXT_HUB_NAME]+iScriptLen+2, msg, iLen);
        } else {
	       memcpy(full+4+SettingManager->ui16TextsLens[SETTXT_HUB_NAME], msg, iLen);
        }
	
		sendto(NewDbg->s, full, pktLen, 0, (sockaddr *)&NewDbg->to, sizeof(NewDbg->to));
        ui64BytesSent += iLen;
		
	    free(full);
    }

    NewDbg->bIsScript = true;

    return true;
}
//---------------------------------------------------------------------------

bool clsUdpDebug::Remove(User * u) {
    UdpDbgItem *next = llist;
	while(next != NULL) {
        UdpDbgItem *cur = next;
        next = cur->next;
    	if(cur->bIsScript == false && cur->ui32Hash == u->ui32NickHash && strcasecmp(cur->Nick, u->Nick) == 0) {
            if(cur->prev == NULL) {
                if(cur->next == NULL) {
                    llist = NULL;
                } else {
                    cur->next->prev = NULL;
                    llist = cur->next;
                }
            } else if(cur->next == NULL) {
                cur->prev->next = NULL;
            } else {
                cur->prev->next = cur->next;
                cur->next->prev = cur->prev;
            }
            
	        delete cur;
            return true;
        }
    }
    return false;
}
//---------------------------------------------------------------------------

void clsUdpDebug::Remove(char * sScriptName) {
    UdpDbgItem *next = ScriptList;
	while(next != NULL) {
        UdpDbgItem *cur = next;
        next = cur->next;

    	if(strcasecmp(cur->Nick, sScriptName) == 0) {
            if(cur->prev == NULL) {
                if(cur->next == NULL) {
                    llist = NULL;
                } else {
                    cur->next->prev = NULL;
                    llist = cur->next;
                }
            } else if(cur->next == NULL) {
                cur->prev->next = NULL;
            } else {
                cur->prev->next = cur->next;
                cur->next->prev = cur->prev;
            }
            
	        delete cur;
            return;
        }
    }

    next = llist;
	while(next != NULL) {
        UdpDbgItem *cur = next;
        next = cur->next;

    	if(cur->bIsScript == true && strcasecmp(cur->Nick, sScriptName) == 0) {
            if(cur->prev == NULL) {
                if(cur->next == NULL) {
                    llist = NULL;
                } else {
                    cur->next->prev = NULL;
                    llist = cur->next;
                }
            } else if(cur->next == NULL) {
                cur->prev->next = NULL;
            } else {
                cur->prev->next = cur->next;
                cur->next->prev = cur->prev;
            }
            
	        delete cur;
            return;
        }
    }
}
//---------------------------------------------------------------------------

bool clsUdpDebug::CheckUdpSub(User * u, bool bSndMess/* = false*/) {
    UdpDbgItem *next = llist;
	while(next != NULL) {
        UdpDbgItem *cur = next;
        next = cur->next;
        if(cur->bIsScript == false && cur->ui32Hash == u->ui32NickHash && strcasecmp(cur->Nick, u->Nick) == 0) {
            if(bSndMess == true) {
				string Txt = "<"+string(SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], (size_t)SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_HUB_SEC])+
					"> *** "+string(LanguageManager->sTexts[LAN_YOU_SUBSCRIBED_UDP_DBG], (size_t)LanguageManager->ui16TextsLens[LAN_YOU_SUBSCRIBED_UDP_DBG])+
					" "+string(ntohs(cur->to.sin_port))+". "+
                    string(LanguageManager->sTexts[LAN_TO_UNSUB_UDP_DBG], (size_t)LanguageManager->ui16TextsLens[LAN_TO_UNSUB_UDP_DBG])+".|";
				UserSendTextDelayed(u, Txt);
            }
                
            return true;
        }
    }
    return false;
}
//---------------------------------------------------------------------------

void clsUdpDebug::Send(char * sScriptName, char * sMsg, const size_t &iLen) {
    UdpDbgItem *next = ScriptList;
	while(next != NULL) {
        UdpDbgItem *cur = next;
        next = cur->next;
        if(strcasecmp(cur->Nick, sScriptName) == 0) {
            size_t iScriptLen = strlen(sScriptName);
            size_t pktLen = 4+SettingManager->ui16TextsLens[SETTXT_HUB_NAME]+iScriptLen+2+iLen;
        
            char *full = (char *) malloc(pktLen);
            if(full == NULL) {
				string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)pktLen)+
					" bytes of memory in clsUdpDebug::Send!";
				AppendSpecialLog(sDbgstr);
        		return;
            }
        
            // create packet
			((uint16_t *)full)[0] = (uint16_t)(SettingManager->ui16TextsLens[SETTXT_HUB_NAME]+iScriptLen+2);
            ((uint16_t *)full)[1] = (uint16_t)iLen;

            memcpy(full+4, SettingManager->sTexts[SETTXT_HUB_NAME], (size_t)SettingManager->ui16TextsLens[SETTXT_HUB_NAME]);
          
			full[4+SettingManager->ui16TextsLens[SETTXT_HUB_NAME]] = '[';
            memcpy(full+4+SettingManager->ui16TextsLens[SETTXT_HUB_NAME]+1, sScriptName, iScriptLen);
			full[4+SettingManager->ui16TextsLens[SETTXT_HUB_NAME]+1+iScriptLen] = ']';

            memcpy(full+4+SettingManager->ui16TextsLens[SETTXT_HUB_NAME]+iScriptLen+2, sMsg, iLen);
             
            sendto(cur->s, full, pktLen, 0, (sockaddr *)&cur->to, sizeof(cur->to));
            ui64BytesSent += pktLen;
            
            free(full);

        	return;
        }
    }
}
//---------------------------------------------------------------------------

void clsUdpDebug::Cleanup() {
    UdpDbgItem *next = llist;
	while(next != NULL) {
        UdpDbgItem *cur = next;
        next = cur->next;
    	delete cur;
    }

    llist = NULL;

    next = ScriptList;
	while(next != NULL) {
        UdpDbgItem *cur = next;
        next = cur->next;
    	delete cur;
    }

    ScriptList = NULL;
}
//---------------------------------------------------------------------------
