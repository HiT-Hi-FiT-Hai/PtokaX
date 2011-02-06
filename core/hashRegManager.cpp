/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
 * Copyright (C) 2004-2011  Petr Kozelka, PPK at PtokaX dot org

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
#include "hashRegManager.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "globalQueue.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
//---------------------------------------------------------------------------
	#ifndef _SERVICE
		#ifdef _MSC_VER
            #include "../gui.win/RegisteredUsersDialog.h"
		#else
			#include "TRegsForm.h"
		#endif
	#endif
//---------------------------------------------------------------------------
	#ifndef _MSC_VER
		#pragma package(smart_init)
	#endif
#endif
//---------------------------------------------------------------------------
hashRegMan *hashRegManager = NULL;
//---------------------------------------------------------------------------

RegUser::RegUser(char * Nick, char * Pass, const uint16_t &iRegProfile) {
    prev = NULL;
    next = NULL;
    
    size_t iNickLen = strlen(Nick);
#ifdef _WIN32
    sNick = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iNickLen+1);
#else
	sNick = (char *) malloc(iNickLen+1);
#endif
    if(sNick == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iNickLen+1))+
			" bytes of memory for sNick in RegUser::RegUser!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
        AppendSpecialLog(sDbgstr);
        return;
    }   
    memcpy(sNick, Nick, iNickLen);
    sNick[iNickLen] = '\0';
    
    size_t iPassLen = strlen(Pass);
#ifdef _WIN32
    sPass = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iPassLen+1);
#else
	sPass = (char *) malloc(iPassLen+1);
#endif
    if(sPass == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iPassLen+1))+
			" bytes of memory for sPass in RegUser::RegUser!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
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
#ifdef _WIN32
    if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sNick) == 0) {
		string sDbgstr = "[BUF] Cannot deallocate sNick in RegUser::~RegUser! "+string((uint32_t)GetLastError())+" "+
			string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
		AppendSpecialLog(sDbgstr);
    }
#else
	free(sNick);
#endif
    sNick = NULL;

#ifdef _WIN32
    if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPass) == 0) {
		string sDbgstr = "[BUF] Cannot deallocate sPass in RegUser::~RegUser! "+string((uint32_t)GetLastError())+" "+
			string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
		AppendSpecialLog(sDbgstr);
    }
#else
	free(sPass);
#endif
    sPass = NULL;
}
//---------------------------------------------------------------------------

hashRegMan::hashRegMan(void) {
    RegListS = RegListE = NULL;

    for(unsigned int i = 0; i < 65536; i++) {
        table[i] = NULL;
    }
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

bool hashRegMan::AddNew(char * sNick, char * sPasswd, const uint16_t &iProfile) {
    if(Find(sNick, strlen(sNick)) != NULL) {
        return false;
    }

    RegUser *newUser = new RegUser(sNick, sPasswd, iProfile);
    if(newUser == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate newUser in hashRegMan::AddNew!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

	Add(newUser);

#ifdef _WIN32
	#ifndef _SERVICE
		#ifdef _MSC_VER
            if(pRegisteredUsersDialog != NULL) {
                pRegisteredUsersDialog->AddReg(newUser);
            }
		#else
			if(RegsForm != NULL) {
                TListItem *Item = RegsForm->RegList->Items->Add();
                Item->Caption = sNick;
                Item->SubItems->Add(sPasswd);
                Item->SubItems->Add(ProfileMan->ProfilesTable[iProfile]->sName);
                Item->Data = (void *)newUser;
            }
		#endif
	#endif
#endif

    if(bServerRunning == false) {
        return true;
    }

	User *AddedUser = hashManager->FindUser(newUser->sNick, strlen(newUser->sNick));

    if(AddedUser != NULL) {
        bool bAllowedOpChat = ProfileMan->IsAllowed(AddedUser, ProfileManager::ALLOWEDOPCHAT);
        AddedUser->iProfile = iProfile;

        if(((AddedUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
            if(ProfileMan->IsAllowed(AddedUser, ProfileManager::HASKEYICON) == true) {
                AddedUser->ui32BoolBits |= User::BIT_OPERATOR;
            } else {
                AddedUser->ui32BoolBits &= ~User::BIT_OPERATOR;
            }

            if(((AddedUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
				colUsers->Add2OpList(AddedUser->Nick, AddedUser->NickLen);
                globalQ->OpListStore(AddedUser->Nick);

                if(bAllowedOpChat != ProfileMan->IsAllowed(AddedUser, ProfileManager::ALLOWEDOPCHAT)) {
					if(SettingManager->bBools[SETBOOL_REG_OP_CHAT] == true &&
                        (SettingManager->bBools[SETBOOL_REG_BOT] == false || SettingManager->bBotsSameNick == false)) {
                        if(((AddedUser->ui32BoolBits & User::BIT_SUPPORT_NOHELLO) == User::BIT_SUPPORT_NOHELLO) == false) {
                            UserSendCharDelayed(AddedUser, SettingManager->sPreTexts[SetMan::SETPRETXT_OP_CHAT_HELLO],
                                SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_OP_CHAT_HELLO]);
                        }

                        UserSendCharDelayed(AddedUser, SettingManager->sPreTexts[SetMan::SETPRETXT_OP_CHAT_MYINFO],
                            SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_OP_CHAT_MYINFO]);

						char msg[128];
						int imsgLen = sprintf(msg, "$OpList %s$$|", SettingManager->sTexts[SETTXT_OP_CHAT_NICK]);
                        if(CheckSprintf(imsgLen, 128, "hashRegMan::AddNew") == true) {
                            UserSendCharDelayed(AddedUser, msg, imsgLen);
                        }
                    }
                }
            }
        }
    }

    return true;
}
//---------------------------------------------------------------------------

void hashRegMan::Add(RegUser * Reg) {
	Add2Table(Reg);
    
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

void hashRegMan::Add2Table(RegUser * Reg) {
    uint16_t ui16dx = ((uint16_t *)&Reg->ui32Hash)[0];

    if(table[ui16dx] != NULL) {
        table[ui16dx]->hashtableprev = Reg;
        Reg->hashtablenext = table[ui16dx];
    }
    
    table[ui16dx] = Reg;
}
//---------------------------------------------------------------------------

void hashRegMan::ChangeReg(RegUser * pReg, char * sNewPasswd, const uint16_t &ui16NewProfile) {
    if(strcmp(pReg->sPass, sNewPasswd) != 0) {
        size_t iPassLen = strlen(sNewPasswd);
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pReg->sPass) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate pReg->sPass in hashRegMan::ChangeReg! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }

        pReg->sPass = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iPassLen+1);
#else
		free(pReg->sPass);

		pReg->sPass = (char *) malloc(iPassLen+1);
#endif
        if(pReg->sPass == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iPassLen+1))+
				" bytes of memory for sPass in hashRegMan::ChangeReg!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);

            return;
        }
        memcpy(pReg->sPass, sNewPasswd, iPassLen);
        pReg->sPass[iPassLen] = '\0';
    }

    pReg->iProfile = ui16NewProfile;

#ifdef _WIN32
	#ifndef _SERVICE
		#ifdef _MSC_VER
            if(pRegisteredUsersDialog != NULL) {
                pRegisteredUsersDialog->RemoveReg(pReg);
                pRegisteredUsersDialog->AddReg(pReg);
            }
		#else
			if(RegsForm != NULL) {
				TListItem *ListItem = RegsForm->RegList->FindCaption(0, reg->sNick, false, true, false);
				if(ListItem != NULL) {
					ListItem->SubItems->Strings[0] = sPass;
					ListItem->SubItems->Strings[1] = ProfileMan->ProfilesTable[iProfile]->sName;
				}
			}
		#endif
	#endif
#endif

    if(bServerRunning == false) {
        return;
    }

    User *ChangedUser = hashManager->FindUser(pReg->sNick, strlen(pReg->sNick));
    if(ChangedUser != NULL && ChangedUser->iProfile != (int32_t)ui16NewProfile) {
        bool bAllowedOpChat = ProfileMan->IsAllowed(ChangedUser, ProfileManager::ALLOWEDOPCHAT);

        ChangedUser->iProfile = (int32_t)ui16NewProfile;

        if(((ChangedUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) !=
            ProfileMan->IsAllowed(ChangedUser, ProfileManager::HASKEYICON)) {
            if(ProfileMan->IsAllowed(ChangedUser, ProfileManager::HASKEYICON) == true) {
                ChangedUser->ui32BoolBits |= User::BIT_OPERATOR;
                colUsers->Add2OpList(ChangedUser->Nick, ChangedUser->NickLen);
                globalQ->OpListStore(ChangedUser->Nick);
            } else {
                ChangedUser->ui32BoolBits &= ~User::BIT_OPERATOR;
                colUsers->DelFromOpList(ChangedUser->Nick);
            }
        }

        if(bAllowedOpChat != ProfileMan->IsAllowed(ChangedUser, ProfileManager::ALLOWEDOPCHAT)) {
            if(ProfileMan->IsAllowed(ChangedUser, ProfileManager::ALLOWEDOPCHAT) == true) {
                if(SettingManager->bBools[SETBOOL_REG_OP_CHAT] == true &&
                    (SettingManager->bBools[SETBOOL_REG_BOT] == false || SettingManager->bBotsSameNick == false)) {
                    if(((ChangedUser->ui32BoolBits & User::BIT_SUPPORT_NOHELLO) == User::BIT_SUPPORT_NOHELLO) == false) {
                        UserSendCharDelayed(ChangedUser, SettingManager->sPreTexts[SetMan::SETPRETXT_OP_CHAT_HELLO],
                        SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_OP_CHAT_HELLO]);
                    }

                    UserSendCharDelayed(ChangedUser, SettingManager->sPreTexts[SetMan::SETPRETXT_OP_CHAT_MYINFO],
                        SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_OP_CHAT_MYINFO]);

                    char msg[128];
                    int imsgLen = sprintf(msg, "$OpList %s$$|", SettingManager->sTexts[SETTXT_OP_CHAT_NICK]);
                    if(CheckSprintf(imsgLen, 128, "hashRegMan::ChangeReg1") == true) {
                        UserSendCharDelayed(ChangedUser, msg, imsgLen);
                    }
                }
            } else {
                if(SettingManager->bBools[SETBOOL_REG_OP_CHAT] == true && (SettingManager->bBools[SETBOOL_REG_BOT] == false || SettingManager->bBotsSameNick == false)) {
                    char msg[128];
                    int imsgLen = sprintf(msg, "$Quit %s|", SettingManager->sTexts[SETTXT_OP_CHAT_NICK]);
                    if(CheckSprintf(imsgLen, 128, "hashRegMan::ChangeReg2") == true) {
                        UserSendCharDelayed(ChangedUser, msg, imsgLen);
                    }
                }
            }
        }
    }
}
//---------------------------------------------------------------------------

void hashRegMan::Delete(RegUser * pReg, const bool &bFromGui/* = false*/) {
    if(bServerRunning == true) {
        User * pRemovedUser = hashManager->FindUser(pReg->sNick, strlen(pReg->sNick));

        if(pRemovedUser != NULL) {
            pRemovedUser->iProfile = -1;
            if(((pRemovedUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                colUsers->DelFromOpList(pRemovedUser->Nick);
                pRemovedUser->ui32BoolBits &= ~User::BIT_OPERATOR;

                if(SettingManager->bBools[SETBOOL_REG_OP_CHAT] == true && (SettingManager->bBools[SETBOOL_REG_BOT] == false || SettingManager->bBotsSameNick == false)) {
                    char msg[128];
                    int imsgLen = sprintf(msg, "$Quit %s|", SettingManager->sTexts[SETTXT_OP_CHAT_NICK]);
                    if(CheckSprintf(imsgLen, 128, "hashRegMan::Delete") == true) {
                        UserSendCharDelayed(pRemovedUser, msg, imsgLen);
                    }
                }
            }
        }
    }

#ifdef _WIN32
	#ifndef _SERVICE
		#ifdef _MSC_VER
            if(bFromGui == false && pRegisteredUsersDialog != NULL) {
                pRegisteredUsersDialog->RemoveReg(pReg);
            }
		#endif
	#endif
#endif

	Rem(pReg);

    delete pReg;
}
//---------------------------------------------------------------------------

void hashRegMan::Rem(RegUser * Reg) {
	RemFromTable(Reg);
    
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

void hashRegMan::RemFromTable(RegUser * Reg) {
    if(Reg->hashtableprev == NULL) {
        uint16_t ui16dx = ((uint16_t *)&Reg->ui32Hash)[0];

        if(Reg->hashtablenext == NULL) {
            table[ui16dx] = NULL;
        } else {
            Reg->hashtablenext->hashtableprev = NULL;
			table[ui16dx] = Reg->hashtablenext;
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

RegUser* hashRegMan::Find(char * sNick, const size_t &iNickLen) {
    uint32_t ui32Hash = HashNick(sNick, iNickLen);
    uint16_t ui16dx = ((uint16_t *)&ui32Hash)[0];

    RegUser *next = table[ui16dx];

    while(next != NULL) {
        RegUser *cur = next;
        next = cur->hashtablenext;

#ifdef _WIN32
		if(cur->ui32Hash == ui32Hash && stricmp(cur->sNick, sNick) == 0) {
#else
		if(cur->ui32Hash == ui32Hash && strcasecmp(cur->sNick, sNick) == 0) {
#endif
            return cur;
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

RegUser* hashRegMan::Find(User * u) {
    uint16_t ui16dx = ((uint16_t *)&u->ui32NickHash)[0];

	RegUser *next = table[ui16dx];

    while(next != NULL) {
        RegUser *cur = next;
        next = cur->hashtablenext;

#ifdef _WIN32
		if(cur->ui32Hash == u->ui32NickHash && stricmp(cur->sNick, u->Nick) == 0) {
#else
		if(cur->ui32Hash == u->ui32NickHash && strcasecmp(cur->sNick, u->Nick) == 0) {
#endif
            return cur;
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

RegUser* hashRegMan::Find(uint32_t ui32Hash, char * sNick) {
    uint16_t ui16dx = ((uint16_t *)&ui32Hash)[0];

	RegUser *next = table[ui16dx];

    while(next != NULL) {
        RegUser *cur = next;
        next = cur->hashtablenext;

#ifdef _WIN32
        if(cur->ui32Hash == ui32Hash && stricmp(cur->sNick, sNick) == 0) {
#else
		if(cur->ui32Hash == ui32Hash && strcasecmp(cur->sNick, sNick) == 0) {
#endif
            return cur;
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

void hashRegMan::Load(void) {
    uint16_t iProfilesCount = (uint16_t)(ProfileMan->iProfileCount-1);
    bool bIsBuggy = false;

#ifdef _WIN32
    TiXmlDocument doc((PATH+"\\cfg\\RegisteredUsers.xml").c_str());
#else
	TiXmlDocument doc((PATH+"/cfg/RegisteredUsers.xml").c_str());
#endif

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
                
				if(strlen(nick) > 64 || (registereduser = child->FirstChild("Password")) == NULL ||
                    (registereduser = registereduser->FirstChild()) == NULL) {
					continue;
				}

                char *pass = (char *)registereduser->Value();
                
				if(strlen(pass) > 64 || (registereduser = child->FirstChild("Profile")) == NULL ||
                    (registereduser = registereduser->FirstChild()) == NULL) {
					continue;
				}

				uint16_t iProfile = (uint16_t)atoi(registereduser->Value());

				if(iProfile > iProfilesCount) {
                    char msg[1024];
                    int imsgLen = sprintf(msg, "%s %s %s! %s %s.", LanguageManager->sTexts[LAN_USER], nick, LanguageManager->sTexts[LAN_HAVE_NOT_EXIST_PROFILE],
                        LanguageManager->sTexts[LAN_CHANGED_PROFILE_TO], ProfileMan->ProfilesTable[iProfilesCount]->sName);
					CheckSprintf(imsgLen, 1024, "hashRegMan::LoadXmlRegList1");

#ifdef _WIN32
	#ifdef _SERVICE
					AppendLog(msg);
	#else
		#ifndef _MSC_VER
					MessageBox(Application->Handle, msg, LanguageManager->sTexts[LAN_NOTE], MB_OK|MB_ICONEXCLAMATION);
		#endif
	#endif
#else
					AppendLog(msg);
#endif

                    iProfile = iProfilesCount;
                    bIsBuggy = true;
                }

                if(Find((char*)nick, strlen(nick)) == NULL) {
                    RegUser *newUser = new RegUser(nick, pass, iProfile);
                    if(newUser == NULL) {
                    	string sDbgstr = "[BUF] Cannot allocate newUser in hashRegMan::LoadXmlRegList!";
#ifdef _WIN32
						sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
						AppendSpecialLog(sDbgstr);
                    	exit(EXIT_FAILURE);
                    }
					Add(newUser);
                } else {
                    char msg[1024];
                    int imsgLen = sprintf(msg, "%s %s %s! %s.", LanguageManager->sTexts[LAN_USER], nick, LanguageManager->sTexts[LAN_IS_ALREADY_IN_REGS], 
                        LanguageManager->sTexts[LAN_USER_DELETED]);
					CheckSprintf(imsgLen, 1024, "hashRegMan::LoadXmlRegList2");

#ifdef _WIN32
	#ifdef _SERVICE
                    AppendLog(msg);
	#else
		#ifndef _MSC_VER
					MessageBox(Application->Handle, msg, LanguageManager->sTexts[LAN_NOTE], MB_OK|MB_ICONEXCLAMATION);
		#endif
	#endif
#else
					AppendLog(msg);
#endif

                    bIsBuggy = true;
                }
            }
			if(bIsBuggy == true)
				Save();
        }
    }
}
//---------------------------------------------------------------------------

void hashRegMan::Save(void) {
#ifdef _WIN32
    TiXmlDocument doc((PATH+"\\cfg\\RegisteredUsers.xml").c_str());
#else
	TiXmlDocument doc((PATH+"/cfg/RegisteredUsers.xml").c_str());
#endif
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
