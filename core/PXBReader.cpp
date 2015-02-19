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

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "stdinc.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "PXBReader.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "ServerManager.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

PXBReader::PXBReader() : bFullRead(false), pActualPosition(NULL), pFile(NULL), szRemainingSize(0) {
    memset(&pItemDatas, 0, sizeof(pItemDatas));
    memset(&ui16ItemLengths, 0, sizeof(ui16ItemLengths));
    memset(&sItemIdentifiers, 0, sizeof(sItemIdentifiers));
    memset(&ui8ItemValues, 0, sizeof(ui8ItemValues));
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
PXBReader::~PXBReader() {
    if(pFile != NULL) {
        fclose(pFile);
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool PXBReader::OpenFileRead(const char * sFilename) {
    pFile = fopen(sFilename, "rb");

    if(pFile == NULL) {
        return false;
    }

    fseek(pFile, 0, SEEK_END);
    long lFileLen = ftell(pFile);

    if(lFileLen <= 0) {
        return false;
    }

    fseek(pFile, 0, SEEK_SET);

    szRemainingSize = 131072;

    if((size_t)lFileLen < szRemainingSize) {
        szRemainingSize = lFileLen;

        bFullRead = true;
    }

    if(fread(clsServerManager::pGlobalBuffer, 1, szRemainingSize, pFile) != szRemainingSize) {
        return false;
    }

    pActualPosition = clsServerManager::pGlobalBuffer;

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void PXBReader::ReadNextFilePart() {
    memmove(clsServerManager::pGlobalBuffer, pActualPosition, szRemainingSize);

    size_t szReadSize = fread(clsServerManager::pGlobalBuffer + szRemainingSize, 1, 131072 - szRemainingSize, pFile);

    if(szReadSize != (131072 - szRemainingSize)) {
        bFullRead = true;
    }

    pActualPosition = clsServerManager::pGlobalBuffer;
    szRemainingSize += szReadSize;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool PXBReader::ReadNextItem(const uint16_t * sExpectedIdentificators, const uint8_t &ui8ExpectedSubItems, const uint8_t &ui8ExtraSubItems/* = 0*/) {
    if(szRemainingSize == 0) {
        return false;
    }

    memset(&pItemDatas, 0, sizeof(pItemDatas));
    memset(&ui16ItemLengths, 0, sizeof(ui16ItemLengths));

    if(szRemainingSize < 4) {
        if(bFullRead == true) {
            return false;
        } else { // read next part of file
            ReadNextFilePart();

            if(szRemainingSize < 4) {
                return false;
            }
        }
    }

    uint32_t ui32ItemSize = ntohl(*((uint32_t *)pActualPosition));

    if(ui32ItemSize > szRemainingSize) {
        if(bFullRead == true) {
            return false;
        } else { // read next part of file
            ReadNextFilePart();

            if(ui32ItemSize > szRemainingSize) {
                return false;
            }
        }
    }

    pActualPosition += 4;
    szRemainingSize -= 4;
    ui32ItemSize -= 4;

    uint8_t ui8ActualItem = 0;

    uint16_t ui16SubItemSize = 0;

    while(ui32ItemSize > 0) {
        ui16SubItemSize = ntohs(*((uint16_t *)pActualPosition));

        if(ui16SubItemSize > ui32ItemSize) {
            return false;
        }

        if(ui8ActualItem < ui8ExpectedSubItems && sExpectedIdentificators[ui8ActualItem] == *((uint16_t *)(pActualPosition+2))) {
            ui16ItemLengths[ui8ActualItem] = (ui16SubItemSize - 4);
            pItemDatas[ui8ActualItem] = (pActualPosition + 4);

            ui8ActualItem++;
        } else { // for compatibility with newer versions...
            for(uint8_t ui8i = 0; ui8i < (ui8ExpectedSubItems + ui8ExtraSubItems); ui8i++) {
                if(sExpectedIdentificators[ui8i] == *((uint16_t *)(pActualPosition+2))) {
                    ui16ItemLengths[ui8i] = (ui16SubItemSize - 4);
                    pItemDatas[ui8i] = (pActualPosition + 4);

                    ui8ActualItem++;
                }
            }
        }

        pActualPosition += ui16SubItemSize;
        szRemainingSize -= ui16SubItemSize;
        ui32ItemSize -= ui16SubItemSize;
    }

    if(ui8ActualItem != ui8ExpectedSubItems) {
        return false;
    } else {
        return true;
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool PXBReader::OpenFileSave(const char * sFilename) {
    pFile = fopen(sFilename, "wb");

    if(pFile == NULL) {
        return false;
    }

    szRemainingSize = 131072;

    pActualPosition = clsServerManager::pGlobalBuffer;

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool PXBReader::WriteNextItem(const uint32_t &ui32Length, const uint8_t &ui8SubItems) {
    uint32_t ui32ItemLength = ui32Length + 4 + (4 * ui8SubItems);

    if(ui32ItemLength > szRemainingSize) {
        fwrite(clsServerManager::pGlobalBuffer, 1, pActualPosition-clsServerManager::pGlobalBuffer, pFile);
        pActualPosition = clsServerManager::pGlobalBuffer;
        szRemainingSize = 131072;
    }

    (*((uint32_t *)pActualPosition)) = htonl(ui32ItemLength);
    pActualPosition += 4;
    szRemainingSize -= 4;

    for(uint8_t ui8i = 0; ui8i < ui8SubItems; ui8i++) {
        (*((uint16_t *)(pActualPosition))) = htons(ui16ItemLengths[ui8i] + 4);

        pActualPosition[2] = sItemIdentifiers[ui8i][0];
        pActualPosition[3] = sItemIdentifiers[ui8i][1];

        switch(ui8ItemValues[ui8i]) {
            case PXB_BYTE:
                pActualPosition[4] = (pItemDatas[ui8i] == 0 ? '0' : '1');
                break;
            case PXB_TWO_BYTES:
                (*((uint16_t *)(pActualPosition+4))) = htons(*((uint16_t *)pItemDatas[ui8i]));
                break;
            case PXB_FOUR_BYTES:
                (*((uint32_t *)(pActualPosition+4))) = htonl(*((uint32_t *)pItemDatas[ui8i]));
                break;
            case PXB_STRING:
                memcpy(pActualPosition+4, pItemDatas[ui8i], ui16ItemLengths[ui8i]);
                break;
            default:
                break;
        }

        pActualPosition += ui16ItemLengths[ui8i] + 4;
        szRemainingSize -= ui16ItemLengths[ui8i] + 4;
    }

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void PXBReader::WriteRemaining() {
    if((pActualPosition-clsServerManager::pGlobalBuffer) > 0) {
        fwrite(clsServerManager::pGlobalBuffer, 1, pActualPosition-clsServerManager::pGlobalBuffer, pFile);
    }

    fclose(pFile);
    pFile = NULL;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
