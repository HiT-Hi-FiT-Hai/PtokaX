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
#include "TextFileManager.h"
//---------------------------------------------------------------------------
#include "SettingManager.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
TextFileMan *TextFileManager = NULL;
//---------------------------------------------------------------------------

TextFileMan::TextFile::~TextFile() {
    if(sCommand != NULL) {
        free(sCommand);
    }

    if(sText != NULL) {
        free(sText);
    }
}
//---------------------------------------------------------------------------

TextFileMan::TextFileMan() {
    TextFiles = NULL;
}
//---------------------------------------------------------------------------
	
TextFileMan::~TextFileMan() {
    TextFile * next = TextFiles;

    while(next != NULL) {
        TextFile * cur = next;
        next = cur->next;

    	delete cur;
    }
}
//---------------------------------------------------------------------------

bool TextFileMan::ProcessTextFilesCmd(User * u, char * cmd, bool fromPM/* = false*/) {
    TextFile * next = TextFiles;

    while(next != NULL) {
        TextFile * cur = next;
        next = cur->next;

        if(strcasecmp(cur->sCommand, cmd) == 0) {
            bool bInPM = (SettingManager->bBools[SETBOOL_SEND_TEXT_FILES_AS_PM] == true || fromPM);
            int iHubSecLen = (int)SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_HUB_SEC];
            int iChatLen = 0;

            // PPK ... to chat or to PM ???
            if(bInPM == true) {
                iChatLen = 18+u->NickLen+(2*iHubSecLen)+strlen(cur->sText);
            } else {
                iChatLen = 4+iHubSecLen+strlen(cur->sText);
            }

            char *MSG = (char *) malloc(iChatLen);
            if(MSG == NULL) {
        		string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") Cannot allocate "+string(iChatLen)+
        			" bytes of memory in ThubForm::ProcessTextFilesCmd!";
        		AppendSpecialLog(sDbgstr);
                return true;
            }

            if(bInPM == true) {
                int iret = sprintf(MSG, "$To: %s From: %s $<%s> %s", u->Nick, SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                    SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], cur->sText);
                if(CheckSprintf(iret, iChatLen, "ThubForm::ProcessTextFilesCmd1") == false) {
                    return true;
                }
            } else {
                int iret = sprintf(MSG,"<%s> %s", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], cur->sText);
                if(CheckSprintf(iret, iChatLen, "ThubForm::ProcessTextFilesCmd2") == false) {
                    return true;
                }
            }

            UserSendCharDelayed(u, MSG, iChatLen-1);

            free(MSG);

        	return true;
        }
    }

    return false;
}
//---------------------------------------------------------------------------

void TextFileMan::RefreshTextFiles() {
	if(SettingManager->bBools[SETBOOL_ENABLE_TEXT_FILES] == false)
        return;

    TextFile * next = TextFiles;

    while(next != NULL) {
        TextFile * cur = next;
        next = cur->next;

    	delete cur;
    }

    TextFiles = NULL;

    string txtdir = PATH + "/texts/";

    DIR * p_txtdir = opendir(txtdir.c_str());

    if(p_txtdir == NULL) {
        return;
    }

    struct dirent * p_dirent;
    struct stat s_buf;

    while((p_dirent = readdir(p_txtdir)) != NULL) {
        string txtfile = txtdir + p_dirent->d_name;

        if(stat(txtfile.c_str(), &s_buf) != 0 || 
            (s_buf.st_mode & S_IFDIR) != 0 || 
            strcasecmp(p_dirent->d_name + (strlen(p_dirent->d_name)-4), ".txt") != 0) {
            continue;
        }

        FILE *f = fopen(txtfile.c_str(), "rb");
		if(f != NULL) {
			if(s_buf.st_size != 0) {
                TextFile * newtxtfile = new TextFile();
				newtxtfile->sText = (char *) malloc(s_buf.st_size+2);
				if(newtxtfile->sText == NULL) {
					string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)s_buf.st_size+2)+
                        " bytes of memory for sText in ThubForm::RefreshTextFiles!";
					AppendSpecialLog(sDbgstr);
					closedir(p_txtdir);
                    return;
                }
    	        int size = fread(newtxtfile->sText, 1, s_buf.st_size, f);
				newtxtfile->sText[size] = '|';
                newtxtfile->sText[size+1] = '\0';

				newtxtfile->sCommand = (char *) malloc(strlen(p_dirent->d_name)-3);
				if(newtxtfile->sCommand == NULL) {
					string sDbgstr = "[BUF] Cannot allocate "+string(strlen(p_dirent->d_name)-3)+
                        " bytes of memory for sCommand in ThubForm::RefreshTextFiles!";
					AppendSpecialLog(sDbgstr);
					closedir(p_txtdir);
                    return;
                }

                memcpy(newtxtfile->sCommand, p_dirent->d_name, strlen(p_dirent->d_name)-4);
                newtxtfile->sCommand[strlen(p_dirent->d_name)-4] = '\0';

                newtxtfile->prev = NULL;

                if(TextFiles == NULL) {
                    newtxtfile->next = NULL;
                } else {
                    TextFiles->prev = newtxtfile;
                    newtxtfile->next = TextFiles;
                }

                TextFiles = newtxtfile;
    	    }

            fclose(f);
    	}
    }

    closedir(p_txtdir);
}
//---------------------------------------------------------------------------
