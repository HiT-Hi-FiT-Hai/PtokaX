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
#include "hashRegManager.h"
//---------------------------------------------------------------------------
#include "hashManager.h"
#include "LanguageManager.h"
#include "ProfileManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
hashRegMan *hashRegManager = NULL;
//---------------------------------------------------------------------------

RegUser::RegUser(char * Nick, char * Pass, const unsigned int &iRegProfile) {
    prev = NULL;
    next = NULL;
    
    int iNickLen = strlen(Nick);
    sNick = (char *) malloc(iNickLen+1);
    if(sNick == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string(iNickLen+1)+
			" bytes of memory for sNick in RegUser::RegUser!";
        AppendSpecialLog(sDbgstr);
        return;
    }   
    memcpy(sNick, Nick, iNickLen);
    sNick[iNickLen] = '\0';
    
    int iPassLen = strlen(Pass);
    sPass = (char *) malloc(iPassLen+1);
    if(sPass == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string(iNickLen+1)+
			" bytes of memory for sPass in RegUser::RegUser!";
		AppendSpecialLog(sDbgstr);
        return;
    }   
    memcpy(sPass, Pass, iPassLen);
    sPass[iPassLen] = '\0';

    tLastBadPass = 0;
    iBadPassCount = 0;
    
    iProfile = iRegProfile;
	ui32Hash = HashNick(sNick, iNickLen);
    hashtableprev = NULL;
    hashtablenext = NULL;
}
//---------------------------------------------------------------------------

RegUser::~RegUser(void) {
    free(sNick);
    sNick = NULL;

    free(sPass);
    sPass = NULL;
}
//---------------------------------------------------------------------------

hashRegMan::hashRegMan(void) {
    RegListS = RegListE = NULL;
}
//---------------------------------------------------------------------------

hashRegMan::~hashRegMan(void) {
    RegUser *next = RegListS;
        
    while(next != NULL) {
        RegUser *curReg = next;
		next = curReg->next;
		delete curReg;
    }
}
//---------------------------------------------------------------------------

bool hashRegMan::AddNewReg(char * sNick, char * sPasswd, const unsigned int &iProfile) {
    if(hashManager->FindReg(sNick, strlen(sNick)) != NULL) {
        return false;
    }

    RegUser *newUser = new RegUser(sNick, sPasswd, iProfile);
    if(newUser == NULL) {
		string sDbgstr = "[BUF] Cannot allocate newUser in hashRegMan::AddNewReg!";
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    AddReg(newUser);
    SaveRegList();

    return true;
}
//---------------------------------------------------------------------------

void hashRegMan::AddReg(RegUser * Reg) {
    hashManager->Add(Reg);
    
    if(RegListE == NULL) {
    	RegListS = Reg;
    	RegListE = Reg;
    } else {
        Reg->prev = RegListE;
    	RegListE->next = Reg;
        RegListE = Reg;
    }

	return;
}
//---------------------------------------------------------------------------

void hashRegMan::RemReg(RegUser * Reg) {
    hashManager->Remove(Reg);
    
    RegUser *prev, *next;
    prev = Reg->prev; next = Reg->next;

    if(prev == NULL) {
        if(next == NULL) {
            RegListS = NULL;
            RegListE = NULL;
        } else {
            next->prev = NULL;
            RegListS = next;
        }
    } else if(next == NULL) {
        prev->next = NULL;
        RegListE = prev;
    } else {
        prev->next = next;
        next->prev = prev;
    }
}
//---------------------------------------------------------------------------

void hashRegMan::LoadRegList(void) {
    int iProfilesCount = ProfileMan->iProfileCount-1;
    bool bIsBuggy = false;

    TiXmlDocument doc((PATH+"/cfg/RegisteredUsers.xml").c_str());
    if(doc.LoadFile()) {
        TiXmlHandle cfg(&doc);
        TiXmlNode *registeredusers = cfg.FirstChild("RegisteredUsers").Node();
        if(registeredusers != NULL) {
            TiXmlNode *child = NULL;
            while((child = registeredusers->IterateChildren(child)) != NULL) {
				TiXmlNode *registereduser = child->FirstChild("Nick");

				if(registereduser == NULL || (registereduser = registereduser->FirstChild()) == NULL) {
					continue;
				}

                char *nick = (char *)registereduser->Value();
                
				if((registereduser = child->FirstChild("Password")) == NULL ||
                    (registereduser = registereduser->FirstChild()) == NULL) {
					continue;
				}

                char *pass = (char *)registereduser->Value();
                
				if((registereduser = child->FirstChild("Profile")) == NULL ||
                    (registereduser = registereduser->FirstChild()) == NULL) {
					continue;
				}

                int iProfile = atoi(registereduser->Value());

                if(iProfile > iProfilesCount) {
                    char msg[1024];
                    int imsgLen = sprintf(msg, "%s %s %s! %s %s.", LanguageManager->sTexts[LAN_USER], nick, LanguageManager->sTexts[LAN_HAVE_NOT_EXIST_PROFILE],
                        LanguageManager->sTexts[LAN_CHANGED_PROFILE_TO], ProfileMan->ProfilesTable[iProfilesCount]->sName);
					CheckSprintf(imsgLen, 1024, "hashRegMan::LoadXmlRegList1");

                    AppendLog(msg);

                    iProfile = iProfilesCount;
                    bIsBuggy = true;
                }

                if(hashManager->FindReg((char*)nick, strlen(nick)) == NULL) {
                    RegUser *newUser = new RegUser(nick, pass, iProfile);
                    if(newUser == NULL) {
						string sDbgstr = "[BUF] Cannot allocate newUser in hashRegMan::LoadXmlRegList!";
						AppendSpecialLog(sDbgstr);
                    	exit(EXIT_FAILURE);
                    }
                    AddReg(newUser);
                } else {
                    char msg[1024];
                    int imsgLen = sprintf(msg, "%s %s %s! %s.", LanguageManager->sTexts[LAN_USER], nick, LanguageManager->sTexts[LAN_IS_ALREADY_IN_REGS], 
                        LanguageManager->sTexts[LAN_USER_DELETED]);
					CheckSprintf(imsgLen, 1024, "hashRegMan::LoadXmlRegList2");

                    AppendLog(msg);

                    bIsBuggy = true;
                }
            }
            if(bIsBuggy == true)
                SaveRegList();
        }
    }
}
//---------------------------------------------------------------------------

void hashRegMan::SaveRegList(void) {
    TiXmlDocument doc((PATH+"/cfg/RegisteredUsers.xml").c_str());
    doc.InsertEndChild(TiXmlDeclaration("1.0", "windows-1252", "yes"));
    TiXmlElement registeredusers("RegisteredUsers");
    RegUser *next = hashRegManager->RegListS;
    while(next != NULL) {
        RegUser *curReg = next;
		next = curReg->next;
		
        TiXmlElement nick("Nick");
        nick.InsertEndChild(TiXmlText(curReg->sNick));
        
        TiXmlElement pass("Password");
        pass.InsertEndChild(TiXmlText(curReg->sPass));
        
        TiXmlElement profile("Profile");
        profile.InsertEndChild(TiXmlText(string(curReg->iProfile).c_str()));
        
        TiXmlElement registereduser("RegisteredUser");
        registereduser.InsertEndChild(nick);
        registereduser.InsertEndChild(pass);
        registereduser.InsertEndChild(profile);
        
        registeredusers.InsertEndChild(registereduser);
    }
    doc.InsertEndChild(registeredusers);
    doc.SaveFile();
}
//---------------------------------------------------------------------------
