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

//------------------------------------------------------------------------------
#include "stdinc.h"
//------------------------------------------------------------------------------
#include "LuaInc.h"
//------------------------------------------------------------------------------
#include "LuaScriptManager.h"
//------------------------------------------------------------------------------
#include "ServerManager.h"
#include "SettingManager.h"
#include "User.h"
#include "utility.h"
//------------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//------------------------------------------------------------------------------
#include "LuaScript.h"
//------------------------------------------------------------------------------
#ifdef _BUILD_GUI
    #include "../gui.win/MainWindowPageScripts.h"
#endif
//------------------------------------------------------------------------------
clsScriptManager * clsScriptManager::mPtr = NULL;
//------------------------------------------------------------------------------

void clsScriptManager::LoadXML() {
	// PPK ... first start all script in order from xml file
#ifdef _WIN32
	TiXmlDocument doc((clsServerManager::sPath+"\\cfg\\Scripts.xml").c_str());
#else
	TiXmlDocument doc((clsServerManager::sPath+"/cfg/Scripts.xml").c_str());
#endif
	if(doc.LoadFile() == false) {
        if(doc.ErrorId() != TiXmlBase::TIXML_ERROR_OPENING_FILE && doc.ErrorId() != TiXmlBase::TIXML_ERROR_DOCUMENT_EMPTY) {
            int iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "Error loading file Scripts.xml. %s (Col: %d, Row: %d)", doc.ErrorDesc(), doc.Column(), doc.Row());
			CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsScriptManager::LoadXML");
#ifdef _BUILD_GUI
			::MessageBox(NULL, clsServerManager::pGlobalBuffer, g_sPtokaXTitle, MB_OK | MB_ICONERROR);
#else
			AppendLog(clsServerManager::pGlobalBuffer);
#endif
            exit(EXIT_FAILURE);
        }
    } else {
		TiXmlHandle cfg(&doc);
		TiXmlNode *scripts = cfg.FirstChild("Scripts").Node();
		if(scripts != NULL) {
			TiXmlNode *child = NULL;
			while((child = scripts->IterateChildren(child)) != NULL) {
				TiXmlNode *script = child->FirstChild("Name");
    
				if(script == NULL || (script = script->FirstChild()) == NULL) {
					continue;
				}
    
				char *name = (char *)script->Value();

				if(FileExist((clsServerManager::sScriptPath+string(name)).c_str()) == false) {
					continue;
				}

				if((script = child->FirstChild("Enabled")) == NULL ||
					(script = script->FirstChild()) == NULL) {
					continue;
				}
    
				bool enabled = atoi(script->Value()) == 0 ? false : true;

				if(FindScript(name) != NULL) {
					continue;
				}

				AddScript(name, enabled, false);
            }
        }
    }
}
//------------------------------------------------------------------------------

clsScriptManager::clsScriptManager() : pRunningScriptE(NULL), pRunningScriptS(NULL), ppScriptTable(NULL), pActualUser(NULL), pTimerListS(NULL), pTimerListE(NULL), ui8ScriptCount(0), ui8BotsCount(0), bMoved(false) {
#ifdef _WIN32
    clsServerManager::hLuaHeap = ::HeapCreate(HEAP_NO_SERIALIZE, 0x80000, 0);
#endif

#ifdef _WIN32
    if(FileExist((clsServerManager::sPath + "\\cfg\\Scripts.pxt").c_str()) == false) {
#else
    if(FileExist((clsServerManager::sPath + "/cfg/Scripts.pxt").c_str()) == false) {
#endif
        LoadXML();

        return;
    }

#ifdef _WIN32
	FILE * fScriptsFile = fopen((clsServerManager::sPath + "\\cfg\\Scripts.pxt").c_str(), "rt");
#else
	FILE * fScriptsFile = fopen((clsServerManager::sPath + "/cfg/Scripts.pxt").c_str(), "rt");
#endif
    if(fScriptsFile == NULL) {
#ifdef _WIN32
        int imsgLen = sprintf(clsServerManager::pGlobalBuffer, "Error loading file Scripts.pxt %s (%d)", WSErrorStr(errno), errno);
#else
		int imsgLen = sprintf(clsServerManager::pGlobalBuffer, "Error loading file Scripts.pxt %s (%d)", ErrnoStr(errno), errno);
#endif
		CheckSprintf(imsgLen, clsServerManager::szGlobalBufferSize, "clsScriptManager::clsScriptManager");
#ifdef _BUILD_GUI
		::MessageBox(NULL, clsServerManager::pGlobalBuffer, g_sPtokaXTitle, MB_OK | MB_ICONERROR);
#else
		AppendLog(clsServerManager::pGlobalBuffer);
#endif
        exit(EXIT_FAILURE);
    }

	char * sReturn = NULL;
	size_t szLen = 0;

	while((sReturn = fgets(clsServerManager::pGlobalBuffer, (int)clsServerManager::szGlobalBufferSize, fScriptsFile)) != NULL) {
		if(clsServerManager::pGlobalBuffer[0] == '#' || clsServerManager::pGlobalBuffer[0] == '\n') {
			continue;
		}

		szLen = strlen(clsServerManager::pGlobalBuffer)-1;

		if(szLen < 7) {
			continue;
		}

		clsServerManager::pGlobalBuffer[szLen] = '\0';

		for(size_t szi = szLen-1; szi != 0; szi--) {
			if(isspace(clsServerManager::pGlobalBuffer[szi-1]) != 0 || clsServerManager::pGlobalBuffer[szi-1] == '=') {
				clsServerManager::pGlobalBuffer[szi-1] = '\0';
				continue;
			}

			break;
		}

		if(clsServerManager::pGlobalBuffer[0] == '\0' || (clsServerManager::pGlobalBuffer[szLen-1] != '1' && clsServerManager::pGlobalBuffer[szLen-1] != '0')) {
			continue;
		}

		if(FileExist((clsServerManager::sScriptPath+string(clsServerManager::pGlobalBuffer)).c_str()) == false || FindScript(clsServerManager::pGlobalBuffer) != NULL) {
			continue;
		}

		AddScript(clsServerManager::pGlobalBuffer, clsServerManager::pGlobalBuffer[szLen-1] == '1' ? true : false, false);
	}

    fclose(fScriptsFile);
}
//------------------------------------------------------------------------------

clsScriptManager::~clsScriptManager() {
    pRunningScriptS = NULL;
    pRunningScriptE = NULL;

    for(uint8_t ui8i = 0; ui8i < ui8ScriptCount; ui8i++) {
		delete ppScriptTable[ui8i];
    }

#ifdef _WIN32
	if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)ppScriptTable) == 0) {
		AppendDebugLog("%s - [MEM] Cannot deallocate ScriptTable in clsScriptManager::~clsScriptManager\n");
	}
#else
	free(ppScriptTable);
#endif

	ppScriptTable = NULL;

	ui8ScriptCount = 0;

    pActualUser = NULL;

    ui8BotsCount = 0;

#ifdef _WIN32
    ::HeapDestroy(clsServerManager::hLuaHeap);
#endif
}
//------------------------------------------------------------------------------

void clsScriptManager::Start() {
    ui8BotsCount = 0;
    pActualUser = NULL;

    
    // PPK ... first look for deleted and new scripts
    CheckForDeletedScripts();
    CheckForNewScripts();

    // PPK ... second start all enabled scripts
    for(uint8_t ui8i = 0; ui8i < ui8ScriptCount; ui8i++) {
		if(ppScriptTable[ui8i]->bEnabled == true) {
        	if(ScriptStart(ppScriptTable[ui8i]) == true) {
				AddRunningScript(ppScriptTable[ui8i]);
			} else {
                ppScriptTable[ui8i]->bEnabled = false;
            }
		}
	}

#ifdef _BUILD_GUI
    clsMainWindowPageScripts::mPtr->AddScriptsToList(true);
#endif
}
//------------------------------------------------------------------------------

#ifdef _BUILD_GUI
bool clsScriptManager::AddScript(char * sName, const bool &bEnabled, const bool &bNew) {
#else
bool clsScriptManager::AddScript(char * sName, const bool &bEnabled, const bool &/*bNew*/) {
#endif
	if(ui8ScriptCount == 254) {
        return false;
    }

    Script ** oldbuf = ppScriptTable;
#ifdef _WIN32
    if(ppScriptTable == NULL) {
        ppScriptTable = (Script **)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, (ui8ScriptCount+1)*sizeof(Script *));
    } else {
		ppScriptTable = (Script **)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, (ui8ScriptCount+1)*sizeof(Script *));
    }
#else
	ppScriptTable = (Script **)realloc(oldbuf, (ui8ScriptCount+1)*sizeof(Script *));
#endif
	if(ppScriptTable == NULL) {
        ppScriptTable = oldbuf;

		AppendDebugLog("%s - [MEM] Cannot (re)allocate ppScriptTable in clsScriptManager::AddScript\n");
        return false;
    }

    ppScriptTable[ui8ScriptCount] = Script::CreateScript(sName, bEnabled);

    if(ppScriptTable[ui8ScriptCount] == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate new Script in clsScriptManager::AddScript\n");

        return false;
    }

    ui8ScriptCount++;

#ifdef _BUILD_GUI
    if(bNew == true) {
        clsMainWindowPageScripts::mPtr->ScriptToList(ui8ScriptCount-1, true, false);
    }
#endif

    return true;
}
//------------------------------------------------------------------------------

void clsScriptManager::Stop() {
	Script * S = NULL,
        * next = pRunningScriptS;

    pRunningScriptS = NULL;
	pRunningScriptE = NULL;

    while(next != NULL) {
		S = next;
		next = S->pNext;

		ScriptStop(S);
	}

	pActualUser = NULL;

#ifdef _BUILD_GUI
    clsMainWindowPageScripts::mPtr->ClearMemUsageAll();
#endif
}
//------------------------------------------------------------------------------

void clsScriptManager::AddRunningScript(Script * curScript) {
	if(pRunningScriptS == NULL) {
		pRunningScriptS = curScript;
		pRunningScriptE = curScript;
		return;
	}

   	curScript->pPrev = pRunningScriptE;
    pRunningScriptE->pNext = curScript;
    pRunningScriptE = curScript;
}
//------------------------------------------------------------------------------

void clsScriptManager::RemoveRunningScript(Script * curScript) {
	// single entry
	if(curScript->pPrev == NULL && curScript->pNext == NULL) {
    	pRunningScriptS = NULL;
        pRunningScriptE = NULL;
        return;
    }

    // first in list
	if(curScript->pPrev == NULL) {
        pRunningScriptS = curScript->pNext;
        pRunningScriptS->pPrev = NULL;
        return;
    }

    // last in list
    if(curScript->pNext == NULL) {
        pRunningScriptE = curScript->pPrev;
    	pRunningScriptE->pNext = NULL;
        return;
    }

    // in the middle
    curScript->pPrev->pNext = curScript->pNext;
    curScript->pNext->pPrev = curScript->pPrev;
}
//------------------------------------------------------------------------------

void clsScriptManager::SaveScripts() {
#ifdef _WIN32
    FILE * fScriptsFile = fopen((clsServerManager::sPath + "\\cfg\\Scripts.pxt").c_str(), "wb");
#else
	FILE * fScriptsFile = fopen((clsServerManager::sPath + "/cfg/Scripts.pxt").c_str(), "wb");
#endif
    if(fScriptsFile == NULL) {
    	return;
    }

	static const char sPtokaXScriptsFile[] = "#\n# PtokaX scripts settings file\n#\n\n";
    fwrite(sPtokaXScriptsFile, 1, sizeof(sPtokaXScriptsFile)-1, fScriptsFile);

	for(uint8_t ui8i = 0; ui8i < ui8ScriptCount; ui8i++) {
		if(FileExist((clsServerManager::sScriptPath+string(ppScriptTable[ui8i]->sName)).c_str()) == false) {
			continue;
        }

        fprintf(fScriptsFile, "%s\t=\t%c\n", ppScriptTable[ui8i]->sName, ppScriptTable[ui8i]->bEnabled == true ? '1' : '0');
    }

    fclose(fScriptsFile);
}
//------------------------------------------------------------------------------

void clsScriptManager::CheckForDeletedScripts() {
    uint8_t ui8i = 0;

	while(ui8i < ui8ScriptCount) {
		if(FileExist((clsServerManager::sScriptPath+string(ppScriptTable[ui8i]->sName)).c_str()) == true || ppScriptTable[ui8i]->pLUA != NULL) {
			ui8i++;
			continue;
        }

		delete ppScriptTable[ui8i];

		for(uint8_t ui8j = ui8i; ui8j+1 < ui8ScriptCount; ui8j++) {
            ppScriptTable[ui8j] = ppScriptTable[ui8j+1];
        }

        ppScriptTable[ui8ScriptCount-1] = NULL;
        ui8ScriptCount--;
    }
}
//------------------------------------------------------------------------------

void clsScriptManager::CheckForNewScripts() {
#ifdef _WIN32
    struct _finddata_t luafile;
    intptr_t hFile = _findfirst((clsServerManager::sScriptPath+"\\*.lua").c_str(), &luafile);

    if(hFile != -1) {
        do {
			if((luafile.attrib & _A_SUBDIR) != 0 ||
				stricmp(luafile.name+(strlen(luafile.name)-4), ".lua") != 0) {
				continue;
			}

			if(FindScript(luafile.name) != NULL) {
                continue;
            }

			AddScript(luafile.name, false, false);
        } while(_findnext(hFile, &luafile) == 0);

		_findclose(hFile);
	}
#else
    DIR * p_scriptdir = opendir(clsServerManager::sScriptPath.c_str());

    if(p_scriptdir == NULL) {
        return;
    }

    struct dirent * p_dirent;
    struct stat s_buf;

    while((p_dirent = readdir(p_scriptdir)) != NULL) {
        if(stat((clsServerManager::sScriptPath + p_dirent->d_name).c_str(), &s_buf) != 0 ||
            (s_buf.st_mode & S_IFDIR) != 0 || 
            strcasecmp(p_dirent->d_name + (strlen(p_dirent->d_name)-4), ".lua") != 0) {
            continue;
        }

		if(FindScript(p_dirent->d_name) != NULL) {
            continue;
        }

		AddScript(p_dirent->d_name, false, false);
    }

    closedir(p_scriptdir);
#endif
}
//------------------------------------------------------------------------------

void clsScriptManager::Restart() {
	OnExit();
	Stop();

    CheckForDeletedScripts();

	Start();
	OnStartup();

#ifdef _BUILD_GUI
    clsMainWindowPageScripts::mPtr->AddScriptsToList(true);
#endif
}
//------------------------------------------------------------------------------

Script * clsScriptManager::FindScript(char * sName) {
    for(uint8_t ui8i = 0; ui8i < ui8ScriptCount; ui8i++) {
		if(strcasecmp(ppScriptTable[ui8i]->sName, sName) == 0) {
            return ppScriptTable[ui8i];
        }
    }

    return NULL;
}
//------------------------------------------------------------------------------

Script * clsScriptManager::FindScript(lua_State * L) {
    Script * cur = NULL,
        * next = pRunningScriptS;

    while(next != NULL) {
    	cur = next;
        next = cur->pNext;

        if(cur->pLUA == L) {
            return cur;
        }
    }

    return NULL;
}
//------------------------------------------------------------------------------

uint8_t clsScriptManager::FindScriptIdx(char * sName) {
    for(uint8_t ui8i = 0; ui8i < ui8ScriptCount; ui8i++) {
		if(strcasecmp(ppScriptTable[ui8i]->sName, sName) == 0) {
            return ui8i;
        }
    }

    return ui8ScriptCount;
}
//------------------------------------------------------------------------------

bool clsScriptManager::StartScript(Script * curScript, const bool &bEnable) {
    uint8_t ui8dx = 255;
    for(uint8_t ui8i = 0; ui8i < ui8ScriptCount; ui8i++) {
        if(curScript == ppScriptTable[ui8i]) {
            ui8dx = ui8i;
            break;
        }
    }

    if(ui8dx == 255) {
        return false;
	}

    if(bEnable == true) {
        curScript->bEnabled = true;
#ifdef _BUILD_GUI
        clsMainWindowPageScripts::mPtr->UpdateCheck(ui8dx);
#endif
    }

    if(ScriptStart(curScript) == false) {
        curScript->bEnabled = false;
#ifdef _BUILD_GUI
        clsMainWindowPageScripts::mPtr->UpdateCheck(ui8dx);
#endif
        return false;
    }

	if(pRunningScriptS == NULL) {
		pRunningScriptS = curScript;
		pRunningScriptE = curScript;
	} else {
		// previous script
		if(ui8dx != 0) {
			for(int16_t i16i = (int16_t)(ui8dx-1); i16i > -1; i16i--) {
				if(ppScriptTable[i16i]->bEnabled == true) {
					ppScriptTable[i16i]->pNext = curScript;
					curScript->pPrev = ppScriptTable[i16i];
					break;
				}
			}

			if(curScript->pPrev == NULL) {
				pRunningScriptS = curScript;
			}
		} else {
			curScript->pNext = pRunningScriptS;
			pRunningScriptS->pPrev = curScript;
			pRunningScriptS = curScript;
        }

		// next script
		if(ui8dx != ui8ScriptCount-1) {
			for(uint8_t ui8i = ui8dx+1; ui8i < ui8ScriptCount; ui8i++) {
				if(ppScriptTable[ui8i]->bEnabled == true) {
					ppScriptTable[ui8i]->pPrev = curScript;
					curScript->pNext = ppScriptTable[ui8i];
					break;
				}
			}

			if(curScript->pNext == NULL) {
				pRunningScriptE = curScript;
			}
		} else {
			curScript->pPrev = pRunningScriptE;
			pRunningScriptE->pNext = curScript;
			pRunningScriptE = curScript;
        }
	}


	if(clsServerManager::bServerRunning == true) {
        ScriptOnStartup(curScript);
    }

    return true;
}
//------------------------------------------------------------------------------

void clsScriptManager::StopScript(Script * curScript, const bool &bDisable) {
    if(bDisable == true) {
		curScript->bEnabled = false;

#ifdef _BUILD_GUI
	    for(uint8_t ui8i = 0; ui8i < ui8ScriptCount; ui8i++) {
            if(curScript == ppScriptTable[ui8i]) {
                clsMainWindowPageScripts::mPtr->UpdateCheck(ui8i);
                break;
            }
        }
#endif
    }

	RemoveRunningScript(curScript);

    if(clsServerManager::bServerRunning == true) {
        ScriptOnExit(curScript);
    }

	ScriptStop(curScript);
}
//------------------------------------------------------------------------------

void clsScriptManager::MoveScript(const uint8_t &ui8ScriptPosInTbl, const bool &bUp) {
    if(bUp == true) {
		if(ui8ScriptPosInTbl == 0) {
            return;
        }

        Script * cur = ppScriptTable[ui8ScriptPosInTbl];
		ppScriptTable[ui8ScriptPosInTbl] = ppScriptTable[ui8ScriptPosInTbl-1];
		ppScriptTable[ui8ScriptPosInTbl-1] = cur;

		// if one of moved scripts not running then return
		if(cur->pLUA == NULL || ppScriptTable[ui8ScriptPosInTbl]->pLUA == NULL) {
			return;
		}

		if(cur->pPrev == NULL) { // first running script, nothing to move
			return;
		} else if(cur->pNext == NULL) { // last running script
			// set prev script as last
			pRunningScriptE = cur->pPrev;

			// change prev prev script next
			if(pRunningScriptE->pPrev != NULL) {
				pRunningScriptE->pPrev->pNext = cur;
			} else {
				pRunningScriptS = cur;
			}

			// change cur script prev and next
			cur->pPrev = pRunningScriptE->pPrev;
			cur->pNext = pRunningScriptE;

			// change prev script prev to cur and his next to NULL
			pRunningScriptE->pPrev = cur;
			pRunningScriptE->pNext = NULL;
		} else { // not first, not last ...
			// remember original prev and next
			Script * prev = cur->pPrev;
			Script * next = cur->pNext;

			// change cur script prev
			cur->pPrev = prev->pPrev;

			// change prev script next
			prev->pNext = next;

			// change cur script next
			cur->pNext = prev;

			// change next script prev
			next->pPrev = prev;

			// change prev prev script next
			if(prev->pPrev != NULL) {
				prev->pPrev->pNext = cur;
			} else {
				pRunningScriptS = cur;
			}

			// change prev script prev
			prev->pPrev = cur;
		}
    } else {
		if(ui8ScriptPosInTbl == ui8ScriptCount-1) {
            return;
		}

        Script * cur = ppScriptTable[ui8ScriptPosInTbl];
		ppScriptTable[ui8ScriptPosInTbl] = ppScriptTable[ui8ScriptPosInTbl+1];
        ppScriptTable[ui8ScriptPosInTbl+1] = cur;

		// if one of moved scripts not running then return
		if(cur->pLUA == NULL || ppScriptTable[ui8ScriptPosInTbl]->pLUA == NULL) {
			return;
		}

        if(cur->pNext == NULL) { // last running script, nothing to move
            return;
        } else if(cur->pPrev == NULL) { // first running script
            //set next running script as first
            pRunningScriptS = cur->pNext;

            // change next next script prev
			if(pRunningScriptS->pNext != NULL) {
				pRunningScriptS->pNext->pPrev = cur;
			} else {
				pRunningScriptE = cur;
			}

			// change cur script prev and next
            cur->pPrev = pRunningScriptS;
			cur->pNext = pRunningScriptS->pNext;

            // change next script next to cur and his prev to NULL
			pRunningScriptS->pPrev = NULL;
			pRunningScriptS->pNext = cur;
		} else { // not first, not last ...
            // remember original prev and next
            Script * prev = cur->pPrev;
            Script * next = cur->pNext;

			// change cur script next
			cur->pNext = next->pNext;

			// change next script prev
			next->pPrev = prev;

			// change cur script prev
			cur->pPrev = next;

			// change prev script next
			prev->pNext = next;

			// change next next script prev
            if(next->pNext != NULL) {
                next->pNext->pPrev = cur;
			} else {
				pRunningScriptE = cur;
			}

			// change next script next
			next->pNext = cur;
		}
	}
}
//------------------------------------------------------------------------------

void clsScriptManager::DeleteScript(const uint8_t &ui8ScriptPosInTbl) {
    Script * cur = ppScriptTable[ui8ScriptPosInTbl];

	if(cur->pLUA != NULL) {
		StopScript(cur, false);
	}

	if(FileExist((clsServerManager::sScriptPath+string(cur->sName)).c_str()) == true) {
#ifdef _WIN32
        DeleteFile((clsServerManager::sScriptPath+string(cur->sName)).c_str());
#else
		unlink((clsServerManager::sScriptPath+string(cur->sName)).c_str());
#endif
    }

    delete cur;

	for(uint8_t ui8i = ui8ScriptPosInTbl; ui8i+1 < ui8ScriptCount; ui8i++) {
        ppScriptTable[ui8i] = ppScriptTable[ui8i+1];
    }

    ppScriptTable[ui8ScriptCount-1] = NULL;
    ui8ScriptCount--;
}
//------------------------------------------------------------------------------

void clsScriptManager::OnStartup() {
    if(clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
        return;
    }

    pActualUser = NULL;
    bMoved = false;

    Script * cur = NULL,
        * next = pRunningScriptS;
        
    while(next != NULL) {
    	cur = next;
        next = cur->pNext;

		if(((cur->ui16Functions & Script::ONSTARTUP) == Script::ONSTARTUP) == true && (bMoved == false || cur->bProcessed == false)) {
            cur->bProcessed = true;
            ScriptOnStartup(cur);
        }
	}
}
//------------------------------------------------------------------------------

void clsScriptManager::OnExit(bool bForce/* = false*/) {
    if(bForce == false && clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
        return;
    }

    pActualUser = NULL;
    bMoved = false;

    Script * cur = NULL,
        * next = pRunningScriptS;
        
    while(next != NULL) {
    	cur = next;
        next = cur->pNext;

		if(((cur->ui16Functions & Script::ONEXIT) == Script::ONEXIT) == true && (bMoved == false || cur->bProcessed == false)) {
            cur->bProcessed = true;
            ScriptOnExit(cur);
        }
    }
}
//------------------------------------------------------------------------------

bool clsScriptManager::Arrival(User * u, char * sData, const size_t &szLen, const unsigned char &uiType) {
	if(clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
		return false;
	}

	static const uint32_t iLuaArrivalBits[] = {
        0x1, 
        0x2, 
        0x4, 
        0x8, 
        0x10, 
        0x20, 
        0x40, 
        0x80, 
        0x100, 
        0x200, 
        0x400, 
        0x800, 
        0x1000, 
        0x2000, 
        0x4000, 
        0x8000, 
        0x10000, 
        0x20000, 
        0x40000, 
        0x80000, 
        0x100000
	};

    bMoved = false;

    int iTop = 0, iTraceback = 0;

    Script * cur = NULL,
        * next = pRunningScriptS;
        
    while(next != NULL) {
    	cur = next;
        next = cur->pNext;

        // if any of the scripts returns a nonzero value,
		// then stop for all other scripts
        if(((cur->ui32DataArrivals & iLuaArrivalBits[uiType]) == iLuaArrivalBits[uiType]) == true && (bMoved == false || cur->bProcessed == false)) {
            cur->bProcessed = true;

            lua_pushcfunction(cur->pLUA, ScriptTraceback);
            iTraceback = lua_gettop(cur->pLUA);

            // PPK ... table of arrivals
            static const char* arrival[] = { "ChatArrival", "KeyArrival", "ValidateNickArrival", "PasswordArrival",
            "VersionArrival", "GetNickListArrival", "MyINFOArrival", "GetINFOArrival", "SearchArrival",
            "ToArrival", "ConnectToMeArrival", "MultiConnectToMeArrival", "RevConnectToMeArrival", "SRArrival",
            "UDPSRArrival", "KickArrival", "OpForceMoveArrival", "SupportsArrival", "BotINFOArrival", 
            "CloseArrival", "UnknownArrival" };

            lua_getglobal(cur->pLUA, arrival[uiType]);
            iTop = lua_gettop(cur->pLUA);
            if(lua_isfunction(cur->pLUA, iTop) == 0) {
                cur->ui32DataArrivals &= ~iLuaArrivalBits[uiType];

                lua_settop(cur->pLUA, 0);
                continue;
            }

            pActualUser = u;

            lua_checkstack(cur->pLUA, 2); // we need 2 empty slots in stack, check it to be sure

			ScriptPushUser(cur->pLUA, u); // usertable
            lua_pushlstring(cur->pLUA, sData, szLen); // sData

            // two passed parameters, zero returned
            if(lua_pcall(cur->pLUA, 2, LUA_MULTRET, iTraceback) != 0) {
                ScriptError(cur);

                lua_settop(cur->pLUA, 0);
                continue;
            }

            pActualUser = NULL;
        
            // check the return value
            // if no return value specified, continue
            // if non-boolean value returned, continue
            // if a boolean true value dwels on the stack, return it

            iTop = lua_gettop(cur->pLUA);
        
            // no return value
            if(iTop == 0) {
                continue;
            }

			if(lua_type(cur->pLUA, iTop) != LUA_TBOOLEAN || lua_toboolean(cur->pLUA, iTop) == 0) {
                lua_settop(cur->pLUA, 0);
                continue;
            }

            // clear the stack for sure
            lua_settop(cur->pLUA, 0);

			return true; // true means DO NOT process data by the hub's core
        }
    }

    return false;
}
//------------------------------------------------------------------------------

bool clsScriptManager::UserConnected(User * u) {
	if(clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
        return false;
    }

    uint8_t ui8Type = 0; // User
    if(u->i32Profile != -1) {
        if(((u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
            ui8Type = 1; // Reg
		} else {
			ui8Type = 2; // OP
		}
    }

    bMoved = false;

    int iTop = 0, iTraceback = 0;

    Script * cur = NULL,
        * next = pRunningScriptS;
        
    while(next != NULL) {
    	cur = next;
        next = cur->pNext;

		static const uint32_t iConnectedBits[] = { Script::USERCONNECTED, Script::REGCONNECTED, Script::OPCONNECTED };

		if(((cur->ui16Functions & iConnectedBits[ui8Type]) == iConnectedBits[ui8Type]) == true && (bMoved == false || cur->bProcessed == false)) {
            cur->bProcessed = true;

            lua_pushcfunction(cur->pLUA, ScriptTraceback);
            iTraceback = lua_gettop(cur->pLUA);

            // PPK ... table of connected functions
            static const char* ConnectedFunction[] = { "UserConnected", "RegConnected", "OpConnected" };

            lua_getglobal(cur->pLUA, ConnectedFunction[ui8Type]);
            iTop = lua_gettop(cur->pLUA);
			if(lua_isfunction(cur->pLUA, iTop) == 0) {
				switch(ui8Type) {
					case 0:
						cur->ui16Functions &= ~Script::USERCONNECTED;
						break;
					case 1:
						cur->ui16Functions &= ~Script::REGCONNECTED;
						break;
					case 2:
						cur->ui16Functions &= ~Script::OPCONNECTED;
						break;
				}

                lua_settop(cur->pLUA, 0);
                continue;
            }

            pActualUser = u;

            lua_checkstack(cur->pLUA, 1); // we need 1 empty slots in stack, check it to be sure

			ScriptPushUser(cur->pLUA, u); // usertable

            // 1 passed parameters, zero returned
			if(lua_pcall(cur->pLUA, 1, LUA_MULTRET, iTraceback) != 0) {
                ScriptError(cur);

                lua_settop(cur->pLUA, 0);
                continue;
            }

            pActualUser = NULL;
            
            // check the return value
            // if no return value specified, continue
            // if non-boolean value returned, continue
            // if a boolean true value dwels on the stack, return
        
            iTop = lua_gettop(cur->pLUA);
        
            // no return value
            if(iTop == 0) {
                continue;
            }
        
			if(lua_type(cur->pLUA, iTop) != LUA_TBOOLEAN || lua_toboolean(cur->pLUA, iTop) == 0) {
                lua_settop(cur->pLUA, 0);
                continue;
            }
       
            // clear the stack for sure
            lua_settop(cur->pLUA, 0);

            UserDisconnected(u, cur);

            return true; // means DO NOT process by next scripts
        }
    }

    return false;
}
//------------------------------------------------------------------------------

void clsScriptManager::UserDisconnected(User * u, Script * pScript/* = NULL*/) {
	if(clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
        return;
    }

    uint8_t ui8Type = 0; // User
    if(u->i32Profile != -1) {
        if(((u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
            ui8Type = 1; // Reg
		} else {
			ui8Type = 2; // OP
		}
    }

    bMoved = false;

    int iTop = 0, iTraceback = 0;

    Script * cur = NULL,
        * next = pRunningScriptS;
        
    while(next != NULL) {
    	cur = next;
        next = cur->pNext;

        if(cur == pScript) {
            return;
        }

		static const uint32_t iDisconnectedBits[] = { Script::USERDISCONNECTED, Script::REGDISCONNECTED, Script::OPDISCONNECTED };

        if(((cur->ui16Functions & iDisconnectedBits[ui8Type]) == iDisconnectedBits[ui8Type]) == true && (bMoved == false || cur->bProcessed == false)) {
            cur->bProcessed = true;

            lua_pushcfunction(cur->pLUA, ScriptTraceback);
            iTraceback = lua_gettop(cur->pLUA);

            // PPK ... table of disconnected functions
            static const char* DisconnectedFunction[] = { "UserDisconnected", "RegDisconnected", "OpDisconnected" };

            lua_getglobal(cur->pLUA, DisconnectedFunction[ui8Type]);
            iTop = lua_gettop(cur->pLUA);
            if(lua_isfunction(cur->pLUA, iTop) == 0) {
				switch(ui8Type) {
					case 0:
						cur->ui16Functions &= ~Script::USERDISCONNECTED;
						break;
					case 1:
						cur->ui16Functions &= ~Script::REGDISCONNECTED;
						break;
					case 2:
						cur->ui16Functions &= ~Script::OPDISCONNECTED;
						break;
				}

                lua_settop(cur->pLUA, 0);
                continue;
            }

            pActualUser = u;

            lua_checkstack(cur->pLUA, 1); // we need 1 empty slots in stack, check it to be sure

			ScriptPushUser(cur->pLUA, u); // usertable

            // 1 passed parameters, zero returned
			if(lua_pcall(cur->pLUA, 1, 0, iTraceback) != 0) {
                ScriptError(cur);

                lua_settop(cur->pLUA, 0);
                continue;
            }

            pActualUser = NULL;

            // clear the stack for sure
            lua_settop(cur->pLUA, 0);
        }
    }
}
//------------------------------------------------------------------------------

void clsScriptManager::PrepareMove(lua_State * L) {
    if(bMoved == true) {
        return;
    }

    bool bBefore = true;

    bMoved = true;

    Script * cur = NULL,
        * next = pRunningScriptS;
        
    while(next != NULL) {
    	cur = next;
        next = cur->pNext;

        if(bBefore == true) {
            cur->bProcessed = true;
        } else {
            cur->bProcessed = false;
        }

        if(cur->pLUA == L) {
            bBefore = false;
        }
    }
}
//------------------------------------------------------------------------------
