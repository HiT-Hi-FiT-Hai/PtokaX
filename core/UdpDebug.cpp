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
#include "UdpDebug.h"
//---------------------------------------------------------------------------
#include "LanguageManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
//---------------------------------------------------------------------------
	#ifndef _MSC_VER
		#pragma package(smart_init)
	#endif
#endif
//---------------------------------------------------------------------------
clsUdpDebug *UdpDebug = NULL;
//---------------------------------------------------------------------------

clsUdpDebug::UdpDbgItem::UdpDbgItem() {
#ifdef _WIN32
    s = INVALID_SOCKET;
#else
    s = -1;
#endif
    memset(&sas_to, 0, sizeof(sockaddr_storage));
    sas_len = 0;

    prev = NULL;
    next = NULL;

    Nick = NULL;

    ui32Hash = 0;

    bIsScript = false;
}
//---------------------------------------------------------------------------

clsUdpDebug::UdpDbgItem::~UdpDbgItem() {
#ifdef _WIN32
    if(Nick != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)Nick) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate Nick in ~UdpDbgItem! "+string((uint32_t)GetLastError())+" "+
                string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
            AppendSpecialLog(sDbgstr);
        }
    }
#else
	free(Nick);
#endif

#ifdef _WIN32
	closesocket(s);
#else
	close(s);
#endif
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

#ifdef _WIN32
    char *full = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, pktLen);
#else
	char *full = (char *) malloc(pktLen);
#endif
    if(full == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)pktLen)+
			" bytes of memory in clsUdpDebug::Broadcast!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#else
#endif
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
#ifdef _WIN32
    	sendto(cur->s, full, (int)pktLen, 0, (struct sockaddr *)&cur->sas_to, cur->sas_len);
#else
		sendto(cur->s, full, pktLen, 0, (struct sockaddr *)&cur->sas_to, cur->sas_len);
#endif
        ui64BytesSent += pktLen;
    }

#ifdef _WIN32
    if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)full) == 0) {
		string sDbgstr = "[BUF] Cannot deallocate full in clsUdpDebug::Broadcast! "+string((uint32_t)GetLastError())+" "+
			string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
		AppendSpecialLog(sDbgstr);
	}
#else
	free(full);
#endif
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
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
    	return false;
    }

    // initialize dbg item
#ifdef _WIN32
    NewDbg->Nick = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, u->ui8NickLen+1);
#else
	NewDbg->Nick = (char *) malloc(u->ui8NickLen+1);
#endif
    if(NewDbg->Nick == NULL) {
		string sDbgstr = "[BUF] "+string(u->sNick, u->ui8NickLen)+" ("+string(u->sIP, u->ui8IpLen)+") Cannot allocate "+string(u->ui8NickLen+1)+
			" bytes of memory for Nick in clsUdpDebug::New!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        return false;
    }

    memcpy(NewDbg->Nick, u->sNick, u->ui8NickLen);
    NewDbg->Nick[u->ui8NickLen] = '\0';

    NewDbg->ui32Hash = u->ui32NickHash;

    bool bIPv6 = (IN6_IS_ADDR_V4MAPPED((in6_addr *)u->ui128IpHash) == 0);

    if(bIPv6 == true) {
        ((struct sockaddr_in6 *)&NewDbg->sas_to)->sin6_family = AF_INET6;
        ((struct sockaddr_in6 *)&NewDbg->sas_to)->sin6_port = htons((unsigned short)port);
        memcpy(((struct sockaddr_in6 *)&NewDbg->sas_to)->sin6_addr.s6_addr, u->ui128IpHash, 16);
        NewDbg->sas_len = sizeof(struct sockaddr_in6);
    } else {
        ((struct sockaddr_in *)&NewDbg->sas_to)->sin_family = AF_INET;
        ((struct sockaddr_in *)&NewDbg->sas_to)->sin_port = htons((unsigned short)port);
        ((struct sockaddr_in *)&NewDbg->sas_to)->sin_addr.s_addr = inet_addr(u->sIP);
        NewDbg->sas_len = sizeof(struct sockaddr_in);
    }

    NewDbg->s = socket((bIPv6 == true ? AF_INET6 : AF_INET), SOCK_DGRAM, IPPROTO_UDP);
#ifdef _WIN32
    if(NewDbg->s == INVALID_SOCKET) {
        int err = WSAGetLastError();
#else
    if(NewDbg->s == -1) {
#endif
		string Txt = "*** [ERR] "+string(LanguageManager->sTexts[LAN_UDP_SCK_CREATE_ERR], (size_t)LanguageManager->ui16TextsLens[LAN_UDP_SCK_CREATE_ERR])+
#ifdef _WIN32
			": "+string(WSErrorStr(err))+" ("+string(err)+")";
#else
			": "+string(errno);
#endif
        UserSendTextDelayed(u, Txt);
        delete NewDbg;
        return false;
    }

    // set non-blocking
#ifdef _WIN32
    uint32_t block = 1;
	if(SOCKET_ERROR == ioctlsocket(NewDbg->s, FIONBIO, (unsigned long *)&block)) {
        int err = WSAGetLastError();
#else
    int oldFlag = fcntl(NewDbg->s, F_GETFL, 0);
    if(fcntl(NewDbg->s, F_SETFL, oldFlag | O_NONBLOCK) == -1) {
#endif
		string Txt = "*** [ERR] "+string(LanguageManager->sTexts[LAN_UDP_NON_BLOCK_FAIL], (size_t)LanguageManager->ui16TextsLens[LAN_UDP_NON_BLOCK_FAIL])+
#ifdef _WIN32
			": "+string(WSErrorStr(err))+" ("+string(err)+")";
#else
			": "+string(errno);
#endif
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
#ifdef _WIN32
    int iLen = sprintf(msg, "[HUB] Subscribed, users online: %I32d", ui32Logged);
#else
	int iLen = sprintf(msg, "[HUB] Subscribed, users online: %u", ui32Logged);
#endif
    if(CheckSprintf(iLen, 128, "clsUdpDebug::New") == true) {
	    size_t pktLen = 4+SettingManager->ui16TextsLens[SETTXT_HUB_NAME]+iLen;
	
#ifdef _WIN32
	    char *full = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, pktLen);
#else
		char *full = (char *) malloc(pktLen);
#endif
	    if(full == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)pktLen)+
				" bytes of memory in clsUdpDebug::New1!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
			return false;
	    }
	
	    // create packet
	    ((uint16_t *)full)[0] = (uint16_t)SettingManager->ui16TextsLens[SETTXT_HUB_NAME];
	    ((uint16_t *)full)[1] = (uint16_t)iLen;
	    memcpy(full+4, SettingManager->sTexts[SETTXT_HUB_NAME], SettingManager->ui16TextsLens[SETTXT_HUB_NAME]);
	    memcpy(full+4+SettingManager->ui16TextsLens[SETTXT_HUB_NAME], msg, iLen);
	
#ifdef _WIN32
		sendto(NewDbg->s, full, (int)pktLen, 0, (struct sockaddr *)&NewDbg->sas_to, NewDbg->sas_len);
#else
		sendto(NewDbg->s, full, pktLen, 0, (struct sockaddr *)&NewDbg->sas_to, NewDbg->sas_len);
#endif
        ui64BytesSent += iLen;

#ifdef _WIN32		
	    if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)full) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate full in clsUdpDebug::New! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(full);
#endif
    }

    NewDbg->bIsScript = false;

    return true;
}
//---------------------------------------------------------------------------

bool clsUdpDebug::New(char * sIP, const uint16_t &usPort, const bool &bAllData, char * sScriptName) {
	UdpDbgItem *NewDbg = new UdpDbgItem();
    if(NewDbg == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate NewDbg in clsUdpDebug::New!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
    	return false;
    }

    // initialize dbg item
    size_t iScriptLen = strlen(sScriptName);
#ifdef _WIN32
    NewDbg->Nick = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iScriptLen+1);
#else
	NewDbg->Nick = (char *) malloc(iScriptLen+1);
#endif
    if(NewDbg->Nick == NULL) {
		string sDbgstr = "[BUF] "+string(sScriptName)+" Cannot allocate "+string((uint64_t)(iScriptLen+1))+
			" bytes of memory for Nick in clsUdpDebug::New!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        delete NewDbg;
        return false;
    }

	memcpy(NewDbg->Nick, sScriptName, iScriptLen);
    NewDbg->Nick[iScriptLen] = '\0';

    NewDbg->ui32Hash = 0;

    uint8_t ui128IP[16];
    HashIP(sIP, ui128IP);

    bool bIPv6 = (IN6_IS_ADDR_V4MAPPED((in6_addr *)ui128IP) == 0);

    if(bIPv6 == true) {
        ((struct sockaddr_in6 *)&NewDbg->sas_to)->sin6_family = AF_INET6;
        ((struct sockaddr_in6 *)&NewDbg->sas_to)->sin6_port = htons(usPort);
        memcpy(((struct sockaddr_in6 *)&NewDbg->sas_to)->sin6_addr.s6_addr, ui128IP, 16);
        NewDbg->sas_len = sizeof(struct sockaddr_in6);
    } else {
        ((struct sockaddr_in *)&NewDbg->sas_to)->sin_family = AF_INET;
        ((struct sockaddr_in *)&NewDbg->sas_to)->sin_port = htons(usPort);
        memcpy(&((struct sockaddr_in *)&NewDbg->sas_to)->sin_addr.s_addr, ui128IP+12, 4);
        NewDbg->sas_len = sizeof(struct sockaddr_in);
    }

    NewDbg->s = socket((bIPv6 == true ? AF_INET6 : AF_INET), SOCK_DGRAM, IPPROTO_UDP);

#ifdef _WIN32
    if(NewDbg->s == INVALID_SOCKET) {
        int err = WSAGetLastError();
#else
    if(NewDbg->s == -1) {
#endif
		string sDbgstr = "*** [ERR] "+string(LanguageManager->sTexts[LAN_UDP_SCK_CREATE_ERR], (size_t)LanguageManager->ui16TextsLens[LAN_UDP_SCK_CREATE_ERR])+
#ifdef _WIN32
			": "+string(WSErrorStr(err))+" ("+string(err)+")";
#else
			": "+string(errno);
#endif
        AppendSpecialLog(sDbgstr);
        delete NewDbg;
        return false;
    }

    // set non-blocking
#ifdef _WIN32
    uint32_t block = 1;
	if(SOCKET_ERROR == ioctlsocket(NewDbg->s, FIONBIO, (unsigned long *)&block)) {
        int err = WSAGetLastError();
#else
    int oldFlag = fcntl(NewDbg->s, F_GETFL, 0);
    if(fcntl(NewDbg->s, F_SETFL, oldFlag | O_NONBLOCK) == -1) {
#endif
		string sDbgstr = "*** [ERR] "+string(LanguageManager->sTexts[LAN_UDP_NON_BLOCK_FAIL], (size_t)LanguageManager->ui16TextsLens[LAN_UDP_NON_BLOCK_FAIL])+
#ifdef _WIN32
			": "+string(WSErrorStr(err))+" ("+string(err)+")";
#else
			": "+string(errno);
#endif
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
#ifdef _WIN32
    int iLen = sprintf(msg, "[HUB] Subscribed, users online: %I32d", ui32Logged);
#else
	int iLen = sprintf(msg, "[HUB] Subscribed, users online: %u", ui32Logged);
#endif
    if(CheckSprintf(iLen, 128, "clsUdpDebug::New") == true) {
	    size_t pktLen = 4+SettingManager->ui16TextsLens[SETTXT_HUB_NAME]+iLen;
	    if(bAllData == false) {
            pktLen += iScriptLen+2;
        }

#ifdef _WIN32
	    char *full = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, pktLen);
#else
		char *full = (char *) malloc(pktLen);
#endif
	    if(full == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)pktLen)+
				" bytes of memory in clsUdpDebug::New1!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
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

#ifdef _WIN32
		sendto(NewDbg->s, full, (int)pktLen, 0, (struct sockaddr *)&NewDbg->sas_to, NewDbg->sas_len);
#else
		sendto(NewDbg->s, full, pktLen, 0, (struct sockaddr *)&NewDbg->sas_to, NewDbg->sas_len);
#endif
        ui64BytesSent += iLen;

#ifdef _WIN32
	    if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)full) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate full in clsUdpDebug::New! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(full);
#endif
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
#ifdef _WIN32
    	if(cur->bIsScript == false && cur->ui32Hash == u->ui32NickHash && stricmp(cur->Nick, u->sNick) == 0) {
#else
		if(cur->bIsScript == false && cur->ui32Hash == u->ui32NickHash && strcasecmp(cur->Nick, u->sNick) == 0) {
#endif
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

#ifdef _WIN32
    	if(stricmp(cur->Nick, sScriptName) == 0) {
#else
		if(strcasecmp(cur->Nick, sScriptName) == 0) {
#endif
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

#ifdef _WIN32
    	if(cur->bIsScript == true && stricmp(cur->Nick, sScriptName) == 0) {
#else
		if(cur->bIsScript == true && strcasecmp(cur->Nick, sScriptName) == 0) {
#endif
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
#ifdef _WIN32
        if(cur->bIsScript == false && cur->ui32Hash == u->ui32NickHash && stricmp(cur->Nick, u->sNick) == 0) {
#else
		if(cur->bIsScript == false && cur->ui32Hash == u->ui32NickHash && strcasecmp(cur->Nick, u->sNick) == 0) {
#endif
            if(bSndMess == true) {
				string Txt = "<"+string(SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], (size_t)SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_HUB_SEC])+
					"> *** "+string(LanguageManager->sTexts[LAN_YOU_SUBSCRIBED_UDP_DBG], (size_t)LanguageManager->ui16TextsLens[LAN_YOU_SUBSCRIBED_UDP_DBG])+
					" "+string(ntohs(cur->sas_to.ss_family == AF_INET6 ? ((struct sockaddr_in6 *)&cur->sas_to)->sin6_port : ((struct sockaddr_in *)&cur->sas_to)->sin_port))+". "+
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
#ifdef _WIN32
        if(stricmp(cur->Nick, sScriptName) == 0) {
#else
		if(strcasecmp(cur->Nick, sScriptName) == 0) {
#endif
            size_t iScriptLen = strlen(sScriptName);
            size_t pktLen = 4+SettingManager->ui16TextsLens[SETTXT_HUB_NAME]+iScriptLen+2+iLen;

#ifdef _WIN32
            char *full = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, pktLen);
#else
			char *full = (char *) malloc(pktLen);
#endif
            if(full == NULL) {
				string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)pktLen)+
					" bytes of memory in clsUdpDebug::Send!";
#ifdef _WIN32
				sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
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

#ifdef _WIN32
            sendto(cur->s, full, (int)pktLen, 0, (struct sockaddr *)&cur->sas_to, cur->sas_len);
#else
			sendto(cur->s, full, pktLen, 0, (struct sockaddr *)&cur->sas_to, cur->sas_len);
#endif
            ui64BytesSent += pktLen;

#ifdef _WIN32
            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)full) == 0) {
				string sDbgstr = "[BUF] Cannot deallocate full in clsUdpDebug::Send! "+string((uint32_t)GetLastError())+" "+
					string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
				AppendSpecialLog(sDbgstr);
        	}
#else
			free(full);
#endif

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
