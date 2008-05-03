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
#include "stdinc.h"
//---------------------------------------------------------------------------
#include "pxstring.h"
//---------------------------------------------------------------------------
#include "utility.h"
//---------------------------------------------------------------------------
static const char * sEmpty = "";
//---------------------------------------------------------------------------

void string::stralloc(const char * sTxt, const size_t & iLen) {
	iDataLen = iLen;

	if(iDataLen == 0) {
		sData = (char *)sEmpty;
		return;
	}

	sData = (char *)malloc(iDataLen+1);
	if(sData == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string(iDataLen+1)+
			" bytes of memory for sData in string::stralloc(const char *, const size_t &)!";
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
	}

	memcpy(sData, sTxt, iDataLen);
	sData[iDataLen] = '\0';
}
//---------------------------------------------------------------------------

string::string() {
	sData = (char *)sEmpty;
    iDataLen = 0;
}
//---------------------------------------------------------------------------

string::string(const char * sTxt) {
    stralloc(sTxt, strlen(sTxt));
}
//---------------------------------------------------------------------------

string::string(const char * sTxt, const size_t & iLen) {
	stralloc(sTxt, iLen);
}
//---------------------------------------------------------------------------

string::string(const string & sStr) {
    stralloc(sStr.c_str(), sStr.size());
}
//---------------------------------------------------------------------------

string::string(const uint32_t & ui32Number) {
	char tmp[16];
	int iLen = sprintf(tmp, "%u", ui32Number);
	stralloc(tmp, iLen);
}
//---------------------------------------------------------------------------

string::string(const int32_t & i32Number) {
	char tmp[16];
	int iLen = sprintf(tmp, "%d", i32Number);
	stralloc(tmp, iLen);
}
//---------------------------------------------------------------------------

string::string(const uint64_t & ui64Number) {
	char tmp[32];
	int iLen = sprintf(tmp, "%" PRIu64, ui64Number);
	stralloc(tmp, iLen);
}
//---------------------------------------------------------------------------

string::string(const int64_t & i64Number) {
	char tmp[32];
	int iLen = sprintf(tmp, "%" PRId64, i64Number);
	stralloc(tmp, iLen);
}
//---------------------------------------------------------------------------

string::string(const string & sStr1, const string & sStr2) {
    iDataLen = sStr1.size()+sStr2.size();

    if(iDataLen == 0) {
        sData = (char *)sEmpty;
        return;
    }

    sData = (char *)malloc(iDataLen+1);
    if(sData == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string(iDataLen+1)+
            " bytes of memory for sData in string::string(const string &, const string &)!";
        AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    memcpy(sData, sStr1.c_str(), sStr1.size());
    memcpy(sData+sStr1.size(), sStr2.c_str(), sStr2.size());
    sData[iDataLen] = '\0';
}
//---------------------------------------------------------------------------

string::string(const char * sTxt, const string & sStr) {
    size_t iLen = strlen(sTxt);
	iDataLen = iLen+sStr.size();

    if(iDataLen == 0) {
        sData = (char *)sEmpty;
        return;
    }

    sData = (char *)malloc(iDataLen+1);
    if(sData == NULL) {
        string sDbgstr = "[BUF] Cannot allocate "+string(iDataLen+1)+
            " bytes of memory for sData in string::string(const char *, const string &)!";
        AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    memcpy(sData, sTxt, iLen);
    memcpy(sData+iLen, sStr.c_str(), sStr.size());
    sData[iDataLen] = '\0';
}
//---------------------------------------------------------------------------

string::string(const string & sStr, const char * sTxt) {
    size_t iLen = strlen(sTxt);
	iDataLen = sStr.size()+iLen;

    if(iDataLen == 0) {
        sData = (char *)sEmpty;
        return;
    }

    sData = (char *)malloc(iDataLen+1);
    if(sData == NULL) {
        string sDbgstr = "[BUF] Cannot allocate "+string(iDataLen+1)+
            " bytes of memory for sData in string::string(const string &, const char *)!";
        AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    memcpy(sData, sStr.c_str(), sStr.size());
    memcpy(sData+sStr.size(), sTxt, iLen);
    sData[iDataLen] = '\0';
}
//---------------------------------------------------------------------------

string::~string() {
    if(sData != sEmpty) {
        free(sData);
    }
}
//---------------------------------------------------------------------------

size_t string::size() const {
    return iDataLen;
}
//---------------------------------------------------------------------------

char * string::c_str() const {
	return sData;
}
//---------------------------------------------------------------------------

string string::operator+(const char * sTxt) const {
	string result(*this, sTxt);
	return result;
}
//---------------------------------------------------------------------------

string string::operator+(const string & sStr) const {
    string result(*this, sStr);
	return result;
}
//---------------------------------------------------------------------------

string operator+(const char * sTxt, const string & sStr) {
	string result(sTxt, sStr);
	return result;
}
//---------------------------------------------------------------------------

string & string::operator+=(const char * sTxt) {
    size_t iLen = strlen(sTxt);

    if(sData == sEmpty) {
        sData = (char *)malloc(iDataLen+iLen+1);
    } else {
        sData = (char *)realloc(sData, iDataLen+iLen+1);
    }
    if(sData == NULL) {
        string sDbgstr = "[BUF] Cannot allocate "+string(iDataLen+iLen+1)+
            " bytes of memory for sData in string::operator+=(const char *)!";
        AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    memcpy(sData+iDataLen, sTxt, iLen);
	iDataLen += iLen;
	sData[iDataLen] = '\0';

    return *this;
}
//---------------------------------------------------------------------------

string & string::operator+=(const string & sStr) {
    if(sStr.c_str() == sEmpty) {
        return *this;
    }

    if(sData == sEmpty) {
        sData = (char *)malloc(iDataLen+sStr.size()+1);
    } else {
        sData = (char *)realloc(sData, iDataLen+sStr.size()+1);
    }
    if(sData == NULL) {
        string sDbgstr = "[BUF] Cannot allocate "+string(iDataLen+sStr.size()+1)+
            " bytes of memory for sData in string::operator+=(const char *)!";
        AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    memcpy(sData+iDataLen, sStr.c_str(), sStr.size());
	iDataLen += sStr.size();
	sData[iDataLen] = '\0';

    return *this;
}
//---------------------------------------------------------------------------

string & string::operator=(const char * sTxt) {
	if(sData != sEmpty) {
        free(sData);
    }

	stralloc(sTxt, strlen(sTxt));

    return *this;
}
//---------------------------------------------------------------------------

string & string::operator=(const string & sStr) {
    if(sData != sEmpty) {
        free(sData);
    }

	stralloc(sStr.c_str(), sStr.size());

    return *this;
}
//---------------------------------------------------------------------------

bool string::operator==(const char * sTxt) {
    if(iDataLen != strlen(sTxt) || 
        memcmp(sData, sTxt, iDataLen) != 0) {
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------

bool string::operator==(const string & sStr) {
    if(iDataLen != sStr.size() || 
        memcmp(sData, sStr.c_str(), iDataLen) != 0) {
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------
