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
#include "ResNickManager.h"
//---------------------------------------------------------------------------
#include "ServerManager.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
clsReservedNicksManager * clsReservedNicksManager::mPtr = NULL;
//---------------------------------------------------------------------------

clsReservedNicksManager::ReservedNick::ReservedNick() : pPrev(NULL), pNext(NULL), sNick(NULL), ui32Hash(0), bFromScript(false) {
	// ...
}
//---------------------------------------------------------------------------

clsReservedNicksManager::ReservedNick::~ReservedNick() {
#ifdef _WIN32
	if(sNick != NULL && HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sNick) == 0) {
        AppendDebugLog("%s - [MEM] Cannot deallocate sNick in clsReservedNicksManager::ReservedNick::~ReservedNick\n");
    }
#else
	free(sNick);
#endif
}
//---------------------------------------------------------------------------

clsReservedNicksManager::ReservedNick * clsReservedNicksManager::ReservedNick::CreateReservedNick(const char * sNewNick, uint32_t ui32NickHash) {
    ReservedNick * pReservedNick = new (std::nothrow) ReservedNick();

    if(pReservedNick == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate new pReservedNick in ReservedNick::CreateReservedNick\n");

        return NULL;
    }

    size_t szNickLen = strlen(sNewNick);
#ifdef _WIN32
    pReservedNick->sNick = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szNickLen+1);
#else
	pReservedNick->sNick = (char *)malloc(szNickLen+1);
#endif
    if(pReservedNick->sNick == NULL) {
        AppendDebugLogFormat("[MEM] Cannot allocate %" PRIu64 " bytes in ReservedNick::CreateReservedNick\n", (uint64_t)(szNickLen+1));

        delete pReservedNick;
        return NULL;
    }
    memcpy(pReservedNick->sNick, sNewNick, szNickLen);
    pReservedNick->sNick[szNickLen] = '\0';

	pReservedNick->ui32Hash = ui32NickHash;

    return pReservedNick;
}
//---------------------------------------------------------------------------

void clsReservedNicksManager::Load() {
#ifdef _WIN32
	FILE * fReservedNicks = fopen((clsServerManager::sPath + "\\cfg\\ReservedNicks.pxt").c_str(), "rt");
#else
	FILE * fReservedNicks = fopen((clsServerManager::sPath + "/cfg/ReservedNicks.pxt").c_str(), "rt");
#endif
    if(fReservedNicks == NULL) {
#ifdef _WIN32
        int imsgLen = sprintf(clsServerManager::pGlobalBuffer, "Error loading file ReservedNicks.pxt %s (%d)", WSErrorStr(errno), errno);
#else
		int imsgLen = sprintf(clsServerManager::pGlobalBuffer, "Error loading file ReservedNicks.pxt %s (%d)", ErrnoStr(errno), errno);
#endif
		CheckSprintf(imsgLen, clsServerManager::szGlobalBufferSize, "clsReservedNicksManager::Load");
#ifdef _BUILD_GUI
		::MessageBox(NULL, clsServerManager::pGlobalBuffer, g_sPtokaXTitle, MB_OK | MB_ICONERROR);
#else
		AppendLog(clsServerManager::pGlobalBuffer);
#endif
        exit(EXIT_FAILURE);
    }

	size_t szLen = 0;

	while(fgets(clsServerManager::pGlobalBuffer, (int)clsServerManager::szGlobalBufferSize, fReservedNicks) != NULL) {
		if(clsServerManager::pGlobalBuffer[0] == '#' || clsServerManager::pGlobalBuffer[0] == '\n') {
			continue;
		}

		szLen = strlen(clsServerManager::pGlobalBuffer)-1;

		clsServerManager::pGlobalBuffer[szLen] = '\0';

		if(clsServerManager::pGlobalBuffer[0] == '\0') {
			continue;
		}

		AddReservedNick(clsServerManager::pGlobalBuffer);
	}

    fclose(fReservedNicks);
}
//---------------------------------------------------------------------------

void clsReservedNicksManager::Save() {
#ifdef _WIN32
    FILE * fReservedNicks = fopen((clsServerManager::sPath + "\\cfg\\ReservedNicks.pxt").c_str(), "wb");
#else
	FILE * fReservedNicks = fopen((clsServerManager::sPath + "/cfg/ReservedNicks.pxt").c_str(), "wb");
#endif
    if(fReservedNicks == NULL) {
    	return;
    }

	static const char sPtokaXResNickFile[] = "#\n# PtokaX reserved nicks file\n#\n\n";
    fwrite(sPtokaXResNickFile, 1, sizeof(sPtokaXResNickFile)-1, fReservedNicks);

    ReservedNick * pCur = NULL,
        * pNext = pReservedNicks;

    while(pNext != NULL) {
        pCur = pNext;
        pNext = pCur->pNext;

		fprintf(fReservedNicks, "%s\n", pCur->sNick);
    }

	fclose(fReservedNicks);
}
//---------------------------------------------------------------------------

void clsReservedNicksManager::LoadXML() {
	TiXmlDocument doc;

#ifdef _WIN32
	if(doc.LoadFile((clsServerManager::sPath+"\\cfg\\ReservedNicks.xml").c_str()) == true) {
#else
	if(doc.LoadFile((clsServerManager::sPath+"/cfg/ReservedNicks.xml").c_str()) == true) {
#endif
		TiXmlHandle cfg(&doc);
		TiXmlNode * reservednicks = cfg.FirstChild("ReservedNicks").Node();
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

clsReservedNicksManager::clsReservedNicksManager() : pReservedNicks(NULL) {
#ifdef _WIN32
    if(FileExist((clsServerManager::sPath + "\\cfg\\ReservedNicks.pxt").c_str()) == true) {
#else
    if(FileExist((clsServerManager::sPath + "/cfg/ReservedNicks.pxt").c_str()) == true) {
#endif
        Load();

        return;
#ifdef _WIN32
    } else if(FileExist((clsServerManager::sPath + "\\cfg\\ReservedNicks.xml").c_str()) == true) {
#else
    } else if(FileExist((clsServerManager::sPath + "/cfg/ReservedNicks.xml").c_str()) == true) {
#endif
        LoadXML();

        return;
    } else {
		const char * sNicks[] = { "Hub-Security", "Admin", "Client", "PtokaX", "OpChat" };
		for(uint8_t ui8i = 0; ui8i < 5; ui8i++) {
			AddReservedNick(sNicks[ui8i]);
		}

		Save();
	}
}
//---------------------------------------------------------------------------
	
clsReservedNicksManager::~clsReservedNicksManager() {
	Save();

    ReservedNick * cur = NULL,
        * next = pReservedNicks;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

        delete cur;
    }
}
//---------------------------------------------------------------------------

// Check for reserved nicks true = reserved
bool clsReservedNicksManager::CheckReserved(const char * sNick, const uint32_t &hash) const {
    ReservedNick * cur = NULL,
        * next = pReservedNicks;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

		if(cur->ui32Hash == hash && strcasecmp(cur->sNick, sNick) == 0) {
            return true;
        }
    }

    return false;
}
//---------------------------------------------------------------------------

void clsReservedNicksManager::AddReservedNick(const char * sNick, const bool &bFromScript/* = false*/) {
    uint32_t ulHash = HashNick(sNick, strlen(sNick));

    if(CheckReserved(sNick, ulHash) == false) {
        ReservedNick * pNewNick = ReservedNick::CreateReservedNick(sNick, ulHash);
        if(pNewNick == NULL) {
			AppendDebugLog("%s - [MEM] Cannot allocate pNewNick in clsReservedNicksManager::AddReservedNick\n");
        	return;
        }

        if(pReservedNicks == NULL) {
            pReservedNicks = pNewNick;
        } else {
            pReservedNicks->pPrev = pNewNick;
            pNewNick->pNext = pReservedNicks;
            pReservedNicks = pNewNick;
        }

        pNewNick->bFromScript = bFromScript;
    }
}
//---------------------------------------------------------------------------

void clsReservedNicksManager::DelReservedNick(char * sNick, const bool &bFromScript/* = false*/) {
    uint32_t hash = HashNick(sNick, strlen(sNick));

    ReservedNick * cur = NULL,
        * next = pReservedNicks;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

        if(cur->ui32Hash == hash && strcmp(cur->sNick, sNick) == 0) {
            if(bFromScript == true && cur->bFromScript == false) {
                continue;
            }

            if(cur->pPrev == NULL) {
                if(cur->pNext == NULL) {
                    pReservedNicks = NULL;
                } else {
                    cur->pNext->pPrev = NULL;
                    pReservedNicks = cur->pNext;
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
