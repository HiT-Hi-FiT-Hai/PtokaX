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

const char * ErrnoStr(const uint32_t &iError);

char * formatBytes(int64_t iBytes);
char * formatBytesPerSecond(int64_t iBytes);
char * formatTime(uint64_t rest);
char * formatSecTime(uint64_t rest);

char * stristr(const char *str1, const char *str2);

bool isIP(char * IP, const size_t ui32Len);
bool GetIpParts(char * sIP, const size_t ui32Len, uint32_t &a, uint32_t &b, uint32_t &c, uint32_t &d);

uint32_t HashNick(const char * sNick, const size_t &sNickLen);

bool HashIP(char * sIP, const size_t ui32Len, uint32_t &ui32Hash);

char * GenerateBanMessage(BanItem * Ban, int32_t &iMsgLen, const time_t &acc_time);
char * GenerateRangeBanMessage(RangeBanItem * RangeBan, int32_t &iMsgLen, const time_t &acc_time);

bool GenerateTempBanTime(const char &cMultiplyer, const uint32_t &iTime, time_t &acc_time, time_t &ban_time);

bool HaveOnlyNumbers(char *sData, const uint16_t &ui16Len);
int GetWlcmMsg(char * sWlcmMsg);

inline size_t Allign256(size_t n) { return ((n+1) & 0xFFFFFF00) + 0x100; }
inline size_t Allign512(size_t n) { return ((n+1) & 0xFFFFFE00) + 0x200; }
inline size_t Allign1024(size_t n) { return ((n+1) & 0xFFFFFC00) + 0x400; }

bool CheckSprintf(int iRetVal, const size_t &iMax, const char * sMsg); // CheckSprintf(imsgLen, 64, "UdpDebug::New");
bool CheckSprintf1(int iRetVal, int iLenVal, const size_t &iMax, const char * sMsg); // CheckSprintf1(iret, imsgLen, 64, "UdpDebug::New");

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
