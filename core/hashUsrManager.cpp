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
#include "hashUsrManager.h"
//---------------------------------------------------------------------------
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
clsHashManager * clsHashManager::mPtr = NULL;
//---------------------------------------------------------------------------

clsHashManager::clsHashManager() {
    memset(pNickTable, 0, sizeof(pNickTable));
    memset(pIpTable, 0, sizeof(pIpTable));

    //Memo("clsHashManager created");
}
//---------------------------------------------------------------------------

clsHashManager::~clsHashManager() {
    //Memo("clsHashManager destroyed");
    for(uint32_t ui32i = 0; ui32i < 65536; ui32i++) {
		IpTableItem * cur = NULL,
            * next = pIpTable[ui32i];
        
        while(next != NULL) {
            cur = next;
            next = cur->pNext;
        
            delete cur;
		}
    }
}
//---------------------------------------------------------------------------

bool clsHashManager::Add(User * u) {
    uint16_t ui16dx = 0;
    memcpy(&ui16dx, &u->ui32NickHash, sizeof(uint16_t));

    if(pNickTable[ui16dx] != NULL) {
        pNickTable[ui16dx]->pHashTablePrev = u;
        u->pHashTableNext = pNickTable[ui16dx];
    }

    pNickTable[ui16dx] = u;

    if(pIpTable[u->ui16IpTableIdx] == NULL) {
        pIpTable[u->ui16IpTableIdx] = new (std::nothrow) IpTableItem();

        if(pIpTable[u->ui16IpTableIdx] == NULL) {
            u->ui32BoolBits |= User::BIT_ERROR;
            u->Close();

            AppendDebugLog("%s - [MEM] Cannot allocate IpTableItem in clsHashManager::Add\n");
            return false;
        }

        pIpTable[u->ui16IpTableIdx]->pNext = NULL;
        pIpTable[u->ui16IpTableIdx]->pPrev = NULL;

        pIpTable[u->ui16IpTableIdx]->pFirstUser = u;
		pIpTable[u->ui16IpTableIdx]->ui16Count = 1;

        return true;
    }

    IpTableItem * cur = NULL,
        * next = pIpTable[u->ui16IpTableIdx];

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

        if(memcmp(cur->pFirstUser->ui128IpHash, u->ui128IpHash, 16) == 0) {
            cur->pFirstUser->pHashIpTablePrev = u;
            u->pHashIpTableNext = cur->pFirstUser;
            cur->pFirstUser = u;
			cur->ui16Count++;

            return true;
        }
    }

    cur = new (std::nothrow) IpTableItem();

    if(cur == NULL) {
        u->ui32BoolBits |= User::BIT_ERROR;
        u->Close();

		AppendDebugLog("%s - [MEM] Cannot allocate IpTableItem2 in clsHashManager::Add\n");
		return false;
    }

    cur->pFirstUser = u;
	cur->ui16Count = 1;

    cur->pNext = pIpTable[u->ui16IpTableIdx];
    cur->pPrev = NULL;

    pIpTable[u->ui16IpTableIdx]->pPrev = cur;
    pIpTable[u->ui16IpTableIdx] = cur;

    return true;
}
//---------------------------------------------------------------------------

void clsHashManager::Remove(User * u) {
    if(u->pHashTablePrev == NULL) {
        uint16_t ui16dx = 0;
        memcpy(&ui16dx, &u->ui32NickHash, sizeof(uint16_t));

        if(u->pHashTableNext == NULL) {
            pNickTable[ui16dx] = NULL;
        } else {
            u->pHashTableNext->pHashTablePrev = NULL;
            pNickTable[ui16dx] = u->pHashTableNext;
        }
    } else if(u->pHashTableNext == NULL) {
        u->pHashTablePrev->pHashTableNext = NULL;
    } else {
        u->pHashTablePrev->pHashTableNext = u->pHashTableNext;
        u->pHashTableNext->pHashTablePrev = u->pHashTablePrev;
    }

    u->pHashTablePrev = NULL;
    u->pHashTableNext = NULL;

	if(u->pHashIpTablePrev == NULL) {
        IpTableItem * cur = NULL,
            * next = pIpTable[u->ui16IpTableIdx];
    
        while(next != NULL) {
            cur = next;
            next = cur->pNext;

            if(memcmp(cur->pFirstUser->ui128IpHash, u->ui128IpHash, 16) == 0) {
				cur->ui16Count--;

                if(u->pHashIpTableNext == NULL) {
                    if(cur->pPrev == NULL) {
                        if(cur->pNext == NULL) {
                            pIpTable[u->ui16IpTableIdx] = NULL;
                        } else {
                            cur->pNext->pPrev = NULL;
                            pIpTable[u->ui16IpTableIdx] = cur->pNext;
                        }
                    } else if(cur->pNext == NULL) {
                        cur->pPrev->pNext = NULL;
                    } else {
                        cur->pPrev->pNext = cur->pNext;
                        cur->pNext->pPrev = cur->pPrev;
                    }

                    delete cur;
                } else {
                    u->pHashIpTableNext->pHashIpTablePrev = NULL;
                    cur->pFirstUser = u->pHashIpTableNext;
                }

                u->pHashIpTablePrev = NULL;
                u->pHashIpTableNext = NULL;

                return;
            }
        }
    } else if(u->pHashIpTableNext == NULL) {
        u->pHashIpTablePrev->pHashIpTableNext = NULL;
    } else {
        u->pHashIpTablePrev->pHashIpTableNext = u->pHashIpTableNext;
        u->pHashIpTableNext->pHashIpTablePrev = u->pHashIpTablePrev;
    }

    u->pHashIpTablePrev = NULL;
    u->pHashIpTableNext = NULL;

    IpTableItem * cur = NULL,
        * next = pIpTable[u->ui16IpTableIdx];

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

        if(memcmp(cur->pFirstUser->ui128IpHash, u->ui128IpHash, 16) == 0) {
			cur->ui16Count--;

            return;
        }
    }
}
//---------------------------------------------------------------------------

User * clsHashManager::FindUser(char * sNick, const size_t &szNickLen) {
    uint32_t ui32Hash = HashNick(sNick, szNickLen);

    uint16_t ui16dx = 0;
    memcpy(&ui16dx, &ui32Hash, sizeof(uint16_t));

    User * next = pNickTable[ui16dx];

    // pointer exists ? Then we need look for nick 
    if(next != NULL) {
        User * cur = NULL;

        while(next != NULL) {
            cur = next;
            next = cur->pHashTableNext;

            // we are looking for duplicate string
			if(cur->ui32NickHash == ui32Hash && cur->ui8NickLen == szNickLen && strcasecmp(cur->sNick, sNick) == 0) {
                return cur;
            }
        }            
    }
    
    // no equal hash found, we dont have the nick in list
    return NULL;
}
//---------------------------------------------------------------------------

User * clsHashManager::FindUser(User * u) {
    uint16_t ui16dx = 0;
    memcpy(&ui16dx, &u->ui32NickHash, sizeof(uint16_t));

    User * next = pNickTable[ui16dx];

    // pointer exists ? Then we need look for nick
    if(next != NULL) { 
        User * cur = NULL;

        while(next != NULL) {
            cur = next;
            next = cur->pHashTableNext;

            // we are looking for duplicate string
			if(cur->ui32NickHash == u->ui32NickHash && cur->ui8NickLen == u->ui8NickLen && strcasecmp(cur->sNick, u->sNick) == 0) {
                return cur;
            }
        }            
    }
    
    // no equal hash found, we dont have the nick in list
    return NULL;
}
//---------------------------------------------------------------------------

User * clsHashManager::FindUser(const uint8_t * ui128IpHash) {
    uint16_t ui16IpTableIdx = 0;

    if(IN6_IS_ADDR_V4MAPPED((const in6_addr *)ui128IpHash)) {
        ui16IpTableIdx = ui128IpHash[14] * ui128IpHash[15];
    } else {
        ui16IpTableIdx = GetIpTableIdx(ui128IpHash);
    }

	IpTableItem * cur = NULL,
        * next = pIpTable[ui16IpTableIdx];

    while(next != NULL) {
		cur = next;
        next = cur->pNext;

        if(memcmp(cur->pFirstUser->ui128IpHash, ui128IpHash, 16) == 0) {
            return cur->pFirstUser;
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

uint32_t clsHashManager::GetUserIpCount(User * u) const {
	IpTableItem * cur = NULL,
        * next = pIpTable[u->ui16IpTableIdx];

	while(next != NULL) {
		cur = next;
		next = cur->pNext;

        if(memcmp(cur->pFirstUser->ui128IpHash, u->ui128IpHash, 16) == 0) {
            return cur->ui16Count;
        }
	}

	return 0;
}
//---------------------------------------------------------------------------
