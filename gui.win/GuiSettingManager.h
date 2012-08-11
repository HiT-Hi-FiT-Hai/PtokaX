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

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef GuiSettingManagerH
#define GuiSettingManagerH
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "GuiSettingIds.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

class GuiSettingManager {
private:
    void Load();
public:
    int32_t iIntegers[GUISETINT_IDS_END]; //g_GuiSettingManager->iIntegers[]

    bool bBools[GUISETBOOL_IDS_END]; //g_GuiSettingManager->bBools[]

    GuiSettingManager(void);
    ~GuiSettingManager(void);

    bool GetDefaultBool(const size_t &szBoolId) const;
    int32_t GetDefaultInteger(const size_t &szIntegerId) const;

    void SetBool(const size_t &szBoolId, const bool &bValue); //g_GuiSettingManager->SetBool()
    void SetInteger(const size_t &szIntegerId, const int32_t &i32Value); //g_GuiSettingManager->SetInteger()

    void Save() const;
};
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern GuiSettingManager * g_GuiSettingManager;
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif
