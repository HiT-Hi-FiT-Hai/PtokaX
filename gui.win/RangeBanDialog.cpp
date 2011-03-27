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
#include "../core/stdinc.h"
//---------------------------------------------------------------------------
#include "RangeBanDialog.h"
//---------------------------------------------------------------------------
#include "../core/hashBanManager.h"
#include "../core/LanguageManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "RangeBansDialog.h"
//---------------------------------------------------------------------------
RangeBanDialog * pRangeBanDialog = NULL;
//---------------------------------------------------------------------------
static ATOM atomRangeBanDialog = 0;
//---------------------------------------------------------------------------

RangeBanDialog::RangeBanDialog() {
    pRangeBanDialog = this;

    memset(&hWndWindowItems, 0, (sizeof(hWndWindowItems) / sizeof(hWndWindowItems[0])) * sizeof(HWND));

    pRangeBanToChange = NULL;
}
//---------------------------------------------------------------------------

RangeBanDialog::~RangeBanDialog() {
    pRangeBanDialog = NULL;
}
//---------------------------------------------------------------------------

LRESULT CALLBACK RangeBanDialog::StaticRangeBanDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    RangeBanDialog * pRangeBanDialog = (RangeBanDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if(pRangeBanDialog == NULL) {
        return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

	return pRangeBanDialog->RangeBanDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT RangeBanDialog::RangeBanDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case BTN_ACCEPT:
                    if(OnAccept() == false) {
                        return 0;
                    }
                case BTN_DISCARD:
                    ::PostMessage(hWndWindowItems[WND_THIS], WM_CLOSE, 0, 0);
                    return 0;
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
        case WM_GETMINMAXINFO: {
            MINMAXINFO *mminfo = (MINMAXINFO*)lParam;
            mminfo->ptMinTrackSize.x = 300;
            mminfo->ptMinTrackSize.y = 278;

            return 0;
        }
        case WM_CLOSE:
            ::EnableWindow(::GetParent(hWndWindowItems[WND_THIS]), TRUE);
            break;
        case WM_NCDESTROY:
            delete this;
            return ::DefWindowProc(hWndWindowItems[WND_THIS], uMsg, wParam, lParam);
    }

	return ::DefWindowProc(hWndWindowItems[WND_THIS], uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void RangeBanDialog::DoModal(HWND hWndParent, RangeBanItem * pRangeBan/* = NULL*/) {
    pRangeBanToChange = pRangeBan;

    if(atomRangeBanDialog == 0) {
        WNDCLASSEX m_wc;
        memset(&m_wc, 0, sizeof(WNDCLASSEX));
        m_wc.cbSize = sizeof(WNDCLASSEX);
        m_wc.lpfnWndProc = ::DefWindowProc;
        m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
        m_wc.lpszClassName = "PtokaX_RangeBanDialog";
        m_wc.hInstance = g_hInstance;
        m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
        m_wc.style = CS_HREDRAW | CS_VREDRAW;

        atomRangeBanDialog = ::RegisterClassEx(&m_wc);
    }

    RECT rcParent;
    ::GetWindowRect(hWndParent, &rcParent);

    int iX = (rcParent.left + (((rcParent.right-rcParent.left))/2))-150;
    int iY = (rcParent.top + ((rcParent.bottom-rcParent.top)/2))-179;

    hWndWindowItems[WND_THIS] = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomRangeBanDialog), LanguageManager->sTexts[LAN_RANGE_BAN],
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, 300, 278,
        hWndParent, NULL, g_hInstance, NULL);

    if(hWndWindowItems[WND_THIS] == NULL) {
        return;
    }

    ::SetWindowLongPtr(hWndWindowItems[WND_THIS], GWLP_USERDATA, (LONG_PTR)this);
    ::SetWindowLongPtr(hWndWindowItems[WND_THIS], GWLP_WNDPROC, (LONG_PTR)StaticRangeBanDialogProc);

    ::GetClientRect(hWndWindowItems[WND_THIS], &rcParent);

    hWndWindowItems[GB_RANGE] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_RANGE], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        3, 1, (rcParent.right-rcParent.left)-6, 60, hWndWindowItems[WND_THIS], NULL, g_hInstance, NULL);

    hWndWindowItems[EDT_FROM_IP] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        11, 16, ((rcParent.right-rcParent.left)/2)-13, 18, hWndWindowItems[WND_THIS], NULL, g_hInstance, NULL);
    ::SendMessage(hWndWindowItems[EDT_FROM_IP], EM_SETLIMITTEXT, 15, 0);

    hWndWindowItems[EDT_TO_IP] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        ((rcParent.right-rcParent.left)/2)+3, 16, ((rcParent.right-rcParent.left)/2)-13, 18, hWndWindowItems[WND_THIS], NULL, g_hInstance, NULL);
    ::SendMessage(hWndWindowItems[EDT_TO_IP], EM_SETLIMITTEXT, 15, 0);

    hWndWindowItems[BTN_FULL_BAN] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_FULL_BAN], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        11, 39, (rcParent.right-rcParent.left)-22, 16, hWndWindowItems[WND_THIS], NULL, g_hInstance, NULL);

    hWndWindowItems[GB_REASON] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_REASON], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        3, 61, (rcParent.right-rcParent.left)-6, 41, hWndWindowItems[WND_THIS], NULL, g_hInstance, NULL);

    hWndWindowItems[EDT_REASON] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        11, 76, (rcParent.right-rcParent.left)-22, 18, hWndWindowItems[WND_THIS], NULL, g_hInstance, NULL);
    ::SendMessage(hWndWindowItems[EDT_REASON], EM_SETLIMITTEXT, 255, 0);

    hWndWindowItems[GB_BY] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_CREATED_BY], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        3, 102, (rcParent.right-rcParent.left)-6, 41, hWndWindowItems[WND_THIS], NULL, g_hInstance, NULL);

    hWndWindowItems[EDT_BY] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        11, 117, (rcParent.right-rcParent.left)-22, 18, hWndWindowItems[WND_THIS], (HMENU)EDT_BY, g_hInstance, NULL);
    ::SendMessage(hWndWindowItems[EDT_BY], EM_SETLIMITTEXT, 64, 0);

    hWndWindowItems[GB_BAN_TYPE] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, "", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        3, 143, (rcParent.right-rcParent.left)-6, 77, hWndWindowItems[WND_THIS], NULL, g_hInstance, NULL);

    hWndWindowItems[RB_PERM_BAN] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_PERMANENT], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON,
        16, 157, (rcParent.right-rcParent.left)-32, 16, hWndWindowItems[WND_THIS], (HMENU)RB_PERM_BAN, g_hInstance, NULL);
    ::SendMessage(hWndWindowItems[RB_PERM_BAN], BM_SETCHECK, BST_CHECKED, 0);

    hWndWindowItems[GB_TEMP_BAN] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, NULL, WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        8, 173, (rcParent.right-rcParent.left)-16, 42, hWndWindowItems[WND_THIS], NULL, g_hInstance, NULL);

    int iThird = ((rcParent.right-rcParent.left)-32)/3;
    hWndWindowItems[RB_TEMP_BAN] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_TEMPORARY], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON,
        16, 189, iThird-2, 16, hWndWindowItems[WND_THIS], (HMENU)RB_TEMP_BAN, g_hInstance, NULL);

    hWndWindowItems[DT_TEMP_BAN_EXPIRE_DATE] = ::CreateWindowEx(0, DATETIMEPICK_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | DTS_SHORTDATECENTURYFORMAT,
        iThird+16, 187, iThird-2, 20, hWndWindowItems[WND_THIS], NULL, g_hInstance, NULL);

    hWndWindowItems[DT_TEMP_BAN_EXPIRE_TIME] = ::CreateWindowEx(0, DATETIMEPICK_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | DTS_TIMEFORMAT | DTS_UPDOWN,
        (iThird*2)+19, 187, iThird-2, 20, hWndWindowItems[WND_THIS], NULL, g_hInstance, NULL);

    hWndWindowItems[BTN_ACCEPT] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_ACCEPT], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        2, 225, ((rcParent.right-rcParent.left)/2)-3, 23, hWndWindowItems[WND_THIS], (HMENU)BTN_ACCEPT, g_hInstance, NULL);

    hWndWindowItems[BTN_DISCARD] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_DISCARD], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        ((rcParent.right-rcParent.left)/2)+2, 225, ((rcParent.right-rcParent.left)/2)-4, 23, hWndWindowItems[WND_THIS], (HMENU)BTN_DISCARD, g_hInstance, NULL);

    for(uint8_t ui8i = 0; ui8i < (sizeof(hWndWindowItems) / sizeof(hWndWindowItems[0])); ui8i++) {
        if(hWndWindowItems[ui8i] == NULL) {
            return;
        }

        ::SendMessage(hWndWindowItems[ui8i], WM_SETFONT, (WPARAM)hfFont, MAKELPARAM(TRUE, 0));
    }

    if(pRangeBanToChange != NULL) {
        ::SetWindowText(hWndWindowItems[EDT_FROM_IP], pRangeBanToChange->sIpFrom);
        ::SetWindowText(hWndWindowItems[EDT_TO_IP], pRangeBanToChange->sIpTo);

        if(((pRangeBanToChange->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == true) {
            ::SendMessage(hWndWindowItems[BTN_FULL_BAN], BM_SETCHECK, BST_CHECKED, 0);
        }

        if(pRangeBanToChange->sReason != NULL) {
            ::SetWindowText(hWndWindowItems[EDT_REASON], pRangeBanToChange->sReason);
        }

        if(pRangeBanToChange->sBy != NULL) {
            ::SetWindowText(hWndWindowItems[EDT_BY], pRangeBanToChange->sBy);
        }

        if(((pRangeBanToChange->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
            ::SendMessage(hWndWindowItems[RB_PERM_BAN], BM_SETCHECK, BST_UNCHECKED, 0);
            ::SendMessage(hWndWindowItems[RB_TEMP_BAN], BM_SETCHECK, BST_CHECKED, 0);

            ::EnableWindow(hWndWindowItems[DT_TEMP_BAN_EXPIRE_DATE], TRUE);
            ::EnableWindow(hWndWindowItems[DT_TEMP_BAN_EXPIRE_TIME], TRUE);

            SYSTEMTIME stDateTime = { 0 };

            struct tm *tm = localtime(&pRangeBanToChange->tempbanexpire);

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

    ::ShowWindow(hWndWindowItems[WND_THIS], SW_SHOW);
}
//------------------------------------------------------------------------------

bool RangeBanDialog::OnAccept() {
    int iIpFromLen = ::GetWindowTextLength(hWndWindowItems[EDT_FROM_IP]);

    char sFromIP[16];
    ::GetWindowText(hWndWindowItems[EDT_FROM_IP], sFromIP, 16);

	if(iIpFromLen == 0) {
		::MessageBox(hWndWindowItems[WND_THIS], LanguageManager->sTexts[LAN_NO_VALID_IP_SPECIFIED], sTitle.c_str(), MB_OK | MB_ICONEXCLAMATION);
		return false;
	} else if(isIP(sFromIP, iIpFromLen) == false) {
		::MessageBox(hWndWindowItems[WND_THIS], (string(sFromIP) + " " + LanguageManager->sTexts[LAN_IS_NOT_VALID_IP_ADDRESS]).c_str(), sTitle.c_str(), MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

    int iIpToLen = ::GetWindowTextLength(hWndWindowItems[EDT_TO_IP]);

    char sToIP[16];
    ::GetWindowText(hWndWindowItems[EDT_TO_IP], sToIP, 16);

	if(iIpToLen == 0) {
		::MessageBox(hWndWindowItems[WND_THIS], LanguageManager->sTexts[LAN_NO_VALID_IP_SPECIFIED], sTitle.c_str(), MB_OK | MB_ICONEXCLAMATION);
		return false;
	} else if(isIP(sToIP, iIpToLen) == false) {
		::MessageBox(hWndWindowItems[WND_THIS], (string(sToIP) + " " + LanguageManager->sTexts[LAN_IS_NOT_VALID_IP_ADDRESS]).c_str(), sTitle.c_str(), MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

    uint32_t ui32FromIpHash = 0, ui32ToIpHash = 0;

	HashIP(sFromIP, iIpFromLen, ui32FromIpHash);
	HashIP(sToIP, iIpToLen, ui32ToIpHash);

    if(ui32ToIpHash <= ui32FromIpHash) {
		::MessageBox(hWndWindowItems[WND_THIS], LanguageManager->sTexts[LAN_NO_VALID_IP_RANGE_SPECIFIED], sTitle.c_str(), MB_OK | MB_ICONEXCLAMATION);
		return false;
    }

	time_t acc_time;
	time(&acc_time);

	time_t ban_time = 0;

    bool bTempBan = ::SendMessage(hWndWindowItems[RB_TEMP_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
    bool bPermBan = ::SendMessage(hWndWindowItems[RB_PERM_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;

	if(bTempBan == true) {
        SYSTEMTIME stDate = { 0 };
        SYSTEMTIME stTime = { 0 };

        if(::SendMessage(hWndWindowItems[DT_TEMP_BAN_EXPIRE_DATE], DTM_GETSYSTEMTIME, 0, (LPARAM)&stDate) != GDT_VALID ||
            ::SendMessage(hWndWindowItems[DT_TEMP_BAN_EXPIRE_TIME], DTM_GETSYSTEMTIME, 0, (LPARAM)&stTime) != GDT_VALID) {
            ::MessageBox(hWndWindowItems[WND_THIS], LanguageManager->sTexts[LAN_BAD_TIME_SPECIFIED], sTitle.c_str(), MB_OK | MB_ICONEXCLAMATION);

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
			::MessageBox(hWndWindowItems[WND_THIS], LanguageManager->sTexts[LAN_BAD_TIME_SPECIFIED_BAN_EXPIRED], sTitle.c_str(), MB_OK | MB_ICONEXCLAMATION);

			return false;
        }
	}

	if(pRangeBanToChange == NULL) {
		RangeBanItem * pRangeBan = new RangeBanItem();
		if(pRangeBan == NULL) {
			string sDbgstr = "[BUF] Cannot allocate RangeBan in RangeBanDialog::OnAccept! "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
			AppendSpecialLog(sDbgstr);
			return false;
		}

		if(bTempBan == true) {
			pRangeBan->ui8Bits |= hashBanMan::TEMP;
			pRangeBan->tempbanexpire = ban_time;
		} else {
			pRangeBan->ui8Bits |= hashBanMan::PERM;
		}

		strcpy(pRangeBan->sIpFrom, sFromIP);
		pRangeBan->ui32FromIpHash = ui32FromIpHash;

		strcpy(pRangeBan->sIpTo, sToIP);
		pRangeBan->ui32ToIpHash = ui32ToIpHash;

        if(::SendMessage(hWndWindowItems[BTN_FULL_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED) {
			pRangeBan->ui8Bits |= hashBanMan::FULL;
        }

		RangeBanItem *nxtBan = hashBanManager->RangeBanListS;

		if(bPermBan == true) {
			// PPK ... don't add range ban if is already here same perm (full) range ban
			while(nxtBan != NULL) {
				RangeBanItem *curBan = nxtBan;
				nxtBan = curBan->next;

				if(curBan->ui32FromIpHash != pRangeBan->ui32FromIpHash || curBan->ui32ToIpHash != pRangeBan->ui32ToIpHash) {
					continue;
				}

				if(((curBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) {
					if(((curBan->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == false || ((pRangeBan->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == true) {
						hashBanManager->RemRange(curBan);
						delete curBan;

						continue;
					}
				}

				if(((curBan->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == false && ((pRangeBan->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == true) {
					hashBanManager->RemRange(curBan);
					delete curBan;

					continue;
				}

				delete pRangeBan;

				::MessageBox(hWndWindowItems[WND_THIS], LanguageManager->sTexts[LAN_SIMILAR_BAN_EXIST], sTitle.c_str(), MB_OK | MB_ICONEXCLAMATION);
				return false;
			}
		} else { // temp ban
			// PPK ... don't add range ban if is already here same perm (full) range ban or longer temp ban for same range
			while(nxtBan != NULL) {
				RangeBanItem *curBan = nxtBan;
				nxtBan = curBan->next;

				if(curBan->ui32FromIpHash != pRangeBan->ui32FromIpHash || curBan->ui32ToIpHash != pRangeBan->ui32ToIpHash) {
					continue;
				}

				if(((curBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true && curBan->tempbanexpire < pRangeBan->tempbanexpire) {
					if(((curBan->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == false || ((pRangeBan->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == true) {
						hashBanManager->RemRange(curBan);
						delete curBan;
					}

					continue;
				}

				if(((curBan->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == false && ((pRangeBan->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == true) {
					continue;
				}

				delete pRangeBan;

				::MessageBox(hWndWindowItems[WND_THIS], LanguageManager->sTexts[LAN_SIMILAR_BAN_EXIST], sTitle.c_str(), MB_OK | MB_ICONEXCLAMATION);
				return false;
			}
		}

        int iReasonLen = ::GetWindowTextLength(hWndWindowItems[EDT_REASON]);

		if(iReasonLen != 0) {
            pRangeBan->sReason = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iReasonLen+1);
            if(pRangeBan->sReason == NULL) {
    			string sDbgstr = "[BUF] Cannot allocate "+string(iReasonLen+1)+
    				" bytes of memory for sReason in RangeBanDialog::OnAccept! "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
                AppendSpecialLog(sDbgstr);
                delete pRangeBan;

                return false;
            }

            ::GetWindowText(hWndWindowItems[EDT_REASON], pRangeBan->sReason, iReasonLen+1);
        }

        int iByLen = ::GetWindowTextLength(hWndWindowItems[EDT_BY]);

        if(iByLen != 0) {
            pRangeBan->sBy = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iByLen+1);
            if(pRangeBan->sBy == NULL) {
                string sDbgstr = "[BUF] Cannot allocate "+string(iByLen+1)+
					" bytes of memory for sBy in RangeBanDialog::OnAccept! "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
                AppendSpecialLog(sDbgstr);
                delete pRangeBan;

    			return false;
            }

            ::GetWindowText(hWndWindowItems[EDT_BY], pRangeBan->sBy, iByLen+1);
		}

		hashBanManager->AddRange(pRangeBan);

		return true;
	} else {
		if(bTempBan == true) {
			pRangeBanToChange->ui8Bits &= ~hashBanMan::PERM;

			pRangeBanToChange->ui8Bits |= hashBanMan::TEMP;
			pRangeBanToChange->tempbanexpire = ban_time;
		} else {
			pRangeBanToChange->ui8Bits &= ~hashBanMan::TEMP;

			pRangeBanToChange->ui8Bits |= hashBanMan::PERM;
		}

		strcpy(pRangeBanToChange->sIpFrom, sFromIP);
		pRangeBanToChange->ui32FromIpHash = ui32FromIpHash;

		strcpy(pRangeBanToChange->sIpTo, sToIP);
		pRangeBanToChange->ui32ToIpHash = ui32ToIpHash;

		if(::SendMessage(hWndWindowItems[BTN_FULL_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED) {
			pRangeBanToChange->ui8Bits |= hashBanMan::FULL;
		} else {
			pRangeBanToChange->ui8Bits &= ~hashBanMan::FULL;
		}

        int iReasonLen = ::GetWindowTextLength(hWndWindowItems[EDT_REASON]);

        char * sReason = NULL;
		if(iReasonLen != 0) {
            sReason = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iReasonLen+1);
            if(sReason == NULL) {
    			string sDbgstr = "[BUF] Cannot allocate "+string(iReasonLen+1)+
    				" bytes of memory for sReason in RangeBanDialog::OnAccept! "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
                AppendSpecialLog(sDbgstr);

                return false;
            }

            ::GetWindowText(hWndWindowItems[EDT_REASON], sReason, iReasonLen+1);
        }

		if(iReasonLen != 0) {
			if(pRangeBanToChange->sReason == NULL || strcmp(pRangeBanToChange->sReason, sReason) != NULL) {
				if(pRangeBanToChange->sReason != NULL) {
					if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pRangeBanToChange->sReason) == 0) {
						string sDbgstr = "[BUF] Cannot deallocate sReason in RangeBanDialog::OnAccept! "+string((uint32_t)GetLastError())+" "+
							string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
						AppendSpecialLog(sDbgstr);
					}
					pRangeBanToChange->sReason = NULL;
				}

				pRangeBanToChange->sReason = sReason;
			}
		} else if(pRangeBanToChange->sReason != NULL) {
			if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pRangeBanToChange->sReason) == 0) {
				string sDbgstr = "[BUF] Cannot deallocate sReason in BanDialog::OnAccept! "+string((uint32_t)GetLastError())+" "+
					string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
				AppendSpecialLog(sDbgstr);
			}

			pRangeBanToChange->sReason = NULL;
        }

        if(sReason != NULL && (pRangeBanToChange->sReason != sReason)) {
			if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sReason) == 0) {
				string sDbgstr = "[BUF] Cannot deallocate sReason in RangeBanDialog::OnAccept! "+string((uint32_t)GetLastError())+" "+
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
					" bytes of memory for sBy in RangeBanDialog::OnAccept! "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
                AppendSpecialLog(sDbgstr);

    			return false;
            }

            ::GetWindowText(hWndWindowItems[EDT_BY], sBy, iByLen+1);
		}

		if(iByLen != 0) {
			if(pRangeBanToChange->sBy == NULL || strcmp(pRangeBanToChange->sBy, sBy) != NULL) {
				if(pRangeBanToChange->sBy != NULL) {
					if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pRangeBanToChange->sBy) == 0) {
						string sDbgstr = "[BUF] Cannot deallocate sBy in RangeBanDialog::OnAccept! "+string((uint32_t)GetLastError())+" "+
							string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
						AppendSpecialLog(sDbgstr);
					}
					pRangeBanToChange->sBy = NULL;
				}

				pRangeBanToChange->sBy = sBy;
			}
		} else if(pRangeBanToChange->sBy != NULL) {
			if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pRangeBanToChange->sBy) == 0) {
				string sDbgstr = "[BUF] Cannot deallocate sBy in RangeBanDialog::OnAccept! "+string((uint32_t)GetLastError())+" "+
					string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
				AppendSpecialLog(sDbgstr);
			}
			pRangeBanToChange->sBy = NULL;
        }

        if(sBy != NULL && (pRangeBanToChange->sBy != sBy)) {
			if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sBy) == 0) {
				string sDbgstr = "[BUF] Cannot deallocate sBy in RangeBanDialog::OnAccept! "+string((uint32_t)GetLastError())+" "+
					string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
				AppendSpecialLog(sDbgstr);
			}
        }

        if(pRangeBansDialog != NULL) {
            pRangeBansDialog->RemoveRangeBan(pRangeBanToChange);
            pRangeBansDialog->AddRangeBan(pRangeBanToChange);
        }

		return true;
    }
}
//------------------------------------------------------------------------------

void RangeBanDialog::RangeBanDeleted(RangeBanItem * pRangeBan) {
    if(pRangeBanToChange == NULL || pRangeBan != pRangeBanToChange) {
        return;
    }

    ::MessageBox(hWndWindowItems[WND_THIS], LanguageManager->sTexts[LAN_RANGE_BAN_DELETED_ACCEPT_TO_NEW], sTitle.c_str(), MB_OK | MB_ICONEXCLAMATION);
}
//------------------------------------------------------------------------------
