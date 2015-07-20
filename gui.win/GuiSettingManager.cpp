/*
 * PtokaX - hub server for Direct Connect peer to peer network.

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

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "../core/stdinc.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "GuiSettingStr.h"
#include "GuiSettingDefaults.h"
#include "GuiSettingManager.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "../core/ServerManager.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#pragma hdrstop
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "../core/PXBReader.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
clsGuiSettingManager * clsGuiSettingManager::mPtr = NULL;
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
float clsGuiSettingManager::fScaleFactor = 1.0;
int clsGuiSettingManager::iGroupBoxMargin = 17;
int clsGuiSettingManager::iCheckHeight = 16;
int clsGuiSettingManager::iEditHeight = 23;
int clsGuiSettingManager::iTextHeight = 15;
int clsGuiSettingManager::iUpDownWidth = 17;
int clsGuiSettingManager::iOneLineGB = 17 + 23 + 8;
int clsGuiSettingManager::iOneLineOneChecksGB = 17 + 16 + 23 + 12;
int clsGuiSettingManager::iOneLineTwoChecksGB = 17 + (2 * 16) + 23 + 15;
HFONT clsGuiSettingManager::hFont = NULL;
HCURSOR clsGuiSettingManager::hArrowCursor = NULL;
HCURSOR clsGuiSettingManager::hVerticalCursor = NULL;
WNDPROC clsGuiSettingManager::wpOldButtonProc = NULL;
WNDPROC clsGuiSettingManager::wpOldEditProc = NULL;
WNDPROC clsGuiSettingManager::wpOldListViewProc = NULL;
WNDPROC clsGuiSettingManager::wpOldMultiRichEditProc = NULL;
WNDPROC clsGuiSettingManager::wpOldNumberEditProc = NULL;
WNDPROC clsGuiSettingManager::wpOldTabsProc = NULL;
WNDPROC clsGuiSettingManager::wpOldTreeProc = NULL;
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static const char sPtokaXGUISettings[] = "PtokaX GUI Settings";
static const size_t szPtokaXGUISettingsLen = sizeof("PtokaX GUI Settings")-1;
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

clsGuiSettingManager::clsGuiSettingManager(void) {
    // Read default bools
    for(size_t szi = 0; szi < GUISETBOOL_IDS_END; szi++) {
        SetBool(szi, GuiSetBoolDef[szi]);
	}

    // Read default integers
    for(size_t szi = 0; szi < GUISETINT_IDS_END; szi++) {
        SetInteger(szi, GuiSetIntegerDef[szi]);
	}

    // Load settings
	Load();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

clsGuiSettingManager::~clsGuiSettingManager(void) {
    // ...
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsGuiSettingManager::Load() {
    PXBReader pxbSetting;

    // Open setting file
    if(pxbSetting.OpenFileRead((clsServerManager::sPath + "\\cfg\\GuiSettigs.pxb").c_str()) == false) {
        return;
    }

    // Read file header
    uint16_t ui16Identificators[2] = { *((uint16_t *)"FI"), *((uint16_t *)"FV") };

    if(pxbSetting.ReadNextItem(ui16Identificators, 2) == false) {
        return;
    }

    // Check header if we have correct file
    if(pxbSetting.ui16ItemLengths[0] != szPtokaXGUISettingsLen || strncmp((char *)pxbSetting.pItemDatas[0], sPtokaXGUISettings, szPtokaXGUISettingsLen) != 0) {
        return;
    }

    {
        uint32_t ui32FileVersion = ntohl(*((uint32_t *)(pxbSetting.pItemDatas[1])));

        if(ui32FileVersion < 1) {
            return;
        }
    }

    // Read settings =)
    ui16Identificators[0] = *((uint16_t *)"SI");
    ui16Identificators[1] = *((uint16_t *)"SV");

    bool bSuccess = pxbSetting.ReadNextItem(ui16Identificators, 2);
    size_t szi = 0;

    while(bSuccess == true) {
        for(szi = 0; szi < GUISETBOOL_IDS_END; szi++) {
            if(pxbSetting.ui16ItemLengths[0] == strlen(GuiSetBoolStr[szi]) && strncmp((char *)pxbSetting.pItemDatas[0], GuiSetBoolStr[szi], pxbSetting.ui16ItemLengths[0]) == 0) {
                SetBool(szi, ((char *)pxbSetting.pItemDatas[1])[0] == '0' ? false : true);
            }
        }

        for(szi = 0; szi < GUISETINT_IDS_END; szi++) {
            if(pxbSetting.ui16ItemLengths[0] == strlen(GuiSetIntegerStr[szi]) && strncmp((char *)pxbSetting.pItemDatas[0], GuiSetIntegerStr[szi], pxbSetting.ui16ItemLengths[0]) == 0) {
                SetInteger(szi, ntohl(*((uint32_t *)(pxbSetting.pItemDatas[1]))));
            }
        }

        bSuccess = pxbSetting.ReadNextItem(ui16Identificators, 2);
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsGuiSettingManager::Save() const {
    PXBReader pxbSetting;

    // Open setting file
    if(pxbSetting.OpenFileSave((clsServerManager::sPath + "\\cfg\\GuiSettigs.pxb").c_str()) == false) {
        return;
    }

    // Write file header
    pxbSetting.sItemIdentifiers[0][0] = 'F';
    pxbSetting.sItemIdentifiers[0][1] = 'I';
    pxbSetting.ui16ItemLengths[0] = (uint16_t)szPtokaXGUISettingsLen;
    pxbSetting.pItemDatas[0] = (void *)sPtokaXGUISettings;
    pxbSetting.ui8ItemValues[0] = PXBReader::PXB_STRING;

    pxbSetting.sItemIdentifiers[1][0] = 'F';
    pxbSetting.sItemIdentifiers[1][1] = 'V';
    pxbSetting.ui16ItemLengths[1] = 4;
    uint32_t ui32Version = 1;
    pxbSetting.pItemDatas[1] = (void *)&ui32Version;
    pxbSetting.ui8ItemValues[1] = PXBReader::PXB_FOUR_BYTES;

    if(pxbSetting.WriteNextItem(23, 2) == false) {
        return;
    }

    pxbSetting.sItemIdentifiers[0][0] = 'S';
    pxbSetting.sItemIdentifiers[0][1] = 'I';
    pxbSetting.sItemIdentifiers[1][0] = 'S';
    pxbSetting.sItemIdentifiers[1][1] = 'V';

    for(size_t szi = 0; szi < GUISETBOOL_IDS_END; szi++) {
        // Don't save setting with default value
        if(bBools[szi] == GuiSetBoolDef[szi]) {
            continue;
        }

        pxbSetting.ui16ItemLengths[0] = (uint16_t)strlen(GuiSetBoolStr[szi]);
        pxbSetting.pItemDatas[0] = (void *)GuiSetBoolStr[szi];
        pxbSetting.ui8ItemValues[0] = PXBReader::PXB_STRING;

        pxbSetting.ui16ItemLengths[1] = 1;
        pxbSetting.pItemDatas[1] = (bBools[szi] == true ? (void *)1 : 0);
        pxbSetting.ui8ItemValues[1] = PXBReader::PXB_BYTE;

        if(pxbSetting.WriteNextItem(pxbSetting.ui16ItemLengths[0] + pxbSetting.ui16ItemLengths[1], 2) == false) {
            break;
        }
    }

    for(size_t szi = 0; szi < GUISETINT_IDS_END; szi++) {
        // Don't save setting with default value
        if(i32Integers[szi] == GuiSetIntegerDef[szi]) {
            continue;
        }

        pxbSetting.ui16ItemLengths[0] = (uint16_t)strlen(GuiSetIntegerStr[szi]);
        pxbSetting.pItemDatas[0] = (void *)GuiSetIntegerStr[szi];
        pxbSetting.ui8ItemValues[0] = PXBReader::PXB_STRING;

        pxbSetting.ui16ItemLengths[1] = 4;
        pxbSetting.pItemDatas[1] = (void *)&i32Integers[szi];
        pxbSetting.ui8ItemValues[1] = PXBReader::PXB_FOUR_BYTES;

        if(pxbSetting.WriteNextItem(pxbSetting.ui16ItemLengths[0] + pxbSetting.ui16ItemLengths[1], 2) == false) {
            break;
        }
    }

    pxbSetting.WriteRemaining();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool clsGuiSettingManager::GetDefaultBool(const size_t &szBoolId) {
    return GuiSetBoolDef[szBoolId];
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int32_t clsGuiSettingManager::GetDefaultInteger(const size_t &szIntegerId) {
    return GuiSetIntegerDef[szIntegerId];;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsGuiSettingManager::SetBool(const size_t &szBoolId, const bool &bValue) {
    if(bBools[szBoolId] == bValue) {
        return;
    }

    bBools[szBoolId] = bValue;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsGuiSettingManager::SetInteger(const size_t &szIntegerId, const int32_t &i32Value) {
    if(i32Value < 0 || i32Integers[szIntegerId] == i32Value) {
        return;
    }

    i32Integers[szIntegerId] = i32Value;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
