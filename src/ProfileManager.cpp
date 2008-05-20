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
#include "ProfileManager.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "hashRegManager.h"
#include "LanguageManager.h"
#include "ServerManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
ProfileManager *ProfileMan = NULL;
//---------------------------------------------------------------------------

ProfileItem::~ProfileItem() {
    free(sName);
}
//---------------------------------------------------------------------------

ProfileManager::ProfileManager() {
    iProfileCount = 0;

    ProfilesTable = NULL;

    TiXmlDocument doc((PATH+"/cfg/Profiles.xml").c_str());
	if(doc.LoadFile() == false) {
		CreateDefaultProfiles();
		if(doc.LoadFile() == false) {
            AppendLog(LanguageManager->sTexts[LAN_PROFILES_LOAD_FAIL]);
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
                
			ProfileItem *newProfile = CreateProfile(sName);

			if(strlen(sRights) == 32) {
				for(uint8_t i = 0; i < 32; i++) {
					if(sRights[i] == '1') {
						newProfile->bPermissions[i] = true;
					} else {
						newProfile->bPermissions[i] = false;
					}
				}
			} else if(strlen(sRights) == 256) {
				for(uint16_t i = 0; i < 256; i++) {
					if(sRights[i] == '1') {
						newProfile->bPermissions[i] = true;
					} else {
						newProfile->bPermissions[i] = false;
					}
				}
			} else {
				delete newProfile;
				continue;
			}
		}
	} else {
        AppendLog(LanguageManager->sTexts[LAN_PROFILES_LOAD_FAIL]);
		exit(EXIT_FAILURE);
	}
}
//---------------------------------------------------------------------------

ProfileManager::~ProfileManager() {
    SaveProfiles();

    for(uint16_t i = 0; i < iProfileCount; i++) {
        delete ProfilesTable[i];
    }

    free(ProfilesTable);
    ProfilesTable = NULL;
}
//---------------------------------------------------------------------------

void ProfileManager::CreateDefaultProfiles() {  
    const char* profilenames[] = { "Master", "Operator", "VIP", "Reg" };
    const char* profilepermisions[] = {
        "1001111111111111111111111111111111111111111110100011111100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        "1001111110111111111001100011111111100000001110100011111100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        "0000000000000001111000000000000110000000000000000000011100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        "0000000000000000000000000000000110000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
    };

    TiXmlDocument doc((PATH+"/cfg/Profiles.xml").c_str());
    doc.InsertEndChild(TiXmlDeclaration("1.0", "windows-1252", "yes"));
    TiXmlElement profiles("Profiles");

    for(uint8_t i = 0; i < 4;i++) {
        TiXmlElement name("Name");
        name.InsertEndChild(TiXmlText(profilenames[i]));
        
        TiXmlElement permisions("Permissions");
        permisions.InsertEndChild(TiXmlText(profilepermisions[i]));
        
        TiXmlElement profile("Profile");
        profile.InsertEndChild(name);
        profile.InsertEndChild(permisions);
        
        profiles.InsertEndChild(profile);
    }

    doc.InsertEndChild(profiles);
    doc.SaveFile();
}
//---------------------------------------------------------------------------

void ProfileManager::SaveProfiles() {
    char permisionsbits[257];

    TiXmlDocument doc((PATH+"/cfg/Profiles.xml").c_str());
    doc.InsertEndChild(TiXmlDeclaration("1.0", "windows-1252", "yes"));
    TiXmlElement profiles("Profiles");

    for(uint16_t j = 0; j < iProfileCount; j++) {
        TiXmlElement name("Name");
        name.InsertEndChild(TiXmlText(ProfilesTable[j]->sName));
        
        for(unsigned i = 0; i < 256; i++) {
            if(ProfilesTable[j]->bPermissions[i] == false) {
                permisionsbits[i] = '0';
            } else {
                permisionsbits[i] = '1';
            }
        }
        permisionsbits[256] = '\0';
        
        TiXmlElement permisions("Permissions");
        permisions.InsertEndChild(TiXmlText(permisionsbits));
        
        TiXmlElement profile("Profile");
        profile.InsertEndChild(name);
        profile.InsertEndChild(permisions);
        
        profiles.InsertEndChild(profile);
    }

    doc.InsertEndChild(profiles);
    doc.SaveFile();
}
//---------------------------------------------------------------------------

bool ProfileManager::IsAllowed(User * u, const uint32_t &iOption) {
    // profile number -1 = normal user/no profile assigned
    if(u->iProfile == -1)
        return false;
        
    // return right of the profile
    return ProfilesTable[u->iProfile]->bPermissions[iOption];
}
//---------------------------------------------------------------------------

bool ProfileManager::IsProfileAllowed(const int32_t &iProfile, const uint32_t &iOption) {
    // profile number -1 = normal user/no profile assigned
    if(iProfile == -1)
        return false;
        
    // return right of the profile
    return ProfilesTable[iProfile]->bPermissions[iOption];
}
//---------------------------------------------------------------------------

int32_t ProfileManager::AddProfile(char * name) {
    for(uint16_t i = 0; i < iProfileCount; i++) {         
        if(strcasecmp(ProfilesTable[i]->sName, name) == 0) {
            return -1;
        }
    }

    uint32_t i = 0;
    while(true) {
        switch(name[i]) {
            case '\0':
                break;
            case '|':
                return -2;
            default:
                if(name[i] < 33) {
                    return -2;
                }
                
                i++;
                continue;
        }

        break;
    }

	CreateProfile(name);

    return (int32_t)(iProfileCount-1);
}
//---------------------------------------------------------------------------

int32_t ProfileManager::GetProfileIndex(const char * name) {
    for(uint16_t i = 0; i < iProfileCount; i++) {      
        if(strcasecmp(ProfilesTable[i]->sName, name) == 0) {
            return i;
        }
    }
    
    return -1;
}
//---------------------------------------------------------------------------

// RemoveProfileByName(name)
// returns: 0 if the name doesnot exists or is a default profile idx 0-3
//          -1 if the profile is in use
//          1 on success
int32_t ProfileManager::RemoveProfileByName(char * name) {
    int32_t idx = -1;
    
    for(uint16_t i = 0; i < iProfileCount; i++) {      
        if(strcasecmp(ProfilesTable[i]->sName, name) == 0) {
            idx = i;
            break;
        }
    }
    
    // PPK ... check this !!! else crash >:-]
    if(idx == -1)
        return 0;

    RegUser *next = hashRegManager->RegListS;
    while(next != NULL) {
        RegUser *curReg = next;
		next = curReg->next;
		if(curReg->iProfile == idx) {
            //MessageBox(Application->Handle, "Failed to delete a profile. Profile is in use.", "Failed!", MB_OK|MB_ICONEXCLAMATION);
            return -1;
        }
    }
    
    iProfileCount--;
    
    delete ProfilesTable[idx];
    
	for(int32_t i = idx; i < iProfileCount; i++) {
        ProfilesTable[i] = ProfilesTable[i+1];
    }

    // Update profiles for online users
    if(bServerRunning == true) {
        User *nextUser = colUsers->llist;
        while(nextUser != NULL) {
            User *curUser = nextUser;
            nextUser = curUser->next;
            
            if(curUser->iProfile > idx) {
                curUser->iProfile--;
            }
        }
    }

    // Update profiles for registered users
    next = hashRegManager->RegListS;
    while(next != NULL) {
        RegUser *curReg = next;
		next = curReg->next;
        if(curReg->iProfile > idx) {
            curReg->iProfile--;
        }
    }

    ProfilesTable = (ProfileItem **) realloc(ProfilesTable, iProfileCount*sizeof(ProfileItem *));
    if(ProfilesTable == NULL) {
		string sDbgstr = "[BUF] Cannot reallocate ProfilesTable in ProfileManager::RemoveProfileByName!";
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    return 1;
}
//---------------------------------------------------------------------------

ProfileItem * ProfileManager::CreateProfile(const char * name) {
    iProfileCount++;
    
    if(ProfilesTable == NULL) {
        ProfilesTable = (ProfileItem **) malloc(iProfileCount*sizeof(ProfileItem *));
        if(ProfilesTable == NULL) {
			string sDbgstr = "[BUF] Cannot allocate ProfilesTable in ProfileManager::CreateProfile!";
			AppendSpecialLog(sDbgstr);
            exit(EXIT_FAILURE);
        }
    } else {
        ProfilesTable = (ProfileItem **) realloc(ProfilesTable, iProfileCount*sizeof(ProfileItem *));
        if(ProfilesTable == NULL) {
			string sDbgstr = "[BUF] Cannot reallocate ProfilesTable in ProfileManager::CreateProfile!";
			AppendSpecialLog(sDbgstr);
            exit(EXIT_FAILURE);
        }
    }

    ProfileItem *newProfile = new ProfileItem();
    if(newProfile == NULL) {
		string sDbgstr = "[BUF] Cannot allocate newProfile in ProfileManager::CreateProfile!";
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }
 
    size_t iLen = strlen(name);
    newProfile->sName = (char *) malloc(iLen+1);
    if(newProfile->sName == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)iLen)+
			" bytes of memory in ProfileManager::CreateProfile for newProfile->sName!";
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    } 
    memcpy(newProfile->sName, name, iLen);
    newProfile->sName[iLen] = '\0';

    for(uint16_t i = 0; i < 256; i++) {
        newProfile->bPermissions[i] = false;
    }
    
    ProfilesTable[iProfileCount-1] = newProfile;

    return newProfile;
}
//---------------------------------------------------------------------------

void ProfileManager::MoveProfileDown(const uint16_t &iProfile) {
    ProfileItem *first = ProfilesTable[iProfile];
    ProfileItem *second = ProfilesTable[iProfile+1];
    
    ProfilesTable[iProfile+1] = first;
    ProfilesTable[iProfile] = second;

    RegUser *nextReg = hashRegManager->RegListS;

	while(nextReg != NULL) {
        RegUser *curReg = nextReg;
		nextReg = curReg->next;

		if(curReg->iProfile == iProfile) {
			curReg->iProfile++;
		} else if(curReg->iProfile == iProfile+1) {
			curReg->iProfile--;
		}
	}

	hashRegManager->Save();

    if(colUsers == NULL) {
        return;
    }

    User *nextUser = colUsers->llist;

	while(nextUser != NULL) {
        User *curUser = nextUser;
		nextUser = curUser->next;

		if(curUser->iProfile == (int32_t)iProfile) {
			curUser->iProfile++;
		} else if(curUser->iProfile == (int32_t)(iProfile+1)) {
			curUser->iProfile--;
		}
    }
}
//---------------------------------------------------------------------------

void ProfileManager::MoveProfileUp(const uint16_t &iProfile) {
    ProfileItem *first = ProfilesTable[iProfile];
    ProfileItem *second = ProfilesTable[iProfile-1];
    
    ProfilesTable[iProfile-1] = first;
    ProfilesTable[iProfile] = second;

	RegUser *nextReg = hashRegManager->RegListS;

	while(nextReg != NULL) {
		RegUser *curReg = nextReg;
		nextReg = curReg->next;

        if(curReg->iProfile == iProfile) {
			curReg->iProfile--;
		} else if(curReg->iProfile == iProfile-1) {
			curReg->iProfile++;
		}
	}

    hashRegManager->Save();

    if(colUsers == NULL) {
        return;
    }

    User *nextUser = colUsers->llist;

    while(nextUser != NULL) {
        User *curUser = nextUser;
		nextUser = curUser->next;

		if(curUser->iProfile == (int32_t)iProfile) {
			curUser->iProfile--;
		} else if(curUser->iProfile == (int32_t)(iProfile-1)) {
			curUser->iProfile++;
		}
    }
}
//---------------------------------------------------------------------------

void ProfileManager::ChangeProfileName(const uint16_t &iProfile, char * sName, const size_t &iLen) {
    free(ProfilesTable[iProfile]->sName);

    ProfilesTable[iProfile]->sName = NULL;

    ProfilesTable[iProfile]->sName = (char *) malloc(iLen+1);
    if(ProfilesTable[iProfile]->sName == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)iLen)+
            " bytes of memory in ProfileManager::ChangeProfileName for ProfilesTable[iProfile]->sName!";
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    } 
	memcpy(ProfilesTable[iProfile]->sName, sName, iLen);
    ProfilesTable[iProfile]->sName[iLen] = '\0';
}
//---------------------------------------------------------------------------

void ProfileManager::ChangeProfilePermission(const uint16_t &iProfile, const size_t &iId, const bool &bValue) {
    ProfilesTable[iProfile]->bPermissions[iId] = bValue;
}
//---------------------------------------------------------------------------
