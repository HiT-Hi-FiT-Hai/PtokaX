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
#include "LanguageXml.h"
#include "LanguageStrings.h"
#include "LanguageManager.h"
//---------------------------------------------------------------------------
#include "SettingManager.h"
#include "utility.h"
//---------------------------------------------------------------------------
LangMan *LanguageManager = NULL;
//---------------------------------------------------------------------------

LangMan::LangMan(void) {
    for(size_t i = 0; i < LANG_IDS_END; i++) {
        size_t TextLen = strlen(LangStr[i]);
        sTexts[i] = (char *) malloc(TextLen+1);
        if(sTexts[i] == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(TextLen+1))+
				" bytes of memory in LangMan::LangMan!";
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
        free(sTexts[i]);
        sTexts[i] = NULL;
        ui16TextsLens[i] = 0;
    }
}
//---------------------------------------------------------------------------

void LangMan::LoadLanguage() {
    if(SettingManager->sTexts[SETTXT_LANGUAGE] == NULL) {
        for(size_t i = 0; i < LANG_IDS_END; i++) {
            free(sTexts[i]);
            
            size_t TextLen = strlen(LangStr[i]);
            sTexts[i] = (char *) malloc(TextLen+1);
            if(sTexts[i] == NULL) {
				string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(TextLen+1))+
					" bytes of memory in LangMan::LoadLanguage!";
				AppendSpecialLog(sDbgstr);
                return;
            }
            memcpy(sTexts[i], LangStr[i], TextLen);
            ui16TextsLens[i] = (uint16_t)TextLen;
            sTexts[i][ui16TextsLens[i]] = '\0';
        }
    } else {
		string sLanguageFile = PATH+"/language/"+string(SettingManager->sTexts[SETTXT_LANGUAGE],
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
                                free(sTexts[i]);
                                
                                sTexts[i] = (char *) malloc(iLen+1);
                                if(sTexts[i] == NULL) {
                                    string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iLen+1))+
										" bytes of memory in LangMan::LoadLanguage1!";
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
