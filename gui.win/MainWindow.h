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

//------------------------------------------------------------------------------
#ifndef MainWindowH
#define MainWindowH
//------------------------------------------------------------------------------

class MainWindow {
public:
    HWND m_hWnd;
    HWND btnStartStop, btnRedirectAll, btnMassMessage;
    HWND gbStats;
    HWND lblStatus, lblStatusValue, lblJoins, lblJoinsValue, lblParts, lblPartsValue, lblActive, lblActiveValue, lblOnline, lblOnlineValue;
    HWND lblPeak, lblPeakValue, lblReceived, lblReceivedValue, lblSent, lblSentValue;

    MainWindow();
    ~MainWindow();

    static LRESULT CALLBACK StaticMainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND CreateEx();

    void UpdateSysTray();
    void UpdateStats();
    void UpdateTitleBar();
    void UpdateLanguage();
private:
    UINT uiTaskBarCreated;
    uint64_t ui64LastTrayMouseMove;

    LRESULT MainWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

    void OnMassMessage();
    void OnRedirectAll();
};
//------------------------------------------------------------------------------
extern MainWindow *pMainWindow;
//------------------------------------------------------------------------------

#endif
