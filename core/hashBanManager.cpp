/*
 * PtokaX - hub server for Direct Connect peer to peer network.

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
#include "hashBanManager.h"
//---------------------------------------------------------------------------
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
#ifdef _BUILD_GUI
	#include "../gui.win/BansDialog.h"
    #include "../gui.win/RangeBansDialog.h"
#endif
//---------------------------------------------------------------------------
clsBanManager * clsBanManager::mPtr = NULL;
//---------------------------------------------------------------------------

BanItem::BanItem(void) : tTempBanExpire(0), ui32NickHash(0), sNick(NULL), sReason(NULL), sBy(NULL), pPrev(NULL), pNext(NULL), pHashNickTablePrev(NULL), pHashNickTableNext(NULL),
	pHashIpTablePrev(NULL), pHashIpTableNext(NULL), ui8Bits(0) {
    sIp[0] = '\0';

    memset(ui128IpHash, 0, 16);
}
//---------------------------------------------------------------------------

BanItem::~BanItem(void) {
#ifdef _WIN32
    if(sNick != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sNick) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate sNick in BanItem::~BanItem\n", 0);
        }
    }
#else
	free(sNick);
#endif

#ifdef _WIN32
    if(sReason != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sReason) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sReason in BanItem::~BanItem\n", 0);
        }
    }
#else
	free(sReason);
#endif

#ifdef _WIN32
    if(sBy != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sBy) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sBy in BanItem::~BanItem\n", 0);
        }
    }
#else
	free(sBy);
#endif
}
//---------------------------------------------------------------------------

RangeBanItem::RangeBanItem(void) : tTempBanExpire(0), sReason(NULL), sBy(NULL), pPrev(NULL), pNext(NULL), ui8Bits(0) {
    sIpFrom[0] = '\0';
    sIpTo[0] = '\0';

    memset(ui128FromIpHash, 0, 16);
    memset(ui128ToIpHash, 0, 16);
}
//---------------------------------------------------------------------------

RangeBanItem::~RangeBanItem(void) {
#ifdef _WIN32
    if(sReason != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sReason) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sReason in RangeBanItem::~RangeBanItem\n", 0);
        }
    }
#else
	free(sReason);
#endif

#ifdef _WIN32
    if(sBy != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sBy) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sBy in RangeBanItem::~RangeBanItem\n", 0);
        }
    }
#else
	free(sBy);
#endif
}
//---------------------------------------------------------------------------

clsBanManager::clsBanManager(void) : ui32SaveCalled(0), pTempBanListS(NULL), pTempBanListE(NULL), pPermBanListS(NULL), pPermBanListE(NULL),
	pRangeBanListS(NULL), pRangeBanListE(NULL) {
    memset(pNickTable, 0, sizeof(pNickTable));
    memset(pIpTable, 0, sizeof(pIpTable));
}
//---------------------------------------------------------------------------

clsBanManager::~clsBanManager(void) {
    BanItem * curBan = NULL,
        * nextBan = pPermBanListS;

    while(nextBan != NULL) {
        curBan = nextBan;
		nextBan = curBan->pNext;

		delete curBan;
	}

    nextBan = pTempBanListS;

    while(nextBan != NULL) {
        curBan = nextBan;
		nextBan = curBan->pNext;

		delete curBan;
	}
    
    RangeBanItem * curRangeBan = NULL,
        * nextRangeBan = pRangeBanListS;

    while(nextRangeBan != NULL) {
        curRangeBan = nextRangeBan;
		nextRangeBan = curRangeBan->pNext;

		delete curRangeBan;
	}

    IpTableItem * cur = NULL, * next = NULL;

    for(uint32_t ui32i = 0; ui32i < 65536; ui32i++) {
        next = pIpTable[ui32i];
        
        while(next != NULL) {
            cur = next;
            next = cur->pNext;
        
            delete cur;
		}
    }
}
//---------------------------------------------------------------------------

bool clsBanManager::Add(BanItem * Ban) {
	if(Add2Table(Ban) == false) {
        return false;
    }

    if(((Ban->ui8Bits & PERM) == PERM) == true) {
		if(pPermBanListE == NULL) {
			pPermBanListS = Ban;
			pPermBanListE = Ban;
		} else {
			pPermBanListE->pNext = Ban;
			Ban->pPrev = pPermBanListE;
			pPermBanListE = Ban;
		}
    } else {
		if(pTempBanListE == NULL) {
			pTempBanListS = Ban;
			pTempBanListE = Ban;
		} else {
			pTempBanListE->pNext = Ban;
			Ban->pPrev = pTempBanListE;
			pTempBanListE = Ban;
		}
    }

#ifdef _BUILD_GUI
	if(clsBansDialog::mPtr != NULL) {
        clsBansDialog::mPtr->AddBan(Ban);
    }
#endif

	return true;
}
//---------------------------------------------------------------------------

bool clsBanManager::Add2Table(BanItem *Ban) {
	if(((Ban->ui8Bits & IP) == IP) == true) {
		if(Add2IpTable(Ban) == false) {
            return false;
        }
    }

    if(((Ban->ui8Bits & NICK) == NICK) == true) {
		Add2NickTable(Ban);
    }

    return true;
}
//---------------------------------------------------------------------------

void clsBanManager::Add2NickTable(BanItem *Ban) {
    uint16_t ui16dx = 0;
    memcpy(&ui16dx, &Ban->ui32NickHash, sizeof(uint16_t));

    if(pNickTable[ui16dx] != NULL) {
        pNickTable[ui16dx]->pHashNickTablePrev = Ban;
        Ban->pHashNickTableNext = pNickTable[ui16dx];
    }

    pNickTable[ui16dx] = Ban;
}
//---------------------------------------------------------------------------

bool clsBanManager::Add2IpTable(BanItem *Ban) {
    uint16_t ui16IpTableIdx = 0;

    if(Ban->ui128IpHash[10] == 255 && Ban->ui128IpHash[11] == 255 && memcmp(Ban->ui128IpHash, "\0\0\0\0\0\0\0\0\0\0", 10) == 0) {
        ui16IpTableIdx = Ban->ui128IpHash[14] * Ban->ui128IpHash[15];
    } else {
        ui16IpTableIdx = GetIpTableIdx(Ban->ui128IpHash);
    }
    
    if(pIpTable[ui16IpTableIdx] == NULL) {
		pIpTable[ui16IpTableIdx] = new (std::nothrow) IpTableItem();

        if(pIpTable[ui16IpTableIdx] == NULL) {
			AppendDebugLog("%s - [MEM] Cannot allocate IpTableItem in clsBanManager::Add2IpTable\n", 0);
            return false;
        }

        pIpTable[ui16IpTableIdx]->pNext = NULL;
        pIpTable[ui16IpTableIdx]->pPrev = NULL;

        pIpTable[ui16IpTableIdx]->pFirstBan = Ban;

        return true;
    }

    IpTableItem * cur = NULL,
        * next = pIpTable[ui16IpTableIdx];

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

		if(memcmp(cur->pFirstBan->ui128IpHash, Ban->ui128IpHash, 16) == 0) {
			cur->pFirstBan->pHashIpTablePrev = Ban;
			Ban->pHashIpTableNext = cur->pFirstBan;
            cur->pFirstBan = Ban;

            return true;
        }
    }

    cur = new (std::nothrow) IpTableItem();

    if(cur == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate IpTableBans2 in clsBanManager::Add2IpTable\n", 0);
        return false;
    }

    cur->pFirstBan = Ban;

    cur->pNext = pIpTable[ui16IpTableIdx];
    cur->pPrev = NULL;

    pIpTable[ui16IpTableIdx]->pPrev = cur;
    pIpTable[ui16IpTableIdx] = cur;

    return true;
}
//---------------------------------------------------------------------------

#ifdef _BUILD_GUI
void clsBanManager::Rem(BanItem * Ban, const bool &bFromGui/* = false*/) {
#else
void clsBanManager::Rem(BanItem * Ban, const bool &/*bFromGui = false*/) {
#endif
	RemFromTable(Ban);

    if(((Ban->ui8Bits & PERM) == PERM) == true) {
		if(Ban->pPrev == NULL) {
			if(Ban->pNext == NULL) {
				pPermBanListS = NULL;
				pPermBanListE = NULL;
			} else {
				Ban->pNext->pPrev = NULL;
				pPermBanListS = Ban->pNext;
			}
		} else if(Ban->pNext == NULL) {
			Ban->pPrev->pNext = NULL;
			pPermBanListE = Ban->pPrev;
		} else {
			Ban->pPrev->pNext = Ban->pNext;
			Ban->pNext->pPrev = Ban->pPrev;
		}
    } else {
        if(Ban->pPrev == NULL) {
			if(Ban->pNext == NULL) {
				pTempBanListS = NULL;
				pTempBanListE = NULL;
			} else {
				Ban->pNext->pPrev = NULL;
				pTempBanListS = Ban->pNext;
			   }
		} else if(Ban->pNext == NULL) {
			Ban->pPrev->pNext = NULL;
			pTempBanListE = Ban->pPrev;
		} else {
			Ban->pPrev->pNext = Ban->pNext;
			Ban->pNext->pPrev = Ban->pPrev;
		}
    }

#ifdef _BUILD_GUI
    if(bFromGui == false && clsBansDialog::mPtr != NULL) {
        clsBansDialog::mPtr->RemoveBan(Ban);
    }
#endif
}
//---------------------------------------------------------------------------

void clsBanManager::RemFromTable(BanItem *Ban) {
    if(((Ban->ui8Bits & IP) == IP) == true) {
		RemFromIpTable(Ban);
    }

    if(((Ban->ui8Bits & NICK) == NICK) == true) {
		RemFromNickTable(Ban);
    }
}
//---------------------------------------------------------------------------

void clsBanManager::RemFromNickTable(BanItem *Ban) {
    if(Ban->pHashNickTablePrev == NULL) {
        uint16_t ui16dx = 0;
        memcpy(&ui16dx, &Ban->ui32NickHash, sizeof(uint16_t));

        if(Ban->pHashNickTableNext == NULL) {
            pNickTable[ui16dx] = NULL;
        } else {
            Ban->pHashNickTableNext->pHashNickTablePrev = NULL;
            pNickTable[ui16dx] = Ban->pHashNickTableNext;
        }
    } else if(Ban->pHashNickTableNext == NULL) {
        Ban->pHashNickTablePrev->pHashNickTableNext = NULL;
    } else {
        Ban->pHashNickTablePrev->pHashNickTableNext = Ban->pHashNickTableNext;
        Ban->pHashNickTableNext->pHashNickTablePrev = Ban->pHashNickTablePrev;
    }

    Ban->pHashNickTablePrev = NULL;
    Ban->pHashNickTableNext = NULL;
}
//---------------------------------------------------------------------------

void clsBanManager::RemFromIpTable(BanItem *Ban) {
    uint16_t ui16IpTableIdx = 0;

    if(Ban->ui128IpHash[10] == 255 && Ban->ui128IpHash[11] == 255 && memcmp(Ban->ui128IpHash, "\0\0\0\0\0\0\0\0\0\0", 10) == 0) {
        ui16IpTableIdx = Ban->ui128IpHash[14] * Ban->ui128IpHash[15];
    } else {
        ui16IpTableIdx = GetIpTableIdx(Ban->ui128IpHash);
    }

	if(Ban->pHashIpTablePrev == NULL) {
        IpTableItem * cur = NULL,
            * next = pIpTable[ui16IpTableIdx];

        while(next != NULL) {
            cur = next;
            next = cur->pNext;

			if(memcmp(cur->pFirstBan->ui128IpHash, Ban->ui128IpHash, 16) == 0) {
				if(Ban->pHashIpTableNext == NULL) {
					if(cur->pPrev == NULL) {
						if(cur->pNext == NULL) {
                            pIpTable[ui16IpTableIdx] = NULL;
						} else {
							cur->pNext->pPrev = NULL;
                            pIpTable[ui16IpTableIdx] = cur->pNext;
                        }
					} else if(cur->pNext == NULL) {
						cur->pPrev->pNext = NULL;
					} else {
						cur->pPrev->pNext = cur->pNext;
                        cur->pNext->pPrev = cur->pPrev;
                    }

                    delete cur;
				} else {
					Ban->pHashIpTableNext->pHashIpTablePrev = NULL;
                    cur->pFirstBan = Ban->pHashIpTableNext;
                }

                break;
            }
        }
	} else if(Ban->pHashIpTableNext == NULL) {
		Ban->pHashIpTablePrev->pHashIpTableNext = NULL;
	} else {
        Ban->pHashIpTablePrev->pHashIpTableNext = Ban->pHashIpTableNext;
        Ban->pHashIpTableNext->pHashIpTablePrev = Ban->pHashIpTablePrev;
    }

    Ban->pHashIpTablePrev = NULL;
    Ban->pHashIpTableNext = NULL;
}
//---------------------------------------------------------------------------

BanItem* clsBanManager::Find(BanItem *Ban) {
	if(pTempBanListS != NULL) {
        time_t acc_time;
        time(&acc_time);

		BanItem * curBan = NULL,
            * nextBan = pTempBanListS;

        while(nextBan != NULL) {
            curBan = nextBan;
    		nextBan = curBan->pNext;

            if(acc_time > curBan->tTempBanExpire) {
				Rem(curBan);
				delete curBan;

                continue;
            }

			if(curBan == Ban) {
				return curBan;
			}
		}
    }
    
	if(pPermBanListS != NULL) {
		BanItem * curBan = NULL,
            * nextBan = pPermBanListS;

        while(nextBan != NULL) {
            curBan = nextBan;
    		nextBan = curBan->pNext;

			if(curBan == Ban) {
				return curBan;
			}
        }
	}

	return NULL;
}
//---------------------------------------------------------------------------

void clsBanManager::Remove(BanItem *Ban) {
	if(pTempBanListS != NULL) {
        time_t acc_time;
        time(&acc_time);

		BanItem * curBan = NULL,
            * nextBan = pTempBanListS;

        while(nextBan != NULL) {
            curBan = nextBan;
    		nextBan = curBan->pNext;

            if(acc_time > curBan->tTempBanExpire) {
				Rem(curBan);
				delete curBan;

                continue;
            }

			if(curBan == Ban) {
				Rem(Ban);
				delete Ban;

				return;
			}
		}
    }
    
	if(pPermBanListS != NULL) {
		BanItem * curBan = NULL,
            * nextBan = pPermBanListS;

        while(nextBan != NULL) {
            curBan = nextBan;
    		nextBan = curBan->pNext;

			if(curBan == Ban) {
				Rem(Ban);
				delete Ban;

				return;
			}
        }
	}
}
//---------------------------------------------------------------------------

void clsBanManager::AddRange(RangeBanItem *RangeBan) {
    if(pRangeBanListE == NULL) {
    	pRangeBanListS = RangeBan;
    	pRangeBanListE = RangeBan;
    } else {
    	pRangeBanListE->pNext = RangeBan;
        RangeBan->pPrev = pRangeBanListE;
        pRangeBanListE = RangeBan;
    }

#ifdef _BUILD_GUI
    if(clsRangeBansDialog::mPtr != NULL) {
        clsRangeBansDialog::mPtr->AddRangeBan(RangeBan);
    }
#endif
}
//---------------------------------------------------------------------------

#ifdef _BUILD_GUI
void clsBanManager::RemRange(RangeBanItem *RangeBan, const bool &bFromGui/* = false*/) {
#else
void clsBanManager::RemRange(RangeBanItem *RangeBan, const bool &/*bFromGui = false*/) {
#endif
    if(RangeBan->pPrev == NULL) {
        if(RangeBan->pNext == NULL) {
            pRangeBanListS = NULL;
            pRangeBanListE = NULL;
        } else {
            RangeBan->pNext->pPrev = NULL;
            pRangeBanListS = RangeBan->pNext;
        }
    } else if(RangeBan->pNext == NULL) {
        RangeBan->pPrev->pNext = NULL;
        pRangeBanListE = RangeBan->pPrev;
    } else {
        RangeBan->pPrev->pNext = RangeBan->pNext;
        RangeBan->pNext->pPrev = RangeBan->pPrev;
    }

#ifdef _BUILD_GUI
    if(bFromGui == false && clsRangeBansDialog::mPtr != NULL) {
        clsRangeBansDialog::mPtr->RemoveRangeBan(RangeBan);
    }
#endif
}
//---------------------------------------------------------------------------

RangeBanItem* clsBanManager::FindRange(RangeBanItem *RangeBan) {
	if(pRangeBanListS != NULL) {
        time_t acc_time;
        time(&acc_time);

		RangeBanItem * curBan = NULL,
            * nextBan = pRangeBanListS;

		while(nextBan != NULL) {
            curBan = nextBan;
			nextBan = curBan->pNext;

			if(((curBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP) == true && acc_time > curBan->tTempBanExpire) {
				RemRange(curBan);
				delete curBan;

                continue;
			}

			if(curBan == RangeBan) {
				return curBan;
			}
		}
	}

	return NULL;
}
//---------------------------------------------------------------------------

void clsBanManager::RemoveRange(RangeBanItem *RangeBan) {
	if(pRangeBanListS != NULL) {
        time_t acc_time;
        time(&acc_time);

		RangeBanItem * curBan = NULL,
            * nextBan = pRangeBanListS;

		while(nextBan != NULL) {
            curBan = nextBan;
    		nextBan = curBan->pNext;

			if(((curBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP) == true && acc_time > curBan->tTempBanExpire) {
				RemRange(curBan);
                delete curBan;

                continue;
			}

			if(curBan == RangeBan) {
				RemRange(RangeBan);
				delete RangeBan;

				return;
			}
		}
	}
}
//---------------------------------------------------------------------------

BanItem* clsBanManager::FindNick(User* u) {
    uint16_t ui16dx = 0;
    memcpy(&ui16dx, &u->ui32NickHash, sizeof(uint16_t));

    time_t acc_time;
    time(&acc_time);

    BanItem * cur = NULL,
        * next = pNickTable[ui16dx];

    while(next != NULL) {
        cur = next;
        next = cur->pHashNickTableNext;

		if(cur->ui32NickHash == u->ui32NickHash && strcasecmp(cur->sNick, u->sNick) == 0) {
            // PPK ... check if temban expired
			if(((cur->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP) == true) {
                if(acc_time >= cur->tTempBanExpire) {
					Rem(cur);
                    delete cur;

					continue;
                }
            }
            return cur;
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

BanItem* clsBanManager::FindIP(User* u) {
    time_t acc_time;
    time(&acc_time);

    IpTableItem * cur = NULL,
        * next = pIpTable[u->ui16IpTableIdx];

	BanItem * curBan = NULL, * nextBan = NULL;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

		if(memcmp(cur->pFirstBan->ui128IpHash, u->ui128IpHash, 16) == 0) {
			nextBan = cur->pFirstBan;

			while(nextBan != NULL) {
                curBan = nextBan;
                nextBan = curBan->pHashIpTableNext;
                
                // PPK ... check if temban expired
				if(((curBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP) == true) {
                    if(acc_time >= curBan->tTempBanExpire) {
						Rem(curBan);
                        delete curBan;
    
    					continue;
                    }
                }

                return curBan;
            }
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

RangeBanItem* clsBanManager::FindRange(User* u) {
    time_t acc_time;
    time(&acc_time);

    RangeBanItem * cur = NULL,
        * next = pRangeBanListS;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

        if(memcmp(cur->ui128FromIpHash, u->ui128IpHash, 16) <= 0 && memcmp(cur->ui128ToIpHash, u->ui128IpHash, 16) >= 0) {
            // PPK ... check if temban expired
            if(((cur->ui8Bits & TEMP) == TEMP) == true) {
                if(acc_time >= cur->tTempBanExpire) {
                    RemRange(cur);
                    delete cur;

					continue;
                }
            }
            return cur;
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

BanItem* clsBanManager::FindFull(const uint8_t * ui128IpHash) {
    time_t acc_time;
    time(&acc_time);

    return FindFull(ui128IpHash, acc_time);
}
//---------------------------------------------------------------------------

BanItem* clsBanManager::FindFull(const uint8_t * ui128IpHash, const time_t &acc_time) {
    uint16_t ui16IpTableIdx = 0;

    if(ui128IpHash[10] == 255 && ui128IpHash[11] == 255 && memcmp(ui128IpHash, "\0\0\0\0\0\0\0\0\0\0", 10) == 0) {
        ui16IpTableIdx = ui128IpHash[14] * ui128IpHash[15];
    } else {
        ui16IpTableIdx = GetIpTableIdx(ui128IpHash);
    }

	IpTableItem * cur = NULL,
        * next = pIpTable[ui16IpTableIdx];

	BanItem * curBan = NULL, * nextBan = NULL, * fnd = NULL;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

        if(memcmp(cur->pFirstBan->ui128IpHash, ui128IpHash, 16) == 0) {
			nextBan = cur->pFirstBan;

            while(nextBan != NULL) {
                curBan = nextBan;
                nextBan = curBan->pHashIpTableNext;
        
                // PPK ... check if temban expired
				if(((curBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP) == true) {
                    if(acc_time >= curBan->tTempBanExpire) {
						Rem(curBan);
                        delete curBan;
    
    					continue;
                    }
                }
                    
				if(((curBan->ui8Bits & clsBanManager::FULL) == clsBanManager::FULL) == true) {
                    return curBan;
                } else if(fnd == NULL) {
                    fnd = curBan;
                }
            }
        }
    }

    return fnd;
}
//---------------------------------------------------------------------------

RangeBanItem* clsBanManager::FindFullRange(const uint8_t * ui128IpHash, const time_t &acc_time) {
    RangeBanItem * cur = NULL, * fnd = NULL,
        * next = pRangeBanListS;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

        if(memcmp(cur->ui128FromIpHash, ui128IpHash, 16) <= 0 && memcmp(cur->ui128ToIpHash, ui128IpHash, 16) >= 0) {
            // PPK ... check if temban expired
            if(((cur->ui8Bits & TEMP) == TEMP) == true) {
                if(acc_time >= cur->tTempBanExpire) {
                    RemRange(cur);
                    delete cur;

					continue;
                }
            }
            
            if(((cur->ui8Bits & FULL) == FULL) == true) {
                return cur;
            } else if(fnd == NULL) {
                fnd = cur;
            }
        }
    }

    return fnd;
}
//---------------------------------------------------------------------------

BanItem* clsBanManager::FindNick(char * sNick, const size_t &szNickLen) {
    uint32_t hash = HashNick(sNick, szNickLen);

    time_t acc_time;
    time(&acc_time);

    return FindNick(hash, acc_time, sNick);
}
//---------------------------------------------------------------------------

BanItem* clsBanManager::FindNick(const uint32_t &ui32Hash, const time_t &acc_time, char * sNick) {
    uint16_t ui16dx = 0;
    memcpy(&ui16dx, &ui32Hash, sizeof(uint16_t));

	BanItem * cur = NULL,
        * next = pNickTable[ui16dx];

    while(next != NULL) {
        cur = next;
        next = cur->pHashNickTableNext;

		if(cur->ui32NickHash == ui32Hash && strcasecmp(cur->sNick, sNick) == 0) {
            // PPK ... check if temban expired
			if(((cur->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP) == true) {
                if(acc_time >= cur->tTempBanExpire) {
					Rem(cur);
                    delete cur;

					continue;
                }
            }
            return cur;
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

BanItem* clsBanManager::FindIP(const uint8_t * ui128IpHash, const time_t &acc_time) {
    uint16_t ui16IpTableIdx = 0;

    if(ui128IpHash[10] == 255 && ui128IpHash[11] == 255 && memcmp(ui128IpHash, "\0\0\0\0\0\0\0\0\0\0", 10) == 0) {
        ui16IpTableIdx = ui128IpHash[14] * ui128IpHash[15];
    } else {
        ui16IpTableIdx = GetIpTableIdx(ui128IpHash);
    }

    IpTableItem * cur = NULL,
        * next = pIpTable[ui16IpTableIdx];

	BanItem * curBan = NULL, * nextBan = NULL;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

        if(memcmp(cur->pFirstBan->ui128IpHash, ui128IpHash, 16) == 0) {
			nextBan = cur->pFirstBan;

            while(nextBan != NULL) {
                curBan = nextBan;
                nextBan = curBan->pHashIpTableNext;

                // PPK ... check if temban expired
				if(((curBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP) == true) {
                    if(acc_time >= curBan->tTempBanExpire) {
						Rem(curBan);
                        delete curBan;
    
    					continue;
                    }
                }

                return curBan;
            }
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

RangeBanItem* clsBanManager::FindRange(const uint8_t * ui128IpHash, const time_t &acc_time) {
    RangeBanItem * cur = NULL,
        * next = pRangeBanListS;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

        // PPK ... check if temban expired
        if(((cur->ui8Bits & TEMP) == TEMP) == true) {
            if(acc_time >= cur->tTempBanExpire) {
                RemRange(cur);
                delete cur;

				continue;
            }
        }

        if(memcmp(cur->ui128FromIpHash, ui128IpHash, 16) <= 0 && memcmp(cur->ui128ToIpHash, ui128IpHash, 16) >= 0) {
            return cur;
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

RangeBanItem* clsBanManager::FindRange(const uint8_t * ui128FromHash, const uint8_t * ui128ToHash, const time_t &acc_time) {
    RangeBanItem * cur = NULL,
        * next = pRangeBanListS;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

        if(memcmp(cur->ui128FromIpHash, ui128FromHash, 16) == 0 && memcmp(cur->ui128ToIpHash, ui128ToHash, 16) == 0) {
            // PPK ... check if temban expired
            if(((cur->ui8Bits & TEMP) == TEMP) == true) {
                if(acc_time >= cur->tTempBanExpire) {
                    RemRange(cur);
                    delete cur;

					continue;
                }
            }

            return cur;
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

BanItem* clsBanManager::FindTempNick(char * sNick, const size_t &szNickLen) {
    uint32_t hash = HashNick(sNick, szNickLen);

    time_t acc_time;
    time(&acc_time);

    return FindTempNick(hash, acc_time, sNick);
}
//---------------------------------------------------------------------------

BanItem* clsBanManager::FindTempNick(const uint32_t &ui32Hash,  const time_t &acc_time, char * sNick) {
    uint16_t ui16dx = 0;
    memcpy(&ui16dx, &ui32Hash, sizeof(uint16_t));

	BanItem * cur = NULL,
        * next = pNickTable[ui16dx];

    while(next != NULL) {
        cur = next;
        next = cur->pHashNickTableNext;

		if(cur->ui32NickHash == ui32Hash && strcasecmp(cur->sNick, sNick) == 0) {
            // PPK ... check if temban expired
			if(((cur->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP) == true) {
                if(acc_time >= cur->tTempBanExpire) {
                    Rem(cur);
                    delete cur;

					continue;
                }
                return cur;
            }
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

BanItem* clsBanManager::FindTempIP(const uint8_t * ui128IpHash, const time_t &acc_time) {
    uint16_t ui16IpTableIdx = 0;

    if(ui128IpHash[10] == 255 && ui128IpHash[11] == 255 && memcmp(ui128IpHash, "\0\0\0\0\0\0\0\0\0\0", 10) == 0) {
        ui16IpTableIdx = ui128IpHash[14] * ui128IpHash[15];
    } else {
        ui16IpTableIdx = GetIpTableIdx(ui128IpHash);
    }

    IpTableItem * cur = NULL,
        * next = pIpTable[ui16IpTableIdx];

    BanItem * curBan = NULL, * nextBan = NULL;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

        if(memcmp(cur->pFirstBan->ui128IpHash, ui128IpHash, 16) == 0) {
			nextBan = cur->pFirstBan;

            while(nextBan != NULL) {
                curBan = nextBan;
                nextBan = curBan->pHashIpTableNext;
                
                // PPK ... check if temban expired
				if(((curBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP) == true) {
                    if(acc_time >= curBan->tTempBanExpire) {
						Rem(curBan);
                        delete curBan;
    
    					continue;
                    }

                    return curBan;
                }
            }
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

BanItem* clsBanManager::FindPermNick(char * sNick, const size_t &szNickLen) {
    uint32_t hash = HashNick(sNick, szNickLen);
    
	return FindPermNick(hash, sNick);
}
//---------------------------------------------------------------------------

BanItem* clsBanManager::FindPermNick(const uint32_t &ui32Hash, char * sNick) {
    uint16_t ui16dx = 0;
    memcpy(&ui16dx, &ui32Hash, sizeof(uint16_t));

    BanItem * cur = NULL,
        * next = pNickTable[ui16dx];

    while(next != NULL) {
        cur = next;
        next = cur->pHashNickTableNext;

		if(cur->ui32NickHash == ui32Hash && strcasecmp(cur->sNick, sNick) == 0) {
            if(((cur->ui8Bits & clsBanManager::PERM) == clsBanManager::PERM) == true) {
                return cur;
            }
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

BanItem* clsBanManager::FindPermIP(const uint8_t * ui128IpHash) {
    uint16_t ui16IpTableIdx = 0;

    if(ui128IpHash[10] == 255 && ui128IpHash[11] == 255 && memcmp(ui128IpHash, "\0\0\0\0\0\0\0\0\0\0", 10) == 0) {
        ui16IpTableIdx = ui128IpHash[14] * ui128IpHash[15];
    } else {
        ui16IpTableIdx = GetIpTableIdx(ui128IpHash);
    }

	IpTableItem * cur = NULL,
        * next = pIpTable[ui16IpTableIdx];

    BanItem * curBan = NULL, * nextBan = NULL;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

        if(memcmp(cur->pFirstBan->ui128IpHash, ui128IpHash, 16) == 0) {
            nextBan = cur->pFirstBan;

            while(nextBan != NULL) {
                curBan = nextBan;
                nextBan = curBan->pHashIpTableNext;
                
                if(((curBan->ui8Bits & clsBanManager::PERM) == clsBanManager::PERM) == true) {
                    return curBan;
                }
            }
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

void clsBanManager::Load(void) {
    double dVer;

#ifdef _WIN32
    TiXmlDocument doc((clsServerManager::sPath+"\\cfg\\BanList.xml").c_str());
#else
	TiXmlDocument doc((clsServerManager::sPath+"/cfg/BanList.xml").c_str());
#endif

    if(doc.LoadFile() == false) {
        if(doc.ErrorId() != TiXmlBase::TIXML_ERROR_OPENING_FILE && doc.ErrorId() != TiXmlBase::TIXML_ERROR_DOCUMENT_EMPTY) {
            char msg[2048];
            int imsgLen = sprintf(msg, "Error loading file BanList.xml. %s (Col: %d, Row: %d)", doc.ErrorDesc(), doc.Column(), doc.Row());
			CheckSprintf(imsgLen, 2048, "clsBanManager::Load");
#ifdef _BUILD_GUI
			::MessageBox(NULL, msg, g_sPtokaXTitle, MB_OK | MB_ICONERROR);
#else
			AppendLog(msg);
#endif
            exit(EXIT_FAILURE);
        }
    } else {
        TiXmlHandle cfg(&doc);
        TiXmlElement *banlist = cfg.FirstChild("BanList").Element();
        if(banlist != NULL) {
            if(banlist->Attribute("version", &dVer) == NULL) {
                return;
            }

            time_t acc_time;
            time(&acc_time);

            TiXmlNode *bans = banlist->FirstChild("Bans");
            if(bans != NULL) {
                TiXmlNode *child = NULL;
                while((child = bans->IterateChildren(child)) != NULL) {
                	const char *ip = NULL, *nick = NULL, *reason = NULL, *by = NULL;
					TiXmlNode *ban = child->FirstChild("Type");

					if(ban == NULL || (ban = ban->FirstChild()) == NULL) {
						continue;
					}

                    char type = atoi(ban->Value()) == 0 ? (char)0 : (char)1;

					if((ban = child->FirstChild("IP")) != NULL &&
                        (ban = ban->FirstChild()) != NULL) {
						ip = ban->Value();
					}

					if((ban = child->FirstChild("Nick")) != NULL &&
                        (ban = ban->FirstChild()) != NULL) {
						nick = ban->Value();
					}

					if((ban = child->FirstChild("Reason")) != NULL &&
                        (ban = ban->FirstChild()) != NULL) {
						reason = ban->Value();
					}

					if((ban = child->FirstChild("By")) != NULL &&
                        (ban = ban->FirstChild()) != NULL) {
						by = ban->Value();
					}

					if((ban = child->FirstChild("NickBan")) == NULL ||
                        (ban = ban->FirstChild()) == NULL) {
						continue;
					}

					bool nickban = (atoi(ban->Value()) == 0 ? false : true);

					if((ban = child->FirstChild("IpBan")) == NULL ||
                        (ban = ban->FirstChild()) == NULL) {
						continue;
					}

                    bool ipban = (atoi(ban->Value()) == 0 ? false : true);

					if((ban = child->FirstChild("FullIpBan")) == NULL ||
                        (ban = ban->FirstChild()) == NULL) {
						continue;
					}

                    bool fullipban = (atoi(ban->Value()) == 0 ? false : true);

                    BanItem * Ban = new (std::nothrow) BanItem();
                    if(Ban == NULL) {
						AppendDebugLog("%s - [MEM] Cannot allocate Ban in clsBanManager::Load\n", 0);
                    	exit(EXIT_FAILURE);
                    }

                    if(type == 0) {
                        Ban->ui8Bits |= PERM;
                    } else {
                        Ban->ui8Bits |= TEMP;
                    }

                    // PPK ... ipban
                    if(ipban == true) {
                        if(ip != NULL && HashIP(ip, Ban->ui128IpHash) == true) {
                            strcpy(Ban->sIp, ip);
                            Ban->ui8Bits |= IP;

                            if(fullipban == true) {
                                Ban->ui8Bits |= FULL;
							}
                        } else {
							delete Ban;

                            continue;
                        }
                    }

                    // PPK ... nickban
                    if(nickban == true) {
                        if(nick != NULL) {
                            size_t szNickLen = strlen(nick);
#ifdef _WIN32
                            Ban->sNick = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szNickLen+1);
#else
							Ban->sNick = (char *)malloc(szNickLen+1);
#endif
                            if(Ban->sNick == NULL) {
								AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sNick in clsBanManager::Load\n", (uint64_t)(szNickLen+1));

                                exit(EXIT_FAILURE);
                            }

                            memcpy(Ban->sNick, nick, szNickLen);
                            Ban->sNick[szNickLen] = '\0';
                            Ban->ui32NickHash = HashNick(Ban->sNick, strlen(Ban->sNick));
                            Ban->ui8Bits |= NICK;
                        } else {
                            delete Ban;

                            continue;
                        }
                    }

                    if(reason != NULL) {
                        size_t szReasonLen = strlen(reason);
                        if(szReasonLen > 255) {
                            szReasonLen = 255;
                        }
#ifdef _WIN32
                        Ban->sReason = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szReasonLen+1);
#else
						Ban->sReason = (char *)malloc(szReasonLen+1);
#endif
                        if(Ban->sReason == NULL) {
							AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sReason in clsBanManager::Load\n", (uint64_t)(szReasonLen+1));

                            exit(EXIT_FAILURE);
                        }

                        memcpy(Ban->sReason, reason, szReasonLen);
                        Ban->sReason[szReasonLen] = '\0';
                    }

                    if(by != NULL) {
                        size_t szByLen = strlen(by);
                        if(szByLen > 63) {
                            szByLen = 63;
                        }
#ifdef _WIN32
                        Ban->sBy = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szByLen+1);
#else
						Ban->sBy = (char *)malloc(szByLen+1);
#endif
                        if(Ban->sBy == NULL) {
                            AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sBy1 in clsBanManager::Load\n", (uint64_t)(szByLen+1));
                            exit(EXIT_FAILURE);
                        }

                        memcpy(Ban->sBy, by, szByLen);
                        Ban->sBy[szByLen] = '\0';
                    }

                    // PPK ... temp ban
                    if(((Ban->ui8Bits & TEMP) == TEMP) == true) {
                        if((ban = child->FirstChild("Expire")) == NULL ||
                            (ban = ban->FirstChild()) == NULL) {
                            delete Ban;

                            continue;
                        }
                        time_t expire = strtoul(ban->Value(), NULL, 10);

                        if(acc_time > expire) {
                            delete Ban;

							continue;
                        }

                        // PPK ... temp ban expiration
                        Ban->tTempBanExpire = expire;
                    }
                    
                    if(fullipban == true) {
                        Ban->ui8Bits |= FULL;
                    }

                    if(Add(Ban) == false) {
                        exit(EXIT_FAILURE);
                    }
                }
            }

            TiXmlNode *rangebans = banlist->FirstChild("RangeBans");
            if(rangebans != NULL) {
                TiXmlNode *child = NULL;
                while((child = rangebans->IterateChildren(child)) != NULL) {
                	const char *reason = NULL, *by = NULL;
					TiXmlNode *rangeban = child->FirstChild("Type");

					if(rangeban == NULL ||
                        (rangeban = rangeban->FirstChild()) == NULL) {
						continue;
					}

                    char type = atoi(rangeban->Value()) == 0 ? (char)0 : (char)1;

					if((rangeban = child->FirstChild("IpFrom")) == NULL ||
                        (rangeban = rangeban->FirstChild()) == NULL) {
						continue;
					}

                    const char *ipfrom = rangeban->Value();

					if((rangeban = child->FirstChild("IpTo")) == NULL ||
                        (rangeban = rangeban->FirstChild()) == NULL) {
						continue;
					}

                    const char *ipto = rangeban->Value();

					if((rangeban = child->FirstChild("Reason")) != NULL &&
                        (rangeban = rangeban->FirstChild()) != NULL) {
						reason = rangeban->Value();
					}

					if((rangeban = child->FirstChild("By")) != NULL &&
                        (rangeban = rangeban->FirstChild()) != NULL) {
						by = rangeban->Value();
					}

					if((rangeban = child->FirstChild("FullIpBan")) == NULL ||
                        (rangeban = rangeban->FirstChild()) == NULL) {
						continue;
					}

                    bool fullipban = (atoi(rangeban->Value()) == 0 ? false : true);

                    RangeBanItem * RangeBan = new (std::nothrow) RangeBanItem();
                    if(RangeBan == NULL) {
						AppendDebugLog("%s - [MEM] Cannot allocate RangeBan in clsBanManager::Load\n", 0);
                    	exit(EXIT_FAILURE);
                    }

                    if(type == 0) {
                        RangeBan->ui8Bits |= PERM;
                    } else {
                        RangeBan->ui8Bits |= TEMP;
                    }

                    // PPK ... fromip
                    if(HashIP(ipfrom, RangeBan->ui128FromIpHash) == true) {
                        strcpy(RangeBan->sIpFrom, ipfrom);
                    } else {
                        delete RangeBan;

                        continue;
                    }

                    // PPK ... toip
                    if(HashIP(ipto, RangeBan->ui128ToIpHash) == true && memcmp(RangeBan->ui128ToIpHash, RangeBan->ui128FromIpHash, 16) > 0) {
                        strcpy(RangeBan->sIpTo, ipto);
                    } else {
                        delete RangeBan;

                        continue;
                    }

                    if(reason != NULL) {
                        size_t szReasonLen = strlen(reason);
                        if(szReasonLen > 255) {
                            szReasonLen = 255;
                        }
#ifdef _WIN32
                        RangeBan->sReason = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szReasonLen+1);
#else
						RangeBan->sReason = (char *)malloc(szReasonLen+1);
#endif
                        if(RangeBan->sReason == NULL) {
							AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sReason3 in clsBanManager::Load\n", (uint64_t)(szReasonLen+1));
                            exit(EXIT_FAILURE);
                        }

                        memcpy(RangeBan->sReason, reason, szReasonLen);
                        RangeBan->sReason[szReasonLen] = '\0';
                    }

                    if(by != NULL) {
                        size_t szByLen = strlen(by);
                        if(szByLen > 63) {
                            szByLen = 63;
                        }
#ifdef _WIN32
                        RangeBan->sBy = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szByLen+1);
#else
						RangeBan->sBy = (char *)malloc(szByLen+1);
#endif
                        if(RangeBan->sBy == NULL) {
							AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sBy3 in clsBanManager::Load\n", (uint64_t)(szByLen+1));
                            exit(EXIT_FAILURE);
                        }

                        memcpy(RangeBan->sBy, by, szByLen);
                        RangeBan->sBy[szByLen] = '\0';
                    }

                    // PPK ... temp ban
                    if(((RangeBan->ui8Bits & TEMP) == TEMP) == true) {
                        if((rangeban = child->FirstChild("Expire")) == NULL ||
                            (rangeban = rangeban->FirstChild()) == NULL) {
                            delete RangeBan;

                            continue;
                        }
                        time_t expire = strtoul(rangeban->Value(), NULL, 10);

                        if(acc_time > expire) {
                            delete RangeBan;
							continue;
                        }

                        // PPK ... temp ban expiration
                        RangeBan->tTempBanExpire = expire;
                    }

                    if(fullipban == true) {
                        RangeBan->ui8Bits |= FULL;
                    }

                    AddRange(RangeBan);
                }
            }
        }
    }
}
//---------------------------------------------------------------------------

void clsBanManager::Save(bool bForce/* = false*/) {
    if(bForce == false) {
        // PPK ... we don't want to kill HDD with save after any change in banlist
        if(ui32SaveCalled < 100) {
            ui32SaveCalled++;
            return;
        }
    }
    
    ui32SaveCalled = 0;

#ifdef _WIN32
    TiXmlDocument doc((clsServerManager::sPath+"\\cfg\\BanList.xml").c_str());
#else
	TiXmlDocument doc((clsServerManager::sPath+"/cfg/BanList.xml").c_str());
#endif

    doc.InsertEndChild(TiXmlDeclaration("1.0", "windows-1252", "yes"));
    TiXmlElement banlist("BanList");
    banlist.SetDoubleAttribute("version", 2.0);

    TiXmlElement bans("Bans");

    if(pTempBanListS != NULL) {
        BanItem * cur = NULL,
            * next = pTempBanListS;

        while(next != NULL) {
            cur = next;
            next = cur->pNext;
           
            TiXmlElement type("Type");
            if(((cur->ui8Bits & PERM) == PERM) == true) {
                type.InsertEndChild(TiXmlText("0"));
            } else {
                type.InsertEndChild(TiXmlText("1"));
            }
            
            TiXmlElement ip("IP");
            ip.InsertEndChild(TiXmlText(cur->sIp[0] == '\0' ? "" : cur->sIp));
            
            TiXmlElement nick("Nick");
            nick.InsertEndChild(TiXmlText(cur->sNick == NULL ? "" : cur->sNick));
            
            TiXmlElement reason("Reason");
            reason.InsertEndChild(TiXmlText(cur->sReason == NULL ? "" : cur->sReason));
            
            TiXmlElement by("By");
            by.InsertEndChild(TiXmlText(cur->sBy == NULL ? "" : cur->sBy));
            
            TiXmlElement nickban("NickBan");
            if(((cur->ui8Bits & NICK) == NICK) == true) {
                nickban.InsertEndChild(TiXmlText("1"));
            } else {
                nickban.InsertEndChild(TiXmlText("0"));
            }
                        
            TiXmlElement ipban("IpBan");
            if(((cur->ui8Bits & IP) == IP) == true) {
                ipban.InsertEndChild(TiXmlText("1"));
            } else {
                ipban.InsertEndChild(TiXmlText("0"));
            }
            
            TiXmlElement fullipban("FullIpBan");
            if(((cur->ui8Bits & FULL) == FULL) == true) {
                fullipban.InsertEndChild(TiXmlText("1"));
            } else {
                fullipban.InsertEndChild(TiXmlText("0"));
            }
            
			TiXmlElement expire("Expire");
			expire.InsertEndChild(TiXmlText(string((uint32_t)cur->tTempBanExpire).c_str()));
            
            TiXmlElement ban("Ban");
			ban.InsertEndChild(type);
            ban.InsertEndChild(ip);
            ban.InsertEndChild(nick);
            ban.InsertEndChild(reason);
            ban.InsertEndChild(by);
            ban.InsertEndChild(nickban);
            ban.InsertEndChild(ipban);
            ban.InsertEndChild(fullipban);
            ban.InsertEndChild(expire);
            
            bans.InsertEndChild(ban);
        }
    }

    if(pPermBanListS != NULL) {
        BanItem * cur = NULL,
            * next = pPermBanListS;

        while(next != NULL) {
            cur = next;
            next = cur->pNext;
           
            TiXmlElement type("Type");
            if(((cur->ui8Bits & PERM) == PERM) == true) {
                type.InsertEndChild(TiXmlText("0"));
            } else {
                type.InsertEndChild(TiXmlText("1"));
            }
            
            TiXmlElement ip("IP");
            ip.InsertEndChild(TiXmlText(cur->sIp[0] == '\0' ? "" : cur->sIp));
            
            TiXmlElement nick("Nick");
            nick.InsertEndChild(TiXmlText(cur->sNick == NULL ? "" : cur->sNick));
            
            TiXmlElement reason("Reason");
            reason.InsertEndChild(TiXmlText(cur->sReason == NULL ? "" : cur->sReason));
            
            TiXmlElement by("By");
            by.InsertEndChild(TiXmlText(cur->sBy == NULL ? "" : cur->sBy));
            
            TiXmlElement nickban("NickBan");
            if(((cur->ui8Bits & NICK) == NICK) == true) {
                nickban.InsertEndChild(TiXmlText("1"));
            } else {
                nickban.InsertEndChild(TiXmlText("0"));
            }
            
            TiXmlElement ipban("IpBan");
            if(((cur->ui8Bits & IP) == IP) == true) {
                ipban.InsertEndChild(TiXmlText("1"));
            } else {
                ipban.InsertEndChild(TiXmlText("0"));
            }
            
            TiXmlElement fullipban("FullIpBan");
            if(((cur->ui8Bits & FULL) == FULL) == true) {
                fullipban.InsertEndChild(TiXmlText("1"));
            } else {
                fullipban.InsertEndChild(TiXmlText("0"));
            }
            
            TiXmlElement expire("Expire");
			expire.InsertEndChild(TiXmlText(string((uint32_t)cur->tTempBanExpire).c_str()));
            
            TiXmlElement ban("Ban");
            ban.InsertEndChild(type);
            ban.InsertEndChild(ip);
            ban.InsertEndChild(nick);
            ban.InsertEndChild(reason);
            ban.InsertEndChild(by);
            ban.InsertEndChild(nickban);
            ban.InsertEndChild(ipban);
            ban.InsertEndChild(fullipban);
            ban.InsertEndChild(expire);
            
            bans.InsertEndChild(ban);
        }
    }

    banlist.InsertEndChild(bans);

    TiXmlElement rangebans("RangeBans");

    if(pRangeBanListS != NULL) {
        RangeBanItem * cur = NULL,
            * next = pRangeBanListS;

        while(next != NULL) {
            cur = next;
            next = cur->pNext;
            
            TiXmlElement type("Type");
            if(((cur->ui8Bits & PERM) == PERM) == true) {
                type.InsertEndChild(TiXmlText("0"));
            } else {
                type.InsertEndChild(TiXmlText("1"));
            }
            
            TiXmlElement ipfrom("IpFrom");
            ipfrom.InsertEndChild(TiXmlText(cur->sIpFrom));
            
            TiXmlElement ipto("IpTo");
            ipto.InsertEndChild(TiXmlText(cur->sIpTo));
            
            TiXmlElement reason("Reason");
            reason.InsertEndChild(TiXmlText(cur->sReason == NULL ? "" : cur->sReason));
            
            TiXmlElement by("By");
            by.InsertEndChild(TiXmlText(cur->sBy == NULL ? "" : cur->sBy));
            
            TiXmlElement fullipban("FullIpBan");
            if(((cur->ui8Bits & FULL) == FULL) == true) {
                fullipban.InsertEndChild(TiXmlText("1"));
            } else {
                fullipban.InsertEndChild(TiXmlText("0"));
            }
            
            TiXmlElement expire("Expire");
			expire.InsertEndChild(TiXmlText(string((uint32_t)cur->tTempBanExpire).c_str()));
            
            TiXmlElement ban("RangeBan");
            ban.InsertEndChild(type);
            ban.InsertEndChild(ipfrom);
            ban.InsertEndChild(ipto);
            ban.InsertEndChild(reason);
            ban.InsertEndChild(by);
            ban.InsertEndChild(fullipban);
            ban.InsertEndChild(expire);
            
            rangebans.InsertEndChild(ban);
        }
    }

    banlist.InsertEndChild(rangebans);

    doc.InsertEndChild(banlist);
    doc.SaveFile();
}
//---------------------------------------------------------------------------

void clsBanManager::ClearTemp(void) {
    BanItem * curBan = NULL,
        * nextBan = pTempBanListS;

    while(nextBan != NULL) {
        curBan = nextBan;
        nextBan = curBan->pNext;

        Rem(curBan);
        delete curBan;
	}
    
	Save();
}
//---------------------------------------------------------------------------

void clsBanManager::ClearPerm(void) {
    BanItem * curBan = NULL,
        * nextBan = pPermBanListS;

    while(nextBan != NULL) {
        curBan = nextBan;
        nextBan = curBan->pNext;

        Rem(curBan);
        delete curBan;
	}
    
	Save();
}
//---------------------------------------------------------------------------

void clsBanManager::ClearRange(void) {
    RangeBanItem * curBan = NULL,
        * nextBan = pRangeBanListS;

    while(nextBan != NULL) {
        curBan = nextBan;
        nextBan = curBan->pNext;

        RemRange(curBan);
        delete curBan;
	}

	Save();
}
//---------------------------------------------------------------------------

void clsBanManager::ClearTempRange(void) {
    RangeBanItem * curBan = NULL,
        * nextBan = pRangeBanListS;

    while(nextBan != NULL) {
        curBan = nextBan;
        nextBan = curBan->pNext;

        if(((curBan->ui8Bits & TEMP) == TEMP) == true) {
            RemRange(curBan);
            delete curBan;
		}
    }
    
	Save();
}
//---------------------------------------------------------------------------

void clsBanManager::ClearPermRange(void) {
    RangeBanItem * curBan = NULL,
        * nextBan = pRangeBanListS;

    while(nextBan != NULL) {
        curBan = nextBan;
        nextBan = curBan->pNext;

        if(((curBan->ui8Bits & PERM) == PERM) == true) {
            RemRange(curBan);
            delete curBan;
		}
    }

	Save();
}
//---------------------------------------------------------------------------

void clsBanManager::Ban(User * u, const char * sReason, char * sBy, const bool &bFull) {
    BanItem * pBan = new (std::nothrow) BanItem();
    if(pBan == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pBan in clsBanManager::Ban\n", 0);
		return;
    }

    pBan->ui8Bits |= PERM;

    strcpy(pBan->sIp, u->sIP);
    memcpy(pBan->ui128IpHash, u->ui128IpHash, 16);
    pBan->ui8Bits |= IP;
    
    if(bFull == true) {
        pBan->ui8Bits |= FULL;
    }

    time_t acc_time;
    time(&acc_time);

    // PPK ... check for <unknown> nick -> bad ban from script
    if(u->sNick[0] != '<') {
#ifdef _WIN32
        pBan->sNick = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, u->ui8NickLen+1);
#else
		pBan->sNick = (char *)malloc(u->ui8NickLen+1);
#endif
		if(pBan->sNick == NULL) {
            delete pBan;

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sNick in clsBanManager::Ban\n", (uint64_t)(u->ui8NickLen+1));

			return;
		}

		memcpy(pBan->sNick, u->sNick, u->ui8NickLen);
		pBan->sNick[u->ui8NickLen] = '\0';
		pBan->ui32NickHash = u->ui32NickHash;
		pBan->ui8Bits |= NICK;
        
        // PPK ... not allow same nickbans ! i don't want this check here, but lame scripter find way to ban same nick/ip multiple times :(
		BanItem * nxtBan = FindNick(pBan->ui32NickHash, acc_time, pBan->sNick);

        if(nxtBan != NULL) {
            if(((nxtBan->ui8Bits & PERM) == PERM) == true) {
                if(((nxtBan->ui8Bits & IP) == IP) == true) {
                    if(memcmp(pBan->ui128IpHash, nxtBan->ui128IpHash, 16) == 0) {
                        if(((pBan->ui8Bits & FULL) == FULL) == false) {
                            // PPK ... same ban and new is not full, delete new
                            delete pBan;

                            return;
                        } else {
                            if(((nxtBan->ui8Bits & FULL) == FULL) == true) {
                                // PPK ... same ban and both full, delete new
                                delete pBan;

                                return;
                            } else {
                                // PPK ... same ban but only new is full, delete old
                                Rem(nxtBan);
                                delete nxtBan;
							}
                        }
                    } else {
                        pBan->ui8Bits &= ~NICK;
                    }
                } else {
                    // PPK ... old ban is only nickban, remove it
                    Rem(nxtBan);
                    delete nxtBan;
				}
            } else {
                if(((nxtBan->ui8Bits & IP) == IP) == true) {
                    if(memcmp(pBan->ui128IpHash, nxtBan->ui128IpHash, 16) == 0) {
                        if(((nxtBan->ui8Bits & FULL) == FULL) == false) {
                            // PPK ... same ban and old is only temp, delete old
                            Rem(nxtBan);
                            delete nxtBan;
						} else {
                            if(((pBan->ui8Bits & FULL) == FULL) == true) {
                                // PPK ... same full ban and old is only temp, delete old
                                Rem(nxtBan);
                                delete nxtBan;
							} else {
                                // PPK ... old ban is full, new not... set old ban to only ipban
								RemFromNickTable(nxtBan);
                                nxtBan->ui8Bits &= ~NICK;
                            }
                        }
                    } else {
                        // PPK ... set old ban to ip ban only
						RemFromNickTable(nxtBan);
                        nxtBan->ui8Bits &= ~NICK;
                    }
                } else {
                    // PPK ... old ban is only nickban, remove it
                    Rem(nxtBan);
                    delete nxtBan;
				}
            }
        }
    }
    
    // PPK ... clear bans with same ip without nickban and fullban if new ban is fullban
	BanItem * curBan = NULL,
        * nxtBan = FindIP(pBan->ui128IpHash, acc_time);

    while(nxtBan != NULL) {
        curBan = nxtBan;
        nxtBan = curBan->pHashIpTableNext;

        if(((curBan->ui8Bits & NICK) == NICK) == true) {
            continue;
        }
        
        if(((curBan->ui8Bits & FULL) == FULL) == true && ((pBan->ui8Bits & FULL) == FULL) == false) {
            continue;
        }

        Rem(curBan);
        delete curBan;
	}
        
    if(sReason != NULL) {
        size_t szReasonLen = strlen(sReason);
#ifdef _WIN32
        pBan->sReason = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szReasonLen > 255 ? 256 : szReasonLen+1);
#else
		pBan->sReason = (char *)malloc(szReasonLen > 255 ? 256 : szReasonLen+1);
#endif
        if(pBan->sReason == NULL) {
            delete pBan;

            AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sReason in clsBanManager::Ban\n", (uint64_t)(szReasonLen > 255 ? 256 : szReasonLen+1));

            return;
        }

        if(szReasonLen > 255) {
            memcpy(pBan->sReason, sReason, 252);
			pBan->sReason[254] = '.';
			pBan->sReason[253] = '.';
            pBan->sReason[252] = '.';
            szReasonLen = 255;
        } else {
            memcpy(pBan->sReason, sReason, szReasonLen);
        }
        pBan->sReason[szReasonLen] = '\0';
    }

    if(sBy != NULL) {
        size_t szByLen = strlen(sBy);
        if(szByLen > 63) {
            szByLen = 63;
        }
#ifdef _WIN32
        pBan->sBy = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szByLen+1);
#else
		pBan->sBy = (char *)malloc(szByLen+1);
#endif
        if(pBan->sBy == NULL) {
            delete pBan;

            AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sBy in clsBanManager::Ban\n", (uint64_t)(szByLen+1));

			return;
        }   
        memcpy(pBan->sBy, sBy, szByLen);
        pBan->sBy[szByLen] = '\0';
    }

	if(Add(pBan) == false) {
        delete pBan;
        return;
    }

	Save();
}
//---------------------------------------------------------------------------

char clsBanManager::BanIp(User * u, char * sIp, char * sReason, char * sBy, const bool &bFull) {
    BanItem * pBan = new (std::nothrow) BanItem();
    if(pBan == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate pBan in clsBanManager::BanIp\n", 0);
    	return 1;
    }

    pBan->ui8Bits |= PERM;

    if(u != NULL) {
        strcpy(pBan->sIp, u->sIP);
        memcpy(pBan->ui128IpHash, u->ui128IpHash, 16);
    } else {
        if(sIp != NULL && HashIP(sIp, pBan->ui128IpHash) == true) {
            strcpy(pBan->sIp, sIp);
        } else {
			delete pBan;

            return 1;
        }
    }

    pBan->ui8Bits |= IP;
    
    if(bFull == true) {
        pBan->ui8Bits |= FULL;
    }
    
    time_t acc_time;
    time(&acc_time);

	BanItem * curBan = NULL,
        * nxtBan = FindIP(pBan->ui128IpHash, acc_time);
        
    // PPK ... don't add ban if is already here perm (full) ban for same ip
    while(nxtBan != NULL) {
        curBan = nxtBan;
        nxtBan = curBan->pHashIpTableNext;
        
        if(((curBan->ui8Bits & TEMP) == TEMP) == true) {
            if(((curBan->ui8Bits & FULL) == FULL) == false || ((pBan->ui8Bits & FULL) == FULL) == true) {
                if(((curBan->ui8Bits & NICK) == NICK) == false) {
                    Rem(curBan);
                    delete curBan;
				}

                continue;
            }

            continue;
        }

        if(((curBan->ui8Bits & FULL) == FULL) == false && ((pBan->ui8Bits & FULL) == FULL) == true) {
            if(((curBan->ui8Bits & NICK) == NICK) == false) {
                Rem(curBan);
                delete curBan;
			}
            continue;
        }

        delete pBan;

        return 2;
    }

    if(sReason != NULL) {
        size_t szReasonLen = strlen(sReason);
#ifdef _WIN32
        pBan->sReason = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szReasonLen > 255 ? 256 : szReasonLen+1);
#else
		pBan->sReason = (char *)malloc(szReasonLen > 255 ? 256 : szReasonLen+1);
#endif
        if(pBan->sReason == NULL) {
            delete pBan;

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sReason in clsBanManager::BanIp\n", (uint64_t)(szReasonLen > 255 ? 256 : szReasonLen+1));

            return 1;
        }

        if(szReasonLen > 255) {
            memcpy(pBan->sReason, sReason, 252);
			pBan->sReason[254] = '.';
			pBan->sReason[253] = '.';
            pBan->sReason[252] = '.';
            szReasonLen = 255;
        } else {
            memcpy(pBan->sReason, sReason, szReasonLen);
        }
        pBan->sReason[szReasonLen] = '\0';
    }

    if(sBy != NULL) {
        size_t szByLen = strlen(sBy);
        if(szByLen > 63) {
            szByLen = 63;
        }
#ifdef _WIN32
        pBan->sBy = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szByLen+1);
#else
		pBan->sBy = (char *)malloc(szByLen+1);
#endif
        if(pBan->sBy == NULL) {
            delete pBan;

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sBy in clsBanManager::BanIp\n", (uint64_t)(szByLen+1));

            return 1;
        }   
        memcpy(pBan->sBy, sBy, szByLen);
        pBan->sBy[szByLen] = '\0';
    }

    if(Add(pBan) == false) {
        delete pBan;
        return 1;
    }

	Save();

    return 0;
}
//---------------------------------------------------------------------------

bool clsBanManager::NickBan(User * u, char * sNick, char * sReason, char * sBy) {
    BanItem * pBan = new (std::nothrow) BanItem();
    if(pBan == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pBan in clsBanManager::NickBan\n", 0);
    	return false;
    }

    pBan->ui8Bits |= PERM;

    if(u == NULL) {
        // PPK ... this should never happen, but to be sure ;)
        if(sNick == NULL) {
            delete pBan;

            return false;
        }

        // PPK ... bad script ban check
        if(sNick[0] == '<') {
            delete pBan;

            return false;
        }
        
        size_t szNickLen = strlen(sNick);
#ifdef _WIN32
        pBan->sNick = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szNickLen+1);
#else
		pBan->sNick = (char *)malloc(szNickLen+1);
#endif
        if(pBan->sNick == NULL) {
            delete pBan;

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sNick in clsBanManager::NickBan\n", (uint64_t)(szNickLen+1));

            return false;
        }

        memcpy(pBan->sNick, sNick, szNickLen);
        pBan->sNick[szNickLen] = '\0';
        pBan->ui32NickHash = HashNick(sNick, szNickLen);
    } else {
        // PPK ... bad script ban check
        if(u->sNick[0] == '<') {
            delete pBan;

            return false;
        }

#ifdef _WIN32
        pBan->sNick = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, u->ui8NickLen+1);
#else
		pBan->sNick = (char *)malloc(u->ui8NickLen+1);
#endif
        if(pBan->sNick == NULL) {
            delete pBan;

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sNick1 in clsBanManager::NickBan\n", (uint64_t)(u->ui8NickLen+1));

            return false;
        }   

        memcpy(pBan->sNick, u->sNick, u->ui8NickLen);
        pBan->sNick[u->ui8NickLen] = '\0';
        pBan->ui32NickHash = u->ui32NickHash;

        strcpy(pBan->sIp, u->sIP);
        memcpy(pBan->ui128IpHash, u->ui128IpHash, 16);
    }

    pBan->ui8Bits |= NICK;

    time_t acc_time;
    time(&acc_time);

	BanItem *nxtBan = FindNick(pBan->ui32NickHash, acc_time, pBan->sNick);
    
    // PPK ... not allow same nickbans !
    if(nxtBan != NULL) {
        if(((nxtBan->ui8Bits & PERM) == PERM) == true) {
            delete pBan;

            return false;
        } else {
            if(((nxtBan->ui8Bits & IP) == IP) == true) {
                // PPK ... set old ban to ip ban only
				RemFromNickTable(nxtBan);
                nxtBan->ui8Bits &= ~NICK;
            } else {
                // PPK ... old ban is only nickban, remove it
                Rem(nxtBan);
                delete nxtBan;
			}
        }
    }

    if(sReason != NULL) {
        size_t szReasonLen = strlen(sReason);
#ifdef _WIN32
        pBan->sReason = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szReasonLen > 255 ? 256 : szReasonLen+1);
#else
		pBan->sReason = (char *)malloc(szReasonLen > 255 ? 256 : szReasonLen+1);
#endif
        if(pBan->sReason == NULL) {
            delete pBan;

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sReason in clsBanManager::NickBan\n", (uint64_t)(szReasonLen > 255 ? 256 : szReasonLen+1));

            return false;
        }   

        if(szReasonLen > 255) {
            memcpy(pBan->sReason, sReason, 252);
			pBan->sReason[254] = '.';
			pBan->sReason[253] = '.';
            pBan->sReason[252] = '.';
            szReasonLen = 255;
        } else {
            memcpy(pBan->sReason, sReason, szReasonLen);
        }
        pBan->sReason[szReasonLen] = '\0';
    }

    if(sBy != NULL) {
        size_t szByLen = strlen(sBy);
        if(szByLen > 63) {
            szByLen = 63;
        }
#ifdef _WIN32
        pBan->sBy = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szByLen+1);
#else
		pBan->sBy = (char *)malloc(szByLen+1);
#endif
        if(pBan->sBy == NULL) {
            delete pBan;

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sBy in clsBanManager::NickBan\n", (uint64_t)(szByLen+1));

            return false;
        }   
        memcpy(pBan->sBy, sBy, szByLen);
        pBan->sBy[szByLen] = '\0';
    }

    if(Add(pBan) == false) {
        delete pBan;
        return false;
    }

	Save();

    return true;
}
//---------------------------------------------------------------------------

void clsBanManager::TempBan(User * u, const char * sReason, char * sBy, const uint32_t &minutes, const time_t &expiretime, const bool &bFull) {
    BanItem * pBan = new (std::nothrow) BanItem();
    if(pBan == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate pBan in clsBanManager::TempBan\n", 0);
    	return;
    }

    pBan->ui8Bits |= TEMP;

    strcpy(pBan->sIp, u->sIP);
    memcpy(pBan->ui128IpHash, u->ui128IpHash, 16);
    pBan->ui8Bits |= IP;
    
    if(bFull == true) {
        pBan->ui8Bits |= FULL;
    }

    time_t acc_time;
    time(&acc_time);

    if(expiretime > 0) {
        pBan->tTempBanExpire = expiretime;
    } else {
        if(minutes > 0) {
            pBan->tTempBanExpire = acc_time+(minutes*60);
        } else {
    	    pBan->tTempBanExpire = acc_time+(clsSettingManager::mPtr->i16Shorts[SETSHORT_DEFAULT_TEMP_BAN_TIME]*60);
        }
    }

    // PPK ... check for <unknown> nick -> bad ban from script
    if(u->sNick[0] != '<') {
        size_t szNickLen = strlen(u->sNick);
#ifdef _WIN32
        pBan->sNick = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szNickLen+1);
#else
		pBan->sNick = (char *)malloc(szNickLen+1);
#endif
        if(pBan->sNick == NULL) {
            delete pBan;

            AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sNick in clsBanManager::TempBan\n", (uint64_t)(szNickLen+1));

            return;
        }

        memcpy(pBan->sNick, u->sNick, szNickLen);
        pBan->sNick[szNickLen] = '\0';
        pBan->ui32NickHash = u->ui32NickHash;
        pBan->ui8Bits |= NICK;

        // PPK ... not allow same nickbans ! i don't want this check here, but lame scripter find way to ban same nick multiple times :(
		BanItem *nxtBan = FindNick(pBan->ui32NickHash, acc_time, pBan->sNick);

        if(nxtBan != NULL) {
            if(((nxtBan->ui8Bits & PERM) == PERM) == true) {
                if(((nxtBan->ui8Bits & IP) == IP) == true) {
                    if(memcmp(pBan->ui128IpHash, nxtBan->ui128IpHash, 16) == 0) {
                        if(((pBan->ui8Bits & FULL) == FULL) == false) {
                            // PPK ... same ban and old is perm, delete new
                            delete pBan;

                            return;
                        } else {
                            if(((nxtBan->ui8Bits & FULL) == FULL) == true) {
                                // PPK ... same ban and old is full perm, delete new
                                delete pBan;

                                return;
                            } else {
                                // PPK ... same ban and only new is full, set new to ipban only
                                pBan->ui8Bits &= ~NICK;
                            }
                        }
                    }
                } else {
                    // PPK ... perm ban to same nick already exist, set new to ipban only
                    pBan->ui8Bits &= ~NICK;
                }
            } else {
                if(nxtBan->tTempBanExpire < pBan->tTempBanExpire) {
                    if(((nxtBan->ui8Bits & IP) == IP) == true) {
                        if(memcmp(pBan->ui128IpHash, nxtBan->ui128IpHash, 16) == 0) {
                            if(((nxtBan->ui8Bits & FULL) == FULL) == false) {
                                // PPK ... same bans, but old with lower expiration -> delete old
                                Rem(nxtBan);
                                delete nxtBan;
							} else {
                                if(((pBan->ui8Bits & FULL) == FULL) == false) {
                                    // PPK ... old ban with lower expiration is full ban, set old to ipban only
									RemFromNickTable(nxtBan);
                                    nxtBan->ui8Bits &= ~NICK;
                                } else {
                                    // PPK ... same bans, old have lower expiration -> delete old
                                    Rem(nxtBan);
                                    delete nxtBan;
								}
                            }
                        } else {
                            // PPK ... set old ban to ipban only
							RemFromNickTable(nxtBan);
                            nxtBan->ui8Bits &= ~NICK;
                        }
                    } else {
                        // PPK ... old ban is only nickban with lower bantime, remove it
                        Rem(nxtBan);
                        delete nxtBan;
					}
                } else {
                    if(((nxtBan->ui8Bits & IP) == IP) == true) {
                        if(memcmp(pBan->ui128IpHash, nxtBan->ui128IpHash, 16) == 0) {
                            if(((pBan->ui8Bits & FULL) == FULL) == false) {
                                // PPK ... same bans, but new with lower expiration -> delete new
                                delete pBan;

								return;
                            } else {
                                if(((nxtBan->ui8Bits & FULL) == FULL) == false) {
                                    // PPK ... new ban with lower expiration is full ban, set new to ipban only
                                    pBan->ui8Bits &= ~NICK;
                                } else {
                                    // PPK ... same bans, new have lower expiration -> delete new
                                    delete pBan;

                                    return;
                                }
                            }
                        } else {
                            // PPK ... set new ban to ipban only
                            pBan->ui8Bits &= ~NICK;
                        }
                    } else {
                        // PPK ... old ban is only nickban with higher bantime, set new to ipban only
                        pBan->ui8Bits &= ~NICK;
                    }
                }
            }
        }
    }

    // PPK ... clear bans with lower timeban and same ip without nickban and fullban if new ban is fullban
	BanItem * curBan = NULL,
        * nxtBan = FindIP(pBan->ui128IpHash, acc_time);

    while(nxtBan != NULL) {
        curBan = nxtBan;
        nxtBan = curBan->pHashIpTableNext;

        if(((curBan->ui8Bits & PERM) == PERM) == true) {
            continue;
        }
        
        if(((curBan->ui8Bits & NICK) == NICK) == true) {
            continue;
        }

        if(((curBan->ui8Bits & FULL) == FULL) == true && ((pBan->ui8Bits & FULL) == FULL) == false) {
            continue;
        }

        if(curBan->tTempBanExpire > pBan->tTempBanExpire) {
            continue;
        }
        
        Rem(curBan);
        delete curBan;
	}

    if(sReason != NULL) {
        size_t szReasonLen = strlen(sReason);
#ifdef _WIN32
        pBan->sReason = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szReasonLen > 255 ? 256 : szReasonLen+1);
#else
		pBan->sReason = (char *)malloc(szReasonLen > 255 ? 256 : szReasonLen+1);
#endif
        if(pBan->sReason == NULL) {
            delete pBan;

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sReason in clsBanManager::TempBan\n", (uint64_t)(szReasonLen > 255 ? 256 : szReasonLen+1));

            return;
        }   

        if(szReasonLen > 255) {
            memcpy(pBan->sReason, sReason, 252);
			pBan->sReason[254] = '.';
			pBan->sReason[253] = '.';
            pBan->sReason[252] = '.';
            szReasonLen = 255;
        } else {
            memcpy(pBan->sReason, sReason, szReasonLen);
        }
        pBan->sReason[szReasonLen] = '\0';
    }

    if(sBy != NULL) {
        size_t szByLen = strlen(sBy);
        if(szByLen > 63) {
            szByLen = 63;
        }
#ifdef _WIN32
        pBan->sBy = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szByLen+1);
#else
		pBan->sBy = (char *)malloc(szByLen+1);
#endif
        if(pBan->sBy == NULL) {
            delete pBan;

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sBy in clsBanManager::TempBan\n", (uint64_t)(szByLen+1));

            return;
        }   
        memcpy(pBan->sBy, sBy, szByLen);
        pBan->sBy[szByLen] = '\0';
    }

    if(Add(pBan) == false) {
        delete pBan;
        return;
    }

	Save();
}
//---------------------------------------------------------------------------

char clsBanManager::TempBanIp(User * u, char * sIp, char * sReason, char * sBy, const uint32_t &minutes, const time_t &expiretime, const bool &bFull) {
    BanItem * pBan = new (std::nothrow) BanItem();
    if(pBan == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pBan in clsBanManager::TempBanIp\n", 0);
    	return 1;
    }

    pBan->ui8Bits |= TEMP;

    if(u != NULL) {
        strcpy(pBan->sIp, u->sIP);
        memcpy(pBan->ui128IpHash, u->ui128IpHash, 16);
    } else {
        if(sIp != NULL && HashIP(sIp, pBan->ui128IpHash) == true) {
            strcpy(pBan->sIp, sIp);
        } else {
            delete pBan;

            return 1;
        }
    }

    pBan->ui8Bits |= IP;
    
    if(bFull == true) {
        pBan->ui8Bits |= FULL;
    }

    time_t acc_time;
    time(&acc_time);

    if(expiretime > 0) {
        pBan->tTempBanExpire = expiretime;
    } else {
        if(minutes == 0) {
    	    pBan->tTempBanExpire = acc_time+(clsSettingManager::mPtr->i16Shorts[SETSHORT_DEFAULT_TEMP_BAN_TIME]*60);
        } else {
            pBan->tTempBanExpire = acc_time+(minutes*60);
        }
    }
    
	BanItem * curBan = NULL,
        * nxtBan = FindIP(pBan->ui128IpHash, acc_time);

    // PPK ... don't add ban if is already here perm (full) ban or longer temp ban for same ip
    while(nxtBan != NULL) {
        curBan = nxtBan;
        nxtBan = curBan->pHashIpTableNext;

        if(((curBan->ui8Bits & TEMP) == TEMP) == true && curBan->tTempBanExpire < pBan->tTempBanExpire) {
            if(((curBan->ui8Bits & FULL) == FULL) == false || ((pBan->ui8Bits & FULL) == FULL) == true) {
                if(((curBan->ui8Bits & NICK) == NICK) == false) {
                    Rem(curBan);
                    delete curBan;
				}

                continue;
            }

            continue;
        }

        if(((curBan->ui8Bits & FULL) == FULL) == false && ((pBan->ui8Bits & FULL) == FULL) == true) continue;

        delete pBan;

        return 2;
    }

    if(sReason != NULL) {
        size_t szReasonLen = strlen(sReason);
#ifdef _WIN32
        pBan->sReason = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szReasonLen > 255 ? 256 : szReasonLen+1);
#else
		pBan->sReason = (char *)malloc(szReasonLen > 255 ? 256 : szReasonLen+1);
#endif
        if(pBan->sReason == NULL) {
            delete pBan;

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sReason in clsBanManager::TempBanIp\n", (uint64_t)(szReasonLen > 255 ? 256 : szReasonLen+1));

            return 1;
        }

        if(szReasonLen > 255) {
            memcpy(pBan->sReason, sReason, 252);
			pBan->sReason[254] = '.';
			pBan->sReason[253] = '.';
            pBan->sReason[252] = '.';
            szReasonLen = 255;
        } else {
            memcpy(pBan->sReason, sReason, szReasonLen);
        }
        pBan->sReason[szReasonLen] = '\0';
    }

    if(sBy != NULL) {
        size_t szByLen = strlen(sBy);
        if(szByLen > 63) {
            szByLen = 63;
        }
#ifdef _WIN32
        pBan->sBy = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szByLen+1);
#else
		pBan->sBy = (char *)malloc(szByLen+1);
#endif
        if(pBan->sBy == NULL) {
            delete pBan;

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sBy in clsBanManager::TempBanIp\n", (uint64_t)(szByLen+1));

            return 1;
        }   
        memcpy(pBan->sBy, sBy, szByLen);
        pBan->sBy[szByLen] = '\0';
    }

    if(Add(pBan) == false) {
        delete pBan;
        return 1;
    }

	Save();

    return 0;
}
//---------------------------------------------------------------------------

bool clsBanManager::NickTempBan(User * u, char * sNick, char * sReason, char * sBy, const uint32_t &minutes, const time_t &expiretime) {
    BanItem * pBan = new (std::nothrow) BanItem();
    if(pBan == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pBan in clsBanManager::NickTempBan\n", 0);
    	return false;
    }

    pBan->ui8Bits |= TEMP;

    if(u == NULL) {
        // PPK ... this should never happen, but to be sure ;)
        if(sNick == NULL) {
            delete pBan;

            return false;
        }

        // PPK ... bad script ban check
        if(sNick[0] == '<') {
            delete pBan;

            return false;
        }
        
        size_t szNickLen = strlen(sNick);
#ifdef _WIN32
        pBan->sNick = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szNickLen+1);
#else
		pBan->sNick = (char *)malloc(szNickLen+1);
#endif
        if(pBan->sNick == NULL) {
            delete pBan;

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sNick in clsBanManager::NickTempBan\n", (uint64_t)(szNickLen+1));

            return false;
        }   
        memcpy(pBan->sNick, sNick, szNickLen);
        pBan->sNick[szNickLen] = '\0';
        pBan->ui32NickHash = HashNick(sNick, szNickLen);
    } else {
        // PPK ... bad script ban check
        if(u->sNick[0] == '<') {
            delete pBan;

            return false;
        }

#ifdef _WIN32
        pBan->sNick = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, u->ui8NickLen+1);
#else
		pBan->sNick = (char *)malloc(u->ui8NickLen+1);
#endif
        if(pBan->sNick == NULL) {
            delete pBan;

            AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sNick1 in clsBanManager::NickTempBan\n", (uint64_t)(u->ui8NickLen+1));

            return false;
        }   
        memcpy(pBan->sNick, u->sNick, u->ui8NickLen);
        pBan->sNick[u->ui8NickLen] = '\0';
        pBan->ui32NickHash = u->ui32NickHash;

        strcpy(pBan->sIp, u->sIP);
        memcpy(pBan->ui128IpHash, u->ui128IpHash, 16);
    }

    pBan->ui8Bits |= NICK;

    time_t acc_time;
    time(&acc_time);

    if(expiretime > 0) {
        pBan->tTempBanExpire = expiretime;
    } else {
        if(minutes > 0) {
            pBan->tTempBanExpire = acc_time+(minutes*60);
        } else {
    	    pBan->tTempBanExpire = acc_time+(clsSettingManager::mPtr->i16Shorts[SETSHORT_DEFAULT_TEMP_BAN_TIME]*60);
        }
    }
    
	BanItem *nxtBan = FindNick(pBan->ui32NickHash, acc_time, pBan->sNick);

    // PPK ... not allow same nickbans !
    if(nxtBan != NULL) {
        if(((nxtBan->ui8Bits & PERM) == PERM) == true) {
            delete pBan;

            return false;
        } else {
            if(nxtBan->tTempBanExpire < pBan->tTempBanExpire) {
                if(((nxtBan->ui8Bits & IP) == IP) == true) {
                    // PPK ... set old ban to ip ban only
                    RemFromNickTable(nxtBan);
                    nxtBan->ui8Bits &= ~NICK;
                } else {
                    // PPK ... old ban is only nickban, remove it
                    Rem(nxtBan);
                    delete nxtBan;
				}
            } else {
                delete pBan;

                return false;
            }
        }
    }

    if(sReason != NULL) {
        size_t szReasonLen = strlen(sReason);
#ifdef _WIN32
        pBan->sReason = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szReasonLen > 255 ? 256 : szReasonLen+1);
#else
		pBan->sReason = (char *)malloc(szReasonLen > 255 ? 256 : szReasonLen+1);
#endif
        if(pBan->sReason == NULL) {
            delete pBan;

            AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sReason in clsBanManager::NickTempBan\n", (uint64_t)(szReasonLen > 255 ? 256 : szReasonLen+1));

            return false;
        }   

        if(szReasonLen > 255) {
            memcpy(pBan->sReason, sReason, 252);
			pBan->sReason[254] = '.';
			pBan->sReason[253] = '.';
            pBan->sReason[252] = '.';
            szReasonLen = 255;
        } else {
            memcpy(pBan->sReason, sReason, szReasonLen);
        }
        pBan->sReason[szReasonLen] = '\0';
    }

    if(sBy != NULL) {
        size_t szByLen = strlen(sBy);
        if(szByLen > 63) {
            szByLen = 63;
        }
#ifdef _WIN32
        pBan->sBy = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szByLen+1);
#else
		pBan->sBy = (char *)malloc(szByLen+1);
#endif
        if(pBan->sBy == NULL) {
            delete pBan;

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sBy in clsBanManager::NickTempBan\n", (uint64_t)(szByLen+1));

            return false;
        }   
        memcpy(pBan->sBy, sBy, szByLen);
        pBan->sBy[szByLen] = '\0';
    }

    if(Add(pBan) == false) {
        delete pBan;
        return false;
    }

	Save();

    return true;
}
//---------------------------------------------------------------------------

bool clsBanManager::Unban(char * sWhat) {
    uint32_t hash = HashNick(sWhat, strlen(sWhat));

    time_t acc_time;
    time(&acc_time);

	BanItem *Ban = FindNick(hash, acc_time, sWhat);

    if(Ban == NULL) {
		uint8_t ui128Hash[16];
		memset(ui128Hash, 0, 16);

		if(HashIP(sWhat, ui128Hash) == true && (Ban = FindIP(ui128Hash, acc_time)) != NULL) {
            Rem(Ban);
            delete Ban;
		} else {
            return false;
        }
    } else {
        Rem(Ban);
        delete Ban;
	}

	Save();
    return true;
}
//---------------------------------------------------------------------------

bool clsBanManager::PermUnban(char * sWhat) {
    uint32_t hash = HashNick(sWhat, strlen(sWhat));

	BanItem *Ban = FindPermNick(hash, sWhat);

    if(Ban == NULL) {
		uint8_t ui128Hash[16];
		memset(ui128Hash, 0, 16);

		if(HashIP(sWhat, ui128Hash) == true && (Ban = FindPermIP(ui128Hash)) != NULL) {
            Rem(Ban);
            delete Ban;
		} else {
            return false;
        }
    } else {
        Rem(Ban);
        delete Ban;
	}

	Save();
    return true;
}
//---------------------------------------------------------------------------

bool clsBanManager::TempUnban(char * sWhat) {
    uint32_t hash = HashNick(sWhat, strlen(sWhat));

    time_t acc_time;
    time(&acc_time);

    BanItem *Ban = FindTempNick(hash, acc_time, sWhat);

    if(Ban == NULL) {
		uint8_t ui128Hash[16];
		memset(ui128Hash, 0, 16);

        if(HashIP(sWhat, ui128Hash) == true && (Ban = FindTempIP(ui128Hash, acc_time)) != NULL) {
            Rem(Ban);
            delete Ban;
		} else {
            return false;
        }
    } else {
        Rem(Ban);
        delete Ban;
	}

	Save();
    return true;
}
//---------------------------------------------------------------------------

void clsBanManager::RemoveAllIP(const uint8_t * ui128IpHash) {
    uint16_t ui16IpTableIdx = 0;

    if(ui128IpHash[10] == 255 && ui128IpHash[11] == 255 && memcmp(ui128IpHash, "\0\0\0\0\0\0\0\0\0\0", 10) == 0) {
        ui16IpTableIdx = ui128IpHash[14] * ui128IpHash[15];
    } else {
        ui16IpTableIdx = GetIpTableIdx(ui128IpHash);
    }

    IpTableItem * cur = NULL,
        * next = pIpTable[ui16IpTableIdx];

    BanItem * curBan = NULL, * nextBan = NULL;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

        if(memcmp(cur->pFirstBan->ui128IpHash, ui128IpHash, 16) == 0) {
			nextBan = cur->pFirstBan;

            while(nextBan != NULL) {
                curBan = nextBan;
                nextBan = curBan->pHashIpTableNext;
                
				Rem(curBan);
                delete curBan;
            }

            return;
        }
    }
}
//---------------------------------------------------------------------------

void clsBanManager::RemovePermAllIP(const uint8_t * ui128IpHash) {
    uint16_t ui16IpTableIdx = 0;

    if(ui128IpHash[10] == 255 && ui128IpHash[11] == 255 && memcmp(ui128IpHash, "\0\0\0\0\0\0\0\0\0\0", 10) == 0) {
        ui16IpTableIdx = ui128IpHash[14] * ui128IpHash[15];
    } else {
        ui16IpTableIdx = GetIpTableIdx(ui128IpHash);
    }

    IpTableItem * cur = NULL,
        * next = pIpTable[ui16IpTableIdx];

    BanItem * curBan = NULL, * nextBan = NULL;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

        if(memcmp(cur->pFirstBan->ui128IpHash, ui128IpHash, 16) == 0) {
			nextBan = cur->pFirstBan;

            while(nextBan != NULL) {
                curBan = nextBan;
                nextBan = curBan->pHashIpTableNext;

                if(((curBan->ui8Bits & clsBanManager::PERM) == clsBanManager::PERM) == true) {
					Rem(curBan);
                    delete curBan;
                }
            }

            return;
        }
    }
}
//---------------------------------------------------------------------------

void clsBanManager::RemoveTempAllIP(const uint8_t * ui128IpHash) {
    uint16_t ui16IpTableIdx = 0;

    if(ui128IpHash[10] == 255 && ui128IpHash[11] == 255 && memcmp(ui128IpHash, "\0\0\0\0\0\0\0\0\0\0", 10) == 0) {
        ui16IpTableIdx = ui128IpHash[14] * ui128IpHash[15];
    } else {
        ui16IpTableIdx = GetIpTableIdx(ui128IpHash);
    }

    IpTableItem * cur = NULL,
        * next = pIpTable[ui16IpTableIdx];

    BanItem * curBan = NULL, * nextBan = NULL;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

        if(memcmp(cur->pFirstBan->ui128IpHash, ui128IpHash, 16) == 0) {
			nextBan = cur->pFirstBan;

            while(nextBan != NULL) {
                curBan = nextBan;
                nextBan = curBan->pHashIpTableNext;

                if(((curBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP) == true) {
					Rem(curBan);
                    delete curBan;
                }
            }

            return;
        }
    }
}
//---------------------------------------------------------------------------

bool clsBanManager::RangeBan(char * sIpFrom, const uint8_t * ui128FromIpHash, char * sIpTo, const uint8_t * ui128ToIpHash, char * sReason, char * sBy, const bool &bFull) {
    RangeBanItem * pRangeBan = new (std::nothrow) RangeBanItem();
    if(pRangeBan == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pRangeBan in clsBanManager::RangeBan\n", 0);
    	return false;
    }

    pRangeBan->ui8Bits |= PERM;

    strcpy(pRangeBan->sIpFrom, sIpFrom);
    memcpy(pRangeBan->ui128FromIpHash, ui128FromIpHash, 16);

    strcpy(pRangeBan->sIpTo, sIpTo);
    memcpy(pRangeBan->ui128ToIpHash, ui128ToIpHash, 16);

    if(bFull == true) {
        pRangeBan->ui8Bits |= FULL;
    }

    RangeBanItem * curBan = NULL,
        * nxtBan = pRangeBanListS;

    // PPK ... don't add range ban if is already here same perm (full) range ban
    while(nxtBan != NULL) {
        curBan = nxtBan;
        nxtBan = curBan->pNext;

        if(memcmp(curBan->ui128FromIpHash, pRangeBan->ui128FromIpHash, 16) != 0 || memcmp(curBan->ui128ToIpHash, pRangeBan->ui128ToIpHash, 16) != 0) {
            continue;
        }

        if((curBan->ui8Bits & TEMP) == TEMP) {
            continue;
        }
        
        if(((curBan->ui8Bits & FULL) == FULL) == false && ((pRangeBan->ui8Bits & FULL) == FULL) == true) {
            RemRange(curBan);
            delete curBan;

            continue;
        }
        
        delete pRangeBan;

        return false;
    }

    if(sReason != NULL) {
        size_t szReasonLen = strlen(sReason);
#ifdef _WIN32
        pRangeBan->sReason = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szReasonLen > 255 ? 256 : szReasonLen+1);
#else
		pRangeBan->sReason = (char *)malloc(szReasonLen > 255 ? 256 : szReasonLen+1);
#endif
        if(pRangeBan->sReason == NULL) {
            delete pRangeBan;

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sReason in clsBanManager::RangeBan\n", (uint64_t)(szReasonLen > 255 ? 256 : szReasonLen+1));

            return false;
        }   

        if(szReasonLen > 255) {
            memcpy(pRangeBan->sReason, sReason, 252);
            pRangeBan->sReason[254] = '.';
            pRangeBan->sReason[253] = '.';
            pRangeBan->sReason[252] = '.';
            szReasonLen = 255;
        } else {
            memcpy(pRangeBan->sReason, sReason, szReasonLen);
        }
        pRangeBan->sReason[szReasonLen] = '\0';
    }

    if(sBy != NULL) {
        size_t szByLen = strlen(sBy);
        if(szByLen > 63) {
            szByLen = 63;
        }
#ifdef _WIN32
        pRangeBan->sBy = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szByLen+1);
#else
		pRangeBan->sBy = (char *)malloc(szByLen+1);
#endif
        if(pRangeBan->sBy == NULL) {
            delete pRangeBan;

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sBy in clsBanManager::RangeBan\n", (uint64_t)(szByLen+1));

            return false;
        }   
        memcpy(pRangeBan->sBy, sBy, szByLen);
        pRangeBan->sBy[szByLen] = '\0';
    }

    AddRange(pRangeBan);
	Save();

    return true;
}
//---------------------------------------------------------------------------

bool clsBanManager::RangeTempBan(char * sIpFrom, const uint8_t * ui128FromIpHash, char * sIpTo, const uint8_t * ui128ToIpHash, char * sReason, char * sBy, const uint32_t &minutes,
    const time_t &expiretime, const bool &bFull) {
    RangeBanItem * pRangeBan = new (std::nothrow) RangeBanItem();
    if(pRangeBan == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pRangeBan in clsBanManager::RangeTempBan\n", 0);
    	return false;
    }

    pRangeBan->ui8Bits |= TEMP;

    strcpy(pRangeBan->sIpFrom, sIpFrom);
    memcpy(pRangeBan->ui128FromIpHash, ui128FromIpHash, 16);

    strcpy(pRangeBan->sIpTo, sIpTo);
    memcpy(pRangeBan->ui128ToIpHash, ui128ToIpHash, 16);

    if(bFull == true) {
        pRangeBan->ui8Bits |= FULL;
    }

    time_t acc_time;
    time(&acc_time);

    if(expiretime > 0) {
        pRangeBan->tTempBanExpire = expiretime;
    } else {
        if(minutes > 0) {
            pRangeBan->tTempBanExpire = acc_time+(minutes*60);
        } else {
    	    pRangeBan->tTempBanExpire = acc_time+(clsSettingManager::mPtr->i16Shorts[SETSHORT_DEFAULT_TEMP_BAN_TIME]*60);
        }
    }
    
    RangeBanItem * curBan = NULL,
        * nxtBan = pRangeBanListS;

    // PPK ... don't add range ban if is already here same perm (full) range ban or longer temp ban for same range
    while(nxtBan != NULL) {
        curBan = nxtBan;
        nxtBan = curBan->pNext;

        if(memcmp(curBan->ui128FromIpHash, pRangeBan->ui128FromIpHash, 16) != 0 || memcmp(curBan->ui128ToIpHash, pRangeBan->ui128ToIpHash, 16) != 0) {
            continue;
        }

        if(((curBan->ui8Bits & TEMP) == TEMP) == true && curBan->tTempBanExpire < pRangeBan->tTempBanExpire) {
            if(((curBan->ui8Bits & FULL) == FULL) == false || ((pRangeBan->ui8Bits & FULL) == FULL) == true) {
                RemRange(curBan);
                delete curBan;

                continue;
            }

            continue;
        }
        
        if(((curBan->ui8Bits & FULL) == FULL) == false && ((pRangeBan->ui8Bits & FULL) == FULL) == true) {
            continue;
        }

        delete pRangeBan;

        return false;
    }

    if(sReason != NULL) {
        size_t szReasonLen = strlen(sReason);
#ifdef _WIN32
        pRangeBan->sReason = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szReasonLen > 255 ? 256 : szReasonLen+1);
#else
		pRangeBan->sReason = (char *)malloc(szReasonLen > 255 ? 256 : szReasonLen+1);
#endif
        if(pRangeBan->sReason == NULL) {
            delete pRangeBan;

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sReason in clsBanManager::RangeTempBan\n", (uint64_t)(szReasonLen > 255 ? 256 : szReasonLen+1));

            return false;
        }   

        if(szReasonLen > 255) {
            memcpy(pRangeBan->sReason, sReason, 252);
            pRangeBan->sReason[254] = '.';
            pRangeBan->sReason[253] = '.';
            pRangeBan->sReason[252] = '.';
            szReasonLen = 255;
        } else {
            memcpy(pRangeBan->sReason, sReason, szReasonLen);
        }
        pRangeBan->sReason[szReasonLen] = '\0';
    }

    if(sBy != NULL) {
        size_t szByLen = strlen(sBy);
        if(szByLen > 63) {
            szByLen = 63;
        }
#ifdef _WIN32
        pRangeBan->sBy = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szByLen+1);
#else
		pRangeBan->sBy = (char *)malloc(szByLen+1);
#endif
        if(pRangeBan->sBy == NULL) {
            delete pRangeBan;

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sBy in clsBanManager::RangeTempBan\n", (uint64_t)(szByLen+1));

            return false;
        }   
        memcpy(pRangeBan->sBy, sBy, szByLen);
        pRangeBan->sBy[szByLen] = '\0';
    }

    AddRange(pRangeBan);
	Save();

    return true;
}
//---------------------------------------------------------------------------

bool clsBanManager::RangeUnban(const uint8_t * ui128FromIpHash, const uint8_t * ui128ToIpHash) {
    RangeBanItem * cur = NULL,
        * next = pRangeBanListS;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

        if(memcmp(cur->ui128FromIpHash, ui128FromIpHash, 16) == 0 && memcmp(cur->ui128ToIpHash, ui128ToIpHash, 16) == 0) {
            RemRange(cur);
            delete cur;

            return true;
        }
    }

	Save();
    return false;
}
//---------------------------------------------------------------------------

bool clsBanManager::RangeUnban(const uint8_t * ui128FromIpHash, const uint8_t * ui128ToIpHash, unsigned char cType) {
    RangeBanItem * cur = NULL,
        * next = pRangeBanListS;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

        if((cur->ui8Bits & cType) == cType && memcmp(cur->ui128FromIpHash, ui128FromIpHash, 16) == 0 && memcmp(cur->ui128ToIpHash, ui128ToIpHash, 16) == 0) {
            RemRange(cur);
            delete cur;

            return true;
        }
    }

    Save();
    return false;
}
//---------------------------------------------------------------------------
