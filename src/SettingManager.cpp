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
#include "SettingXml.h"
#include "SettingDefaults.h"
#include "SettingManager.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "globalQueue.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
//---------------------------------------------------------------------------
	#ifndef _SERVICE
		#include "frmHub.h"
	#endif
#endif
#include "ResNickManager.h"
#include "ServerThread.h"
#ifdef _WIN32
	#ifndef _SERVICE
		#include "TBansForm.h"
	#endif
#endif
#include "TextFileManager.h"
#ifdef _WIN32
	#ifndef _SERVICE
		#include "TinfoForm.h"
		#include "TnewUserForm.h"
		#include "TProfileManagerForm.h"
		#include "TRangeBansForm.h"
		#include "TRegsForm.h"
		#include "TScriptMemoryForm.h"
		#include "TUsersChatForm.h"
	#endif
#endif
#include "UDPThread.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#ifndef _MSC_VER
		#pragma package(smart_init)
	#endif
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

    for(size_t i = 0; i < 25; i++) {
        iPortNumbers[i] = 0;
	}

    for(size_t i = 0; i < SETPRETXT_IDS_END; i++) {
        sPreTexts[i] = NULL;
        ui16PreTextsLens[i] = 0;
	}

    // Read default bools
    for(size_t i = 0; i < SETBOOL_IDS_END; i++) {
        SetBool(i, SetBoolDef[i]);
	}

    // Read default shorts
    for(size_t i = 0; i < SETSHORT_IDS_END; i++) {
        SetShort(i, SetShortDef[i]);
	}

    // Read default texts
	for(size_t i = 0; i < SETTXT_IDS_END; i++) {
        sTexts[i] = NULL;
		ui16TextsLens[i] = 0;

		SetText(i, SetTxtDef[i]);
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
			string sDbgstr = "[BUF] Cannot deallocate sMOTD in SetMan::~SetMan! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(sMOTD);
#endif
        sMOTD = NULL;
        ui16MOTDLen = 0;
    }

    for(size_t i = 0; i < SETTXT_IDS_END; i++) {
        if(sTexts[i] == NULL) {
            continue;
        }

#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sTexts[i]) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate sTexts[i] in SetMan::~SetMan! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(sTexts[i]);
#endif
        sTexts[i] = NULL;
        ui16TextsLens[i] = 0;
    }

    for(size_t i = 0; i < SETPRETXT_IDS_END; i++) {
        if(sPreTexts[i] == NULL || (i == SETPRETXT_HUB_SEC && sPreTexts[i] == sHubSec)) {
            continue;
        }

#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[i]) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate sPreTexts["+string(i)+"] in SetMan::~SetMan! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(sPreTexts[i]);
#endif

        sPreTexts[i] = NULL;
        ui16PreTextsLens[i] = 0;
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
    sMOTD = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, 18);
#else
	sMOTD = (char *) malloc(18);
#endif
    if(sMOTD == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate 18 bytes of memory in SetMan::SetMan for sMOTD!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
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
			string sDbgstr = "[BUF] Cannot deallocate sMOTD in SetMan::SetMan! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
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
        if(ulflen != 0) {
            fseek(fr, 0, SEEK_SET);
            ui16MOTDLen = (uint16_t)(ulflen < 65024 ? ulflen : 65024);

            // allocate memory for sMOTD
#ifdef _WIN32
            sMOTD = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, ui16MOTDLen+1);
#else
			sMOTD = (char *) malloc(ui16MOTDLen+1);
#endif
            if(sMOTD == NULL) {
				string sDbgstr = "[BUF] Cannot allocate "+string(ui16MOTDLen+1)+
					" bytes of memory in SetMan::SetMan for sMOTD!";
#ifdef _WIN32
				sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
				AppendSpecialLog(sDbgstr);
                exit(EXIT_FAILURE);
            }

            // read from file to sMOTD, if it failed then free sMOTD and create default one
            if(fread(sMOTD, 1, (size_t)ui16MOTDLen, fr) == (size_t)ui16MOTDLen) {
                sMOTD[ui16MOTDLen] = '\0';
            } else {
#ifdef _WIN32
                if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMOTD) == 0) {
					string sDbgstr = "[BUF] Cannot deallocate sMOTD in SetMan::SetMan! "+string((uint32_t)GetLastError())+" "+
						string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
					AppendSpecialLog(sDbgstr);
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
    for(uint16_t i = 0; i < ui16MOTDLen; i++) {
        if(sMOTD[i] == '|') {
            sMOTD[i] = '0';
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

                for(size_t i = 0; i < SETBOOL_IDS_END; i++) {
                    if(strcmp(SetBoolXmlStr[i], sName) == 0) {
                        SetBool(i, bValue);
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

                for(size_t i = 0; i < SETSHORT_IDS_END; i++) {
                    if(strcmp(SetShortXmlStr[i], sName) == 0) {
                        SetShort(i, (int16_t)iValue);
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

                for(size_t i = 0; i < SETTXT_IDS_END; i++) {
                    if(strcmp(SetTxtXmlStr[i], sName) == 0) {
                        SetText(i, sText);
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
    for(size_t i = 0; i < SETBOOL_IDS_END; i++) {
        // Don't save setting with default value
        if(bBools[i] == SetBoolDef[i]) {
            continue;
        }

        TiXmlElement boolean("Bool");
        boolean.SetAttribute("Name", SetBoolXmlStr[i]);
        boolean.InsertEndChild(TiXmlText(bBools[i] == 0 ? "0" : "1"));
        booleans.InsertEndChild(boolean);
    }
    settings.InsertEndChild(booleans);

    // Save integers
    TiXmlElement integers("Integers");
    char msg[8];
    for(size_t i = 0; i < SETSHORT_IDS_END; i++) {
        // Don't save setting with default value
        if(iShorts[i] == SetShortDef[i]) {
            continue;
        }

        TiXmlElement integer("Integer");
        integer.SetAttribute("Name", SetShortXmlStr[i]);
		sprintf(msg, "%d", (int)iShorts[i]);
        integer.InsertEndChild(TiXmlText(msg));
        integers.InsertEndChild(integer);
    }
    settings.InsertEndChild(integers);

    // Save strings
    TiXmlElement setstrings("Strings");
    for(size_t i = 0; i < SETTXT_IDS_END; i++) {
        // Don't save setting with default value
        if((sTexts[i] == NULL && strlen(SetTxtDef[i]) == 0) || 
            (sTexts[i] != NULL && strcmp(sTexts[i], SetTxtDef[i]) == 0)) {
            continue;
        }

        TiXmlElement setstring("String");
        setstring.SetAttribute("Name", SetTxtXmlStr[i]);
        setstring.InsertEndChild(TiXmlText(sTexts[i] != NULL ? sTexts[i] : ""));
        setstrings.InsertEndChild(setstring);
    }
    settings.InsertEndChild(setstrings);
    
    doc.InsertEndChild(settings);

    doc.SaveFile();
}
//---------------------------------------------------------------------------

bool SetMan::GetBool(size_t iBoolId) {
#ifdef _WIN32
    EnterCriticalSection(&csSetting);
#else
	pthread_mutex_lock(&mtxSetting);
#endif

    bool bValue = bBools[iBoolId];

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

int16_t SetMan::GetShort(size_t iShortId) {
#ifdef _WIN32
    EnterCriticalSection(&csSetting);
#else
	pthread_mutex_lock(&mtxSetting);
#endif

    int16_t iValue = iShorts[iShortId];

#ifdef _WIN32
    LeaveCriticalSection(&csSetting);
#else
	pthread_mutex_unlock(&mtxSetting);
#endif
    
    return iValue;
}
//---------------------------------------------------------------------------

void SetMan::GetText(size_t iTxtId, char * sMsg) {
#ifdef _WIN32
    EnterCriticalSection(&csSetting);
#else
	pthread_mutex_lock(&mtxSetting);
#endif

    if(sTexts[iTxtId] != NULL) {
        strcat(sMsg, sTexts[iTxtId]);
    }

#ifdef _WIN32
    LeaveCriticalSection(&csSetting);
#else
	pthread_mutex_unlock(&mtxSetting);
#endif
}
//---------------------------------------------------------------------------

void SetMan::SetBool(size_t iBoolId, const bool &bValue) {
    if(bBools[iBoolId] == bValue) {
        return;
    }
    
    if(iBoolId == SETBOOL_ANTI_MOGLO) {
#ifdef _WIN32
        EnterCriticalSection(&csSetting);
#else
		pthread_mutex_lock(&mtxSetting);
#endif

        bBools[iBoolId] = bValue;

#ifdef _WIN32
        LeaveCriticalSection(&csSetting);
#else
		pthread_mutex_unlock(&mtxSetting);
#endif

        return;
    }

    bBools[iBoolId] = bValue;

    switch(iBoolId) {
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
		case SETBOOL_ENABLE_TRAY_ICON:
#ifdef _WIN32
	#ifndef _SERVICE
	            if(bUpdateLocked == false) {
					hubForm->UpdateSysTray();
				}
	#endif
#endif
            break;
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

void SetMan::SetMOTD(char * sTxt, const size_t &iLen) {
    if(ui16MOTDLen == (uint16_t)iLen && 
        (sMOTD != NULL && strcmp(sMOTD, sTxt) == 0)) {
        return;
    }

    if(sMOTD != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMOTD) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate sMOTD in SetMan::SetMan! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(sMOTD);
#endif
        sMOTD = NULL;
        ui16MOTDLen = 0;
    }

    if(iLen == 0) {
        return;
    }

    ui16MOTDLen = (uint16_t)(iLen < 65024 ? iLen : 65024);

    // allocate memory for sMOTD
#ifdef _WIN32
    sMOTD = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, ui16MOTDLen+1);
#else
	sMOTD = (char *) malloc(ui16MOTDLen+1);
#endif
    if(sMOTD == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string(ui16MOTDLen+1)+
			" bytes of memory in SetMan::SetMan for sMOTD!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
        AppendSpecialLog(sDbgstr);
        return;
    }

    memcpy(sMOTD, sTxt, (size_t)ui16MOTDLen);
    sMOTD[ui16MOTDLen] = '\0';

    CheckMOTD();
}
//---------------------------------------------------------------------------

void SetMan::SetShort(size_t iShortId, const int16_t &iValue) {
    if(iValue < 0 || iShorts[iShortId] == iValue) {
        return;
    }

    switch(iShortId) {
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
        case SETSHORT_DEFAULT_TEMP_BAN_TIME:
        case SETSHORT_DEFLOOD_TEMP_BAN_TIME:
        case SETSHORT_BRUTE_FORCE_PASS_PROTECT_TEMP_BAN_TIME:
            if(iValue == 0) {
                return;
            }
            break;
        case SETSHORT_MAIN_CHAT_MESSAGES:
        case SETSHORT_MAIN_CHAT_MESSAGES2:
        case SETSHORT_MAIN_CHAT_TIME:
        case SETSHORT_MAIN_CHAT_TIME2:
        case SETSHORT_SAME_MAIN_CHAT_MESSAGES:
        case SETSHORT_SAME_MAIN_CHAT_TIME:
        case SETSHORT_PM_MESSAGES:
        case SETSHORT_PM_MESSAGES2:
        case SETSHORT_PM_TIME:
        case SETSHORT_PM_TIME2:
        case SETSHORT_SAME_PM_MESSAGES:
        case SETSHORT_SAME_PM_TIME:
        case SETSHORT_SEARCH_MESSAGES:
        case SETSHORT_SEARCH_MESSAGES2:
        case SETSHORT_SEARCH_TIME:
        case SETSHORT_SEARCH_TIME2:
        case SETSHORT_SAME_SEARCH_MESSAGES:
        case SETSHORT_SAME_SEARCH_TIME:
        case SETSHORT_MYINFO_MESSAGES:
        case SETSHORT_MYINFO_MESSAGES2:
        case SETSHORT_MYINFO_TIME:
        case SETSHORT_MYINFO_TIME2:
        case SETSHORT_GETNICKLIST_MESSAGES:
        case SETSHORT_GETNICKLIST_TIME:
        case SETSHORT_DEFLOOD_WARNING_COUNT:
        case SETSHORT_GLOBAL_MAIN_CHAT_MESSAGES:
        case SETSHORT_GLOBAL_MAIN_CHAT_TIME:
        case SETSHORT_GLOBAL_MAIN_CHAT_TIMEOUT:
        case SETSHORT_CTM_MESSAGES:
        case SETSHORT_CTM_MESSAGES2:
        case SETSHORT_CTM_TIME:
        case SETSHORT_CTM_TIME2:
        case SETSHORT_RCTM_MESSAGES:
        case SETSHORT_RCTM_MESSAGES2:
        case SETSHORT_RCTM_TIME:
        case SETSHORT_RCTM_TIME2:
        case SETSHORT_SR_MESSAGES:
        case SETSHORT_SR_MESSAGES2:
        case SETSHORT_SR_TIME:
        case SETSHORT_SR_TIME2:
        case SETSHORT_MAX_DOWN_KB:
        case SETSHORT_MAX_DOWN_KB2:
        case SETSHORT_MAX_DOWN_TIME:
        case SETSHORT_MAX_DOWN_TIME2:
        case SETSHORT_CHAT_INTERVAL_MESSAGES:
        case SETSHORT_CHAT_INTERVAL_TIME:
        case SETSHORT_PM_INTERVAL_MESSAGES:
        case SETSHORT_PM_INTERVAL_TIME:
        case SETSHORT_SEARCH_INTERVAL_MESSAGES:
        case SETSHORT_SEARCH_INTERVAL_TIME:
            if(iValue == 0 || iValue > 29999) {
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

            iShorts[iShortId] = iValue;

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
            if(iValue == 0 || iValue > 500) {
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

    iShorts[iShortId] = iValue;

    switch(iShortId) {
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

void SetMan::SetText(size_t iTxtId, char * sTxt) {
    SetText(iTxtId, sTxt, strlen(sTxt));
}
//---------------------------------------------------------------------------

void SetMan::SetText(size_t iTxtId, const char * sTxt) {
    SetText(iTxtId, (char *)sTxt, strlen(sTxt));
}
//---------------------------------------------------------------------------

void SetMan::SetText(size_t iTxtId, const char * sTxt, const size_t &iLen) {
    if((ui16TextsLens[iTxtId] == (uint16_t)iLen && 
        (sTexts[iTxtId] != NULL && strcmp(sTexts[iTxtId], sTxt) == 0)) || 
        strchr(sTxt, '|') != NULL) {
        return;
    }

    switch(iTxtId) {
        case SETTXT_HUB_NAME:
        case SETTXT_HUB_ADDRESS:
        case SETTXT_REG_ONLY_MSG:
        case SETTXT_SHARE_LIMIT_MSG:
        case SETTXT_SLOTS_LIMIT_MSG:
        case SETTXT_HUB_SLOT_RATIO_MSG:
        case SETTXT_MAX_HUBS_LIMIT_MSG:
        case SETTXT_NO_TAG_MSG:
        case SETTXT_NICK_LIMIT_MSG:
            if(iLen == 0 || iLen > 256) {
                return;
            }
            break;
        case SETTXT_BOT_NICK:
            if(strchr(sTxt, '$') != NULL || strchr(sTxt, ' ') != NULL || iLen == 0 || iLen > 64) {
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
            if(strchr(sTxt, '$') != NULL || strchr(sTxt, ' ') != NULL || iLen == 0 || iLen > 64) {
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
            if(iLen == 0 || iLen > 64) {
                return;
            }
            break;
        case SETTXT_UDP_PORT:
            if(iLen == 0 || iLen > 5) {
                return;
            }
            UpdateUDPPort();
            break;
        case SETTXT_CHAT_COMMANDS_PREFIXES:
            if(iLen == 0 || iLen > 5) {
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
            if(iLen > 256) {
                return;
            }
            break;
        case SETTXT_REGISTER_SERVERS:
            if(iLen > 1024) {
                return;
            }
            break;
        case SETTXT_BOT_DESCRIPTION:
        case SETTXT_BOT_EMAIL:
            if(strchr(sTxt, '$') != NULL || iLen > 64) {
                return;
            }
            if(bBools[SETBOOL_REG_BOT] == true) {
                DisableBot(false);
            }
            break;
        case SETTXT_OP_CHAT_DESCRIPTION:
        case SETTXT_OP_CHAT_EMAIL:
            if(strchr(sTxt, '$') != NULL || iLen > 64) {
                return;
            }
            if(bBools[SETBOOL_REG_OP_CHAT] == true) {
                DisableOpChat(false);
            }
            break;
        case SETTXT_HUB_OWNER_EMAIL:
            if(iLen > 64) {
                return;
            }
            break;
        case SETTXT_LANGUAGE:
#ifdef _WIN32
            if(iLen != 0 && FileExist((PATH+"\\language\\"+string(sTxt, iLen)+".xml").c_str()) == false) {
#else
			if(iLen != 0 && FileExist((PATH+"/language/"+string(sTxt, iLen)+".xml").c_str()) == false) {
#endif
                return;
            }
            break;
        default:
            if(iLen > 4096) {
                return;
            }
            break;
    }

    if(iTxtId == SETTXT_HUB_NAME || iTxtId == SETTXT_HUB_ADDRESS || iTxtId == SETTXT_HUB_DESCRIPTION) {
#ifdef _WIN32
        EnterCriticalSection(&csSetting);
#else
		pthread_mutex_lock(&mtxSetting);
#endif
    }

    if(sTexts[iTxtId] != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sTexts[iTxtId]) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate memory in SetMan::SetText! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(sTexts[iTxtId]);
#endif
        sTexts[iTxtId] = NULL;
        ui16TextsLens[iTxtId] = 0;
    }

    if(iLen != 0) {
#ifdef _WIN32
        sTexts[iTxtId] = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iLen+1);
#else
		sTexts[iTxtId] = (char *) malloc(iLen+1);
#endif
        if(sTexts[iTxtId] == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iLen+1))+
				" bytes of memory in SetMan::SetText!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            exit(EXIT_FAILURE);
        }
    
        memcpy(sTexts[iTxtId], sTxt, iLen);
        sTexts[iTxtId][iLen] = '\0';
        ui16TextsLens[iTxtId] = (uint16_t)iLen;
    }

    if(iTxtId == SETTXT_HUB_NAME || iTxtId == SETTXT_HUB_ADDRESS || iTxtId == SETTXT_HUB_DESCRIPTION) {
#ifdef _WIN32
        LeaveCriticalSection(&csSetting);
#else
		pthread_mutex_unlock(&mtxSetting);
#endif
    }

    switch(iTxtId) {
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
#ifdef _WIN32
	#ifndef _SERVICE
				if(bUpdateLocked == false) {
					hubForm->UpdateCaption();
				}
	#endif
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

void SetMan::SetText(size_t iTxtId, const string & sTxt) {
    SetText(iTxtId, sTxt.c_str(), sTxt.size());
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

    if(sPreTexts[SetMan::SETPRETXT_HUB_SEC] != NULL) {
        if(sPreTexts[SetMan::SETPRETXT_HUB_SEC] != sHubSec) {
#ifdef _WIN32
            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[SetMan::SETPRETXT_HUB_SEC]) == 0) {
				string sDbgstr = "[BUF] Cannot deallocate memory in SetMan::UpdateHubSec! "+string((uint32_t)GetLastError())+" "+
					string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
				AppendSpecialLog(sDbgstr);
            }
#else
			free(sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
#endif
        }
        sPreTexts[SetMan::SETPRETXT_HUB_SEC] = NULL;
        ui16PreTextsLens[SetMan::SETPRETXT_HUB_SEC] = 0;
    }

    if(bBools[SETBOOL_USE_BOT_NICK_AS_HUB_SEC] == true) {
#ifdef _WIN32
        sPreTexts[SetMan::SETPRETXT_HUB_SEC] = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, ui16TextsLens[SETTXT_BOT_NICK]+1);
#else
		sPreTexts[SetMan::SETPRETXT_HUB_SEC] = (char *) malloc(ui16TextsLens[SETTXT_BOT_NICK]+1);
#endif
        if(sPreTexts[SetMan::SETPRETXT_HUB_SEC] == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string(ui16TextsLens[SETTXT_BOT_NICK]+1)+
				" bytes of memory in SetMan::UpdateHubSec!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            return;
        }   
        memcpy(sPreTexts[SetMan::SETPRETXT_HUB_SEC], sTexts[SETTXT_BOT_NICK], ui16TextsLens[SETTXT_BOT_NICK]);
        ui16PreTextsLens[SetMan::SETPRETXT_HUB_SEC] = ui16TextsLens[SETTXT_BOT_NICK];
        sPreTexts[SetMan::SETPRETXT_HUB_SEC][ui16PreTextsLens[SetMan::SETPRETXT_HUB_SEC]] = '\0';
    } else {
        sPreTexts[SetMan::SETPRETXT_HUB_SEC] = (char *)sHubSec;
        ui16PreTextsLens[SetMan::SETPRETXT_HUB_SEC] = 12;
    }
}
//---------------------------------------------------------------------------

void SetMan::UpdateMOTD() {
    if(bUpdateLocked == true) {
        return;
    }

    if(sPreTexts[SETPRETXT_MOTD] != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[SETPRETXT_MOTD]) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate memory in SetMan::UpdateMOTD! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(sPreTexts[SETPRETXT_MOTD]);
#endif
        sPreTexts[SETPRETXT_MOTD] = NULL;
        ui16PreTextsLens[SETPRETXT_MOTD] = 0;
    }

    if(bBools[SETBOOL_DISABLE_MOTD] == false && sMOTD != NULL) {
        // PPK ... MOTD to chat or MOTD to PM ???
        if(bBools[SETBOOL_MOTD_AS_PM] == true) {
            size_t iNeededMem = (2*(ui16PreTextsLens[SetMan::SETPRETXT_HUB_SEC]))+ui16MOTDLen+21;

#ifdef _WIN32
            sPreTexts[SETPRETXT_MOTD] = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iNeededMem);
#else
			sPreTexts[SETPRETXT_MOTD] = (char *) malloc(iNeededMem);
#endif
            if(sPreTexts[SETPRETXT_MOTD] == NULL) {
				string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)iNeededMem)+
					" bytes of memory in SetMan::UpdateMOTD!";
#ifdef _WIN32
				sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
				AppendSpecialLog(sDbgstr);
                exit(EXIT_FAILURE);
            }
            
            int imsgLen = sprintf(sPreTexts[SETPRETXT_MOTD], "$To: %%s From: %s $<%s> %s|",
                sPreTexts[SetMan::SETPRETXT_HUB_SEC], sPreTexts[SetMan::SETPRETXT_HUB_SEC], sMOTD);
            if(CheckSprintf(imsgLen, iNeededMem, "SetMan::UpdateMOTD") == false) {
                exit(EXIT_FAILURE);
            }
            ui16PreTextsLens[SETPRETXT_MOTD] = (uint16_t)imsgLen;
        } else {
            size_t iNeededMem = ui16PreTextsLens[SetMan::SETPRETXT_HUB_SEC]+ui16MOTDLen+5;

#ifdef _WIN32
            sPreTexts[SETPRETXT_MOTD] = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iNeededMem);
#else
			sPreTexts[SETPRETXT_MOTD] = (char *) malloc(iNeededMem);
#endif
            if(sPreTexts[SETPRETXT_MOTD] == NULL) {
				string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)iNeededMem)+
					" bytes of memory in SetMan::UpdateMOTD!";
#ifdef _WIN32
				sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
				AppendSpecialLog(sDbgstr);
                exit(EXIT_FAILURE);
            }
            int imsgLen = sprintf(sPreTexts[SETPRETXT_MOTD], "<%s> %s|", sPreTexts[SetMan::SETPRETXT_HUB_SEC], sMOTD);
            if(CheckSprintf(imsgLen, iNeededMem, "SetMan::UpdateMOTD1") == false) {
                exit(EXIT_FAILURE);
            }
            ui16PreTextsLens[SETPRETXT_MOTD] = (uint16_t)imsgLen;
        }
    }
}
//---------------------------------------------------------------------------

void SetMan::UpdateHubNameWelcome() {
    if(bUpdateLocked == true) {
        return;
    }

    if(sPreTexts[SETPRETXT_HUB_NAME_WLCM] != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[SETPRETXT_HUB_NAME_WLCM]) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate memory in SetMan::UpdateHubNameWelcome! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(sPreTexts[SETPRETXT_HUB_NAME_WLCM]);
#endif
        sPreTexts[SETPRETXT_HUB_NAME_WLCM] = NULL;
        ui16PreTextsLens[SETPRETXT_HUB_NAME_WLCM] = 0;
    }

    char msg[2048];
	int iLen;

    if(sTexts[SETTXT_HUB_TOPIC] == NULL) {
        iLen = sprintf(msg, "$HubName %s|<%s> %s %s (%s: ",
            sTexts[SETTXT_HUB_NAME], sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_THIS_HUB_IS_RUNNING],
			sTitle.c_str(), LanguageManager->sTexts[LAN_UPTIME]);
    } else {
        iLen =  sprintf(msg, "$HubName %s - %s|<%s> %s %s (%s: ",
            sTexts[SETTXT_HUB_NAME], sTexts[SETTXT_HUB_TOPIC], sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_THIS_HUB_IS_RUNNING], 
			sTitle.c_str(), LanguageManager->sTexts[LAN_UPTIME]);
    }
    
    if(CheckSprintf(iLen, 2048, "SetMan::UpdateHubNameWelcome") == false) {
        exit(EXIT_FAILURE);
    }

#ifdef _WIN32
    sPreTexts[SETPRETXT_HUB_NAME_WLCM] = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iLen+1);
#else
	sPreTexts[SETPRETXT_HUB_NAME_WLCM] = (char *) malloc(iLen+1);
#endif
    if(sPreTexts[SETPRETXT_HUB_NAME_WLCM] == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string(iLen+1)+
			" bytes of memory in SetMan::UpdateHubNameWelcome!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    memcpy(sPreTexts[SETPRETXT_HUB_NAME_WLCM], msg, iLen);
    ui16PreTextsLens[SETPRETXT_HUB_NAME_WLCM] = (uint16_t)iLen;
    sPreTexts[SETPRETXT_HUB_NAME_WLCM][ui16PreTextsLens[SETPRETXT_HUB_NAME_WLCM]] = '\0';
}
//---------------------------------------------------------------------------

void SetMan::UpdateHubName() {
    if(bUpdateLocked == true) {
        return;
    }

    if(sPreTexts[SETPRETXT_HUB_NAME] != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[SETPRETXT_HUB_NAME]) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate memory in SetMan::UpdateHubName! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
            AppendSpecialLog(sDbgstr);
        }
#else
		free(sPreTexts[SETPRETXT_HUB_NAME]);
#endif
        sPreTexts[SETPRETXT_HUB_NAME] = NULL;
        ui16PreTextsLens[SETPRETXT_HUB_NAME] = 0;
    }

    char msg[1024];
	int iLen;

    if(sTexts[SETTXT_HUB_TOPIC] == NULL) {
        iLen = sprintf(msg, "$HubName %s|", sTexts[SETTXT_HUB_NAME]);
    } else {
        iLen = sprintf(msg, "$HubName %s - %s|", sTexts[SETTXT_HUB_NAME], sTexts[SETTXT_HUB_TOPIC]);
    }

    if(CheckSprintf(iLen, 1024, "SetMan::UpdateHubName") == false) {
        exit(EXIT_FAILURE);
    }

#ifdef _WIN32
    sPreTexts[SETPRETXT_HUB_NAME] = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iLen+1);
#else
	sPreTexts[SETPRETXT_HUB_NAME] = (char *) malloc(iLen+1);
#endif
    if(sPreTexts[SETPRETXT_HUB_NAME] == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string(iLen+1)+
			" bytes of memory in SetMan::UpdateHubName!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    memcpy(sPreTexts[SETPRETXT_HUB_NAME], msg, iLen);
    ui16PreTextsLens[SETPRETXT_HUB_NAME] = (uint16_t)iLen;
    sPreTexts[SETPRETXT_HUB_NAME][ui16PreTextsLens[SETPRETXT_HUB_NAME]] = '\0';

	if(bServerRunning == true) {
        globalQ->Store(sPreTexts[SetMan::SETPRETXT_HUB_NAME],
            ui16PreTextsLens[SetMan::SETPRETXT_HUB_NAME]);
    }
}
//---------------------------------------------------------------------------

void SetMan::UpdateRedirectAddress() {
    if(bUpdateLocked == true) {
        return;
    }

    if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[SETPRETXT_REDIRECT_ADDRESS]) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate memory in SetMan::UpdateRedirectAddress! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(sPreTexts[SETPRETXT_REDIRECT_ADDRESS]);
#endif
        sPreTexts[SETPRETXT_REDIRECT_ADDRESS] = NULL;
        ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS] = 0;
    }

    if(sTexts[SETTXT_REDIRECT_ADDRESS] == NULL) {
        return;
    }

    size_t iNeededLen = 13+ui16TextsLens[SETTXT_REDIRECT_ADDRESS];
#ifdef _WIN32
    sPreTexts[SETPRETXT_REDIRECT_ADDRESS] = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iNeededLen);
#else
	sPreTexts[SETPRETXT_REDIRECT_ADDRESS] = (char *) malloc(iNeededLen);
#endif
    if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)iNeededLen)+
			" bytes of memory in SetMan::UpdateRedirectAddress!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    int imsgLen = sprintf(sPreTexts[SETPRETXT_REDIRECT_ADDRESS], "$ForceMove %s|", sTexts[SETTXT_REDIRECT_ADDRESS]);
    if(CheckSprintf(imsgLen, iNeededLen, "SetMan::UpdateRedirectAddress") == false) {
        exit(EXIT_FAILURE);
    }
    ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS] = (uint16_t)imsgLen;
}
//---------------------------------------------------------------------------

void SetMan::UpdateRegOnlyMessage() {
    if(bUpdateLocked == true) {
        return;
    }

    if(sPreTexts[SETPRETXT_REG_ONLY_MSG] != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[SETPRETXT_REG_ONLY_MSG]) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate memory in SetMan::UpdateRegOnlyMessage! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(sPreTexts[SETPRETXT_REG_ONLY_MSG]);
#endif
        sPreTexts[SETPRETXT_REG_ONLY_MSG] = NULL;
        ui16PreTextsLens[SETPRETXT_REG_ONLY_MSG] = 0;
    }

    char msg[1024];

    int imsgLen = sprintf(msg, "<%s> %s|", sPreTexts[SetMan::SETPRETXT_HUB_SEC], sTexts[SETTXT_REG_ONLY_MSG]);
    if(CheckSprintf(imsgLen, 1024, "SetMan::UpdateRegOnlyMessage") == false) {
        exit(EXIT_FAILURE);
    }

    if(bBools[SETBOOL_REG_ONLY_REDIR] == true) {
        if(sTexts[SETTXT_REG_ONLY_REDIR_ADDRESS] != NULL) {
            int iret = sprintf(msg+imsgLen, "$ForceMove %s|", sTexts[SETTXT_REG_ONLY_REDIR_ADDRESS]);
            imsgLen += iret;
            if(CheckSprintf1(iret, imsgLen, 1024, "SetMan::UpdateRegOnlyMessage1") == false) {
                exit(EXIT_FAILURE);
            }
        } else if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	memcpy(msg+imsgLen, sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
            imsgLen += (int)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

#ifdef _WIN32
    sPreTexts[SETPRETXT_REG_ONLY_MSG] = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, imsgLen+1);
#else
	sPreTexts[SETPRETXT_REG_ONLY_MSG] = (char *) malloc(imsgLen+1);
#endif
    if(sPreTexts[SETPRETXT_REG_ONLY_MSG] == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string(imsgLen+1)+
			" bytes of memory in SetMan::UpdateRegOnlyMessage!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    memcpy(sPreTexts[SETPRETXT_REG_ONLY_MSG], msg, imsgLen);
    ui16PreTextsLens[SETPRETXT_REG_ONLY_MSG] = (uint16_t)imsgLen;
    sPreTexts[SETPRETXT_REG_ONLY_MSG][ui16PreTextsLens[SETPRETXT_REG_ONLY_MSG]] = '\0';
}
//---------------------------------------------------------------------------

void SetMan::UpdateShareLimitMessage() {
    if(bUpdateLocked == true) {
        return;
    }

    if(sPreTexts[SETPRETXT_SHARE_LIMIT_MSG] != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[SETPRETXT_SHARE_LIMIT_MSG]) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate memory in SetMan::UpdateShareLimitMessage! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(sPreTexts[SETPRETXT_SHARE_LIMIT_MSG]);
#endif
        sPreTexts[SETPRETXT_SHARE_LIMIT_MSG] = NULL;
        ui16PreTextsLens[SETPRETXT_SHARE_LIMIT_MSG] = 0;
    }

    char msg[1024];
    int imsgLen = sprintf(msg, "<%s> ", sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
    if(CheckSprintf(imsgLen, 1024, "SetMan::UpdateShareLimitMessage0") == false) {
        exit(EXIT_FAILURE);
    }
    
    static const char* units[] = { "B", "kB", "MB", "GB", "TB", "PB", "EB", " ", " ", " ", " ", " ", " ", " ", " ", " " };

    for(uint16_t i = 0; i < ui16TextsLens[SETTXT_SHARE_LIMIT_MSG]; i++) {
        if(sTexts[SETTXT_SHARE_LIMIT_MSG][i] == '%') {
            if(strncmp(sTexts[SETTXT_SHARE_LIMIT_MSG]+i+1, sMin, 5) == 0) {
                if(ui64MinShare != 0) {
#ifdef _WIN32
                    int iret = sprintf(msg+imsgLen, "%I16d %s", iShorts[SETSHORT_MIN_SHARE_LIMIT],
#else
					int iret = sprintf(msg+imsgLen, "%d %s", iShorts[SETSHORT_MIN_SHARE_LIMIT],
#endif
                        units[iShorts[SETSHORT_MIN_SHARE_UNITS]]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "SetMan::UpdateShareLimitMessage") == false) {
                        exit(EXIT_FAILURE);
                    }
                } else {
                    memcpy(msg+imsgLen, "0 B", 3);
                    imsgLen += 3;
                }
                i += (uint16_t)5;
                continue;
            } else if(strncmp(sTexts[SETTXT_SHARE_LIMIT_MSG]+i+1, sMax, 5) == 0) {
                if(ui64MaxShare != 0) {
#ifdef _WIN32
                    int iret = sprintf(msg+imsgLen, "%I16d %s", iShorts[SETSHORT_MAX_SHARE_LIMIT], 
#else
					int iret = sprintf(msg+imsgLen, "%d %s", iShorts[SETSHORT_MAX_SHARE_LIMIT], 
#endif
						units[iShorts[SETSHORT_MAX_SHARE_UNITS]]);
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "SetMan::UpdateShareLimitMessage1") == false) {
                        exit(EXIT_FAILURE);
                    }
                } else {
                    memcpy(msg+imsgLen, "unlimited", 9);
                    imsgLen += 9;
                }
                i += (uint16_t)5;
                continue;
            }
        }

        msg[imsgLen] = sTexts[SETTXT_SHARE_LIMIT_MSG][i];
        imsgLen++;
    }

    msg[imsgLen] = '|';
    imsgLen++;

    if(bBools[SETBOOL_SHARE_LIMIT_REDIR] == true) {
        if(sTexts[SETTXT_SHARE_LIMIT_REDIR_ADDRESS] != NULL) {
        	int iret = sprintf(msg+imsgLen, "$ForceMove %s|", sTexts[SETTXT_SHARE_LIMIT_REDIR_ADDRESS]);
        	imsgLen += iret;
        	if(CheckSprintf1(iret, imsgLen, 1024, "SetMan::UpdateShareLimitMessage6") == false) {
                exit(EXIT_FAILURE);
            }
        }  else if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	memcpy(msg+imsgLen, sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
            imsgLen += (int)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

#ifdef _WIN32
    sPreTexts[SETPRETXT_SHARE_LIMIT_MSG] = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, imsgLen+1);
#else
	sPreTexts[SETPRETXT_SHARE_LIMIT_MSG] = (char *) malloc(imsgLen+1);
#endif
    if(sPreTexts[SETPRETXT_SHARE_LIMIT_MSG] == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string(imsgLen+1)+
			" bytes of memory in SetMan::UpdateShareLimitMessage!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    memcpy(sPreTexts[SETPRETXT_SHARE_LIMIT_MSG], msg, imsgLen);
    ui16PreTextsLens[SETPRETXT_SHARE_LIMIT_MSG] = (uint16_t)imsgLen;
    sPreTexts[SETPRETXT_SHARE_LIMIT_MSG][ui16PreTextsLens[SETPRETXT_SHARE_LIMIT_MSG]] = '\0';
}
//---------------------------------------------------------------------------

void SetMan::UpdateSlotsLimitMessage() {
    if(bUpdateLocked == true) {
        return;
    }

    if(sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG] != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG]) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate memory in SetMan::UpdateSlotsLimitMessage! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG]);
#endif
        sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG] = NULL;
        ui16PreTextsLens[SETPRETXT_SLOTS_LIMIT_MSG] = 0;
    }

    char msg[1024];
    int imsgLen = sprintf(msg, "<%s> ", sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
    if(CheckSprintf(imsgLen, 1024, "SetMan::UpdateSlotsLimitMessage0") == false) {
        exit(EXIT_FAILURE);
    }

    for(uint16_t i = 0; i < ui16TextsLens[SETTXT_SLOTS_LIMIT_MSG]; i++) {
        if(sTexts[SETTXT_SLOTS_LIMIT_MSG][i] == '%') {
            if(strncmp(sTexts[SETTXT_SLOTS_LIMIT_MSG]+i+1, sMin, 5) == 0) {
                if(iShorts[SETSHORT_MIN_SLOTS_LIMIT] != 0) {
#ifdef _WIN32
                    int iret = sprintf(msg+imsgLen, "%I16d", iShorts[SETSHORT_MIN_SLOTS_LIMIT]);
#else
					int iret = sprintf(msg+imsgLen, "%d", iShorts[SETSHORT_MIN_SLOTS_LIMIT]);
#endif
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "SetMan::UpdateSlotsLimitMessage") == false) {
                        exit(EXIT_FAILURE);
                    }
                } else {
                    memcpy(msg+imsgLen, "0", 1);
                    imsgLen += 1;
                }
                i += (uint16_t)5;
                continue;
            } else if(strncmp(sTexts[SETTXT_SLOTS_LIMIT_MSG]+i+1, sMax, 5) == 0) {
                if(iShorts[SETSHORT_MAX_SLOTS_LIMIT] != 0) {
#ifdef _WIN32
                    int iret = sprintf(msg+imsgLen, "%I16d", iShorts[SETSHORT_MAX_SLOTS_LIMIT]);
#else
					int iret = sprintf(msg+imsgLen, "%d", iShorts[SETSHORT_MAX_SLOTS_LIMIT]);
#endif
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "SetMan::UpdateSlotsLimitMessage1") == false) {
                        exit(EXIT_FAILURE);
                    }
                } else {
                    memcpy(msg+imsgLen, "unlimited", 9);
                    imsgLen += 9;
                }
                i += (uint16_t)5;
                continue;
            }
        }

        msg[imsgLen] = sTexts[SETTXT_SLOTS_LIMIT_MSG][i];
        imsgLen++;
    }

    msg[imsgLen] = '|';
    imsgLen++;

    if(bBools[SETBOOL_SLOTS_LIMIT_REDIR] == true) {
        if(sTexts[SETTXT_SLOTS_LIMIT_REDIR_ADDRESS] != NULL) {
        	int iret = sprintf(msg+imsgLen, "$ForceMove %s|", sTexts[SETTXT_SLOTS_LIMIT_REDIR_ADDRESS]);
            imsgLen += iret;
            if(CheckSprintf1(iret, imsgLen, 1024, "SetMan::UpdateSlotsLimitMessage2") == false) {
                exit(EXIT_FAILURE);
            }
        }  else if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	memcpy(msg+imsgLen, sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
            imsgLen += (int)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

#ifdef _WIN32
    sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG] = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, imsgLen+1);
#else
	sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG] = (char *) malloc(imsgLen+1);
#endif
    if(sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG] == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string(imsgLen+1)+
			" bytes of memory in SetMan::UpdateSlotsLimitMessage!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    memcpy(sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG], msg, imsgLen);
    ui16PreTextsLens[SETPRETXT_SLOTS_LIMIT_MSG] = (uint16_t)imsgLen;
    sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG][ui16PreTextsLens[SETPRETXT_SLOTS_LIMIT_MSG]] = '\0';
}
//---------------------------------------------------------------------------

void SetMan::UpdateHubSlotRatioMessage() {
    if(bUpdateLocked == true) {
        return;
    }

    if(sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG] != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG]) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate memory in SetMan::UpdateHubSlotRatioMessage! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
            AppendSpecialLog(sDbgstr);
        }
#else
		free(sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG]);
#endif
        sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG] = NULL;
        ui16PreTextsLens[SETPRETXT_HUB_SLOT_RATIO_MSG] = 0;
    }

    char msg[1024];
    int imsgLen = sprintf(msg, "<%s> ", sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
    if(CheckSprintf(imsgLen, 1024, "SetMan::UpdateHubSlotRatioMessage0") == false) {
        exit(EXIT_FAILURE);
    }

    static const char* sHubs = "[hubs]";
    static const char* sSlots = "[slots]";

    for(uint16_t i = 0; i < ui16TextsLens[SETTXT_HUB_SLOT_RATIO_MSG]; i++) {
        if(sTexts[SETTXT_HUB_SLOT_RATIO_MSG][i] == '%') {
            if(strncmp(sTexts[SETTXT_HUB_SLOT_RATIO_MSG]+i+1, sHubs, 6) == 0) {
#ifdef _WIN32
                int iret = sprintf(msg+imsgLen, "%I16d", iShorts[SETSHORT_HUB_SLOT_RATIO_HUBS]);
#else
				int iret = sprintf(msg+imsgLen, "%d", iShorts[SETSHORT_HUB_SLOT_RATIO_HUBS]);
#endif
                imsgLen += iret;
                if(CheckSprintf1(iret, imsgLen, 1024, "SetMan::UpdateHubSlotRatioMessage") == false) {
                    exit(EXIT_FAILURE);
                }
                i += (uint16_t)6;
                continue;
            } else if(strncmp(sTexts[SETTXT_HUB_SLOT_RATIO_MSG]+i+1, sSlots, 7) == 0) {
#ifdef _WIN32
                int iret = sprintf(msg+imsgLen, "%I16d", iShorts[SETSHORT_HUB_SLOT_RATIO_SLOTS]);
#else
				int iret = sprintf(msg+imsgLen, "%d", iShorts[SETSHORT_HUB_SLOT_RATIO_SLOTS]);
#endif
                imsgLen += iret;
                if(CheckSprintf1(iret, imsgLen, 1024, "SetMan::UpdateHubSlotRatioMessage1") == false) {
                    exit(EXIT_FAILURE);
                }
                i += (uint16_t)7;
                continue;
            }
        }

        msg[imsgLen] = sTexts[SETTXT_HUB_SLOT_RATIO_MSG][i];
        imsgLen++;
    }

    msg[imsgLen] = '|';
    imsgLen++;

    if(bBools[SETBOOL_HUB_SLOT_RATIO_REDIR] == true) {
        if(sTexts[SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS] != NULL) {
        	int iret = sprintf(msg+imsgLen, "$ForceMove %s|", sTexts[SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS]);
            imsgLen += iret;
            if(CheckSprintf1(iret, imsgLen, 1024, "SetMan::UpdateHubSlotRatioMessage2") == false) {
                exit(EXIT_FAILURE);
            }
        }  else if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	memcpy(msg+imsgLen, sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
            imsgLen += (int)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

#ifdef _WIN32
    sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG] = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, imsgLen+1);
#else
	sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG] = (char *) malloc(imsgLen+1);
#endif
    if(sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG] == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string(imsgLen+1)+
			" bytes of memory in SetMan::UpdateHubSlotRatioMessage!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    memcpy(sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG], msg, imsgLen);
    ui16PreTextsLens[SETPRETXT_HUB_SLOT_RATIO_MSG] = (uint16_t)imsgLen;
    sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG][ui16PreTextsLens[SETPRETXT_HUB_SLOT_RATIO_MSG]] = '\0';
}
//---------------------------------------------------------------------------

void SetMan::UpdateMaxHubsLimitMessage() {
    if(bUpdateLocked == true) {
        return;
    }

    if(sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG] != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG]) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate memory in SetMan::UpdateMaxHubsLimitMessage! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG]);
#endif
        sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG] = NULL;
        ui16PreTextsLens[SETPRETXT_MAX_HUBS_LIMIT_MSG] = 0;
    }

    char msg[1024];
    int imsgLen = sprintf(msg, "<%s> ", sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
    if(CheckSprintf(imsgLen, 1024, "SetMan::UpdateMaxHubsLimitMessage0") == false) {
        exit(EXIT_FAILURE);
    }

    static const char* sHubs = "%[hubs]";

    char * sMatch = strstr(sTexts[SETTXT_MAX_HUBS_LIMIT_MSG], sHubs);

    if(sMatch != NULL) {
        if(sMatch > sTexts[SETTXT_MAX_HUBS_LIMIT_MSG]) {
            size_t iLen = sMatch-sTexts[SETTXT_MAX_HUBS_LIMIT_MSG];
            memcpy(msg+imsgLen, sTexts[SETTXT_MAX_HUBS_LIMIT_MSG], iLen);
            imsgLen += (int)iLen;
        }

#ifdef _WIN32
        int iret = sprintf(msg+imsgLen, "%I16d", iShorts[SETSHORT_MAX_HUBS_LIMIT]);
#else
		int iret = sprintf(msg+imsgLen, "%d", iShorts[SETSHORT_MAX_HUBS_LIMIT]);
#endif
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "SetMan::UpdateMaxHubsLimitMessage") == false) {
            exit(EXIT_FAILURE);
        }

        if(sMatch+7 < sTexts[SETTXT_MAX_HUBS_LIMIT_MSG]+ui16TextsLens[SETTXT_MAX_HUBS_LIMIT_MSG]) {
            size_t iLen = (sTexts[SETTXT_MAX_HUBS_LIMIT_MSG]+ui16TextsLens[SETTXT_MAX_HUBS_LIMIT_MSG])-(sMatch+7);
            memcpy(msg+imsgLen, sMatch+7, iLen);
            imsgLen += (int)iLen;
        }
    } else {
        memcpy(msg, sTexts[SETTXT_MAX_HUBS_LIMIT_MSG], ui16TextsLens[SETTXT_MAX_HUBS_LIMIT_MSG]);
        imsgLen = (int)ui16TextsLens[SETTXT_MAX_HUBS_LIMIT_MSG];
    }

    msg[imsgLen] = '|';
    imsgLen++;

    if(bBools[SETBOOL_MAX_HUBS_LIMIT_REDIR] == true) {
        if(sTexts[SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS] != NULL) {
        	int iret = sprintf(msg+imsgLen, "$ForceMove %s|", sTexts[SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS]);
            imsgLen += iret;
            if(CheckSprintf1(iret, imsgLen, 1024, "SetMan::UpdateMaxHubsLimitMessage1") == false) {
                exit(EXIT_FAILURE);
            }
        }  else if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	memcpy(msg+imsgLen, sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
            imsgLen += (int)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

#ifdef _WIN32
    sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG] = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, imsgLen+1);
#else
	sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG] = (char *) malloc(imsgLen+1);
#endif
    if(sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG] == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string(imsgLen+1)+
			" bytes of memory in SetMan::UpdateMaxHubsLimitMessage!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    memcpy(sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG], msg, imsgLen);
    ui16PreTextsLens[SETPRETXT_MAX_HUBS_LIMIT_MSG] = (uint16_t)imsgLen;
    sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG][ui16PreTextsLens[SETPRETXT_MAX_HUBS_LIMIT_MSG]] = '\0';
}
//---------------------------------------------------------------------------

void SetMan::UpdateNoTagMessage() {
    if(bUpdateLocked == true) {
        return;
    }

    if(sPreTexts[SETPRETXT_NO_TAG_MSG] != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[SETPRETXT_NO_TAG_MSG]) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate memory in SetMan::UpdateNoTagMessage! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(sPreTexts[SETPRETXT_NO_TAG_MSG]);
#endif
        sPreTexts[SETPRETXT_NO_TAG_MSG] = NULL;
        ui16PreTextsLens[SETPRETXT_NO_TAG_MSG] = 0;
    }

    if(iShorts[SETSHORT_NO_TAG_OPTION] == 0) {
        return;
    }

    char msg[1024];
    int imsgLen = sprintf(msg, "<%s> %s|", sPreTexts[SetMan::SETPRETXT_HUB_SEC], sTexts[SETTXT_NO_TAG_MSG]);
    if(CheckSprintf(imsgLen, 1024, "SetMan::UpdateNoTagMessage") == false) {
        exit(EXIT_FAILURE);
    }

    if(iShorts[SETSHORT_NO_TAG_OPTION] == 2) { 
        if(sTexts[SETTXT_NO_TAG_REDIR_ADDRESS] != NULL) {
           	int iret = sprintf(msg+imsgLen, "$ForceMove %s|", sTexts[SETTXT_NO_TAG_REDIR_ADDRESS]);
            imsgLen += iret;
            if(CheckSprintf1(iret, imsgLen, 1024, "SetMan::UpdateNoTagMessage1") == false) {
                exit(EXIT_FAILURE);
            }
        }  else if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	memcpy(msg+imsgLen, sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
            imsgLen += (int)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

#ifdef _WIN32
    sPreTexts[SETPRETXT_NO_TAG_MSG] = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, imsgLen+1);
#else
	sPreTexts[SETPRETXT_NO_TAG_MSG] = (char *) malloc(imsgLen+1);
#endif
    if(sPreTexts[SETPRETXT_NO_TAG_MSG] == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string(imsgLen+1)+
			" bytes of memory in SetMan::UpdateNoTagMessage!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    memcpy(sPreTexts[SETPRETXT_NO_TAG_MSG], msg, imsgLen);
    ui16PreTextsLens[SETPRETXT_NO_TAG_MSG] = (uint16_t)imsgLen;
    sPreTexts[SETPRETXT_NO_TAG_MSG][ui16PreTextsLens[SETPRETXT_NO_TAG_MSG]] = '\0';
}
//---------------------------------------------------------------------------

void SetMan::UpdateTempBanRedirAddress() {
    if(bUpdateLocked == true) {
        return;
    }

    if(sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS]) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate memory in SetMan::UpdateTempBanRedirAddress! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS]);
#endif
        sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] = NULL;
        ui16PreTextsLens[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] = 0;
    }

    char msg[1024];
    int imsgLen = 0;

    if(sTexts[SETTXT_TEMP_BAN_REDIR_ADDRESS] != NULL) {
        imsgLen = sprintf(msg, "$ForceMove %s|", sTexts[SETTXT_TEMP_BAN_REDIR_ADDRESS]);
        if(CheckSprintf(imsgLen, 1024, "SetMan::UpdateTempBanRedirAddress") == false) {
            exit(EXIT_FAILURE);
        }
    } else if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		memcpy(msg, sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
        imsgLen = (int)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
    }

#ifdef _WIN32
    sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, imsgLen+1);
#else
	sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] = (char *) malloc(imsgLen+1);
#endif
    if(sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string(imsgLen+1)+
			" bytes of memory in SetMan::UpdateTempBanRedirAddress!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    memcpy(sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS], msg, imsgLen);
    ui16PreTextsLens[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] = (uint16_t)imsgLen;
    sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS][ui16PreTextsLens[SETPRETXT_TEMP_BAN_REDIR_ADDRESS]] = '\0';
}
//---------------------------------------------------------------------------

void SetMan::UpdatePermBanRedirAddress() {
    if(bUpdateLocked == true) {
        return;
    }

    if(sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS] != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS]) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate memory in SetMan::UpdatePermBanRedirAddress! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS]);
#endif
        sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS] = NULL;
        ui16PreTextsLens[SETPRETXT_PERM_BAN_REDIR_ADDRESS] = 0;
    }

    char msg[1024];
    int imsgLen = 0;

    if(sTexts[SETTXT_PERM_BAN_REDIR_ADDRESS] != NULL) {
        imsgLen = sprintf(msg, "$ForceMove %s|", sTexts[SETTXT_PERM_BAN_REDIR_ADDRESS]);
        if(CheckSprintf(imsgLen, 1024, "SetMan::UpdatePermBanRedirAddress") == false) {
            exit(EXIT_FAILURE);
        }
    } else if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		memcpy(msg, sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
        imsgLen = (int)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
    }

#ifdef _WIN32
    sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS] = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, imsgLen+1);
#else
	sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS] = (char *) malloc(imsgLen+1);
#endif
    if(sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS] == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string(imsgLen+1)+
			" bytes of memory in SetMan::UpdatePermBanRedirAddress!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    memcpy(sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS], msg, imsgLen);
    ui16PreTextsLens[SETPRETXT_PERM_BAN_REDIR_ADDRESS] = (uint16_t)imsgLen;
    sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS][ui16PreTextsLens[SETPRETXT_PERM_BAN_REDIR_ADDRESS]] = '\0';
}
//---------------------------------------------------------------------------

void SetMan::UpdateNickLimitMessage() {
    if(bUpdateLocked == true) {
        return;
    }

    if(sPreTexts[SETPRETXT_NICK_LIMIT_MSG] != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[SETPRETXT_NICK_LIMIT_MSG]) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate memory in SetMan::UpdateNickLimitMessage! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
            AppendSpecialLog(sDbgstr);
        }
#else
		free(sPreTexts[SETPRETXT_NICK_LIMIT_MSG]);
#endif
        sPreTexts[SETPRETXT_NICK_LIMIT_MSG] = NULL;
        ui16PreTextsLens[SETPRETXT_NICK_LIMIT_MSG] = 0;
    }

    char msg[1024];
    int imsgLen = sprintf(msg, "<%s> ", sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
    if(CheckSprintf(imsgLen, 1024, "SetMan::UpdateNickLimitMessage0") == false) {
        exit(EXIT_FAILURE);
    }

    for(uint16_t i = 0; i < ui16TextsLens[SETTXT_NICK_LIMIT_MSG]; i++) {
        if(sTexts[SETTXT_NICK_LIMIT_MSG][i] == '%') {
            if(strncmp(sTexts[SETTXT_NICK_LIMIT_MSG]+i+1, sMin, 5) == 0) {
                if(iShorts[SETSHORT_MIN_NICK_LEN] != 0) {
#ifdef _WIN32
                    int iret = sprintf(msg+imsgLen, "%I16d", iShorts[SETSHORT_MIN_NICK_LEN]);
#else
					int iret = sprintf(msg+imsgLen, "%d", iShorts[SETSHORT_MIN_NICK_LEN]);
#endif
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "SetMan::UpdateNickLimitMessage") == false) {
                        exit(EXIT_FAILURE);
                    }
                } else {
                    memcpy(msg+imsgLen, "0", 1);
                    imsgLen += 1;
                }
                i += (uint16_t)5;
                continue;
            } else if(strncmp(sTexts[SETTXT_NICK_LIMIT_MSG]+i+1, sMax, 5) == 0) {
                if(iShorts[SETSHORT_MAX_NICK_LEN] != 0) {
#ifdef _WIN32
                    int iret = sprintf(msg+imsgLen, "%I16d", iShorts[SETSHORT_MAX_NICK_LEN]);
#else
					int iret = sprintf(msg+imsgLen, "%d", iShorts[SETSHORT_MAX_NICK_LEN]);
#endif
                    imsgLen += iret;
                    if(CheckSprintf1(iret, imsgLen, 1024, "SetMan::UpdateNickLimitMessage1") == false) {
                        exit(EXIT_FAILURE);
                    }
                } else {
                    memcpy(msg+imsgLen, "unlimited", 9);
                    imsgLen += 9;
                }
                i += (uint16_t)5;
                continue;
            }
        }

        msg[imsgLen] = sTexts[SETTXT_NICK_LIMIT_MSG][i];
        imsgLen++;
    }

    msg[imsgLen] = '|';
    imsgLen++;

    if(bBools[SETBOOL_NICK_LIMIT_REDIR] == true) {
        if(sTexts[SETTXT_NICK_LIMIT_REDIR_ADDRESS] != NULL) {
            int iret = sprintf(msg+imsgLen, "$ForceMove %s|", sTexts[SETTXT_NICK_LIMIT_REDIR_ADDRESS]);
            imsgLen += iret;
            if(CheckSprintf1(iret, imsgLen, 1024, "SetMan::UpdateNickLimitMessage2") == false) {
                exit(EXIT_FAILURE);
            }
        }  else if(sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	memcpy(msg+imsgLen, sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
            imsgLen += (int)ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

#ifdef _WIN32
    sPreTexts[SETPRETXT_NICK_LIMIT_MSG] = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, imsgLen+1);
#else
	sPreTexts[SETPRETXT_NICK_LIMIT_MSG] = (char *) malloc(imsgLen+1);
#endif
    if(sPreTexts[SETPRETXT_NICK_LIMIT_MSG] == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string(imsgLen+1)+
			" bytes of memory in SetMan::UpdateNickLimitMessage!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    memcpy(sPreTexts[SETPRETXT_NICK_LIMIT_MSG], msg, imsgLen);
    ui16PreTextsLens[SETPRETXT_NICK_LIMIT_MSG] = (uint16_t)imsgLen;
    sPreTexts[SETPRETXT_NICK_LIMIT_MSG][ui16PreTextsLens[SETPRETXT_NICK_LIMIT_MSG]] = '\0';
}
//---------------------------------------------------------------------------

void SetMan::UpdateMinShare() {
    if(bUpdateLocked == true) {
        return;
    }

	ui64MinShare = (uint64_t)(iShorts[SETSHORT_MIN_SHARE_LIMIT] == 0 ? 0 :
#ifdef _WIN32
		iShorts[SETSHORT_MIN_SHARE_LIMIT] * pow((float)1024, iShorts[SETSHORT_MIN_SHARE_UNITS]));
#else
		iShorts[SETSHORT_MIN_SHARE_LIMIT] * pow(1024, iShorts[SETSHORT_MIN_SHARE_UNITS]));
#endif
}
//---------------------------------------------------------------------------

void SetMan::UpdateMaxShare() {
    if(bUpdateLocked == true) {
        return;
    }

	ui64MaxShare = (uint64_t)(iShorts[SETSHORT_MAX_SHARE_LIMIT] == 0 ? 0 :
#ifdef _WIN32
		iShorts[SETSHORT_MAX_SHARE_LIMIT] * pow((float)1024, iShorts[SETSHORT_MAX_SHARE_UNITS]));
#else
		iShorts[SETSHORT_MAX_SHARE_LIMIT] * pow(1024, iShorts[SETSHORT_MAX_SHARE_UNITS]));
#endif
}
//---------------------------------------------------------------------------

void SetMan::UpdateTCPPorts() {
    if(bUpdateLocked == true) {
        return;
    }

    char * sPort = sTexts[SETTXT_TCP_PORTS];
    uint8_t iActualPort = 0;
    for(uint16_t i = 0; i < ui16TextsLens[SETTXT_TCP_PORTS] && iActualPort < 25; i++) {
        if(sTexts[SETTXT_TCP_PORTS][i] == ';') {
            sTexts[SETTXT_TCP_PORTS][i] = '\0';

            if(iActualPort != 0) {
                iPortNumbers[iActualPort] = (uint16_t)atoi(sPort);
            } else {
#ifdef _WIN32
                EnterCriticalSection(&csSetting);
#else
				pthread_mutex_lock(&mtxSetting);
#endif

                iPortNumbers[iActualPort] = (uint16_t)atoi(sPort);

#ifdef _WIN32
                LeaveCriticalSection(&csSetting);
#else
				pthread_mutex_unlock(&mtxSetting);
#endif
            }

            sTexts[SETTXT_TCP_PORTS][i] = ';';

            sPort = sTexts[SETTXT_TCP_PORTS]+i+1;
            iActualPort++;
            continue;
        }
    }

    if(sPort[0] != '\0') {
        iPortNumbers[iActualPort] = (uint16_t)atoi(sPort);
        iActualPort++;
    }

    while(iActualPort < 25) {
        iPortNumbers[iActualPort] = 0;
        iActualPort++;
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
#ifdef _WIN32
        bBotsSameNick = (stricmp(sTexts[SETTXT_BOT_NICK], sTexts[SETTXT_OP_CHAT_NICK]) == 0);
#else
		bBotsSameNick = (strcasecmp(sTexts[SETTXT_BOT_NICK], sTexts[SETTXT_OP_CHAT_NICK]) == 0);
#endif
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

#ifdef _WIN32
	#ifndef _SERVICE
		hubForm->ReadLanguage();
	
	    if(BansForm != NULL) {
	        BansForm->ReadLanguage();
	    }
	
	    if(infoForm != NULL) {
	        infoForm->ReadLanguage();
	    }
	
	    if(newUserForm != NULL) {
	        newUserForm->ReadLanguage();
	    }
	
	    if(ProfileManForm != NULL) {
	        ProfileManForm->ReadLanguage();
	    }
	
	    if(RangeBansForm != NULL) {
	        RangeBansForm->ReadLanguage();
	    }
	
	    if(RegsForm != NULL) {
	        RegsForm->ReadLanguage();
	    }
	
	    if(ScriptMemoryForm != NULL) {
	        ScriptMemoryForm->ReadLanguage();
	    }
	
	    if(UsersChatForm != NULL) {
	        UsersChatForm->ReadLanguage();
		}
	#endif
#endif
}
//---------------------------------------------------------------------------

void SetMan::UpdateBot(const bool &bNickChanged/* = true*/) {
    if(bUpdateLocked == true) {
        return;
	}

    if(sPreTexts[SETPRETXT_HUB_BOT_MYINFO] != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[SETPRETXT_HUB_BOT_MYINFO]) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate memory in SetMan::UpdateBot! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(sPreTexts[SETPRETXT_HUB_BOT_MYINFO]);
#endif
        sPreTexts[SETPRETXT_HUB_BOT_MYINFO] = NULL;
        ui16PreTextsLens[SETPRETXT_HUB_BOT_MYINFO] = 0;
    }

    if(bBools[SETBOOL_REG_BOT] == false) {
        return;
    }

    char msg[512];
    int imsgLen = sprintf(msg, "$MyINFO $ALL %s %s$ $$%s$$|", sTexts[SETTXT_BOT_NICK], 
        sTexts[SETTXT_BOT_DESCRIPTION] != NULL ? sTexts[SETTXT_BOT_DESCRIPTION] : "", 
        sTexts[SETTXT_BOT_EMAIL] != NULL ? sTexts[SETTXT_BOT_EMAIL] : "");
    if(CheckSprintf(imsgLen, 512, "SetMan::UpdateBot") == false) {
        exit(EXIT_FAILURE);
    }

#ifdef _WIN32
    sPreTexts[SETPRETXT_HUB_BOT_MYINFO] = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, imsgLen+1);
#else
	sPreTexts[SETPRETXT_HUB_BOT_MYINFO] = (char *) malloc(imsgLen+1);
#endif
    if(sPreTexts[SETPRETXT_HUB_BOT_MYINFO] == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string(imsgLen+1)+
			" bytes of memory in SetMan::UpdateBot!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    memcpy(sPreTexts[SETPRETXT_HUB_BOT_MYINFO], msg, imsgLen);
    ui16PreTextsLens[SETPRETXT_HUB_BOT_MYINFO] = (uint16_t)imsgLen;
    sPreTexts[SETPRETXT_HUB_BOT_MYINFO][ui16PreTextsLens[SETPRETXT_HUB_BOT_MYINFO]] = '\0';

    if(ServersS == NULL) {
        return;
    }

    if(bNickChanged == true && (bBotsSameNick == false || ServersS->bActive == false)) {
        colUsers->Add2NickList(sTexts[SETTXT_BOT_NICK], (size_t)ui16TextsLens[SETTXT_BOT_NICK], true);
    }

    colUsers->AddBot2MyInfos(sPreTexts[SETPRETXT_HUB_BOT_MYINFO]);

    if(ServersS->bActive == false) {
        return;
    }

    if(bNickChanged == true && bBotsSameNick == false) {
        int iMsgLen = sprintf(msg, "$Hello %s|", sTexts[SETTXT_BOT_NICK]);
        if(CheckSprintf(iMsgLen, 512, "SetMan::UpdateBot1") == true) {
            globalQ->HStore(msg, iMsgLen);
        }
    }

    globalQ->InfoStore(sPreTexts[SETPRETXT_HUB_BOT_MYINFO], ui16PreTextsLens[SETPRETXT_HUB_BOT_MYINFO]);

    if(bNickChanged == true && bBotsSameNick == false) {
        globalQ->OpListStore(sTexts[SETTXT_BOT_NICK]);
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

        char msg[128];
        int iMsgLen = sprintf(msg, "$Quit %s|", sTexts[SETTXT_BOT_NICK]);
        if(CheckSprintf(iMsgLen, 128, "SetMan::DisableBot") == true) {
            if(bBotsSameNick == true) {
                // PPK ... send Quit only to users without opchat permission...
                User *next = colUsers->llist;
                while(next != NULL) {
                    User *curUser = next;
                    next = curUser->next;
                    if(curUser->iState == User::STATE_ADDED && ProfileMan->IsAllowed(curUser, ProfileManager::ALLOWEDOPCHAT) == false) {
                        UserSendCharDelayed(curUser, msg, iMsgLen);
                    }
                }
            } else {
                globalQ->InfoStore(msg, iMsgLen);
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

    if(bNickChanged == true) {
        if(sPreTexts[SETPRETXT_OP_CHAT_HELLO] != NULL) {
#ifdef _WIN32
            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[SETPRETXT_OP_CHAT_HELLO]) == 0) {
				string sDbgstr = "[BUF] Cannot deallocate memory in SetMan::UpdateOpChat! "+string((uint32_t)GetLastError())+" "+
					string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
				AppendSpecialLog(sDbgstr);
            }
#else
			free(sPreTexts[SETPRETXT_OP_CHAT_HELLO]);
#endif
            sPreTexts[SETPRETXT_OP_CHAT_HELLO] = NULL;
            ui16PreTextsLens[SETPRETXT_OP_CHAT_HELLO] = 0;
        }
    }

    if(sPreTexts[SETPRETXT_OP_CHAT_MYINFO] != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPreTexts[SETPRETXT_OP_CHAT_MYINFO]) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate memory in SetMan::UpdateOpChat1! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(sPreTexts[SETPRETXT_OP_CHAT_MYINFO]);
#endif
        sPreTexts[SETPRETXT_OP_CHAT_MYINFO] = NULL;
        ui16PreTextsLens[SETPRETXT_OP_CHAT_MYINFO] = 0;
    }

    if(bBools[SETBOOL_REG_OP_CHAT] == false) {
        return;
    }

    char msg[512];

    int imsgLen = sprintf(msg, "$Hello %s|", SettingManager->sTexts[SETTXT_OP_CHAT_NICK]);
    if(CheckSprintf(imsgLen, 512, "SetMan::UpdateOpChat") == false) {
        exit(EXIT_FAILURE);
    }

#ifdef _WIN32
    sPreTexts[SETPRETXT_OP_CHAT_HELLO] = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, imsgLen+1);
#else
	sPreTexts[SETPRETXT_OP_CHAT_HELLO] = (char *) malloc(imsgLen+1);
#endif
    if(sPreTexts[SETPRETXT_OP_CHAT_HELLO] == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string(imsgLen+1)+
			" bytes of memory in SetMan::UpdateOpChat!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    memcpy(sPreTexts[SETPRETXT_OP_CHAT_HELLO], msg, imsgLen);
    ui16PreTextsLens[SETPRETXT_OP_CHAT_HELLO] = (uint16_t)imsgLen;
    sPreTexts[SETPRETXT_OP_CHAT_HELLO][ui16PreTextsLens[SETPRETXT_OP_CHAT_HELLO]] = '\0';

    imsgLen = sprintf(msg, "$MyINFO $ALL %s %s$ $$%s$$|", sTexts[SETTXT_OP_CHAT_NICK], 
        sTexts[SETTXT_OP_CHAT_DESCRIPTION] != NULL ? sTexts[SETTXT_OP_CHAT_DESCRIPTION] : "", 
        sTexts[SETTXT_OP_CHAT_EMAIL] != NULL ? sTexts[SETTXT_OP_CHAT_EMAIL] : "");
    if(CheckSprintf(imsgLen, 512, "SetMan::UpdateOpChat1") == false) {
        exit(EXIT_FAILURE);
    }

#ifdef _WIN32
    sPreTexts[SETPRETXT_OP_CHAT_MYINFO] = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, imsgLen+1);
#else
	sPreTexts[SETPRETXT_OP_CHAT_MYINFO] = (char *) malloc(imsgLen+1);
#endif
    if(sPreTexts[SETPRETXT_OP_CHAT_MYINFO] == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string(imsgLen+1)+
			" bytes of memory in SetMan::UpdateOpChat1!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    memcpy(sPreTexts[SETPRETXT_OP_CHAT_MYINFO], msg, imsgLen);
    ui16PreTextsLens[SETPRETXT_OP_CHAT_MYINFO] = (uint16_t)imsgLen;
    sPreTexts[SETPRETXT_OP_CHAT_MYINFO][ui16PreTextsLens[SETPRETXT_OP_CHAT_MYINFO]] = '\0';

	if(bServerRunning == false) {
        return;
    }

    if(bBotsSameNick == false) {
        imsgLen = sprintf(msg, "$OpList %s$$|", sTexts[SETTXT_OP_CHAT_NICK]);
        if(CheckSprintf(imsgLen, 512, "SetMan::UpdateOpChat2") == false) {
            exit(EXIT_FAILURE);
        }

        User *next = colUsers->llist;
        while(next != NULL) {
            User *curUser = next;
            next = curUser->next;
            if(curUser->iState == User::STATE_ADDED && ProfileMan->IsAllowed(curUser, ProfileManager::ALLOWEDOPCHAT) == true) {
                if(bNickChanged == true && (((curUser->ui32BoolBits & User::BIT_SUPPORT_NOHELLO) == User::BIT_SUPPORT_NOHELLO) == false)) {
                    UserSendCharDelayed(curUser, SettingManager->sPreTexts[SETPRETXT_OP_CHAT_HELLO], 
                        SettingManager->ui16PreTextsLens[SETPRETXT_OP_CHAT_HELLO]);
                }
                UserSendCharDelayed(curUser, SettingManager->sPreTexts[SETPRETXT_OP_CHAT_MYINFO], 
                    SettingManager->ui16PreTextsLens[SETPRETXT_OP_CHAT_MYINFO]);
                if(bNickChanged == true) {
                    UserSendCharDelayed(curUser, msg, imsgLen);
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
                if(curUser->iState == User::STATE_ADDED && ProfileMan->IsAllowed(curUser, ProfileManager::ALLOWEDOPCHAT) == true) {
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

    if(UDPThread != NULL) {
        UDPThread->Close();
        UDPThread->WaitFor();
        delete UDPThread;
        UDPThread = NULL;
    }

    if((uint16_t)atoi(sTexts[SETTXT_UDP_PORT]) != 0) {
        UDPThread = new UDPRecvThread();
        if(UDPThread == NULL) {
        	string sDbgstr = "[BUF] Cannot allocate UDPThread!";
#ifdef _WIN32
    		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
        	AppendSpecialLog(sDbgstr);
        	exit(EXIT_FAILURE);
        }

        if(UDPThread->Listen() == true) {
            UDPThread->Resume();
        } else {
            delete UDPThread;
            UDPThread = NULL;
        }
    }
}
//---------------------------------------------------------------------------

void SetMan::UpdateScripting() {
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
