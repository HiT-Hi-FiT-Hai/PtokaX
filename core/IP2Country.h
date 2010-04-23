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

//---------------------------------------------------------------------------
#ifndef IP2CountryH
#define IP2CountryH
//---------------------------------------------------------------------------

class IP2CC {
private:
    uint32_t ui32Size;
    uint32_t * ui32RangeFrom, * ui32RangeTo;
    uint8_t * ui8RangeCI;
public:
    uint32_t ui32Count;

	IP2CC();
	~IP2CC();

	const char * Find(const uint32_t &ui32Hash, const bool &bCountryName);
	uint8_t Find(const uint32_t &ui32Hash);

    const char * GetCountry(const uint8_t &ui8dx, const bool &bCountryName);
};
//---------------------------------------------------------------------------
extern IP2CC *IP2Country;
//--------------------------------------------------------------------------- 

#endif
