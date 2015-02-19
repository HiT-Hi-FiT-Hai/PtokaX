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

//---------------------------------------------------------------------------
#include "../core/stdinc.h"
//---------------------------------------------------------------------------
#include "ProfilesDialog.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/ProfileManager.h"
#include "../core/ServerManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------
#include "LineDialog.h"
//---------------------------------------------------------------------------
clsProfilesDialog * clsProfilesDialog::mPtr = NULL;
//---------------------------------------------------------------------------
#define IDC_RENAME_PROFILE      1000
#define IDC_REMOVE_PROFILE      1001
//---------------------------------------------------------------------------
static ATOM atomProfilesDialog = 0;
//---------------------------------------------------------------------------

clsProfilesDialog::clsProfilesDialog() : bIgnoreItemChanged(false) {
    memset(&hWndWindowItems, 0, sizeof(hWndWindowItems));
}
//---------------------------------------------------------------------------

clsProfilesDialog::~clsProfilesDialog() {
    clsProfilesDialog::mPtr = NULL;
}
//---------------------------------------------------------------------------

LRESULT CALLBACK clsProfilesDialog::StaticProfilesDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    clsProfilesDialog * pProfilesDialog = (clsProfilesDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if(pProfilesDialog == NULL) {
        return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

	return pProfilesDialog->ProfilesDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void OnNewProfileOk(char * sLine, const int &/*iLen*/) {
    int32_t iRet = clsProfileManager::mPtr->AddProfile(sLine);

    if(iRet == -1) {
        ::MessageBox(clsProfilesDialog::mPtr->hWndWindowItems[clsProfilesDialog::WINDOW_HANDLE], clsLanguageManager::mPtr->sTexts[LAN_PROFILE_NAME_EXIST], clsServerManager::sTitle.c_str(), MB_OK);
    } else if(iRet == -2) {
        ::MessageBox(clsProfilesDialog::mPtr->hWndWindowItems[clsProfilesDialog::WINDOW_HANDLE], clsLanguageManager::mPtr->sTexts[LAN_CHARS_NOT_ALLOWED_IN_PROFILE], clsServerManager::sTitle.c_str(), MB_OK);
    }
}
//---------------------------------------------------------------------------

LRESULT clsProfilesDialog::ProfilesDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_WINDOWPOSCHANGED: {
            RECT rcParent;
            ::GetClientRect(hWndWindowItems[WINDOW_HANDLE], &rcParent);

            int iProfilesWidth = rcParent.right / 3;

            int iPermissionsWidth = rcParent.right - (iProfilesWidth + 17);

            ::SetWindowPos(hWndWindowItems[BTN_CLEAR_ALL], NULL, iProfilesWidth + 11 + (iPermissionsWidth / 2), rcParent.bottom - clsGuiSettingManager::iEditHeight - 10,
                rcParent.right - (iProfilesWidth + 21 + (iPermissionsWidth / 2)), clsGuiSettingManager::iEditHeight, SWP_NOZORDER);
            ::SetWindowPos(hWndWindowItems[BTN_SET_ALL], NULL, iProfilesWidth + 11, rcParent.bottom - clsGuiSettingManager::iEditHeight - 10, (iPermissionsWidth / 2) - 3, clsGuiSettingManager::iEditHeight, SWP_NOZORDER);
            ::SetWindowPos(hWndWindowItems[LV_PERMISSIONS], NULL, iProfilesWidth + 12, rcParent.top + clsGuiSettingManager::iGroupBoxMargin,
                rcParent.right - (iProfilesWidth + 23), rcParent.bottom - clsGuiSettingManager::iGroupBoxMargin - clsGuiSettingManager::iEditHeight - 14, SWP_NOZORDER);
            ::SendMessage(hWndWindowItems[LV_PERMISSIONS], LVM_SETCOLUMNWIDTH, 0, iPermissionsWidth-30);
            ::SetWindowPos(hWndWindowItems[GB_PERMISSIONS], NULL, iProfilesWidth + 4, rcParent.top, rcParent.right - (iProfilesWidth + 7), rcParent.bottom - 3,
                SWP_NOZORDER);
            ::SetWindowPos(hWndWindowItems[BTN_MOVE_DOWN], NULL, (iProfilesWidth / 2) + 2, rcParent.bottom - clsGuiSettingManager::iEditHeight - 2,
                iProfilesWidth - (iProfilesWidth / 2), clsGuiSettingManager::iEditHeight, SWP_NOZORDER);
            ::SetWindowPos(hWndWindowItems[BTN_MOVE_UP], NULL, 2, rcParent.bottom - clsGuiSettingManager::iEditHeight - 2, (iProfilesWidth / 2) - 1, clsGuiSettingManager::iEditHeight, SWP_NOZORDER);
            ::SetWindowPos(hWndWindowItems[LV_PROFILES], NULL, 0, 0, iProfilesWidth - 2, rcParent.bottom - (2 * clsGuiSettingManager::iEditHeight) - 12, SWP_NOMOVE | SWP_NOZORDER);
            ::SendMessage(hWndWindowItems[LV_PROFILES], LVM_SETCOLUMNWIDTH, 0, iProfilesWidth - 6);
            ::SetWindowPos(hWndWindowItems[BTN_ADD_PROFILE], NULL, 0, 0, iProfilesWidth, clsGuiSettingManager::iEditHeight, SWP_NOMOVE | SWP_NOZORDER);

            return 0;
        }
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case (BTN_ADD_PROFILE+100): {
                    LineDialog * pNewProfileDlg = new (std::nothrow) LineDialog(&OnNewProfileOk);

                    if(pNewProfileDlg != NULL) {
                        pNewProfileDlg->DoModal(hWndWindowItems[WINDOW_HANDLE], clsLanguageManager::mPtr->sTexts[LAN_NEW_PROFILE_NAME], "");
                    }

                    return 0;
                }
                case IDC_RENAME_PROFILE: {
                    int iSel = (int)::SendMessage(hWndWindowItems[LV_PROFILES], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

                    if(iSel == -1) {
                        return 0;
                    }

                    RenameProfile(iSel);

                    return 0;
                }
                case IDC_REMOVE_PROFILE: {
                    int iSel = (int)::SendMessage(hWndWindowItems[LV_PROFILES], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

                    if(iSel == -1 || ::MessageBox(hWndWindowItems[WINDOW_HANDLE], (string(clsLanguageManager::mPtr->sTexts[LAN_ARE_YOU_SURE],
                        (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_ARE_YOU_SURE])+" ?").c_str(), clsServerManager::sTitle.c_str(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO) {
                        return 0;
                    }

                    if(clsProfileManager::mPtr->RemoveProfile((uint16_t)iSel) == false) {
                        ::MessageBox(hWndWindowItems[WINDOW_HANDLE], clsLanguageManager::mPtr->sTexts[LAN_PROFILE_DEL_FAIL], clsServerManager::sTitle.c_str(), MB_OK);
                    }

                    return 0;
                }
                case BTN_SET_ALL:
                    ChangePermissionChecks(true);
                    return 0;
                case BTN_CLEAR_ALL:
                    ChangePermissionChecks(false);
                    return 0;
                case BTN_MOVE_UP: {
                    int iSel = (int)::SendMessage(hWndWindowItems[LV_PROFILES], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

                    if(iSel == -1) {
                        return 0;
                    }

                    clsProfileManager::mPtr->MoveProfileUp((uint16_t)iSel);

                    return 0;
                }
                case BTN_MOVE_DOWN: {
                    int iSel = (int)::SendMessage(hWndWindowItems[LV_PROFILES], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

                    if(iSel == -1) {
                        return 0;
                    }

                    clsProfileManager::mPtr->MoveProfileDown((uint16_t)iSel);

                    return 0;
                }
                case IDOK: { // NM_RETURN
                    if(::GetFocus() == hWndWindowItems[LV_PROFILES]) {
                        int iSel = (int)::SendMessage(hWndWindowItems[LV_PROFILES], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

                        if(iSel == -1) {
                            return 0;
                        }

                        RenameProfile(iSel);

                        return 0;
                    }

                    break;
                }
                case IDCANCEL:
                    ::PostMessage(hWndWindowItems[WINDOW_HANDLE], WM_CLOSE, 0, 0);
					return 0;
            }

            break;
        case WM_CONTEXTMENU:
            OnContextMenu((HWND)wParam, lParam);
            break;
        case WM_NOTIFY:
            if(((LPNMHDR)lParam)->hwndFrom == hWndWindowItems[LV_PROFILES]) {
                if(((LPNMHDR)lParam)->code == LVN_ITEMCHANGED) {
                    OnProfileChanged((LPNMLISTVIEW)lParam);
                } else if(((LPNMHDR)lParam)->code == NM_DBLCLK) {
                    if(((LPNMITEMACTIVATE)lParam)->iItem == -1) {
                        break;
                    }

                    RenameProfile(((LPNMITEMACTIVATE)lParam)->iItem);

                    return 0;
                }
            } else if(((LPNMHDR)lParam)->hwndFrom == hWndWindowItems[LV_PERMISSIONS]) {
                if(((LPNMHDR)lParam)->code == LVN_ITEMCHANGED) {
                    OnPermissionChanged((LPNMLISTVIEW)lParam);
                }
            }

            break;
        case WM_GETMINMAXINFO: {
            MINMAXINFO *mminfo = (MINMAXINFO*)lParam;
            mminfo->ptMinTrackSize.x = ScaleGui(clsGuiSettingManager::mPtr->GetDefaultInteger(GUISETINT_PROFILES_WINDOW_WIDTH));
            mminfo->ptMinTrackSize.y = ScaleGui(clsGuiSettingManager::mPtr->GetDefaultInteger(GUISETINT_PROFILES_WINDOW_HEIGHT));

            return 0;
        }
        case WM_CLOSE: {
            RECT rcProfiles;
            ::GetWindowRect(hWndWindowItems[WINDOW_HANDLE], &rcProfiles);

            clsGuiSettingManager::mPtr->SetInteger(GUISETINT_PROFILES_WINDOW_WIDTH, rcProfiles.right - rcProfiles.left);
            clsGuiSettingManager::mPtr->SetInteger(GUISETINT_PROFILES_WINDOW_HEIGHT, rcProfiles.bottom - rcProfiles.top);

            ::EnableWindow(::GetParent(hWndWindowItems[WINDOW_HANDLE]), TRUE);
            clsServerManager::hWndActiveDialog = NULL;

            break;
        }
        case WM_NCDESTROY: {
            HWND hWnd = hWndWindowItems[WINDOW_HANDLE];
            delete this;
            return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
        case WM_SETFOCUS:
            ::SetFocus(hWndWindowItems[LV_PROFILES]);
            return 0;
        case WM_ACTIVATE:
            if(LOWORD(wParam) != WA_INACTIVE) {
                clsServerManager::hWndActiveDialog = hWndWindowItems[WINDOW_HANDLE];
            }

            break;
    }

	return ::DefWindowProc(hWndWindowItems[WINDOW_HANDLE], uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void clsProfilesDialog::DoModal(HWND hWndParent) {
    if(atomProfilesDialog == 0) {
        WNDCLASSEX m_wc;
        memset(&m_wc, 0, sizeof(WNDCLASSEX));
        m_wc.cbSize = sizeof(WNDCLASSEX);
        m_wc.lpfnWndProc = ::DefWindowProc;
        m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
        m_wc.lpszClassName = "PtokaX_ProfilesDialog";
        m_wc.hInstance = clsServerManager::hInstance;
        m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
        m_wc.style = CS_HREDRAW | CS_VREDRAW;

        atomProfilesDialog = ::RegisterClassEx(&m_wc);
    }

    RECT rcParent;
    ::GetWindowRect(hWndParent, &rcParent);

    int iX = (rcParent.left + (((rcParent.right-rcParent.left))/2)) - (ScaleGuiDefaultsOnly(GUISETINT_PROFILES_WINDOW_WIDTH) / 2);
    int iY = (rcParent.top + ((rcParent.bottom-rcParent.top)/2)) - (ScaleGuiDefaultsOnly(GUISETINT_PROFILES_WINDOW_HEIGHT) / 2);

    hWndWindowItems[WINDOW_HANDLE] = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomProfilesDialog), clsLanguageManager::mPtr->sTexts[LAN_PROFILES],
        WS_POPUP | WS_CAPTION | WS_MAXIMIZEBOX | WS_SYSMENU | WS_SIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, ScaleGuiDefaultsOnly(GUISETINT_PROFILES_WINDOW_WIDTH), ScaleGuiDefaultsOnly(GUISETINT_PROFILES_WINDOW_HEIGHT),
        hWndParent, NULL, clsServerManager::hInstance, NULL);

    if(hWndWindowItems[WINDOW_HANDLE] == NULL) {
        return;
    }

    clsServerManager::hWndActiveDialog = hWndWindowItems[WINDOW_HANDLE];

    ::SetWindowLongPtr(hWndWindowItems[WINDOW_HANDLE], GWLP_USERDATA, (LONG_PTR)this);
    ::SetWindowLongPtr(hWndWindowItems[WINDOW_HANDLE], GWLP_WNDPROC, (LONG_PTR)StaticProfilesDialogProc);

    ::GetClientRect(hWndWindowItems[WINDOW_HANDLE], &rcParent);

    int iProfilesWidth = rcParent.right / 3;

    hWndWindowItems[BTN_ADD_PROFILE] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_ADD_NEW_PROFILE], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        2, 2, iProfilesWidth, clsGuiSettingManager::iEditHeight, hWndWindowItems[WINDOW_HANDLE], (HMENU)(BTN_ADD_PROFILE+100), clsServerManager::hInstance, NULL);

    hWndWindowItems[LV_PROFILES] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_NOCOLUMNHEADER | LVS_REPORT | LVS_SHOWSELALWAYS |
        LVS_SINGLESEL, 3, clsGuiSettingManager::iEditHeight + 6, iProfilesWidth - 2, rcParent.bottom - (2 * clsGuiSettingManager::iEditHeight) - 12, hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
    ::SendMessage(hWndWindowItems[LV_PROFILES], LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

    hWndWindowItems[BTN_MOVE_UP] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_MOVE_UP], WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP | BS_PUSHBUTTON,
        2, rcParent.bottom - clsGuiSettingManager::iEditHeight - 2, (iProfilesWidth / 2) - 1, clsGuiSettingManager::iEditHeight, hWndWindowItems[WINDOW_HANDLE], (HMENU)BTN_MOVE_UP, clsServerManager::hInstance, NULL);

    hWndWindowItems[BTN_MOVE_DOWN] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_MOVE_DOWN], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        (iProfilesWidth / 2) + 2, rcParent.bottom - clsGuiSettingManager::iEditHeight - 2, iProfilesWidth - (iProfilesWidth / 2), clsGuiSettingManager::iEditHeight, hWndWindowItems[WINDOW_HANDLE], (HMENU)BTN_MOVE_DOWN, clsServerManager::hInstance, NULL);

    hWndWindowItems[GB_PERMISSIONS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_PROFILE_PERMISSIONS],
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | BS_GROUPBOX, iProfilesWidth + 4, 0, rcParent.right -(iProfilesWidth + 7), rcParent.bottom - 3,
        hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);

    hWndWindowItems[LV_PERMISSIONS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_NOCOLUMNHEADER | LVS_REPORT | LVS_SHOWSELALWAYS |
        LVS_SINGLESEL, iProfilesWidth + 12, clsGuiSettingManager::iGroupBoxMargin, rcParent.right - (iProfilesWidth + 23), rcParent.bottom - clsGuiSettingManager::iGroupBoxMargin - clsGuiSettingManager::iEditHeight - 14,
        hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
    ::SendMessage(hWndWindowItems[LV_PERMISSIONS], LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_CHECKBOXES);

    int iPermissionsWidth = rcParent.right - (iProfilesWidth + 17);

    hWndWindowItems[BTN_SET_ALL] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_SET_ALL], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        iProfilesWidth + 11, rcParent.bottom - clsGuiSettingManager::iEditHeight - 11, (iPermissionsWidth / 2) - 3, clsGuiSettingManager::iEditHeight, hWndWindowItems[WINDOW_HANDLE], (HMENU)BTN_SET_ALL, clsServerManager::hInstance, NULL);

    hWndWindowItems[BTN_CLEAR_ALL] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_CLEAR_ALL], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        iProfilesWidth + 11 + (iPermissionsWidth / 2), rcParent.bottom - clsGuiSettingManager::iEditHeight - 11, rcParent.right - (iProfilesWidth + 21 + (iPermissionsWidth / 2)), clsGuiSettingManager::iEditHeight,
        hWndWindowItems[WINDOW_HANDLE], (HMENU)BTN_CLEAR_ALL, clsServerManager::hInstance, NULL);

    for(uint8_t ui8i = 0; ui8i < (sizeof(hWndWindowItems) / sizeof(hWndWindowItems[0])); ui8i++) {
        if(hWndWindowItems[ui8i] == NULL) {
            return;
        }

        ::SendMessage(hWndWindowItems[ui8i], WM_SETFONT, (WPARAM)clsGuiSettingManager::hFont, MAKELPARAM(TRUE, 0));
    }

	RECT rcProfiles;
	::GetClientRect(hWndWindowItems[LV_PROFILES], &rcProfiles);

    LVCOLUMN lvColumn = { 0 };
    lvColumn.mask = LVCF_FMT | LVCF_WIDTH;
    lvColumn.fmt = LVCFMT_LEFT;
    lvColumn.cx = rcProfiles.right-5;

    ::SendMessage(hWndWindowItems[LV_PROFILES], LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn);

	RECT rcPermissions;
	::GetClientRect(hWndWindowItems[LV_PERMISSIONS], &rcPermissions);

    lvColumn.cx = rcPermissions.right;

    ::SendMessage(hWndWindowItems[LV_PERMISSIONS], LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn);

    const int iPermissionsStrings[] = {
        LAN_ENT_FULL_HUB, LAN_ALLOW_LOGIN_BANNED_IP, LAN_CLOSE, LAN_DROP, LAN_KICK, LAN_GET_USER_INFO,
        LAN_REDIR, LAN_TEMPOR_BAN, LAN_TEMPOR_UNBAN, LAN_PERMANENT_BAN, LAN_UNBAN_PERM_UNBAN, LAN_CLEAR_TEMP_BANS,
        LAN_CLEAR_PERM_BANS, LAN_GET_BANS, LAN_RANGE_PERM_BAN, LAN_RANGE_UNBAN_RANGE_PERM_UNBAN, LAN_TEMP_RANGE_BAN,
        LAN_TEMP_RANGE_UNBAN, LAN_GET_RANGE_BANS, LAN_CLEAR_PERM_RANGE_BANS, LAN_CLEAR_TEMP_RANGE_BANS, LAN_GAG_UNGAG,
        LAN_TEMP_OP, LAN_START_STOP_RESTART_SCRIPTS, LAN_RESTART_HUB, LAN_RLD_TXT_FILES, LAN_NO_SHARE_LIMIT, LAN_NO_SLOT_CHECK,
        LAN_NO_RATIO_CHECK, LAN_NO_MAX_HUBS_CHECK, LAN_NO_CHAT_LIMITS, LAN_NO_TAG_CHECK, LAN_NO_MAIN_DFLD,
        LAN_NO_PM_DFLD, LAN_NO_SEARCH_LIMITS, LAN_NO_SEARCH_DFLD, LAN_NO_MYINFO_DFLD, LAN_NO_GETNICKLIST_DFLD,
        LAN_NO_CTM_DFLD, LAN_NO_RCTM_DFLD, LAN_NO_SR_DFLD, LAN_NO_RECV_DFLD, LAN_TOPIC,
        LAN_MASS_MSG, LAN_ADD_REG_USER, LAN_DEL_REG_USER, LAN_ALLOW_OPCHAT, LAN_SEND_LONG_MYINFOS,
        LAN_SEND_USER_IPS, LAN_DONT_CHECK_IP, LAN_HAVE_KEY_IS_OP, LAN_NO_CHAT_INTERVAL, LAN_NO_PM_INTERVAL,
        LAN_NO_SEARCH_INTERVAL, LAN_NO_MAX_USR_SAME_IP, LAN_NO_RECONN_TIME
    };

    const int iPermissionsIds[] = {
        clsProfileManager::ENTERFULLHUB, clsProfileManager::ENTERIFIPBAN, clsProfileManager::CLOSE, clsProfileManager::DROP, clsProfileManager::KICK, clsProfileManager::GETINFO,
        clsProfileManager::REDIRECT, clsProfileManager::TEMP_BAN, clsProfileManager::TEMP_UNBAN, clsProfileManager::BAN, clsProfileManager::UNBAN, clsProfileManager::CLRTEMPBAN,
        clsProfileManager::CLRPERMBAN, clsProfileManager::GETBANLIST, clsProfileManager::RANGE_BAN, clsProfileManager::RANGE_UNBAN, clsProfileManager::RANGE_TBAN,
        clsProfileManager::RANGE_TUNBAN, clsProfileManager::GET_RANGE_BANS, clsProfileManager::CLR_RANGE_BANS, clsProfileManager::CLR_RANGE_TBANS, clsProfileManager::GAG,
        clsProfileManager::TEMPOP, clsProfileManager::RSTSCRIPTS, clsProfileManager::RSTHUB, clsProfileManager::REFRESHTXT, clsProfileManager::NOSHARELIMIT, clsProfileManager::NOSLOTCHECK,
        clsProfileManager::NOSLOTHUBRATIO, clsProfileManager::NOMAXHUBCHECK, clsProfileManager::NOCHATLIMITS, clsProfileManager::NOTAGCHECK, clsProfileManager::NODEFLOODMAINCHAT,
        clsProfileManager::NODEFLOODPM, clsProfileManager::NOSEARCHLIMITS, clsProfileManager::NODEFLOODSEARCH, clsProfileManager::NODEFLOODMYINFO, clsProfileManager::NODEFLOODGETNICKLIST,
        clsProfileManager::NODEFLOODCTM, clsProfileManager::NODEFLOODRCTM, clsProfileManager::NODEFLOODSR, clsProfileManager::NODEFLOODRECV, clsProfileManager::TOPIC,
        clsProfileManager::MASSMSG, clsProfileManager::ADDREGUSER, clsProfileManager::DELREGUSER, clsProfileManager::ALLOWEDOPCHAT, clsProfileManager::SENDFULLMYINFOS,
        clsProfileManager::SENDALLUSERIP, clsProfileManager::NOIPCHECK, clsProfileManager::HASKEYICON, clsProfileManager::NOCHATINTERVAL, clsProfileManager::NOPMINTERVAL,
        clsProfileManager::NOSEARCHINTERVAL, clsProfileManager::NOUSRSAMEIP, clsProfileManager::NORECONNTIME
    };

    LVITEM lvItem = { 0 };
    lvItem.mask = LVIF_PARAM | LVIF_TEXT;

    for(uint16_t ui16i = 0; ui16i < (sizeof(iPermissionsIds) / sizeof(iPermissionsIds[0])); ui16i++) {
        lvItem.iItem = ui16i;
        lvItem.lParam = (LPARAM)iPermissionsIds[ui16i];
        lvItem.pszText = clsLanguageManager::mPtr->sTexts[iPermissionsStrings[ui16i]];

        ::SendMessage(hWndWindowItems[LV_PERMISSIONS], LVM_INSERTITEM, 0, (LPARAM)&lvItem);
    }

    ListViewSelectFirstItem(hWndWindowItems[LV_PERMISSIONS]);

    AddAllProfiles();

    ::EnableWindow(hWndParent, FALSE);

    ::ShowWindow(hWndWindowItems[WINDOW_HANDLE], SW_SHOW);
}
//------------------------------------------------------------------------------

void clsProfilesDialog::AddAllProfiles() {
    ::SendMessage(hWndWindowItems[LV_PROFILES], WM_SETREDRAW, (WPARAM)FALSE, 0);

    LVITEM lvItem = { 0 };
    lvItem.mask = LVIF_PARAM | LVIF_TEXT;

	for(uint16_t ui16i = 0; ui16i < clsProfileManager::mPtr->ui16ProfileCount; ui16i++) {
        lvItem.iItem = ui16i;
        lvItem.lParam = (LPARAM)clsProfileManager::mPtr->ppProfilesTable[ui16i];
        lvItem.pszText = clsProfileManager::mPtr->ppProfilesTable[ui16i]->sName;

        ::SendMessage(hWndWindowItems[LV_PROFILES], LVM_INSERTITEM, 0, (LPARAM)&lvItem);
    }

    ListViewSelectFirstItem(hWndWindowItems[LV_PROFILES]);

    ::SendMessage(hWndWindowItems[LV_PROFILES], WM_SETREDRAW, (WPARAM)TRUE, 0);
}
//------------------------------------------------------------------------------

void clsProfilesDialog::OnContextMenu(HWND hWindow, LPARAM lParam) {
    if(hWindow != hWndWindowItems[LV_PROFILES]) {
        return;
    }

    int iSel = (int)::SendMessage(hWndWindowItems[LV_PROFILES], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    if(iSel == -1) {
        return;
    }

    HMENU hMenu = ::CreatePopupMenu();

    ::AppendMenu(hMenu, MF_STRING, IDC_RENAME_PROFILE, clsLanguageManager::mPtr->sTexts[LAN_RENAME]);
    ::AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    ::AppendMenu(hMenu, MF_STRING, IDC_REMOVE_PROFILE, clsLanguageManager::mPtr->sTexts[LAN_REMOVE]);

    int iX = GET_X_LPARAM(lParam);
    int iY = GET_Y_LPARAM(lParam);

    ListViewGetMenuPos(hWndWindowItems[LV_PROFILES], iX, iY);

    ::TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, iX, iY, hWndWindowItems[WINDOW_HANDLE], NULL);

    ::DestroyMenu(hMenu);
}
//------------------------------------------------------------------------------

void clsProfilesDialog::OnProfileChanged(const LPNMLISTVIEW &pListView) {
    UpdateUpDown();

    if(pListView->iItem == -1 || (pListView->uOldState & LVIS_SELECTED) == LVIS_SELECTED || (pListView->uNewState & LVIS_SELECTED) != LVIS_SELECTED) {
        return;
    }

    bIgnoreItemChanged = true;

    uint16_t iItemCount = (uint16_t)::SendMessage(hWndWindowItems[LV_PERMISSIONS], LVM_GETITEMCOUNT, 0, 0);

    for(uint16_t ui16i = 0; ui16i < iItemCount; ui16i++) {
        LVITEM lvItem = { 0 };
        lvItem.mask = LVIF_PARAM;
        lvItem.iItem = ui16i;

        if((BOOL)::SendMessage(hWndWindowItems[LV_PERMISSIONS], LVM_GETITEM, 0, (LPARAM)&lvItem) == FALSE) {
            continue;
        }

        lvItem.mask = LVIF_STATE;
        lvItem.state = INDEXTOSTATEIMAGEMASK(clsProfileManager::mPtr->ppProfilesTable[pListView->iItem]->bPermissions[(int)lvItem.lParam] == true ? 2 : 1);
        lvItem.stateMask = LVIS_STATEIMAGEMASK;

        ::SendMessage(hWndWindowItems[LV_PERMISSIONS], LVM_SETITEMSTATE, ui16i, (LPARAM)&lvItem);
    }

    bIgnoreItemChanged = false;
}
//------------------------------------------------------------------------------

void clsProfilesDialog::ChangePermissionChecks(const bool &bCheck) {
    int iSel = (int)::SendMessage(hWndWindowItems[LV_PROFILES], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    if(iSel == -1) {
        return;
    }

    bIgnoreItemChanged = true;

    uint16_t iItemCount = (uint16_t)::SendMessage(hWndWindowItems[LV_PERMISSIONS], LVM_GETITEMCOUNT, 0, 0);

    LVITEM lvItem = { 0 };
    lvItem.mask = LVIF_STATE;
    lvItem.stateMask = LVIS_STATEIMAGEMASK;

    for(uint16_t ui16i = 0; ui16i < iItemCount; ui16i++) {
        clsProfileManager::mPtr->ppProfilesTable[iSel]->bPermissions[ui16i] = bCheck;

        lvItem.iItem = ui16i;
        lvItem.state = INDEXTOSTATEIMAGEMASK(bCheck == true ? 2 : 1);

        ::SendMessage(hWndWindowItems[LV_PERMISSIONS], LVM_SETITEMSTATE, ui16i, (LPARAM)&lvItem);
    }

    bIgnoreItemChanged = false;
}
//------------------------------------------------------------------------------

void OnRenameProfileOk(char * sLine, const int &iLen) {
    int iSel = (int)::SendMessage(clsProfilesDialog::mPtr->hWndWindowItems[clsProfilesDialog::LV_PROFILES], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    if(iSel == -1) {
        return;
    }

    clsProfileManager::mPtr->ChangeProfileName((uint16_t)iSel, sLine, iLen);

    LVITEM lvItem = { 0 };
    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = iSel;
    lvItem.pszText = sLine;

    ::SendMessage(clsProfilesDialog::mPtr->hWndWindowItems[clsProfilesDialog::LV_PROFILES], LVM_SETITEMTEXT, iSel, (LPARAM)&lvItem);
}
//---------------------------------------------------------------------------

void clsProfilesDialog::RenameProfile(const int &iProfile) {
    LineDialog * pRenameProfileDlg = new (std::nothrow) LineDialog(&OnRenameProfileOk);

    if(pRenameProfileDlg != NULL) {
        pRenameProfileDlg->DoModal(hWndWindowItems[WINDOW_HANDLE], clsLanguageManager::mPtr->sTexts[LAN_NEW_PROFILE_NAME], clsProfileManager::mPtr->ppProfilesTable[iProfile]->sName);
    }
}
//------------------------------------------------------------------------------

void clsProfilesDialog::RemoveProfile(const uint16_t &iProfile) {
    ::SendMessage(hWndWindowItems[LV_PROFILES], LVM_DELETEITEM, iProfile, 0);

    int iSel = (int)::SendMessage(hWndWindowItems[LV_PROFILES], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    if(iSel == -1) {
        LVITEM lvItem = { 0 };
        lvItem.mask = LVIF_STATE;
        lvItem.state = LVIS_SELECTED;
        lvItem.stateMask = LVIS_SELECTED;

        ::SendMessage(hWndWindowItems[LV_PROFILES], LVM_SETITEMSTATE, 0, (LPARAM)&lvItem);
    }

    UpdateUpDown();
}
//------------------------------------------------------------------------------

void clsProfilesDialog::UpdateUpDown() {
    int iSel = (int)::SendMessage(hWndWindowItems[LV_PROFILES], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    if(iSel == -1) {
		::EnableWindow(hWndWindowItems[BTN_MOVE_UP], FALSE);
		::EnableWindow(hWndWindowItems[BTN_MOVE_DOWN], FALSE);
	} else if(iSel == 0) {
		::EnableWindow(hWndWindowItems[BTN_MOVE_UP], FALSE);
		if(iSel == (clsProfileManager::mPtr->ui16ProfileCount-1)) {
			::EnableWindow(hWndWindowItems[BTN_MOVE_DOWN], FALSE);
		} else {
            ::EnableWindow(hWndWindowItems[BTN_MOVE_DOWN], TRUE);
		}
	} else if(iSel == (clsProfileManager::mPtr->ui16ProfileCount-1)) {
		::EnableWindow(hWndWindowItems[BTN_MOVE_UP], TRUE);
		::EnableWindow(hWndWindowItems[BTN_MOVE_DOWN], FALSE);
	} else {
		::EnableWindow(hWndWindowItems[BTN_MOVE_UP], TRUE);
		::EnableWindow(hWndWindowItems[BTN_MOVE_DOWN], TRUE);
	}
}
//------------------------------------------------------------------------------

void clsProfilesDialog::AddProfile() {
    LVITEM lvItem = { 0 };
    lvItem.mask = LVIF_PARAM | LVIF_TEXT;
    lvItem.iItem = clsProfileManager::mPtr->ui16ProfileCount-1;
    lvItem.lParam = (LPARAM)clsProfileManager::mPtr->ppProfilesTable[clsProfileManager::mPtr->ui16ProfileCount-1];
    lvItem.pszText = clsProfileManager::mPtr->ppProfilesTable[clsProfileManager::mPtr->ui16ProfileCount-1]->sName;

    ::SendMessage(hWndWindowItems[LV_PROFILES], LVM_INSERTITEM, 0, (LPARAM)&lvItem);

    UpdateUpDown();
}
//------------------------------------------------------------------------------

void clsProfilesDialog::MoveDown(const uint16_t &iProfile) {
    HWND hWndFocus = ::GetFocus();

    LVITEM lvItem1 = { 0 };
    lvItem1.mask = LVIF_PARAM | LVIF_STATE;
    lvItem1.iItem = iProfile;
    lvItem1.stateMask = LVIS_SELECTED;

    if((BOOL)::SendMessage(hWndWindowItems[LV_PROFILES], LVM_GETITEM, 0, (LPARAM)&lvItem1) == FALSE) {
        return;
    }

    LVITEM lvItem2 = { 0 };
    lvItem2.mask = LVIF_PARAM | LVIF_STATE;
    lvItem2.iItem = iProfile+1;
    lvItem2.stateMask = LVIS_SELECTED;

    if((BOOL)::SendMessage(hWndWindowItems[LV_PROFILES], LVM_GETITEM, 0, (LPARAM)&lvItem2) == FALSE) {
        return;
    }

    lvItem1.mask |= LVIF_TEXT;
    lvItem1.iItem++;
    lvItem1.pszText = clsProfileManager::mPtr->ppProfilesTable[lvItem1.iItem]->sName;

    lvItem2.mask |= LVIF_TEXT;
    lvItem2.iItem--;
    lvItem2.pszText = clsProfileManager::mPtr->ppProfilesTable[lvItem2.iItem]->sName;

    ::SendMessage(hWndWindowItems[LV_PROFILES], LVM_SETITEM, 0, (LPARAM)&lvItem1);
    ::SendMessage(hWndWindowItems[LV_PROFILES], LVM_SETITEM, 0, (LPARAM)&lvItem2);

    if(iProfile == clsProfileManager::mPtr->ui16ProfileCount-2 && hWndFocus == hWndWindowItems[BTN_MOVE_DOWN]) {
        ::SetFocus(hWndWindowItems[BTN_MOVE_UP]);
    } else if(hWndFocus == hWndWindowItems[BTN_MOVE_DOWN] || hWndFocus == hWndWindowItems[BTN_MOVE_UP]) {
        ::SetFocus(hWndFocus);
    }
}
//------------------------------------------------------------------------------

void clsProfilesDialog::MoveUp(const uint16_t &iProfile) {
    HWND hWndFocus = ::GetFocus();

    LVITEM lvItem1 = { 0 };
    lvItem1.mask = LVIF_PARAM | LVIF_STATE;
    lvItem1.iItem = iProfile;
    lvItem1.stateMask = LVIS_SELECTED;

    if((BOOL)::SendMessage(hWndWindowItems[LV_PROFILES], LVM_GETITEM, 0, (LPARAM)&lvItem1) == FALSE) {
        return;
    }

    LVITEM lvItem2 = { 0 };
    lvItem2.mask = LVIF_PARAM | LVIF_STATE;
    lvItem2.iItem = iProfile-1;
    lvItem2.stateMask = LVIS_SELECTED;

    if((BOOL)::SendMessage(hWndWindowItems[LV_PROFILES], LVM_GETITEM, 0, (LPARAM)&lvItem2) == FALSE) {
        return;
    }

    lvItem1.mask |= LVIF_TEXT;
    lvItem1.iItem--;
    lvItem1.pszText = clsProfileManager::mPtr->ppProfilesTable[lvItem1.iItem]->sName;

    lvItem2.mask |= LVIF_TEXT;
    lvItem2.iItem++;
    lvItem2.pszText = clsProfileManager::mPtr->ppProfilesTable[lvItem2.iItem]->sName;

    ::SendMessage(hWndWindowItems[LV_PROFILES], LVM_SETITEM, 0, (LPARAM)&lvItem1);
    ::SendMessage(hWndWindowItems[LV_PROFILES], LVM_SETITEM, 0, (LPARAM)&lvItem2);

    if(iProfile == 1 && hWndFocus == hWndWindowItems[BTN_MOVE_UP]) {
        ::SetFocus(hWndWindowItems[BTN_MOVE_DOWN]);
    } else if(hWndFocus == hWndWindowItems[BTN_MOVE_DOWN] || hWndFocus == hWndWindowItems[BTN_MOVE_UP]) {
        ::SetFocus(hWndFocus);
    }
}
//------------------------------------------------------------------------------

void clsProfilesDialog::OnPermissionChanged(const LPNMLISTVIEW &pListView) {
    if(bIgnoreItemChanged == true || pListView->iItem == -1 || (pListView->uNewState & LVIS_STATEIMAGEMASK) == (pListView->uOldState & LVIS_STATEIMAGEMASK)) {
        return;
    }

    int iSel = (int)::SendMessage(hWndWindowItems[LV_PROFILES], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    if(iSel == -1) {
        return;
    }

    bool bEnabled = ((((pListView->uNewState & LVIS_STATEIMAGEMASK) >> 12) - 1) == 0 ? false : true);

    LVITEM lvItem = { 0 };
    lvItem.mask = LVIF_PARAM;
    lvItem.iItem = pListView->iItem;

    if((BOOL)::SendMessage(hWndWindowItems[LV_PERMISSIONS], LVM_GETITEM, 0, (LPARAM)&lvItem) == FALSE) {
        return;
    }

    clsProfileManager::mPtr->ppProfilesTable[iSel]->bPermissions[(int)lvItem.lParam] = bEnabled;
}
//------------------------------------------------------------------------------
