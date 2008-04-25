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
#ifndef utilityH
#define utilityH
//---------------------------------------------------------------------------
struct BanItem;
struct RangeBanItem;
//---------------------------------------------------------------------------

void Cout(const string & msg);
//---------------------------------------------------------------------------

char * Lock2Key(char * cLock);

const char * ErrnoStr(const int &iError);

char * formatBytes(int64_t iBytes);
char * formatBytesPerSecond(int64_t iBytes);
char * formatTime(long rest);
char * formatSecTime(long rest);

char * stristr(const char *str1, const char *str2);

bool isIP(char * IP, const uint32_t ui32Len);
bool GetIpParts(char * sIP, const uint32_t ui32Len, unsigned int &a, unsigned int &b, unsigned int &c, unsigned int &d);

uint32_t HashNick(const char * sNick, const unsigned int &sNickLen);

bool HashIP(char * sIP, const uint32_t ui32Len, uint32_t &ui32Hash);

char * GenerateBanMessage(BanItem * Ban, int &iMsgLen, const time_t &acc_time);
char * GenerateRangeBanMessage(RangeBanItem * RangeBan, int &iMsgLen, const time_t &acc_time);

bool GenerateTempBanTime(const char &cMultiplyer, const int &iTime, time_t &acc_time, time_t &ban_time);

bool HaveOnlyNumbers(char *sData, const uint16_t &ui16Len);
int GetWlcmMsg(char * sWlcmMsg);

inline int Allign256(int n) { return ((n+1) & 0xFFFFFF00) + 0x100; }
inline int Allign512(int n) { return ((n+1) & 0xFFFFFE00) + 0x200; }
inline int Allign1024(int n) { return ((n+1) & 0xFFFFFC00) + 0x400; }

bool CheckSprintf(int iRetVal, int iMax, const char * sMsg); // CheckSprintf(imsgLen, 64, "UdpDebug::New");
bool CheckSprintf1(int iRetVal, int iLenVal, int iMax, const char * sMsg); // CheckSprintf1(iret, imsgLen, 64, "UdpDebug::New");

void AppendLog(const string & sData, const bool &bScript = false);
void AppendSpecialLog(const string & sData);

void Memo(const string & sMessage);

bool FileExist(char * sPath);
bool DirExist(char * sPath);
//---------------------------------------------------------------------------
extern string PATH, SCRIPT_PATH, sTitle;
extern bool bCmdAutoStart, bCmdNoAutoStart, bCmdNoTray, bCmdNoKeyCheck;
//---------------------------------------------------------------------------

#endif
