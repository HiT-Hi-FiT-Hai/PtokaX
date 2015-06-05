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

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef DBSQLiteH
#define DBSQLiteH
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct User;
typedef struct sqlite3 sqlite3;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

class DBSQLite {
private:
	sqlite3 * pDB;

	bool bConnected;

    DBSQLite(const DBSQLite&);
    const DBSQLite& operator=(const DBSQLite&);
public:
    static DBSQLite * mPtr;

	DBSQLite();
	~DBSQLite();

	void UpdateRecord(User * pUser);

	bool SearchNick(char * sNick, const uint8_t &ui8NickLen, User * pUser, const bool &bFromPM);
	bool SearchIP(char * sIP, User * pUser, const bool &bFromPM);

	void RemoveOldRecords(const uint16_t &ui16Days);
};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif
