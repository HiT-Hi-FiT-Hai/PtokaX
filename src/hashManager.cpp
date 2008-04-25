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
#include "hashManager.h"
//---------------------------------------------------------------------------
#include "hashBanManager.h"
#include "hashRegManager.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
hashMan *hashManager = NULL;
//---------------------------------------------------------------------------

hashMan::hashMan() {
    for(unsigned int i = 0; i < 65536; i++) {
        nicktable[i] = NULL;
        iptable[i] = NULL;
    }

    //Memo("hashManager created");
}
//---------------------------------------------------------------------------

hashMan::~hashMan() {
    //Memo("hashManager destroyed");
    for(unsigned int i = 0; i < 65536; i++) {
        if(nicktable[i] != NULL) {
            delete nicktable[i];
        }

        if(iptable[i] != NULL) {
            if(iptable[i]->UserIPs != NULL) {
				IpTableUsers * next = iptable[i]->UserIPs;
        
                while(next != NULL) {
                    IpTableUsers * cur = next;
                    next = cur->next;
        
                    delete cur;
                }
			}

            if(iptable[i]->BanIPs != NULL) {
                IpTableBans * next = iptable[i]->BanIPs;
        
                while(next != NULL) {
                    IpTableBans * cur = next;
                    next = cur->next;
        
                    delete cur;
                }
            }

            delete iptable[i];
		}
    }
}
//---------------------------------------------------------------------------

void hashMan::AddIP(BanItem *Ban) {
    uint16_t ui16dx = ((uint16_t *)&Ban->ui32IpHash)[0];
    
    if(iptable[ui16dx] == NULL) {
		iptable[ui16dx] = new IpTableItem();

        if(iptable[ui16dx] == NULL) {
			string sDbgstr = "[BUF] Cannot allocate IpTableItem in hashMan::AddIP!";
			AppendSpecialLog(sDbgstr);
            exit(EXIT_FAILURE);
        }

        iptable[ui16dx]->BanIPs = new IpTableBans();

        if(iptable[ui16dx]->BanIPs == NULL) {
			string sDbgstr = "[BUF] Cannot allocate IpTableBans in hashMan::AddIP!";
			AppendSpecialLog(sDbgstr);
            exit(EXIT_FAILURE);
        }

        iptable[ui16dx]->UserIPs = NULL;

        iptable[ui16dx]->BanIPs->next = NULL;
        iptable[ui16dx]->BanIPs->prev = NULL;

        iptable[ui16dx]->BanIPs->Bans = Ban;

        return;
    } else if(iptable[ui16dx]->BanIPs == NULL) {
        iptable[ui16dx]->BanIPs = new IpTableBans();

        if(iptable[ui16dx]->BanIPs == NULL) {
			string sDbgstr = "[BUF] Cannot allocate IpTableBan1 in hashMan::AddIP!";
			AppendSpecialLog(sDbgstr);
            exit(EXIT_FAILURE);
        }

        iptable[ui16dx]->BanIPs->next = NULL;
        iptable[ui16dx]->BanIPs->prev = NULL;

		iptable[ui16dx]->BanIPs->Bans = Ban;

        return;
    }

    IpTableBans * next = iptable[ui16dx]->BanIPs;

    while(next != NULL) {
        IpTableBans * cur = next;
        next = cur->next;

		if(cur->Bans->ui32IpHash == Ban->ui32IpHash) {
			cur->Bans->hashiptableprev = Ban;
			Ban->hashiptablenext = cur->Bans;
            cur->Bans = Ban;
            return;
        }
    }

    IpTableBans * cur = new IpTableBans();

    if(cur == NULL) {
		string sDbgstr = "[BUF] Cannot allocate IpTableBans2 in hashMan::AddIP!";
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    cur->next = iptable[ui16dx]->BanIPs;
    cur->prev = NULL;

    cur->Bans = Ban;

    iptable[ui16dx]->BanIPs->prev = cur;
    iptable[ui16dx]->BanIPs = cur;
}
//---------------------------------------------------------------------------

void hashMan::RemoveIP(BanItem *Ban) {   
	uint16_t ui16dx = ((uint16_t *)&Ban->ui32IpHash)[0];

	if(Ban->hashiptableprev == NULL) {
        IpTableBans * next = iptable[ui16dx]->BanIPs;

        while(next != NULL) {
            IpTableBans * cur = next;
            next = cur->next;

			if(cur->Bans->ui32IpHash == Ban->ui32IpHash) {
				if(Ban->hashiptablenext == NULL) {
					if(cur->prev == NULL) {
						if(cur->next == NULL) {
                            if(iptable[ui16dx]->UserIPs == NULL) {
                                delete iptable[ui16dx];
                                iptable[ui16dx] = NULL;
                            } else {
                                iptable[ui16dx]->BanIPs = NULL;
                            }
						} else {
							cur->next->prev = NULL;
                            iptable[ui16dx]->BanIPs = cur->next;
                        }
					} else if(cur->next == NULL) {
						cur->prev->next = NULL;
					} else {
						cur->prev->next = cur->next;
                        cur->next->prev = cur->prev;
                    }

                    delete cur;
				} else {
					Ban->hashiptablenext->hashiptableprev = NULL;
                    cur->Bans = Ban->hashiptablenext;
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

void hashMan::RemoveIPAll(const uint32_t &hash) {
    uint16_t ui16dx = ((uint16_t *)&hash)[0];

    if(iptable[ui16dx] == NULL) {
		return;
    }

    IpTableBans * next = iptable[ui16dx]->BanIPs;

    while(next != NULL) {
        IpTableBans * cur = next;
        next = cur->next;

        if(cur->Bans->ui32IpHash == hash) {
            BanItem * nextBan = cur->Bans;

            while(nextBan != NULL) {
                BanItem * curBan = nextBan;
                nextBan = curBan->hashiptablenext;
                
				hashBanManager->RemBan(curBan);
                delete curBan;
            }

            return;
        }
    }
}
//---------------------------------------------------------------------------

void hashMan::RemoveIPPermAll(const uint32_t &hash) {
    uint16_t ui16dx = ((uint16_t *)&hash)[0];

    if(iptable[ui16dx] == NULL) {
		return;
    }

    IpTableBans * next = iptable[ui16dx]->BanIPs;

    while(next != NULL) {
        IpTableBans * cur = next;
        next = cur->next;

        if(cur->Bans->ui32IpHash == hash) {
            BanItem * nextBan = cur->Bans;

            while(nextBan != NULL) {
                BanItem * curBan = nextBan;
                nextBan = curBan->hashiptablenext;

                if(((curBan->ui8Bits & hashBanMan::PERM) == hashBanMan::PERM) == true) {
					hashBanManager->RemBan(curBan);
                    delete curBan;
                }
            }

            return;
        }
    }
}
//---------------------------------------------------------------------------

void hashMan::RemoveIPTempAll(const uint32_t &hash) {
    uint16_t ui16dx = ((uint16_t *)&hash)[0];

    if(iptable[ui16dx] == NULL) {
        return;
    }

    IpTableBans * next = iptable[ui16dx]->BanIPs;

    while(next != NULL) {
        IpTableBans * cur = next;
        next = cur->next;

        if(cur->Bans->ui32IpHash == hash) {
            BanItem * nextBan = cur->Bans;

            while(nextBan != NULL) {
                BanItem * curBan = nextBan;
                nextBan = curBan->hashiptablenext;

                if(((curBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
					hashBanManager->RemBan(curBan);
                    delete curBan;
                }
            }

            return;
        }
    }
}
//---------------------------------------------------------------------------

void hashMan::AddNick(BanItem *Ban) {
    uint16_t ui16dx = ((uint16_t *)&Ban->ui32NickHash)[0];

    if(nicktable[ui16dx] != NULL) {
        if(nicktable[ui16dx]->Bans != NULL) {
            nicktable[ui16dx]->Bans->hashnicktableprev = Ban;
            Ban->hashnicktablenext = nicktable[ui16dx]->Bans;
        }
    } else {
        nicktable[ui16dx] = new NickTableItem();

        if(nicktable[ui16dx] == NULL) {
			string sDbgstr = "[BUF] Cannot allocate NickTableItem in hashMan::Add!";
			AppendSpecialLog(sDbgstr);
            exit(EXIT_FAILURE);
        }

        nicktable[ui16dx]->Regs = NULL;
        nicktable[ui16dx]->Users = NULL;
    }

    nicktable[ui16dx]->Bans = Ban;
}
//---------------------------------------------------------------------------

void hashMan::RemoveNick(BanItem *Ban) {
    if(Ban->hashnicktableprev == NULL) {
        if(Ban->hashnicktablenext == NULL) {
            uint16_t ui16dx = ((uint16_t *)&Ban->ui32NickHash)[0];

            if(nicktable[ui16dx]->Regs == NULL && nicktable[ui16dx]->Users == NULL) {
                delete nicktable[ui16dx];
                nicktable[ui16dx] = NULL;
            } else {
                nicktable[ui16dx]->Bans = NULL;
            }
        } else {
            Ban->hashnicktablenext->hashnicktableprev = NULL;
            nicktable[((uint16_t *)&Ban->ui32NickHash)[0]]->Bans = Ban->hashnicktablenext;
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

void hashMan::Add(RegUser * Reg) {
    uint16_t ui16dx = ((uint16_t *)&Reg->ui32Hash)[0];

    if(nicktable[ui16dx] != NULL) {
        if(nicktable[ui16dx]->Regs != NULL) {
            nicktable[ui16dx]->Regs->hashtableprev = Reg;
            Reg->hashtablenext = nicktable[ui16dx]->Regs;
        }
    } else {
        nicktable[ui16dx] = new NickTableItem();

        if(nicktable[ui16dx] == NULL) {
			string sDbgstr = "[BUF] Cannot allocate NickTableItem in hashMan::Add!";
			AppendSpecialLog(sDbgstr);
            exit(EXIT_FAILURE);
        }

        nicktable[ui16dx]->Bans = NULL;
        nicktable[ui16dx]->Users = NULL;
    }
    
    nicktable[ui16dx]->Regs = Reg;
}
//---------------------------------------------------------------------------

void hashMan::Remove(RegUser * Reg) {
    if(Reg->hashtableprev == NULL) {
        if(Reg->hashtablenext == NULL) {
            uint16_t ui16dx = ((uint16_t *)&Reg->ui32Hash)[0];

            if(nicktable[ui16dx]->Bans == NULL && nicktable[ui16dx]->Users == NULL) {
                delete nicktable[ui16dx];
                nicktable[ui16dx] = NULL;
            } else {
                nicktable[ui16dx]->Regs = NULL;
            }
        } else {
            Reg->hashtablenext->hashtableprev = NULL;
			nicktable[((uint16_t *)&Reg->ui32Hash)[0]]->Regs = Reg->hashtablenext;
        }
    } else if(Reg->hashtablenext == NULL) {
        Reg->hashtableprev->hashtablenext = NULL;
    } else {
        Reg->hashtableprev->hashtablenext = Reg->hashtablenext;
        Reg->hashtablenext->hashtableprev = Reg->hashtableprev;
    }

	Reg->hashtableprev = NULL;
    Reg->hashtablenext = NULL;
}
//---------------------------------------------------------------------------

void hashMan::Add(User * u) {
    uint16_t ui16dx = ((uint16_t *)&u->ui32NickHash)[0];
    
    if(nicktable[ui16dx] != NULL) {
        if(nicktable[ui16dx]->Users != NULL) {
            nicktable[ui16dx]->Users->hashtableprev = u;
            u->hashtablenext = nicktable[ui16dx]->Users;
        }
    } else {
        nicktable[ui16dx] = new NickTableItem();

        if(nicktable[ui16dx] == NULL) {
			string sDbgstr = "[BUF] Cannot allocate NickTableItem in hashMan::Add!";
			AppendSpecialLog(sDbgstr);
            exit(EXIT_FAILURE);
        }

        nicktable[ui16dx]->Bans = NULL;
        nicktable[ui16dx]->Regs = NULL;
    }

    nicktable[ui16dx]->Users = u;

    ui16dx = ((uint16_t *)&u->ui32IpHash)[0];
    
    if(iptable[ui16dx] == NULL) {
        iptable[ui16dx] = new IpTableItem();

        if(iptable[ui16dx] == NULL) {
			string sDbgstr = "[BUF] Cannot allocate IpTableItem in hashMan::Add!";
			AppendSpecialLog(sDbgstr);
            exit(EXIT_FAILURE);
        }

        iptable[ui16dx]->UserIPs = new IpTableUsers();

        if(iptable[ui16dx]->UserIPs == NULL) {
			string sDbgstr = "[BUF] Cannot allocate IpTableUsers in hashMan::Add!";
			AppendSpecialLog(sDbgstr);
            exit(EXIT_FAILURE);
        }

        iptable[ui16dx]->BanIPs = NULL;

        iptable[ui16dx]->UserIPs->next = NULL;
        iptable[ui16dx]->UserIPs->prev = NULL;

        iptable[ui16dx]->UserIPs->Users = u;
        iptable[ui16dx]->UserIPs->ui32Count = 1;

        return;
    } else if(iptable[ui16dx]->UserIPs == NULL) {
        iptable[ui16dx]->UserIPs = new IpTableUsers();

        if(iptable[ui16dx]->UserIPs == NULL) {
			string sDbgstr = "[BUF] Cannot allocate IpTableUsers1 in hashMan::Add!";
			AppendSpecialLog(sDbgstr);
            exit(EXIT_FAILURE);
        }

        iptable[ui16dx]->UserIPs->next = NULL;
        iptable[ui16dx]->UserIPs->prev = NULL;

        iptable[ui16dx]->UserIPs->Users = u;
        iptable[ui16dx]->UserIPs->ui32Count = 1;

        return;
    }

    IpTableUsers * next = iptable[ui16dx]->UserIPs;

    while(next != NULL) {
        IpTableUsers * cur = next;
        next = cur->next;

        if(cur->Users->ui32IpHash == u->ui32IpHash) {
            cur->Users->hashiptableprev = u;
            u->hashiptablenext = cur->Users;
            cur->Users = u;
            cur->ui32Count++;
            return;
        }
    }

    IpTableUsers * cur = new IpTableUsers();

    if(cur == NULL) {
		string sDbgstr = "[BUF] Cannot allocate IpTableUsers2 in hashMan::Add!";
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    cur->next = iptable[ui16dx]->UserIPs;
    cur->prev = NULL;

    cur->Users = u;
    cur->ui32Count = 1;

    iptable[ui16dx]->UserIPs->prev = cur;
    iptable[ui16dx]->UserIPs = cur;
}
//---------------------------------------------------------------------------

void hashMan::Remove(User * u) {
    if(u->hashtableprev == NULL) {
        if(u->hashtablenext == NULL) {
            uint16_t ui16dx = ((uint16_t *)&u->ui32NickHash)[0];

            if(nicktable[ui16dx]->Bans == NULL && nicktable[ui16dx]->Regs == NULL) {
                delete nicktable[ui16dx];
                nicktable[ui16dx] = NULL;
            } else {
                nicktable[ui16dx]->Users = NULL;
            }
        } else {
            u->hashtablenext->hashtableprev = NULL;
            nicktable[((uint16_t *)&u->ui32NickHash)[0]]->Users = u->hashtablenext;
        }
    } else if(u->hashtablenext == NULL) {
        u->hashtableprev->hashtablenext = NULL;
    } else {
        u->hashtableprev->hashtablenext = u->hashtablenext;
        u->hashtablenext->hashtableprev = u->hashtableprev;
    }

    u->hashtableprev = NULL;
    u->hashtablenext = NULL;

	uint16_t ui16dx = ((uint16_t *)&u->ui32IpHash)[0];

	if(u->hashiptableprev == NULL) {
        IpTableUsers * next = iptable[ui16dx]->UserIPs;
    
        while(next != NULL) {
            IpTableUsers * cur = next;
            next = cur->next;
    
            if(cur->Users->ui32IpHash == u->ui32IpHash) {
                cur->ui32Count--;

                if(u->hashiptablenext == NULL) {
                    if(cur->prev == NULL) {
                        if(cur->next == NULL) {
                            if(iptable[ui16dx]->BanIPs == NULL) {
                                delete iptable[ui16dx];
                                iptable[ui16dx] = NULL;
                            } else {
                                iptable[ui16dx]->UserIPs = NULL;
                            }
                        } else {
                            cur->next->prev = NULL;
                            iptable[ui16dx]->UserIPs = cur->next;
                        }
                    } else if(cur->next == NULL) {
                        cur->prev->next = NULL;
                    } else {
                        cur->prev->next = cur->next;
                        cur->next->prev = cur->prev;
                    }

                    delete cur;
                } else {
                    u->hashiptablenext->hashiptableprev = NULL;
                    cur->Users = u->hashiptablenext;
                }

                u->hashiptableprev = NULL;
                u->hashiptablenext = NULL;
                return;
            }
        }
    } else if(u->hashiptablenext == NULL) {
        u->hashiptableprev->hashiptablenext = NULL;
    } else {
        u->hashiptableprev->hashiptablenext = u->hashiptablenext;
        u->hashiptablenext->hashiptableprev = u->hashiptableprev;
    }

    u->hashiptableprev = NULL;
    u->hashiptablenext = NULL;

    IpTableUsers * next = iptable[ui16dx]->UserIPs;

    while(next != NULL) {
        IpTableUsers * cur = next;
        next = cur->next;

        if(cur->Users->ui32IpHash == u->ui32IpHash) {
            cur->ui32Count--;
            return;
        }
    }
}
//---------------------------------------------------------------------------

BanItem* hashMan::FindBanIP(User* u) {
    uint16_t ui16dx = ((uint16_t *)&u->ui32IpHash)[0];

    if(iptable[ui16dx] == NULL) {
        return NULL;
    }

    IpTableBans * next = iptable[ui16dx]->BanIPs;

    time_t acc_time;
    time(&acc_time);

    while(next != NULL) {
        IpTableBans * cur = next;
        next = cur->next;

        if(cur->Bans->ui32IpHash == u->ui32IpHash) {
            BanItem * nextBan = cur->Bans;

            while(nextBan != NULL) {
                BanItem * curBan = nextBan;
                nextBan = curBan->hashiptablenext;
                
                // PPK ... check if temban expired
				if(((curBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
                    if(acc_time >= curBan->tempbanexpire) {
						hashBanManager->RemBan(curBan);
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

BanItem* hashMan::FindBanIP(const uint32_t &hash, const time_t &acc_time) {
    uint16_t ui16dx = ((uint16_t *)&hash)[0];

    if(iptable[ui16dx] == NULL) {
        return NULL;
    }

    IpTableBans * next = iptable[ui16dx]->BanIPs;

    while(next != NULL) {
        IpTableBans * cur = next;
        next = cur->next;

        if(cur->Bans->ui32IpHash == hash) {
            BanItem * nextBan = cur->Bans;

            while(nextBan != NULL) {
                BanItem * curBan = nextBan;
                nextBan = curBan->hashiptablenext;

                // PPK ... check if temban expired
				if(((curBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
                    if(acc_time >= curBan->tempbanexpire) {
						hashBanManager->RemBan(curBan);
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

BanItem* hashMan::FindBanIPFull(const uint32_t &hash) {
    time_t acc_time; time(&acc_time);

    return FindBanIPFull(hash, acc_time);
}
//---------------------------------------------------------------------------

BanItem* hashMan::FindBanIPFull(const uint32_t &hash, const time_t &acc_time) {
    uint16_t ui16dx = ((uint16_t *)&hash)[0];

    if(iptable[ui16dx] == NULL) {
        return NULL;
    }

    IpTableBans * next = iptable[ui16dx]->BanIPs;

    BanItem *fnd = NULL;

    while(next != NULL) {
        IpTableBans * cur = next;
        next = cur->next;

		if(cur->Bans->ui32IpHash == hash) {
            BanItem * nextBan = cur->Bans;

            while(nextBan != NULL) {
                BanItem * curBan = nextBan;
                nextBan = curBan->hashiptablenext;
        
                // PPK ... check if temban expired
				if(((curBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
                    if(acc_time >= curBan->tempbanexpire) {
						hashBanManager->RemBan(curBan);
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

BanItem* hashMan::FindBanIPTemp(const uint32_t &hash, const time_t &acc_time) {
    uint16_t ui16dx = ((uint16_t *)&hash)[0];

    if(iptable[ui16dx] == NULL) {
        return NULL;
    }

    IpTableBans * next = iptable[ui16dx]->BanIPs;

    while(next != NULL) {
        IpTableBans * cur = next;
        next = cur->next;

        if(cur->Bans->ui32IpHash == hash) {
            BanItem * nextBan = cur->Bans;

            while(nextBan != NULL) {
                BanItem * curBan = nextBan;
                nextBan = curBan->hashiptablenext;
                
                // PPK ... check if temban expired
				if(((curBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
                    if(acc_time >= curBan->tempbanexpire) {
						hashBanManager->RemBan(curBan);
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

BanItem* hashMan::FindBanIPPerm(const uint32_t &hash) {
    uint16_t ui16dx = ((uint16_t *)&hash)[0];

    if(iptable[ui16dx] == NULL) {
        return NULL;
    }

    IpTableBans * next = iptable[ui16dx]->BanIPs;

    while(next != NULL) {
        IpTableBans * cur = next;
        next = cur->next;

        if(cur->Bans->ui32IpHash == hash) {
            BanItem * nextBan = cur->Bans;

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

BanItem* hashMan::FindBanNick(User* u) {
    uint16_t ui16dx = ((uint16_t *)&u->ui32NickHash)[0];

    if(nicktable[ui16dx] == NULL) {
        return NULL;
    }

    time_t acc_time;
    time(&acc_time);

    BanItem *next = nicktable[ui16dx]->Bans;

    while(next != NULL) {
        BanItem *cur = next;
        next = cur->hashnicktablenext;

        if(cur->ui32NickHash == u->ui32NickHash && strcasecmp(cur->sNick, u->Nick) == 0) {
            // PPK ... check if temban expired
			if(((cur->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
                if(acc_time >= cur->tempbanexpire) {
					hashBanManager->RemBan(cur);
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

BanItem* hashMan::FindBanNick(char * sNick, int iNickLen) {
    uint32_t hash = HashNick(sNick, iNickLen);
    time_t acc_time; time(&acc_time);

    return FindBanNick(hash, acc_time, sNick);
}
//---------------------------------------------------------------------------

BanItem* hashMan::FindBanNick(const uint32_t &ui32Hash, const time_t &acc_time, char * sNick) {
    uint16_t ui16dx = ((uint16_t *)&ui32Hash)[0];

    if(nicktable[ui16dx] == NULL) {
        return NULL;
    }

	BanItem *next = nicktable[ui16dx]->Bans;

    while(next != NULL) {
        BanItem *cur = next;
        next = cur->hashnicktablenext;

        if(cur->ui32NickHash == ui32Hash && strcasecmp(cur->sNick, sNick) == 0) {
            // PPK ... check if temban expired
			if(((cur->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
                if(acc_time >= cur->tempbanexpire) {
					hashBanManager->RemBan(cur);
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

BanItem* hashMan::FindBanNickTemp(char * sNick, int iNickLen) {
    uint32_t hash = HashNick(sNick, iNickLen);
    time_t acc_time; time(&acc_time);

    return FindBanNickTemp(hash, acc_time, sNick);
}
//---------------------------------------------------------------------------

BanItem* hashMan::FindBanNickTemp(const uint32_t &ui32Hash,  const time_t &acc_time, char * sNick) {
    uint16_t ui16dx = ((uint16_t *)&ui32Hash)[0];

    if(nicktable[ui16dx] == NULL) {
        return NULL;
    }

	BanItem *next = nicktable[ui16dx]->Bans;

    while(next != NULL) {
        BanItem *cur = next;
        next = cur->hashnicktablenext;

        if(cur->ui32NickHash == ui32Hash && strcasecmp(cur->sNick, sNick) == 0) {
            // PPK ... check if temban expired
			if(((cur->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
                if(acc_time >= cur->tempbanexpire) {
                    hashBanManager->RemBan(cur);
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

BanItem* hashMan::FindBanNickPerm(char * sNick, int iNickLen) {
    uint32_t hash = HashNick(sNick, iNickLen);
    
	return FindBanNickPerm(hash, sNick);
}
//---------------------------------------------------------------------------

BanItem* hashMan::FindBanNickPerm(const uint32_t &ui32Hash, char * sNick) {
    uint16_t ui16dx = ((uint16_t *)&ui32Hash)[0];

    if(nicktable[ui16dx] == NULL) {
        return NULL;
    }

    BanItem *next = nicktable[ui16dx]->Bans;

    while(next != NULL) {
        BanItem *cur = next;
        next = cur->hashnicktablenext;

        if(cur->ui32NickHash == ui32Hash && strcasecmp(cur->sNick, sNick) == 0) {
            if(((cur->ui8Bits & hashBanMan::PERM) == hashBanMan::PERM) == true) {
                return cur;
            }
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

RegUser* hashMan::FindReg(char * sNick, int iNickLen) {
    uint32_t ui32Hash = HashNick(sNick, iNickLen);
    uint16_t ui16dx = ((uint16_t *)&ui32Hash)[0];

    if(nicktable[ui16dx] == NULL) {
        return NULL;
    }

    RegUser *next = nicktable[ui16dx]->Regs;

    while(next != NULL) {
        RegUser *cur = next;
        next = cur->hashtablenext;

		if(cur->ui32Hash == ui32Hash && strcasecmp(cur->sNick, sNick) == 0) {
            return cur;
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

RegUser* hashMan::FindReg(User * u) {
    uint16_t ui16dx = ((uint16_t *)&u->ui32NickHash)[0];

    if(nicktable[ui16dx] == NULL) {
        return NULL;
    }

    RegUser *next = nicktable[ui16dx]->Regs;

    while(next != NULL) {
        RegUser *cur = next;
        next = cur->hashtablenext;

		if(cur->ui32Hash == u->ui32NickHash && strcasecmp(cur->sNick, u->Nick) == 0) {
            return cur;
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

RegUser* hashMan::FindReg(uint32_t ui32Hash, char * sNick) {
    uint16_t ui16dx = ((uint16_t *)&ui32Hash)[0];

    if(nicktable[ui16dx] == NULL) {
        return NULL;
    }

    RegUser *next = nicktable[ui16dx]->Regs;

    while(next != NULL) {
        RegUser *cur = next;
        next = cur->hashtablenext;

        if(cur->ui32Hash == ui32Hash && strcasecmp(cur->sNick, sNick) == 0) {
            return cur;
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

User * hashMan::FindUser(char * sNick, const unsigned int &iNickLen) {
    uint32_t ui32Hash = HashNick(sNick, iNickLen);
    uint16_t ui16dx = ((uint16_t *)&ui32Hash)[0]; 

    if(nicktable[ui16dx] == NULL) {
        return NULL;
    }

    User *next = nicktable[ui16dx]->Users;

    // pointer exists ? Then we need look for nick 
    if(next != NULL) {
        while(next != NULL) {
            User *cur = next;
            next = cur->hashtablenext;

            // we are looking for duplicate string
			if(cur->ui32NickHash == ui32Hash && cur->NickLen == iNickLen && strcasecmp(cur->Nick, sNick) == 0) {
                return cur;
            }
        }            
    }
    
    // no equal hash found, we dont have the nick in list
    return NULL;
}
//---------------------------------------------------------------------------

User * hashMan::FindUser(User * u) {
    uint16_t ui16dx = ((uint16_t *)&u->ui32NickHash)[0];

    if(nicktable[ui16dx] == NULL) {
        return NULL;
    }

    User *next = nicktable[ui16dx]->Users;  

    // pointer exists ? Then we need look for nick
    if(next != NULL) { 
        while(next != NULL) {
            User *cur = next;
            next = cur->hashtablenext;

            // we are looking for duplicate string
            if(cur->ui32NickHash == u->ui32NickHash && cur->NickLen == u->NickLen && strcasecmp(cur->Nick, u->Nick) == 0) {
                return cur;
            }
        }            
    }
    
    // no equal hash found, we dont have the nick in list
    return NULL;
}
//---------------------------------------------------------------------------

User * hashMan::FindUser(const uint32_t &ui32IpHash) {
    uint16_t ui16dx = ((uint16_t *)&ui32IpHash)[0];

    if(iptable[ui16dx] == NULL) {
        return NULL;
    }

	IpTableUsers * next = iptable[ui16dx]->UserIPs;

    while(next != NULL) {
		IpTableUsers * cur = next;
        next = cur->next;

        if(cur->Users->ui32IpHash == ui32IpHash) {
            return cur->Users;
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

uint32_t hashMan::GetUserIpCount(User * u) {
    uint16_t ui16dx = ((uint16_t *)&u->ui32IpHash)[0];

    if(iptable[ui16dx] == NULL) {
        return 0;
    }

	IpTableUsers * next = iptable[ui16dx]->UserIPs;

	while(next != NULL) {
		IpTableUsers * cur = next;
		next = cur->next;

        if(cur->Users->ui32IpHash == u->ui32IpHash) {
            return cur->ui32Count;
        }
	}

	return 0;
}
//---------------------------------------------------------------------------

void hashMan::RemoveAllUsers() {
    for(unsigned int i = 0; i < 65536; i++) {
        if(nicktable[i] != NULL) {
			if(nicktable[i]->Bans == NULL && nicktable[i]->Regs == NULL) {
				delete nicktable[i];
				nicktable[i] = NULL;
            } else {
                nicktable[i]->Users = NULL;
            }
        }
    }
}
//---------------------------------------------------------------------------
