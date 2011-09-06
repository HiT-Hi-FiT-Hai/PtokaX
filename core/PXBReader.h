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

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef PXBReaderH
#define PXBReaderH
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

class PXBReader {
private:
    bool bFullRead;

    char * sFile, *sActualPosition;

    FILE * pFile;

    size_t szRemainingSize;
public:
    enum enmDataTypes {
        PXB_BYTE,
        PXB_TWO_BYTES,
        PXB_FOUR_BYTES,
        PXB_STRING
    };

    void * pItemDatas[10];

    uint16_t ui16ItemLengths[10];

    char sItemIdentifiers[10][2];

    uint8_t ui8ItemValues[10];

	PXBReader();
	~PXBReader();

    bool LoadFile(const char * sFilename);
    bool ReadNextItem(const uint16_t * sExpectedIdentificators, const uint8_t &ui8ExpectedSubItems);

    bool SaveFile(const char * sFilename);
    bool WriteNextItem(const uint32_t &ui32Length, const uint8_t &ui8SubItems);
    void WriteRemaining();
};
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif