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
#include "stdinc.h"
//---------------------------------------------------------------------------
#include "utility.h"
//---------------------------------------------------------------------------
#include "hashBanManager.h"
#include "LanguageManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
//---------------------------------------------------------------------------
	#ifndef _SERVICE
		#ifndef _MSC_VER
			#include "TUsersChatForm.h"
		#else
			#include "../gui.win/GuiUtil.h"
            #include "../gui.win/MainWindowPageUsersChat.h"
		#endif
	#endif
//---------------------------------------------------------------------------
	#ifndef _MSC_VER
		#pragma package(smart_init)
	#endif
#endif
//---------------------------------------------------------------------------
static const int MAX_PAT_SIZE = 64;
static const int MAX_ALPHABET_SIZE = 255;
//---------------------------------------------------------------------------
string PATH = "", SCRIPT_PATH = "", sTitle = "";
bool bCmdAutoStart = false, bCmdNoAutoStart = false, bCmdNoTray = false, bCmdNoKeyCheck = false;
#ifdef _WIN32
	HANDLE hConsole = NULL, hPtokaXHeap = NULL, hRecvHeap = NULL, hSendHeap = NULL;
	string PATH_LUA = "", sOs = "";
	bool b2K = false;
#endif
//---------------------------------------------------------------------------

void Cout(const string & msg) {
#ifdef _WIN32
	if(hConsole != NULL) {
		WriteConsole(hConsole, (msg+"\n").c_str(), (DWORD)msg.size()+1, 0, 0);
        return;
    }
#else
	return;
#endif
}
//---------------------------------------------------------------------------

#ifdef _WIN32
	// Boyer-Moore string matching algo.
	// returns :
	// offset in bytes or -1 if no match
	int BMFind(char *text, int N, char *pat, int M) {
	   int i, j, DD[MAX_PAT_SIZE], D[MAX_ALPHABET_SIZE];
	
	   /* Predzpracovani */
	   preDD(pat, M, DD);
	   preD(pat, M, D);
	
	   /* Vyhledavani */
	   /* Searching */
	   i = 0;
	   while (i <= N-M) {
	      for(j = M-1; j >= 0  &&  pat[j] == text[j+i]; --j);
	      if (j < 0) {
	         return i;
	         //i += DD[0];
	      }
	      else
	         i += DD[j] > (D[text[j + i]] - M + 1 + j) ? DD[j] : (D[text[j + i]] - M + 1 + j);
	   }
	   return -1;
	}
//---------------------------------------------------------------------------
	
	void preD(char *pat, int M, int D[]) {
	   int i;
	   for(i = 0; i < MAX_ALPHABET_SIZE; ++i)
	      D[i] = M;
	   for(i = 0; i < M - 1; ++i)
	      D[pat[i]] = M - i - 1;
	}
//---------------------------------------------------------------------------
	
	/* Funkce ulozi do suff[i] delku nejdelsiho podretezce,
	 * ktery konci na pozici pat[i]
	 * a soucasne je priponou pat
	 */
	void suffixes(char *pat, int M, int *suff) {
	   int f = 0, g, i;
	
	   suff[M - 1] = M;
	   g = M - 1;
	   for(i = M - 2; i >= 0; --i) {
	      if (i > g && suff[i + M - 1 - f] < i - g)
	         suff[i] = suff[i + M - 1 - f];
	      else {
	         if (i < g)
	            g = i;
	         f = i;
	         while (g >= 0 && pat[g] == pat[g + M - 1 - f])
	            --g;
	         suff[i] = f - g;
	      }
	   }
	}
//---------------------------------------------------------------------------
	
	void preDD(char *pat, int M, int DD[]) {
	   int i, j, suff[MAX_PAT_SIZE];
	
	   suffixes(pat, M, suff);
	
	   for(i = 0; i < M; ++i)
	      DD[i] = M;
	   j = 0;
	   for(i = M - 1; i >= -1; --i)
	      if (suff[i] == i+1  ||  i == -1)
	         for( ; j < M-1-i; ++j)
	            if (DD[j] == M)
	               DD[j] = M-1-i;
	   for(i = 0; i <= M-2; ++i)
	      DD[M - 1 - suff[i]] = M-1-i;
	}
#endif
//---------------------------------------------------------------------------

char * Lock2Key(char * sLock) {
    static char cKey[128];
    cKey[0] = '\0';

    // $Lock EXTENDEDPROTOCOL33MTL6@h5AIad^P2UoPv?fZU]ivM6 Pk=PtokaX|

    sLock = sLock+6; // set begin after $Lock_
    sLock[46] = '\0'; // set end before Pk
    
    uint8_t v;
    
	// first make the crypting stuff
	for(uint8_t i = 0; i < 46; i++) {
        if(i == 0) {
            v = (uint8_t)(sLock[0] ^ sLock[45] ^ sLock[44] ^ 5);
        } else {
            v = sLock[i] ^ sLock[i-1];
        }
        
        // swap nibbles (0xF0 = 11110000, 0x0F = 00001111)

        v = (uint8_t)(((v << 4) & 0xF0) | ((v >> 4) & 0x0F));
        
    	switch(v) {
            case   0:
                strcat(cKey, "/%DCN000%/");
                break;
    		case   5:
                strcat(cKey, "/%DCN005%/");
                break;
    		case  36:
                strcat(cKey, "/%DCN036%/");
                break;
			case  96:
                strcat(cKey, "/%DCN096%/");
                break;
    		case 124:
                strcat(cKey, "/%DCN124%/");
                break;
    		case 126:
                strcat(cKey, "/%DCN126%/");
                break;
    		default:
                strncat(cKey, (char *)&v, 1);
                break;
    	}
    }
    return cKey;
}
//---------------------------------------------------------------------------

#ifdef _WIN32
	char * WSErrorStr(const uint32_t &iError) {
		static char errStrings[][64] = {
			{"0"},{"1"},{"2"},{"3"},
			{"WSAEINTR"},
			{"5"},{"6"},{"7"},{"8"},
			{"WSAEBADF"},
			{"10"},{"11"},{"12"},
			{"WSAEACCES"},
			{"WSAEFAULT"},
			{"15"},{"16"},{"17"},{"18"},{"19"},{"20"},{"21"},
			{"WSAEINVAL"},
			{"23"},
			{"WSAEMFILE"},
			{"25"},{"26"},{"27"},{"28"},{"29"},{"30"},{"31"},{"32"},{"33"},{"34"},
	        {"WSAEWOULDBLOCK"},
	        {"WSAEINPROGRESS"},   // This error is returned if any Windows Sockets API function is called while a blocking function is in progress.
	        {"WSAEALREADY"},
	        {"WSAENOTSOCK"},
	        {"WSAEDESTADDRREQ"},
	        {"WSAEMSGSIZE"},
	        {"WSAEPROTOTYPE"},
	        {"WSAENOPROTOOPT"},
	        {"WSAEPROTONOSUPPORT"},
	        {"WSAESOCKTNOSUPPORT"},
	        {"WSAEOPNOTSUPP"},
	        {"WSAEPFNOSUPPORT"},
	        {"WSAEAFNOSUPPORT"},
	        {"WSAEADDRINUSE"},
	        {"WSAEADDRNOTAVAIL"},
	        {"WSAENETDOWN"}, // This error may be reported at any time if the Windows Sockets implementation detects an underlying failure.
	        {"WSAENETUNREACH"},
	        {"WSAENETRESET"},
	        {"WSAECONNABORTED"},
	        {"WSAECONNRESET"},
	        {"WSAENOBUFS"},
	        {"WSAEISCONN"},
	        {"WSAENOTCONN"},
	        {"WSAESHUTDOWN"},
	        {"WSAETOOMANYREFS"},
	        {"WSAETIMEDOUT"},
	        {"WSAECONNREFUSED"},
	        {"WSAELOOP"},
	        {"WSAENAMETOOLONG"},
	        {"WSAEHOSTDOWN"},
	        {"WSAEHOSTUNREACH"}, // 10065
	        {"WSASYSNOTREADY"}, // 10091
	        {"WSAVERNOTSUPPORTED"}, // 10092
	        {"WSANOTINITIALISED"}, // 10093
	        {"WSAHOST_NOT_FOUND"}, // 11001
	        {"WSATRY_AGAIN"}, // 11002
	        {"WSANO_RECOVERY"}, // 11003
	        {"WSANO_DATA"} // 11004
		};
	
		switch(iError) {
			case 10091 :
		    	return errStrings[66]; // WSASYSNOTREADY
			case 10092 :
		    	return errStrings[67]; // WSAVERNOTSUPPORTED
			case 10093 :
		    	return errStrings[68]; // WSANOTINITIALISED
			case 11001 :
		    	return errStrings[69]; // WSAHOST
			case 11002 :
		    	return errStrings[70]; // WSATRY
			case 11003 :
		    	return errStrings[71]; // WSANO
			case 11004 :
		    	return errStrings[72]; // WSANO
		    default :
		    	return errStrings[iError-10000];
	    }
	}
#else
	const char * ErrnoStr(const uint32_t &iError) {
		static const char *errStrings[] = {
	        "UNDEFINED", 
	        "EADDRINUSE", 
	        "ECONNRESET", 
	        "ETIMEDOUT", 
	        "ECONNREFUSED", 
	        "EHOSTUNREACH", 
		};
	
		switch(iError) {
			case 98:
		    	return errStrings[1];
			case 104:
		    	return errStrings[2];
			case 110:
		    	return errStrings[3];
			case 111:
		    	return errStrings[4];
			case 113:
		    	return errStrings[5];
		    default :
		    	return errStrings[0];
	    }
	}
#endif
//---------------------------------------------------------------------------

char * formatBytes(int64_t iBytes) {
    static const char *unit[] = {"B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB", " ", " ", " ", " ", " ", " ", " "};
    static char sBytes[128];

   	if(iBytes < 1024) {
        int iLen = sprintf(sBytes, "%d %s", (int32_t)(iBytes&0xffffffff), unit[0]);
        if(CheckSprintf(iLen, 128, "formatBytes1") == false) {
            sBytes[0] = '\0';
        }
   	} else {
       	long double ldBytes = (long double)iBytes;
        uint8_t iter = 0;
       	for(; ldBytes > 1024; iter++)
           	ldBytes /= 1024;
           	
   	    int iLen = sprintf(sBytes, "%0.2Lf %s", ldBytes, unit[iter]);
   	    if(CheckSprintf(iLen, 128, "formatBytes2") == false) {
            sBytes[0] = '\0';
        }
    }

    return sBytes;
}
//---------------------------------------------------------------------------

char * formatBytesPerSecond(int64_t iBytes) {
    static const char *secondunit[] = {"B/s", "kB/s", "MB/s", "GB/s", "TB/s", "PB/s", "EB/s", "ZB/s", "YB/s", " ", " ", " ", " ", " ", " ", " "};
    static char sBytes[128];

   	if(iBytes < 1024) {
        int iLen = sprintf(sBytes, "%d %s", (int32_t)(iBytes&0xffffffff), secondunit[0]);
        if(CheckSprintf(iLen, 128, "formatBytesPerSecond1") == false) {
            sBytes[0] = '\0';
        }
   	} else {
       	long double ldBytes = (long double)iBytes;
        uint8_t iter = 0;
       	for(; ldBytes > 1024; iter++)
           	ldBytes /= 1024;
           	
   	    int iLen = sprintf(sBytes, "%0.2Lf %s", ldBytes, secondunit[iter]);
   	    if(CheckSprintf(iLen, 128, "formatBytesPerSecond2") == false) {
            sBytes[0] = '\0';
        }
    }
	
    return sBytes;
}
//---------------------------------------------------------------------------

char * formatTime(uint64_t rest) {
    static char time[256];
    time[0] = '\0';

	char buf[128];
	uint8_t i = 0;
    uint64_t n = rest / 525600;
	rest -= n*525600;

	if(n != 0) {
#ifdef _WIN32
		int iLen = sprintf(buf, "%I64d %s", n, n > 1 ? LanguageManager->sTexts[LAN_YEARS_LWR] : LanguageManager->sTexts[LAN_YEAR_LWR]);
#else
		int iLen = sprintf(buf, "%" PRIu64 " %s", n, n > 1 ? LanguageManager->sTexts[LAN_YEARS_LWR] : LanguageManager->sTexts[LAN_YEAR_LWR]);
#endif
		if(CheckSprintf(iLen, 128, "formatTime1") == false) {
            time[0] = '\0';
            return time;
        }
		strcat(time, buf);
		i++;
	}

    n = rest / 43200;
	rest -= n*43200;

	if(n != 0) {       
#ifdef _WIN32
		int iLen = sprintf(buf, "%s%I64d %s", i > 0 ? " " : "", n, n > 1 ? LanguageManager->sTexts[LAN_MONTHS_LWR] : LanguageManager->sTexts[LAN_MONTH_LWR]);
#else
		int iLen = sprintf(buf, "%s%" PRIu64 " %s", i > 0 ? " " : "", n, n > 1 ? LanguageManager->sTexts[LAN_MONTHS_LWR] : LanguageManager->sTexts[LAN_MONTH_LWR]);
#endif
		if(CheckSprintf(iLen, 128, "formatTime3") == false) {
            time[0] = '\0';
            return time;
        }

		strcat(time, buf);
		i++;
	}

	n = rest / 1440;
	rest -= n*1440;

	if(n != 0) {       
#ifdef _WIN32
		int iLen = sprintf(buf, "%s%I64d %s", i > 0 ? " " : "", n, n > 1 ? LanguageManager->sTexts[LAN_DAYS_LWR] : LanguageManager->sTexts[LAN_DAY_LWR]); 
#else
		int iLen = sprintf(buf, "%s%" PRIu64 " %s", i > 0 ? " " : "", n, n > 1 ? LanguageManager->sTexts[LAN_DAYS_LWR] : LanguageManager->sTexts[LAN_DAY_LWR]); 
#endif
		if(CheckSprintf(iLen, 128, "formatTime5") == false) {
            time[0] = '\0';
            return time;
        }
		
		strcat(time, buf);
		i++;
	}

    n = rest / 60;
	rest -= n*60;

	if(n != 0) {	
#ifdef _WIN32
		int iLen = sprintf(buf, "%s%I64d %s", i > 0 ? " " : "", n, n > 1 ? LanguageManager->sTexts[LAN_HOURS_LWR] : LanguageManager->sTexts[LAN_HOUR_LWR]);
#else
		int iLen = sprintf(buf, "%s%" PRIu64 " %s", i > 0 ? " " : "", n, n > 1 ? LanguageManager->sTexts[LAN_HOURS_LWR] : LanguageManager->sTexts[LAN_HOUR_LWR]);
#endif
		if(CheckSprintf(iLen, 128, "formatTime7") == false) {
            time[0] = '\0';
            return time;
        }

		strcat(time, buf);
		i++;
	}

	if(rest != 0) {
#ifdef _WIN32
		int iLen = sprintf(buf, "%s%I64d %s", i > 0 ? " " : "", rest, LanguageManager->sTexts[LAN_MIN_LWR]);
#else
		int iLen = sprintf(buf, "%s%" PRIu64 " %s", i > 0 ? " " : "", rest, LanguageManager->sTexts[LAN_MIN_LWR]);
#endif
		if(CheckSprintf(iLen, 128, "formatTime9") == false) {
            time[0] = '\0';
            return time;
        }
		strcat(time, buf);
	}

	return time;
}
//---------------------------------------------------------------------------

char * formatSecTime(uint64_t rest) {
    static char time[256];
    time[0] = '\0';

	char buf[128];
	uint8_t i = 0;
    uint64_t n = rest / 31536000;
	rest -= n*31536000;

	if(n != 0) {
#ifdef _WIN32
		int iLen = sprintf(buf, "%I64d %s", n, n > 1 ? LanguageManager->sTexts[LAN_YEARS_LWR] : LanguageManager->sTexts[LAN_YEAR_LWR]);
#else
		int iLen = sprintf(buf, "%" PRIu64 " %s", n, n > 1 ? LanguageManager->sTexts[LAN_YEARS_LWR] : LanguageManager->sTexts[LAN_YEAR_LWR]);
#endif
		if(CheckSprintf(iLen, 128, "formatSecTime1") == false) {
            time[0] = '\0';
            return time;
        }

		strcat(time, buf);
		i++;
	}

    n = rest / 2592000;
	rest -= n*2592000;

	if(n != 0) {
#ifdef _WIN32
		int iLen = sprintf(buf, "%s%I64d %s", i > 0 ? " " : "", n, n > 1 ? LanguageManager->sTexts[LAN_MONTHS_LWR] : LanguageManager->sTexts[LAN_MONTH_LWR]);
#else
		int iLen = sprintf(buf, "%s%" PRIu64 " %s", i > 0 ? " " : "", n, n > 1 ? LanguageManager->sTexts[LAN_MONTHS_LWR] : LanguageManager->sTexts[LAN_MONTH_LWR]);
#endif
		if(CheckSprintf(iLen, 128, "formatSecTime3") == false) {
            time[0] = '\0';
            return time;
        }
		
		strcat(time, buf);
		i++;
	}

	n = rest / 86400;
	rest -= n*86400;

	if(n != 0) {
#ifdef _WIN32
		int iLen = sprintf(buf, "%s%I64d %s", i > 0 ? " " : "", n, n > 1 ? LanguageManager->sTexts[LAN_DAYS_LWR] : LanguageManager->sTexts[LAN_DAY_LWR]);
#else
		int iLen = sprintf(buf, "%s%" PRIu64 " %s", i > 0 ? " " : "", n, n > 1 ? LanguageManager->sTexts[LAN_DAYS_LWR] : LanguageManager->sTexts[LAN_DAY_LWR]);
#endif
		if(CheckSprintf(iLen, 128, "formatSecTime5") == false) {
            time[0] = '\0';
            return time;
        }

		strcat(time, buf);
		i++;
	}

    n = rest / 3600;
	rest -= n*3600;

	if(n != 0) {
#ifdef _WIN32
		int iLen = sprintf(buf, "%s%I64d %s", i > 0 ? " " : "", n, n > 1 ? LanguageManager->sTexts[LAN_HOURS_LWR] : LanguageManager->sTexts[LAN_HOUR_LWR]);
#else
		int iLen = sprintf(buf, "%s%" PRIu64 " %s", i > 0 ? " " : "", n, n > 1 ? LanguageManager->sTexts[LAN_HOURS_LWR] : LanguageManager->sTexts[LAN_HOUR_LWR]);
#endif
		if(CheckSprintf(iLen, 128, "formatSecTime7") == false) {
            time[0] = '\0';
            return time;
        }
		
		strcat(time, buf);
		i++;
	}

	n = rest / 60;
	rest -= n*60;

	if(n != 0) {
#ifdef _WIN32
		int iLen = sprintf(buf, "%s%I64d %s", i > 0 ? " " : "", n, LanguageManager->sTexts[LAN_MIN_LWR]);
#else
		int iLen = sprintf(buf, "%s%" PRIu64 " %s", i > 0 ? " " : "", n, LanguageManager->sTexts[LAN_MIN_LWR]);
#endif
		if(CheckSprintf(iLen, 128, "formatSecTime9") == false) {
            time[0] = '\0';
            return time;
        }

		strcat(time, buf);
		i++;
	}

	if(rest != 0) {
#ifdef _WIN32
    	int iLen = sprintf(buf, "%s%I64d %s", i > 0 ? " " : "", rest, LanguageManager->sTexts[LAN_SEC_LWR]);
#else
		int iLen = sprintf(buf, "%s%" PRIu64 " %s", i > 0 ? " " : "", rest, LanguageManager->sTexts[LAN_SEC_LWR]);
#endif
		if(CheckSprintf(iLen, 128, "formatSecTime10") == false) {
            time[0] = '\0';
            return time;
        }

		strcat(time, buf);
	}
	return time;
}
//---------------------------------------------------------------------------

char* stristr(const char *str1, const char *str2) {
	char *s1, *s2;
	char *cp = (char *)str1;
	if(*str2 == 0)
		return (char *)str1;
	while(*cp != 0) {
		s1 = cp;
		s2 = (char *) str2;
		while(*s1 != 0 && *s2 != 0 && ((*s1-*s2) == 0 ||
			(*s1-tolower(*s2)) == 0 || (*s1-toupper(*s2)) == 0))
				s1++, s2++;
		if(*s2 == 0) {
			return cp;
		}
		cp++;
	}
	return NULL;
}
//---------------------------------------------------------------------------

char* stristr2(const char *str1, const char *str2) {
	char *s1, *s2;
	char *cp = (char *)str1;
	if(*str2 == 0)
		return (char *)str1;
	while(*cp != 0) {
		s1 = cp;
		s2 = (char *) str2;
		while(*s1 != 0 && *s2 != 0 && ((*s1-*s2) == 0 ||
			(tolower(*s1)-*s2) == 0))
				s1++, s2++;
		if(*s2 == 0) {
			return cp;
		}
		cp++;
	}
	return NULL;
}
//---------------------------------------------------------------------------

// check IP string.
// false - no ip
// true - valid ip
bool isIP(char * IP, const size_t ui32Len) {
	uint32_t a, b, c, d;
	if(GetIpParts(IP, ui32Len, a, b, c, d) == true) {
        return true;
    } else {
	   return false;
    }
}
//---------------------------------------------------------------------------

bool GetIpParts(char * sIP, const size_t ui32Len, uint32_t &a, uint32_t &b, uint32_t &c, uint32_t &d) {
    if(ui32Len < 7 || ui32Len > 15)
        return false;

    uint8_t iDots = 0, iChars = 0, iActualChar;
    uint16_t iFirst = 0, iSecond = 0, iThird = 0;

    for(uint32_t i = 0; i < ui32Len; i++) {
        iChars++;
        switch(sIP[i]) {
            case '0':
                iActualChar = 0;
                break;
            case '1':
                iActualChar = 1;
                break;
            case '2':
                iActualChar = 2;
                break;
            case '3':
                iActualChar = 3;
                break;
            case '4':
                iActualChar = 4;
                break;
            case '5':
                iActualChar = 5;
                break;
            case '6':
                iActualChar = 6;
                break;
            case '7':
                iActualChar = 7;
                break;
            case '8':
                iActualChar = 8;
                break;
            case '9':
                iActualChar = 9;
                break;
            case '.':
                if(iDots == 3) {
                    return false;
                }

                iDots++;

                switch(iDots) {
                    case 1:
                        switch(iChars) {
                            case 2:
                                a = iFirst;
                                break;
                            case 3:
                                a = (iFirst*10)+(iSecond);
                                break;
                            case 4:
                                a = (iFirst*100)+(iSecond*10)+(iThird);
                                break;
                            default:
                                return false;
                        }
                        break;
                    case 2:
                        switch(iChars) {
                            case 2:
                                b = iFirst;
                                break;
                            case 3:
                                b = (iFirst*10)+(iSecond);
                                break;
                            case 4:
                                b = (iFirst*100)+(iSecond*10)+(iThird);
                                break;
                            default:
                                return false;
                        }
                        break;
                    case 3:
                        switch(iChars) {
                            case 2:
                                c = iFirst;
                                break;
                            case 3:
                                c = (iFirst*10)+(iSecond);
                                break;
                            case 4:
                                c = (iFirst*100)+(iSecond*10)+(iThird);
                                break;
                            default:
                                return false;
                        }
                        break;
                }

                iChars = 0;
                continue;
            default:
                return false;
        }

        switch(iChars) {
            case 1:
                iFirst = iActualChar;
                break;
            case 2:
                iSecond = iActualChar;
                break;
            case 3:
                iThird = iActualChar;
                break;
            default:
                return false;
        }
    }

    switch(iChars++) {
        case 1:
            d = iFirst;
            break;
        case 2:
            d = (iFirst*10)+(iSecond);
            break;
        case 3:
            d = (iFirst*100)+(iSecond*10)+(iThird);
            break;
        default:
            return false;
    }

    if(iDots != 3 || a > 255 || b > 255 || c > 255 || d > 255) {
        return false;
    } else {
        return true;
    }
}
//---------------------------------------------------------------------------

uint32_t HashNick(const char * sNick, const size_t &iNickLen) {
	char c;
    uint32_t h = 5381;

	for(size_t i = 0; i < iNickLen; i++) {
        c = (char)tolower(sNick[i]);
        h += (h << 5);
        h ^= c;
    }

	return (h+1);
}
//---------------------------------------------------------------------------

bool HashIP(char * sIP, const size_t ui32Len, uint32_t &ui32Hash) {
	uint32_t a, b, c, d;
    if(GetIpParts(sIP, ui32Len, a, b, c, d) == false) {
        return false;
    }

	ui32Hash = 16777216 * a + 65536 * b + 256 * c + d;

	return true;
}
//---------------------------------------------------------------------------

char * GenerateBanMessage(BanItem * Ban, int32_t &iMsgLen, const time_t &acc_time) {
    static char banmsg[2048], banmsg1[512];

    if(((Ban->ui8Bits & hashBanMan::PERM) == hashBanMan::PERM) == true) {
        iMsgLen = sprintf(banmsg, "<%s> %s.", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SORRY_PERM_BANNED]);
        if(CheckSprintf(iMsgLen, 2048, "GenerateBanMessage1") == false) {
            banmsg[0] = '\0';
            iMsgLen = 0;
            return banmsg;
        }
    } else {
        iMsgLen = sprintf(banmsg, "<%s> %s: %s", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SORRY_TEMP_BANNED], 
            formatSecTime(Ban->tempbanexpire-acc_time));
        if(CheckSprintf(iMsgLen, 2048, "GenerateBanMessage2") == false) {
            banmsg[0] = '\0';
            iMsgLen = 0;
            return banmsg;
        }
    }

    if(SettingManager->bBools[SETBOOL_BAN_MSG_SHOW_IP] == true && Ban->sIp[0] != '\0') {
        int iLen = sprintf(banmsg1, NEW_LINE_CHARS "%s: %s", LanguageManager->sTexts[LAN_IP], Ban->sIp);
        if(CheckSprintf(iLen, 512, "GenerateBanMessage3") == false) {
            banmsg[0] = '\0';
            iMsgLen = 0;
            return banmsg;
        }
        iMsgLen += iLen;
        strcat(banmsg, banmsg1);
    }

    if(SettingManager->bBools[SETBOOL_BAN_MSG_SHOW_NICK] == true && Ban->sNick != NULL) {
        int iLen = sprintf(banmsg1, NEW_LINE_CHARS "%s: %s", LanguageManager->sTexts[LAN_NICK], Ban->sNick);
        if(CheckSprintf(iLen, 512, "GenerateBanMessage4") == false) {
            banmsg[0] = '\0';
            iMsgLen = 0;
            return banmsg;
        }
        iMsgLen += iLen;
        strcat(banmsg, banmsg1);
    }

    if(SettingManager->bBools[SETBOOL_BAN_MSG_SHOW_REASON] == true && Ban->sReason != NULL) {
        int iLen = sprintf(banmsg1, NEW_LINE_CHARS "%s: %s", LanguageManager->sTexts[LAN_REASON], Ban->sReason);
        if(CheckSprintf(iLen, 512, "GenerateBanMessage5") == false) {
            banmsg[0] = '\0';
            iMsgLen = 0;
            return banmsg;
        }
        iMsgLen += iLen;
        strcat(banmsg, banmsg1);
    }

    if(SettingManager->bBools[SETBOOL_BAN_MSG_SHOW_BY] == true && Ban->sBy != NULL) {
        int iLen = sprintf(banmsg1, NEW_LINE_CHARS "%s: %s", LanguageManager->sTexts[LAN_BANNED_BY], Ban->sBy);
        if(CheckSprintf(iLen, 512, "GenerateBanMessage6") == false) {
            banmsg[0] = '\0';
            iMsgLen = 0;
            return banmsg;
        }
        iMsgLen += iLen;
        strcat(banmsg, banmsg1);
    }

    if(SettingManager->sTexts[SETTXT_MSG_TO_ADD_TO_BAN_MSG] != NULL) {
        int iLen = sprintf(banmsg1, NEW_LINE_CHARS "%s|", SettingManager->sTexts[SETTXT_MSG_TO_ADD_TO_BAN_MSG]);
        if(CheckSprintf(iLen, 512, "GenerateBanMessage7") == false) {
            banmsg[0] = '\0';
            iMsgLen = 0;
            return banmsg;
        }
        iMsgLen += iLen;
        strcat(banmsg, banmsg1);
    } else {
        banmsg[iMsgLen] = '|';
        iMsgLen++;
        banmsg[iMsgLen] = '\0';
    }

    if(((Ban->ui8Bits & hashBanMan::PERM) == hashBanMan::PERM) == true) {
        if(SettingManager->bBools[SETBOOL_PERM_BAN_REDIR] == true) {
            iMsgLen += (int)SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_PERM_BAN_REDIR_ADDRESS];
            strcat(banmsg, SettingManager->sPreTexts[SetMan::SETPRETXT_PERM_BAN_REDIR_ADDRESS]);
        }
    } else {
        if(SettingManager->bBools[SETBOOL_TEMP_BAN_REDIR] == true) {
            iMsgLen += (int)SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_TEMP_BAN_REDIR_ADDRESS];
            strcat(banmsg, SettingManager->sPreTexts[SetMan::SETPRETXT_TEMP_BAN_REDIR_ADDRESS]);
        }
    }

    return banmsg;
}
//---------------------------------------------------------------------------

char * GenerateRangeBanMessage(RangeBanItem * RangeBan, int32_t &iMsgLen, const time_t &acc_time) {
    static char banmsg[2048], banmsg1[512];

    if(((RangeBan->ui8Bits & hashBanMan::PERM) == hashBanMan::PERM) == true) {
        iMsgLen = sprintf(banmsg, "<%s> %s.", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SORRY_PERM_BANNED]);
        if(CheckSprintf(iMsgLen, 2048, "GenerateRangeBanMessage1") == false) {
            banmsg[0] = '\0';
            iMsgLen = 0;
            return banmsg;
        }
    } else {
        iMsgLen = sprintf(banmsg, "<%s> %s: %s", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SORRY_TEMP_BANNED],
            formatSecTime(RangeBan->tempbanexpire-acc_time));
        if(CheckSprintf(iMsgLen, 2048, "GenerateRangeBanMessage2") == false) {
            banmsg[0] = '\0';
            iMsgLen = 0;
            return banmsg;
        }
    }

    if(SettingManager->bBools[SETBOOL_BAN_MSG_SHOW_RANGE] == true) {
        int iLen = sprintf(banmsg1, NEW_LINE_CHARS "%s: %s-%s", LanguageManager->sTexts[LAN_RANGE], RangeBan->sIpFrom, RangeBan->sIpTo);
        if(CheckSprintf(iLen, 512, "GenerateRangeBanMessage3") == false) {
            banmsg[0] = '\0';
            iMsgLen = 0;
            return banmsg;
        }
        iMsgLen += iLen;
        strcat(banmsg, banmsg1);
    }

    if(SettingManager->bBools[SETBOOL_BAN_MSG_SHOW_REASON] == true && RangeBan->sReason != NULL) {
        int iLen = sprintf(banmsg1, NEW_LINE_CHARS "%s: %s", LanguageManager->sTexts[LAN_REASON], RangeBan->sReason);
        if(CheckSprintf(iLen, 512, "GenerateRangeBanMessage4") == false) {
            banmsg[0] = '\0';
            iMsgLen = 0;
            return banmsg;
        }
        iMsgLen += iLen;
        strcat(banmsg, banmsg1);
    }

    if(SettingManager->bBools[SETBOOL_BAN_MSG_SHOW_BY] == true && RangeBan->sBy != NULL) {
        int iLen = sprintf(banmsg1, NEW_LINE_CHARS "%s: %s", LanguageManager->sTexts[LAN_BANNED_BY], RangeBan->sBy);
        if(CheckSprintf(iLen, 512, "GenerateRangeBanMessage5") == false) {
            banmsg[0] = '\0';
            iMsgLen = 0;
            return banmsg;
        }
        iMsgLen += iLen;
        strcat(banmsg, banmsg1);
    }

    if(SettingManager->sTexts[SETTXT_MSG_TO_ADD_TO_BAN_MSG] != NULL) {
        int iLen = sprintf(banmsg1, NEW_LINE_CHARS "%s|", SettingManager->sTexts[SETTXT_MSG_TO_ADD_TO_BAN_MSG]);
        if(CheckSprintf(iLen, 512, "GenerateRangeBanMessage6") == false) {
            banmsg[0] = '\0';
            iMsgLen = 0;
            return banmsg;
        }
        iMsgLen += iLen;
        strcat(banmsg, banmsg1);
    } else {
        banmsg[iMsgLen] = '|';
        iMsgLen++;
        banmsg[iMsgLen] = '\0';
    }

    if(((RangeBan->ui8Bits & hashBanMan::PERM) == hashBanMan::PERM) == true) {
        if(SettingManager->bBools[SETBOOL_PERM_BAN_REDIR] == true) {
            iMsgLen += (int)SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_PERM_BAN_REDIR_ADDRESS];
            strcat(banmsg, SettingManager->sPreTexts[SetMan::SETPRETXT_PERM_BAN_REDIR_ADDRESS]);
        }
    } else {
        if(SettingManager->bBools[SETBOOL_TEMP_BAN_REDIR] == true) {
            iMsgLen += (int)SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_TEMP_BAN_REDIR_ADDRESS];
            strcat(banmsg, SettingManager->sPreTexts[SetMan::SETPRETXT_TEMP_BAN_REDIR_ADDRESS]);
        }
    }
    return banmsg;
}
//---------------------------------------------------------------------------

bool GenerateTempBanTime(const char &cMultiplyer, const uint32_t &iTime, time_t &acc_time, time_t &ban_time) {
    time(&acc_time);
    struct tm *tm = localtime(&acc_time);

    switch(cMultiplyer) {
        case 'm':
            tm->tm_min += iTime;
            break;
        case 'h':
            tm->tm_hour += iTime;
            break;
        case 'd':
            tm->tm_mday += iTime;
            break;
        case 'w':
            tm->tm_mday += iTime*7;
            break;
        case 'M':
            tm->tm_mon += iTime;
            break;
        case 'Y':
            tm->tm_year += iTime;
            break;
        default:
            return false;
    }

    tm->tm_isdst = -1;

    ban_time = mktime(tm);

    if(ban_time == (time_t)-1) {
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------

bool HaveOnlyNumbers(char *sData, const uint16_t &ui16Len) {
    for(uint16_t i = 0; i < ui16Len; i++) {
        if(isdigit(sData[i]) == 0)
            return false;
    }
    return true;
}
//---------------------------------------------------------------------------

int GetWlcmMsg(char * sWlcmMsg) {
#ifdef _WIN32
    int iLen =  sprintf(sWlcmMsg, "%s%I64d %s, %I64d %s, %I64d %s / %s: %I32d)|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_NAME_WLCM],
#else
	int iLen =  sprintf(sWlcmMsg, "%s%" PRIu64 " %s, %" PRIu64 " %s, %" PRIu64 " %s / %s: %u)|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_NAME_WLCM],
#endif
        iDays, LanguageManager->sTexts[LAN_DAYS_LWR], iHours, LanguageManager->sTexts[LAN_HOURS_LWR], 
        iMins, LanguageManager->sTexts[LAN_MINUTES_LWR], 
        LanguageManager->sTexts[LAN_USERS], ui32Logged);
    if(CheckSprintf(iLen, 1024, "GetWlcmMsg2") == false) {
        sWlcmMsg[0] = '\0';
        return 0;
    }

    return iLen;
}
//---------------------------------------------------------------------------

#ifdef _WIN32
	string GetMemStat() {
		string sStat = "";
	    if(b2K == true) {
	        PROCESS_MEMORY_COUNTERS pmc;
	        pmc.cb = sizeof(pmc);

			typedef BOOL (WINAPI *PGPMI)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);
			PGPMI pGPMI = (PGPMI)GetProcAddress(LoadLibrary("psapi.dll"), "GetProcessMemoryInfo");

            if(pGPMI != NULL) {
				pGPMI(GetCurrentProcess(), &pmc, sizeof(pmc));
					   
                sStat += "\r\nMem usage (Peak): "+string(formatBytes(pmc.WorkingSetSize))+ " ("+string(formatBytes(pmc.PeakWorkingSetSize))+")";
                sStat += "\r\nVM size (Peak): "+string(formatBytes(pmc.PagefileUsage))+ " ("+string(formatBytes(pmc.PeakPagefileUsage))+")";
            }
	    }
		return sStat;
	}
#endif
//---------------------------------------------------------------------------

bool CheckSprintf(int iRetVal, const size_t &iMax, const char * sMsg) {
    if(iRetVal > 0) {
		if(iMax != 0 && iRetVal >= (int)iMax) {
			string sDbgstr = "sprintf high value "+string(iRetVal)+"/"+string((uint64_t)iMax)+" in "+string(sMsg);
            AppendSpecialLog(sDbgstr);
            return false;
        }
    } else {
		string sDbgstr = "sprintf low value "+string(iRetVal)+" in "+string(sMsg);
		AppendSpecialLog(sDbgstr);
        return false;
    }
    return true;
}
//---------------------------------------------------------------------------

bool CheckSprintf1(int iRetVal, int iLenVal, const size_t &iMax, const char * sMsg) {
    if(iRetVal > 0) {
        if(iMax != 0 && iLenVal >= (int)iMax) {
			string sDbgstr = "sprintf high value "+string(iLenVal)+"/"+string((uint64_t)iMax)+" in "+string(sMsg);
			AppendSpecialLog(sDbgstr);
            return false;
        }
    } else {
		string sDbgstr = "sprintf low value "+string(iRetVal)+" in "+string(sMsg);
		AppendSpecialLog(sDbgstr);
        return false;
    }
    return true;
}
//---------------------------------------------------------------------------

void AppendLog(const string & sData, const bool &bScript/* == false*/) {
    if(UdpDebug != NULL && bScript == false) {
        static char msg[1024];
        if(sData.size() < 1000) {
            int imsgLen = sprintf(msg, "[LOG] %s", sData.c_str());
            if(CheckSprintf(imsgLen, 1024, "AppendLog1") == true) {
                UdpDebug->Broadcast(msg, imsgLen);
            }
        } else {
#ifdef _WIN32
            char *MSG = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, sData.size()+64);
#else
			char *MSG = (char *) malloc(sData.size()+64);
#endif
            if(MSG == NULL) {
    			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(sData.size()+64))+
    				" bytes of memory in AppendLog!";
#ifdef _WIN32
    			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
    			AppendSpecialLog(sDbgstr);
                UdpDebug->Broadcast(sDbgstr);
                return;
            }
            int imsgLen = sprintf(MSG, "[LOG] %s", sData.c_str());
            if(CheckSprintf(imsgLen, sData.size()+64, "AppendLog2") == true) {
                UdpDebug->Broadcast(MSG, imsgLen);
            }

#ifdef _WIN32
            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)MSG) == 0) {
    			string sDbgstr = "[BUF] Cannot deallocate MSG in AppendLog! "+string((uint32_t)GetLastError())+" "+
    				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
    			AppendSpecialLog(sDbgstr);
                UdpDebug->Broadcast(sDbgstr);
            }
#else
			free(MSG);
#endif
        }
    }

	FILE * fw;

	if(bScript == false) {
#ifdef _WIN32
        fw = fopen((PATH + "\\logs\\system.log").c_str(), "a");
#else
		fw = fopen((PATH + "/logs/system.log").c_str(), "a");
#endif
    } else {
#ifdef _WIN32
        fw = fopen((PATH + "\\logs\\script.log").c_str(), "a");
#else
		fw = fopen((PATH + "/logs/script.log").c_str(), "a");
#endif
    }

	if(fw == NULL) {
		return;
	}

	time_t acc_time;
	time(&acc_time);

	struct tm * acc_tm;
	acc_tm = localtime(&acc_time);

	char sBuf[64];
	strftime(sBuf, 64, "%c", acc_tm);

	string sTmp = string(sBuf) + " - " + sData + "\n";
	fprintf(fw, sTmp.c_str());

	fclose(fw);
}
//---------------------------------------------------------------------------

void AppendSpecialLog(const string & sData) {
#ifdef _WIN32
	FILE * fw = fopen((PATH + "\\logs\\debug.log").c_str(), "a");
#else
	FILE * fw = fopen((PATH + "/logs/debug.log").c_str(), "a");
#endif

	if(fw == NULL) {
		return;
	}

	time_t acc_time;
	time(&acc_time);

	struct tm * acc_tm;
	acc_tm = localtime(&acc_time);

	char sBuf[64];
	strftime(sBuf, 64, "%c", acc_tm);

	string sTmp = string(sBuf) + " - " + sData + NEW_LINE_CHARS;

	fprintf(fw, sTmp.c_str());

	fclose(fw);
}
//---------------------------------------------------------------------------

#ifdef _WIN32
	void GetHeapStats(void *hHeap, DWORD &dwCommitted, DWORD &dwUnCommitted) {
	    PROCESS_HEAP_ENTRY *lpEntry;
	
	    lpEntry = (PROCESS_HEAP_ENTRY *) calloc(1, sizeof(PROCESS_HEAP_ENTRY));
	    lpEntry->lpData = NULL;
	
	    while(HeapWalk((HANDLE)hHeap, (PROCESS_HEAP_ENTRY *)lpEntry) != 0) {
	        if((lpEntry->wFlags & PROCESS_HEAP_REGION)) {
	            dwCommitted += lpEntry->Region.dwCommittedSize;
	            dwUnCommitted += lpEntry->Region.dwUnCommittedSize;
	        }
	    }
	
	    free(lpEntry);
	}
	//---------------------------------------------------------------------------

	char * ExtractFileName(char * sPath) {
		char * sName = strrchr(sPath, '\\');
	
		if(sName != NULL) {
			return sName;
		} else {
	        return sPath;
	    }
	}
	//---------------------------------------------------------------------------

	#ifdef _SERVICE
		void Memo(const string & /*sMessage*/) {
	#else
		void Memo(const string & sMessage) {
			#ifndef _MSC_VER
				if(UsersChatForm != NULL) {
					UsersChatForm->Memo(sMessage);
				}
            #else
                RichEditAppendText(pMainWindowPageUsersChat->hWndPageItems[MainWindowPageUsersChat::REDT_CHAT], sMessage.c_str());
			#endif
	#endif
	}
#else
	void Memo(const string & /*sMessage*/) {
	    // ...
	}
#endif
//---------------------------------------------------------------------------

bool FileExist(char * sPath) {
#ifdef _WIN32
	DWORD code = GetFileAttributes(sPath);
	if(code != INVALID_FILE_ATTRIBUTES && code != FILE_ATTRIBUTE_DIRECTORY) {
#else
    struct stat st;
	if(stat(sPath, &st) == 0 && S_ISDIR(st.st_mode) == 0) {
#endif
		return true;
	}

	return false;
}
//---------------------------------------------------------------------------

bool DirExist(char * sPath) {
#ifdef _WIN32
	DWORD code = GetFileAttributes(sPath);
	if(code != INVALID_FILE_ATTRIBUTES && code == FILE_ATTRIBUTE_DIRECTORY) {
#else
    struct stat st;
	if(stat(sPath, &st) == 0 && S_ISDIR(st.st_mode)) {
#endif
		return true;
	}

	return false;
}
//---------------------------------------------------------------------------

#ifdef _WIN32
	void SetupOsVersion() {
		OSVERSIONINFOEX ver;
		memset(&ver, 0, sizeof(OSVERSIONINFOEX));
		ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	
		if(GetVersionEx((OSVERSIONINFO*)&ver) == 0) {
			sOs = "Windows (unknown version)";
		}
	
		if(ver.dwPlatformId != VER_PLATFORM_WIN32_NT) {
			sOs = "Windows 9x/ME";
	    } else if(ver.dwMajorVersion == 6) {
            if(ver.dwMinorVersion == 1) {
	           if(ver.wProductType == VER_NT_WORKSTATION) {
	               sOs = "Windows 7";
	           } else {
	               sOs = "Windows 2008 R2";
	           }
            } else {
	           if(ver.wProductType == VER_NT_WORKSTATION) {
	               sOs = "Windows Vista";
	           } else {
	               sOs = "Windows 2008";
	           }
            }
		} else if(ver.dwMajorVersion == 5) {
	        if(ver.dwMinorVersion == 2) {
                if(ver.wProductType != VER_NT_WORKSTATION) {
				    sOs = "Windows 2003";
				    return;
                } else if(ver.wProductType == VER_NT_WORKSTATION) {
                    SYSTEM_INFO si;
                    memset(&si, 0, sizeof(SYSTEM_INFO));

                    typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
                    PGNSI pGNSI = (PGNSI)::GetProcAddress(::GetModuleHandle("kernel32.dll"), "GetNativeSystemInfo");
                    if(pGNSI != NULL) {
                        pGNSI(&si);
                    } else {
                        GetSystemInfo(&si);
                    }

                    if(si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64) {
                        sOs = "Windows XP x64";
                        return;
                    }
                }

                // should not happen, but for sure...
                sOs = "Windows 2003/XP64";
			} else if(ver.dwMinorVersion == 1) {
				sOs = "Windows XP";
			} else if(ver.dwMinorVersion == 0) {
				sOs = "Windows 2000";
			}
		} else if(ver.dwMajorVersion == 4) {
			sOs = "Windows NT4";
		}
	}
#endif
//---------------------------------------------------------------------------
