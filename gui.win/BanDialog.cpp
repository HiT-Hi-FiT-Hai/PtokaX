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
#include "../core/stdinc.h"
//---------------------------------------------------------------------------
#include "BanDialog.h"
//---------------------------------------------------------------------------
#include "../core/hashBanManager.h"
#include "../core/LanguageManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "BansDialog.h"
//---------------------------------------------------------------------------
BanDialog * pBanDialog = NULL;
//---------------------------------------------------------------------------
static ATOM atomBanDialog = 0;
//---------------------------------------------------------------------------

BanDialog::BanDialog() {
    memset(&hWndWindowItems, 0, (sizeof(hWndWindowItems) / sizeof(hWndWindowItems[0])) * sizeof(HWND));

    pBanToChange = NULL;
}
//---------------------------------------------------------------------------

BanDialog::~BanDialog() {
    pBanDialog = NULL;
}
//---------------------------------------------------------------------------

LRESULT CALLBACK BanDialog::StaticBanDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    BanDialog * pBanDialog = (BanDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if(pBanDialog == NULL) {
        return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

	return pBanDialog->BanDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT BanDialog::BanDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case IDOK:
                    if(OnAccept() == false) {
                        return 0;
                    }
                case IDCANCEL:
                    ::PostMessage(hWndWindowItems[WINDOW_HANDLE], WM_CLOSE, 0, 0);
                    return 0;
                case (EDT_NICK+100):
                    if(HIWORD(wParam) == EN_CHANGE) {
                        char buf[65];
                        ::GetWindowText((HWND)lParam, buf, 65);

                        bool bChanged = false;

                        for(uint16_t ui16i = 0; buf[ui16i] != '\0'; ui16i++) {
                            if(buf[ui16i] == '|' || buf[ui16i] == '$' || buf[ui16i] == ' ') {
                                strcpy(buf+ui16i, buf+ui16i+1);
                                bChanged = true;
                                ui16i--;
                            }
                        }

                        if(bChanged == true) {
                            int iStart, iEnd;

                            ::SendMessage((HWND)lParam, EM_GETSEL, (WPARAM)&iStart, (LPARAM)&iEnd);

                            ::SetWindowText((HWND)lParam, buf);

                            ::SendMessage((HWND)lParam, EM_SETSEL, iStart, iEnd);
                        }

                        return 0;
                    }

                    break;
                case BTN_IP_BAN:
                    if(HIWORD(wParam) == BN_CLICKED) {
                        bool bChecked = ::SendMessage(hWndWindowItems[BTN_IP_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
                        ::EnableWindow(hWndWindowItems[BTN_FULL_BAN], bChecked == true ? TRUE : FALSE);

                        return 0;
                    }

                    break;
                case RB_PERM_BAN:
                    if(HIWORD(wParam) == BN_CLICKED) {
                        ::EnableWindow(hWndWindowItems[DT_TEMP_BAN_EXPIRE_DATE], FALSE);
                        ::EnableWindow(hWndWindowItems[DT_TEMP_BAN_EXPIRE_TIME], FALSE);
                    }

                    break;
                case RB_TEMP_BAN:
                    if(HIWORD(wParam) == BN_CLICKED) {
                        ::EnableWindow(hWndWindowItems[DT_TEMP_BAN_EXPIRE_DATE], TRUE);
                        ::EnableWindow(hWndWindowItems[DT_TEMP_BAN_EXPIRE_TIME], TRUE);
                    }

                    break;
            }

            break;
        case WM_CLOSE:
            ::EnableWindow(::GetParent(hWndWindowItems[WINDOW_HANDLE]), TRUE);
            g_hWndActiveDialog = NULL;
            break;
        case WM_NCDESTROY:
            delete this;
            return ::DefWindowProc(hWndWindowItems[WINDOW_HANDLE], uMsg, wParam, lParam);
        case WM_SETFOCUS:
            ::SetFocus(hWndWindowItems[EDT_NICK]);
            return 0;
    }

	return ::DefWindowProc(hWndWindowItems[WINDOW_HANDLE], uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void BanDialog::DoModal(HWND hWndParent, BanItem * pBan/* = NULL*/) {
    pBanToChange = pBan;

    if(atomBanDialog == 0) {
        WNDCLASSEX m_wc;
        memset(&m_wc, 0, sizeof(WNDCLASSEX));
        m_wc.cbSize = sizeof(WNDCLASSEX);
        m_wc.lpfnWndProc = ::DefWindowProc;
        m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
        m_wc.lpszClassName = "PtokaX_BanDialog";
        m_wc.hInstance = g_hInstance;
        m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
        m_wc.style = CS_HREDRAW | CS_VREDRAW;

        atomBanDialog = ::RegisterClassEx(&m_wc);
    }

    RECT rcParent;
    ::GetWindowRect(hWndParent, &rcParent);

    int iX = (rcParent.left + (((rcParent.right-rcParent.left))/2)) - (ScaleGui(300) / 2);
    int iY = (rcParent.top + ((rcParent.bottom-rcParent.top)/2)) - (ScaleGui(394) / 2);

    hWndWindowItems[WINDOW_HANDLE] = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomBanDialog), LanguageManager->sTexts[LAN_BAN],
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, ScaleGui(300), ScaleGui(394),
        hWndParent, NULL, g_hInstance, NULL);

    if(hWndWindowItems[WINDOW_HANDLE] == NULL) {
        return;
    }

    g_hWndActiveDialog = hWndWindowItems[WINDOW_HANDLE];

    ::SetWindowLongPtr(hWndWindowItems[WINDOW_HANDLE], GWLP_USERDATA, (LONG_PTR)this);
    ::SetWindowLongPtr(hWndWindowItems[WINDOW_HANDLE], GWLP_WNDPROC, (LONG_PTR)StaticBanDialogProc);

    ::GetClientRect(hWndWindowItems[WINDOW_HANDLE], &rcParent);

    {
        int iHeight = iOneLineOneChecksGB + iOneLineTwoChecksGB + (2 * iOneLineGB) + (iGroupBoxMargin + iCheckHeight + iOneLineGB + 5) + iEditHeight + 6;

        int iDiff = rcParent.bottom - iHeight;

        if(iDiff != 0) {
            ::GetWindowRect(hWndParent, &rcParent);

            iY = (rcParent.top + ((rcParent.bottom-rcParent.top)/2)) - ((ScaleGui(307) - iDiff) / 2);

            ::GetWindowRect(hWndWindowItems[WINDOW_HANDLE], &rcParent);

            ::SetWindowPos(hWndWindowItems[WINDOW_HANDLE], NULL, iX, iY, (rcParent.right-rcParent.left), (rcParent.bottom-rcParent.top) - iDiff, SWP_NOZORDER);
        }
    }

    ::GetClientRect(hWndWindowItems[WINDOW_HANDLE], &rcParent);

    hWndWindowItems[GB_NICK] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_NICK], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        3, 0, rcParent.right - 6, iOneLineOneChecksGB, hWndWindowItems[WINDOW_HANDLE], NULL, g_hInstance, NULL);

    hWndWindowItems[EDT_NICK] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        11, iGroupBoxMargin, rcParent.right - 22, iEditHeight, hWndWindowItems[WINDOW_HANDLE], (HMENU)(EDT_NICK+100), g_hInstance, NULL);
    ::SendMessage(hWndWindowItems[EDT_NICK], EM_SETLIMITTEXT, 64, 0);

    hWndWindowItems[BTN_NICK_BAN] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_NICK_BAN], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        11, iGroupBoxMargin + iEditHeight + 4, rcParent.right - 22, iCheckHeight, hWndWindowItems[WINDOW_HANDLE], NULL, g_hInstance, NULL);

    int iPosX = iOneLineOneChecksGB;

    hWndWindowItems[GB_IP] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_IP], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        3, iPosX, rcParent.right - 6, iOneLineTwoChecksGB, hWndWindowItems[WINDOW_HANDLE], NULL, g_hInstance, NULL);

    hWndWindowItems[EDT_IP] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        11, iPosX + iGroupBoxMargin, rcParent.right - 22, iEditHeight, hWndWindowItems[WINDOW_HANDLE], NULL, g_hInstance, NULL);
    ::SendMessage(hWndWindowItems[EDT_IP], EM_SETLIMITTEXT, 39, 0);

    hWndWindowItems[BTN_IP_BAN] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_IP_BAN], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        11, iPosX + iGroupBoxMargin + iEditHeight + 4, rcParent.right - 22, iCheckHeight, hWndWindowItems[WINDOW_HANDLE], (HMENU)BTN_IP_BAN, g_hInstance, NULL);

    hWndWindowItems[BTN_FULL_BAN] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_FULL_BAN], WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED | BS_AUTOCHECKBOX,
        11, iPosX + iGroupBoxMargin + iEditHeight + iCheckHeight + 7, rcParent.right - 22, iCheckHeight, hWndWindowItems[WINDOW_HANDLE], NULL, g_hInstance, NULL);

    iPosX += iOneLineTwoChecksGB;

    hWndWindowItems[GB_REASON] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_REASON], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        3, iPosX, rcParent.right - 6, iOneLineGB, hWndWindowItems[WINDOW_HANDLE], NULL, g_hInstance, NULL);

    hWndWindowItems[EDT_REASON] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        11, iPosX + iGroupBoxMargin, rcParent.right - 22, iEditHeight, hWndWindowItems[WINDOW_HANDLE], (HMENU)EDT_REASON, g_hInstance, NULL);
    ::SendMessage(hWndWindowItems[EDT_REASON], EM_SETLIMITTEXT, 255, 0);

    iPosX += iOneLineGB;

    hWndWindowItems[GB_BY] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_CREATED_BY], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        3, iPosX, rcParent.right - 6, iOneLineGB, hWndWindowItems[WINDOW_HANDLE], NULL, g_hInstance, NULL);

    hWndWindowItems[EDT_BY] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        11, iPosX + iGroupBoxMargin, rcParent.right - 22, iEditHeight, hWndWindowItems[WINDOW_HANDLE], (HMENU)EDT_BY, g_hInstance, NULL);
    ::SendMessage(hWndWindowItems[EDT_BY], EM_SETLIMITTEXT, 64, 0);

    iPosX += iOneLineGB;

    hWndWindowItems[GB_BAN_TYPE] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, "", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        3, iPosX, rcParent.right - 6, iGroupBoxMargin + iCheckHeight + iOneLineGB + 5, hWndWindowItems[WINDOW_HANDLE], NULL, g_hInstance, NULL);

    hWndWindowItems[GB_TEMP_BAN] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, NULL, WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        8, iPosX + iGroupBoxMargin + iCheckHeight, rcParent.right - 16, iOneLineGB, hWndWindowItems[WINDOW_HANDLE], NULL, g_hInstance, NULL);

    hWndWindowItems[RB_PERM_BAN] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_PERMANENT], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON,
        16, iPosX + iGroupBoxMargin, rcParent.right - 32, iCheckHeight, hWndWindowItems[WINDOW_HANDLE], (HMENU)RB_PERM_BAN, g_hInstance, NULL);
    ::SendMessage(hWndWindowItems[RB_PERM_BAN], BM_SETCHECK, BST_CHECKED, 0);

    int iThird = (rcParent.right - 32) / 3;

    hWndWindowItems[RB_TEMP_BAN] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_TEMPORARY], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON,
        16, iPosX + (2 * iGroupBoxMargin) + iCheckHeight + ((iEditHeight - iCheckHeight) / 2), iThird - 2, iCheckHeight, hWndWindowItems[WINDOW_HANDLE], (HMENU)RB_TEMP_BAN, g_hInstance, NULL);
    ::SendMessage(hWndWindowItems[RB_TEMP_BAN], BM_SETCHECK, BST_UNCHECKED, 0);

    hWndWindowItems[DT_TEMP_BAN_EXPIRE_DATE] = ::CreateWindowEx(0, DATETIMEPICK_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED | DTS_SHORTDATECENTURYFORMAT,
        iThird + 16, iPosX + (2 * iGroupBoxMargin) + iCheckHeight, iThird - 2, iEditHeight, hWndWindowItems[WINDOW_HANDLE], NULL, g_hInstance, NULL);

    hWndWindowItems[DT_TEMP_BAN_EXPIRE_TIME] = ::CreateWindowEx(0, DATETIMEPICK_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED | DTS_TIMEFORMAT | DTS_UPDOWN,
        (iThird * 2) + 19, iPosX + (2 * iGroupBoxMargin) + iCheckHeight, iThird - 2, iEditHeight, hWndWindowItems[WINDOW_HANDLE], NULL, g_hInstance, NULL);

    iPosX += iGroupBoxMargin + iCheckHeight + iOneLineGB + 9;

    hWndWindowItems[BTN_ACCEPT] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_ACCEPT], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        2, iPosX, (rcParent.right / 2) - 3, iEditHeight, hWndWindowItems[WINDOW_HANDLE], (HMENU)IDOK, g_hInstance, NULL);

    hWndWindowItems[BTN_DISCARD] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_DISCARD], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        (rcParent.right / 2) + 2, iPosX, (rcParent.right / 2) - 4, iEditHeight, hWndWindowItems[WINDOW_HANDLE], (HMENU)IDCANCEL, g_hInstance, NULL);

    for(uint8_t ui8i = 0; ui8i < (sizeof(hWndWindowItems) / sizeof(hWndWindowItems[0])); ui8i++) {
        if(hWndWindowItems[ui8i] == NULL) {
            return;
        }

        ::SendMessage(hWndWindowItems[ui8i], WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
    }

    if(pBanToChange != NULL) {
        if(pBanToChange->sNick != NULL) {
            ::SetWindowText(hWndWindowItems[EDT_NICK], pBanToChange->sNick);

            if(((pBanToChange->ui8Bits & hashBanMan::NICK) == hashBanMan::NICK) == true) {
                ::SendMessage(hWndWindowItems[BTN_NICK_BAN], BM_SETCHECK, BST_CHECKED, 0);
            }
        }

        ::SetWindowText(hWndWindowItems[EDT_IP], pBanToChange->sIp);

        if(((pBanToChange->ui8Bits & hashBanMan::IP) == hashBanMan::IP) == true) {
            ::SendMessage(hWndWindowItems[BTN_IP_BAN], BM_SETCHECK, BST_CHECKED, 0);

            ::EnableWindow(hWndWindowItems[BTN_FULL_BAN], TRUE);

            if(((pBanToChange->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == true) {
                ::SendMessage(hWndWindowItems[BTN_FULL_BAN], BM_SETCHECK, BST_CHECKED, 0);
            }
        }

        if(pBanToChange->sReason != NULL) {
            ::SetWindowText(hWndWindowItems[EDT_REASON], pBanToChange->sReason);
        }

        if(pBanToChange->sBy != NULL) {
            ::SetWindowText(hWndWindowItems[EDT_BY], pBanToChange->sBy);
        }

        if(((pBanToChange->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
            ::SendMessage(hWndWindowItems[RB_PERM_BAN], BM_SETCHECK, BST_UNCHECKED, 0);
            ::SendMessage(hWndWindowItems[RB_TEMP_BAN], BM_SETCHECK, BST_CHECKED, 0);

            ::EnableWindow(hWndWindowItems[DT_TEMP_BAN_EXPIRE_DATE], TRUE);
            ::EnableWindow(hWndWindowItems[DT_TEMP_BAN_EXPIRE_TIME], TRUE);

            SYSTEMTIME stDateTime = { 0 };

            struct tm *tm = localtime(&pBanToChange->tempbanexpire);

            stDateTime.wDay = (WORD)tm->tm_mday;
            stDateTime.wMonth = (WORD)(tm->tm_mon+1);
            stDateTime.wYear = (WORD)(tm->tm_year+1900);

            stDateTime.wHour = (WORD)tm->tm_hour;
            stDateTime.wMinute = (WORD)tm->tm_min;
            stDateTime.wSecond = (WORD)tm->tm_sec;

            ::SendMessage(hWndWindowItems[DT_TEMP_BAN_EXPIRE_DATE], DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&stDateTime);
            ::SendMessage(hWndWindowItems[DT_TEMP_BAN_EXPIRE_TIME], DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&stDateTime);
        }
    }

    ::EnableWindow(hWndParent, FALSE);

    ::ShowWindow(hWndWindowItems[WINDOW_HANDLE], SW_SHOW);
}
//------------------------------------------------------------------------------

bool BanDialog::OnAccept() {
    bool bNickBan = ::SendMessage(hWndWindowItems[BTN_NICK_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
    bool bIpBan = ::SendMessage(hWndWindowItems[BTN_IP_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;

	if(bNickBan == false && bIpBan == false) {
		::MessageBox(hWndWindowItems[WINDOW_HANDLE], LanguageManager->sTexts[LAN_YOU_CANT_CREATE_BAN_WITHOUT_NICK_OR_IP], sTitle.c_str(), MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

    int iNickLen = ::GetWindowTextLength(hWndWindowItems[EDT_NICK]);

	if(bNickBan == true && iNickLen == 0) {
		::MessageBox(hWndWindowItems[WINDOW_HANDLE], LanguageManager->sTexts[LAN_FOR_NICK_BAN_SPECIFY_NICK], sTitle.c_str(), MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

    int iIpLen = ::GetWindowTextLength(hWndWindowItems[EDT_IP]);

    char sIP[40];
    ::GetWindowText(hWndWindowItems[EDT_IP], sIP, 40);

    uint8_t ui128IpHash[16];
    memset(ui128IpHash, 0, 16);

	if(bIpBan == true) {
		if(iIpLen == 0) {
			::MessageBox(hWndWindowItems[WINDOW_HANDLE], LanguageManager->sTexts[LAN_FOR_IP_BAN_SPECIFY_IP], sTitle.c_str(), MB_OK | MB_ICONEXCLAMATION);
			return false;
		} else if(HashIP(sIP, ui128IpHash) == false) {
			::MessageBox(hWndWindowItems[WINDOW_HANDLE], (string(sIP) + " " + LanguageManager->sTexts[LAN_IS_NOT_VALID_IP_ADDRESS]).c_str(), sTitle.c_str(), MB_OK | MB_ICONEXCLAMATION);
			return false;
        }
	}

	time_t acc_time;
	time(&acc_time);

	time_t ban_time = 0;

    bool bTempBan = ::SendMessage(hWndWindowItems[RB_TEMP_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;

	if(bTempBan == true) {
        SYSTEMTIME stDate = { 0 };
        SYSTEMTIME stTime = { 0 };

        if(::SendMessage(hWndWindowItems[DT_TEMP_BAN_EXPIRE_DATE], DTM_GETSYSTEMTIME, 0, (LPARAM)&stDate) != GDT_VALID ||
            ::SendMessage(hWndWindowItems[DT_TEMP_BAN_EXPIRE_TIME], DTM_GETSYSTEMTIME, 0, (LPARAM)&stTime) != GDT_VALID) {
            ::MessageBox(hWndWindowItems[WINDOW_HANDLE], LanguageManager->sTexts[LAN_BAD_TIME_SPECIFIED], sTitle.c_str(), MB_OK | MB_ICONEXCLAMATION);

            return false;
        }

		struct tm *tm = localtime(&acc_time);

		tm->tm_mday = stDate.wDay;
		tm->tm_mon = (stDate.wMonth-1);
		tm->tm_year = (stDate.wYear-1900);

		tm->tm_hour = stTime.wHour;
		tm->tm_min = stTime.wMinute;
		tm->tm_sec = stTime.wSecond;

		tm->tm_isdst = -1;

		ban_time = mktime(tm);

		if(ban_time <= acc_time || ban_time == (time_t)-1) {
			::MessageBox(hWndWindowItems[WINDOW_HANDLE], LanguageManager->sTexts[LAN_BAD_TIME_SPECIFIED_BAN_EXPIRED], sTitle.c_str(), MB_OK | MB_ICONEXCLAMATION);

			return false;
        }
	}

	if(pBanToChange == NULL) {
		BanItem * pBan = new BanItem();
		if(pBan == NULL) {
			string sDbgstr = "[BUF] BanDialog::OnAccept! "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
			AppendSpecialLog(sDbgstr);
			return false;
		}

        if(iIpLen != 0) {
            strcpy(pBan->sIp, sIP);
            memcpy(pBan->ui128IpHash, ui128IpHash, 16);

            if(bIpBan == true) {
				pBan->ui8Bits |= hashBanMan::IP;

                if(::SendMessage(hWndWindowItems[BTN_FULL_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED) {
					pBan->ui8Bits |= hashBanMan::FULL;
                }
            }
        }

		if(bTempBan == true) {
			pBan->ui8Bits |= hashBanMan::TEMP;
            pBan->tempbanexpire = ban_time;
		} else {
			pBan->ui8Bits |= hashBanMan::PERM;
        }

		if(iNickLen != 0) {
			pBan->sNick = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iNickLen+1);
			if(pBan->sNick == NULL) {
				string sDbgstr = "[BUF] Cannot allocate "+string(iNickLen+1)+
					" bytes of memory for sNick in BanDialog::OnAccept! "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
				AppendSpecialLog(sDbgstr);
				delete pBan;

				return false;
			}

			::GetWindowText(hWndWindowItems[EDT_NICK], pBan->sNick, iNickLen+1);
			pBan->ui32NickHash = HashNick(pBan->sNick, iNickLen);

			if(bNickBan == true) {
				pBan->ui8Bits |= hashBanMan::NICK;

                // PPK ... not allow same nickbans !
				BanItem *nxtBan = hashBanManager->FindNick(pBan->ui32NickHash, acc_time, pBan->sNick);

                if(nxtBan != NULL) {
                    // PPK ... same ban exist, delete new
                    delete pBan;

                    ::MessageBox(hWndWindowItems[WINDOW_HANDLE], LanguageManager->sTexts[LAN_SIMILAR_BAN_EXIST], sTitle.c_str(), MB_OK | MB_ICONEXCLAMATION);
                    return false;
                }
			}
		}

        if(bIpBan == true) {
			BanItem *nxtBan = hashBanManager->FindIP(pBan->ui128IpHash, acc_time);

            if(nxtBan != NULL) {
                // PPK ... same ban exist, delete new
                delete pBan;

                ::MessageBox(hWndWindowItems[WINDOW_HANDLE], LanguageManager->sTexts[LAN_SIMILAR_BAN_EXIST], sTitle.c_str(), MB_OK | MB_ICONEXCLAMATION);
                return false;
            }
        }

        int iReasonLen = ::GetWindowTextLength(hWndWindowItems[EDT_REASON]);

		if(iReasonLen != 0) {
            pBan->sReason = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iReasonLen+1);
            if(pBan->sReason == NULL) {
    			string sDbgstr = "[BUF] Cannot allocate "+string(iReasonLen+1)+
    				" bytes of memory for sReason in BanDialog::OnAccept! "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
                AppendSpecialLog(sDbgstr);
                delete pBan;

                return false;
            }

            ::GetWindowText(hWndWindowItems[EDT_REASON], pBan->sReason, iReasonLen+1);
        }

        int iByLen = ::GetWindowTextLength(hWndWindowItems[EDT_BY]);

        if(iByLen != 0) {
            pBan->sBy = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iByLen+1);
            if(pBan->sBy == NULL) {
                string sDbgstr = "[BUF] Cannot allocate "+string(iByLen+1)+
					" bytes of memory for sBy in BanDialog::OnAccept! "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
                AppendSpecialLog(sDbgstr);
                delete pBan;

    			return false;
            }

            ::GetWindowText(hWndWindowItems[EDT_BY], pBan->sBy, iByLen+1);
		}

		hashBanManager->Add(pBan);

		return true;
	} else {
		if(pBanToChange->sIp[0] != '\0' && iIpLen == 0) {
			if(((pBanToChange->ui8Bits & hashBanMan::IP) == hashBanMan::IP) == true) {
				hashBanManager->RemFromIpTable(pBanToChange);
            }

			pBanToChange->sIp[0] = '\0';
			memset(pBanToChange->ui128IpHash, 0, 16);

			pBanToChange->ui8Bits &= ~hashBanMan::IP;
			pBanToChange->ui8Bits &= ~hashBanMan::FULL;
		} else if((pBanToChange->sIp[0] == '\0' && iIpLen != 0) ||
			(pBanToChange->sIp[0] != '\0' && iIpLen != 0 && strcmp(pBanToChange->sIp, sIP) != NULL)) {
			if(((pBanToChange->ui8Bits & hashBanMan::IP) == hashBanMan::IP) == true) {
				hashBanManager->RemFromIpTable(pBanToChange);
			}

			strcpy(pBanToChange->sIp, sIP);
			memcpy(pBanToChange->ui128IpHash, ui128IpHash, 16);

			if(bIpBan == true) {
				pBanToChange->ui8Bits |= hashBanMan::IP;

				if(::SendMessage(hWndWindowItems[BTN_FULL_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED) {
					pBanToChange->ui8Bits |= hashBanMan::FULL;
				} else {
					pBanToChange->ui8Bits &= ~hashBanMan::FULL;
                }

				hashBanManager->Add2IpTable(pBanToChange);
			} else {
				pBanToChange->ui8Bits &= ~hashBanMan::IP;
				pBanToChange->ui8Bits &= ~hashBanMan::FULL;
            }
		}

		if(iIpLen != 0) {
			if(bIpBan == true) {
				if(((pBanToChange->ui8Bits & hashBanMan::IP) == hashBanMan::IP) == false) {
					hashBanManager->Add2IpTable(pBanToChange);
				}

				pBanToChange->ui8Bits |= hashBanMan::IP;

				if(::SendMessage(hWndWindowItems[BTN_FULL_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED) {
					pBanToChange->ui8Bits |= hashBanMan::FULL;
				} else {
                    pBanToChange->ui8Bits &= ~hashBanMan::FULL;
                }
			} else {
				if(((pBanToChange->ui8Bits & hashBanMan::IP) == hashBanMan::IP) == true) {
					hashBanManager->RemFromIpTable(pBanToChange);
				}

				pBanToChange->ui8Bits &= ~hashBanMan::IP;
				pBanToChange->ui8Bits &= ~hashBanMan::FULL;
			}
		}

		if(bTempBan == true) {
			if(((pBanToChange->ui8Bits & hashBanMan::PERM) == hashBanMan::PERM) == true) {
				pBanToChange->ui8Bits &= ~hashBanMan::PERM;

				// remove from perm
				if(pBanToChange->prev == NULL) {
					if(pBanToChange->next == NULL) {
						hashBanManager->PermBanListS = NULL;
						hashBanManager->PermBanListE = NULL;
					} else {
						pBanToChange->next->prev = NULL;
						hashBanManager->PermBanListS = pBanToChange->next;
					}
				} else if(pBanToChange->next == NULL) {
					pBanToChange->prev->next = NULL;
					hashBanManager->PermBanListE = pBanToChange->prev;
				} else {
					pBanToChange->prev->next = pBanToChange->next;
					pBanToChange->next->prev = pBanToChange->prev;
				}

                pBanToChange->prev = NULL;
                pBanToChange->next = NULL;

				// add to temp
				if(hashBanManager->TempBanListE == NULL) {
					hashBanManager->TempBanListS = pBanToChange;
					hashBanManager->TempBanListE = pBanToChange;
				} else {
					hashBanManager->TempBanListE->next = pBanToChange;
					pBanToChange->prev = hashBanManager->TempBanListE;
					hashBanManager->TempBanListE = pBanToChange;
				}
			}

			pBanToChange->ui8Bits |= hashBanMan::TEMP;
			pBanToChange->tempbanexpire = ban_time;
		} else {
			if(((pBanToChange->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
				pBanToChange->ui8Bits &= ~hashBanMan::TEMP;

				// remove from temp
				if(pBanToChange->prev == NULL) {
					if(pBanToChange->next == NULL) {
						hashBanManager->TempBanListS = NULL;
						hashBanManager->TempBanListE = NULL;
					} else {
						pBanToChange->next->prev = NULL;
						hashBanManager->TempBanListS = pBanToChange->next;
					}
				} else if(pBanToChange->next == NULL) {
					pBanToChange->prev->next = NULL;
					hashBanManager->TempBanListE = pBanToChange->prev;
				} else {
					pBanToChange->prev->next = pBanToChange->next;
					pBanToChange->next->prev = pBanToChange->prev;
				}

                pBanToChange->prev = NULL;
                pBanToChange->next = NULL;

				// add to perm
				if(hashBanManager->PermBanListE == NULL) {
					hashBanManager->PermBanListS = pBanToChange;
					hashBanManager->PermBanListE = pBanToChange;
				} else {
					hashBanManager->PermBanListE->next = pBanToChange;
					pBanToChange->prev = hashBanManager->PermBanListE;
					hashBanManager->PermBanListE = pBanToChange;
				}
			}

			pBanToChange->ui8Bits |= hashBanMan::PERM;
			pBanToChange->tempbanexpire = 0;
        }

		char * sNick = NULL;
		if(iNickLen != 0) {
            sNick = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iNickLen+1);
            if(sNick == NULL) {
                string sDbgstr = "[BUF] Cannot allocate "+string(iNickLen+1)+
                    " bytes of memory for sNick in BanDialog::OnAccept! "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
                AppendSpecialLog(sDbgstr);

                return false;
            }

            ::GetWindowText(hWndWindowItems[EDT_NICK], sNick, iNickLen+1);
        }

		if(pBanToChange->sNick != NULL && iNickLen == 0) {
			if(((pBanToChange->ui8Bits & hashBanMan::NICK) == hashBanMan::NICK) == true) {
				hashBanManager->RemFromNickTable(pBanToChange);
            }

			if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pBanToChange->sNick) == 0) {
				string sDbgstr = "[BUF] Cannot deallocate sNick in BanDialog::OnAccept! "+string((uint32_t)GetLastError())+" "+
					string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
				AppendSpecialLog(sDbgstr);
			}
			pBanToChange->sNick = NULL;

			pBanToChange->ui32NickHash = 0;

			pBanToChange->ui8Bits &= ~hashBanMan::NICK;
		} else if((pBanToChange->sNick == NULL && iNickLen != 0) ||
			(pBanToChange->sNick != NULL && iNickLen != 0 && strcmp(pBanToChange->sNick, sNick) != NULL)) {
			if(((pBanToChange->ui8Bits & hashBanMan::NICK) == hashBanMan::NICK) == true) {
				hashBanManager->RemFromNickTable(pBanToChange);
			}

			if(pBanToChange->sNick != NULL) {
				if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pBanToChange->sNick) == 0) {
					string sDbgstr = "[BUF] Cannot deallocate sNick in BanDialog::OnAccept! "+string((uint32_t)GetLastError())+" "+
						string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
					AppendSpecialLog(sDbgstr);
				}
				pBanToChange->sNick = NULL;
			}

			pBanToChange->sNick = sNick;

			pBanToChange->ui32NickHash = HashNick(pBanToChange->sNick, iNickLen);

			if(bNickBan == true) {
				pBanToChange->ui8Bits |= hashBanMan::NICK;

				hashBanManager->Add2NickTable(pBanToChange);
			} else {
				pBanToChange->ui8Bits &= ~hashBanMan::NICK;
			}
		}

        if(sNick != NULL && (pBanToChange->sNick != sNick)) {
			if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sNick) == 0) {
				string sDbgstr = "[BUF] Cannot deallocate sNick in BanDialog::OnAccept! "+string((uint32_t)GetLastError())+" "+
					string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
				AppendSpecialLog(sDbgstr);
			}
        }

		if(iNickLen != 0) {
			if(bNickBan == true) {
				if(((pBanToChange->ui8Bits & hashBanMan::NICK) == hashBanMan::NICK) == false) {
					hashBanManager->Add2NickTable(pBanToChange);
				}

				pBanToChange->ui8Bits |= hashBanMan::NICK;
			} else {
				if(((pBanToChange->ui8Bits & hashBanMan::NICK) == hashBanMan::NICK) == true) {
					hashBanManager->RemFromNickTable(pBanToChange);
				}

				pBanToChange->ui8Bits &= ~hashBanMan::NICK;
            }
		}


        int iReasonLen = ::GetWindowTextLength(hWndWindowItems[EDT_REASON]);

        char * sReason = NULL;
		if(iReasonLen != 0) {
            sReason = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iReasonLen+1);
            if(sReason == NULL) {
    			string sDbgstr = "[BUF] Cannot allocate "+string(iReasonLen+1)+
    				" bytes of memory for sReason in BanDialog::OnAccept! "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
                AppendSpecialLog(sDbgstr);

                return false;
            }

            ::GetWindowText(hWndWindowItems[EDT_REASON], sReason, iReasonLen+1);
        }

		if(iReasonLen != 0) {
			if(pBanToChange->sReason == NULL || strcmp(pBanToChange->sReason, sReason) != NULL) {
				if(pBanToChange->sReason != NULL) {
					if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pBanToChange->sReason) == 0) {
						string sDbgstr = "[BUF] Cannot deallocate sReason in BanDialog::OnAccept! "+string((uint32_t)GetLastError())+" "+
							string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
						AppendSpecialLog(sDbgstr);
					}
					pBanToChange->sReason = NULL;
				}

				pBanToChange->sReason = sReason;
			}
		} else if(pBanToChange->sReason != NULL) {
			if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pBanToChange->sReason) == 0) {
				string sDbgstr = "[BUF] Cannot deallocate sReason in BanDialog::OnAccept! "+string((uint32_t)GetLastError())+" "+
					string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
				AppendSpecialLog(sDbgstr);
			}

			pBanToChange->sReason = NULL;
        }

        if(sReason != NULL && (pBanToChange->sReason != sReason)) {
			if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sReason) == 0) {
				string sDbgstr = "[BUF] Cannot deallocate sReason in BanDialog::OnAccept! "+string((uint32_t)GetLastError())+" "+
					string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
				AppendSpecialLog(sDbgstr);
			}
        }

        int iByLen = ::GetWindowTextLength(hWndWindowItems[EDT_BY]);

        char * sBy = NULL;
        if(iByLen != 0) {
            sBy = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iByLen+1);
            if(sBy == NULL) {
                string sDbgstr = "[BUF] Cannot allocate "+string(iByLen+1)+
					" bytes of memory for sBy in BanDialog::OnAccept! "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
                AppendSpecialLog(sDbgstr);

    			return false;
            }

            ::GetWindowText(hWndWindowItems[EDT_BY], sBy, iByLen+1);
		}

		if(iByLen != 0) {
			if(pBanToChange->sBy == NULL || strcmp(pBanToChange->sBy, sBy) != NULL) {
				if(pBanToChange->sBy != NULL) {
					if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pBanToChange->sBy) == 0) {
						string sDbgstr = "[BUF] Cannot deallocate sBy in BanDialog::OnAccept! "+string((uint32_t)GetLastError())+" "+
							string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
						AppendSpecialLog(sDbgstr);
					}
					pBanToChange->sBy = NULL;
				}

				pBanToChange->sBy = sBy;
			}
		} else if(pBanToChange->sBy != NULL) {
			if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pBanToChange->sBy) == 0) {
				string sDbgstr = "[BUF] Cannot deallocate sBy in BanDialog::OnAccept! "+string((uint32_t)GetLastError())+" "+
					string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
				AppendSpecialLog(sDbgstr);
			}
			pBanToChange->sBy = NULL;
        }

        if(sBy != NULL && (pBanToChange->sBy != sBy)) {
			if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sBy) == 0) {
				string sDbgstr = "[BUF] Cannot deallocate sBy in BanDialog::OnAccept! "+string((uint32_t)GetLastError())+" "+
					string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
				AppendSpecialLog(sDbgstr);
			}
        }

        if(pBansDialog != NULL) {
            pBansDialog->RemoveBan(pBanToChange);
            pBansDialog->AddBan(pBanToChange);
        }

		return true;
    }
}
//------------------------------------------------------------------------------

void BanDialog::BanDeleted(BanItem * pBan) {
    if(pBanToChange == NULL || pBan != pBanToChange) {
        return;
    }

    ::MessageBox(hWndWindowItems[WINDOW_HANDLE], LanguageManager->sTexts[LAN_BAN_DELETED_ACCEPT_TO_NEW], sTitle.c_str(), MB_OK | MB_ICONEXCLAMATION);
}
//------------------------------------------------------------------------------
