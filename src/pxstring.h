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

//------------------------------------------------------------------------------
#ifndef pxstringh
#define pxstringh
//------------------------------------------------------------------------------

class string {
private:
    char * sData;
	size_t iDataLen;

	void stralloc(const char * sTxt, const size_t & iLen);
    string(const string & sStr1, const string & sStr2);
    string(const char * sTxt, const string & sStr);
    string(const string & sStr, const char * sTxt);
public:
    string();
    string(const char * sTxt);
    string(const char * sTxt, const size_t & iLen);
	string(const string & sStr);
	string(const uint32_t & ui32Number);
	string(const int32_t & i32Number);
	string(const uint64_t & ui64Number);
	string(const int64_t & i64Number);

    ~string();

    size_t size() const;
    char * c_str() const;

	string operator+(const char * sTxt) const;
	string operator+(const string & sStr) const;
	friend string operator+(const char * sTxt, const string & sStr);

	string & operator+=(const char * sTxt);
	string & operator+=(const string & sStr);

    string & operator=(const char * sTxt);
    string & operator=(const string & sStr);

	bool operator==(const char * sTxt);
	bool operator==(const string & sStr);
};
//------------------------------------------------------------------------------

#endif
