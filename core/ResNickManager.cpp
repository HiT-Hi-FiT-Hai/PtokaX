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
#include "ResNickManager.h"
//---------------------------------------------------------------------------
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
ResNickMan *ResNickManager = NULL;
//---------------------------------------------------------------------------

ResNickMan::ReservedNick::ReservedNick(const char * nick, uint32_t ui32NickHash) {
    size_t szNickLen = strlen(nick);
#ifdef _WIN32
    sNick = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szNickLen+1);
#else
	sNick = (char *)malloc(szNickLen+1);
#endif
    if(sNick == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes in ReservedNick::ReservedNick\n", (uint64_t)(szNickLen+1));

        return;
    }   
    memcpy(sNick, nick, szNickLen);
    sNick[szNickLen] = '\0';

	ui32Hash = ui32NickHash;

	prev = NULL;
    next = NULL;

    bFromScript = false;
}
//---------------------------------------------------------------------------

ResNickMan::ReservedNick::~ReservedNick() {
#ifdef _WIN32
	if(sNick != NULL && HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sNick) == 0) {
        AppendDebugLog("%s - [MEM] Cannot deallocate sNick in ResNickMan::ReservedNick::~ReservedNick\n", 0);
    }
#else
	free(sNick);
#endif
}
//---------------------------------------------------------------------------

ResNickMan::ResNickMan() {
    ReservedNicks = NULL;

	TiXmlDocument doc;
#ifdef _WIN32
	if(doc.LoadFile((PATH+"\\cfg\\ReservedNicks.xml").c_str()) == false) {
		TiXmlDocument doc((PATH+"\\cfg\\ReservedNicks.xml").c_str());
#else
	if(doc.LoadFile((PATH+"/cfg/ReservedNicks.xml").c_str()) == false) {
		TiXmlDocument doc((PATH+"/cfg/ReservedNicks.xml").c_str());
#endif
		doc.InsertEndChild(TiXmlDeclaration("1.0", "windows-1252", "yes"));
		TiXmlElement reservednicks("ReservedNicks");
		const char* Nicks[] = { "Hub-Security", "Admin", "Client", "PtokaX", "OpChat" };
		for(uint8_t ui8i = 0; ui8i < 5; ui8i++) {
			AddReservedNick(Nicks[ui8i]);
			TiXmlElement reservednick("ReservedNick");
			reservednick.InsertEndChild(TiXmlText(Nicks[ui8i]));

			reservednicks.InsertEndChild(reservednick);
		}
		doc.InsertEndChild(reservednicks);
		doc.SaveFile();
    }

#ifdef _WIN32
	if(doc.LoadFile((PATH+"\\cfg\\ReservedNicks.xml").c_str())) {
#else
	if(doc.LoadFile((PATH+"/cfg/ReservedNicks.xml").c_str())) {
#endif
		TiXmlHandle cfg(&doc);
		TiXmlNode *reservednicks = cfg.FirstChild("ReservedNicks").Node();
		if(reservednicks != NULL) {
			TiXmlNode *child = NULL;
			while((child = reservednicks->IterateChildren(child)) != NULL) {
				TiXmlNode *reservednick = child->FirstChild();
                    
				if(reservednick == NULL) {
					continue;
				}

				char *sNick = (char *)reservednick->Value();
                    
				AddReservedNick(sNick);
			}
        }
    }
}
//---------------------------------------------------------------------------
	
ResNickMan::~ResNickMan() {
    ReservedNick *next = ReservedNicks;

    while(next != NULL) {
        ReservedNick *cur = next;
        next = cur->next;

        delete cur;
    }
}
//---------------------------------------------------------------------------

// Check for reserved nicks true = reserved
bool ResNickMan::CheckReserved(const char * sNick, const uint32_t &hash) const {
    ReservedNick *next = ReservedNicks;

    while(next != NULL) {
        ReservedNick *cur = next;
        next = cur->next;

		if(cur->ui32Hash == hash && strcasecmp(cur->sNick, sNick) == 0) {
            return true;
        }
    }

    return false;
}
//---------------------------------------------------------------------------

void ResNickMan::AddReservedNick(const char * sNick, const bool &bFromScript/* = false*/) {
    uint32_t ulHash = HashNick(sNick, strlen(sNick));

    if(CheckReserved(sNick, ulHash) == false) {
        ReservedNick * pNewNick = new ReservedNick(sNick, ulHash);
        if(pNewNick == NULL) {
			AppendDebugLog("%s - [MEM] Cannot allocate pNewNick in ResNickMan::AddReservedNick\n", 0);
        	return;
        } else if(pNewNick->sNick == NULL) {
            delete pNewNick;

			AppendDebugLog("%s - [MEM] Cannot allocate pNewNick->sNick in ResNickMan::AddReservedNick\n", 0);
        	return;
        }

        if(ReservedNicks == NULL) {
            ReservedNicks = pNewNick;
        } else {
            ReservedNicks->prev = pNewNick;
            pNewNick->next = ReservedNicks;
            ReservedNicks = pNewNick;
        }

        pNewNick->bFromScript = bFromScript;
    }
}
//---------------------------------------------------------------------------

void ResNickMan::DelReservedNick(char * sNick, const bool &bFromScript/* = false*/) {
    uint32_t hash = HashNick(sNick, strlen(sNick));

    ReservedNick *next = ReservedNicks;
    while(next != NULL) {
        ReservedNick *cur = next;
        next = cur->next;

        if(cur->ui32Hash == hash && strcmp(cur->sNick, sNick) == 0) {
            if(bFromScript == true && cur->bFromScript == false) {
                continue;
            }

            if(cur->prev == NULL) {
                if(cur->next == NULL) {
                    ReservedNicks = NULL;
                } else {
                    cur->next->prev = NULL;
                    ReservedNicks = cur->next;
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
