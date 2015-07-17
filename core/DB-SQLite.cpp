/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2004-2015 Petr Kozelka, PPK at PtokaX dot org

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

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "stdinc.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "DB-SQLite.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "hashRegManager.h"
#include "hashUsrManager.h"
#include "IP2Country.h"
#include "LanguageManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "TextConverter.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include <sqlite3.h>
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
DBSQLite * DBSQLite::mPtr = NULL;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

DBSQLite::DBSQLite() {
	if(clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_DATABASE] == false) {
		bConnected = false;

		return;
	}

#ifdef _WIN32
	int iRet = sqlite3_open((clsServerManager::sPath + "\\cfg\\users.sqlite").c_str(), &pDB);
#else
	int iRet = sqlite3_open((clsServerManager::sPath + "/cfg/users.sqlite").c_str(), &pDB);
#endif
	if(iRet != SQLITE_OK) {
		bConnected = false;
		AppendLog(string("DBSQLite connection failed: ")+sqlite3_errmsg(pDB));
		sqlite3_close(pDB);

		return;
	}

	char * sErrMsg = NULL;

	iRet = sqlite3_exec(pDB, "PRAGMA synchronous = NORMAL;"
		"PRAGMA journal_mode = WAL;",
		NULL, NULL, &sErrMsg);

	if(iRet != SQLITE_OK) {
		bConnected = false;
		AppendLog(string("DBSQLite PRAGMA set failed: ")+sErrMsg);
		sqlite3_free(sErrMsg);
		sqlite3_close(pDB);

		return;
	}

	iRet = sqlite3_exec(pDB, 
		"CREATE TABLE IF NOT EXISTS userinfo ("
			"nick VARCHAR(64) NOT NULL PRIMARY KEY,"
			"last_updated DATETIME NOT NULL,"
			"ip_address VARCHAR(39) NOT NULL,"
			"share VARCHAR(24) NOT NULL,"
			"description VARCHAR(192),"
			"tag VARCHAR(192),"
			"connection VARCHAR(32),"
			"email VARCHAR(96),"
			"UNIQUE (nick COLLATE NOCASE)"
		");", NULL, NULL, &sErrMsg);

	if(iRet != SQLITE_OK) {
		bConnected = false;
		AppendLog(string("DBSQLite check/create table failed: ")+sErrMsg);
		sqlite3_free(sErrMsg);
		sqlite3_close(pDB);

		return;
	}

	bConnected = true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
DBSQLite::~DBSQLite() {
	// When user don't want to save data in database forever then he can set to remove records older than X days.
	if(clsSettingManager::mPtr->i16Shorts[SETSHORT_DB_REMOVE_OLD_RECORDS] != 0) {
		RemoveOldRecords(clsSettingManager::mPtr->i16Shorts[SETSHORT_DB_REMOVE_OLD_RECORDS]);
	}

	if(bConnected == true) {
		sqlite3_close(pDB);
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Now that important part. Function to update or insert user to database.
void DBSQLite::UpdateRecord(User * pUser) {
	if(bConnected == false) {
		return;
	}

	char sNick[65];
	if(TextConverter::mPtr->CheckUtf8AndConvert(pUser->sNick, pUser->ui8NickLen, sNick, 65) == 0) {
		return;
	}

	char sShare[24];
	sprintf(sShare, "%0.02f GB", (double)pUser->ui64SharedSize/1073741824);

	char sDescription[193];
	if(pUser->sDescription != NULL) {
		TextConverter::mPtr->CheckUtf8AndConvert(pUser->sDescription, pUser->ui8DescriptionLen, sDescription, 193);
	}

	char sTag[193];
	if(pUser->sTag != NULL) {
		TextConverter::mPtr->CheckUtf8AndConvert(pUser->sTag, pUser->ui8TagLen, sTag, 193);
	}

	char sConnection[33];
	if(pUser->sConnection != NULL) {
		TextConverter::mPtr->CheckUtf8AndConvert(pUser->sConnection, pUser->ui8ConnectionLen, sConnection, 33);
	}

	char sEmail[97];
	if(pUser->sEmail != NULL) {
		TextConverter::mPtr->CheckUtf8AndConvert(pUser->sEmail, pUser->ui8EmailLen, sEmail, 97);
	}

	char sSQLCommand[1024];
	sqlite3_snprintf(1024, sSQLCommand,
		"UPDATE userinfo SET "
		"nick = %Q,"
		"last_updated = DATETIME('now')," // last_updated
		"ip_address = %Q," // ip
		"share = %Q," // share
		"description = %Q," // description
		"tag = %Q," // tag
		"connection = %Q," // connection
		"email = %Q" // email
		"WHERE LOWER(nick) = LOWER(%Q);", // nick
		sNick, pUser->sIP, sShare, sDescription, sTag, sConnection, sEmail, sNick
	);

	char * sErrMsg = NULL;

	int iRet = sqlite3_exec(pDB, sSQLCommand, NULL, NULL, &sErrMsg);

	if(iRet != SQLITE_OK) {
		clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBSQLite update record failed: %s", sErrMsg);
		sqlite3_free(sErrMsg);
	}

	iRet = sqlite3_changes(pDB);
	if(iRet != 0) {
		return;
	}

	sqlite3_snprintf(1024, sSQLCommand,
		"INSERT INTO userinfo (nick, last_updated, ip_address, share, description, tag, connection, email) VALUES ("
		"%Q," // nick
		"DATETIME('now')," // last_updated
		"%Q," // ip
		"%Q," // share
		"%Q," // description
		"%Q," // tag
		"%Q," // connection
		"%Q" // email
		");",
		sNick, pUser->sIP, sShare, sDescription, sTag, sConnection, sEmail
	);

	iRet = sqlite3_exec(pDB, sSQLCommand, NULL, NULL, &sErrMsg);

	if(iRet != SQLITE_OK) {
		clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBSQLite insert record failed: %s", sErrMsg);
		sqlite3_free(sErrMsg);
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static bool bFirst = true;
static bool bSecond = false;
static char sFirstNick[65];
static char sFirstIP[40];
static int iMsgLen = 0;
static int iAfterHubSecMsgLen = 0;

static int SelectCallBack(void *, int iArgCount, char ** ppArgSTrings, char **) {
	if(iArgCount != 8) {
		clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBSQLite SelectCallBack wrong iArgCount: %d", iArgCount);
		return 0;
	}

	if(bFirst == true) {
		bFirst = false;
		bSecond = true;

		size_t szLength = strlen(ppArgSTrings[0]);
		memcpy(sFirstNick, ppArgSTrings[0], szLength);
		sFirstNick[szLength] = '\0';

		szLength = strlen(ppArgSTrings[2]);
		memcpy(sFirstIP, ppArgSTrings[2], szLength);
		sFirstIP[szLength] = '\0';

		szLength = strlen(ppArgSTrings[0]);
		if(szLength == 0 || szLength > 64) {
			clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBSQLite search returned invalid nick length: " PRIu64, (uint64_t)szLength);
			return 0;
		}

        int iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: %s", clsLanguageManager::mPtr->sTexts[LAN_NICK], ppArgSTrings[0]);
        iMsgLen += iRet;
        if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SelectCallBack1") == false) {
			return 0;
        }

		RegUser * pReg = clsRegManager::mPtr->Find(ppArgSTrings[0], szLength);
        if(pReg != NULL) {
            iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: %s", clsLanguageManager::mPtr->sTexts[LAN_PROFILE], clsProfileManager::mPtr->ppProfilesTable[pReg->ui16Profile]->sName);
        	iMsgLen += iRet;
        	if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SelectCallBack2") == false) {
        		return 0;
            }
        }

		// In case when SQL wildcards were used is possible that user is online. Then we don't use data from database, but data that are in server memory.
		User * pOnlineUser = clsHashManager::mPtr->FindUser(ppArgSTrings[0], szLength);
		if(pOnlineUser != NULL) {
            iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: %s ", clsLanguageManager::mPtr->sTexts[LAN_STATUS], clsLanguageManager::mPtr->sTexts[LAN_ONLINE_FROM]);
        	iMsgLen += iRet;
        	if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SelectCallBack3") == false) {
        		return 0;
            }

            struct tm *tm = localtime(&pOnlineUser->tLoginTime);
            iRet = (int)strftime(clsServerManager::pGlobalBuffer+iMsgLen, 256, "%c", tm);
            iMsgLen += iRet;
            if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SelectCallBack4") == false) {
        		return 0;
            }

			if(pOnlineUser->sIPv4[0] != '\0') {
				iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: %s / %s\n%s: %0.02f %s", clsLanguageManager::mPtr->sTexts[LAN_IP], pOnlineUser->sIP, pOnlineUser->sIPv4, clsLanguageManager::mPtr->sTexts[LAN_SHARE_SIZE], (double)pOnlineUser->ui64SharedSize/1073741824, clsLanguageManager::mPtr->sTexts[LAN_GIGA_BYTES]);
            	iMsgLen += iRet;
            	if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SelectCallBack5") == false) {
        			return 0;
				}
			} else {
				iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: %s\n%s: %0.02f %s", clsLanguageManager::mPtr->sTexts[LAN_IP], pOnlineUser->sIP, clsLanguageManager::mPtr->sTexts[LAN_SHARE_SIZE], (double)pOnlineUser->ui64SharedSize/1073741824, clsLanguageManager::mPtr->sTexts[LAN_GIGA_BYTES]);
            	iMsgLen += iRet;
            	if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SelectCallBack6") == false) {
        			return 0;
				}
			}

            if(pOnlineUser->sDescription != NULL) {
                iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: ", clsLanguageManager::mPtr->sTexts[LAN_DESCRIPTION]);
            	iMsgLen += iRet;
            	if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SelectCallBack7") == false) {
                    return 0;
                }
                memcpy(clsServerManager::pGlobalBuffer+iMsgLen, pOnlineUser->sDescription, pOnlineUser->ui8DescriptionLen);
                iMsgLen += (int)pOnlineUser->ui8DescriptionLen;
            }

            if(pOnlineUser->sTag != NULL) {
                iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: ", clsLanguageManager::mPtr->sTexts[LAN_TAG]);
            	iMsgLen += iRet;
            	if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SelectCallBack8") == false) {
                    return 0;
                }
                memcpy(clsServerManager::pGlobalBuffer+iMsgLen, pOnlineUser->sTag, pOnlineUser->ui8TagLen);
                iMsgLen += (int)pOnlineUser->ui8TagLen;
            }
                    
            if(pOnlineUser->sConnection != NULL) {
                iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: ", clsLanguageManager::mPtr->sTexts[LAN_CONNECTION]);
            	iMsgLen += iRet;
            	if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SelectCallBack9") == false) {
                    return 0;
                }
                memcpy(clsServerManager::pGlobalBuffer+iMsgLen, pOnlineUser->sConnection, pOnlineUser->ui8ConnectionLen);
                iMsgLen += (int)pOnlineUser->ui8ConnectionLen;
            }

            if(pOnlineUser->sEmail != NULL) {
                iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: ", clsLanguageManager::mPtr->sTexts[LAN_EMAIL]);
            	iMsgLen += iRet;
            	if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SelectCallBack10") == false) {
                    return 0;
                }
                memcpy(clsServerManager::pGlobalBuffer+iMsgLen, pOnlineUser->sEmail, pOnlineUser->ui8EmailLen);
                iMsgLen += (int)pOnlineUser->ui8EmailLen;
            }

            if(clsIpP2Country::mPtr->ui32Count != 0) {
                iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: ", clsLanguageManager::mPtr->sTexts[LAN_COUNTRY]);
            	iMsgLen += iRet;
            	if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SelectCallBack11") == false) {
                    return 0;
                }
                memcpy(clsServerManager::pGlobalBuffer+iMsgLen, clsIpP2Country::mPtr->GetCountry(pOnlineUser->ui8Country, false), 2);
                iMsgLen += 2;
            }

            return 0;
		} else { // User is offline, then we use data from database.
            iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: %s ", clsLanguageManager::mPtr->sTexts[LAN_STATUS], clsLanguageManager::mPtr->sTexts[LAN_OFFLINE_FROM]);
        	iMsgLen += iRet;
        	if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SelectCallBack12") == false) {
        		return 0;
            }
#ifdef _WIN32
        	time_t tTime = (time_t)_strtoui64(ppArgSTrings[1], NULL, 10);
#else
			time_t tTime = (time_t)strtoull(ppArgSTrings[1], NULL, 10);
#endif
            struct tm *tm = localtime(&tTime);
            iRet = (int)strftime(clsServerManager::pGlobalBuffer+iMsgLen, 256, "%c", tm);
            iMsgLen += iRet;
            if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SelectCallBack13") == false) {
        		return 0;
            }

			szLength = strlen(ppArgSTrings[2]);
			if(szLength == 0 || szLength > 39) {
				clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBSQLite search returned invalid ip length: " PRIu64, (uint64_t)szLength);
				return 0;
			}

			szLength = strlen(ppArgSTrings[3]);
			if(szLength == 0 || szLength > 24) {
				clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBSQLite search returned invalid share length: " PRIu64, (uint64_t)szLength);
				return 0;
			}

			char * sIP = ppArgSTrings[2];

			iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: %s\n%s: %s", clsLanguageManager::mPtr->sTexts[LAN_IP], sIP, clsLanguageManager::mPtr->sTexts[LAN_SHARE_SIZE], ppArgSTrings[3]);
            iMsgLen += iRet;
            if(CheckSprintf1(iRet, iMsgLen, 1024, "SelectCallBack14") == false) {
        		return 0;
			}

			szLength = strlen(ppArgSTrings[4]);
            if(szLength != 0) {
				if(szLength > 192) {
					clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBSQLite search returned invalid description length: " PRIu64, (uint64_t)szLength);
					return 0;
				}

				if(szLength > 0) {
                	iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: %s", clsLanguageManager::mPtr->sTexts[LAN_DESCRIPTION], ppArgSTrings[4]);
            		iMsgLen += iRet;
            		if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SelectCallBack15") == false) {
        				return 0;
        			}
                }
            }

			szLength = strlen(ppArgSTrings[5]);
            if(szLength != 0) {
				if(szLength > 192) {
					clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBSQLite search returned invalid tag length: " PRIu64, (uint64_t)szLength);
					return 0;
				}

				if(szLength > 0) {
                	iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: %s", clsLanguageManager::mPtr->sTexts[LAN_TAG], ppArgSTrings[5]);
            		iMsgLen += iRet;
            		if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SelectCallBack16") == false) {
                    	return 0;
                    }
                }
            }

			szLength = strlen(ppArgSTrings[6]);
            if(szLength != 0) {
				if(szLength > 32) {
					clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBSQLite search returned invalid connection length: " PRIu64, (uint64_t)szLength);
					return 0;
				}

				if(szLength > 0) {
                	iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: %s", clsLanguageManager::mPtr->sTexts[LAN_CONNECTION], ppArgSTrings[6]);
            		iMsgLen += iRet;
            		if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SelectCallBack17") == false) {
                    	return 0;
                    }
                }
            }

			szLength = strlen(ppArgSTrings[7]);
            if(szLength != 0) {
				if(szLength > 96) {
					clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBPostgreSQL search returned invalid email length: " PRIu64, (uint64_t)szLength);
					return 0;
				}

				if(szLength > 0) {
                	iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: %s", clsLanguageManager::mPtr->sTexts[LAN_EMAIL], ppArgSTrings[7]);
            		iMsgLen += iRet;
            		if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SelectCallBack18") == false) {
                    	return 0;
                    }
                }
            }

			uint8_t ui128IPHash[16];
			memset(ui128IPHash, 0, 16);

            if(clsIpP2Country::mPtr->ui32Count != 0 && HashIP(sIP, ui128IPHash) == true) {
                iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: ", clsLanguageManager::mPtr->sTexts[LAN_COUNTRY]);
            	iMsgLen += iRet;
            	if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SelectCallBack19") == false) {
                    return 0;
                }

                memcpy(clsServerManager::pGlobalBuffer+iMsgLen, clsIpP2Country::mPtr->Find(ui128IPHash, false), 2);
                iMsgLen += 2;
            }

            return 0;
        }
	} else if(bSecond == true) {
		bSecond = false;

		size_t szLength = strlen(sFirstNick);
		if(szLength == 0 || szLength > 64) {
			clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBSQLite search returned invalid nick length: " PRIu64, (uint64_t)szLength);
			return 0;
		}
	
		szLength = strlen(sFirstIP);
		if(szLength == 0 || szLength > 39) {
			clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBSQLite search returned invalid ip length: " PRIu64, (uint64_t)szLength);
			return 0;
		}
	
	    int iRet = sprintf(clsServerManager::pGlobalBuffer+iAfterHubSecMsgLen, "\n%s: %s\t\t%s: %s", clsLanguageManager::mPtr->sTexts[LAN_NICK], sFirstNick, clsLanguageManager::mPtr->sTexts[LAN_IP], sFirstIP);
	    iMsgLen = iAfterHubSecMsgLen+iRet;
	    if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SelectCallBack20") == false) {
			return 0;
	    }
	}

	size_t szLength = strlen(ppArgSTrings[0]);
	if(szLength == 0 || szLength > 64) {
		clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBSQLite search returned invalid nick length: " PRIu64, (uint64_t)szLength);
		return 0;
	}

	szLength = strlen(ppArgSTrings[2]);
	if(szLength == 0 || szLength > 39) {
		clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBSQLite search returned invalid ip length: " PRIu64, (uint64_t)szLength);
		return 0;
	}

    int iRet = sprintf(clsServerManager::pGlobalBuffer+iMsgLen, "\n%s: %s\t\t%s: %s", clsLanguageManager::mPtr->sTexts[LAN_NICK], ppArgSTrings[0], clsLanguageManager::mPtr->sTexts[LAN_IP], ppArgSTrings[2]);
    iMsgLen += iRet;
    if(CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SelectCallBack21") == false) {
		return 0;
    }

	return 0;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// First of two functions to search data in database. Nick will be probably most used.
bool DBSQLite::SearchNick(char * sNick, const uint8_t &ui8NickLen, User * pUser, const bool &bFromPM) {
	if(bConnected == false) {
		return false;
	}

	char sUtfNick[65];
	if(TextConverter::mPtr->CheckUtf8AndConvert(sNick, ui8NickLen, sUtfNick, 65) == 0) {
		return false;
	}

	if(bFromPM == true) {
    	iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "$To: %s From: %s $<%s> ", pUser->sNick, clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC],
			clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC]);
    	if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "DBSQLite::SearchNick1") == false) {
        	return false;
    	}
    } else {
    	iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "<%s> ", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC]);
    	if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "DBSQLite::SearchNick2") == false) {
        	return false;
    	}
	}

	iAfterHubSecMsgLen = iMsgLen;

	bFirst = true;
	bSecond = false;

	char sSQLCommand[256];
	sqlite3_snprintf(256, sSQLCommand, "SELECT nick, %s, ip_address, share, description, tag, connection, email FROM userinfo WHERE LOWER(nick) LIKE LOWER(%Q) ORDER BY last_updated DESC LIMIT 50;", "strftime('%s', last_updated)", sUtfNick);

	char * sErrMsg = NULL;

	int iRet = sqlite3_exec(pDB, sSQLCommand, SelectCallBack, NULL, &sErrMsg);

	if(iRet != SQLITE_OK) {
		clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBSQLite search for nick failed: %s", sErrMsg);
		sqlite3_free(sErrMsg);

		return false;
	}

	if(iMsgLen == iAfterHubSecMsgLen) {
		return false;
	} else {
		clsServerManager::pGlobalBuffer[iMsgLen] = '|';
        clsServerManager::pGlobalBuffer[iMsgLen+1] = '\0';  
        pUser->SendCharDelayed(clsServerManager::pGlobalBuffer, iMsgLen+1);

		return true;
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Second of two fnctions to search data in database. Now using IP.
bool DBSQLite::SearchIP(char * sIP, User * pUser, const bool &bFromPM) {
	if(bConnected == false) {
		return false;
	}

	if(bFromPM == true) {
    	iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "$To: %s From: %s $<%s> ", pUser->sNick, clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC],
			clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC]);
    	if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "DBSQLite::SearchIP1") == false) {
        	return false;
    	}
    } else {
    	iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "<%s> ", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC]);
    	if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "DBSQLite::SearchIP2") == false) {
        	return false;
    	}
	}

	iAfterHubSecMsgLen = iMsgLen;

	bFirst = true;
	bSecond = false;

	char sSQLCommand[256];
	sqlite3_snprintf(256, sSQLCommand, "SELECT nick, %s, ip_address, share, description, tag, connection, email FROM userinfo WHERE ip_address LIKE %Q ORDER BY last_updated DESC LIMIT 50;", "strftime('%s', last_updated)", sIP);

	char * sErrMsg = NULL;

	int iRet = sqlite3_exec(pDB, sSQLCommand, SelectCallBack, NULL, &sErrMsg);

	if(iRet != SQLITE_OK) {
		clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBSQLite search for nick failed: %s", sErrMsg);
		sqlite3_free(sErrMsg);

		return false;
	}

	if(iMsgLen == iAfterHubSecMsgLen) {
		return false;
	} else {
        clsServerManager::pGlobalBuffer[iMsgLen] = '|';
        clsServerManager::pGlobalBuffer[iMsgLen+1] = '\0';  
        pUser->SendCharDelayed(clsServerManager::pGlobalBuffer, iMsgLen+1);

		return true;
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Function to remove X days old records from database.
void DBSQLite::RemoveOldRecords(const uint16_t &ui16Days) {
	if(bConnected == false) {
		return;
	}

	char sSQLCommand[256];
	sprintf(sSQLCommand, "DELETE FROM userinfo WHERE last_updated < DATETIME('now', '-%hu days', 'localtime');", ui16Days);

	char * sErrMsg = NULL;

	int iRet = sqlite3_exec(pDB, sSQLCommand, SelectCallBack, NULL, &sErrMsg);

	if(iRet != SQLITE_OK) {
		clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBSQLite remove old records failed: %s", sErrMsg);
		sqlite3_free(sErrMsg);
	}

	iRet = sqlite3_changes(pDB);
	if(iRet != 0) {
		clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBSQLite removed old records: %d", iRet);
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
