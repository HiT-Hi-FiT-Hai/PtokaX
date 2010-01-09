/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
 * Copyright (C) 2004-2010  Petr Kozelka, PPK at PtokaX dot org

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
#include "TextFileManager.h"
//---------------------------------------------------------------------------
#include "SettingManager.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
//---------------------------------------------------------------------------
	#ifndef _MSC_VER
		#pragma package(smart_init)
	#endif
#endif
//---------------------------------------------------------------------------
TextFileMan *TextFileManager = NULL;
//---------------------------------------------------------------------------

TextFileMan::TextFile::~TextFile() {
    if(sCommand != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sCommand) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate sCommand in ~TextFile! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(sCommand);
#endif
    }

    if(sText != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sText) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate sText in ~TextFile! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(sText);
#endif
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

#ifdef _WIN32
        if(stricmp(cur->sCommand, cmd) == 0) {
#else
		if(strcasecmp(cur->sCommand, cmd) == 0) {
#endif
            bool bInPM = (SettingManager->bBools[SETBOOL_SEND_TEXT_FILES_AS_PM] == true || fromPM);
            size_t iHubSecLen = (size_t)SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_HUB_SEC];
            size_t iChatLen = 0;

            // PPK ... to chat or to PM ???
            if(bInPM == true) {
                iChatLen = 18+u->NickLen+(2*iHubSecLen)+strlen(cur->sText);
            } else {
                iChatLen = 4+iHubSecLen+strlen(cur->sText);
            }

#ifdef _WIN32
            char *MSG = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iChatLen);
#else
			char *MSG = (char *) malloc(iChatLen);
#endif
            if(MSG == NULL) {
        		string sDbgstr = "[BUF] "+string(u->Nick,u->NickLen)+" ("+string(u->IP, u->ui8IpLen)+") Cannot allocate "+string((uint64_t)iChatLen)+
        			" bytes of memory in ThubForm::ProcessTextFilesCmd!";
#ifdef _WIN32
        		sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
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

#ifdef _WIN32
            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)MSG) == 0) {
        		string sDbgstr = "[BUF] Cannot deallocate MSG in ThubForm::ProcessTextFilesCmd! "+string((uint32_t)GetLastError())+" "+
        			string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
        		AppendSpecialLog(sDbgstr);
            }
#else
			free(MSG);
#endif

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

#ifdef _WIN32
    struct _finddata_t textfile;
    intptr_t hFile = _findfirst((PATH+"\\texts\\*.txt").c_str(), &textfile);

	if(hFile != -1) {
		do {
			if((textfile.attrib & _A_SUBDIR) != 0 ||
				stricmp(textfile.name+(strlen(textfile.name)-4), ".txt") != 0) {
				continue;
			}

        	FILE *f = fopen((PATH+"\\texts\\"+textfile.name).c_str(), "rb");
			if(f != NULL) {
				if(textfile.size != 0) {
					TextFile * newtxtfile = new TextFile();

					newtxtfile->sText = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, textfile.size+2);

					if(newtxtfile->sText == NULL) {
						string sDbgstr = "[BUF] Cannot allocate "+string((uint32_t)textfile.size+2)+
	                        " bytes of memory for sText in ThubForm::RefreshTextFiles! "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();

						AppendSpecialLog(sDbgstr);

						return;
 					}

					size_t size = fread(newtxtfile->sText, 1, textfile.size, f);

					newtxtfile->sText[size] = '|';
					newtxtfile->sText[size+1] = '\0';

					newtxtfile->sCommand = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, strlen(textfile.name)-3);
					if(newtxtfile->sCommand == NULL) {
						string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(strlen(textfile.name)-3))+
                        " bytes of memory for sCommand in ThubForm::RefreshTextFiles! "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();

						AppendSpecialLog(sDbgstr);

						return;
					}

					memcpy(newtxtfile->sCommand, textfile.name, strlen(textfile.name)-4);
					newtxtfile->sCommand[strlen(textfile.name)-4] = '\0';

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
	    } while(_findnext(hFile, &textfile) == 0);

		_findclose(hFile);
    }
#else
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
    	        size_t size = fread(newtxtfile->sText, 1, s_buf.st_size, f);
				newtxtfile->sText[size] = '|';
                newtxtfile->sText[size+1] = '\0';

				newtxtfile->sCommand = (char *) malloc(strlen(p_dirent->d_name)-3);
				if(newtxtfile->sCommand == NULL) {
					string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(strlen(p_dirent->d_name)-3))+
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
#endif
}
//---------------------------------------------------------------------------
