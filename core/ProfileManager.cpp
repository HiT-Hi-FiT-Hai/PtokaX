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
#include "ProfileManager.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "hashRegManager.h"
#include "LanguageManager.h"
#include "PXBReader.h"
#include "ServerManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
    #include "../gui.win/ProfilesDialog.h"
    #include "../gui.win/RegisteredUserDialog.h"
    #include "../gui.win/RegisteredUsersDialog.h"
#endif
//---------------------------------------------------------------------------
clsProfileManager * clsProfileManager::mPtr = NULL;
//---------------------------------------------------------------------------
static const char sPtokaXProfiles[] = "PtokaX Profiles";
static const size_t szPtokaXProfilesLen = sizeof(sPtokaXProfiles)-1;
static const char sProfilePermissionIds[] = // PN reserved for profile name!
		"OP" // HASKEYICON
		"DG" // NODEFLOODGETNICKLIST
		"DM" // NODEFLOODMYINFO
		"DS" // NODEFLOODSEARCH
		"DP" // NODEFLOODPM
		"DN" // NODEFLOODMAINCHAT
		"MM" // MASSMSG
		"TO" // TOPIC
		"TB" // TEMP_BAN
		"RT" // REFRESHTXT
		"NT" // NOTAGCHECK
		"TU" // TEMP_UNBAN
		"DR" // DELREGUSER
		"AR" // ADDREGUSER
		"NC" // NOCHATLIMITS
		"NH" // NOMAXHUBCHECK
		"NR" // NOSLOTHUBRATIO
		"NS" // NOSLOTCHECK
		"NA" // NOSHARELIMIT
		"CP" // CLRPERMBAN
		"CT" // CLRTEMPBAN
		"GI" // GETINFO
		"GB" // GETBANLIST
		"RS" // RSTSCRIPTS
		"RH" // RSTHUB
		"TP" // TEMPOP
		"GG" // GAG
		"RE" // REDIRECT
		"BN" // BAN
		"KI" // KICK
		"DR" // DROP
		"EF" // ENTERFULLHUB
		"EB" // ENTERIFIPBAN
		"AO" // ALLOWEDOPCHAT
		"UI" // SENDALLUSERIP
		"RB" // RANGE_BAN
		"RU" // RANGE_UNBAN
		"RT" // RANGE_TBAN
		"RV" // RANGE_TUNBAN
		"GR" // GET_RANGE_BANS
		"CR" // CLR_RANGE_BANS
		"CU" // CLR_RANGE_TBANS
		"UN" // UNBAN
		"NT" // NOSEARCHLIMITS
		"SM" // SENDFULLMYINFOS
		"NI" // NOIPCHECK
		"CL" // CLOSE
		"DC" // NODEFLOODCTM
		"DR" // NODEFLOODRCTM
		"DT" // NODEFLOODSR
		"DU" // NODEFLOODRECV
		"CI" // NOCHATINTERVAL
		"PI" // NOPMINTERVAL
		"SI" // NOSEARCHINTERVAL
		"UI" // NOUSRSAMEIP
		"RT" // NORECONNTIME
;
static const size_t szProfilePermissionIdsLen = sizeof(sProfilePermissionIds)-1;
//---------------------------------------------------------------------------

ProfileItem::ProfileItem() : sName(NULL) {
	for(uint16_t ui16i = 0; ui16i < 256; ui16i++) {
        bPermissions[ui16i] = false;
    }
}
//---------------------------------------------------------------------------

ProfileItem::~ProfileItem() {
#ifdef _WIN32
    if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sName) == 0) {
        AppendDebugLog("%s - [MEM] Cannot deallocate sName in ProfileItem::~ProfileItem\n");
    }
#else
	free(sName);
#endif
}
//---------------------------------------------------------------------------
void clsProfileManager::Load() {
    PXBReader pxbProfiles;

    // Open profiles file
#ifdef _WIN32
    if(pxbProfiles.OpenFileRead((clsServerManager::sPath + "\\cfg\\Profiles.pxb").c_str(), NORECONNTIME+2) == false) {
#else
    if(pxbProfiles.OpenFileRead((clsServerManager::sPath + "/cfg/Profiles.pxb").c_str(), NORECONNTIME+2) == false) {
#endif
    	AppendDebugLog("%s - [ERR] Cannot open Profiles.pxb in clsProfileManager::Load\n");
        return;
    }

    // Read file header
    uint16_t ui16Identificators[NORECONNTIME+2];
    ui16Identificators[0] = *((uint16_t *)"FI");
    ui16Identificators[1] = *((uint16_t *)"FV");

    if(pxbProfiles.ReadNextItem(ui16Identificators, 2) == false) {
        return;
    }

    // Check header if we have correct file
    if(pxbProfiles.ui16ItemLengths[0] != szPtokaXProfilesLen || strncmp((char *)pxbProfiles.pItemDatas[0], sPtokaXProfiles, szPtokaXProfilesLen) != 0) {
        return;
    }

    {
        uint32_t ui32FileVersion = ntohl(*((uint32_t *)(pxbProfiles.pItemDatas[1])));

        if(ui32FileVersion < 1) {
            return;
        }
    }

    // Read settings =)
    ui16Identificators[0] = *((uint16_t *)"PN");
    memcpy(ui16Identificators+1, sProfilePermissionIds, szProfilePermissionIdsLen);

    bool bSuccess = pxbProfiles.ReadNextItem(ui16Identificators, NORECONNTIME+2);

    while(bSuccess == true) {
    	ProfileItem * pNewProfile = CreateProfile((char *)pxbProfiles.pItemDatas[0]);

        for(uint16_t ui16i = 0; ui16i <= NORECONNTIME; ui16i++) {
			if(((char *)pxbProfiles.pItemDatas[ui16i+1])[0] == '0') {
				pNewProfile->bPermissions[ui16i] = false;
			} else {
				pNewProfile->bPermissions[ui16i] = true;
			}
        }

        bSuccess = pxbProfiles.ReadNextItem(ui16Identificators, NORECONNTIME+2);
    }
}
//---------------------------------------------------------------------------

void clsProfileManager::LoadXML() {
#ifdef _WIN32
    TiXmlDocument doc((clsServerManager::sPath+"\\cfg\\Profiles.xml").c_str());
#else
	TiXmlDocument doc((clsServerManager::sPath+"/cfg/Profiles.xml").c_str());
#endif
	if(doc.LoadFile() == false) {
         if(doc.ErrorId() != TiXmlBase::TIXML_ERROR_OPENING_FILE && doc.ErrorId() != TiXmlBase::TIXML_ERROR_DOCUMENT_EMPTY) {
            int iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "Error loading file Profiles.xml. %s (Col: %d, Row: %d)", doc.ErrorDesc(), doc.Column(), doc.Row());
			CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsProfileManager::LoadXML");
#ifdef _BUILD_GUI
			::MessageBox(NULL, clsServerManager::pGlobalBuffer, g_sPtokaXTitle, MB_OK | MB_ICONERROR);
#else
			AppendLog(clsServerManager::pGlobalBuffer);
#endif
            exit(EXIT_FAILURE);
        }
	}

	TiXmlHandle cfg(&doc);
	TiXmlNode *profiles = cfg.FirstChild("Profiles").Node();
	if(profiles != NULL) {
		TiXmlNode *child = NULL;
		while((child = profiles->IterateChildren(child)) != NULL) {
			TiXmlNode *profile = child->FirstChild("Name");

			if(profile == NULL || (profile = profile->FirstChild()) == NULL) {
				continue;
			}

			const char *sName = profile->Value();

			if((profile = child->FirstChildElement("Permissions")) == NULL ||
				(profile = profile->FirstChild()) == NULL) {
				continue;
			}

			const char *sRights = profile->Value();

			ProfileItem * pNewProfile = CreateProfile(sName);

			if(strlen(sRights) == 32) {
				for(uint8_t ui8i = 0; ui8i < 32; ui8i++) {
					if(sRights[ui8i] == '1') {
						pNewProfile->bPermissions[ui8i] = true;
					} else {
						pNewProfile->bPermissions[ui8i] = false;
					}
				}
			} else if(strlen(sRights) == 256) {
				for(uint16_t ui8i = 0; ui8i < 256; ui8i++) {
					if(sRights[ui8i] == '1') {
						pNewProfile->bPermissions[ui8i] = true;
					} else {
						pNewProfile->bPermissions[ui8i] = false;
					}
				}
			} else {
				delete pNewProfile;
				continue;
			}
		}
	} else {
#ifdef _BUILD_GUI
		::MessageBox(NULL, clsLanguageManager::mPtr->sTexts[LAN_PROFILES_LOAD_FAIL], g_sPtokaXTitle, MB_OK | MB_ICONERROR);
#else
		AppendLog(clsLanguageManager::mPtr->sTexts[LAN_PROFILES_LOAD_FAIL]);
#endif
		exit(EXIT_FAILURE);
	}
}
//---------------------------------------------------------------------------

clsProfileManager::clsProfileManager() : ppProfilesTable(NULL), ui16ProfileCount(0) {
	#ifdef _WIN32
    if(FileExist((clsServerManager::sPath + "\\cfg\\Profiles.pxb").c_str()) == true) {
#else
    if(FileExist((clsServerManager::sPath + "/cfg/Profiles.pxb").c_str()) == true) {
#endif
        Load();
        return;
#ifdef _WIN32
    } else if(FileExist((clsServerManager::sPath + "\\cfg\\Profiles.xml").c_str()) == true) {
#else
    } else if(FileExist((clsServerManager::sPath + "/cfg/Profiles.xml").c_str()) == true) {
#endif
        LoadXML();
        return;
    } else {
	    const char * sProfileNames[] = { "Master", "Operator", "VIP", "Reg" };
	    const char * sProfilePermisions[] = {
	        "10011111111111111111111111111111111111111111101000111111",
	        "10011111101111111110011000111111111000000011101000111111",
	        "00000000000000011110000000000001100000000000000000000111",
	        "00000000000000000000000000000001100000000000000000000000"
	    };

		for(uint8_t ui8i = 0; ui8i < 4; ui8i++) {
			ProfileItem * pNewProfile = CreateProfile(sProfileNames[ui8i]);

			for(uint8_t ui8j = 0; ui8j < strlen(sProfilePermisions[ui8i]); ui8j++) {
				if(sProfilePermisions[ui8i][ui8j] == '1') {
					pNewProfile->bPermissions[ui8j] = true;
				} else {
					pNewProfile->bPermissions[ui8j] = false;
				}
			}
		}

	    SaveProfiles();
	}

}
//---------------------------------------------------------------------------

clsProfileManager::~clsProfileManager() {
    SaveProfiles();

    for(uint16_t ui16i = 0; ui16i < ui16ProfileCount; ui16i++) {
        delete ppProfilesTable[ui16i];
    }

#ifdef _WIN32
    if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)ppProfilesTable) == 0) {
        AppendDebugLog("%s - [MEM] Cannot deallocate ProfilesTable in clsProfileManager::~clsProfileManager\n");
    }
#else
	free(ppProfilesTable);
#endif
    ppProfilesTable = NULL;
}
//---------------------------------------------------------------------------

void clsProfileManager::SaveProfiles() {
    PXBReader pxbProfiles;

    // Open profiles file
#ifdef _WIN32
    if(pxbProfiles.OpenFileSave((clsServerManager::sPath + "\\cfg\\Profiles.pxb").c_str(), NORECONNTIME+2) == false) {
#else
    if(pxbProfiles.OpenFileSave((clsServerManager::sPath + "/cfg/Profiles.pxb").c_str(), NORECONNTIME+2) == false) {
#endif
		AppendDebugLog("%s - [ERR] Cannot open Profiles.pxb in clsProfileManager::SaveProfiles\n");
        return;
    }

    // Write file header
    pxbProfiles.sItemIdentifiers[0] = 'F';
    pxbProfiles.sItemIdentifiers[1] = 'I';
    pxbProfiles.ui16ItemLengths[0] = (uint16_t)szPtokaXProfilesLen;
    pxbProfiles.pItemDatas[0] = (void *)sPtokaXProfiles;
    pxbProfiles.ui8ItemValues[0] = PXBReader::PXB_STRING;

    pxbProfiles.sItemIdentifiers[2] = 'F';
    pxbProfiles.sItemIdentifiers[3] = 'V';
    pxbProfiles.ui16ItemLengths[1] = 4;
    uint32_t ui32Version = 1;
    pxbProfiles.pItemDatas[1] = (void *)&ui32Version;
    pxbProfiles.ui8ItemValues[1] = PXBReader::PXB_FOUR_BYTES;

    if(pxbProfiles.WriteNextItem(szPtokaXProfilesLen+4, 2) == false) {
        return;
    }

    pxbProfiles.sItemIdentifiers[0] = 'P';
    pxbProfiles.sItemIdentifiers[1] = 'N';
    pxbProfiles.ui8ItemValues[0] = PXBReader::PXB_STRING;

	memcpy(pxbProfiles.sItemIdentifiers+2, sProfilePermissionIds, szProfilePermissionIdsLen);
	memset(pxbProfiles.ui8ItemValues+1, PXBReader::PXB_BYTE, NORECONNTIME+1);

    for(uint16_t ui16i = 0; ui16i < ui16ProfileCount; ui16i++) {
        pxbProfiles.ui16ItemLengths[0] = (uint16_t)strlen(ppProfilesTable[ui16i]->sName);
        pxbProfiles.pItemDatas[0] = (void *)ppProfilesTable[ui16i]->sName;
        pxbProfiles.ui8ItemValues[0] = PXBReader::PXB_STRING;

        for(uint16_t ui16j = 0; ui16j <= NORECONNTIME; ui16j++) {
	        pxbProfiles.ui16ItemLengths[ui16j+1] = 1;
	        pxbProfiles.pItemDatas[ui16j+1] = (ppProfilesTable[ui16i]->bPermissions[ui16j] == true ? (void *)1 : 0);
	        pxbProfiles.ui8ItemValues[ui16j+1] = PXBReader::PXB_BYTE;
        }

        if(pxbProfiles.WriteNextItem(pxbProfiles.ui16ItemLengths[0] + NORECONNTIME+1, NORECONNTIME+2) == false) {
            break;
        }
    }

    pxbProfiles.WriteRemaining();
}
//---------------------------------------------------------------------------

bool clsProfileManager::IsAllowed(User * u, const uint32_t &iOption) const {
    // profile number -1 = normal user/no profile assigned
    if(u->i32Profile == -1)
        return false;
        
    // return right of the profile
    return ppProfilesTable[u->i32Profile]->bPermissions[iOption];
}
//---------------------------------------------------------------------------

bool clsProfileManager::IsProfileAllowed(const int32_t &iProfile, const uint32_t &iOption) const {
    // profile number -1 = normal user/no profile assigned
    if(iProfile == -1)
        return false;
        
    // return right of the profile
    return ppProfilesTable[iProfile]->bPermissions[iOption];
}
//---------------------------------------------------------------------------

int32_t clsProfileManager::AddProfile(char * name) {
    for(uint16_t ui16i = 0; ui16i < ui16ProfileCount; ui16i++) {
		if(strcasecmp(ppProfilesTable[ui16i]->sName, name) == 0) {
            return -1;
        }
    }

    uint32_t ui32j = 0;
    while(true) {
        switch(name[ui32j]) {
            case '\0':
                break;
            case '|':
                return -2;
            default:
                if(name[ui32j] < 33) {
                    return -2;
                }
                
                ui32j++;
                continue;
        }

        break;
    }

    CreateProfile(name);

#ifdef _BUILD_GUI
    if(clsProfilesDialog::mPtr != NULL) {
        clsProfilesDialog::mPtr->AddProfile();
    }
#endif

#ifdef _BUILD_GUI
    if(clsRegisteredUserDialog::mPtr != NULL) {
        clsRegisteredUserDialog::mPtr->UpdateProfiles();
    }
#endif

    return (int32_t)(ui16ProfileCount-1);
}
//---------------------------------------------------------------------------

int32_t clsProfileManager::GetProfileIndex(const char * name) {
    for(uint16_t ui16i = 0; ui16i < ui16ProfileCount; ui16i++) {
		if(strcasecmp(ppProfilesTable[ui16i]->sName, name) == 0) {
            return ui16i;
        }
    }
    
    return -1;
}
//---------------------------------------------------------------------------

// RemoveProfileByName(name)
// returns: 0 if the name doesnot exists or is a default profile idx 0-3
//          -1 if the profile is in use
//          1 on success
int32_t clsProfileManager::RemoveProfileByName(char * name) {
    for(uint16_t ui16i = 0; ui16i < ui16ProfileCount; ui16i++) {
		if(strcasecmp(ppProfilesTable[ui16i]->sName, name) == 0) {
            return (RemoveProfile(ui16i) == true ? 1 : -1);
        }
    }
    
    return 0;
}

//---------------------------------------------------------------------------

bool clsProfileManager::RemoveProfile(const uint16_t &iProfile) {
    RegUser * curReg = NULL,
        * next = clsRegManager::mPtr->pRegListS;

    while(next != NULL) {
        curReg = next;
		next = curReg->pNext;

		if(curReg->ui16Profile == iProfile) {
            //Profile in use can't be deleted!
            return false;
        }
    }
    
    ui16ProfileCount--;

#ifdef _BUILD_GUI
    if(clsProfilesDialog::mPtr != NULL) {
        clsProfilesDialog::mPtr->RemoveProfile(iProfile);
    }
#endif
    
    delete ppProfilesTable[iProfile];
    
	for(uint16_t ui16i = iProfile; ui16i < ui16ProfileCount; ui16i++) {
        ppProfilesTable[ui16i] = ppProfilesTable[ui16i+1];
    }

    // Update profiles for online users
    if(clsServerManager::bServerRunning == true) {
        User * curUser = NULL,
            * nextUser = clsUsers::mPtr->pListS;

        while(nextUser != NULL) {
            curUser = nextUser;
            nextUser = curUser->pNext;
            
            if(curUser->i32Profile > iProfile) {
                curUser->i32Profile--;
            }
        }
    }

    // Update profiles for registered users
    next = clsRegManager::mPtr->pRegListS;
    while(next != NULL) {
        curReg = next;
		next = curReg->pNext;
        if(curReg->ui16Profile > iProfile) {
            curReg->ui16Profile--;
        }
    }

    ProfileItem ** pOldTable = ppProfilesTable;
#ifdef _WIN32
    ppProfilesTable = (ProfileItem **)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldTable, ui16ProfileCount*sizeof(ProfileItem *));
#else
	ppProfilesTable = (ProfileItem **)realloc(pOldTable, ui16ProfileCount*sizeof(ProfileItem *));
#endif
    if(ppProfilesTable == NULL) {
        ppProfilesTable = pOldTable;

		AppendDebugLog("%s - [MEM] Cannot reallocate ProfilesTable in clsProfileManager::RemoveProfile\n");
    }

#ifdef _BUILD_GUI
    if(clsRegisteredUserDialog::mPtr != NULL) {
        clsRegisteredUserDialog::mPtr->UpdateProfiles();
    }

	if(clsRegisteredUsersDialog::mPtr != NULL) {
		clsRegisteredUsersDialog::mPtr->UpdateProfiles();
	}
#endif

    return true;
}
//---------------------------------------------------------------------------

ProfileItem * clsProfileManager::CreateProfile(const char * name) {
    ProfileItem ** pOldTable = ppProfilesTable;
#ifdef _WIN32
    if(ppProfilesTable == NULL) {
        ppProfilesTable = (ProfileItem **)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (ui16ProfileCount+1)*sizeof(ProfileItem *));
    } else {
        ppProfilesTable = (ProfileItem **)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldTable, (ui16ProfileCount+1)*sizeof(ProfileItem *));
    }
#else
	ppProfilesTable = (ProfileItem **)realloc(pOldTable, (ui16ProfileCount+1)*sizeof(ProfileItem *));
#endif
    if(ppProfilesTable == NULL) {
#ifdef _WIN32
        HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldTable);
#else
        free(pOldTable);
#endif
		AppendDebugLog("%s - [MEM] Cannot (re)allocate ProfilesTable in clsProfileManager::CreateProfile\n");
        exit(EXIT_FAILURE);
    }

    ProfileItem * pNewProfile = new (std::nothrow) ProfileItem();
    if(pNewProfile == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pNewProfile in clsProfileManager::CreateProfile\n");
        exit(EXIT_FAILURE);
    }
 
    size_t szLen = strlen(name);
#ifdef _WIN32
    pNewProfile->sName = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szLen+1);
#else
	pNewProfile->sName = (char *)malloc(szLen+1);
#endif
    if(pNewProfile->sName == NULL) {
		AppendDebugLogFormat("[MEM] Cannot allocate %" PRIu64 " bytes in clsProfileManager::CreateProfile for pNewProfile->sName\n", (uint64_t)szLen);

        exit(EXIT_FAILURE);
    } 
    memcpy(pNewProfile->sName, name, szLen);
    pNewProfile->sName[szLen] = '\0';

    for(uint16_t ui16i = 0; ui16i < 256; ui16i++) {
        pNewProfile->bPermissions[ui16i] = false;
    }
    
    ui16ProfileCount++;

    ppProfilesTable[ui16ProfileCount-1] = pNewProfile;

    return pNewProfile;
}
//---------------------------------------------------------------------------

void clsProfileManager::MoveProfileDown(const uint16_t &iProfile) {
    ProfileItem *first = ppProfilesTable[iProfile];
    ProfileItem *second = ppProfilesTable[iProfile+1];
    
    ppProfilesTable[iProfile+1] = first;
    ppProfilesTable[iProfile] = second;

    RegUser * curReg = NULL,
        * nextReg = clsRegManager::mPtr->pRegListS;

	while(nextReg != NULL) {
        curReg = nextReg;
		nextReg = curReg->pNext;

		if(curReg->ui16Profile == iProfile) {
			curReg->ui16Profile++;
		} else if(curReg->ui16Profile == iProfile+1) {
			curReg->ui16Profile--;
		}
	}

#ifdef _BUILD_GUI
    if(clsProfilesDialog::mPtr != NULL) {
        clsProfilesDialog::mPtr->MoveDown(iProfile);
    }

    if(clsRegisteredUserDialog::mPtr != NULL) {
        clsRegisteredUserDialog::mPtr->UpdateProfiles();
    }

	if(clsRegisteredUsersDialog::mPtr != NULL) {
		clsRegisteredUsersDialog::mPtr->UpdateProfiles();
	}
#endif

    if(clsUsers::mPtr == NULL) {
        return;
    }

    User * curUser = NULL,
        * nextUser = clsUsers::mPtr->pListS;

	while(nextUser != NULL) {
        curUser = nextUser;
		nextUser = curUser->pNext;

		if(curUser->i32Profile == (int32_t)iProfile) {
			curUser->i32Profile++;
		} else if(curUser->i32Profile == (int32_t)(iProfile+1)) {
			curUser->i32Profile--;
		}
    }
}
//---------------------------------------------------------------------------

void clsProfileManager::MoveProfileUp(const uint16_t &iProfile) {
    ProfileItem *first = ppProfilesTable[iProfile];
    ProfileItem *second = ppProfilesTable[iProfile-1];
    
    ppProfilesTable[iProfile-1] = first;
    ppProfilesTable[iProfile] = second;

	RegUser * curReg = NULL,
        * nextReg = clsRegManager::mPtr->pRegListS;

	while(nextReg != NULL) {
		curReg = nextReg;
		nextReg = curReg->pNext;

        if(curReg->ui16Profile == iProfile) {
			curReg->ui16Profile--;
		} else if(curReg->ui16Profile == iProfile-1) {
			curReg->ui16Profile++;
		}
	}

#ifdef _BUILD_GUI
    if(clsProfilesDialog::mPtr != NULL) {
        clsProfilesDialog::mPtr->MoveUp(iProfile);
    }

    if(clsRegisteredUserDialog::mPtr != NULL) {
        clsRegisteredUserDialog::mPtr->UpdateProfiles();
    }

	if(clsRegisteredUsersDialog::mPtr != NULL) {
		clsRegisteredUsersDialog::mPtr->UpdateProfiles();
	}
#endif

    if(clsUsers::mPtr == NULL) {
        return;
    }

    User * curUser = NULL,
        * nextUser = clsUsers::mPtr->pListS;

    while(nextUser != NULL) {
        curUser = nextUser;
		nextUser = curUser->pNext;

		if(curUser->i32Profile == (int32_t)iProfile) {
			curUser->i32Profile--;
		} else if(curUser->i32Profile == (int32_t)(iProfile-1)) {
			curUser->i32Profile++;
		}
    }
}
//---------------------------------------------------------------------------

void clsProfileManager::ChangeProfileName(const uint16_t &iProfile, char * sName, const size_t &szLen) {
    char * sOldName = ppProfilesTable[iProfile]->sName;

#ifdef _WIN32
    ppProfilesTable[iProfile]->sName = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldName, szLen+1);
#else
	ppProfilesTable[iProfile]->sName = (char *)realloc(sOldName, szLen+1);
#endif
    if(ppProfilesTable[iProfile]->sName == NULL) {
        ppProfilesTable[iProfile]->sName = sOldName;

		AppendDebugLogFormat("[MEM] Cannot reallocate %" PRIu64 " bytes in clsProfileManager::ChangeProfileName for ProfilesTable[iProfile]->sName\n", (uint64_t)szLen);

        return;
    } 
	memcpy(ppProfilesTable[iProfile]->sName, sName, szLen);
    ppProfilesTable[iProfile]->sName[szLen] = '\0';

#ifdef _BUILD_GUI
    if(clsRegisteredUserDialog::mPtr != NULL) {
        clsRegisteredUserDialog::mPtr->UpdateProfiles();
    }

	if(clsRegisteredUsersDialog::mPtr != NULL) {
		clsRegisteredUsersDialog::mPtr->UpdateProfiles();
	}
#endif
}
//---------------------------------------------------------------------------

void clsProfileManager::ChangeProfilePermission(const uint16_t &iProfile, const size_t &szId, const bool &bValue) {
    ppProfilesTable[iProfile]->bPermissions[szId] = bValue;
}
//---------------------------------------------------------------------------
