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
#ifndef TextConverterH
#define TextConverterH
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef _WIN32
	typedef void * iconv_t;
#endif
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

class TextConverter {
private:
#ifndef _WIN32
	iconv_t iconvUtfCheck;
	iconv_t iconvAsciiToUtf;
#endif

	bool CheckUtf8Validity(char * sInput, const uint8_t &ui8InputLen, char * sOutput, const uint8_t &ui8OutputSize);

    TextConverter(const TextConverter&);
    const TextConverter& operator=(const TextConverter&);
public:
    static TextConverter * mPtr;

	TextConverter();
	~TextConverter();

	char * CheckUtf8AndConvert(char * sInput, const uint8_t &ui8InputLen, char * sOutput, const uint8_t &ui8OutputSize);
};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif
