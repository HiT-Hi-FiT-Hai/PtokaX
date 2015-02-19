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
#endif
//---------------------------------------------------------------------------
clsUdpDebug * clsUdpDebug::mPtr = NULL;
//---------------------------------------------------------------------------

clsUdpDebug::UdpDbgItem::UdpDbgItem() :
#ifdef _WIN32
    s(INVALID_SOCKET),
#else
    s(-1),
#endif
	sas_len(0), ui32Hash(0), sNick(NULL), pPrev(NULL), pNext(NULL), bIsScript(false) {
    memset(&sas_to, 0, sizeof(sockaddr_storage));
}
//---------------------------------------------------------------------------

clsUdpDebug::UdpDbgItem::~UdpDbgItem() {
#ifdef _WIN32
    if(sNick != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sNick) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate sNick in clsUdpDebug::UdpDbgItem::~UdpDbgItem\n", 0);
        }
    }
#else
	free(sNick);
#endif

#ifdef _WIN32
	closesocket(s);
#else
	close(s);
#endif
}
//---------------------------------------------------------------------------

clsUdpDebug::clsUdpDebug() : pList(NULL), pScriptList(NULL) {
	// ...
}
//---------------------------------------------------------------------------

clsUdpDebug::~clsUdpDebug() {
    UdpDbgItem * cur = NULL,
        * next = pList;

	while(next != NULL) {
        cur = next;
        next = cur->pNext;

    	delete cur;
    }
    
    pList = NULL;

    next = pScriptList;

	while(next != NULL) {
        cur = next;
        next = cur->pNext;

    	delete cur;
    }

    pScriptList = NULL;
}
//---------------------------------------------------------------------------

void clsUdpDebug::Broadcast(const char * msg) const {
    Broadcast(msg, strlen(msg));
}
//---------------------------------------------------------------------------

void clsUdpDebug::Broadcast(const char * msg, const size_t &szMsgLen) const {
    if(pList == NULL)
        return;

    size_t szLen = 4+clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME]+szMsgLen;

#ifdef _WIN32
    char * sMsg = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szLen);
#else
	char * sMsg = (char *)malloc(szLen);
#endif
    if(sMsg == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sMsg in clsUdpDebug::Broadcast\n", (uint64_t)szLen);

		return;
    }

    // create packet
    ((uint16_t *)sMsg)[0] = (uint16_t)clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME];
    ((uint16_t *)sMsg)[1] = (uint16_t)szMsgLen;
    memcpy(sMsg+4, clsSettingManager::mPtr->sTexts[SETTXT_HUB_NAME], (size_t)clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME]);
    memcpy(sMsg+4+clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME], msg, szMsgLen);
     
    UdpDbgItem * cur = NULL,
        * next = pList;

	while(next != NULL) {
        cur = next;
        next = cur->pNext;
#ifdef _WIN32
    	sendto(cur->s, sMsg, (int)szLen, 0, (struct sockaddr *)&cur->sas_to, cur->sas_len);
#else
		sendto(cur->s, sMsg, szLen, 0, (struct sockaddr *)&cur->sas_to, cur->sas_len);
#endif
        clsServerManager::ui64BytesSent += szLen;
    }

#ifdef _WIN32
    if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMsg) == 0) {
		AppendDebugLog("%s - [MEM] Cannot deallocate sMsg in clsUdpDebug::Broadcast\n", 0);
	}
#else
	free(sMsg);
#endif
}
//---------------------------------------------------------------------------

void clsUdpDebug::Broadcast(const string & msg) const {
    Broadcast(msg.c_str(), msg.size());
}
//---------------------------------------------------------------------------

bool clsUdpDebug::New(User * u, const int32_t &port) {
	UdpDbgItem * pNewDbg = new (std::nothrow) UdpDbgItem();
    if(pNewDbg == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pNewDbg in clsUdpDebug::New\n", 0);
    	return false;
    }

    // initialize dbg item
#ifdef _WIN32
    pNewDbg->sNick = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, u->ui8NickLen+1);
#else
	pNewDbg->sNick = (char *)malloc(u->ui8NickLen+1);
#endif
    if(pNewDbg->sNick == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sNick in clsUdpDebug::New\n", (uint64_t)(u->ui8NickLen+1));

		delete pNewDbg;
        return false;
    }

    memcpy(pNewDbg->sNick, u->sNick, u->ui8NickLen);
    pNewDbg->sNick[u->ui8NickLen] = '\0';

    pNewDbg->ui32Hash = u->ui32NickHash;

    struct in6_addr i6addr;
    memcpy(&i6addr, &u->ui128IpHash, 16);

    bool bIPv6 = (IN6_IS_ADDR_V4MAPPED(&i6addr) == 0);

    if(bIPv6 == true) {
        ((struct sockaddr_in6 *)&pNewDbg->sas_to)->sin6_family = AF_INET6;
        ((struct sockaddr_in6 *)&pNewDbg->sas_to)->sin6_port = htons((unsigned short)port);
        memcpy(((struct sockaddr_in6 *)&pNewDbg->sas_to)->sin6_addr.s6_addr, u->ui128IpHash, 16);
        pNewDbg->sas_len = sizeof(struct sockaddr_in6);
    } else {
        ((struct sockaddr_in *)&pNewDbg->sas_to)->sin_family = AF_INET;
        ((struct sockaddr_in *)&pNewDbg->sas_to)->sin_port = htons((unsigned short)port);
        ((struct sockaddr_in *)&pNewDbg->sas_to)->sin_addr.s_addr = inet_addr(u->sIP);
        pNewDbg->sas_len = sizeof(struct sockaddr_in);
    }

    pNewDbg->s = socket((bIPv6 == true ? AF_INET6 : AF_INET), SOCK_DGRAM, IPPROTO_UDP);
#ifdef _WIN32
    if(pNewDbg->s == INVALID_SOCKET) {
        int err = WSAGetLastError();
#else
    if(pNewDbg->s == -1) {
#endif
		string Txt = "*** [ERR] "+string(clsLanguageManager::mPtr->sTexts[LAN_UDP_SCK_CREATE_ERR], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_UDP_SCK_CREATE_ERR])+
#ifdef _WIN32
			": "+string(WSErrorStr(err))+" ("+string(err)+")";
#else
			": "+string(errno);
#endif
        u->SendTextDelayed(Txt);
        delete pNewDbg;
        return false;
    }

    // set non-blocking
#ifdef _WIN32
    uint32_t block = 1;
	if(SOCKET_ERROR == ioctlsocket(pNewDbg->s, FIONBIO, (unsigned long *)&block)) {
        int err = WSAGetLastError();
#else
    int oldFlag = fcntl(pNewDbg->s, F_GETFL, 0);
    if(fcntl(pNewDbg->s, F_SETFL, oldFlag | O_NONBLOCK) == -1) {
#endif
		string Txt = "*** [ERR] "+string(clsLanguageManager::mPtr->sTexts[LAN_UDP_NON_BLOCK_FAIL], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_UDP_NON_BLOCK_FAIL])+
#ifdef _WIN32
			": "+string(WSErrorStr(err))+" ("+string(err)+")";
#else
			": "+string(errno);
#endif
		u->SendTextDelayed(Txt);
        delete pNewDbg;
        return false;
    }

    pNewDbg->pPrev = NULL;
    pNewDbg->pNext = NULL;

    if(pList == NULL){
        pList = pNewDbg;
    } else {
        pList->pPrev = pNewDbg;
        pNewDbg->pNext = pList;
        pList = pNewDbg;
    }

    char msg[128];

	int iLen = sprintf(msg, "[HUB] Subscribed, users online: %u", clsServerManager::ui32Logged);
    if(CheckSprintf(iLen, 128, "clsUdpDebug::New") == true) {
	    size_t szLen = 4+clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME]+iLen;
	
#ifdef _WIN32
	    char * sMsg = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szLen);
#else
		char * sMsg = (char *)malloc(szLen);
#endif
	    if(sMsg == NULL) {
			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes in clsUdpDebug::New1\n", (uint64_t)szLen);

			return false;
	    }
	
	    // create packet
	    ((uint16_t *)sMsg)[0] = (uint16_t)clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME];
	    ((uint16_t *)sMsg)[1] = (uint16_t)iLen;
	    memcpy(sMsg+4, clsSettingManager::mPtr->sTexts[SETTXT_HUB_NAME], clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME]);
	    memcpy(sMsg+4+clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME], msg, iLen);
	
#ifdef _WIN32
		sendto(pNewDbg->s, sMsg, (int)szLen, 0, (struct sockaddr *)&pNewDbg->sas_to, pNewDbg->sas_len);
#else
		sendto(pNewDbg->s, sMsg, szLen, 0, (struct sockaddr *)&pNewDbg->sas_to, pNewDbg->sas_len);
#endif
        clsServerManager::ui64BytesSent += szLen;

#ifdef _WIN32		
	    if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMsg) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate full in clsUdpDebug::New\n", 0);
        }
#else
		free(sMsg);
#endif
    }

    pNewDbg->bIsScript = false;

    return true;
}
//---------------------------------------------------------------------------

bool clsUdpDebug::New(char * sIP, const uint16_t &usPort, const bool &bAllData, char * sScriptName) {
	UdpDbgItem * pNewDbg = new (std::nothrow) UdpDbgItem();
    if(pNewDbg == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pNewDbg in clsUdpDebug::New\n", 0);
    	return false;
    }

    // initialize dbg item
    size_t szNameLen = strlen(sScriptName);
#ifdef _WIN32
    pNewDbg->sNick = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szNameLen+1);
#else
	pNewDbg->sNick = (char *)malloc(szNameLen+1);
#endif
    if(pNewDbg->sNick == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sNick in clsUdpDebug::New\n", (uint64_t)(szNameLen+1));

        delete pNewDbg;
        return false;
    }

	memcpy(pNewDbg->sNick, sScriptName, szNameLen);
    pNewDbg->sNick[szNameLen] = '\0';

    pNewDbg->ui32Hash = 0;

    uint8_t ui128IP[16];
    HashIP(sIP, ui128IP);

    struct in6_addr i6addr;
    memcpy(&i6addr, &ui128IP, 16);

    bool bIPv6 = (IN6_IS_ADDR_V4MAPPED(&i6addr) == 0);

    if(bIPv6 == true) {
        ((struct sockaddr_in6 *)&pNewDbg->sas_to)->sin6_family = AF_INET6;
        ((struct sockaddr_in6 *)&pNewDbg->sas_to)->sin6_port = htons(usPort);
        memcpy(((struct sockaddr_in6 *)&pNewDbg->sas_to)->sin6_addr.s6_addr, ui128IP, 16);
        pNewDbg->sas_len = sizeof(struct sockaddr_in6);
    } else {
        ((struct sockaddr_in *)&pNewDbg->sas_to)->sin_family = AF_INET;
        ((struct sockaddr_in *)&pNewDbg->sas_to)->sin_port = htons(usPort);
        memcpy(&((struct sockaddr_in *)&pNewDbg->sas_to)->sin_addr.s_addr, ui128IP+12, 4);
        pNewDbg->sas_len = sizeof(struct sockaddr_in);
    }

    pNewDbg->s = socket((bIPv6 == true ? AF_INET6 : AF_INET), SOCK_DGRAM, IPPROTO_UDP);

#ifdef _WIN32
    if(pNewDbg->s == INVALID_SOCKET) {
#else
    if(pNewDbg->s == -1) {
#endif
        delete pNewDbg;
        return false;
    }

    // set non-blocking
#ifdef _WIN32
    uint32_t block = 1;
	if(SOCKET_ERROR == ioctlsocket(pNewDbg->s, FIONBIO, (unsigned long *)&block)) {
#else
    int oldFlag = fcntl(pNewDbg->s, F_GETFL, 0);
    if(fcntl(pNewDbg->s, F_SETFL, oldFlag | O_NONBLOCK) == -1) {
#endif
        delete pNewDbg;
        return false;
    }
        
    pNewDbg->pPrev = NULL;
    pNewDbg->pNext = NULL;

    if(bAllData == true) {
        if(pList == NULL){
            pList = pNewDbg;
        } else {
            pList->pPrev = pNewDbg;
            pNewDbg->pNext = pList;
            pList = pNewDbg;
        }
    } else {
        if(pScriptList == NULL){
            pScriptList = pNewDbg;
        } else {
            pScriptList->pPrev = pNewDbg;
            pNewDbg->pNext = pScriptList;
            pScriptList = pNewDbg;
        }
    }

    char msg[128];

	int iLen = sprintf(msg, "[HUB] Subscribed, users online: %u", clsServerManager::ui32Logged);
    if(CheckSprintf(iLen, 128, "clsUdpDebug::New") == true) {
	    size_t szLen = 4+clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME]+iLen;
	    if(bAllData == false) {
            szLen += szNameLen+2;
        }

#ifdef _WIN32
	    char * sMsg = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szLen);
#else
		char * sMsg = (char *)malloc(szLen);
#endif
	    if(sMsg == NULL) {
			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes in clsUdpDebug::New1\n", (uint64_t)szLen);

			return false;
	    }
	
	    // create packet
	    if(bAllData == true) {
	       ((uint16_t *)sMsg)[0] = (uint16_t)clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME];
        } else {
            ((uint16_t *)sMsg)[0] = (uint16_t)(clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME]+szNameLen+2);
        }

	    ((uint16_t *)sMsg)[1] = (uint16_t)iLen;

	    memcpy(sMsg+4, clsSettingManager::mPtr->sTexts[SETTXT_HUB_NAME], clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME]);
	    if(bAllData == false) {
			sMsg[4+clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME]] = '[';
            memcpy(sMsg+4+clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME]+1, sScriptName, szNameLen);
			sMsg[4+clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME]+1+szNameLen] = ']';
            memcpy(sMsg+4+clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME]+szNameLen+2, msg, iLen);
        } else {
	       memcpy(sMsg+4+clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME], msg, iLen);
        }

#ifdef _WIN32
		sendto(pNewDbg->s, sMsg, (int)szLen, 0, (struct sockaddr *)&pNewDbg->sas_to, pNewDbg->sas_len);
#else
		sendto(pNewDbg->s, sMsg, szLen, 0, (struct sockaddr *)&pNewDbg->sas_to, pNewDbg->sas_len);
#endif
        clsServerManager::ui64BytesSent += szLen;

#ifdef _WIN32
	    if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMsg) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate full in clsUdpDebug::New\n", 0);
        }
#else
		free(sMsg);
#endif
    }

    pNewDbg->bIsScript = true;

    return true;
}
//---------------------------------------------------------------------------

bool clsUdpDebug::Remove(User * u) {
    UdpDbgItem * cur = NULL,
        * next = pList;

	while(next != NULL) {
        cur = next;
        next = cur->pNext;

		if(cur->bIsScript == false && cur->ui32Hash == u->ui32NickHash && strcasecmp(cur->sNick, u->sNick) == 0) {
            if(cur->pPrev == NULL) {
                if(cur->pNext == NULL) {
                    pList = NULL;
                } else {
                    cur->pNext->pPrev = NULL;
                    pList = cur->pNext;
                }
            } else if(cur->pNext == NULL) {
                cur->pPrev->pNext = NULL;
            } else {
                cur->pPrev->pNext = cur->pNext;
                cur->pNext->pPrev = cur->pPrev;
            }
            
	        delete cur;
            return true;
        }
    }
    return false;
}
//---------------------------------------------------------------------------

void clsUdpDebug::Remove(char * sScriptName) {
    UdpDbgItem * cur = NULL,
        * next = pScriptList;

	while(next != NULL) {
        cur = next;
        next = cur->pNext;

		if(strcasecmp(cur->sNick, sScriptName) == 0) {
            if(cur->pPrev == NULL) {
                if(cur->pNext == NULL) {
                    pList = NULL;
                } else {
                    cur->pNext->pPrev = NULL;
                    pList = cur->pNext;
                }
            } else if(cur->pNext == NULL) {
                cur->pPrev->pNext = NULL;
            } else {
                cur->pPrev->pNext = cur->pNext;
                cur->pNext->pPrev = cur->pPrev;
            }
            
	        delete cur;
            return;
        }
    }

    next = pList;
	while(next != NULL) {
        cur = next;
        next = cur->pNext;

		if(cur->bIsScript == true && strcasecmp(cur->sNick, sScriptName) == 0) {
            if(cur->pPrev == NULL) {
                if(cur->pNext == NULL) {
                    pList = NULL;
                } else {
                    cur->pNext->pPrev = NULL;
                    pList = cur->pNext;
                }
            } else if(cur->pNext == NULL) {
                cur->pPrev->pNext = NULL;
            } else {
                cur->pPrev->pNext = cur->pNext;
                cur->pNext->pPrev = cur->pPrev;
            }
            
	        delete cur;
            return;
        }
    }
}
//---------------------------------------------------------------------------

bool clsUdpDebug::CheckUdpSub(User * u, bool bSndMess/* = false*/) const {
    UdpDbgItem * cur = NULL,
        * next = pList;

	while(next != NULL) {
        cur = next;
        next = cur->pNext;

		if(cur->bIsScript == false && cur->ui32Hash == u->ui32NickHash && strcasecmp(cur->sNick, u->sNick) == 0) {
            if(bSndMess == true) {
				string Txt = "<"+string(clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], (size_t)clsSettingManager::mPtr->ui16PreTextsLens[clsSettingManager::SETPRETXT_HUB_SEC])+
					"> *** "+string(clsLanguageManager::mPtr->sTexts[LAN_YOU_SUBSCRIBED_UDP_DBG], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_YOU_SUBSCRIBED_UDP_DBG])+
					" "+string(ntohs(cur->sas_to.ss_family == AF_INET6 ? ((struct sockaddr_in6 *)&cur->sas_to)->sin6_port : ((struct sockaddr_in *)&cur->sas_to)->sin_port))+". "+
                    string(clsLanguageManager::mPtr->sTexts[LAN_TO_UNSUB_UDP_DBG], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_TO_UNSUB_UDP_DBG])+".|";
				u->SendTextDelayed(Txt);
            }
                
            return true;
        }
    }
    return false;
}
//---------------------------------------------------------------------------

void clsUdpDebug::Send(char * sScriptName, char * sMessage, const size_t &szMsgLen) const {
    UdpDbgItem * cur = NULL,
        * next = pScriptList;

	while(next != NULL) {
        cur = next;
        next = cur->pNext;

		if(strcasecmp(cur->sNick, sScriptName) == 0) {
            size_t szNameLen = strlen(sScriptName);
            size_t szLen = 4+clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME]+szNameLen+2+szMsgLen;

#ifdef _WIN32
            char * sMsg = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szLen);
#else
			char * sMsg = (char *)malloc(szLen);
#endif
            if(sMsg == NULL) {
				AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes in clsUdpDebug::Send\n", (uint64_t)szLen);

        		return;
            }
        
            // create packet
			((uint16_t *)sMsg)[0] = (uint16_t)(clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME]+szNameLen+2);
            ((uint16_t *)sMsg)[1] = (uint16_t)szMsgLen;

            memcpy(sMsg+4, clsSettingManager::mPtr->sTexts[SETTXT_HUB_NAME], (size_t)clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME]);
          
			sMsg[4+clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME]] = '[';
            memcpy(sMsg+4+clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME]+1, sScriptName, szNameLen);
			sMsg[4+clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME]+1+szNameLen] = ']';

            memcpy(sMsg+4+clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME]+szNameLen+2, sMessage, szMsgLen);

#ifdef _WIN32
            sendto(cur->s, sMsg, (int)szLen, 0, (struct sockaddr *)&cur->sas_to, cur->sas_len);
#else
			sendto(cur->s, sMsg, szLen, 0, (struct sockaddr *)&cur->sas_to, cur->sas_len);
#endif
            clsServerManager::ui64BytesSent += szLen;

#ifdef _WIN32
            if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMsg) == 0) {
				AppendDebugLog("%s - [MEM] Cannot deallocate full in clsUdpDebug::Send\n", 0);
        	}
#else
			free(sMsg);
#endif

        	return;
        }
    }
}
//---------------------------------------------------------------------------

void clsUdpDebug::Cleanup() {
    UdpDbgItem * cur = NULL,
        * next = pList;

	while(next != NULL) {
        cur = next;
        next = cur->pNext;
    	delete cur;
    }

    pList = NULL;

    next = pScriptList;
	while(next != NULL) {
        cur = next;
        next = cur->pNext;
    	delete cur;
    }

    pScriptList = NULL;
}
//---------------------------------------------------------------------------
