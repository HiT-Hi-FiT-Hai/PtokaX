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
#include "ResNickManager.h"
//---------------------------------------------------------------------------
#include "utility.h"
//---------------------------------------------------------------------------
ResNickMan *ResNickManager = NULL;
//---------------------------------------------------------------------------

ResNickMan::ReservedNick::ReservedNick(const char * nick, uint32_t ui32NickHash) {
    int iNickLen = strlen(nick);
    sNick = (char *) malloc(iNickLen+1);
    if(sNick == NULL) {
        string sDbgstr = "[BUF] Cannot allocate "+string(iNickLen+1)+
			" bytes of memory in ReservedNick::ReservedNick!";
        AppendSpecialLog(sDbgstr);
        return;
    }   
    memcpy(sNick, nick, iNickLen);
    sNick[iNickLen] = '\0';

	ui32Hash = ui32NickHash;

	prev = NULL;
    next = NULL;

    bFromScript = false;
}
//---------------------------------------------------------------------------

ResNickMan::ReservedNick::~ReservedNick() {
	free(sNick);
}
//---------------------------------------------------------------------------

ResNickMan::ResNickMan() {
    ReservedNicks = NULL;

	TiXmlDocument doc;
	if(doc.LoadFile((PATH+"/cfg/ReservedNicks.xml").c_str()) == false) {
		TiXmlDocument doc((PATH+"/cfg/ReservedNicks.xml").c_str());
		doc.InsertEndChild(TiXmlDeclaration("1.0", "windows-1252", "yes"));
		TiXmlElement reservednicks("ReservedNicks");
		const char* Nicks[] = { "Hub-Security", "Admin", "Client", "PtokaX", "OpChat" };
		for(unsigned int i = 0;i < 5;i++) {
			AddReservedNick(Nicks[i]);
			TiXmlElement reservednick("ReservedNick");
			reservednick.InsertEndChild(TiXmlText(Nicks[i]));

			reservednicks.InsertEndChild(reservednick);
		}
		doc.InsertEndChild(reservednicks);
		doc.SaveFile();
    }

	if(doc.LoadFile((PATH+"/cfg/ReservedNicks.xml").c_str())) {
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
bool ResNickMan::CheckReserved(const char * sNick, const uint32_t &hash) {
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
    unsigned long ulHash = HashNick(sNick, strlen(sNick));

    if(CheckReserved(sNick, ulHash) == false) {
        ReservedNick *newNick = new ReservedNick(sNick, ulHash);
        if(newNick == NULL) {
			string sDbgstr = "[BUF] Cannot allocate ResNickMan::AddReservedNick!";
			AppendSpecialLog(sDbgstr);
        	exit(EXIT_FAILURE);
        }

        if(ReservedNicks == NULL) {
            ReservedNicks = newNick;
        } else {
            ReservedNicks->prev = newNick;
            newNick->next = ReservedNicks;
            ReservedNicks = newNick;
        }

        newNick->bFromScript = bFromScript;
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
