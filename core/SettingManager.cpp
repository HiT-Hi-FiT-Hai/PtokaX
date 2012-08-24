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
#include "SettingXml.h"
#include "SettingDefaults.h"
#include "SettingManager.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "GlobalDataQueue.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "ResNickManager.h"
#include "ServerThread.h"
#include "TextFileManager.h"
#include "UDPThread.h"
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
	#include "../gui.win/MainWindow.h"
#endif
//---------------------------------------------------------------------------
static const char* sMin = "[min]";
static const char* sMax = "[max]";
static const char * sHubSec = "Hub-Security";
//---------------------------------------------------------------------------
SetMan *SettingManager = NULL;
//---------------------------------------------------------------------------

SetMan::SetMan(void) {
    bUpdateLocked = true;

#ifdef _WIN32
    InitializeCriticalSection(&csSetting);
#else
	pthread_mutex_init(&mtxSetting, NULL);
#endif

    sMOTD = NULL;
    ui16MOTDLen = 0;

    ui8FullMyINFOOption = 0;

    ui64MinShare = 0;
    ui64MaxShare = 0;

    bBotsSameNick = false;

	bFirstRun = false;

    for(size_t szi = 0; szi < 25; szi++) {
        iPortNumbers[szi] = 0;
	}

    for(size_t szi = 0; szi < SETPRETXT_IDS_END; szi++) {
        sPreTexts[szi] = NULL;
        ui16PreTextsLens[szi] = 0;
	}

    // Read default bools
    for(size_t szi = 0; szi < SETBOOL_IDS_END; szi++) {
        SetBool(szi, SetBoolDef[szi]);
	}

    // Read default shorts
    for(size_t szi = 0; szi < SETSHORT_IDS_END; szi++) {
        SetShort(szi, SetShortDef[szi]);
	}

    // Read default texts
	for(size_t szi = 0; szi < SETTXT_IDS_END; szi++) {
        sTexts[szi] = NULL;
		ui16TextsLens[szi] = 0;

		SetText(szi, SetTxtDef[szi]);
	}

    // Load settings
	Load();

    //Always enable after startup !
    bBools[SETBOOL_CHECK_IP_IN_COMMANDS] = true;
}
//---------------------------------------------------------------------------

SetMan::~SetMan(void) {
    Save();

    if(sMOTD != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMOTD) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sMOTD in SetMan::~SetMan\n", 0);
        }
#else
		free(sMOTD);
#endif
        sMOTD = NULL;
        ui16MOTDLen = 0;
    }

    for(size_t szi = 0; szi < SETTXT_IDS_END; szi++) {
        if(sTexts[szi] == NULL) {
            continue;
        }

#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sTexts[szi]) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sTexts[%" PRIu64 "] in SetMan::~SetMan\n", (uint64_t)szi);
        }
#else
		free(sTexts[szi]);
#endif
    }

    for(size_t szi = 0; szi < SETPRETXT_IDS_END; szi++) {
        if(sPreTexts[szi] == NULL || (szi == SETPRETXT_HUB_SEC && sPreTexts[szi] == sHubSec)) {
            continue;
        }

#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[szi]) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sPreTexts[%" PRIu64 "] in SetMan::~SetMan\n", (uint64_t)szi);
        }
#else
		free(sPreTexts[szi]);
#endif
    }

#ifdef _WIN32
    DeleteCriticalSection(&csSetting);
#else
	pthread_mutex_destroy(&mtxSetting);
#endif
}
//---------------------------------------------------------------------------

void SetMan::CreateDefaultMOTD() {
#ifdef _WIN32
    sMOTD = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, 18);
#else
	sMOTD = (char *)malloc(18);
#endif
    if(sMOTD == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate 18 bytes for sMOTD in SetMan::CreateDefaultMOTD\n", 0);
        exit(EXIT_FAILURE);
    }
    memcpy(sMOTD, "Welcome to PtokaX", 17);
    sMOTD[17] = '\0';
    ui16MOTDLen = 17;
}
//---------------------------------------------------------------------------

void SetMan::LoadMOTD() {
    if(sMOTD != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMOTD) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sMOTD in SetMan::LoadMOTD\n", 0);
        }
#else
		free(sMOTD);
#endif
        sMOTD = NULL;
        ui16MOTDLen = 0;
    }

#ifdef _WIN32
	FILE *fr = fopen((PATH + "\\cfg\\Motd.txt").c_str(), "rb");
#else
	FILE *fr = fopen((PATH + "/cfg/Motd.txt").c_str(), "rb");
#endif
    if(fr != NULL) {
        fseek(fr, 0, SEEK_END);
        uint32_t ulflen = ftell(fr);
        if(ulflen > 0) {
            fseek(fr, 0, SEEK_SET);
            ui16MOTDLen = (uint16_t)(ulflen < 65024 ? ulflen : 65024);

            // allocate memory for sMOTD
#ifdef _WIN32
            sMOTD = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, ui16MOTDLen+1);
#else
			sMOTD = (char *)malloc(ui16MOTDLen+1);
#endif
            if(sMOTD == NULL) {
				AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sMOTD in SetMan::LoadMOTD\n", (uint64_t)(ui16MOTDLen+1));
                exit(EXIT_FAILURE);
            }

            // read from file to sMOTD, if it failed then free sMOTD and create default one
            if(fread(sMOTD, 1, (size_t)ui16MOTDLen, fr) == (size_t)ui16MOTDLen) {
                sMOTD[ui16MOTDLen] = '\0';
            } else {
#ifdef _WIN32
                if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMOTD) == 0) {
					AppendDebugLog("%s - [MEM] Cannot deallocate sMOTD in SetMan::LoadMOTD\n", 0);
                }
#else
				free(sMOTD);
#endif
                sMOTD = NULL;
                ui16MOTDLen = 0;

                // motd loading failed ? create default one...
                CreateDefaultMOTD();
            }
        }
        fclose(fr);
    } else {
        // no motd to load ? create default one...
        CreateDefaultMOTD();
    }

    CheckMOTD();
}
//---------------------------------------------------------------------------

void SetMan::SaveMOTD() {
#ifdef _WIN32
    FILE *fw = fopen((PATH + "\\cfg\\Motd.txt").c_str(), "wb");
#else
	FILE *fw = fopen((PATH + "/cfg/Motd.txt").c_str(), "wb");
#endif
    if(fw != NULL) {
        if(ui16MOTDLen != 0) {
            fwrite(sMOTD, 1, (size_t)ui16MOTDLen, fw);
        }
        fclose(fw);
    }
}
//---------------------------------------------------------------------------

void SetMan::CheckMOTD() {
    for(uint16_t ui16i = 0; ui16i < ui16MOTDLen; ui16i++) {
        if(sMOTD[ui16i] == '|') {
            sMOTD[ui16i] = '0';
        }
    }
}
//---------------------------------------------------------------------------

void SetMan::Load() {
    bUpdateLocked = true;

    // Load MOTD
    LoadMOTD();

#ifdef _WIN32
    TiXmlDocument doc((PATH + "\\cfg\\Settings.xml").c_str());
#else
	TiXmlDocument doc((PATH + "/cfg/Settings.xml").c_str());
#endif
    if(doc.LoadFile()) {
        TiXmlHandle cfg(&doc);

        TiXmlElement *settings = cfg.FirstChild("PtokaX").Element();
        if(settings == NULL) {
            return;
        }

        // version first run
        const char * sVersion;
        if(settings->ToElement() == NULL || (sVersion = settings->ToElement()->Attribute("Version")) == NULL || 
            strcmp(sVersion, PtokaXVersionString) != 0) {
        	bFirstRun = true;
        } else {
        	bFirstRun = false;
        }

        // Read bools
        TiXmlNode *SettingNode = settings->FirstChild("Booleans");
        if(SettingNode != NULL) {
            TiXmlNode *SettingValue = NULL;
            while((SettingValue = SettingNode->IterateChildren(SettingValue)) != NULL) {
                const char * sName;

                if(SettingValue->ToElement() == NULL || (sName = SettingValue->ToElement()->Attribute("Name")) == NULL) {
                    continue;
                }

                bool bValue = atoi(SettingValue->ToElement()->GetText()) == 0 ? false : true;

                for(size_t szi = 0; szi < SETBOOL_IDS_END; szi++) {
                    if(strcmp(SetBoolXmlStr[szi], sName) == 0) {
                        SetBool(szi, bValue);
                    }
                }
            }
        }

        // Read integers
        SettingNode = settings->FirstChild("Integers");
        if(SettingNode != NULL) {
            TiXmlNode *SettingValue = NULL;
            while((SettingValue = SettingNode->IterateChildren(SettingValue)) != NULL) {
                const char * sName;

                if(SettingValue->ToElement() == NULL || (sName = SettingValue->ToElement()->Attribute("Name")) == NULL) {
                    continue;
                }

                int32_t iValue = atoi(SettingValue->ToElement()->GetText());
                // Check if is valid value
                if(iValue < 0 || iValue > 32767) {
                    continue;
                }

                for(size_t szi = 0; szi < SETSHORT_IDS_END; szi++) {
                    if(strcmp(SetShortXmlStr[szi], sName) == 0) {
                        SetShort(szi, (int16_t)iValue);
                    }
                }
            }
        }

        // Read strings
        SettingNode = settings->FirstChild("Strings");
        if(SettingNode != NULL) {
            TiXmlNode *SettingValue = NULL;
            while((SettingValue = SettingNode->IterateChildren(SettingValue)) != NULL) {
                const char * sName;

                if(SettingValue->ToElement() == NULL || (sName = SettingValue->ToElement()->Attribute("Name")) == NULL) {
                    continue;
                }

                const char * sText = SettingValue->ToElement()->GetText();

                if(sText == NULL) {
                    continue;
                }

                for(size_t szi = 0; szi < SETTXT_IDS_END; szi++) {
                    if(strcmp(SetTxtXmlStr[szi], sName) == 0) {
                        SetText(szi, sText);
                    }
                }
            }
        }
    }

    bUpdateLocked = false;
}
//---------------------------------------------------------------------------

void SetMan::Save() {
    SaveMOTD();

#ifdef _WIN32
    TiXmlDocument doc((PATH + "\\cfg\\Settings.xml").c_str());
#else
	TiXmlDocument doc((PATH + "/cfg/Settings.xml").c_str());
#endif
    doc.InsertEndChild(TiXmlDeclaration("1.0", "windows-1252", "yes"));
    TiXmlElement settings("PtokaX");
    settings.SetAttribute("Version", PtokaXVersionString);

    // Save bools
    TiXmlElement booleans("Booleans");
    for(size_t szi = 0; szi < SETBOOL_IDS_END; szi++) {
        // Don't save setting with default value
        if(bBools[szi] == SetBoolDef[szi] || strlen(SetBoolXmlStr[szi]) == 0) {
            continue;
        }

        TiXmlElement boolean("Bool");
        boolean.SetAttribute("Name", SetBoolXmlStr[szi]);
        boolean.InsertEndChild(TiXmlText(bBools[szi] == 0 ? "0" : "1"));
        booleans.InsertEndChild(boolean);
    }
    settings.InsertEndChild(booleans);

    // Save integers
    TiXmlElement integers("Integers");
    char msg[8];
    for(size_t szi = 0; szi < SETSHORT_IDS_END; szi++) {
        // Don't save setting with default value
        if(iShorts[szi] == SetShortDef[szi] || strlen(SetShortXmlStr[szi]) == 0) {
            continue;
        }

        TiXmlElement integer("Integer");
        integer.SetAttribute("Name", SetShortXmlStr[szi]);
		sprintf(msg, "%hd", iShorts[szi]);
        integer.InsertEndChild(TiXmlText(msg));
        integers.InsertEndChild(integer);
    }
    settings.InsertEndChild(integers);

    // Save strings
    TiXmlElement setstrings("Strings");
    for(size_t szi = 0; szi < SETTXT_IDS_END; szi++) {
        // Don't save setting with default value
        if((sTexts[szi] == NULL && strlen(SetTxtDef[szi]) == 0) ||
            (sTexts[szi] != NULL && strcmp(sTexts[szi], SetTxtDef[szi]) == 0) ||
            strlen(SetTxtXmlStr[szi]) == 0) {
            continue;
        }

        TiXmlElement setstring("String");
        setstring.SetAttribute("Name", SetTxtXmlStr[szi]);
        setstring.InsertEndChild(TiXmlText(sTexts[szi] != NULL ? sTexts[szi] : ""));
        setstrings.InsertEndChild(setstring);
    }
    settings.InsertEndChild(setstrings);
    
    doc.InsertEndChild(settings);

    doc.SaveFile();
}
//---------------------------------------------------------------------------

bool SetMan::GetBool(const size_t &szBoolId) {
#ifdef _WIN32
    EnterCriticalSection(&csSetting);
#else
	pthread_mutex_lock(&mtxSetting);
#endif

    bool bValue = bBools[szBoolId];

#ifdef _WIN32
    LeaveCriticalSection(&csSetting);
#else
	pthread_mutex_unlock(&mtxSetting);
#endif
    
    return bValue;
}
//---------------------------------------------------------------------------
uint16_t SetMan::GetFirstPort() {
#ifdef _WIN32
    EnterCriticalSection(&csSetting);
#else
	pthread_mutex_lock(&mtxSetting);
#endif

    uint16_t iValue = iPortNumbers[0];

#ifdef _WIN32
    LeaveCriticalSection(&csSetting);
#else
	pthread_mutex_unlock(&mtxSetting);
#endif
    
    return iValue;
}
//---------------------------------------------------------------------------

int16_t SetMan::GetShort(const size_t &szShortId) {
#ifdef _WIN32
    EnterCriticalSection(&csSetting);
#else
	pthread_mutex_lock(&mtxSetting);
#endif

    int16_t iValue = iShorts[szShortId];

#ifdef _WIN32
    LeaveCriticalSection(&csSetting);
#else
	pthread_mutex_unlock(&mtxSetting);
#endif
    
    return iValue;
}
//---------------------------------------------------------------------------

void SetMan::GetText(const size_t &szTxtId, char * sMsg) {
#ifdef _WIN32
    EnterCriticalSection(&csSetting);
#else
	pthread_mutex_lock(&mtxSetting);
#endif

    if(sTexts[szTxtId] != NULL) {
        strcat(sMsg, sTexts[szTxtId]);
    }

#ifdef _WIN32
    LeaveCriticalSection(&csSetting);
#else
	pthread_mutex_unlock(&mtxSetting);
#endif
}
//---------------------------------------------------------------------------

void SetMan::SetBool(const size_t &szBoolId, const bool &bValue) {
    if(bBools[szBoolId] == bValue) {
        return;
    }
    
    if(szBoolId == SETBOOL_ANTI_MOGLO) {
#ifdef _WIN32
        EnterCriticalSection(&csSetting);
#else
		pthread_mutex_lock(&mtxSetting);
#endif

        bBools[szBoolId] = bValue;

#ifdef _WIN32
        LeaveCriticalSection(&csSetting);
#else
		pthread_mutex_unlock(&mtxSetting);
#endif

        return;
    }

    bBools[szBoolId] = bValue;

    switch(szBoolId) {
        case SETBOOL_REG_BOT:
            UpdateBotsSameNick();
            if(bValue == false) {
                DisableBot();
            }
            UpdateBot();
            break;
        case SETBOOL_REG_OP_CHAT:
            UpdateBotsSameNick();
            if(bValue == false) {
                DisableOpChat();
            }
            UpdateOpChat();
            break;
        case SETBOOL_USE_BOT_NICK_AS_HUB_SEC:
            UpdateHubSec();
            UpdateMOTD();
            UpdateHubNameWelcome();
            UpdateRegOnlyMessage();
            UpdateShareLimitMessage();
            UpdateSlotsLimitMessage();
            UpdateHubSlotRatioMessage();
            UpdateMaxHubsLimitMessage();
            UpdateNoTagMessage();
            UpdateNickLimitMessage();
            break;
        case SETBOOL_DISABLE_MOTD:
        case SETBOOL_MOTD_AS_PM:
            UpdateMOTD();
            break;
        case SETBOOL_REG_ONLY_REDIR:
            UpdateRegOnlyMessage();
            break;
        case SETBOOL_SHARE_LIMIT_REDIR:
            UpdateShareLimitMessage();
            break;
        case SETBOOL_SLOTS_LIMIT_REDIR:
            UpdateSlotsLimitMessage();
            break;
        case SETBOOL_HUB_SLOT_RATIO_REDIR:
            UpdateHubSlotRatioMessage();
            break;
        case SETBOOL_MAX_HUBS_LIMIT_REDIR:
            UpdateMaxHubsLimitMessage();
            break;
        case SETBOOL_NICK_LIMIT_REDIR:
            UpdateNickLimitMessage();
            break;
        case SETBOOL_ENABLE_TEXT_FILES:
            if(bValue == true && bUpdateLocked == false) {
                TextFileManager->RefreshTextFiles();
            }
            break;
#ifdef _BUILD_GUI
		case SETBOOL_ENABLE_TRAY_ICON:
	        if(bUpdateLocked == false) {
                pMainWindow->UpdateSysTray();
			}

            break;
#endif
        case SETBOOL_AUTO_REG:
            if(bUpdateLocked == false) {
				ServerUpdateAutoRegState();
            }
            break;
        case SETBOOL_ENABLE_SCRIPTING:
            UpdateScripting();
            break;
        default:
            break;
    }
}
//---------------------------------------------------------------------------

void SetMan::SetMOTD(char * sTxt, const size_t &szLen) {
    if(ui16MOTDLen == (uint16_t)szLen &&
        (sMOTD != NULL && strcmp(sMOTD, sTxt) == 0)) {
        return;
    }

    if(szLen == 0) {
        if(sMOTD != NULL) {
#ifdef _WIN32
            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMOTD) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate sMOTD in SetMan::SetMOTD\n", 0);
            }
#else
            free(sMOTD);
#endif
            sMOTD = NULL;
            ui16MOTDLen = 0;
        }
    } else {
        uint16_t ui16OldMOTDLen = ui16MOTDLen;
        char * sOldMOTD = sMOTD;

        ui16MOTDLen = (uint16_t)(szLen < 65024 ? szLen : 65024);

        // (re)allocate memory for sMOTD
#ifdef _WIN32
        if(sMOTD == NULL) {
            sMOTD = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, ui16MOTDLen+1);
        } else {
            sMOTD = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldMOTD, ui16MOTDLen+1);
        }
#else
        sMOTD = (char *)realloc(sOldMOTD, ui16MOTDLen+1);
#endif
        if(sMOTD == NULL) {
            sMOTD = sOldMOTD;
            ui16MOTDLen = ui16OldMOTDLen;

            AppendDebugLog("%s - [MEM] Cannot (re)allocate %" PRIu64 " bytes in SetMan::SetMOTD for sMOTD\n", (uint64_t)(ui16MOTDLen+1));

            return;
        }

        memcpy(sMOTD, sTxt, (size_t)ui16MOTDLen);
        sMOTD[ui16MOTDLen] = '\0';

        CheckMOTD();
    }
}
//---------------------------------------------------------------------------

void SetMan::SetShort(const size_t &szShortId, const int16_t &iValue) {
    if(iValue < 0 || iShorts[szShortId] == iValue) {
        return;
    }

    switch(szShortId) {
        case SETSHORT_MIN_SHARE_LIMIT:
        case SETSHORT_MAX_SHARE_LIMIT:
            if(iValue > 9999) {
                return;
            }
            break;
        case SETSHORT_MIN_SHARE_UNITS:
        case SETSHORT_MAX_SHARE_UNITS:
            if(iValue > 4) {
                return;
            }
            break;
        case SETSHORT_NO_TAG_OPTION:
        case SETSHORT_FULL_MYINFO_OPTION:
        case SETSHORT_GLOBAL_MAIN_CHAT_ACTION:
        case SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE:
            if(iValue > 2) {
                return;
            }
            break;
        case SETSHORT_MAX_USERS:
        case SETSHORT_DEFAULT_TEMP_BAN_TIME:
        case SETSHORT_DEFLOOD_TEMP_BAN_TIME:
        case SETSHORT_SR_MESSAGES:
        case SETSHORT_SR_MESSAGES2:
            if(iValue == 0) {
                return;
            }
            break;
        case SETSHORT_MIN_SLOTS_LIMIT:
        case SETSHORT_MAX_SLOTS_LIMIT:
        case SETSHORT_HUB_SLOT_RATIO_HUBS:
        case SETSHORT_HUB_SLOT_RATIO_SLOTS:
        case SETSHORT_MAX_HUBS_LIMIT:
        case SETSHORT_MAX_CHAT_LINES:
        case SETSHORT_MAX_PM_LINES:
        case SETSHORT_MYINFO_DELAY:
        case SETSHORT_MIN_SEARCH_LEN:
        case SETSHORT_MAX_SEARCH_LEN:
        case SETSHORT_MAX_PM_COUNT_TO_USER:
            if(iValue > 999) {
                return;
            }
            break;
        case SETSHORT_MAIN_CHAT_MESSAGES:
        case SETSHORT_MAIN_CHAT_TIME:
        case SETSHORT_SAME_MAIN_CHAT_MESSAGES:
        case SETSHORT_SAME_MAIN_CHAT_TIME:
        case SETSHORT_PM_MESSAGES:
        case SETSHORT_PM_TIME:
        case SETSHORT_SAME_PM_MESSAGES:
        case SETSHORT_SAME_PM_TIME:
        case SETSHORT_SEARCH_MESSAGES:
        case SETSHORT_SEARCH_TIME:
        case SETSHORT_SAME_SEARCH_MESSAGES:
        case SETSHORT_SAME_SEARCH_TIME:
        case SETSHORT_MYINFO_MESSAGES:
        case SETSHORT_MYINFO_TIME:
        case SETSHORT_GETNICKLIST_MESSAGES:
        case SETSHORT_GETNICKLIST_TIME:
        case SETSHORT_DEFLOOD_WARNING_COUNT:
        case SETSHORT_GLOBAL_MAIN_CHAT_MESSAGES:
        case SETSHORT_GLOBAL_MAIN_CHAT_TIME:
        case SETSHORT_GLOBAL_MAIN_CHAT_TIMEOUT:
        case SETSHORT_BRUTE_FORCE_PASS_PROTECT_TEMP_BAN_TIME:
        case SETSHORT_MAIN_CHAT_MESSAGES2:
        case SETSHORT_MAIN_CHAT_TIME2:
        case SETSHORT_PM_MESSAGES2:
        case SETSHORT_PM_TIME2:
        case SETSHORT_SEARCH_MESSAGES2:
        case SETSHORT_SEARCH_TIME2:
        case SETSHORT_MYINFO_MESSAGES2:
        case SETSHORT_MYINFO_TIME2:
        case SETSHORT_CHAT_INTERVAL_MESSAGES:
        case SETSHORT_CHAT_INTERVAL_TIME:
        case SETSHORT_PM_INTERVAL_MESSAGES:
        case SETSHORT_PM_INTERVAL_TIME:
        case SETSHORT_SEARCH_INTERVAL_MESSAGES:
        case SETSHORT_SEARCH_INTERVAL_TIME:
            if(iValue == 0 || iValue > 999) {
                return;
            }
            break;
        case SETSHORT_CTM_MESSAGES:
        case SETSHORT_CTM_TIME:
        case SETSHORT_CTM_MESSAGES2:
        case SETSHORT_CTM_TIME2:
        case SETSHORT_RCTM_MESSAGES:
        case SETSHORT_RCTM_TIME:
        case SETSHORT_RCTM_MESSAGES2:
        case SETSHORT_RCTM_TIME2:
        case SETSHORT_SR_TIME:
        case SETSHORT_SR_TIME2:
        case SETSHORT_MAX_DOWN_KB:
        case SETSHORT_MAX_DOWN_TIME:
        case SETSHORT_MAX_DOWN_KB2:
        case SETSHORT_MAX_DOWN_TIME2:
            if(iValue == 0 || iValue > 9999) {
                return;
            }
            break;
        case SETSHORT_NEW_CONNECTIONS_COUNT:
        case SETSHORT_NEW_CONNECTIONS_TIME:
            if(iValue == 0 || iValue > 999) {
                return;
            }

#ifdef _WIN32
            EnterCriticalSection(&csSetting);
#else
			pthread_mutex_lock(&mtxSetting);
#endif

            iShorts[szShortId] = iValue;

#ifdef _WIN32
            LeaveCriticalSection(&csSetting);
#else
			pthread_mutex_unlock(&mtxSetting);
#endif
            return;
        case SETSHORT_SAME_MULTI_MAIN_CHAT_MESSAGES:
        case SETSHORT_SAME_MULTI_MAIN_CHAT_LINES:
        case SETSHORT_SAME_MULTI_PM_MESSAGES:
        case SETSHORT_SAME_MULTI_PM_LINES:
            if(iValue < 2 || iValue > 999) {
                return;
            }
            break;
        case SETSHORT_MAIN_CHAT_ACTION:
        case SETSHORT_MAIN_CHAT_ACTION2:
        case SETSHORT_SAME_MAIN_CHAT_ACTION:
        case SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION:
        case SETSHORT_PM_ACTION:
        case SETSHORT_PM_ACTION2:
        case SETSHORT_SAME_PM_ACTION:
        case SETSHORT_SAME_MULTI_PM_ACTION:
        case SETSHORT_SEARCH_ACTION:
        case SETSHORT_SEARCH_ACTION2:
        case SETSHORT_SAME_SEARCH_ACTION:
        case SETSHORT_MYINFO_ACTION:
        case SETSHORT_MYINFO_ACTION2:
        case SETSHORT_GETNICKLIST_ACTION:
        case SETSHORT_CTM_ACTION:
        case SETSHORT_CTM_ACTION2:
        case SETSHORT_RCTM_ACTION:
        case SETSHORT_RCTM_ACTION2:
        case SETSHORT_SR_ACTION:
        case SETSHORT_SR_ACTION2:
        case SETSHORT_MAX_DOWN_ACTION:
        case SETSHORT_MAX_DOWN_ACTION2:
            if(iValue > 6) {
                return;
            }
            break;
        case SETSHORT_DEFLOOD_WARNING_ACTION:
            if(iValue > 3) {
                return;
            }
            break;
        case SETSHORT_MIN_NICK_LEN:
        case SETSHORT_MAX_NICK_LEN:
            if(iValue > 64) {
                return;
            }
            break;
        case SETSHORT_MAX_SIMULTANEOUS_LOGINS:
            if(iValue == 0 || iValue > 1000) {
                return;
            }
            break;
        case SETSHORT_MAX_MYINFO_LEN:
            if(iValue < 64 || iValue > 512) {
                return;
            }
            break;
        case SETSHORT_MAX_CTM_LEN:
        case SETSHORT_MAX_RCTM_LEN:
            if(iValue == 0 || iValue > 512) {
                return;
            }
            break;
        case SETSHORT_MAX_SR_LEN:
            if(iValue == 0 || iValue > 8192) {
                return;
            }
            break;
        case SETSHORT_MAX_CONN_SAME_IP:
        case SETSHORT_MIN_RECONN_TIME:
            if(iValue == 0 || iValue > 256) {
                return;
            }
            break;
        default:
            break;
    }

    iShorts[szShortId] = iValue;

    switch(szShortId) {
        case SETSHORT_MIN_SHARE_LIMIT:
        case SETSHORT_MIN_SHARE_UNITS:
            UpdateMinShare();
            UpdateShareLimitMessage();
            break;
        case SETSHORT_MAX_SHARE_LIMIT:
        case SETSHORT_MAX_SHARE_UNITS:
            UpdateMaxShare();
            UpdateShareLimitMessage();
            break;
        case SETSHORT_MIN_SLOTS_LIMIT:
        case SETSHORT_MAX_SLOTS_LIMIT:
            UpdateSlotsLimitMessage();
            break;
        case SETSHORT_HUB_SLOT_RATIO_HUBS:
        case SETSHORT_HUB_SLOT_RATIO_SLOTS:
            UpdateHubSlotRatioMessage();
            break;
        case SETSHORT_MAX_HUBS_LIMIT:
            UpdateMaxHubsLimitMessage();
            break;
        case SETSHORT_NO_TAG_OPTION:
            UpdateNoTagMessage();
            break;
        case SETSHORT_MIN_NICK_LEN:
        case SETSHORT_MAX_NICK_LEN:
            UpdateNickLimitMessage();
            break;
        default:
            break;
    }
}
//---------------------------------------------------------------------------

void SetMan::SetText(const size_t &szTxtId, char * sTxt) {
    SetText(szTxtId, sTxt, strlen(sTxt));
}
//---------------------------------------------------------------------------

void SetMan::SetText(const size_t &szTxtId, const char * sTxt) {
    SetText(szTxtId, (char *)sTxt, strlen(sTxt));
}
//---------------------------------------------------------------------------

void SetMan::SetText(const size_t &szTxtId, const char * sTxt, const size_t &szLen) {
    if((ui16TextsLens[szTxtId] == (uint16_t)szLen &&
        (sTexts[szTxtId] != NULL && strcmp(sTexts[szTxtId], sTxt) == 0)) ||
        strchr(sTxt, '|') != NULL) {
        return;
    }

    switch(szTxtId) {
        case SETTXT_HUB_NAME:
        case SETTXT_HUB_ADDRESS:
        case SETTXT_REG_ONLY_MSG:
        case SETTXT_SHARE_LIMIT_MSG:
        case SETTXT_SLOTS_LIMIT_MSG:
        case SETTXT_HUB_SLOT_RATIO_MSG:
        case SETTXT_MAX_HUBS_LIMIT_MSG:
        case SETTXT_NO_TAG_MSG:
        case SETTXT_NICK_LIMIT_MSG:
            if(szLen == 0 || szLen > 256) {
                return;
            }
            break;
        case SETTXT_BOT_NICK:
            if(strchr(sTxt, '$') != NULL || strchr(sTxt, ' ') != NULL || szLen == 0 || szLen > 64) {
                return;
            }
            if(ServersS != NULL && bBotsSameNick == false) {
                ResNickManager->DelReservedNick(sTexts[SETTXT_BOT_NICK]);
            }
            if(bBools[SETBOOL_REG_BOT] == true) {
                DisableBot();
            }
            break;
        case SETTXT_OP_CHAT_NICK:
            if(strchr(sTxt, '$') != NULL || strchr(sTxt, ' ') != NULL || szLen == 0 || szLen > 64) {
                return;
            }
            if(ServersS != NULL && bBotsSameNick == false) {
                ResNickManager->DelReservedNick(sTexts[SETTXT_OP_CHAT_NICK]);
            }
            if(bBools[SETBOOL_REG_OP_CHAT] == true) {
                DisableOpChat();
            }
            break;
        case SETTXT_ADMIN_NICK:
            if(strchr(sTxt, '$') != NULL || strchr(sTxt, ' ') != NULL) {
                return;
            }
        case SETTXT_TCP_PORTS:
            if(szLen == 0 || szLen > 64) {
                return;
            }
            break;
        case SETTXT_UDP_PORT:
            if(szLen == 0 || szLen > 5) {
                return;
            }
            UpdateUDPPort();
            break;
        case SETTXT_CHAT_COMMANDS_PREFIXES:
            if(szLen == 0 || szLen > 5) {
                return;
            }
            break;
        case SETTXT_HUB_DESCRIPTION:
        case SETTXT_REDIRECT_ADDRESS:
        case SETTXT_REG_ONLY_REDIR_ADDRESS:
        case SETTXT_HUB_TOPIC:
        case SETTXT_SHARE_LIMIT_REDIR_ADDRESS:
        case SETTXT_SLOTS_LIMIT_REDIR_ADDRESS:
        case SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS:
        case SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS:
        case SETTXT_NO_TAG_REDIR_ADDRESS:
        case SETTXT_TEMP_BAN_REDIR_ADDRESS:
        case SETTXT_PERM_BAN_REDIR_ADDRESS:
        case SETTXT_NICK_LIMIT_REDIR_ADDRESS:
        case SETTXT_MSG_TO_ADD_TO_BAN_MSG:
            if(szLen > 256) {
                return;
            }
            break;
        case SETTXT_REGISTER_SERVERS:
            if(szLen > 1024) {
                return;
            }
            break;
        case SETTXT_BOT_DESCRIPTION:
        case SETTXT_BOT_EMAIL:
            if(strchr(sTxt, '$') != NULL || szLen > 64) {
                return;
            }
            if(bBools[SETBOOL_REG_BOT] == true) {
                DisableBot(false);
            }
            break;
        case SETTXT_OP_CHAT_DESCRIPTION:
        case SETTXT_OP_CHAT_EMAIL:
            if(strchr(sTxt, '$') != NULL || szLen > 64) {
                return;
            }
            if(bBools[SETBOOL_REG_OP_CHAT] == true) {
                DisableOpChat(false);
            }
            break;
        case SETTXT_HUB_OWNER_EMAIL:
            if(szLen > 64) {
                return;
            }
            break;
        case SETTXT_LANGUAGE:
#ifdef _WIN32
            if(szLen != 0 && FileExist((PATH+"\\language\\"+string(sTxt, szLen)+".xml").c_str()) == false) {
#else
			if(szLen != 0 && FileExist((PATH+"/language/"+string(sTxt, szLen)+".xml").c_str()) == false) {
#endif
                return;
            }
            break;
        default:
            if(szLen > 4096) {
                return;
            }
            break;
    }

    if(szTxtId == SETTXT_HUB_NAME || szTxtId == SETTXT_HUB_ADDRESS || szTxtId == SETTXT_HUB_DESCRIPTION) {
#ifdef _WIN32
        EnterCriticalSection(&csSetting);
#else
		pthread_mutex_lock(&mtxSetting);
#endif
    }

    if(szLen == 0) {
        if(sTexts[szTxtId] != NULL) {
#ifdef _WIN32
            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sTexts[szTxtId]) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate sTexts[%" PRIu64 "] in SetMan::SetText\n", (uint64_t)(szTxtId));
            }
#else
            free(sTexts[szTxtId]);
#endif
            sTexts[szTxtId] = NULL;
            ui16TextsLens[szTxtId] = 0;
        }
    } else {
        char * sOldText = sTexts[szTxtId];
#ifdef _WIN32
        if(sTexts[szTxtId] == NULL) {
            sTexts[szTxtId] = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szLen+1);
        } else {
            sTexts[szTxtId] = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldText, szLen+1);
        }
#else
		sTexts[szTxtId] = (char *)realloc(sOldText, szLen+1);
#endif
        if(sTexts[szTxtId] == NULL) {
            sTexts[szTxtId] = sOldText;

			AppendDebugLog("%s - [MEM] Cannot (re)allocate %" PRIu64 " bytes in SetMan::SetText\n", (uint64_t)(szLen+1));

            if(szTxtId == SETTXT_HUB_NAME || szTxtId == SETTXT_HUB_ADDRESS || szTxtId == SETTXT_HUB_DESCRIPTION) {
#ifdef _WIN32
                LeaveCriticalSection(&csSetting);
#else
                pthread_mutex_unlock(&mtxSetting);
#endif
            }

            return;
        }
    
        memcpy(sTexts[szTxtId], sTxt, szLen);
        sTexts[szTxtId][szLen] = '\0';
        ui16TextsLens[szTxtId] = (uint16_t)szLen;
    }

    if(szTxtId == SETTXT_HUB_NAME || szTxtId == SETTXT_HUB_ADDRESS || szTxtId == SETTXT_HUB_DESCRIPTION) {
#ifdef _WIN32
        LeaveCriticalSection(&csSetting);
#else
		pthread_mutex_unlock(&mtxSetting);
#endif
    }

    switch(szTxtId) {
        case SETTXT_BOT_NICK:
            UpdateHubSec();
            UpdateMOTD();
            UpdateHubNameWelcome();
            UpdateRegOnlyMessage();
            UpdateShareLimitMessage();
            UpdateSlotsLimitMessage();
            UpdateHubSlotRatioMessage();
            UpdateMaxHubsLimitMessage();
            UpdateNoTagMessage();
            UpdateNickLimitMessage();
            UpdateBotsSameNick();
            if(ServersS != NULL && bBotsSameNick == false) {
                ResNickManager->AddReservedNick(sTexts[SETTXT_BOT_NICK]);
            }
            UpdateBot();
            break;
        case SETTXT_BOT_DESCRIPTION:
        case SETTXT_BOT_EMAIL:
            UpdateBot(false);
            break;
        case SETTXT_OP_CHAT_NICK:
            UpdateBotsSameNick();
            if(ServersS != NULL && bBotsSameNick == false) {
                ResNickManager->AddReservedNick(sTexts[SETTXT_OP_CHAT_NICK]);
            }
            UpdateOpChat();
            break;
        case SETTXT_HUB_TOPIC:
		case SETTXT_HUB_NAME:
#ifdef _BUILD_GUI
			if(bUpdateLocked == false) {
                pMainWindow->UpdateTitleBar();
			}
#endif
            UpdateHubNameWelcome();
            UpdateHubName();
            break;
        case SETTXT_LANGUAGE:
            UpdateLanguage();
            UpdateHubNameWelcome();
            break;
        case SETTXT_REDIRECT_ADDRESS:
            UpdateRedirectAddress();
            if(bBools[SETBOOL_REG_ONLY_REDIR] == true) {
                UpdateRegOnlyMessage();
            }
            if(bBools[SETBOOL_SHARE_LIMIT_REDIR] == true) {
                UpdateShareLimitMessage();
            }
            if(bBools[SETBOOL_SLOTS_LIMIT_REDIR] == true) {
                UpdateSlotsLimitMessage();
            }
            if(bBools[SETBOOL_HUB_SLOT_RATIO_REDIR] == true) {
                UpdateHubSlotRatioMessage();
            }
            if(bBools[SETBOOL_MAX_HUBS_LIMIT_REDIR] == true) {
                UpdateMaxHubsLimitMessage();
            }
            if(iShorts[SETSHORT_NO_TAG_OPTION] == 2) {
                UpdateNoTagMessage();
            }
            if(sTexts[SETTXT_TEMP_BAN_REDIR_ADDRESS] != NULL) {
                UpdateTempBanRedirAddress();
            }
            if(sTexts[SETTXT_PERM_BAN_REDIR_ADDRESS] != NULL) {
                UpdatePermBanRedirAddress();
            }
            if(bBools[SETBOOL_NICK_LIMIT_REDIR] == true) {
                UpdateNickLimitMessage();
            }
            break;
        case SETTXT_REG_ONLY_MSG:
        case SETTXT_REG_ONLY_REDIR_ADDRESS:
            UpdateRegOnlyMessage();
            break;
        case SETTXT_SHARE_LIMIT_MSG:
        case SETTXT_SHARE_LIMIT_REDIR_ADDRESS:
            UpdateShareLimitMessage();
            break;
        case SETTXT_SLOTS_LIMIT_MSG:
        case SETTXT_SLOTS_LIMIT_REDIR_ADDRESS:
            UpdateSlotsLimitMessage();
            break;
        case SETTXT_HUB_SLOT_RATIO_MSG:
        case SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS:
            UpdateHubSlotRatioMessage();
            break;
        case SETTXT_MAX_HUBS_LIMIT_MSG:
        case SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS:
            UpdateMaxHubsLimitMessage();
            break;
        case SETTXT_NO_TAG_MSG:
        case SETTXT_NO_TAG_REDIR_ADDRESS:
            UpdateNoTagMessage();
            break;
        case SETTXT_TEMP_BAN_REDIR_ADDRESS:
            UpdateTempBanRedirAddress();
            break;
        case SETTXT_PERM_BAN_REDIR_ADDRESS:
            UpdatePermBanRedirAddress();
            break;
        case SETTXT_NICK_LIMIT_MSG:
        case SETTXT_NICK_LIMIT_REDIR_ADDRESS:
            UpdateNickLimitMessage();
            break;
        case SETTXT_TCP_PORTS:
            UpdateTCPPorts();
            break;
        default:
            break;
    }
}
//---------------------------------------------------------------------------

void SetMan::SetText(const size_t &szTxtId, const string & sTxt) {
    SetText(szTxtId, sTxt.c_str(), sTxt.size());
}
//---------------------------------------------------------------------------

void SetMan::UpdateAll() {
	ui8FullMyINFOOption = (uint8_t)iShorts[SETSHORT_FULL_MYINFO_OPTION];

    UpdateHubSec();
    UpdateMOTD();
    UpdateHubNameWelcome();
    UpdateHubName();
    UpdateRedirectAddress();
    UpdateRegOnlyMessage();
    UpdateMinShare();
    UpdateMaxShare();
    UpdateShareLimitMessage();
    UpdateSlotsLimitMessage();
    UpdateHubSlotRatioMessage();
    UpdateMaxHubsLimitMessage();
    UpdateNoTagMessage();
    UpdateTempBanRedirAddress();
    UpdatePermBanRedirAddress();
    UpdateNickLimitMessage();
    UpdateTCPPorts();
    UpdateBotsSameNick();
}
//---------------------------------------------------------------------------

void SetMan::UpdateHubSec() {
    if(bUpdateLocked == true) {
        return;
    }

    if(bBools[SETBOOL_USE_BOT_NICK_AS_HUB_SEC] == true) {
        char * sOldHubSec = sPreTexts[SetMan::SETPRETXT_HUB_SEC];

#ifdef _WIN32
        if(sPreTexts[SetMan::SETPRETXT_HUB_SEC] == NULL || sPreTexts[SetMan::SETPRETXT_HUB_SEC] == sHubSec) {
            sPreTexts[SetMan::SETPRETXT_HUB_SEC] = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, ui16TextsLens[SETTXT_BOT_NICK]+1);
        } else {
            sPreTexts[SetMan::SETPRETXT_HUB_SEC] = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldHubSec, ui16TextsLens[SETTXT_BOT_NICK]+1);
        }
#else
		sPreTexts[SetMan::SETPRETXT_HUB_SEC] = (char *)realloc(sOldHubSec == sHubSec ? NULL : sOldHubSec, ui16TextsLens[SETTXT_BOT_NICK]+1);
#endif
        if(sPreTexts[SetMan::SETPRETXT_HUB_SEC] == NULL) {
            sPreTexts[SetMan::SETPRETXT_HUB_SEC] = sOldHubSec;

			AppendDebugLog("%s - [MEM] Cannot (re)allocate %" PRIu64 " bytes in SetMan::UpdateHubSec\n", (uint64_t)(ui16TextsLens[SETTXT_BOT_NICK]+1));

            return;
        }

        memcpy(sPreTexts[SetMan::SETPRETXT_HUB_SEC], sTexts[SETTXT_BOT_NICK], ui16TextsLens[SETTXT_BOT_NICK]);
        ui16PreTextsLens[SetMan::SETPRETXT_HUB_SEC] = ui16TextsLens[SETTXT_BOT_NICK];
        sPreTexts[SetMan::SETPRETXT_HUB_SEC][ui16PreTextsLens[SetMan::SETPRETXT_HUB_SEC]] = '\0';
    } else {
        if(sPreTexts[SetMan::SETPRETXT_HUB_SEC] != sHubSec) {
#ifdef _WIN32
            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[SetMan::SETPRETXT_HUB_SEC]) == 0) {
				AppendDebugLog("%s - [MEM] Cannot deallocate memory in SetMan::UpdateHubSec\n", 0);
            }
#else
			free(sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
#endif
        }

        sPreTexts[SetMan::SETPRETXT_HUB_SEC] = (char *)sHubSec;
        ui16PreTextsLens[SetMan::SETPRETXT_HUB_SEC] = 12;
    }
}
//---------------------------------------------------------------------------

void SetMan::UpdateMOTD() {
    if(bUpdateLocked == true) {
        return;
    }

    if(bBools[SETBOOL_DISABLE_MOTD] == true || sMOTD == NULL) {
        if(sPreTexts[SETPRETXT_MOTD] != NULL) {
#ifdef _WIN32
            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[SETPRETXT_MOTD]) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate memory in SetMan::UpdateMOTD\n", 0);
            }
#else
            free(sPreTexts[SETPRETXT_MOTD]);
#endif
            sPreTexts[SETPRETXT_MOTD] = NULL;
            ui16PreTextsLens[SETPRETXT_MOTD] = 0;
        }

        return;
    }

    size_t szNeededMem = (bBools[SETBOOL_MOTD_AS_PM] == true ?
        ((2*(ui16PreTextsLens[SetMan::SETPRETXT_HUB_SEC]))+ui16MOTDLen+21) :
        (ui16PreTextsLens[SetMan::SETPRETXT_HUB_SEC]+ui16MOTDLen+5));

    char * sOldMotd = sPreTexts[SETPRETXT_MOTD];

#ifdef _WIN32
    if(sPreTexts[SETPRETXT_MOTD] == NULL) {
        sPreTexts[SETPRETXT_MOTD] = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededMem);
    } else {
        sPreTexts[SETPRETXT_MOTD] = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldMotd, szNeededMem);
    }
#else
	sPreTexts[SETPRETXT_MOTD] = (char *)realloc(sOldMotd, szNeededMem);
#endif
    if(sPreTexts[SETPRETXT_MOTD] == NULL) {
        sPreTexts[SETPRETXT_MOTD] = sOldMotd;

		AppendDebugLog("%s - [MEM] Cannot (re)allocate %" PRIu64 " bytes in SetMan::UpdateMOTD\n", (uint64_t)szNeededMem);

        return;
    }

    int imsgLen = 0;

    if(bBools[SETBOOL_MOTD_AS_PM] == true) {
        imsgLen = sprintf(sPreTexts[SETPRETXT_MOTD], "$To: %%s From: %s $<%s> %s|", sPreTexts[SetMan::SETPRETXT_HUB_SEC], sPreTexts[SetMan::SETPRETXT_HUB_SEC], sMOTD);
    } else {
        imsgLen = sprintf(sPreTexts[SETPRETXT_MOTD], "<%s> %s|", sPreTexts[SetMan::SETPRETXT_HUB_SEC], sMOTD);
    }

    if(CheckSprintf(imsgLen, szNeededMem, "SetMan::UpdateMOTD") == false) {
        exit(EXIT_FAILURE);
    }

    ui16PreTextsLens[SETPRETXT_MOTD] = (uint16_t)imsgLen;
}
//---------------------------------------------------------------------------

void SetMan::UpdateHubNameWelcome() {
    if(bUpdateLocked == true) {
        return;
    }

    size_t szNeededMem = 19 + ui16TextsLens[SETTXT_HUB_NAME] + ui16PreTextsLens[SetMan::SETPRETXT_HUB_SEC] + LanguageManager->ui16TextsLens[LAN_THIS_HUB_IS_RUNNING] + sTitle.size() +
        LanguageManager->ui16TextsLens[LAN_UPTIME];

    if(sTexts[SETTXT_HUB_TOPIC] != NULL) {
        szNeededMem += ui16TextsLens[SETTXT_HUB_TOPIC] + 3;
    }

#ifdef _PtokaX_TESTING_
    szNeededMem += 9 + strlen(BUILD_NUMBER);
#endif

    char * sOldWelcome = sPreTexts[SETPRETXT_HUB_NAME_WLCM];

#ifdef _WIN32
    if(sPreTexts[SETPRETXT_HUB_NAME_WLCM] == NULL) {
        sPreTexts[SETPRETXT_HUB_NAME_WLCM] = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededMem);
    } else {
        sPreTexts[SETPRETXT_HUB_NAME_WLCM] = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldWelcome, szNeededMem);
    }
#else
	sPreTexts[SETPRETXT_HUB_NAME_WLCM] = (char *)realloc(sOldWelcome, szNeededMem);
#endif
    if(sPreTexts[SETPRETXT_HUB_NAME_WLCM] == NULL) {
        sPreTexts[SETPRETXT_HUB_NAME_WLCM] = sOldWelcome;

		AppendDebugLog("%s - [MEM] Cannot (re)allocate %" PRIu64 " bytes in SetMan::UpdateHubNameWelcome\n", (uint64_t)szNeededMem);

        return;
    }

    int iMsgLen = 0;

    if(sTexts[SETTXT_HUB_TOPIC] == NULL) {
        iMsgLen = sprintf(sPreTexts[SETPRETXT_HUB_NAME_WLCM], "$HubName %s|<%s> %s %s"
#ifdef _PtokaX_TESTING_
            " [build " BUILD_NUMBER "]"
#endif
            " (%s: ", sTexts[SETTXT_HUB_NAME], sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_THIS_HUB_IS_RUNNING], sTitle.c_str(), LanguageManager->sTexts[LAN_UPTIME]);
    } else {
        iMsgLen =  sprintf(sPreTexts[SETPRETXT_HUB_NAME_WLCM], "$HubName %s - %s|<%s> %s %s"
#ifdef _PtokaX_TESTING_
            " [build " BUILD_NUMBER "]"
#endif
            " (%s: ", sTexts[SETTXT_HUB_NAME], sTexts[SETTXT_HUB_TOPIC], sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_THIS_HUB_IS_RUNNING], sTitle.c_str(),
            LanguageManager->sTexts[LAN_UPTIME]);
    }
    
    if(CheckSprintf(iMsgLen, szNeededMem, "SetMan::UpdateHubNameWelcome") == false) {
        exit(EXIT_FAILURE);
    }

    ui16PreTextsLens[SETPRETXT_HUB_NAME_WLCM] = (uint16_t)iMsgLen;
}
//---------------------------------------------------------------------------

void SetMan::UpdateHubName() {
    if(bUpdateLocked == true) {
        return;
    }

    size_t szNeededMem = 11 + ui16TextsLens[SETTXT_HUB_NAME];

    if(sTexts[SETTXT_HUB_TOPIC] != NULL) {
        szNeededMem += ui16TextsLens[SETTXT_HUB_TOPIC] + 3;
    }

    char * sOldHubName = sPreTexts[SETPRETXT_HUB_NAME];

#ifdef _WIN32
    if(sPreTexts[SETPRETXT_HUB_NAME] == NULL) {
        sPreTexts[SETPRETXT_HUB_NAME] = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededMem);
    } else {
        sPreTexts[SETPRETXT_HUB_NAME] = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldHubName, szNeededMem);
    }
#else
	sPreTexts[SETPRETXT_HUB_NAME] = (char *)realloc(sOldHubName, szNeededMem);
#endif
    if(sPreTexts[SETPRETXT_HUB_NAME] == NULL) {
        sPreTexts[SETPRETXT_HUB_NAME] = sOldHubName;

		AppendDebugLog("%s - [MEM] Cannot (re)allocate %" PRIu64 " bytes in SetMan::UpdateHubName\n", (uint64_t)szNeededMem);

        return;
    }

	int iMsgLen = 0;

    if(sTexts[SETTXT_HUB_TOPIC] == NULL) {
        iMsgLen = sprintf(sPreTexts[SETPRETXT_HUB_NAME], "$HubName %s|", sTexts[SETTXT_HUB_NAME]);
    } else {
        iMsgLen = sprintf(sPreTexts[SETPRETXT_HUB_NAME], "$HubName %s - %s|", sTexts[SETTXT_HUB_NAME], sTexts[SETTXT_HUB_TOPIC]);
    }

    if(CheckSprintf(iMsgLen, szNeededMem, "SetMan::UpdateHubName") == false) {
        exit(EXIT_FAILURE);
    }

    ui16PreTextsLens[SETPRETXT_HUB_NAME] = (uint16_t)iMsgLen;

	if(bServerRunning == true) {
        g_GlobalDataQueue->AddQueueItem(sPreTexts[SetMan::SETPRETXT_HUB_NAME], ui16PreTextsLens[SetMan::SETPRETXT_HUB_NAME], NULL, 0, GlobalDataQueue::CMD_HUBNAME);
    }
}
//---------------------------------------------------------------------------

void SetMan::UpdateRedirectAddress() {
    if(bUpdateLocked == true) {
        return;
    }

    if(sTexts[SETTXT_REDIRECT_ADDRESS] == NULL) {
        if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
#ifdef _WIN32
            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[SETPRETXT_REDIRECT_ADDRESS]) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate memory in SetMan::UpdateRedirectAddress\n", 0);
            }
#else
            free(sPreTexts[SETPRETXT_REDIRECT_ADDRESS]);
#endif
            sPreTexts[SETPRETXT_REDIRECT_ADDRESS] = NULL;
            ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS] = 0;
        }

        return;
    }

    size_t szNeededLen = 13 + ui16TextsLens[SETTXT_REDIRECT_ADDRESS];

    char * sOldRedirAddr = sPreTexts[SETPRETXT_REDIRECT_ADDRESS];

#ifdef _WIN32
    if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] == NULL) {
        sPreTexts[SETPRETXT_REDIRECT_ADDRESS] = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededLen);
    } else {
        sPreTexts[SETPRETXT_REDIRECT_ADDRESS] = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldRedirAddr, szNeededLen);
    }
#else
	sPreTexts[SETPRETXT_REDIRECT_ADDRESS] = (char *)realloc(sOldRedirAddr, szNeededLen);
#endif
    if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] == NULL) {
        sPreTexts[SETPRETXT_REDIRECT_ADDRESS] = sOldRedirAddr;

		AppendDebugLog("%s - [MEM] Cannot (re)allocate %" PRIu64 " bytes in SetMan::UpdateRedirectAddress\n", (uint64_t)szNeededLen);

        return;
    }

    int iMsgLen = sprintf(sPreTexts[SETPRETXT_REDIRECT_ADDRESS], "$ForceMove %s|", sTexts[SETTXT_REDIRECT_ADDRESS]);
    if(CheckSprintf(iMsgLen, szNeededLen, "SetMan::UpdateRedirectAddress") == false) {
        exit(EXIT_FAILURE);
    }
    ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS] = (uint16_t)iMsgLen;
}
//---------------------------------------------------------------------------

void SetMan::UpdateRegOnlyMessage() {
    if(bUpdateLocked == true) {
        return;
    }

    size_t szNeededMem = 5 + ui16PreTextsLens[SetMan::SETPRETXT_HUB_SEC] + ui16TextsLens[SETTXT_REG_ONLY_MSG];

    if(bBools[SETBOOL_REG_ONLY_REDIR] == true) {
        if(sTexts[SETTXT_REG_ONLY_REDIR_ADDRESS] != NULL) {
            szNeededMem += 12 + ui16TextsLens[SETTXT_REG_ONLY_REDIR_ADDRESS];
        } else {
            szNeededMem += ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

    char * sOldRegOnlyMsg = sPreTexts[SETPRETXT_REG_ONLY_MSG];

#ifdef _WIN32
    if(sPreTexts[SETPRETXT_REG_ONLY_MSG] == NULL) {
        sPreTexts[SETPRETXT_REG_ONLY_MSG] = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededMem);
    } else {
        sPreTexts[SETPRETXT_REG_ONLY_MSG] = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldRegOnlyMsg, szNeededMem);
    }
#else
	sPreTexts[SETPRETXT_REG_ONLY_MSG] = (char *)realloc(sOldRegOnlyMsg, szNeededMem);
#endif
    if(sPreTexts[SETPRETXT_REG_ONLY_MSG] == NULL) {
        sPreTexts[SETPRETXT_REG_ONLY_MSG] = sOldRegOnlyMsg;

		AppendDebugLog("%s - [MEM] Cannot (re)allocate %" PRIu64 " bytes in SetMan::UpdateRegOnlyMessage\n", (uint64_t)szNeededMem);

        return;
    }

    int iMsgLen = sprintf(sPreTexts[SETPRETXT_REG_ONLY_MSG], "<%s> %s|", sPreTexts[SetMan::SETPRETXT_HUB_SEC], sTexts[SETTXT_REG_ONLY_MSG]);
    if(CheckSprintf(iMsgLen, szNeededMem, "SetMan::UpdateRegOnlyMessage") == false) {
        exit(EXIT_FAILURE);
    }

    if(bBools[SETBOOL_REG_ONLY_REDIR] == true) {
        if(sTexts[SETTXT_REG_ONLY_REDIR_ADDRESS] != NULL) {
            int iRet = sprintf(sPreTexts[SETPRETXT_REG_ONLY_MSG]+iMsgLen, "$ForceMove %s|", sTexts[SETTXT_REG_ONLY_REDIR_ADDRESS]);
            iMsgLen += iRet;
            if(CheckSprintf1(iRet, iMsgLen, szNeededMem, "SetMan::UpdateRegOnlyMessage1") == false) {
                exit(EXIT_FAILURE);
            }
        } else if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	memcpy(sPreTexts[SETPRETXT_REG_ONLY_MSG]+iMsgLen, sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
            iMsgLen += (int)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

    ui16PreTextsLens[SETPRETXT_REG_ONLY_MSG] = (uint16_t)iMsgLen;
    sPreTexts[SETPRETXT_REG_ONLY_MSG][iMsgLen] = '\0';
}
//---------------------------------------------------------------------------

void SetMan::UpdateShareLimitMessage() {
    if(bUpdateLocked == true) {
        return;
    }

    size_t szNeededMem = 9 + ui16PreTextsLens[SetMan::SETPRETXT_HUB_SEC] + ui16TextsLens[SETTXT_SHARE_LIMIT_MSG];

    if(bBools[SETBOOL_SHARE_LIMIT_REDIR] == true) {
        if(sTexts[SETTXT_SHARE_LIMIT_REDIR_ADDRESS] != NULL) {
            szNeededMem += 12 + ui16TextsLens[SETTXT_SHARE_LIMIT_REDIR_ADDRESS];
        }  else if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	szNeededMem += ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

    char * sOldShareLimitMsg = sPreTexts[SETPRETXT_SHARE_LIMIT_MSG];

#ifdef _WIN32
    if(sPreTexts[SETPRETXT_SHARE_LIMIT_MSG] == NULL) {
        sPreTexts[SETPRETXT_SHARE_LIMIT_MSG] = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededMem);
    } else {
        sPreTexts[SETPRETXT_SHARE_LIMIT_MSG] = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldShareLimitMsg, szNeededMem);
    }
#else
	sPreTexts[SETPRETXT_SHARE_LIMIT_MSG] = (char *)realloc(sOldShareLimitMsg, szNeededMem);
#endif
    if(sPreTexts[SETPRETXT_SHARE_LIMIT_MSG] == NULL) {
        sPreTexts[SETPRETXT_SHARE_LIMIT_MSG] = sOldShareLimitMsg;

		AppendDebugLog("%s - [MEM] Cannot (re)allocate %" PRIu64 " bytes in SetMan::UpdateShareLimitMessage\n", (uint64_t)szNeededMem);

        return;
    }

    int iMsgLen = sprintf(sPreTexts[SETPRETXT_SHARE_LIMIT_MSG], "<%s> ", sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
    if(CheckSprintf(iMsgLen, szNeededMem, "SetMan::UpdateShareLimitMessage0") == false) {
        exit(EXIT_FAILURE);
    }
    
    static const char* units[] = { "B", "kB", "MB", "GB", "TB", "PB", "EB", " ", " ", " ", " ", " ", " ", " ", " ", " " };

    for(uint16_t ui16i = 0; ui16i < ui16TextsLens[SETTXT_SHARE_LIMIT_MSG]; ui16i++) {
        if(sTexts[SETTXT_SHARE_LIMIT_MSG][ui16i] == '%') {
            if(strncmp(sTexts[SETTXT_SHARE_LIMIT_MSG]+ui16i+1, sMin, 5) == 0) {
                if(ui64MinShare != 0) {
                    int iRet = sprintf(sPreTexts[SETPRETXT_SHARE_LIMIT_MSG]+iMsgLen, "%hd %s", iShorts[SETSHORT_MIN_SHARE_LIMIT], units[iShorts[SETSHORT_MIN_SHARE_UNITS]]);
                    iMsgLen += iRet;
                    if(CheckSprintf1(iRet, iMsgLen, szNeededMem, "SetMan::UpdateShareLimitMessage") == false) {
                        exit(EXIT_FAILURE);
                    }
                } else {
                    memcpy(sPreTexts[SETPRETXT_SHARE_LIMIT_MSG]+iMsgLen, "0 B", 3);
                    iMsgLen += 3;
                }
                ui16i += (uint16_t)5;
                continue;
            } else if(strncmp(sTexts[SETTXT_SHARE_LIMIT_MSG]+ui16i+1, sMax, 5) == 0) {
                if(ui64MaxShare != 0) {
                    int iRet = sprintf(sPreTexts[SETPRETXT_SHARE_LIMIT_MSG]+iMsgLen, "%hd %s", iShorts[SETSHORT_MAX_SHARE_LIMIT], units[iShorts[SETSHORT_MAX_SHARE_UNITS]]);
                    iMsgLen += iRet;
                    if(CheckSprintf1(iRet, iMsgLen, szNeededMem, "SetMan::UpdateShareLimitMessage1") == false) {
                        exit(EXIT_FAILURE);
                    }
                } else {
                    memcpy(sPreTexts[SETPRETXT_SHARE_LIMIT_MSG]+iMsgLen, "unlimited", 9);
                    iMsgLen += 9;
                }
                ui16i += (uint16_t)5;
                continue;
            }
        }

        sPreTexts[SETPRETXT_SHARE_LIMIT_MSG][iMsgLen] = sTexts[SETTXT_SHARE_LIMIT_MSG][ui16i];
        iMsgLen++;
    }

    sPreTexts[SETPRETXT_SHARE_LIMIT_MSG][iMsgLen] = '|';
    iMsgLen++;

    if(bBools[SETBOOL_SHARE_LIMIT_REDIR] == true) {
        if(sTexts[SETTXT_SHARE_LIMIT_REDIR_ADDRESS] != NULL) {
        	int iRet = sprintf(sPreTexts[SETPRETXT_SHARE_LIMIT_MSG]+iMsgLen, "$ForceMove %s|", sTexts[SETTXT_SHARE_LIMIT_REDIR_ADDRESS]);
        	iMsgLen += iRet;
        	if(CheckSprintf1(iRet, iMsgLen, szNeededMem, "SetMan::UpdateShareLimitMessage6") == false) {
                exit(EXIT_FAILURE);
            }
        }  else if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	memcpy(sPreTexts[SETPRETXT_SHARE_LIMIT_MSG]+iMsgLen, sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
            iMsgLen += (int)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

    ui16PreTextsLens[SETPRETXT_SHARE_LIMIT_MSG] = (uint16_t)iMsgLen;
    sPreTexts[SETPRETXT_SHARE_LIMIT_MSG][iMsgLen] = '\0';
}
//---------------------------------------------------------------------------

void SetMan::UpdateSlotsLimitMessage() {
    if(bUpdateLocked == true) {
        return;
    }

    size_t szNeededMem = 8 + ui16PreTextsLens[SetMan::SETPRETXT_HUB_SEC] + ui16TextsLens[SETTXT_SLOTS_LIMIT_MSG];

    if(bBools[SETBOOL_SLOTS_LIMIT_REDIR] == true) {
        if(sTexts[SETTXT_SLOTS_LIMIT_REDIR_ADDRESS] != NULL) {
            szNeededMem += 12 + ui16TextsLens[SETTXT_SLOTS_LIMIT_REDIR_ADDRESS];
        }  else if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	szNeededMem += ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

    char * sOldSlotsLimitMsg = sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG];

#ifdef _WIN32
    if(sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG] == NULL) {
        sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG] = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededMem);
    } else {
        sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG] = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldSlotsLimitMsg, szNeededMem);
    }
#else
	sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG] = (char *)realloc(sOldSlotsLimitMsg, szNeededMem);
#endif
    if(sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG] == NULL) {
        sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG] = sOldSlotsLimitMsg;

		AppendDebugLog("%s - [MEM] Cannot (re)allocate %" PRIu64 " bytes in SetMan::UpdateSlotsLimitMessage\n", (uint64_t)szNeededMem);

        return;
    }

    int iMsgLen = sprintf(sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG], "<%s> ", sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
    if(CheckSprintf(iMsgLen, szNeededMem, "SetMan::UpdateSlotsLimitMessage0") == false) {
        exit(EXIT_FAILURE);
    }

    for(uint16_t ui16i = 0; ui16i < ui16TextsLens[SETTXT_SLOTS_LIMIT_MSG]; ui16i++) {
        if(sTexts[SETTXT_SLOTS_LIMIT_MSG][ui16i] == '%') {
            if(strncmp(sTexts[SETTXT_SLOTS_LIMIT_MSG]+ui16i+1, sMin, 5) == 0) {
                int iRet = sprintf(sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG]+iMsgLen, "%hd", iShorts[SETSHORT_MIN_SLOTS_LIMIT]);
                iMsgLen += iRet;
                if(CheckSprintf1(iRet, iMsgLen, szNeededMem, "SetMan::UpdateSlotsLimitMessage") == false) {
                    exit(EXIT_FAILURE);
                }

                ui16i += (uint16_t)5;
                continue;
            } else if(strncmp(sTexts[SETTXT_SLOTS_LIMIT_MSG]+ui16i+1, sMax, 5) == 0) {
                if(iShorts[SETSHORT_MAX_SLOTS_LIMIT] != 0) {
                    int iRet = sprintf(sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG]+iMsgLen, "%hd", iShorts[SETSHORT_MAX_SLOTS_LIMIT]);
                    iMsgLen += iRet;
                    if(CheckSprintf1(iRet, iMsgLen, szNeededMem, "SetMan::UpdateSlotsLimitMessage1") == false) {
                        exit(EXIT_FAILURE);
                    }
                } else {
                    memcpy(sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG]+iMsgLen, "unlimited", 9);
                    iMsgLen += 9;
                }
                ui16i += (uint16_t)5;
                continue;
            }
        }

        sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG][iMsgLen] = sTexts[SETTXT_SLOTS_LIMIT_MSG][ui16i];
        iMsgLen++;
    }

    sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG][iMsgLen] = '|';
    iMsgLen++;

    if(bBools[SETBOOL_SLOTS_LIMIT_REDIR] == true) {
        if(sTexts[SETTXT_SLOTS_LIMIT_REDIR_ADDRESS] != NULL) {
        	int iRet = sprintf(sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG]+iMsgLen, "$ForceMove %s|", sTexts[SETTXT_SLOTS_LIMIT_REDIR_ADDRESS]);
            iMsgLen += iRet;
            if(CheckSprintf1(iRet, iMsgLen, szNeededMem, "SetMan::UpdateSlotsLimitMessage2") == false) {
                exit(EXIT_FAILURE);
            }
        }  else if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	memcpy(sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG]+iMsgLen, sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
            iMsgLen += (int)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

    ui16PreTextsLens[SETPRETXT_SLOTS_LIMIT_MSG] = (uint16_t)iMsgLen;
    sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG][iMsgLen] = '\0';
}
//---------------------------------------------------------------------------

void SetMan::UpdateHubSlotRatioMessage() {
    if(bUpdateLocked == true) {
        return;
    }

    size_t szNeededMem = 5 + ui16PreTextsLens[SetMan::SETPRETXT_HUB_SEC] + ui16TextsLens[SETTXT_HUB_SLOT_RATIO_MSG];

    if(bBools[SETBOOL_HUB_SLOT_RATIO_REDIR] == true) {
        if(sTexts[SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS] != NULL) {
            szNeededMem += 12 + ui16TextsLens[SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS];
        }  else if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	szNeededMem += ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

    char * sOldHubSlotLimitMsg = sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG];

#ifdef _WIN32
    if(sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG] == NULL) {
        sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG] = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededMem);
    } else {
        sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG] = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldHubSlotLimitMsg, szNeededMem);
    }
#else
	sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG] = (char *)realloc(sOldHubSlotLimitMsg, szNeededMem);
#endif
    if(sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG] == NULL) {
        sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG] = sOldHubSlotLimitMsg;

		AppendDebugLog("%s - [MEM] Cannot (re)allocate %" PRIu64 " bytes in SetMan::UpdateHubSlotRatioMessage\n", (uint64_t)szNeededMem);

        return;
    }

    int iMsgLen = sprintf(sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG], "<%s> ", sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
    if(CheckSprintf(iMsgLen, szNeededMem, "SetMan::UpdateHubSlotRatioMessage0") == false) {
        exit(EXIT_FAILURE);
    }

    static const char * sHubs = "[hubs]";
    static const char * sSlots = "[slots]";

    for(uint16_t ui16i = 0; ui16i < ui16TextsLens[SETTXT_HUB_SLOT_RATIO_MSG]; ui16i++) {
        if(sTexts[SETTXT_HUB_SLOT_RATIO_MSG][ui16i] == '%') {
            if(strncmp(sTexts[SETTXT_HUB_SLOT_RATIO_MSG]+ui16i+1, sHubs, 6) == 0) {
                int iRet = sprintf(sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG]+iMsgLen, "%hd", iShorts[SETSHORT_HUB_SLOT_RATIO_HUBS]);
                iMsgLen += iRet;
                if(CheckSprintf1(iRet, iMsgLen, szNeededMem, "SetMan::UpdateHubSlotRatioMessage") == false) {
                    exit(EXIT_FAILURE);
                }
                ui16i += (uint16_t)6;
                continue;
            } else if(strncmp(sTexts[SETTXT_HUB_SLOT_RATIO_MSG]+ui16i+1, sSlots, 7) == 0) {
                int iRet = sprintf(sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG]+iMsgLen, "%hd", iShorts[SETSHORT_HUB_SLOT_RATIO_SLOTS]);
                iMsgLen += iRet;
                if(CheckSprintf1(iRet, iMsgLen, szNeededMem, "SetMan::UpdateHubSlotRatioMessage1") == false) {
                    exit(EXIT_FAILURE);
                }
                ui16i += (uint16_t)7;
                continue;
            }
        }

        sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG][iMsgLen] = sTexts[SETTXT_HUB_SLOT_RATIO_MSG][ui16i];
        iMsgLen++;
    }

    sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG][iMsgLen] = '|';
    iMsgLen++;

    if(bBools[SETBOOL_HUB_SLOT_RATIO_REDIR] == true) {
        if(sTexts[SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS] != NULL) {
        	int iRet = sprintf(sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG]+iMsgLen, "$ForceMove %s|", sTexts[SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS]);
            iMsgLen += iRet;
            if(CheckSprintf1(iRet, iMsgLen, szNeededMem, "SetMan::UpdateHubSlotRatioMessage2") == false) {
                exit(EXIT_FAILURE);
            }
        }  else if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	memcpy(sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG]+iMsgLen, sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
            iMsgLen += (int)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

    ui16PreTextsLens[SETPRETXT_HUB_SLOT_RATIO_MSG] = (uint16_t)iMsgLen;
    sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG][iMsgLen] = '\0';
}
//---------------------------------------------------------------------------

void SetMan::UpdateMaxHubsLimitMessage() {
    if(bUpdateLocked == true) {
        return;
    }

    size_t szNeededMem = 5 + ui16PreTextsLens[SetMan::SETPRETXT_HUB_SEC] + ui16TextsLens[SETTXT_MAX_HUBS_LIMIT_MSG];

    if(bBools[SETBOOL_MAX_HUBS_LIMIT_REDIR] == true) {
        if(sTexts[SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS] != NULL) {
            szNeededMem += 12 + ui16TextsLens[SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS];
        }  else if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	szNeededMem += ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

    char * sOldHubLimitMsg = sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG];

#ifdef _WIN32
    if(sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG] == NULL) {
        sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG] = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededMem);
    } else {
        sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG] = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldHubLimitMsg, szNeededMem);
    }
#else
	sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG] = (char *)realloc(sOldHubLimitMsg, szNeededMem);
#endif
    if(sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG] == NULL) {
        sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG] = sOldHubLimitMsg;

		AppendDebugLog("%s - [MEM] Cannot (re)allocate %" PRIu64 " bytes in SetMan::UpdateMaxHubsLimitMessage\n", (uint64_t)szNeededMem);

        return;
    }

    int iMsgLen = sprintf(sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG], "<%s> ", sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
    if(CheckSprintf(iMsgLen, szNeededMem, "SetMan::UpdateMaxHubsLimitMessage") == false) {
        exit(EXIT_FAILURE);
    }

    static const char* sHubs = "%[hubs]";

    char * sMatch = strstr(sTexts[SETTXT_MAX_HUBS_LIMIT_MSG], sHubs);

    if(sMatch != NULL) {
        if(sMatch > sTexts[SETTXT_MAX_HUBS_LIMIT_MSG]) {
            size_t szLen = sMatch-sTexts[SETTXT_MAX_HUBS_LIMIT_MSG];
            memcpy(sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG]+iMsgLen, sTexts[SETTXT_MAX_HUBS_LIMIT_MSG], szLen);
            iMsgLen += (int)szLen;
        }

        int iRet = sprintf(sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG]+iMsgLen, "%hd", iShorts[SETSHORT_MAX_HUBS_LIMIT]);
        iMsgLen += iRet;
        if(CheckSprintf1(iRet, iMsgLen, szNeededMem, "SetMan::UpdateMaxHubsLimitMessage") == false) {
            exit(EXIT_FAILURE);
        }

        if(sMatch+7 < sTexts[SETTXT_MAX_HUBS_LIMIT_MSG]+ui16TextsLens[SETTXT_MAX_HUBS_LIMIT_MSG]) {
            size_t szLen = (sTexts[SETTXT_MAX_HUBS_LIMIT_MSG]+ui16TextsLens[SETTXT_MAX_HUBS_LIMIT_MSG])-(sMatch+7);
            memcpy(sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG]+iMsgLen, sMatch+7, szLen);
            iMsgLen += (int)szLen;
        }
    } else {
        memcpy(sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG], sTexts[SETTXT_MAX_HUBS_LIMIT_MSG], ui16TextsLens[SETTXT_MAX_HUBS_LIMIT_MSG]);
        iMsgLen = (int)ui16TextsLens[SETTXT_MAX_HUBS_LIMIT_MSG];
    }

    sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG][iMsgLen] = '|';
    iMsgLen++;

    if(bBools[SETBOOL_MAX_HUBS_LIMIT_REDIR] == true) {
        if(sTexts[SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS] != NULL) {
        	int iRet = sprintf(sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG]+iMsgLen, "$ForceMove %s|", sTexts[SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS]);
            iMsgLen += iRet;
            if(CheckSprintf1(iRet, iMsgLen, szNeededMem, "SetMan::UpdateMaxHubsLimitMessage1") == false) {
                exit(EXIT_FAILURE);
            }
        }  else if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	memcpy(sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG]+iMsgLen, sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
            iMsgLen += (int)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

    ui16PreTextsLens[SETPRETXT_MAX_HUBS_LIMIT_MSG] = (uint16_t)iMsgLen;
    sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG][iMsgLen] = '\0';
}
//---------------------------------------------------------------------------

void SetMan::UpdateNoTagMessage() {
    if(bUpdateLocked == true) {
        return;
    }

    if(iShorts[SETSHORT_NO_TAG_OPTION] == 0) {
        if(sPreTexts[SETPRETXT_NO_TAG_MSG] != NULL) {
#ifdef _WIN32
            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[SETPRETXT_NO_TAG_MSG]) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate memory in SetMan::UpdateNoTagMessage\n", 0);
            }
#else
            free(sPreTexts[SETPRETXT_NO_TAG_MSG]);
#endif
            sPreTexts[SETPRETXT_NO_TAG_MSG] = NULL;
            ui16PreTextsLens[SETPRETXT_NO_TAG_MSG] = 0;
        }

        return;
    }

    size_t szNeededMem = 5 + ui16PreTextsLens[SetMan::SETPRETXT_HUB_SEC] + ui16TextsLens[SETTXT_NO_TAG_MSG];

    if(iShorts[SETSHORT_NO_TAG_OPTION] == 2) {
        if(sTexts[SETTXT_NO_TAG_REDIR_ADDRESS] != NULL) {
            szNeededMem += 12 + ui16TextsLens[SETTXT_NO_TAG_REDIR_ADDRESS];
        }  else if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	szNeededMem += ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

    char * sOldNoTagMsg = sPreTexts[SETPRETXT_NO_TAG_MSG];

#ifdef _WIN32
    if(sPreTexts[SETPRETXT_NO_TAG_MSG] == NULL) {
        sPreTexts[SETPRETXT_NO_TAG_MSG] = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededMem);
    } else {
        sPreTexts[SETPRETXT_NO_TAG_MSG] = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldNoTagMsg, szNeededMem);
    }
#else
	sPreTexts[SETPRETXT_NO_TAG_MSG] = (char *)realloc(sOldNoTagMsg, szNeededMem);
#endif
    if(sPreTexts[SETPRETXT_NO_TAG_MSG] == NULL) {
        sPreTexts[SETPRETXT_NO_TAG_MSG] = sOldNoTagMsg;

		AppendDebugLog("%s - [MEM] Cannot (re)allocate %" PRIu64 " bytes in SetMan::UpdateNoTagMessage\n", (uint64_t)szNeededMem);

        return;
    }

    int iMsgLen = sprintf(sPreTexts[SETPRETXT_NO_TAG_MSG], "<%s> %s|", sPreTexts[SetMan::SETPRETXT_HUB_SEC], sTexts[SETTXT_NO_TAG_MSG]);
    if(CheckSprintf(iMsgLen, szNeededMem, "SetMan::UpdateNoTagMessage") == false) {
        exit(EXIT_FAILURE);
    }

    if(iShorts[SETSHORT_NO_TAG_OPTION] == 2) { 
        if(sTexts[SETTXT_NO_TAG_REDIR_ADDRESS] != NULL) {
           	int iRet = sprintf(sPreTexts[SETPRETXT_NO_TAG_MSG]+iMsgLen, "$ForceMove %s|", sTexts[SETTXT_NO_TAG_REDIR_ADDRESS]);
            iMsgLen += iRet;
            if(CheckSprintf1(iRet, iMsgLen, szNeededMem, "SetMan::UpdateNoTagMessage1") == false) {
                exit(EXIT_FAILURE);
            }
        }  else if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	memcpy(sPreTexts[SETPRETXT_NO_TAG_MSG]+iMsgLen, sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
            iMsgLen += (int)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

    ui16PreTextsLens[SETPRETXT_NO_TAG_MSG] = (uint16_t)iMsgLen;
    sPreTexts[SETPRETXT_NO_TAG_MSG][iMsgLen] = '\0';
}
//---------------------------------------------------------------------------

void SetMan::UpdateTempBanRedirAddress() {
    if(bUpdateLocked == true) {
        return;
    }

    size_t szNeededMem = 1;

    if(sTexts[SETTXT_TEMP_BAN_REDIR_ADDRESS] != NULL) {
        szNeededMem += 12 + ui16TextsLens[SETTXT_TEMP_BAN_REDIR_ADDRESS];
    }  else if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		szNeededMem += ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
    } else {
        if(sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] != NULL) {
#ifdef _WIN32
            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS]) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate memory in SetMan::UpdateTempBanRedirAddress\n", 0);
            }
#else
            free(sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS]);
#endif
            sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] = NULL;
            ui16PreTextsLens[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] = 0;
        }

        return;
    }

    char * sOldTempBanRedirMsg = sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS];

#ifdef _WIN32
    if(sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] == NULL) {
        sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededMem);
    } else {
        sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldTempBanRedirMsg, szNeededMem);
    }
#else
	sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] = (char *)realloc(sOldTempBanRedirMsg, szNeededMem);
#endif
    if(sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] == NULL) {
        sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] = sOldTempBanRedirMsg;

		AppendDebugLog("%s - [MEM] Cannot (re)allocate %" PRIu64 " bytes in SetMan::UpdateTempBanRedirAddress\n", (uint64_t)szNeededMem);

        return;
    }

    int iMsgLen = 0;

    if(sTexts[SETTXT_TEMP_BAN_REDIR_ADDRESS] != NULL) {
        iMsgLen = sprintf(sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS], "$ForceMove %s|", sTexts[SETTXT_TEMP_BAN_REDIR_ADDRESS]);
        if(CheckSprintf(iMsgLen, szNeededMem, "SetMan::UpdateTempBanRedirAddress") == false) {
            exit(EXIT_FAILURE);
        }
    } else if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		memcpy(sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS], sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
        iMsgLen = (int)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
    }

    ui16PreTextsLens[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] = (uint16_t)iMsgLen;
    sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS][iMsgLen] = '\0';
}
//---------------------------------------------------------------------------

void SetMan::UpdatePermBanRedirAddress() {
    if(bUpdateLocked == true) {
        return;
    }

    size_t szNeededMem = 1;

    if(sTexts[SETTXT_PERM_BAN_REDIR_ADDRESS] != NULL) {
        szNeededMem += 12 + ui16TextsLens[SETTXT_PERM_BAN_REDIR_ADDRESS];
    }  else if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		szNeededMem += ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
    } else {
        if(sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS] != NULL) {
#ifdef _WIN32
            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS]) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate memory in SetMan::UpdatePermBanRedirAddress\n", 0);
            }
#else
            free(sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS]);
#endif
            sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS] = NULL;
            ui16PreTextsLens[SETPRETXT_PERM_BAN_REDIR_ADDRESS] = 0;
        }

        return;
    }

    char * sOldPermBanRedirMsg = sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS];

#ifdef _WIN32
    if(sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS] == NULL) {
        sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS] = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededMem);
    } else {
        sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS] = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldPermBanRedirMsg, szNeededMem);
    }
#else
	sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS] = (char *)realloc(sOldPermBanRedirMsg, szNeededMem);
#endif
    if(sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS] == NULL) {
        sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS] = sOldPermBanRedirMsg;

		AppendDebugLog("%s - [MEM] Cannot (re)allocate %" PRIu64 " bytes in SetMan::UpdatePermBanRedirAddress\n", (uint64_t)szNeededMem);

        return;
    }

    int iMsgLen = 0;

    if(sTexts[SETTXT_PERM_BAN_REDIR_ADDRESS] != NULL) {
        iMsgLen = sprintf(sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS], "$ForceMove %s|", sTexts[SETTXT_PERM_BAN_REDIR_ADDRESS]);
        if(CheckSprintf(iMsgLen, szNeededMem, "SetMan::UpdatePermBanRedirAddress") == false) {
            exit(EXIT_FAILURE);
        }
    } else if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		memcpy(sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS], sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
        iMsgLen = (int)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
    }

    ui16PreTextsLens[SETPRETXT_PERM_BAN_REDIR_ADDRESS] = (uint16_t)iMsgLen;
    sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS][iMsgLen] = '\0';
}
//---------------------------------------------------------------------------

void SetMan::UpdateNickLimitMessage() {
    if(bUpdateLocked == true) {
        return;
    }

    size_t szNeededMem = 8 + ui16PreTextsLens[SetMan::SETPRETXT_HUB_SEC] + ui16TextsLens[SETTXT_NICK_LIMIT_MSG];

    if(bBools[SETBOOL_NICK_LIMIT_REDIR] == true) {
        if(sTexts[SETTXT_NICK_LIMIT_REDIR_ADDRESS] != NULL) {
            szNeededMem += 12 + ui16TextsLens[SETTXT_NICK_LIMIT_REDIR_ADDRESS];
        }  else if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	szNeededMem += ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

    char * sOldNickLimitMsg = sPreTexts[SETPRETXT_NICK_LIMIT_MSG];

#ifdef _WIN32
    if(sPreTexts[SETPRETXT_NICK_LIMIT_MSG] == NULL) {
        sPreTexts[SETPRETXT_NICK_LIMIT_MSG] = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededMem);
    } else {
        sPreTexts[SETPRETXT_NICK_LIMIT_MSG] = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldNickLimitMsg, szNeededMem);
    }
#else
	sPreTexts[SETPRETXT_NICK_LIMIT_MSG] = (char *)realloc(sOldNickLimitMsg, szNeededMem);
#endif
    if(sPreTexts[SETPRETXT_NICK_LIMIT_MSG] == NULL) {
        sPreTexts[SETPRETXT_NICK_LIMIT_MSG] = sOldNickLimitMsg;

		AppendDebugLog("%s - [MEM] Cannot (re)allocate %" PRIu64 " bytes in SetMan::UpdateNickLimitMessage\n", (uint64_t)szNeededMem);

        return;
    }

    int iMsgLen = sprintf(sPreTexts[SETPRETXT_NICK_LIMIT_MSG], "<%s> ", sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
    if(CheckSprintf(iMsgLen, szNeededMem, "SetMan::UpdateNickLimitMessage0") == false) {
        exit(EXIT_FAILURE);
    }

    for(uint16_t ui16i = 0; ui16i < ui16TextsLens[SETTXT_NICK_LIMIT_MSG]; ui16i++) {
        if(sTexts[SETTXT_NICK_LIMIT_MSG][ui16i] == '%') {
            if(strncmp(sTexts[SETTXT_NICK_LIMIT_MSG]+ui16i+1, sMin, 5) == 0) {
                int iRet = sprintf(sPreTexts[SETPRETXT_NICK_LIMIT_MSG]+iMsgLen, "%hd", iShorts[SETSHORT_MIN_NICK_LEN]);
                iMsgLen += iRet;
                if(CheckSprintf1(iRet, iMsgLen, szNeededMem, "SetMan::UpdateNickLimitMessage") == false) {
                    exit(EXIT_FAILURE);
                }

                ui16i += (uint16_t)5;
                continue;
            } else if(strncmp(sTexts[SETTXT_NICK_LIMIT_MSG]+ui16i+1, sMax, 5) == 0) {
                if(iShorts[SETSHORT_MAX_NICK_LEN] != 0) {
                    int iRet = sprintf(sPreTexts[SETPRETXT_NICK_LIMIT_MSG]+iMsgLen, "%hd", iShorts[SETSHORT_MAX_NICK_LEN]);
                    iMsgLen += iRet;
                    if(CheckSprintf1(iRet, iMsgLen, szNeededMem, "SetMan::UpdateNickLimitMessage1") == false) {
                        exit(EXIT_FAILURE);
                    }
                } else {
                    memcpy(sPreTexts[SETPRETXT_NICK_LIMIT_MSG]+iMsgLen, "unlimited", 9);
                    iMsgLen += 9;
                }
                ui16i += (uint16_t)5;
                continue;
            }
        }

        sPreTexts[SETPRETXT_NICK_LIMIT_MSG][iMsgLen] = sTexts[SETTXT_NICK_LIMIT_MSG][ui16i];
        iMsgLen++;
    }

    sPreTexts[SETPRETXT_NICK_LIMIT_MSG][iMsgLen] = '|';
    iMsgLen++;

    if(bBools[SETBOOL_NICK_LIMIT_REDIR] == true) {
        if(sTexts[SETTXT_NICK_LIMIT_REDIR_ADDRESS] != NULL) {
            int iRet = sprintf(sPreTexts[SETPRETXT_NICK_LIMIT_MSG]+iMsgLen, "$ForceMove %s|", sTexts[SETTXT_NICK_LIMIT_REDIR_ADDRESS]);
            iMsgLen += iRet;
            if(CheckSprintf1(iRet, iMsgLen, szNeededMem, "SetMan::UpdateNickLimitMessage2") == false) {
                exit(EXIT_FAILURE);
            }
        }  else if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	memcpy(sPreTexts[SETPRETXT_NICK_LIMIT_MSG]+iMsgLen, sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
            iMsgLen += (int)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

    ui16PreTextsLens[SETPRETXT_NICK_LIMIT_MSG] = (uint16_t)iMsgLen;
    sPreTexts[SETPRETXT_NICK_LIMIT_MSG][iMsgLen] = '\0';
}
//---------------------------------------------------------------------------

void SetMan::UpdateMinShare() {
    if(bUpdateLocked == true) {
        return;
    }

	ui64MinShare = (uint64_t)(iShorts[SETSHORT_MIN_SHARE_LIMIT] == 0 ? 0 : iShorts[SETSHORT_MIN_SHARE_LIMIT] * pow(1024.0, (int)iShorts[SETSHORT_MIN_SHARE_UNITS]));
}
//---------------------------------------------------------------------------

void SetMan::UpdateMaxShare() {
    if(bUpdateLocked == true) {
        return;
    }

	ui64MaxShare = (uint64_t)(iShorts[SETSHORT_MAX_SHARE_LIMIT] == 0 ? 0 : iShorts[SETSHORT_MAX_SHARE_LIMIT] * pow(1024.0, (int)iShorts[SETSHORT_MAX_SHARE_UNITS]));
}
//---------------------------------------------------------------------------

void SetMan::UpdateTCPPorts() {
    if(bUpdateLocked == true) {
        return;
    }

    char * sPort = sTexts[SETTXT_TCP_PORTS];
    uint8_t ui8ActualPort = 0;
    for(uint16_t ui16i = 0; ui16i < ui16TextsLens[SETTXT_TCP_PORTS] && ui8ActualPort < 25; ui16i++) {
        if(sTexts[SETTXT_TCP_PORTS][ui16i] == ';') {
            sTexts[SETTXT_TCP_PORTS][ui16i] = '\0';

            if(ui8ActualPort != 0) {
                iPortNumbers[ui8ActualPort] = (uint16_t)atoi(sPort);
            } else {
#ifdef _WIN32
                EnterCriticalSection(&csSetting);
#else
				pthread_mutex_lock(&mtxSetting);
#endif

                iPortNumbers[ui8ActualPort] = (uint16_t)atoi(sPort);

#ifdef _WIN32
                LeaveCriticalSection(&csSetting);
#else
				pthread_mutex_unlock(&mtxSetting);
#endif
            }

            sTexts[SETTXT_TCP_PORTS][ui16i] = ';';

            sPort = sTexts[SETTXT_TCP_PORTS]+ui16i+1;
            ui8ActualPort++;
            continue;
        }
    }

    if(sPort[0] != '\0') {
        iPortNumbers[ui8ActualPort] = (uint16_t)atoi(sPort);
        ui8ActualPort++;
    }

    while(ui8ActualPort < 25) {
        iPortNumbers[ui8ActualPort] = 0;
        ui8ActualPort++;
    }

	if(bServerRunning == false) {
        return;
    }

	ServerUpdateServers();
}
//---------------------------------------------------------------------------

void SetMan::UpdateBotsSameNick() {
    if(bUpdateLocked == true) {
        return;
    }

    if(sTexts[SETTXT_BOT_NICK] != NULL && sTexts[SETTXT_OP_CHAT_NICK] != NULL && 
        bBools[SETBOOL_REG_BOT] == true && bBools[SETBOOL_REG_OP_CHAT] == true) {
		bBotsSameNick = (strcasecmp(sTexts[SETTXT_BOT_NICK], sTexts[SETTXT_OP_CHAT_NICK]) == 0);
    } else {
        bBotsSameNick = false;
    }
}
//---------------------------------------------------------------------------
void SetMan::UpdateLanguage() {
    if(bUpdateLocked == true) {
        return;
    }

    LanguageManager->LoadLanguage();

    UpdateHubNameWelcome();

#ifdef _BUILD_GUI
    pMainWindow->UpdateLanguage();
#endif
}
//---------------------------------------------------------------------------

void SetMan::UpdateBot(const bool &bNickChanged/* = true*/) {
    if(bUpdateLocked == true) {
        return;
	}

    if(bBools[SETBOOL_REG_BOT] == false) {
        if(sPreTexts[SETPRETXT_HUB_BOT_MYINFO] != NULL) {
#ifdef _WIN32
            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[SETPRETXT_HUB_BOT_MYINFO]) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate memory in SetMan::UpdateBot\n", 0);
            }
#else
            free(sPreTexts[SETPRETXT_HUB_BOT_MYINFO]);
#endif
            sPreTexts[SETPRETXT_HUB_BOT_MYINFO] = NULL;
            ui16PreTextsLens[SETPRETXT_HUB_BOT_MYINFO] = 0;
        }

        return;
    }

    size_t szNeededMem = 23 + ui16TextsLens[SETTXT_BOT_NICK] + ui16TextsLens[SETTXT_BOT_DESCRIPTION] + ui16TextsLens[SETTXT_BOT_EMAIL];

    char * sOldHubBotMyinfoMsg = sPreTexts[SETPRETXT_HUB_BOT_MYINFO];

#ifdef _WIN32
    if(sPreTexts[SETPRETXT_HUB_BOT_MYINFO] == NULL) {
        sPreTexts[SETPRETXT_HUB_BOT_MYINFO] = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededMem);
    } else {
        sPreTexts[SETPRETXT_HUB_BOT_MYINFO] = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldHubBotMyinfoMsg, szNeededMem);
    }
#else
	sPreTexts[SETPRETXT_HUB_BOT_MYINFO] = (char *)realloc(sOldHubBotMyinfoMsg, szNeededMem);
#endif
    if(sPreTexts[SETPRETXT_HUB_BOT_MYINFO] == NULL) {
        sPreTexts[SETPRETXT_HUB_BOT_MYINFO] = sOldHubBotMyinfoMsg;

		AppendDebugLog("%s - [MEM] Cannot (re)allocate %" PRIu64 " bytes in SetMan::UpdateBot\n", (uint64_t)szNeededMem);

        return;
    }

    int iMsgLen = sprintf(sPreTexts[SETPRETXT_HUB_BOT_MYINFO], "$MyINFO $ALL %s %s$ $$%s$$|", sTexts[SETTXT_BOT_NICK],
        sTexts[SETTXT_BOT_DESCRIPTION] != NULL ? sTexts[SETTXT_BOT_DESCRIPTION] : "",
        sTexts[SETTXT_BOT_EMAIL] != NULL ? sTexts[SETTXT_BOT_EMAIL] : "");
    if(CheckSprintf(iMsgLen, szNeededMem, "SetMan::UpdateBot") == false) {
        exit(EXIT_FAILURE);
    }

    ui16PreTextsLens[SETPRETXT_HUB_BOT_MYINFO] = (uint16_t)iMsgLen;
    sPreTexts[SETPRETXT_HUB_BOT_MYINFO][iMsgLen] = '\0';

    if(ServersS == NULL) {
        return;
    }

    if(bNickChanged == true && (bBotsSameNick == false || ServersS->bActive == false)) {
        colUsers->AddBot2NickList(sTexts[SETTXT_BOT_NICK], (size_t)ui16TextsLens[SETTXT_BOT_NICK], true);
    }

    colUsers->AddBot2MyInfos(sPreTexts[SETPRETXT_HUB_BOT_MYINFO]);

    if(ServersS->bActive == false) {
        return;
    }

    if(bNickChanged == true && bBotsSameNick == false) {
        char sMsg[128];
        iMsgLen = sprintf(sMsg, "$Hello %s|", sTexts[SETTXT_BOT_NICK]);
        if(CheckSprintf(iMsgLen, 128, "SetMan::UpdateBot1") == true) {
            g_GlobalDataQueue->AddQueueItem(sMsg, iMsgLen, NULL, 0, GlobalDataQueue::CMD_HELLO);
        }
    }

    g_GlobalDataQueue->AddQueueItem(sPreTexts[SETPRETXT_HUB_BOT_MYINFO], ui16PreTextsLens[SETPRETXT_HUB_BOT_MYINFO], NULL, 0, GlobalDataQueue::CMD_MYINFO);

    if(bNickChanged == true && bBotsSameNick == false) {
        g_GlobalDataQueue->OpListStore(sTexts[SETTXT_BOT_NICK]);
    }
}
//---------------------------------------------------------------------------

void SetMan::DisableBot(const bool &bNickChanged/* = true*/) {
	if(bUpdateLocked == true || bServerRunning == false) {
        return;
    }

    if(bNickChanged == true) {
        if(bBotsSameNick == false) {
            colUsers->DelFromNickList(sTexts[SETTXT_BOT_NICK], true);
        }

        char sMsg[128];
        int iMsgLen = sprintf(sMsg, "$Quit %s|", sTexts[SETTXT_BOT_NICK]);
        if(CheckSprintf(iMsgLen, 128, "SetMan::DisableBot") == true) {
            if(bBotsSameNick == true) {
                // PPK ... send Quit only to users without opchat permission...
                User *next = colUsers->llist;
                while(next != NULL) {
                    User *curUser = next;
                    next = curUser->next;
                    if(curUser->ui8State == User::STATE_ADDED && ProfileMan->IsAllowed(curUser, ProfileManager::ALLOWEDOPCHAT) == false) {
                        UserSendCharDelayed(curUser, sMsg, iMsgLen);
                    }
                }
            } else {
                g_GlobalDataQueue->AddQueueItem(sMsg, iMsgLen, NULL, 0, GlobalDataQueue::CMD_QUIT);
            }
        }
    }

    if(sPreTexts[SETPRETXT_HUB_BOT_MYINFO] != NULL && bBotsSameNick == false) {
        colUsers->DelBotFromMyInfos(sPreTexts[SETPRETXT_HUB_BOT_MYINFO]);
    }
}
//---------------------------------------------------------------------------

void SetMan::UpdateOpChat(const bool &bNickChanged/* = true*/) {
    if(bUpdateLocked == true) {
        return;
    }

    if(bBools[SETBOOL_REG_OP_CHAT] == false) {
        if(sPreTexts[SETPRETXT_OP_CHAT_HELLO] != NULL) {
#ifdef _WIN32
            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[SETPRETXT_OP_CHAT_HELLO]) == 0) {
				AppendDebugLog("%s - [MEM] Cannot deallocate memory in SetMan::UpdateOpChat\n", 0);
            }
#else
			free(sPreTexts[SETPRETXT_OP_CHAT_HELLO]);
#endif
            sPreTexts[SETPRETXT_OP_CHAT_HELLO] = NULL;
            ui16PreTextsLens[SETPRETXT_OP_CHAT_HELLO] = 0;
        }

        if(sPreTexts[SETPRETXT_OP_CHAT_MYINFO] != NULL) {
#ifdef _WIN32
            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[SETPRETXT_OP_CHAT_MYINFO]) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate memory in SetMan::UpdateOpChat1\n", 0);
            }
#else
            free(sPreTexts[SETPRETXT_OP_CHAT_MYINFO]);
#endif
            sPreTexts[SETPRETXT_OP_CHAT_MYINFO] = NULL;
            ui16PreTextsLens[SETPRETXT_OP_CHAT_MYINFO] = 0;
        }

        return;
    }

    size_t szNeededMem = 9 + ui16TextsLens[SETTXT_OP_CHAT_NICK];

    char * sOldOpChatHelloMsg = sPreTexts[SETPRETXT_OP_CHAT_HELLO];

#ifdef _WIN32
    if(sPreTexts[SETPRETXT_OP_CHAT_HELLO] == NULL) {
        sPreTexts[SETPRETXT_OP_CHAT_HELLO] = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededMem);
    } else {
        sPreTexts[SETPRETXT_OP_CHAT_HELLO] = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldOpChatHelloMsg, szNeededMem);
    }
#else
	sPreTexts[SETPRETXT_OP_CHAT_HELLO] = (char *)realloc(sOldOpChatHelloMsg, szNeededMem);
#endif
    if(sPreTexts[SETPRETXT_OP_CHAT_HELLO] == NULL) {
        sPreTexts[SETPRETXT_OP_CHAT_HELLO] = sOldOpChatHelloMsg;

		AppendDebugLog("%s - [MEM] Cannot (re)allocate %" PRIu64 " bytes in SetMan::UpdateOpChat\n", (uint64_t)szNeededMem);

        return;
    }

    int iMsgLen = sprintf(sPreTexts[SETPRETXT_OP_CHAT_HELLO], "$Hello %s|", SettingManager->sTexts[SETTXT_OP_CHAT_NICK]);
    if(CheckSprintf(iMsgLen, szNeededMem, "SetMan::UpdateOpChat") == false) {
        exit(EXIT_FAILURE);
    }

    ui16PreTextsLens[SETPRETXT_OP_CHAT_HELLO] = (uint16_t)iMsgLen;
    sPreTexts[SETPRETXT_OP_CHAT_HELLO][iMsgLen] = '\0';

    szNeededMem = 23 + ui16TextsLens[SETTXT_OP_CHAT_NICK] + ui16TextsLens[SETTXT_OP_CHAT_DESCRIPTION] + ui16TextsLens[SETTXT_OP_CHAT_EMAIL];

    char * sOldOpChatMyInfoMsg = sPreTexts[SETPRETXT_OP_CHAT_MYINFO];

#ifdef _WIN32
    if(sPreTexts[SETPRETXT_OP_CHAT_MYINFO] == NULL) {
        sPreTexts[SETPRETXT_OP_CHAT_MYINFO] = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededMem);
    } else {
        sPreTexts[SETPRETXT_OP_CHAT_MYINFO] = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldOpChatMyInfoMsg, szNeededMem);
    }
#else
	sPreTexts[SETPRETXT_OP_CHAT_MYINFO] = (char *)realloc(sOldOpChatMyInfoMsg, szNeededMem);
#endif
    if(sPreTexts[SETPRETXT_OP_CHAT_MYINFO] == NULL) {
        sPreTexts[SETPRETXT_OP_CHAT_MYINFO] = sOldOpChatMyInfoMsg;

		AppendDebugLog("%s - [MEM] Cannot (re)allocate %" PRIu64 " bytes in SetMan::UpdateOpChat1\n", (uint64_t)szNeededMem);

		if(sPreTexts[SETPRETXT_OP_CHAT_MYINFO] == NULL) {
            exit(EXIT_FAILURE);
        }
        return;
    }

    iMsgLen = sprintf(sPreTexts[SETPRETXT_OP_CHAT_MYINFO], "$MyINFO $ALL %s %s$ $$%s$$|", sTexts[SETTXT_OP_CHAT_NICK],
        sTexts[SETTXT_OP_CHAT_DESCRIPTION] != NULL ? sTexts[SETTXT_OP_CHAT_DESCRIPTION] : "", 
        sTexts[SETTXT_OP_CHAT_EMAIL] != NULL ? sTexts[SETTXT_OP_CHAT_EMAIL] : "");
    if(CheckSprintf(iMsgLen, szNeededMem, "SetMan::UpdateOpChat1") == false) {
        exit(EXIT_FAILURE);
    }

    ui16PreTextsLens[SETPRETXT_OP_CHAT_MYINFO] = (uint16_t)iMsgLen;
    sPreTexts[SETPRETXT_OP_CHAT_MYINFO][iMsgLen] = '\0';

	if(bServerRunning == false) {
        return;
    }

    if(bBotsSameNick == false) {
        char sMsg[128];
        iMsgLen = sprintf(sMsg, "$OpList %s$$|", sTexts[SETTXT_OP_CHAT_NICK]);
        if(CheckSprintf(iMsgLen, 128, "SetMan::UpdateOpChat2") == false) {
            exit(EXIT_FAILURE);
        }

        User *next = colUsers->llist;
        while(next != NULL) {
            User *curUser = next;
            next = curUser->next;
            if(curUser->ui8State == User::STATE_ADDED && ProfileMan->IsAllowed(curUser, ProfileManager::ALLOWEDOPCHAT) == true) {
                if(bNickChanged == true && (((curUser->ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == false)) {
                    UserSendCharDelayed(curUser, SettingManager->sPreTexts[SETPRETXT_OP_CHAT_HELLO], 
                        SettingManager->ui16PreTextsLens[SETPRETXT_OP_CHAT_HELLO]);
                }
                UserSendCharDelayed(curUser, SettingManager->sPreTexts[SETPRETXT_OP_CHAT_MYINFO], 
                    SettingManager->ui16PreTextsLens[SETPRETXT_OP_CHAT_MYINFO]);
                if(bNickChanged == true) {
                    UserSendCharDelayed(curUser, sMsg, iMsgLen);
                }    
            }
        }
    }
}
//---------------------------------------------------------------------------

void SetMan::DisableOpChat(const bool &bNickChanged/* = true*/) {
    if(bUpdateLocked == true || ServersS == NULL || bBotsSameNick == true) {
        return;
    }

    if(bNickChanged == true) {
        colUsers->DelFromNickList(sTexts[SETTXT_OP_CHAT_NICK], true);

        char msg[128];
        int iMsgLen = sprintf(msg, "$Quit %s|", sTexts[SETTXT_OP_CHAT_NICK]);
        if(CheckSprintf(iMsgLen, 128, "SetMan::DisableOpChat") == true) {
            User *next = colUsers->llist;
            while(next != NULL) {
                User *curUser = next;
                next = curUser->next;
                if(curUser->ui8State == User::STATE_ADDED && ProfileMan->IsAllowed(curUser, ProfileManager::ALLOWEDOPCHAT) == true) {
                    UserSendCharDelayed(curUser, msg, iMsgLen);
                }
            }
        }
    }
}
//---------------------------------------------------------------------------

void SetMan::UpdateUDPPort() {
	if(bUpdateLocked == true || bServerRunning == false) {
        return;
    }

    UDPThread::Destroy(g_pUDPThread6);
    g_pUDPThread6 = NULL;

    UDPThread::Destroy(g_pUDPThread4);
    g_pUDPThread4 = NULL;

    if((uint16_t)atoi(sTexts[SETTXT_UDP_PORT]) != 0) {
        if(SettingManager->bBools[SETBOOL_BIND_ONLY_SINGLE_IP] == true || (bUseIPv6 == true && bIPv6DualStack == false)) {
            if(bUseIPv6 == true) {
                g_pUDPThread6 = UDPThread::Create(AF_INET6);
            }

            g_pUDPThread4 = UDPThread::Create(AF_INET);
        } else {
            g_pUDPThread6 = UDPThread::Create(bUseIPv6 == true ? AF_INET6 : AF_INET);
        }
    }
}
//---------------------------------------------------------------------------

void SetMan::UpdateScripting() const {
    if(bUpdateLocked == true || bServerRunning == false) {
        return;
    }

    if(bBools[SETBOOL_ENABLE_SCRIPTING] == true) {
		ScriptManager->Start();
		ScriptManager->OnStartup();
    } else {
		ScriptManager->OnExit(true);
		ScriptManager->Stop();
    }
}
//---------------------------------------------------------------------------
