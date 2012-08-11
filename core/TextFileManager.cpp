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
#include "TextFileManager.h"
//---------------------------------------------------------------------------
#include "SettingManager.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
TextFileMan *TextFileManager = NULL;
//---------------------------------------------------------------------------

TextFileMan::TextFile::~TextFile() {
#ifdef _WIN32
    if(sCommand != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sCommand) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sCommand in TextFileMan::TextFile::~TextFile\n", 0);
        }
    }
#else
	free(sCommand);
#endif

#ifdef _WIN32
    if(sText != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sText) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sText in TextFileMan::TextFile::~TextFile\n", 0);
        }
    }
#else
	free(sText);
#endif
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

bool TextFileMan::ProcessTextFilesCmd(User * u, char * cmd, bool fromPM/* = false*/) const {
    TextFile * next = TextFiles;

    while(next != NULL) {
        TextFile * cur = next;
        next = cur->next;

		if(strcasecmp(cur->sCommand, cmd) == 0) {
            bool bInPM = (SettingManager->bBools[SETBOOL_SEND_TEXT_FILES_AS_PM] == true || fromPM);
            size_t szHubSecLen = (size_t)SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_HUB_SEC];
            size_t szChatLen = 0;

            // PPK ... to chat or to PM ???
            if(bInPM == true) {
                szChatLen = 18+u->ui8NickLen+(2*szHubSecLen)+strlen(cur->sText);
            } else {
                szChatLen = 4+szHubSecLen+strlen(cur->sText);
            }

#ifdef _WIN32
            char * sMSG = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szChatLen);
#else
			char * sMSG = (char *)malloc(szChatLen);
#endif
            if(sMSG == NULL) {
        		AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sMsg in TextFileMan::ProcessTextFilesCmd\n", (uint64_t)szChatLen);

                return true;
            }

            if(bInPM == true) {
                int iret = sprintf(sMSG, "$To: %s From: %s $<%s> %s", u->sNick, SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                    cur->sText);
                if(CheckSprintf(iret, szChatLen, "TextFileMan::ProcessTextFilesCmd1") == false) {
                    free(sMSG);
                    return true;
                }
            } else {
                int iret = sprintf(sMSG,"<%s> %s", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], cur->sText);
                if(CheckSprintf(iret, szChatLen, "TextFileMan::ProcessTextFilesCmd2") == false) {
                    free(sMSG);
                    return true;
                }
            }

            UserSendCharDelayed(u, sMSG, szChatLen-1);

#ifdef _WIN32
            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMSG) == 0) {
        		AppendDebugLog("%s - [MEM] Cannot deallocate sMSG in TextFileMan::ProcessTextFilesCmd\n", 0);
            }
#else
			free(sMSG);
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
					TextFile * pNewTxtFile = new TextFile();
					if(pNewTxtFile == NULL) {
						AppendDebugLog("%s - [MEM] Cannot allocate pNewTxtFile in TextFileMan::RefreshTextFiles\n", 0);

						fclose(f);
						_findclose(hFile);

                        return;
                    }

					pNewTxtFile->sText = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, textfile.size+2);

					if(pNewTxtFile->sText == NULL) {
						AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sText in TextFileMan::RefreshTextFiles\n", (uint64_t)(textfile.size+2));

						fclose(f);
						_findclose(hFile);

						return;
 					}

					size_t size = fread(pNewTxtFile->sText, 1, textfile.size, f);

					pNewTxtFile->sText[size] = '|';
					pNewTxtFile->sText[size+1] = '\0';

					pNewTxtFile->sCommand = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, strlen(textfile.name)-3);
					if(pNewTxtFile->sCommand == NULL) {
						AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sCommand in TextFileMan::RefreshTextFiles\n", (uint64_t)(strlen(textfile.name)-3));

						fclose(f);
						_findclose(hFile);

						return;
					}

					memcpy(pNewTxtFile->sCommand, textfile.name, strlen(textfile.name)-4);
					pNewTxtFile->sCommand[strlen(textfile.name)-4] = '\0';

					pNewTxtFile->prev = NULL;

					if(TextFiles == NULL) {
						pNewTxtFile->next = NULL;
					} else {
						TextFiles->prev = pNewTxtFile;
						pNewTxtFile->next = TextFiles;
					}

					TextFiles = pNewTxtFile;
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
                TextFile * pNewTxtFile = new TextFile();
				if(pNewTxtFile == NULL) {
					AppendDebugLog("%s - [MEM] Cannot allocate pNewTxtFile in TextFileMan::RefreshTextFiles1\n", 0);

					fclose(f);
					closedir(p_txtdir);

                    return;
                }

				pNewTxtFile->sText = (char *)malloc(s_buf.st_size+2);
				if(pNewTxtFile->sText == NULL) {
					AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sText in TextFileMan::RefreshTextFiles\n", (uint64_t)(s_buf.st_size+2));

					fclose(f);
					closedir(p_txtdir);

                    return;
                }
    	        size_t size = fread(pNewTxtFile->sText, 1, s_buf.st_size, f);
				pNewTxtFile->sText[size] = '|';
                pNewTxtFile->sText[size+1] = '\0';

				pNewTxtFile->sCommand = (char *)malloc(strlen(p_dirent->d_name)-3);
				if(pNewTxtFile->sCommand == NULL) {
					AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sCommand in TextFileMan::RefreshTextFiles\n", (uint64_t)(strlen(p_dirent->d_name)-3));

					fclose(f);
					closedir(p_txtdir);

                    return;
                }

                memcpy(pNewTxtFile->sCommand, p_dirent->d_name, strlen(p_dirent->d_name)-4);
                pNewTxtFile->sCommand[strlen(p_dirent->d_name)-4] = '\0';

                pNewTxtFile->prev = NULL;

                if(TextFiles == NULL) {
                    pNewTxtFile->next = NULL;
                } else {
                    TextFiles->prev = pNewTxtFile;
                    pNewTxtFile->next = TextFiles;
                }

                TextFiles = pNewTxtFile;
    	    }

            fclose(f);
    	}
    }

    closedir(p_txtdir);
#endif
}
//---------------------------------------------------------------------------
