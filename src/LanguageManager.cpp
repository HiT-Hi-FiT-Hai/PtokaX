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
#include "LanguageXml.h"
#include "LanguageStrings.h"
#include "LanguageManager.h"
//---------------------------------------------------------------------------
#include "SettingManager.h"
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
LangMan *LanguageManager = NULL;
//---------------------------------------------------------------------------

LangMan::LangMan(void) {
    for(size_t i = 0; i < LANG_IDS_END; i++) {
        size_t TextLen = strlen(LangStr[i]);
#ifdef _WIN32
        sTexts[i] = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, TextLen+1);
#else
		sTexts[i] = (char *) malloc(TextLen+1);
#endif
        if(sTexts[i] == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(TextLen+1))+
				" bytes of memory in LangMan::LangMan!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
            AppendSpecialLog(sDbgstr);
            exit(EXIT_FAILURE);
        }
        memcpy(sTexts[i], LangStr[i], TextLen);
        ui16TextsLens[i] = (uint16_t)TextLen;
        sTexts[i][ui16TextsLens[i]] = '\0';
    }
}
//---------------------------------------------------------------------------

LangMan::~LangMan(void) {
    for(size_t i = 0; i < LANG_IDS_END; i++) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sTexts[i]) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate sTexts[i] in LangMan::~LangMan! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
#else
		free(sTexts[i]);
#endif
        sTexts[i] = NULL;
        ui16TextsLens[i] = 0;
    }
}
//---------------------------------------------------------------------------

void LangMan::LoadLanguage() {
    if(SettingManager->sTexts[SETTXT_LANGUAGE] == NULL) {
        for(size_t i = 0; i < LANG_IDS_END; i++) {
#ifdef _WIN32
            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sTexts[i]) == 0) {
				string sDbgstr = "[BUF] Cannot deallocate sTexts[i] in LangMan::LoadLanguage! "+string((uint32_t)GetLastError())+" "+
					string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
				AppendSpecialLog(sDbgstr);
            }
#else
			free(sTexts[i]);
#endif
            
            size_t TextLen = strlen(LangStr[i]);
#ifdef _WIN32
            sTexts[i] = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, TextLen+1);
#else
			sTexts[i] = (char *) malloc(TextLen+1);
#endif
            if(sTexts[i] == NULL) {
				string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(TextLen+1))+
					" bytes of memory in LangMan::LoadLanguage!";
#ifdef _WIN32
				sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
				AppendSpecialLog(sDbgstr);
                return;
            }
            memcpy(sTexts[i], LangStr[i], TextLen);
            ui16TextsLens[i] = (uint16_t)TextLen;
            sTexts[i][ui16TextsLens[i]] = '\0';
        }
    } else {
#ifdef _WIN32
		string sLanguageFile = PATH+"\\language\\"+string(SettingManager->sTexts[SETTXT_LANGUAGE],
#else
		string sLanguageFile = PATH+"/language/"+string(SettingManager->sTexts[SETTXT_LANGUAGE],
#endif
            (size_t)SettingManager->ui16TextsLens[SETTXT_LANGUAGE])+".xml";

        TiXmlDocument doc(sLanguageFile.c_str());
        if(doc.LoadFile()) {
            TiXmlHandle cfg(&doc);
            TiXmlNode *language = cfg.FirstChild("Language").Node();
            if(language != NULL) {
                TiXmlNode *text = NULL;
                while((text = language->IterateChildren(text)) != NULL) {
                    if(text->ToElement() == NULL) {
                        continue;
                    }

                    const char * sName = text->ToElement()->Attribute("Name");
                    const char * sText = text->ToElement()->GetText();
                    size_t iLen = (sText != NULL ? strlen(sText) : 0);
                    if(iLen != 0 && iLen < 129) {
                        for(size_t i = 0; i < LANG_IDS_END; i++) {
                            if(strcmp(LangXmlStr[i], sName) == 0) {
#ifdef _WIN32
                                if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sTexts[i]) == 0) {
									string sDbgstr = "[BUF] Cannot deallocate sTexts[i]1 in LangMan::LoadLanguage! "+string((uint32_t)GetLastError())+" "+
										string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
									AppendSpecialLog(sDbgstr);
                                }
                                
                                sTexts[i] = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iLen+1);
#else
                                free(sTexts[i]);
                                
                                sTexts[i] = (char *) malloc(iLen+1);
#endif
                                if(sTexts[i] == NULL) {
                                    string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iLen+1))+
                                    	" bytes of memory in LangMan::LoadLanguage1!";
#ifdef _WIN32
									sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
									AppendSpecialLog(sDbgstr);
                                    return;
                                }
                                memcpy(sTexts[i], sText, iLen);
                                ui16TextsLens[i] = (uint16_t)iLen;
                                sTexts[i][ui16TextsLens[i]] = '\0';
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}
//---------------------------------------------------------------------------
