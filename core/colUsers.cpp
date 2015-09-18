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
//---------------------------------------------------------------------------
#include "GlobalDataQueue.h"
#include "LanguageManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
static const uint32_t MYINFOLISTSIZE = 1024*256;
static const uint32_t IPLISTSIZE = 1024*64;
//---------------------------------------------------------------------------
clsUsers * clsUsers::mPtr = NULL;
//---------------------------------------------------------------------------

clsUsers::RecTime::RecTime(const uint8_t * pIpHash) : ui64DisConnTick(0), pPrev(NULL), pNext(NULL), sNick(NULL), ui32NickHash(0) {
    memcpy(ui128IpHash, pIpHash, 16);
};
//---------------------------------------------------------------------------

clsUsers::clsUsers() : ui64ChatMsgsTick(0), ui64ChatLockFromTick(0), pRecTimeList(NULL), pListE(NULL), ui16ChatMsgs(0), bChatLocked(false), pListS(NULL), pNickList(NULL), pZNickList(NULL), pOpList(NULL), pZOpList(NULL), pUserIPList(NULL), pZUserIPList(NULL), pMyInfos(NULL), pZMyInfos(NULL), pMyInfosTag(NULL), 
	pZMyInfosTag(NULL), ui32MyInfosLen(0), ui32MyInfosSize(0), ui32ZMyInfosLen(0), ui32ZMyInfosSize(0), ui32MyInfosTagLen(0), ui32MyInfosTagSize(0), ui32ZMyInfosTagLen(0), ui32ZMyInfosTagSize(0), ui32NickListLen(0), ui32NickListSize(0), ui32ZNickListLen(0), ui32ZNickListSize(0), ui32OpListLen(0), ui32OpListSize(0), 
	ui32ZOpListLen(0), ui32ZOpListSize(0), ui32UserIPListSize(0), ui32UserIPListLen(0), ui32ZUserIPListSize(0), ui32ZUserIPListLen(0), ui16ActSearchs(0), ui16PasSearchs(0) {
#ifdef _WIN32
    clsServerManager::hRecvHeap = HeapCreate(HEAP_NO_SERIALIZE, 0x20000, 0);
    clsServerManager::hSendHeap = HeapCreate(HEAP_NO_SERIALIZE, 0x40000, 0);

    pNickList = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, NICKLISTSIZE);
#else
	pNickList = (char *)calloc(NICKLISTSIZE, 1);
#endif
    if(pNickList == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create pNickList\n");
		exit(EXIT_FAILURE);
    }
    memcpy(pNickList, "$NickList |", 11);
    pNickList[11] = '\0';
    ui32NickListLen = 11;
    ui32NickListSize = NICKLISTSIZE - 1;
#ifdef _WIN32
    pZNickList = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ZLISTSIZE);
#else
	pZNickList = (char *)calloc(ZLISTSIZE, 1);
#endif
    if(pZNickList == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create pZNickList\n");
		exit(EXIT_FAILURE);
    }
    ui32ZNickListLen = 0;
    ui32ZNickListSize = ZLISTSIZE - 1;
#ifdef _WIN32
    pOpList = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, OPLISTSIZE);
#else
	pOpList = (char *)calloc(OPLISTSIZE, 1);
#endif
    if(pOpList == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create pOpList\n");
		exit(EXIT_FAILURE);
    }
    memcpy(pOpList, "$OpList |", 9);
    pOpList[9] = '\0';
    ui32OpListLen = 9;
    ui32OpListSize = OPLISTSIZE - 1;
#ifdef _WIN32
    pZOpList = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ZLISTSIZE);
#else
	pZOpList = (char *)calloc(ZLISTSIZE, 1);
#endif
    if(pZOpList == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create pZOpList\n");
		exit(EXIT_FAILURE);
    }
    ui32ZOpListLen = 0;
    ui32ZOpListSize = ZLISTSIZE - 1;
    
    if(clsSettingManager::mPtr->ui8FullMyINFOOption != 0) {
#ifdef _WIN32
        pMyInfos = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, MYINFOLISTSIZE);
#else
		pMyInfos = (char *)calloc(MYINFOLISTSIZE, 1);
#endif
        if(pMyInfos == NULL) {
    		AppendDebugLog("%s - [MEM] Cannot create pMyInfos\n");
    		exit(EXIT_FAILURE);
        }
        ui32MyInfosSize = MYINFOLISTSIZE - 1;
        
#ifdef _WIN32
        pZMyInfos = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ZMYINFOLISTSIZE);
#else
		pZMyInfos = (char *)calloc(ZMYINFOLISTSIZE, 1);
#endif
        if(pZMyInfos == NULL) {
    		AppendDebugLog("%s - [MEM] Cannot create pZMyInfos\n");
    		exit(EXIT_FAILURE);
        }
        ui32ZMyInfosSize = ZMYINFOLISTSIZE - 1;
    } else {
        pMyInfos = NULL;
        ui32MyInfosSize = 0;
        pZMyInfos = NULL;
        ui32ZMyInfosSize = 0;
    }
    ui32MyInfosLen = 0;
    ui32ZMyInfosLen = 0;
    
    if(clsSettingManager::mPtr->ui8FullMyINFOOption != 2) {
#ifdef _WIN32
        pMyInfosTag = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, MYINFOLISTSIZE);
#else
		pMyInfosTag = (char *)calloc(MYINFOLISTSIZE, 1);
#endif
        if(pMyInfosTag == NULL) {
    		AppendDebugLog("%s - [MEM] Cannot create pMyInfosTag\n");
    		exit(EXIT_FAILURE);
        }
        ui32MyInfosTagSize = MYINFOLISTSIZE - 1;

#ifdef _WIN32
        pZMyInfosTag = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ZMYINFOLISTSIZE);
#else
		pZMyInfosTag = (char *)calloc(ZMYINFOLISTSIZE, 1);
#endif
        if(pZMyInfosTag == NULL) {
    		AppendDebugLog("%s - [MEM] Cannot create pZMyInfosTag\n");
    		exit(EXIT_FAILURE);
        }
        ui32ZMyInfosTagSize = ZMYINFOLISTSIZE - 1;
    } else {
        pMyInfosTag = NULL;
        ui32MyInfosTagSize = 0;
        pZMyInfosTag = NULL;
        ui32ZMyInfosTagSize = 0;
    }
    ui32MyInfosTagLen = 0;
    ui32ZMyInfosTagLen = 0;

#ifdef _WIN32
    pUserIPList = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, IPLISTSIZE);
#else
	pUserIPList = (char *)calloc(IPLISTSIZE, 1);
#endif
    if(pUserIPList == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create pUserIPList\n");
		exit(EXIT_FAILURE);
    }
    memcpy(pUserIPList, "$UserIP |", 9);
    pUserIPList[9] = '\0';
    ui32UserIPListLen = 9;
    ui32UserIPListSize = IPLISTSIZE - 1;

#ifdef _WIN32
    pZUserIPList = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ZLISTSIZE);
#else
	pZUserIPList = (char *)calloc(ZLISTSIZE, 1);
#endif
    if(pZUserIPList == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create pZUserIPList\n");
		exit(EXIT_FAILURE);
    }
    ui32ZUserIPListLen = 0;
    ui32ZUserIPListSize = ZLISTSIZE - 1;
}
//---------------------------------------------------------------------------

clsUsers::~clsUsers() {
    RecTime * cur = NULL,
        * next = pRecTimeList;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

#ifdef _WIN32
        if(cur->sNick != NULL) {
            if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->sNick) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate cur->sNick in clsUsers::~clsUsers\n");
            }
        }
#else
		free(cur->sNick);
#endif

        delete cur;
    }

#ifdef _WIN32
    if(pNickList != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pNickList) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate pNickList in clsUsers::~clsUsers\n");
        }
    }
#else
	free(pNickList);
#endif

#ifdef _WIN32
    if(pZNickList != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pZNickList) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate pZNickList in clsUsers::~clsUsers\n");
        }
    }
#else
	free(pZNickList);
#endif

#ifdef _WIN32
    if(pOpList != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOpList) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate pOpList in clsUsers::~clsUsers\n");
        }
    }
#else
	free(pOpList);
#endif

#ifdef _WIN32
    if(pZOpList != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pZOpList) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate pZOpList in clsUsers::~clsUsers\n");
        }
    }
#else
	free(pZOpList);
#endif

#ifdef _WIN32
    if(pMyInfos != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pMyInfos) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate pMyInfos in clsUsers::~clsUsers\n");
        }
    }
#else
	free(pMyInfos);
#endif

#ifdef _WIN32
    if(pZMyInfos != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pZMyInfos) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate pZMyInfos in clsUsers::~clsUsers\n");
        }
    }
#else
	free(pZMyInfos);
#endif

#ifdef _WIN32
    if(pMyInfosTag != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pMyInfosTag) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate pMyInfosTag in clsUsers::~clsUsers\n");
        }
    }
#else
	free(pMyInfosTag);
#endif

#ifdef _WIN32
    if(pZMyInfosTag != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pZMyInfosTag) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate pZMyInfosTag in clsUsers::~clsUsers\n");
        }
    }
#else
	free(pZMyInfosTag);
#endif

#ifdef _WIN32
    if(pUserIPList != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pUserIPList) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate pUserIPList in clsUsers::~clsUsers\n");
        }
    }
#else
	free(pUserIPList);
#endif

#ifdef _WIN32
    if(pZUserIPList != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pZUserIPList) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate pZUserIPList in clsUsers::~clsUsers\n");
        }
    }
#else
	free(pZUserIPList);
#endif

#ifdef _WIN32
    HeapDestroy(clsServerManager::hRecvHeap);
    HeapDestroy(clsServerManager::hSendHeap);
#endif
}
//---------------------------------------------------------------------------

void clsUsers::AddUser(User * pUser) {
    if(pListS == NULL) {
    	pListS = pUser;
    	pListE = pUser;
    } else {
        pUser->pPrev = pListE;
        pListE->pNext = pUser;
        pListE = pUser;
    }
}
//---------------------------------------------------------------------------

void clsUsers::RemUser(User * pUser) {
    if(pUser->pPrev == NULL) {
        if(pUser->pNext == NULL) {
            pListS = NULL;
            pListE = NULL;
        } else {
            pUser->pNext->pPrev = NULL;
            pListS = pUser->pNext;
        }
    } else if(pUser->pNext == NULL) {
        pUser->pPrev->pNext = NULL;
        pListE = pUser->pPrev;
    } else {
        pUser->pPrev->pNext = pUser->pNext;
        pUser->pNext->pPrev = pUser->pPrev;
    }
}
//---------------------------------------------------------------------------

void clsUsers::DisconnectAll() {
    uint32_t iCloseLoops = 0;

#ifndef _WIN32
    struct timespec sleeptime;
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = 50000000;
#endif

    User * u = NULL, * next = NULL;

    while(pListS != NULL && iCloseLoops <= 100) {
        next = pListS;

        while(next != NULL) {
            u = next;
    		next = u->pNext;

            if(((u->ui32BoolBits & User::BIT_ERROR) == User::BIT_ERROR) == true || u->ui32SendBufDataLen == 0) {
//              Memo("*** User " + string(u->Nick, u->NickLen) + " closed...");
                if(u->pPrev == NULL) {
                    if(u->pNext == NULL) {
                        pListS = NULL;
                    } else {
                        u->pNext->pPrev = NULL;
                        pListS = u->pNext;
                    }
                } else if(u->pNext == NULL) {
                    u->pPrev->pNext = NULL;
                } else {
                    u->pPrev->pNext = u->pNext;
                    u->pNext->pPrev = u->pPrev;
                }

#ifdef _WIN32
                shutdown(u->Sck, SD_SEND);
				closesocket(u->Sck);
#else
                shutdown(u->Sck, SHUT_RD);
				close(u->Sck);
#endif

				delete u;
            } else {
                u->Try2Send();
            }
        }
        iCloseLoops++;
#ifdef _WIN32
        ::Sleep(50);
#else
        nanosleep(&sleeptime, NULL);
#endif
    }

    next = pListS;

    while(next != NULL) {
        u = next;
    	next = u->pNext;

#ifdef _WIN32
    	shutdown(u->Sck, SD_SEND);
		closesocket(u->Sck);
#else
    	shutdown(u->Sck, SHUT_RDWR);
		close(u->Sck);
#endif

		delete u;
	}
}
//---------------------------------------------------------------------------

void clsUsers::Add2NickList(User * pUser) {
    // $NickList nick$$nick2$$|

    if(ui32NickListSize < ui32NickListLen+pUser->ui8NickLen+2) {
        char * pOldBuf = pNickList;
#ifdef _WIN32
        pNickList = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, ui32NickListSize+NICKLISTSIZE+1);
#else
		pNickList = (char *)realloc(pOldBuf, ui32NickListSize+NICKLISTSIZE+1);
#endif
        if(pNickList == NULL) {
            pNickList = pOldBuf;
            pUser->ui32BoolBits |= User::BIT_ERROR;
            pUser->Close();

			AppendDebugLogFormat("Cannot reallocate %u bytes in clsUsers::Add2NickList for NickList\n", ui32NickListSize+NICKLISTSIZE+1);

            return;
		}
        ui32NickListSize += NICKLISTSIZE;
    }

    memcpy(pNickList+ui32NickListLen-1, pUser->sNick, pUser->ui8NickLen);
    ui32NickListLen += (uint32_t)(pUser->ui8NickLen+2);

    pNickList[ui32NickListLen-3] = '$';
    pNickList[ui32NickListLen-2] = '$';
    pNickList[ui32NickListLen-1] = '|';
    pNickList[ui32NickListLen] = '\0';

    ui32ZNickListLen = 0;

    if(((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        return;
    }

    if(ui32OpListSize < ui32OpListLen+pUser->ui8NickLen+2) {
        char * pOldBuf = pOpList;
#ifdef _WIN32
        pOpList = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, ui32OpListSize+OPLISTSIZE+1);
#else
		pOpList = (char *)realloc(pOldBuf, ui32OpListSize+OPLISTSIZE+1);
#endif
        if(pOpList == NULL) {
            pOpList = pOldBuf;
            pUser->ui32BoolBits |= User::BIT_ERROR;
            pUser->Close();

            AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in clsUsers::Add2NickList for pOpList\n", ui32OpListSize+OPLISTSIZE+1);

            return;
        }
        ui32OpListSize += OPLISTSIZE;
    }

    memcpy(pOpList+ui32OpListLen-1, pUser->sNick, pUser->ui8NickLen);
    ui32OpListLen += (uint32_t)(pUser->ui8NickLen+2);

    pOpList[ui32OpListLen-3] = '$';
    pOpList[ui32OpListLen-2] = '$';
    pOpList[ui32OpListLen-1] = '|';
    pOpList[ui32OpListLen] = '\0';

    ui32ZOpListLen = 0;
}
//---------------------------------------------------------------------------

void clsUsers::AddBot2NickList(char * sNick, const size_t &szNickLen, const bool &bIsOp) {
    // $NickList nick$$nick2$$|

    if(ui32NickListSize < ui32NickListLen+szNickLen+2) {
        char * pOldBuf = pNickList;
#ifdef _WIN32
        pNickList = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, ui32NickListSize+NICKLISTSIZE+1);
#else
		pNickList = (char *)realloc(pOldBuf, ui32NickListSize+NICKLISTSIZE+1);
#endif
        if(pNickList == NULL) {
            pNickList = pOldBuf;

			AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in clsUsers::AddBot2NickList for NickList\n", ui32NickListSize+NICKLISTSIZE+1);

            return;
		}
        ui32NickListSize += NICKLISTSIZE;
    }

    memcpy(pNickList+ui32NickListLen-1, sNick, szNickLen);
    ui32NickListLen += (uint32_t)(szNickLen+2);

    pNickList[ui32NickListLen-3] = '$';
    pNickList[ui32NickListLen-2] = '$';
    pNickList[ui32NickListLen-1] = '|';
    pNickList[ui32NickListLen] = '\0';

    ui32ZNickListLen = 0;

    if(bIsOp == false)
        return;

    if(ui32OpListSize < ui32OpListLen+szNickLen+2) {
        char * pOldBuf = pOpList;
#ifdef _WIN32
        pOpList = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, ui32OpListSize+OPLISTSIZE+1);
#else
		pOpList = (char *)realloc(pOldBuf, ui32OpListSize+OPLISTSIZE+1);
#endif
        if(pOpList == NULL) {
            pOpList = pOldBuf;

            AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in clsUsers::AddBot2NickList for pOpList\n", ui32OpListSize+OPLISTSIZE+1);

            return;
        }
        ui32OpListSize += OPLISTSIZE;
    }

    memcpy(pOpList+ui32OpListLen-1, sNick, szNickLen);
    ui32OpListLen += (uint32_t)(szNickLen+2);

    pOpList[ui32OpListLen-3] = '$';
    pOpList[ui32OpListLen-2] = '$';
    pOpList[ui32OpListLen-1] = '|';
    pOpList[ui32OpListLen] = '\0';

    ui32ZOpListLen = 0;
}
//---------------------------------------------------------------------------

void clsUsers::Add2OpList(User * pUser) {
    if(ui32OpListSize < ui32OpListLen+pUser->ui8NickLen+2) {
        char * pOldBuf = pOpList;
#ifdef _WIN32
        pOpList = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, ui32OpListSize+OPLISTSIZE+1);
#else
		pOpList = (char *)realloc(pOldBuf, ui32OpListSize+OPLISTSIZE+1);
#endif
        if(pOpList == NULL) {
            pOpList = pOldBuf;
            pUser->ui32BoolBits |= User::BIT_ERROR;
            pUser->Close();

            AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in clsUsers::Add2OpList for pOpList\n", ui32OpListSize+OPLISTSIZE+1);

            return;
        }
        ui32OpListSize += OPLISTSIZE;
    }

    memcpy(pOpList+ui32OpListLen-1, pUser->sNick, pUser->ui8NickLen);
    ui32OpListLen += (uint32_t)(pUser->ui8NickLen+2);

    pOpList[ui32OpListLen-3] = '$';
    pOpList[ui32OpListLen-2] = '$';
    pOpList[ui32OpListLen-1] = '|';
    pOpList[ui32OpListLen] = '\0';

    ui32ZOpListLen = 0;
}
//---------------------------------------------------------------------------

void clsUsers::DelFromNickList(char * sNick, const bool &bIsOp) {
    int iRet = sprintf(clsServerManager::pGlobalBuffer, "$%s$", sNick);
    if(CheckSprintf(iRet, clsServerManager::szGlobalBufferSize, "clsUsers::DelFromNickList") == false) {
        return;
    }

    pNickList[9] = '$';
    char * sFound = strstr(pNickList, clsServerManager::pGlobalBuffer);
    pNickList[9] = ' ';

    if(sFound != NULL) {
        memmove(sFound+1, sFound+(iRet+1), ui32NickListLen-((sFound+iRet)-pNickList));
        ui32NickListLen -= iRet;
        ui32ZNickListLen = 0;
    }

    if(!bIsOp) return;

    pOpList[7] = '$';
    sFound = strstr(pOpList, clsServerManager::pGlobalBuffer);
    pOpList[7] = ' ';

    if(sFound != NULL) {
        memmove(sFound+1, sFound+(iRet+1), ui32OpListLen-((sFound+iRet)-pOpList));
        ui32OpListLen -= iRet;
        ui32ZOpListLen = 0;
    }
}
//---------------------------------------------------------------------------

void clsUsers::DelFromOpList(char * sNick) {
    int iRet = sprintf(clsServerManager::pGlobalBuffer, "$%s$", sNick);
    if(CheckSprintf(iRet, clsServerManager::szGlobalBufferSize, "clsUsers::DelFromOpList") == false) {
        return;
    }

    pOpList[7] = '$';
    char * sFound = strstr(pOpList, clsServerManager::pGlobalBuffer);
    pOpList[7] = ' ';

    if(sFound != NULL) {
        memmove(sFound+1, sFound+(iRet+1), ui32OpListLen-((sFound+iRet)-pOpList));
        ui32OpListLen -= iRet;
        ui32ZOpListLen = 0;
    }
}
//---------------------------------------------------------------------------

// PPK ... check global mainchat flood and add to global queue
void clsUsers::SendChat2All(User * pUser, char * sData, const size_t &szChatLen, void * pToUser) {
    clsUdpDebug::mPtr->Broadcast(sData, szChatLen);

    if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::NODEFLOODMAINCHAT) == false && clsSettingManager::mPtr->i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] != 0) {
        if(ui16ChatMsgs == 0) {
			ui64ChatMsgsTick = clsServerManager::ui64ActualTick;
			ui64ChatLockFromTick = clsServerManager::ui64ActualTick;
            ui16ChatMsgs = 0;
            bChatLocked = false;
		} else if((ui64ChatMsgsTick+clsSettingManager::mPtr->i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_TIME]) < clsServerManager::ui64ActualTick) {
			ui64ChatMsgsTick = clsServerManager::ui64ActualTick;
            ui16ChatMsgs = 0;
        }

        ui16ChatMsgs++;

        if(ui16ChatMsgs > (uint16_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_MESSAGES]) {
			ui64ChatLockFromTick = clsServerManager::ui64ActualTick;
            if(bChatLocked == false) {
                if(clsSettingManager::mPtr->bBools[SETBOOL_DEFLOOD_REPORT] == true) {
                    clsGlobalDataQueue::mPtr->StatusMessageFormat("clsUsers::SendChat2All", "<%s> *** %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_GLOBAL_CHAT_FLOOD_DETECTED]);
                }

                bChatLocked = true;
            }
        }

        if(bChatLocked == true) {
            if((ui64ChatLockFromTick+clsSettingManager::mPtr->i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_TIMEOUT]) > clsServerManager::ui64ActualTick) {
                if(clsSettingManager::mPtr->i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 1) {
                    return;
                } else if(clsSettingManager::mPtr->i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 2) {
                	memcpy(clsServerManager::pGlobalBuffer, pUser->sIP, pUser->ui8IpLen);
                	clsServerManager::pGlobalBuffer[pUser->ui8IpLen] = ' ';
					size_t szLen = pUser->ui8IpLen+1;
                    memcpy(clsServerManager::pGlobalBuffer+szLen, sData, szChatLen);
                    szLen += szChatLen;
                    clsServerManager::pGlobalBuffer[szLen] = '\0';
                    clsGlobalDataQueue::mPtr->AddQueueItem(clsServerManager::pGlobalBuffer, szLen, NULL, 0, clsGlobalDataQueue::CMD_OPS);

                    return;
                }
            } else {
                bChatLocked = false;
            }
        }
    }

    if(pToUser == NULL) {
        clsGlobalDataQueue::mPtr->AddQueueItem(sData, szChatLen, NULL, 0, clsGlobalDataQueue::CMD_CHAT);
    } else {
        clsGlobalDataQueue::mPtr->FillBlankQueueItem(sData, szChatLen, pToUser);
    }
}
//---------------------------------------------------------------------------

void clsUsers::Add2MyInfos(User * pUser) {
    if(ui32MyInfosSize < ui32MyInfosLen+pUser->ui16MyInfoShortLen) {
        char * pOldBuf = pMyInfos;
#ifdef _WIN32
        pMyInfos = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, ui32MyInfosSize+MYINFOLISTSIZE+1);
#else
		pMyInfos = (char *)realloc(pOldBuf, ui32MyInfosSize+MYINFOLISTSIZE+1);
#endif
        if(pMyInfos == NULL) {
            pMyInfos = pOldBuf;
            pUser->ui32BoolBits |= User::BIT_ERROR;
            pUser->Close();

			AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in clsUsers::Add2MyInfos\n", ui32MyInfosSize+MYINFOLISTSIZE+1);

            return;
        }
        ui32MyInfosSize += MYINFOLISTSIZE;
    }

    memcpy(pMyInfos+ui32MyInfosLen, pUser->sMyInfoShort, pUser->ui16MyInfoShortLen);
    ui32MyInfosLen += pUser->ui16MyInfoShortLen;

    pMyInfos[ui32MyInfosLen] = '\0';

    ui32ZMyInfosLen = 0;
}
//---------------------------------------------------------------------------

void clsUsers::DelFromMyInfos(User * pUser) {
	char * sMatch = strstr(pMyInfos, pUser->sMyInfoShort+8);
    if(sMatch != NULL) {
		sMatch -= 8;
		memmove(sMatch, sMatch+pUser->ui16MyInfoShortLen, ui32MyInfosLen-((sMatch+(pUser->ui16MyInfoShortLen-1))-pMyInfos));
        ui32MyInfosLen -= pUser->ui16MyInfoShortLen;
        ui32ZMyInfosLen = 0;
    }
}
//---------------------------------------------------------------------------

void clsUsers::Add2MyInfosTag(User * pUser) {
    if(ui32MyInfosTagSize < ui32MyInfosTagLen+pUser->ui16MyInfoLongLen) {
        char * pOldBuf = pMyInfosTag;
#ifdef _WIN32
        pMyInfosTag = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, ui32MyInfosTagSize+MYINFOLISTSIZE+1);
#else
		pMyInfosTag = (char *)realloc(pOldBuf, ui32MyInfosTagSize+MYINFOLISTSIZE+1);
#endif
        if(pMyInfosTag == NULL) {
            pMyInfosTag = pOldBuf;
            pUser->ui32BoolBits |= User::BIT_ERROR;
            pUser->Close();

			AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in clsUsers::Add2MyInfosTag\n", ui32MyInfosTagSize+MYINFOLISTSIZE+1);

            return;
        }
        ui32MyInfosTagSize += MYINFOLISTSIZE;
    }

    memcpy(pMyInfosTag+ui32MyInfosTagLen, pUser->sMyInfoLong, pUser->ui16MyInfoLongLen);
    ui32MyInfosTagLen += pUser->ui16MyInfoLongLen;

    pMyInfosTag[ui32MyInfosTagLen] = '\0';

    ui32ZMyInfosTagLen = 0;
}
//---------------------------------------------------------------------------

void clsUsers::DelFromMyInfosTag(User * pUser) {
	char * sMatch = strstr(pMyInfosTag, pUser->sMyInfoLong+8);
    if(sMatch != NULL) {
		sMatch -= 8;
        memmove(sMatch, sMatch+pUser->ui16MyInfoLongLen, ui32MyInfosTagLen-((sMatch+(pUser->ui16MyInfoLongLen-1))-pMyInfosTag));
        ui32MyInfosTagLen -= pUser->ui16MyInfoLongLen;
        ui32ZMyInfosTagLen = 0;
    }
}
//---------------------------------------------------------------------------

void clsUsers::AddBot2MyInfos(char * sMyInfo) {
	size_t szLen = strlen(sMyInfo);
	if(pMyInfosTag != NULL) {
	    if(strstr(pMyInfosTag, sMyInfo) == NULL ) {
            if(ui32MyInfosTagSize < ui32MyInfosTagLen+szLen) {
                char * pOldBuf = pMyInfosTag;
#ifdef _WIN32
                pMyInfosTag = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, ui32MyInfosTagSize+MYINFOLISTSIZE+1);
#else
				pMyInfosTag = (char *)realloc(pOldBuf, ui32MyInfosTagSize+MYINFOLISTSIZE+1);
#endif
                if(pMyInfosTag == NULL) {
                    pMyInfosTag = pOldBuf;

					AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes for pMyInfosTag in clsUsers::AddBot2MyInfos\n", ui32MyInfosTagSize+MYINFOLISTSIZE+1);

                    return;
                }
                ui32MyInfosTagSize += MYINFOLISTSIZE;
            }
        	memcpy(pMyInfosTag+ui32MyInfosTagLen, sMyInfo, szLen);
            ui32MyInfosTagLen += (uint32_t)szLen;
            pMyInfosTag[ui32MyInfosTagLen] = '\0';
            ui32ZMyInfosLen = 0;
        }
    }
    if(pMyInfos != NULL) {
    	if(strstr(pMyInfos, sMyInfo) == NULL ) {
            if(ui32MyInfosSize < ui32MyInfosLen+szLen) {
                char * pOldBuf = pMyInfos;
#ifdef _WIN32
                pMyInfos = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, ui32MyInfosSize+MYINFOLISTSIZE+1);
#else
				pMyInfos = (char *)realloc(pOldBuf, ui32MyInfosSize+MYINFOLISTSIZE+1);
#endif
                if(pMyInfos == NULL) {
                    pMyInfos = pOldBuf;

					AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes for pMyInfos in clsUsers::AddBot2MyInfos\n", ui32MyInfosSize+MYINFOLISTSIZE+1);

                    return;
                }
                ui32MyInfosSize += MYINFOLISTSIZE;
            }
        	memcpy(pMyInfos+ui32MyInfosLen, sMyInfo, szLen);
            ui32MyInfosLen += (uint32_t)szLen;
            pMyInfos[ui32MyInfosLen] = '\0';
            ui32ZMyInfosTagLen = 0;
         }
    }
}
//---------------------------------------------------------------------------

void clsUsers::DelBotFromMyInfos(char * sMyInfo) {
	size_t szLen = strlen(sMyInfo);
	if(pMyInfosTag) {
		char * sMatch = strstr(pMyInfosTag,  sMyInfo);
	    if(sMatch) {
    		memmove(sMatch, sMatch+szLen, ui32MyInfosTagLen-((sMatch+(szLen-1))-pMyInfosTag));
        	ui32MyInfosTagLen -= (uint32_t)szLen;
        	ui32ZMyInfosTagLen = 0;
         }
    }
	if(pMyInfos) {
		char * sMatch = strstr(pMyInfos,  sMyInfo);
	    if(sMatch) {
    		memmove(sMatch, sMatch+szLen, ui32MyInfosLen-((sMatch+(szLen-1))-pMyInfos));
        	ui32MyInfosLen -= (uint32_t)szLen;
        	ui32ZMyInfosLen = 0;
         }
    }
}
//---------------------------------------------------------------------------

void clsUsers::Add2UserIP(User * pUser) {
    int iRet = sprintf(clsServerManager::pGlobalBuffer, "$%s %s$", pUser->sNick, pUser->sIP);
    if(CheckSprintf(iRet, clsServerManager::szGlobalBufferSize, "clsUsers::Add2UserIP") == false) {
        return;
    }

    if(ui32UserIPListSize < ui32UserIPListLen+iRet) {
        char * pOldBuf = pUserIPList;
#ifdef _WIN32
        pUserIPList = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, ui32UserIPListSize+IPLISTSIZE+1);
#else
		pUserIPList = (char *)realloc(pOldBuf, ui32UserIPListSize+IPLISTSIZE+1);
#endif
        if(pUserIPList == NULL) {
            pUserIPList = pOldBuf;
            pUser->ui32BoolBits |= User::BIT_ERROR;
            pUser->Close();

			AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in clsUsers::Add2UserIP\n", ui32UserIPListSize+IPLISTSIZE+1);

            return;
        }
        ui32UserIPListSize += IPLISTSIZE;
    }

    memcpy(pUserIPList+ui32UserIPListLen-1, clsServerManager::pGlobalBuffer+1, iRet-1);
    ui32UserIPListLen += iRet;

    pUserIPList[ui32UserIPListLen-2] = '$';
    pUserIPList[ui32UserIPListLen-1] = '|';
    pUserIPList[ui32UserIPListLen] = '\0';

    ui32ZUserIPListLen = 0;
}
//---------------------------------------------------------------------------

void clsUsers::DelFromUserIP(User * pUser) {
    int iRet = sprintf(clsServerManager::pGlobalBuffer, "$%s %s$", pUser->sNick, pUser->sIP);
    if(CheckSprintf(iRet, clsServerManager::szGlobalBufferSize, "clsUsers::DelFromUserIP") == false) {
        return;
    }

    pUserIPList[7] = '$';
    char * sFound = strstr(pUserIPList, clsServerManager::pGlobalBuffer);
    pUserIPList[7] = ' ';
    
	if(sFound != NULL) {
        memmove(sFound+1, sFound+(iRet+1), ui32UserIPListLen-((sFound+iRet)-pUserIPList));
        ui32UserIPListLen -= iRet;
        ui32ZUserIPListLen = 0;
    }
}
//---------------------------------------------------------------------------

void clsUsers::Add2RecTimes(User * pUser) {
    time_t tmAccTime;
    time(&tmAccTime);

    if(clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::NOUSRSAMEIP) == true || (tmAccTime-pUser->tLoginTime) >= clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_RECONN_TIME]) {
        return;
    }

    RecTime * pNewRecTime = new (std::nothrow) RecTime(pUser->ui128IpHash);

	if(pNewRecTime == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pNewRecTime in clsUsers::Add2RecTimes\n");
        return;
	}

#ifdef _WIN32
    pNewRecTime->sNick = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, pUser->ui8NickLen+1);
#else
	pNewRecTime->sNick = (char *)malloc(pUser->ui8NickLen+1);
#endif
	if(pNewRecTime->sNick == NULL) {
        delete pNewRecTime;

        AppendDebugLogFormat("[MEM] Cannot allocate %" PRIu8 " bytes in clsUsers::Add2RecTimes\n", pUser->ui8NickLen+1);

        return;
    }

    memcpy(pNewRecTime->sNick, pUser->sNick, pUser->ui8NickLen);
    pNewRecTime->sNick[pUser->ui8NickLen] = '\0';

	pNewRecTime->ui64DisConnTick = clsServerManager::ui64ActualTick-(tmAccTime-pUser->tLoginTime);
    pNewRecTime->ui32NickHash = pUser->ui32NickHash;

    pNewRecTime->pNext = pRecTimeList;

	if(pRecTimeList != NULL) {
		pRecTimeList->pPrev = pNewRecTime;
	}

	pRecTimeList = pNewRecTime;
}
//---------------------------------------------------------------------------

bool clsUsers::CheckRecTime(User * pUser) {
    RecTime * pCur = NULL,
        * pNext = pRecTimeList;

    while(pNext != NULL) {
        pCur = pNext;
        pNext = pCur->pNext;

        // check expires...
        if(pCur->ui64DisConnTick+clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_RECONN_TIME] <= clsServerManager::ui64ActualTick) {
#ifdef _WIN32
            if(pCur->sNick != NULL) {
                if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pCur->sNick) == 0) {
                    AppendDebugLog("%s - [MEM] Cannot deallocate pCur->sNick in clsUsers::CheckRecTime\n");
                }
            }
#else
			free(pCur->sNick);
#endif

            if(pCur->pPrev == NULL) {
                if(pCur->pNext == NULL) {
                    pRecTimeList = NULL;
                } else {
                    pCur->pNext->pPrev = NULL;
                    pRecTimeList = pCur->pNext;
                }
            } else if(pCur->pNext == NULL) {
                pCur->pPrev->pNext = NULL;
            } else {
                pCur->pPrev->pNext = pCur->pNext;
                pCur->pNext->pPrev = pCur->pPrev;
            }

            delete pCur;
            continue;
        }

        if(pCur->ui32NickHash == pUser->ui32NickHash && memcmp(pCur->ui128IpHash, pUser->ui128IpHash, 16) == 0 && strcasecmp(pCur->sNick, pUser->sNick) == 0) {
            pUser->SendFormat("clsUsers::CheckRecTime", false, "<%s> %s %" PRIu64 " %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_PLEASE_WAIT],
                (pCur->ui64DisConnTick+clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_RECONN_TIME])-clsServerManager::ui64ActualTick, clsLanguageManager::mPtr->sTexts[LAN_SECONDS_BEFORE_RECONN]);

            return true;
        }
    }

    return false;
}
//---------------------------------------------------------------------------
