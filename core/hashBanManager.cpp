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
#include "hashBanManager.h"
//---------------------------------------------------------------------------
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
hashBanMan *hashBanManager = NULL;
//---------------------------------------------------------------------------

BanItem::BanItem(void) {
    sNick = NULL;
    sIp[0] = '\0';
    sReason = NULL;
    sBy = NULL;

    ui8Bits = 0;
    
    ui32NickHash = 0;
    ui32IpHash = 0;

    tempbanexpire = 0;

    prev = NULL;
    next = NULL;
    hashnicktableprev = NULL;
    hashnicktablenext = NULL;
    hashiptableprev = NULL;
    hashiptablenext = NULL;
}
//---------------------------------------------------------------------------

BanItem::~BanItem(void) {
#ifdef _WIN32
    if(sNick != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sNick) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate sNick in BanItem::~BanItem! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
            AppendSpecialLog(sDbgstr);
        }
    }
#else
	free(sNick);
#endif

#ifdef _WIN32
    if(sReason != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sReason) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate sReason in BanItem::~BanItem! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
    }
#else
	free(sReason);
#endif

#ifdef _WIN32
    if(sBy != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sBy) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate sBy in BanItem::~BanItem! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
    }
#else
	free(sBy);
#endif
}
//---------------------------------------------------------------------------

RangeBanItem::RangeBanItem(void) {
    sIpFrom[0] = '\0';
    sIpTo[0] = '\0';
    sReason = NULL;
    sBy = NULL;

    ui8Bits = 0;

    ui32FromIpHash = 0;
    ui32ToIpHash = 0;

    tempbanexpire = 0;

    prev = NULL;
    next = NULL;

}
//---------------------------------------------------------------------------

RangeBanItem::~RangeBanItem(void) {
#ifdef _WIN32
    if(sReason != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sReason) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate sReason in RangeBanItem::~RangeBanItem! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
    }
#else
	free(sReason);
#endif

#ifdef _WIN32
    if(sBy != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sBy) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate sBy in RangeBanItem::~RangeBanItem! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
    }
#else
	free(sBy);
#endif
}
//---------------------------------------------------------------------------

hashBanMan::hashBanMan(void) {
    PermBanListS = PermBanListE = NULL;
    TempBanListS = TempBanListE = NULL;
    RangeBanListS = RangeBanListE = NULL;
    
    iSaveCalled = 0;

    for(uint32_t i = 0; i < 65536; i++) {
        nicktable[i] = NULL;
        iptable[i] = NULL;
    }
}
//---------------------------------------------------------------------------

hashBanMan::~hashBanMan(void) {
    BanItem *nextBan = PermBanListS;

    while(nextBan != NULL) {
        BanItem *curBan = nextBan;
		nextBan = curBan->next;
		delete curBan;
	}

    nextBan = TempBanListS;

    while(nextBan != NULL) {
        BanItem *curBan = nextBan;
		nextBan = curBan->next;
		delete curBan;
	}
    
    RangeBanItem *nextRangeBan = RangeBanListS;

    while(nextRangeBan != NULL) {
        RangeBanItem *curRangeBan = nextRangeBan;
		nextRangeBan = curRangeBan->next;
		delete curRangeBan;
	}

    for(uint32_t i = 0; i < 65536; i++) {
        IpTableItem * next = iptable[i];
        
        while(next != NULL) {
            IpTableItem * cur = next;
            next = cur->next;
        
            delete cur;
		}
    }
}
//---------------------------------------------------------------------------

void hashBanMan::Add(BanItem * Ban) {
	Add2Table(Ban);

    if(((Ban->ui8Bits & PERM) == PERM) == true) {
		if(PermBanListE == NULL) {
			PermBanListS = Ban;
			PermBanListE = Ban;
		} else {
			PermBanListE->next = Ban;
			Ban->prev = PermBanListE;
			PermBanListE = Ban;
		}
    } else {
		if(TempBanListE == NULL) {
			TempBanListS = Ban;
			TempBanListE = Ban;
		} else {
			TempBanListE->next = Ban;
			Ban->prev = TempBanListE;
			TempBanListE = Ban;
		}
    }

#ifdef _BUILD_GUI
	if(pBansDialog != NULL) {
        pBansDialog->AddBan(Ban);
    }
#endif
}
//---------------------------------------------------------------------------

void hashBanMan::Add2Table(BanItem *Ban) {
	if(((Ban->ui8Bits & IP) == IP) == true) {
		Add2IpTable(Ban);
    }

    if(((Ban->ui8Bits & NICK) == NICK) == true) {
		Add2NickTable(Ban);
    }
}
//---------------------------------------------------------------------------

void hashBanMan::Add2NickTable(BanItem *Ban) {
    uint16_t ui16dx = ((uint16_t *)&Ban->ui32NickHash)[0];

    if(nicktable[ui16dx] != NULL) {
        nicktable[ui16dx]->hashnicktableprev = Ban;
        Ban->hashnicktablenext = nicktable[ui16dx];
    }

    nicktable[ui16dx] = Ban;
}
//---------------------------------------------------------------------------

void hashBanMan::Add2IpTable(BanItem *Ban) {
    uint16_t ui16dx = ((uint16_t *)&Ban->ui32IpHash)[0];
    
    if(iptable[ui16dx] == NULL) {
		iptable[ui16dx] = new IpTableItem();

        if(iptable[ui16dx] == NULL) {
        	string sDbgstr = "[BUF] Cannot allocate IpTableItem in hashBanMan::AddBan2IpTable!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            exit(EXIT_FAILURE);
        }

        iptable[ui16dx]->next = NULL;
        iptable[ui16dx]->prev = NULL;

        iptable[ui16dx]->FirstBan = Ban;

        return;
    }

    IpTableItem * next = iptable[ui16dx];

    while(next != NULL) {
        IpTableItem * cur = next;
        next = cur->next;

		if(cur->FirstBan->ui32IpHash == Ban->ui32IpHash) {
			cur->FirstBan->hashiptableprev = Ban;
			Ban->hashiptablenext = cur->FirstBan;
            cur->FirstBan = Ban;

            return;
        }
    }

    IpTableItem * cur = new IpTableItem();

    if(cur == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate IpTableBans2 in hashBanMan::AddBan2IpTable!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    cur->FirstBan = Ban;

    cur->next = iptable[ui16dx];
    cur->prev = NULL;

    iptable[ui16dx]->prev = cur;
    iptable[ui16dx] = cur;
}
//---------------------------------------------------------------------------

#ifdef _BUILD_GUI
void hashBanMan::Rem(BanItem * Ban, const bool &bFromGui/* = false*/) {
#else
void hashBanMan::Rem(BanItem * Ban, const bool &/*bFromGui = false*/) {
#endif
	RemFromTable(Ban);

    if(((Ban->ui8Bits & PERM) == PERM) == true) {
		if(Ban->prev == NULL) {
			if(Ban->next == NULL) {
				PermBanListS = NULL;
				PermBanListE = NULL;
			} else {
				Ban->next->prev = NULL;
				PermBanListS = Ban->next;
			}
		} else if(Ban->next == NULL) {
			Ban->prev->next = NULL;
			PermBanListE = Ban->prev;
		} else {
			Ban->prev->next = Ban->next;
			Ban->next->prev = Ban->prev;
		}
    } else {
        if(Ban->prev == NULL) {
			if(Ban->next == NULL) {
				TempBanListS = NULL;
				TempBanListE = NULL;
			} else {
				Ban->next->prev = NULL;
				TempBanListS = Ban->next;
			   }
		} else if(Ban->next == NULL) {
			Ban->prev->next = NULL;
			TempBanListE = Ban->prev;
		} else {
			Ban->prev->next = Ban->next;
			Ban->next->prev = Ban->prev;
		}
    }

#ifdef _BUILD_GUI
    if(bFromGui == false && pBansDialog != NULL) {
        pBansDialog->RemoveBan(Ban);
    }
#endif
}
//---------------------------------------------------------------------------

void hashBanMan::RemFromTable(BanItem *Ban) {
    if(((Ban->ui8Bits & IP) == IP) == true) {
		RemFromIpTable(Ban);
    }

    if(((Ban->ui8Bits & NICK) == NICK) == true) {
		RemFromNickTable(Ban);
    }
}
//---------------------------------------------------------------------------

void hashBanMan::RemFromNickTable(BanItem *Ban) {
    if(Ban->hashnicktableprev == NULL) {
        uint16_t ui16dx = ((uint16_t *)&Ban->ui32NickHash)[0];

        if(Ban->hashnicktablenext == NULL) {
            nicktable[ui16dx] = NULL;
        } else {
            Ban->hashnicktablenext->hashnicktableprev = NULL;
            nicktable[ui16dx] = Ban->hashnicktablenext;
        }
    } else if(Ban->hashnicktablenext == NULL) {
        Ban->hashnicktableprev->hashnicktablenext = NULL;
    } else {
        Ban->hashnicktableprev->hashnicktablenext = Ban->hashnicktablenext;
        Ban->hashnicktablenext->hashnicktableprev = Ban->hashnicktableprev;
    }

    Ban->hashnicktableprev = NULL;
    Ban->hashnicktablenext = NULL;
}
//---------------------------------------------------------------------------

void hashBanMan::RemFromIpTable(BanItem *Ban) {   
	uint16_t ui16dx = ((uint16_t *)&Ban->ui32IpHash)[0];

	if(Ban->hashiptableprev == NULL) {
        IpTableItem * next = iptable[ui16dx];

        while(next != NULL) {
            IpTableItem * cur = next;
            next = cur->next;

			if(cur->FirstBan->ui32IpHash == Ban->ui32IpHash) {
				if(Ban->hashiptablenext == NULL) {
					if(cur->prev == NULL) {
						if(cur->next == NULL) {
                            delete cur;

                            iptable[ui16dx] = NULL;
						} else {
							cur->next->prev = NULL;
                            iptable[ui16dx] = cur->next;
                        }
					} else if(cur->next == NULL) {
						cur->prev->next = NULL;
					} else {
						cur->prev->next = cur->next;
                        cur->next->prev = cur->prev;
                    }
				} else {
					Ban->hashiptablenext->hashiptableprev = NULL;
                    cur->FirstBan = Ban->hashiptablenext;
                }

                break;
            }
        }
	} else if(Ban->hashiptablenext == NULL) {
		Ban->hashiptableprev->hashiptablenext = NULL;
	} else {
        Ban->hashiptableprev->hashiptablenext = Ban->hashiptablenext;
        Ban->hashiptablenext->hashiptableprev = Ban->hashiptableprev;
    }

    Ban->hashiptableprev = NULL;
    Ban->hashiptablenext = NULL;
}
//---------------------------------------------------------------------------

BanItem* hashBanMan::Find(BanItem *Ban) {
	if(TempBanListS != NULL) {
        time_t acc_time;
        time(&acc_time);

		BanItem *nextBan = TempBanListS;

        while(nextBan != NULL) {
            BanItem *curBan = nextBan;
    		nextBan = curBan->next;

            if(acc_time > curBan->tempbanexpire) {
				Rem(curBan);
				delete curBan;

                continue;
            }

			if(curBan == Ban) {
				return curBan;
			}
		}
    }
    
	if(PermBanListS != NULL) {
		BanItem *nextBan = PermBanListS;

        while(nextBan != NULL) {
            BanItem *curBan = nextBan;
    		nextBan = curBan->next;

			if(curBan == Ban) {
				return curBan;
			}
        }
	}

	return NULL;
}
//---------------------------------------------------------------------------

void hashBanMan::Remove(BanItem *Ban) {
	if(TempBanListS != NULL) {
        time_t acc_time;
        time(&acc_time);

		BanItem *nextBan = TempBanListS;

        while(nextBan != NULL) {
            BanItem *curBan = nextBan;
    		nextBan = curBan->next;

            if(acc_time > curBan->tempbanexpire) {
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
    
	if(PermBanListS != NULL) {
		BanItem *nextBan = PermBanListS;

        while(nextBan != NULL) {
            BanItem *curBan = nextBan;
    		nextBan = curBan->next;

			if(curBan == Ban) {
				Rem(Ban);
				delete Ban;

				return;
			}
        }
	}
}
//---------------------------------------------------------------------------

void hashBanMan::AddRange(RangeBanItem *RangeBan) {
    if(RangeBanListE == NULL) {
    	RangeBanListS = RangeBan;
    	RangeBanListE = RangeBan;
    } else {
    	RangeBanListE->next = RangeBan;
        RangeBan->prev = RangeBanListE;
        RangeBanListE = RangeBan;
    }

#ifdef _BUILD_GUI
    if(pRangeBansDialog != NULL) {
        pRangeBansDialog->AddRangeBan(RangeBan);
    }
#endif
}
//---------------------------------------------------------------------------

#ifdef _BUILD_GUI
void hashBanMan::RemRange(RangeBanItem *RangeBan, const bool &bFromGui/* = false*/) {
#else
void hashBanMan::RemRange(RangeBanItem *RangeBan, const bool &/*bFromGui = false*/) {
#endif
    if(RangeBan->prev == NULL) {
        if(RangeBan->next == NULL) {
            RangeBanListS = NULL;
            RangeBanListE = NULL;
        } else {
            RangeBan->next->prev = NULL;
            RangeBanListS = RangeBan->next;
        }
    } else if(RangeBan->next == NULL) {
        RangeBan->prev->next = NULL;
        RangeBanListE = RangeBan->prev;
    } else {
        RangeBan->prev->next = RangeBan->next;
        RangeBan->next->prev = RangeBan->prev;
    }

#ifdef _BUILD_GUI
    if(bFromGui == false && pRangeBansDialog != NULL) {
        pRangeBansDialog->RemoveRangeBan(RangeBan);
    }
#endif
}
//---------------------------------------------------------------------------

RangeBanItem* hashBanMan::FindRange(RangeBanItem *RangeBan) {
	if(RangeBanListS != NULL) {
        time_t acc_time;
        time(&acc_time);

		RangeBanItem *nextBan = RangeBanListS;

		while(nextBan != NULL) {
            RangeBanItem *curBan = nextBan;
			nextBan = curBan->next;

			if(((curBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true && acc_time > curBan->tempbanexpire) {
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

void hashBanMan::RemoveRange(RangeBanItem *RangeBan) {
	if(RangeBanListS != NULL) {
        time_t acc_time;
        time(&acc_time);

		RangeBanItem *nextBan = RangeBanListS;

		while(nextBan != NULL) {
            RangeBanItem *curBan = nextBan;
    		nextBan = curBan->next;

			if(((curBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true && acc_time > curBan->tempbanexpire) {
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

BanItem* hashBanMan::FindNick(User* u) {
    uint16_t ui16dx = ((uint16_t *)&u->ui32NickHash)[0];

    time_t acc_time;
    time(&acc_time);

    BanItem *next = nicktable[ui16dx];

    while(next != NULL) {
        BanItem *cur = next;
        next = cur->hashnicktablenext;

#ifdef _WIN32
        if(cur->ui32NickHash == u->ui32NickHash && stricmp(cur->sNick, u->Nick) == 0) {
#else
		if(cur->ui32NickHash == u->ui32NickHash && strcasecmp(cur->sNick, u->Nick) == 0) {
#endif
            // PPK ... check if temban expired
			if(((cur->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
                if(acc_time >= cur->tempbanexpire) {
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

BanItem* hashBanMan::FindIP(User* u) {
    uint16_t ui16dx = ((uint16_t *)&u->ui32IpHash)[0];

    IpTableItem * next = iptable[ui16dx];

    time_t acc_time;
    time(&acc_time);

    while(next != NULL) {
        IpTableItem * cur = next;
        next = cur->next;

		if(cur->FirstBan->ui32IpHash == u->ui32IpHash) {
			BanItem * nextBan = cur->FirstBan;

			while(nextBan != NULL) {
                BanItem * curBan = nextBan;
                nextBan = curBan->hashiptablenext;
                
                // PPK ... check if temban expired
				if(((curBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
                    if(acc_time >= curBan->tempbanexpire) {
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

RangeBanItem* hashBanMan::FindRange(User* u) {
    RangeBanItem *next = RangeBanListS;

    time_t acc_time;
    time(&acc_time);
                
    while(next != NULL) {
        RangeBanItem *cur = next;
        next = cur->next;

        if(cur->ui32FromIpHash <= u->ui32IpHash && cur->ui32ToIpHash >= u->ui32IpHash) {
            // PPK ... check if temban expired
            if(((cur->ui8Bits & TEMP) == TEMP) == true) {
                if(acc_time >= cur->tempbanexpire) {
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

BanItem* hashBanMan::FindFull(const uint32_t &hash) {
    time_t acc_time;
    time(&acc_time);

    return FindFull(hash, acc_time);
}
//---------------------------------------------------------------------------

BanItem* hashBanMan::FindFull(const uint32_t &hash, const time_t &acc_time) {
    uint16_t ui16dx = ((uint16_t *)&hash)[0];

	IpTableItem * next = iptable[ui16dx];

    BanItem *fnd = NULL;

    while(next != NULL) {
        IpTableItem * cur = next;
        next = cur->next;

		if(cur->FirstBan->ui32IpHash == hash) {
			BanItem * nextBan = cur->FirstBan;

            while(nextBan != NULL) {
                BanItem * curBan = nextBan;
                nextBan = curBan->hashiptablenext;
        
                // PPK ... check if temban expired
				if(((curBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
                    if(acc_time >= curBan->tempbanexpire) {
						Rem(curBan);
                        delete curBan;
    
    					continue;
                    }
                }
                    
				if(((curBan->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == true) {
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

RangeBanItem* hashBanMan::FindFullRange(const uint32_t &hash, const time_t &acc_time) {
    RangeBanItem *fnd = NULL,
        *next = RangeBanListS;

    while(next != NULL) {
        RangeBanItem *cur = next;
        next = cur->next;

        if(cur->ui32FromIpHash <= hash && cur->ui32ToIpHash >= hash) {
            // PPK ... check if temban expired
            if(((cur->ui8Bits & TEMP) == TEMP) == true) {
                if(acc_time >= cur->tempbanexpire) {
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

BanItem* hashBanMan::FindNick(char * sNick, const size_t &iNickLen) {
    uint32_t hash = HashNick(sNick, iNickLen);

    time_t acc_time;
    time(&acc_time);

    return FindNick(hash, acc_time, sNick);
}
//---------------------------------------------------------------------------

BanItem* hashBanMan::FindNick(const uint32_t &ui32Hash, const time_t &acc_time, char * sNick) {
    uint16_t ui16dx = ((uint16_t *)&ui32Hash)[0];

	BanItem *next = nicktable[ui16dx];

    while(next != NULL) {
        BanItem *cur = next;
        next = cur->hashnicktablenext;

#ifdef _WIN32
        if(cur->ui32NickHash == ui32Hash && stricmp(cur->sNick, sNick) == 0) {
#else
		if(cur->ui32NickHash == ui32Hash && strcasecmp(cur->sNick, sNick) == 0) {
#endif
            // PPK ... check if temban expired
			if(((cur->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
                if(acc_time >= cur->tempbanexpire) {
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

BanItem* hashBanMan::FindIP(const uint32_t &hash, const time_t &acc_time) {
    uint16_t ui16dx = ((uint16_t *)&hash)[0];

    IpTableItem * next = iptable[ui16dx];

    while(next != NULL) {
        IpTableItem * cur = next;
        next = cur->next;

        if(cur->FirstBan->ui32IpHash == hash) {
			BanItem * nextBan = cur->FirstBan;

            while(nextBan != NULL) {
                BanItem * curBan = nextBan;
                nextBan = curBan->hashiptablenext;

                // PPK ... check if temban expired
				if(((curBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
                    if(acc_time >= curBan->tempbanexpire) {
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

RangeBanItem* hashBanMan::FindRange(const uint32_t &hash, const time_t &acc_time) {
    RangeBanItem *next = RangeBanListS;

    while(next != NULL) {
        RangeBanItem *cur = next;
        next = cur->next;

        if(cur->ui32FromIpHash <= hash && cur->ui32ToIpHash >= hash) {
            // PPK ... check if temban expired
            if(((cur->ui8Bits & TEMP) == TEMP) == true) {
                if(acc_time >= cur->tempbanexpire) {
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

RangeBanItem* hashBanMan::FindRange(const uint32_t &fromhash, const uint32_t &tohash, const time_t &acc_time) {
    RangeBanItem *next = RangeBanListS;

    while(next != NULL) {
        RangeBanItem *cur = next;
        next = cur->next;

        if(cur->ui32FromIpHash == fromhash && cur->ui32ToIpHash == tohash) {
            // PPK ... check if temban expired
            if(((cur->ui8Bits & TEMP) == TEMP) == true) {
                if(acc_time >= cur->tempbanexpire) {
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

BanItem* hashBanMan::FindTempNick(char * sNick, const size_t &iNickLen) {
    uint32_t hash = HashNick(sNick, iNickLen);

    time_t acc_time;
    time(&acc_time);

    return FindTempNick(hash, acc_time, sNick);
}
//---------------------------------------------------------------------------

BanItem* hashBanMan::FindTempNick(const uint32_t &ui32Hash,  const time_t &acc_time, char * sNick) {
    uint16_t ui16dx = ((uint16_t *)&ui32Hash)[0];

	BanItem *next = nicktable[ui16dx];

    while(next != NULL) {
        BanItem *cur = next;
        next = cur->hashnicktablenext;

#ifdef _WIN32
        if(cur->ui32NickHash == ui32Hash && stricmp(cur->sNick, sNick) == 0) {
#else
		if(cur->ui32NickHash == ui32Hash && strcasecmp(cur->sNick, sNick) == 0) {
#endif
            // PPK ... check if temban expired
			if(((cur->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
                if(acc_time >= cur->tempbanexpire) {
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

BanItem* hashBanMan::FindTempIP(const uint32_t &hash, const time_t &acc_time) {
    uint16_t ui16dx = ((uint16_t *)&hash)[0];

    IpTableItem * next = iptable[ui16dx];

    while(next != NULL) {
        IpTableItem * cur = next;
        next = cur->next;

        if(cur->FirstBan->ui32IpHash == hash) {
			BanItem * nextBan = cur->FirstBan;

            while(nextBan != NULL) {
                BanItem * curBan = nextBan;
                nextBan = curBan->hashiptablenext;
                
                // PPK ... check if temban expired
				if(((curBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
                    if(acc_time >= curBan->tempbanexpire) {
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

BanItem* hashBanMan::FindPermNick(char * sNick, const size_t &iNickLen) {
    uint32_t hash = HashNick(sNick, iNickLen);
    
	return FindPermNick(hash, sNick);
}
//---------------------------------------------------------------------------

BanItem* hashBanMan::FindPermNick(const uint32_t &ui32Hash, char * sNick) {
    uint16_t ui16dx = ((uint16_t *)&ui32Hash)[0];

    BanItem *next = nicktable[ui16dx];

    while(next != NULL) {
        BanItem *cur = next;
        next = cur->hashnicktablenext;

#ifdef _WIN32
        if(cur->ui32NickHash == ui32Hash && stricmp(cur->sNick, sNick) == 0) {
#else
		if(cur->ui32NickHash == ui32Hash && strcasecmp(cur->sNick, sNick) == 0) {
#endif
            if(((cur->ui8Bits & hashBanMan::PERM) == hashBanMan::PERM) == true) {
                return cur;
            }
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

BanItem* hashBanMan::FindPermIP(const uint32_t &hash) {
    uint16_t ui16dx = ((uint16_t *)&hash)[0];

	IpTableItem * next = iptable[ui16dx];

    while(next != NULL) {
        IpTableItem * cur = next;
        next = cur->next;

        if(cur->FirstBan->ui32IpHash == hash) {
            BanItem * nextBan = cur->FirstBan;

            while(nextBan != NULL) {
                BanItem * curBan = nextBan;
                nextBan = curBan->hashiptablenext;
                
                if(((curBan->ui8Bits & hashBanMan::PERM) == hashBanMan::PERM) == true) {
                    return curBan;
                }
            }
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

void hashBanMan::Load(void) {
    double dVer;

#ifdef _WIN32
    TiXmlDocument doc((PATH+"\\cfg\\BanList.xml").c_str());
#else
	TiXmlDocument doc((PATH+"/cfg/BanList.xml").c_str());
#endif

    if(doc.LoadFile()) {
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

                    BanItem *Ban = new BanItem();
                    if(Ban == NULL) {
                    	string sDbgstr = "[BUF] Cannot allocate Ban in hashBanMan::LoadXmlBanList!";
#ifdef _WIN32
						sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
						AppendSpecialLog(sDbgstr);
                    	exit(EXIT_FAILURE);
                    }

                    if(type == 0) {
                        Ban->ui8Bits |= PERM;
                    } else {
                        Ban->ui8Bits |= TEMP;
                    }

                    // PPK ... ipban
                    if(ipban == true) {
                        uint32_t a, b, c, d;
                        if(GetIpParts((char *)ip, strlen(ip), a, b, c, d) == true) {
                            strcpy(Ban->sIp, ip);
                            Ban->ui32IpHash = 16777216 * a + 65536 * b + 256 * c + d;
                            Ban->ui8Bits |= IP;

                            if(fullipban == true)
                                Ban->ui8Bits |= FULL;
                        } else {
							delete Ban;

                            continue;
                        }
                    }

                    // PPK ... nickban
                    if(nickban == true) {
                        if(nick != NULL) {
                            size_t iNickLen = strlen(nick);
#ifdef _WIN32
                            Ban->sNick = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iNickLen+1);
#else
							Ban->sNick = (char *) malloc(iNickLen+1);
#endif
                            if(Ban->sNick == NULL) {
								string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iNickLen+1))+
									" bytes of memory for sNick in hashBanMan::LoadXmlBanList!";
#ifdef _WIN32
								sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
								AppendSpecialLog(sDbgstr);
                                exit(EXIT_FAILURE);
                                return;
                            }
                            memcpy(Ban->sNick, nick, iNickLen);
                            Ban->sNick[iNickLen] = '\0';
                            Ban->ui32NickHash = HashNick(Ban->sNick, strlen(Ban->sNick));
                            Ban->ui8Bits |= NICK;
                        } else {
                            delete Ban;

                            continue;
                        }
                    }

                    if(reason != NULL) {
                        size_t iReasonLen = strlen(reason);
                        if(iReasonLen > 255) {
#ifdef _WIN32
                            Ban->sReason = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, 256);
#else
							Ban->sReason = (char *) malloc(256);
#endif
                            if(Ban->sReason == NULL) {
                            	string sDbgstr = "[BUF] Cannot allocate 256 bytes of memory for sReason in hashBanMan::LoadXmlBanList!";
#ifdef _WIN32
								sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
								AppendSpecialLog(sDbgstr);
                                exit(EXIT_FAILURE);
                                return;
                            }
                            memcpy(Ban->sReason, reason, 252);
                            Ban->sReason[255] = '\0';
                            Ban->sReason[254] = '.';
                            Ban->sReason[253] = '.';
                            Ban->sReason[252] = '.';
                        } else {
#ifdef _WIN32
                            Ban->sReason = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iReasonLen+1);
#else
							Ban->sReason = (char *) malloc(iReasonLen+1);
#endif
                            if(Ban->sReason == NULL) {
								string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iReasonLen+1))+
									" bytes of memory for sReason1 in hashBanMan::LoadXmlBanList!";
#ifdef _WIN32
								sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
								AppendSpecialLog(sDbgstr);
                                exit(EXIT_FAILURE);
                                return;
                            }
                            memcpy(Ban->sReason, reason, iReasonLen);
                            Ban->sReason[iReasonLen] = '\0';
                        }
                    }

                    if(by != NULL) {
                        size_t iByLen = strlen(by);
                        if(iByLen > 63) {
#ifdef _WIN32
                            Ban->sBy = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, 64);
#else
							Ban->sBy = (char *) malloc(64);
#endif
                            if(Ban->sBy == NULL) {
                            	string sDbgstr = "[BUF] Cannot allocate 64 bytes of memory for sBy in hashBanMan::LoadXmlBanList!";
#ifdef _WIN32
								sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
								AppendSpecialLog(sDbgstr);
                                exit(EXIT_FAILURE);
                                return;
                            }
                            memcpy(Ban->sBy, by, 63);
                            Ban->sBy[63] = '\0';
                        } else {
#ifdef _WIN32
                            Ban->sBy = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iByLen+1);
#else
							Ban->sBy = (char *) malloc(iByLen+1);
#endif
                            if(Ban->sBy == NULL) {
								string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iByLen+1))+
									" bytes of memory for sBy1 in hashBanMan::LoadXmlBanList!";
#ifdef _WIN32
								sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
                                AppendSpecialLog(sDbgstr);
                                exit(EXIT_FAILURE);
                                return;
                            }
                            memcpy(Ban->sBy, by, iByLen);
                            Ban->sBy[iByLen] = '\0';
                        }
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
                        Ban->tempbanexpire = expire;
                    }
                    
                    if(fullipban == true) {
                        Ban->ui8Bits |= FULL;
                    }

                    Add(Ban);
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

                    RangeBanItem *RangeBan = new RangeBanItem();
                    if(RangeBan == NULL) {
                    	string sDbgstr = "[BUF] Cannot allocate RangeBan in hashBanMan::LoadXmlBanList!";
#ifdef _WIN32
						sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
						AppendSpecialLog(sDbgstr);
                    	exit(EXIT_FAILURE);
                    }

                    if(type == 0) {
                        RangeBan->ui8Bits |= PERM;
                    } else {
                        RangeBan->ui8Bits |= TEMP;
                    }

                    // PPK ... fromip
                    if(HashIP((char *)ipfrom, strlen(ipfrom), RangeBan->ui32FromIpHash) == true) {
                        strcpy(RangeBan->sIpFrom, ipfrom);
                    } else {
                        delete RangeBan;

                        continue;
                    }

                    // PPK ... toip
                    if(HashIP((char *)ipto, strlen(ipto), RangeBan->ui32ToIpHash) == true && RangeBan->ui32ToIpHash > RangeBan->ui32FromIpHash) {
                        strcpy(RangeBan->sIpTo, ipto);
                    } else {
                        delete RangeBan;

                        continue;
                    }

                    if(reason != NULL) {
                        size_t iReasonLen = strlen(reason);
                        if(iReasonLen > 255) {
#ifdef _WIN32
                            RangeBan->sReason = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, 256);
#else
							RangeBan->sReason = (char *) malloc(256);
#endif
                            if(RangeBan->sReason == NULL) {
                            	string sDbgstr = "[BUF] Cannot allocate 256 bytes of memory for sReason2 in hashBanMan::LoadXmlBanList!";
#ifdef _WIN32
								sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
								AppendSpecialLog(sDbgstr);
                                exit(EXIT_FAILURE);
                                return;
                            }
                            memcpy(RangeBan->sReason, reason, 252);
                            RangeBan->sReason[255] = '\0';
                            RangeBan->sReason[254] = '.';
                            RangeBan->sReason[253] = '.';
                            RangeBan->sReason[252] = '.';
                        } else {
#ifdef _WIN32
                            RangeBan->sReason = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iReasonLen+1);
#else
							RangeBan->sReason = (char *) malloc(iReasonLen+1);
#endif
                            if(RangeBan->sReason == NULL) {
								string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iReasonLen+1))+
									" bytes of memory for sReason3 in hashBanMan::LoadXmlBanList!";
#ifdef _WIN32
								sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
								AppendSpecialLog(sDbgstr);
                                exit(EXIT_FAILURE);
                                return;
                            }
                            memcpy(RangeBan->sReason, reason, iReasonLen);
                            RangeBan->sReason[iReasonLen] = '\0';
                        }
                    }

                    if(by != NULL) {
                        size_t iByLen = strlen(by);
                        if(iByLen > 63) {
#ifdef _WIN32
                            RangeBan->sBy = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, 64);
#else
							RangeBan->sBy = (char *) malloc(64);
#endif
                            if(RangeBan->sBy == NULL) {
                            	string sDbgstr = "[BUF] Cannot allocate 64 bytes of memory for sBy2 in hashBanMan::LoadXmlBanList!";
#ifdef _WIN32
								sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
								AppendSpecialLog(sDbgstr);
                                exit(EXIT_FAILURE);
                                return;
                            }
                            memcpy(RangeBan->sBy, by, 63);
                            RangeBan->sBy[63] = '\0';
                        } else {
#ifdef _WIN32
                            RangeBan->sBy = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iByLen+1);
#else
							RangeBan->sBy = (char *) malloc(iByLen+1);
#endif
                            if(RangeBan->sBy == NULL) {
								string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iByLen+1))+
									" bytes of memory for sBy3 in hashBanMan::LoadXmlBanList!";
#ifdef _WIN32
								sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
								AppendSpecialLog(sDbgstr);
                                exit(EXIT_FAILURE);
                                return;
                            }
                            memcpy(RangeBan->sBy, by, iByLen);
                            RangeBan->sBy[iByLen] = '\0';
                        }
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
                        RangeBan->tempbanexpire = expire;
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

void hashBanMan::Save(bool bForce/* = false*/) {
    if(bForce == false) {
        // PPK ... we don't want to kill HDD with save after any change in banlist
        if(iSaveCalled < 100) {
            iSaveCalled++;
            return;
        }
    }
    
    iSaveCalled = 0;

#ifdef _WIN32
    TiXmlDocument doc((PATH+"\\cfg\\BanList.xml").c_str());
#else
	TiXmlDocument doc((PATH+"/cfg/BanList.xml").c_str());
#endif

    doc.InsertEndChild(TiXmlDeclaration("1.0", "windows-1252", "yes"));
    TiXmlElement banlist("BanList");
    banlist.SetDoubleAttribute("version", 2.0);

    TiXmlElement bans("Bans");

    if(TempBanListS != NULL) {
        BanItem *next = TempBanListS;

        while(next != NULL) {
            BanItem *cur = next;
            next = cur->next;
           
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
			expire.InsertEndChild(TiXmlText(string((uint32_t)cur->tempbanexpire).c_str()));
            
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

    if(PermBanListS != NULL) {
        BanItem *next = PermBanListS;

        while(next != NULL) {
            BanItem *cur = next;
            next = cur->next;
           
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
			expire.InsertEndChild(TiXmlText(string((uint32_t)cur->tempbanexpire).c_str()));
            
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

    if(RangeBanListS != NULL) {
        RangeBanItem *next = RangeBanListS;

        while(next != NULL) {
            RangeBanItem *cur = next;
            next = cur->next;
            
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
			expire.InsertEndChild(TiXmlText(string((uint32_t)cur->tempbanexpire).c_str()));
            
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

void hashBanMan::CreateTemp(char * first, char * second, const uint32_t &iTime, const time_t &acc_time) {
    uint32_t a, b, c, d;
    if(GetIpParts(first, strlen(first), a, b, c, d) == false)
        return;
    
    BanItem *Ban = new BanItem();
    if(Ban == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate Ban in hashBanMan::CreateTempBan!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    // PPK ... set ban type to TEMP
    Ban->ui8Bits |= TEMP;

    // PPK ... temp ban ip
    strcpy(Ban->sIp, first);
    Ban->ui32IpHash = 16777216 * a + 65536 * b + 256 * c + d;
    Ban->ui8Bits |= IP;

    // PPK ... temp ban nick
    size_t iNickLen = strlen(second);
#ifdef _WIN32
    Ban->sNick = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iNickLen+1);
#else
	Ban->sNick = (char *) malloc(iNickLen+1);
#endif
    if(Ban->sNick == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iNickLen+1))+
			" bytes of memory in hashBanMan::CreateTempBan!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
        AppendSpecialLog(sDbgstr);
        delete Ban;

        return;
    }   
    memcpy(Ban->sNick, second, iNickLen);
    Ban->sNick[iNickLen] = '\0';
    Ban->ui32NickHash = HashNick(Ban->sNick, strlen(Ban->sNick));
    Ban->ui8Bits |= NICK;

    // PPK ... temp ban expiration
    Ban->tempbanexpire = (iTime*60) + acc_time;

    // PPK ... old tempban not allow same ip/nick bans, no check needed ;)
    Add(Ban);
}
//---------------------------------------------------------------------------

void hashBanMan::CreatePerm(char * first, char * second){
    BanItem *Ban = new BanItem();
    if(Ban == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate Ban in hashBanMan::CreatePermBan!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    // PPK ... set ban type to perm
    Ban->ui8Bits |= PERM;

    uint32_t a, b, c, d;
    if(GetIpParts(first, strlen(first), a, b, c, d) == true) {
        // PPK ... perm ban ip
        strcpy(Ban->sIp, first);
        Ban->ui32IpHash = 16777216 * a + 65536 * b + 256 * c + d;
        Ban->ui8Bits |= IP;
        
        if(second != NULL) {
#ifdef _WIN32
            if(stricmp(second, "3x bad password") == 0) {
#else
			if(strcasecmp(second, "3x bad password") == 0) {
#endif
                size_t iReasonLen = strlen(second);
#ifdef _WIN32
                Ban->sReason = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iReasonLen+1);
#else
				Ban->sReason = (char *) malloc(iReasonLen+1);
#endif
                if(Ban->sReason == NULL) {
					string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iReasonLen+1))+
						" bytes of memory for sReason in hashBanMan::CreatePermBan!";
#ifdef _WIN32
					sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
					AppendSpecialLog(sDbgstr);
                    delete Ban;

                    return;
                }   
                memcpy(Ban->sReason, second, iReasonLen);
                Ban->sReason[iReasonLen] = '\0';
                Ban->ui8Bits |= FULL;
            } else {
                // PPK ... perm ban nick
                size_t iNickLen = strlen(second);
#ifdef _WIN32
                Ban->sNick = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iNickLen+1);
#else
				Ban->sNick = (char *) malloc(iNickLen+1);
#endif
                if(Ban->sNick == NULL) {
					string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iNickLen+1))+
						" bytes of memory for sNick in hashBanMan::CreatePermBan!";
#ifdef _WIN32
					sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
					AppendSpecialLog(sDbgstr);
                    delete Ban;

                    return;
                }   
                memcpy(Ban->sNick, second, iNickLen);
                Ban->sNick[iNickLen] = '\0';
                Ban->ui32NickHash = HashNick(Ban->sNick, strlen(Ban->sNick));
                Ban->ui8Bits |= NICK;
            }
        }
    } else {
        // PPK ... perm ban nick
        size_t iNickLen = strlen(first);
#ifdef _WIN32
        Ban->sNick = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iNickLen+1);
#else
		Ban->sNick = (char *) malloc(iNickLen+1);
#endif
        if(Ban->sNick == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iNickLen+1))+
				" bytes of memory for sNick1 in hashBanMan::CreatePermBan!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            delete Ban;

            return;
        }   
        memcpy(Ban->sNick, first, iNickLen);
        Ban->sNick[iNickLen] = '\0';
        Ban->ui32NickHash = HashNick(Ban->sNick, strlen(Ban->sNick));
        Ban->ui8Bits |= NICK;
        
        if(second != NULL && GetIpParts(second, strlen(second), a, b, c, d) == true) {
            // PPK ... perm ban ip
            strcpy(Ban->sIp, second);
            Ban->ui32IpHash = 16777216 * a + 65536 * b + 256 * c + d;
            Ban->ui8Bits |= IP;
        }
    }
    
    time_t acc_time;
    time(&acc_time);
    
    // PPK ... check existing bans for duplicity
    if(((Ban->ui8Bits & NICK) == NICK) == true) {
        BanItem *nxtBan;
        
        // PPK ... not allow same nickbans !
		if((nxtBan = FindNick(Ban->ui32NickHash, acc_time, Ban->sNick)) != NULL) {
            if(((nxtBan->ui8Bits & PERM) == PERM) == true) {
                if(((nxtBan->ui8Bits & IP) == IP) == true) {
                    if(((Ban->ui8Bits & IP) == IP) == true) {
                        if(nxtBan->ui32IpHash == Ban->ui32IpHash) {
                            // PPK ... same perm ban already exist, delete new
                            delete Ban;

                            return;
                        } else {
                            // PPK ... already existing perm ban have same nick but diferent ip, set new ban to ip ban only
                            Ban->ui8Bits &= ~NICK;
                        }
                    } else {
                        // PPK ... new ban is only nick ban, delete it
                        delete Ban;

                        return;
                    }
                } else {
                    if(((Ban->ui8Bits & IP) == IP) == false) {
                        // PPK ... old ban is only nickban, new ban is only nickban too... delete new
                        delete Ban;

                        return;
                    } else {
                        // PPK ... old ban is only nick ban, remove it
                        Rem(nxtBan);
                        delete nxtBan;
					}
                }
            } else {
                if(((nxtBan->ui8Bits & IP) == IP) == true) {
                    if(((Ban->ui8Bits & IP) == IP) == true) {
                        if(nxtBan->ui32IpHash == Ban->ui32IpHash) {
                            // PPK ... same ban already exist, delete old tempban
                            Rem(nxtBan);
                            delete nxtBan;
						} else {
                            // PPK ... already existing temp ban have same nick but diferent ip, set old ban to ip ban only
							RemFromNickTable(nxtBan);
                            nxtBan->ui8Bits &= ~NICK;
                        }
                    } else {
						// PPK ... new ban is only nick ban, set old ban to ip ban only
						RemFromNickTable(nxtBan);
                        nxtBan->ui8Bits &= ~NICK;
                    }
                } else {
                    // PPK ... old ban is only nick ban, remove it
                    Rem(nxtBan);
                    delete nxtBan;
				}
            }
        }

        // PPK ... not allow bans with same ip and nick !
        if(((Ban->ui8Bits & IP) == IP) == true) {
			nxtBan = FindIP(Ban->ui32IpHash, acc_time);
            
            while(nxtBan != NULL) {
                BanItem *curBan = nxtBan;
                nxtBan = curBan->hashiptablenext;

                if(((curBan->ui8Bits & PERM) == PERM) == true) {
                    if(((curBan->ui8Bits & NICK) == NICK) == false && curBan->ui32NickHash == curBan->ui32NickHash) {
                        // PPK ... ban with same ip and nick, set nickban to old ban and delete new ban
                        curBan->ui8Bits |= NICK;
						Add2NickTable(curBan);
                        
                        delete Ban;

						return;
                    }
                } else {
                    if(((curBan->ui8Bits & NICK) == NICK) == false && curBan->ui32NickHash == curBan->ui32NickHash) {
                        // PPK ... old ban with same ip and nick is only tempban, delete it
                        Rem(curBan);
                        delete curBan;
					}
                }
            }
        }
    }
    
    Add(Ban);
}
//---------------------------------------------------------------------------

void hashBanMan::ClearTemp(void) {
    BanItem *nextBan = TempBanListS;

    while(nextBan != NULL) {
        BanItem *curBan = nextBan;
        nextBan = curBan->next;

        Rem(curBan);
        delete curBan;
	}
    
	Save();
}
//---------------------------------------------------------------------------

void hashBanMan::ClearPerm(void) {
    BanItem *nextBan = PermBanListS;

    while(nextBan != NULL) {
        BanItem *curBan = nextBan;
        nextBan = curBan->next;

        Rem(curBan);
        delete curBan;
	}
    
	Save();
}
//---------------------------------------------------------------------------

void hashBanMan::ClearRange(void) {
    RangeBanItem *nextBan = RangeBanListS;

    while(nextBan != NULL) {
        RangeBanItem *curBan = nextBan;
        nextBan = curBan->next;

        RemRange(curBan);
        delete curBan;
	}

	Save();
}
//---------------------------------------------------------------------------

void hashBanMan::ClearTempRange(void) {
    RangeBanItem *nextBan = RangeBanListS;

    while(nextBan != NULL) {
        RangeBanItem *curBan = nextBan;
        nextBan = curBan->next;

        if(((curBan->ui8Bits & TEMP) == TEMP) == true) {
            RemRange(curBan);
            delete curBan;
		}
    }
    
	Save();
}
//---------------------------------------------------------------------------

void hashBanMan::ClearPermRange(void) {
    RangeBanItem *nextBan = RangeBanListS;

    while(nextBan != NULL) {
        RangeBanItem *curBan = nextBan;
        nextBan = curBan->next;

        if(((curBan->ui8Bits & PERM) == PERM) == true) {
            RemRange(curBan);
            delete curBan;
		}
    }

	Save();
}
//---------------------------------------------------------------------------

void hashBanMan::Ban(User * u, const char * sReason, char * sBy, const bool &bFull) {
    time_t acc_time;
    time(&acc_time);
    
    BanItem *Ban = new BanItem();
    if(Ban == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate Ban in hashBanMan::Ban!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
		return;
    }
    Ban->ui8Bits |= PERM;

    strcpy(Ban->sIp, u->IP);
    Ban->ui32IpHash = u->ui32IpHash;
    Ban->ui8Bits |= IP;
    
    if(bFull == true)
        Ban->ui8Bits |= FULL;
    
    // PPK ... check for <unknown> nick -> bad ban from script
    if(u->Nick[0] != '<') {
#ifdef _WIN32
        Ban->sNick = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, u->NickLen+1);
#else
		Ban->sNick = (char *) malloc(u->NickLen+1);
#endif
		if(Ban->sNick == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string(u->NickLen+1)+
				" bytes of memory for sNick in hashBanMan::Ban!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
			delete Ban;

			return;
		}
		memcpy(Ban->sNick, u->Nick, u->NickLen);
		Ban->sNick[u->NickLen] = '\0';
		Ban->ui32NickHash = u->ui32NickHash;
		Ban->ui8Bits |= NICK;
        
        // PPK ... not allow same nickbans ! i don't want this check here, but lame scripter find way to ban same nick/ip multiple times :(
		BanItem *nxtBan = FindNick(Ban->ui32NickHash, acc_time, Ban->sNick);

        if(nxtBan != NULL) {
            if(((nxtBan->ui8Bits & PERM) == PERM) == true) {
                if(((nxtBan->ui8Bits & IP) == IP) == true) {
                    if(Ban->ui32IpHash == nxtBan->ui32IpHash) {
                        if(((Ban->ui8Bits & FULL) == FULL) == false) {
                            // PPK ... same ban and new is not full, delete new
                            delete Ban;

                            return;
                        } else {
                            if(((nxtBan->ui8Bits & FULL) == FULL) == true) {
                                // PPK ... same ban and both full, delete new
                                delete Ban;

                                return;
                            } else {
                                // PPK ... same ban but only new is full, delete old
                                Rem(nxtBan);
                                delete nxtBan;
							}
                        }
                    } else {
                        Ban->ui8Bits &= ~NICK;
                    }
                } else {
                    // PPK ... old ban is only nickban, remove it
                    Rem(nxtBan);
                    delete nxtBan;
				}
            } else {
                if(((nxtBan->ui8Bits & IP) == IP) == true) {
                    if(Ban->ui32IpHash == nxtBan->ui32IpHash) {
                        if(((nxtBan->ui8Bits & FULL) == FULL) == false) {
                            // PPK ... same ban and old is only temp, delete old
                            Rem(nxtBan);
                            delete nxtBan;
						} else {
                            if(((Ban->ui8Bits & FULL) == FULL) == true) {
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
	BanItem *nxtBan = FindIP(Ban->ui32IpHash, acc_time);

    while(nxtBan != NULL) {
        BanItem *curBan = nxtBan;
        nxtBan = curBan->hashiptablenext;

        if(((curBan->ui8Bits & NICK) == NICK) == true) {
            continue;
        }
        
        if(((curBan->ui8Bits & FULL) == FULL) == true && ((Ban->ui8Bits & FULL) == FULL) == false) {
            continue;
        }

        Rem(curBan);
        delete curBan;
	}
        
    if(sReason != NULL) {
        size_t iReasonLen = strlen(sReason);
#ifdef _WIN32
        Ban->sReason = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iReasonLen > 255 ? 256 : iReasonLen+1);
#else
		Ban->sReason = (char *) malloc(iReasonLen > 255 ? 256 : iReasonLen+1);
#endif
        if(Ban->sReason == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iReasonLen > 255 ? 256 : iReasonLen+1))+
				" bytes of memory for sReason in hashBanMan::Ban!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
            AppendSpecialLog(sDbgstr);
            delete Ban;

            return;
        }

        if(iReasonLen > 255) {
            memcpy(Ban->sReason, sReason, 252);
			Ban->sReason[254] = '.';
			Ban->sReason[253] = '.';
            Ban->sReason[252] = '.';
            iReasonLen = 255;
        } else {
            memcpy(Ban->sReason, sReason, iReasonLen);
        }
        Ban->sReason[iReasonLen] = '\0';
    }

    if(sBy != NULL) {
        size_t iByLen = strlen(sBy);
        if(iByLen > 63) {
            iByLen = 63;
        }
#ifdef _WIN32
        Ban->sBy = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iByLen+1);
#else
		Ban->sBy = (char *) malloc(iByLen+1);
#endif
        if(Ban->sBy == NULL) {
            string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iByLen+1))+
            	" bytes of memory for sBy in hashBanMan::Ban!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
            AppendSpecialLog(sDbgstr);
            delete Ban;

			return;
        }   
        memcpy(Ban->sBy, sBy, iByLen);
        Ban->sBy[iByLen] = '\0';
    }

	Add(Ban);
	Save();
}
//---------------------------------------------------------------------------

char hashBanMan::BanIp(User * u, char * sIp, char * sReason, char * sBy, const bool &bFull) {
    BanItem *Ban = new BanItem();
    if(Ban == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate Ban in hashBanMan::BanIp!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
    	AppendSpecialLog(sDbgstr);
    	return 1;
    }

    Ban->ui8Bits |= PERM;

    if(u != NULL) {
        strcpy(Ban->sIp, u->IP);
        Ban->ui32IpHash = u->ui32IpHash;
    } else {
        uint32_t a, b, c, d;
        if(sIp != NULL && GetIpParts(sIp, strlen(sIp), a, b, c, d) == true) {
            strcpy(Ban->sIp, sIp);
            Ban->ui32IpHash = 16777216 * a + 65536 * b + 256 * c + d;
        } else {
			delete Ban;

            return 1;
        }
    }

    Ban->ui8Bits |= IP;
    
    if(bFull == true)
        Ban->ui8Bits |= FULL;
    
    time_t acc_time;
    time(&acc_time);

	BanItem *nxtBan = FindIP(Ban->ui32IpHash, acc_time);
        
    // PPK ... don't add ban if is already here perm (full) ban for same ip
    while(nxtBan != NULL) {
        BanItem *curBan = nxtBan;
        nxtBan = curBan->hashiptablenext;
        
        if(((curBan->ui8Bits & TEMP) == TEMP) == true) {
            if(((curBan->ui8Bits & FULL) == FULL) == false || ((Ban->ui8Bits & FULL) == FULL) == true) {
                if(((curBan->ui8Bits & NICK) == NICK) == false) {
                    Rem(curBan);
                    delete curBan;
				}
                continue;
            }
            continue;
        }

        if(((curBan->ui8Bits & FULL) == FULL) == false && ((Ban->ui8Bits & FULL) == FULL) == true) {
            if(((curBan->ui8Bits & NICK) == NICK) == false) {
                Rem(curBan);
                delete curBan;
			}
            continue;
        }

        delete Ban;

        return 2;
    }

    if(sReason != NULL) {
        size_t iReasonLen = strlen(sReason);
#ifdef _WIN32
        Ban->sReason = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iReasonLen > 255 ? 256 : iReasonLen+1);
#else
		Ban->sReason = (char *) malloc(iReasonLen > 255 ? 256 : iReasonLen+1);
#endif
        if(Ban->sReason == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iReasonLen > 255 ? 256 : iReasonLen+1))+
				" bytes of memory for sReason in hashBanMan::BanIp!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            delete Ban;

            return 0;
        }

        if(iReasonLen > 255) {
            memcpy(Ban->sReason, sReason, 252);
			Ban->sReason[254] = '.';
			Ban->sReason[253] = '.';
            Ban->sReason[252] = '.';
            iReasonLen = 255;
        } else {
            memcpy(Ban->sReason, sReason, iReasonLen);
        }
        Ban->sReason[iReasonLen] = '\0';
    }

    if(sBy != NULL) {
        size_t iByLen = strlen(sBy);
        if(iByLen > 63) {
            iByLen = 63;
        }
#ifdef _WIN32
        Ban->sBy = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iByLen+1);
#else
		Ban->sBy = (char *) malloc(iByLen+1);
#endif
        if(Ban->sBy == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iByLen+1))+
				" bytes of memory for sBy in hashBanMan::BanIp!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            delete Ban;

            return 0;
        }   
        memcpy(Ban->sBy, sBy, iByLen);
        Ban->sBy[iByLen] = '\0';
    }

    Add(Ban);
	Save();
    return 0;
}
//---------------------------------------------------------------------------

bool hashBanMan::NickBan(User * u, char * sNick, char * sReason, char * sBy) {
    BanItem *Ban = new BanItem();
    if(Ban == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate Ban in hashBanMan::NickBan!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
    	return false;
    }
    Ban->ui8Bits |= PERM;

    if(u == NULL) {
        // PPK ... this never happen, but to be sure ;)
        if(sNick == NULL) {
            delete Ban;

            return false;
        }

        // PPK ... bad script ban check
        if(sNick[0] == '<') {
            delete Ban;

            return false;
        }
        
        size_t iNickLen = strlen(sNick);
#ifdef _WIN32
        Ban->sNick = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iNickLen+1);
#else
		Ban->sNick = (char *) malloc(iNickLen+1);
#endif
        if(Ban->sNick == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iNickLen+1))+
				" bytes of memory for sNick in hashBanMan::NickBan!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            delete Ban;

            return false;
        }   
        memcpy(Ban->sNick, sNick, iNickLen);
        Ban->sNick[iNickLen] = '\0';
        Ban->ui32NickHash = HashNick(sNick, strlen(sNick));
    } else {
        // PPK ... bad script ban check
        if(u->Nick[0] == '<') {
            delete Ban;

            return false;
        }

#ifdef _WIN32
        Ban->sNick = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, u->NickLen+1);
#else
		Ban->sNick = (char *) malloc(u->NickLen+1);
#endif
        if(Ban->sNick == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string(u->NickLen+1)+
				" bytes of memory for sNick1 in hashBanMan::NickBan!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            delete Ban;

            return false;
        }   
        memcpy(Ban->sNick, u->Nick, u->NickLen);
        Ban->sNick[u->NickLen] = '\0';
        Ban->ui32NickHash = u->ui32NickHash;

        strcpy(Ban->sIp, u->IP);
        Ban->ui32IpHash = u->ui32IpHash;
    }

    Ban->ui8Bits |= NICK;

    time_t acc_time;
    time(&acc_time);

	BanItem *nxtBan = FindNick(Ban->ui32NickHash, acc_time, Ban->sNick);
    
    // PPK ... not allow same nickbans !
    if(nxtBan != NULL) {
        if(((nxtBan->ui8Bits & PERM) == PERM) == true) {
            delete Ban;

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
        size_t iReasonLen = strlen(sReason);
#ifdef _WIN32
        Ban->sReason = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iReasonLen > 255 ? 256 : iReasonLen+1);
#else
		Ban->sReason = (char *) malloc(iReasonLen > 255 ? 256 : iReasonLen+1);
#endif
        if(Ban->sReason == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iReasonLen > 255 ? 256 : iReasonLen+1))+
				" bytes of memory for sReason in hashBanMan::NickBan!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            delete Ban;

            return false;
        }   

        if(iReasonLen > 255) {
            memcpy(Ban->sReason, sReason, 252);
			Ban->sReason[254] = '.';
			Ban->sReason[253] = '.';
            Ban->sReason[252] = '.';
            iReasonLen = 255;
        } else {
            memcpy(Ban->sReason, sReason, iReasonLen);
        }
        Ban->sReason[iReasonLen] = '\0';
    }

    if(sBy != NULL) {
        size_t iByLen = strlen(sBy);
        if(iByLen > 63) {
            iByLen = 63;
        }
#ifdef _WIN32
        Ban->sBy = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iByLen+1);
#else
		Ban->sBy = (char *) malloc(iByLen+1);
#endif
        if(Ban->sBy == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iByLen+1))+
				" bytes of memory for sBy in hashBanMan::NickBan!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            delete Ban;

            return false;
        }   
        memcpy(Ban->sBy, sBy, iByLen);
        Ban->sBy[iByLen] = '\0';
    }

    Add(Ban);
	Save();
    return true;
}
//---------------------------------------------------------------------------

void hashBanMan::TempBan(User * u, const char * sReason, char * sBy, const uint32_t &minutes, const time_t &expiretime, const bool &bFull) {
    time_t acc_time;
    time(&acc_time);
    
    BanItem *Ban = new BanItem();
    if(Ban == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate Ban in hashBanMan::TempBan!";
#ifdef _WIN32
    	sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
    	AppendSpecialLog(sDbgstr);
    	return;
    }
    Ban->ui8Bits |= TEMP;

    strcpy(Ban->sIp, u->IP);
    Ban->ui32IpHash = u->ui32IpHash;
    Ban->ui8Bits |= IP;
    
    if(bFull == true)
        Ban->ui8Bits |= FULL;

    if(expiretime > 0) {
        Ban->tempbanexpire = expiretime;
    } else {
        if(minutes > 0) {
            Ban->tempbanexpire = acc_time+(minutes*60);
        } else {
    	    Ban->tempbanexpire = acc_time+(SettingManager->iShorts[SETSHORT_DEFAULT_TEMP_BAN_TIME]*60);
        }
    }
    
    // PPK ... check for <unknown> nick -> bad ban from script
    if(u->Nick[0] != '<') {
        size_t iNickLen = strlen(u->Nick);
#ifdef _WIN32
        Ban->sNick = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iNickLen+1);
#else
		Ban->sNick = (char *) malloc(iNickLen+1);
#endif
        if(Ban->sNick == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iNickLen+1))+
				" bytes of memory for sNick in hashBanMan::TempBan!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
            AppendSpecialLog(sDbgstr);
            delete Ban;

            return;
        }   
        memcpy(Ban->sNick, u->Nick, iNickLen);
        Ban->sNick[iNickLen] = '\0';
        Ban->ui32NickHash = u->ui32NickHash;
        Ban->ui8Bits |= NICK;

        // PPK ... not allow same nickbans ! i don't want this check here, but lame scripter find way to ban same nick multiple times :(
		BanItem *nxtBan = FindNick(Ban->ui32NickHash, acc_time, Ban->sNick);

        if(nxtBan != NULL) {
            if(((nxtBan->ui8Bits & PERM) == PERM) == true) {
                if(((nxtBan->ui8Bits & IP) == IP) == true) {
                    if(Ban->ui32IpHash == nxtBan->ui32IpHash) {
                        if(((Ban->ui8Bits & FULL) == FULL) == false) {
                            // PPK ... same ban and old is perm, delete new
                            delete Ban;

                            return;
                        } else {
                            if(((nxtBan->ui8Bits & FULL) == FULL) == true) {
                                // PPK ... same ban and old is full perm, delete new
                                delete Ban;

                                return;
                            } else {
                                // PPK ... same ban and only new is full, set new to ipban only
                                Ban->ui8Bits &= ~NICK;
                            }
                        }
                    }
                } else {
                    // PPK ... perm ban to same nick already exist, set new to ipban only
                    Ban->ui8Bits &= ~NICK;
                }
            } else {
                if(nxtBan->tempbanexpire < Ban->tempbanexpire) {
                    if(((nxtBan->ui8Bits & IP) == IP) == true) {
                        if(Ban->ui32IpHash == nxtBan->ui32IpHash) {
                            if(((nxtBan->ui8Bits & FULL) == FULL) == false) {
                                // PPK ... same bans, but old with lower expiration -> delete old
                                Rem(nxtBan);
                                delete nxtBan;
							} else {
                                if(((Ban->ui8Bits & FULL) == FULL) == false) {
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
                        if(Ban->ui32IpHash == nxtBan->ui32IpHash) {
                            if(((Ban->ui8Bits & FULL) == FULL) == false) {
                                // PPK ... same bans, but new with lower expiration -> delete new
                                delete Ban;

								return;
                            } else {
                                if(((nxtBan->ui8Bits & FULL) == FULL) == false) {
                                    // PPK ... new ban with lower expiration is full ban, set new to ipban only
                                    Ban->ui8Bits &= ~NICK;
                                } else {
                                    // PPK ... same bans, new have lower expiration -> delete new
                                    delete Ban;

                                    return;
                                }
                            }
                        } else {
                            // PPK ... set new ban to ipban only
                            Ban->ui8Bits &= ~NICK;
                        }
                    } else {
                        // PPK ... old ban is only nickban with higher bantime, set new to ipban only
                        Ban->ui8Bits &= ~NICK;
                    }
                }
            }
        }
    }

    // PPK ... clear bans with lower timeban and same ip without nickban and fullban if new ban is fullban
	BanItem *nxtBan = FindIP(Ban->ui32IpHash, acc_time);

    while(nxtBan != NULL) {
        BanItem *curBan = nxtBan;
        nxtBan = curBan->hashiptablenext;

        if(((curBan->ui8Bits & PERM) == PERM) == true) {
            continue;
        }
        
        if(((curBan->ui8Bits & NICK) == NICK) == true) {
            continue;
        }

        if(((curBan->ui8Bits & FULL) == FULL) == true && ((Ban->ui8Bits & FULL) == FULL) == false) {
            continue;
        }

        if(curBan->tempbanexpire > Ban->tempbanexpire) {
            continue;
        }
        
        Rem(curBan);
        delete curBan;
	}
    
    if(sReason != NULL) {
        size_t iReasonLen = strlen(sReason);
#ifdef _WIN32
        Ban->sReason = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iReasonLen > 255 ? 256 : iReasonLen+1);
#else
		Ban->sReason = (char *) malloc(iReasonLen > 255 ? 256 : iReasonLen+1);
#endif
        if(Ban->sReason == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iReasonLen > 255 ? 256 : iReasonLen+1))+
				" bytes of memory for sReason in hashBanMan::TempBan!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            delete Ban;

            return;
        }   

        if(iReasonLen > 255) {
            memcpy(Ban->sReason, sReason, 252);
			Ban->sReason[254] = '.';
			Ban->sReason[253] = '.';
            Ban->sReason[252] = '.';
            iReasonLen = 255;
        } else {
            memcpy(Ban->sReason, sReason, iReasonLen);
        }
        Ban->sReason[iReasonLen] = '\0';
    }

    if(sBy != NULL) {
        size_t iByLen = strlen(sBy);
        if(iByLen > 63) {
            iByLen = 63;
        }
#ifdef _WIN32
        Ban->sBy = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iByLen+1);
#else
		Ban->sBy = (char *) malloc(iByLen+1);
#endif
        if(Ban->sBy == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iByLen+1))+
				" bytes of memory for sBy in hashBanMan::TempBan!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            delete Ban;

            return;
        }   
        memcpy(Ban->sBy, sBy, iByLen);
        Ban->sBy[iByLen] = '\0';
    }

    Add(Ban);
	Save();
}
//---------------------------------------------------------------------------

char hashBanMan::TempBanIp(User * u, char * sIp, char * sReason, char * sBy, const uint32_t &minutes, const time_t &expiretime, const bool &bFull) {
    BanItem *Ban = new BanItem();
    if(Ban == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate Ban in hashBanMan::TempBanIp!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
    	return 1;
    }
    Ban->ui8Bits |= TEMP;

    if(u != NULL) {
        strcpy(Ban->sIp, u->IP);
        Ban->ui32IpHash = u->ui32IpHash;
    } else {
        uint32_t a, b, c, d;
        if(sIp != NULL && GetIpParts(sIp, strlen(sIp), a, b, c, d) == true) {
            strcpy(Ban->sIp, sIp);
            Ban->ui32IpHash = 16777216 * a + 65536 * b + 256 * c + d;
        } else {
            delete Ban;

            return 1;
        }
    }

    Ban->ui8Bits |= IP;
    
    if(bFull == true)
        Ban->ui8Bits |= FULL;

    time_t acc_time;
    time(&acc_time);

    if(expiretime > 0) {
        Ban->tempbanexpire = expiretime;
    } else {
        if(minutes == 0) {
    	    Ban->tempbanexpire = acc_time+(SettingManager->iShorts[SETSHORT_DEFAULT_TEMP_BAN_TIME]*60);
        } else {
            Ban->tempbanexpire = acc_time+(minutes*60);
        }
    }
    
	BanItem *nxtBan = FindIP(Ban->ui32IpHash, acc_time);

    // PPK ... don't add ban if is already here perm (full) ban or longer temp ban for same ip
    while(nxtBan != NULL) {
        BanItem *curBan = nxtBan;
        nxtBan = curBan->hashiptablenext;

        if(((curBan->ui8Bits & TEMP) == TEMP) == true && curBan->tempbanexpire < Ban->tempbanexpire) {
            if(((curBan->ui8Bits & FULL) == FULL) == false || ((Ban->ui8Bits & FULL) == FULL) == true) {
                if(((curBan->ui8Bits & NICK) == NICK) == false) {
                    Rem(curBan);
                    delete curBan;
				}
                continue;
            }
            continue;
        }

        if(((curBan->ui8Bits & FULL) == FULL) == false && ((Ban->ui8Bits & FULL) == FULL) == true) continue;

        delete Ban;

        return 2;
    }
    
    if(sReason != NULL) {
        size_t iReasonLen = strlen(sReason);
#ifdef _WIN32
        Ban->sReason = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iReasonLen > 255 ? 256 : iReasonLen+1);
#else
		Ban->sReason = (char *) malloc(iReasonLen > 255 ? 256 : iReasonLen+1);
#endif
        if(Ban->sReason == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iReasonLen > 255 ? 256 : iReasonLen+1))+
				" bytes of memory for sReason in hashBanMan::TempBanIp!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            delete Ban;

            return 0;
        }

        if(iReasonLen > 255) {
            memcpy(Ban->sReason, sReason, 252);
			Ban->sReason[254] = '.';
			Ban->sReason[253] = '.';
            Ban->sReason[252] = '.';
            iReasonLen = 255;
        } else {
            memcpy(Ban->sReason, sReason, iReasonLen);
        }
        Ban->sReason[iReasonLen] = '\0';
    }

    if(sBy != NULL) {
        size_t iByLen = strlen(sBy);
        if(iByLen > 63) {
            iByLen = 63;
        }
#ifdef _WIN32
        Ban->sBy = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iByLen+1);
#else
		Ban->sBy = (char *) malloc(iByLen+1);
#endif
        if(Ban->sBy == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iByLen+1))+
				" bytes of memory for sBy in hashBanMan::TempBanIp!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            delete Ban;

            return 0;
        }   
        memcpy(Ban->sBy, sBy, iByLen);
        Ban->sBy[iByLen] = '\0';
    }

    Add(Ban);
	Save();
    return 0;
}
//---------------------------------------------------------------------------

bool hashBanMan::NickTempBan(User * u, char * sNick, char * sReason, char * sBy, const uint32_t &minutes, const time_t &expiretime) {
    BanItem *Ban = new BanItem();
    if(Ban == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate Ban in hashBanMan::NickTempBan!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
    	return false;
    }
    Ban->ui8Bits |= TEMP;

    if(u == NULL) {
        // PPK ... this never happen, but to be sure ;)
        if(sNick == NULL) {
            delete Ban;

            return false;
        }

        // PPK ... bad script ban check
        if(sNick[0] == '<') {
            delete Ban;

            return false;
        }
        
        size_t iNickLen = strlen(sNick);
#ifdef _WIN32
        Ban->sNick = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iNickLen+1);
#else
		Ban->sNick = (char *) malloc(iNickLen+1);
#endif
        if(Ban->sNick == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iNickLen+1))+
				" bytes of memory for sNick in hashBanMan::NickTempBan!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            delete Ban;

            return false;
        }   
        memcpy(Ban->sNick, sNick, iNickLen);
        Ban->sNick[iNickLen] = '\0';
        Ban->ui32NickHash = HashNick(sNick, strlen(sNick));
    } else {
        // PPK ... bad script ban check
        if(u->Nick[0] == '<') {
            delete Ban;

            return false;
        }

#ifdef _WIN32
        Ban->sNick = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, u->NickLen+1);
#else
		Ban->sNick = (char *) malloc(u->NickLen+1);
#endif
        if(Ban->sNick == NULL) {
            string sDbgstr = "[BUF] Cannot allocate "+string(u->NickLen+1)+
            	" bytes of memory for sNick1 in hashBanMan::NickTempBan!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
            AppendSpecialLog(sDbgstr);
            delete Ban;

            return false;
        }   
        memcpy(Ban->sNick, u->Nick, u->NickLen);
        Ban->sNick[u->NickLen] = '\0';
        Ban->ui32NickHash = u->ui32NickHash;

        strcpy(Ban->sIp, u->IP);
        Ban->ui32IpHash = u->ui32IpHash;
    }

    Ban->ui8Bits |= NICK;

    time_t acc_time;
    time(&acc_time);

    if(expiretime > 0) {
        Ban->tempbanexpire = expiretime;
    } else {
        if(minutes > 0) {
            Ban->tempbanexpire = acc_time+(minutes*60);
        } else {
    	    Ban->tempbanexpire = acc_time+(SettingManager->iShorts[SETSHORT_DEFAULT_TEMP_BAN_TIME]*60);
        }
    }
    
	BanItem *nxtBan = FindNick(Ban->ui32NickHash, acc_time, Ban->sNick);

    // PPK ... not allow same nickbans !
    if(nxtBan != NULL) {
        if(((nxtBan->ui8Bits & PERM) == PERM) == true) {
            delete Ban;

            return false;
        } else {
            if(nxtBan->tempbanexpire < Ban->tempbanexpire) {
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
                delete Ban;

                return false;
            }
        }
    }

    if(sReason != NULL) {
        size_t iReasonLen = strlen(sReason);
#ifdef _WIN32
        Ban->sReason = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iReasonLen > 255 ? 256 : iReasonLen+1);
#else
		Ban->sReason = (char *) malloc(iReasonLen > 255 ? 256 : iReasonLen+1);
#endif
        if(Ban->sReason == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iReasonLen > 255 ? 256 : iReasonLen+1))+
				" bytes of memory for sReason in hashBanMan::NickTempBan!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
            AppendSpecialLog(sDbgstr);
            delete Ban;

            return false;
        }   

        if(iReasonLen > 255) {
            memcpy(Ban->sReason, sReason, 252);
			Ban->sReason[254] = '.';
			Ban->sReason[253] = '.';
            Ban->sReason[252] = '.';
            iReasonLen = 255;
        } else {
            memcpy(Ban->sReason, sReason, iReasonLen);
        }
        Ban->sReason[iReasonLen] = '\0';
    }

    if(sBy != NULL) {
        size_t iByLen = strlen(sBy);
        if(iByLen > 63) {
            iByLen = 63;
        }
#ifdef _WIN32
        Ban->sBy = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iByLen+1);
#else
		Ban->sBy = (char *) malloc(iByLen+1);
#endif
        if(Ban->sBy == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iByLen+1))+
				" bytes of memory for sBy in hashBanMan::NickTempBan!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            delete Ban;

            return false;
        }   
        memcpy(Ban->sBy, sBy, iByLen);
        Ban->sBy[iByLen] = '\0';
    }

    Add(Ban);
	Save();
    return true;
}
//---------------------------------------------------------------------------

bool hashBanMan::Unban(char * sWhat) {
    uint32_t hash = HashNick(sWhat, strlen(sWhat));

    time_t acc_time;
    time(&acc_time);

	BanItem *Ban = FindNick(hash, acc_time, sWhat);

    if(Ban == NULL) {
		if(HashIP(sWhat, strlen(sWhat), hash) == true && (Ban = FindIP(hash, acc_time)) != NULL) {
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

bool hashBanMan::PermUnban(char * sWhat) {
    uint32_t hash = HashNick(sWhat, strlen(sWhat));

	BanItem *Ban = FindPermNick(hash, sWhat);

    if(Ban == NULL) {
		if(HashIP(sWhat, strlen(sWhat), hash) == true && (Ban = FindPermIP(hash)) != NULL) {
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

bool hashBanMan::TempUnban(char * sWhat) {
    uint32_t hash = HashNick(sWhat, strlen(sWhat));

    time_t acc_time;
    time(&acc_time);

    BanItem *Ban = FindTempNick(hash, acc_time, sWhat);

    if(Ban == NULL) {
        if(HashIP(sWhat, strlen(sWhat), hash) == true && (Ban = FindTempIP(hash, acc_time)) != NULL) {
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

void hashBanMan::RemoveAllIP(const uint32_t &hash) {
    uint16_t ui16dx = ((uint16_t *)&hash)[0];

    IpTableItem * next = iptable[ui16dx];

    while(next != NULL) {
        IpTableItem * cur = next;
        next = cur->next;

        if(cur->FirstBan->ui32IpHash == hash) {
			BanItem * nextBan = cur->FirstBan;

            while(nextBan != NULL) {
                BanItem * curBan = nextBan;
                nextBan = curBan->hashiptablenext;
                
				Rem(curBan);
                delete curBan;
            }

            return;
        }
    }
}
//---------------------------------------------------------------------------

void hashBanMan::RemovePermAllIP(const uint32_t &hash) {
    uint16_t ui16dx = ((uint16_t *)&hash)[0];

    IpTableItem * next = iptable[ui16dx];

    while(next != NULL) {
        IpTableItem * cur = next;
        next = cur->next;

        if(cur->FirstBan->ui32IpHash == hash) {
			BanItem * nextBan = cur->FirstBan;

            while(nextBan != NULL) {
                BanItem * curBan = nextBan;
                nextBan = curBan->hashiptablenext;

                if(((curBan->ui8Bits & hashBanMan::PERM) == hashBanMan::PERM) == true) {
					Rem(curBan);
                    delete curBan;
                }
            }

            return;
        }
    }
}
//---------------------------------------------------------------------------

void hashBanMan::RemoveTempAllIP(const uint32_t &hash) {
    uint16_t ui16dx = ((uint16_t *)&hash)[0];

    IpTableItem * next = iptable[ui16dx];

    while(next != NULL) {
        IpTableItem * cur = next;
        next = cur->next;

        if(cur->FirstBan->ui32IpHash == hash) {
			BanItem * nextBan = cur->FirstBan;

            while(nextBan != NULL) {
                BanItem * curBan = nextBan;
                nextBan = curBan->hashiptablenext;

                if(((curBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
					Rem(curBan);
                    delete curBan;
                }
            }

            return;
        }
    }
}
//---------------------------------------------------------------------------

bool hashBanMan::RangeBan(char * sIpFrom, const uint32_t &ui32FromIpHash, char * sIpTo, const uint32_t &ui32ToIpHash, 
    char * sReason, char * sBy, const bool &bFull) {
    RangeBanItem *RangeBan = new RangeBanItem();
    if(RangeBan == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate RangeBan in hashBanMan::RangeBan!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
    	return false;
    }
    RangeBan->ui8Bits |= PERM;

    strcpy(RangeBan->sIpFrom, sIpFrom);
    RangeBan->ui32FromIpHash = ui32FromIpHash;

    strcpy(RangeBan->sIpTo, sIpTo);
    RangeBan->ui32ToIpHash = ui32ToIpHash;

    if(bFull == true)
        RangeBan->ui8Bits |= FULL;

    RangeBanItem *nxtBan = RangeBanListS;

    // PPK ... don't add range ban if is already here same perm (full) range ban
    while(nxtBan != NULL) {
        RangeBanItem *curBan = nxtBan;
        nxtBan = curBan->next;

        if(curBan->ui32FromIpHash != RangeBan->ui32FromIpHash || curBan->ui32ToIpHash != RangeBan->ui32ToIpHash) {
            continue;
        }

        if(((curBan->ui8Bits & TEMP) == TEMP) == true) {
            if(((curBan->ui8Bits & FULL) == FULL) == false || ((RangeBan->ui8Bits & FULL) == FULL) == true) {
                RemRange(curBan);
                delete curBan;

                continue;
            }
        }
        
        if(((curBan->ui8Bits & FULL) == FULL) == false && ((RangeBan->ui8Bits & FULL) == FULL) == true) {
            RemRange(curBan);
            delete curBan;

            continue;
        }
        
        delete RangeBan;

        return false;
    }

    if(sReason != NULL) {
        size_t iReasonLen = strlen(sReason);
#ifdef _WIN32
        RangeBan->sReason = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iReasonLen > 255 ? 256 : iReasonLen+1);
#else
		RangeBan->sReason = (char *) malloc(iReasonLen > 255 ? 256 : iReasonLen+1);
#endif
        if(RangeBan->sReason == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iReasonLen > 255 ? 256 : iReasonLen+1))+
				" bytes of memory for sReason in hashBanMan::RangeBan!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            delete RangeBan;

            return false;
        }   

        if(iReasonLen > 255) {
            memcpy(RangeBan->sReason, sReason, 252);
            RangeBan->sReason[254] = '.';
            RangeBan->sReason[253] = '.';
            RangeBan->sReason[252] = '.';
            iReasonLen = 255;
        } else {
            memcpy(RangeBan->sReason, sReason, iReasonLen);
        }
        RangeBan->sReason[iReasonLen] = '\0';
    }

    if(sBy != NULL) {
        size_t iByLen = strlen(sBy);
        if(iByLen > 63) {
            iByLen = 63;
        }
#ifdef _WIN32
        RangeBan->sBy = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iByLen+1);
#else
		RangeBan->sBy = (char *) malloc(iByLen+1);
#endif
        if(RangeBan->sBy == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iByLen+1))+
				" bytes of memory for sBy in hashBanMan::RangeBan!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            delete RangeBan;

            return false;
        }   
        memcpy(RangeBan->sBy, sBy, iByLen);
        RangeBan->sBy[iByLen] = '\0';
    }

    AddRange(RangeBan);
	Save();
    return true;
}
//---------------------------------------------------------------------------

bool hashBanMan::RangeTempBan(char * sIpFrom, const uint32_t &ui32FromIpHash, char * sIpTo, const uint32_t &ui32ToIpHash, 
    char * sReason, char * sBy, const uint32_t &minutes, const time_t &expiretime, const bool &bFull) {
    RangeBanItem *RangeBan = new RangeBanItem();
    if(RangeBan == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate RangeBan in hashBanMan::RangeTempBan!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
    	return false;
    }
    RangeBan->ui8Bits |= TEMP;

    strcpy(RangeBan->sIpFrom, sIpFrom);
    RangeBan->ui32FromIpHash = ui32FromIpHash;

    strcpy(RangeBan->sIpTo, sIpTo);
    RangeBan->ui32ToIpHash = ui32ToIpHash;

    if(bFull == true)
        RangeBan->ui8Bits |= FULL;

    time_t acc_time;
    time(&acc_time);

    if(expiretime > 0) {
        RangeBan->tempbanexpire = expiretime;
    } else {
        if(minutes > 0) {
            RangeBan->tempbanexpire = acc_time+(minutes*60);
        } else {
    	    RangeBan->tempbanexpire = acc_time+(SettingManager->iShorts[SETSHORT_DEFAULT_TEMP_BAN_TIME]*60);
        }
    }
    
    RangeBanItem *nxtBan = RangeBanListS;

    // PPK ... don't add range ban if is already here same perm (full) range ban or longer temp ban for same range
    while(nxtBan != NULL) {
        RangeBanItem *curBan = nxtBan;
        nxtBan = curBan->next;

        if(curBan->ui32FromIpHash != RangeBan->ui32FromIpHash || curBan->ui32ToIpHash != RangeBan->ui32ToIpHash) {
            continue;
        }

        if(((curBan->ui8Bits & TEMP) == TEMP) == true && curBan->tempbanexpire < RangeBan->tempbanexpire) {
            if(((curBan->ui8Bits & FULL) == FULL) == false || ((RangeBan->ui8Bits & FULL) == FULL) == true) {
                RemRange(curBan);
                delete curBan;

                continue;
            }
            continue;
        }
        
        if(((curBan->ui8Bits & FULL) == FULL) == false && ((RangeBan->ui8Bits & FULL) == FULL) == true) continue;

        delete RangeBan;

        return false;
    }
    
    if(sReason != NULL) {
        size_t iReasonLen = strlen(sReason);
#ifdef _WIN32
        RangeBan->sReason = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iReasonLen > 255 ? 256 : iReasonLen+1);
#else
		RangeBan->sReason = (char *) malloc(iReasonLen > 255 ? 256 : iReasonLen+1);
#endif
        if(RangeBan->sReason == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iReasonLen > 255 ? 256 : iReasonLen+1))+
				" bytes of memory for sReason in hashBanMan::RangeTempBan!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            delete RangeBan;

            return false;
        }   

        if(iReasonLen > 255) {
            memcpy(RangeBan->sReason, sReason, 252);
            RangeBan->sReason[254] = '.';
            RangeBan->sReason[253] = '.';
            RangeBan->sReason[252] = '.';
            iReasonLen = 255;
        } else {
            memcpy(RangeBan->sReason, sReason, iReasonLen);
        }
        RangeBan->sReason[iReasonLen] = '\0';
    }

    if(sBy != NULL) {
        size_t iByLen = strlen(sBy);
        if(iByLen > 63) {
            iByLen = 63;
        }
#ifdef _WIN32
        RangeBan->sBy = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iByLen+1);
#else
		RangeBan->sBy = (char *) malloc(iByLen+1);
#endif
        if(RangeBan->sBy == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iByLen+1))+
				" bytes of memory for sBy in hashBanMan::RangeTempBan!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            delete RangeBan;

            return false;
        }   
        memcpy(RangeBan->sBy, sBy, iByLen);
        RangeBan->sBy[iByLen] = '\0';
    }

    AddRange(RangeBan);
	Save();
    return true;
}
//---------------------------------------------------------------------------

bool hashBanMan::RangeUnban(const uint32_t &ui32FromIpHash, const uint32_t &ui32ToIpHash) {
    RangeBanItem *next = RangeBanListS;

    while(next != NULL) {
        RangeBanItem *cur = next;
        next = cur->next;

        if(cur->ui32FromIpHash == ui32FromIpHash && cur->ui32ToIpHash == ui32ToIpHash) {
            RemRange(cur);
            delete cur;

            return true;
        }
    }

	Save();
    return false;
}
//---------------------------------------------------------------------------

bool hashBanMan::RangeUnban(const uint32_t &ui32FromIpHash, const uint32_t &ui32ToIpHash, unsigned char cType) {
    RangeBanItem *next = RangeBanListS;

    while(next != NULL) {
        RangeBanItem *cur = next;
        next = cur->next;

        if(((cur->ui8Bits & cType) == cType) == true && cur->ui32FromIpHash == ui32FromIpHash && cur->ui32ToIpHash == ui32ToIpHash) {
            RemRange(cur);
            delete cur;

            return true;
        }
    }

    Save();
    return false;
}
//---------------------------------------------------------------------------
