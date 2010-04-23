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

/*
 * "This application uses the IP-to-Country Database
 *  provided by WebHosting.Info (http://www.webhosting.info),
 *  available from http://ip-to-country.webhosting.info."
*/

//---------------------------------------------------------------------------
#include "stdinc.h"
//---------------------------------------------------------------------------
#include "IP2Country.h"
//---------------------------------------------------------------------------
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
//---------------------------------------------------------------------------
	#ifndef _MSC_VER
		#pragma package(smart_init)
	#endif
#endif
//---------------------------------------------------------------------------
IP2CC * IP2Country = NULL;
//---------------------------------------------------------------------------

static const char * CountryNames[] = { "Andorra", "United Arab Emirates", "Afghanistan", "Antigua and Barbuda", "Anguilla", "Albania", "Armenia",
	"Netherlands Antilles", "Angola", "Antarctica", "Argentina", "American Samoa", "Austria", "Australia",
	"Aruba", "A*Land Islands", "Azerbaijan", "Bosnia and Herzegovina", "Barbados", "Bangladesh", "Belgium",
	"Burkina Faso", "Bulgaria", "Bahrain", "Burundi", "Benin", "Saint Barthélemy", "Bermuda",
	"Brunei Darussalam", "Bolivia", "Brazil", "Bahamas", "Bhutan", "Bouvet Island", "Botswana",
	"Belarus", "Belize", "Canada", "Cocos (Keeling) Islands", "The Democratic Republic of the Congo", "Central African Republic", "Congo",
	"Côte D'Ivoire", "Cook Islands", "Chile", "Cameroon", "China", "Colombia", "Costa Rica",
	"Cuba", "Cape Verde", "Christmas Island", "Cyprus", "Czech Republic", "Germany", "Djibouti",
    "Denmark", "Dominica", "Dominican Republic", "Algeria", "Ecuador", "Estonia", "Egypt",
    "Western Sahara", "Eritrea", "Spain", "Ethiopia", "Finland", "Fiji", "Falkland Islands (Malvinas)",
    "Federated States of Micronesia", "Faroe Islands", "France", "Gabon", "United Kingdom", "Grenada", "Georgia",
    "French Guiana", "Guernsey", "Ghana", "Gibraltar", "Greenland", "Gambia", "Guinea",
    "Guadeloupe", "Equatorial Guinea", "Greece", "South Georgia and the South Sandwich Islands", "Guatemala", "Guam", "Guinea-Bissau",
    "Guyana", "Hong Kong", "Heard Island and McDonald Islands", "Honduras", "Croatia", "Haiti", "Hungary",
    "Switzerland", "Indonesia", "Ireland", "Israel", "Isle of Man", "India", "British Indian Ocean Territory",
    "Iraq", "Islamic Republic of Iran", "Iceland", "Italy", "Jersey", "Jamaica", "Jordan",
    "Japan", "Kenya", "Kyrgyzstan", "Cambodia", "Kiribati", "Comoros", "Saint Kitts and Nevis",
    "Democratic People's Republic of Korea", "Republic of Korea", "Kuwait", "Cayman Islands", "Kazakhstan", "Lao People's Democratic Republic", "Lebanon",
    "Saint Lucia", "Liechtenstein", "Sri Lanka", "Liberia", "Lesotho", "Lithuania", "Luxembourg",
    "Latvia", "Libyan Arab Jamahiriya", "Morocco", "Monaco", "Republic of Moldova", "Montenegro", "Saint Martin",
    "Madagascar", "Marshall Islands", "The Former Yugoslav Republic of Macedonia", "Mali", "Myanmar", "Mongolia", "Macao",
    "Northern Mariana Islands", "Martinique", "Mauritania", "Montserrat", "Malta", "Mauritius", "Maldives",
    "Malawi", "Mexico", "Malaysia", "Mozambique", "Namibia", "New Caledonia", "Niger",
    "Norfolk Island", "Nigeria", "Nicaragua", "Netherlands", "Norway", "Nepal", "Nauru",
    "Niue", "New Zealand", "Oman", "Panama", "Peru", "French Polynesia", "Papua New Guinea",
    "Philippines", "Pakistan", "Poland", "Saint Pierre and Miquelon", "Pitcairn", "Puerto Rico", "Palestinian Territory, Occupied",
    "Portugal", "Palau", "Paraguay", "Qatar", "Réunion", "Romania", "Serbia",
    "Russian Federation", "Rwanda", "Saudi Arabia", "Solomon Islands", "Seychelles", "Sudan", "Sweden",
    "Singapore", "Saint Helena", "Slovenia", "Svalbard and Jan Mayen", "Slovakia", "Sierra Leone", "San Marino",
    "Senegal", "Somalia", "Suriname", "Sao Tome and Principe", "El Salvador", "Syrian Arab Republic", "Swaziland",
    "Turks and Caicos Islands", "Chad", "French Southern Territories", "Togo", "Thailand", "Tajikistan", "Tokelau",
    "Timor-Leste", "Turkmenistan", "Tunisia", "Tonga", "Turkey", "Trinidad and Tobago", "Tuvalu",
    "Taiwan", "United Republic of Tanzania", "Ukraine", "Uganda", "United States Minor Outlying Islands", "United States", "Uruguay",
    "Uzbekistan", "Holy See (Vatican City State)", "Saint Vincent and the Grenadines", "Venezuela", "Virgin Islands, British", "Virgin Islands, U.S.", "Viet Nam",
    "Vanuatu", "Wallis and Futuna", "Samoa", "Yemen", "Mayotte", "South Africa", "Zambia",
    "Zimbabwe", "Unknown",
};

static const char * CountryCodes[] = { "AD", "AE", "AF", "AG", "AI", "AL", "AM", "AN", "AO", "AQ", "AR", "AS", "AT", "AU", "AW",
	"AX", "AZ", "BA", "BB", "BD", "BE", "BF", "BG", "BH", "BI", "BJ", "BL", "BM", "BN", "BO",
	"BR", "BS", "BT", "BV", "BW", "BY", "BZ", "CA", "CC", "CD", "CF", "CG", "CI", "CK", "CL",
	"CM", "CN", "CO", "CR", "CU", "CV", "CX", "CY", "CZ", "DE", "DJ", "DK", "DM", "DO", "DZ",
	"EC", "EE", "EG", "EH", "ER", "ES", "ET", "FI", "FJ", "FK", "FM", "FO", "FR", "GA", "GB",
	"GD", "GE", "GF", "GG", "GH", "GI", "GL", "GM", "GN", "GP", "GQ", "GR", "GS", "GT", "GU",
	"GW", "GY", "HK", "HM", "HN", "HR", "HT", "HU", "CH", "ID", "IE", "IL", "IM", "IN", "IO",
	"IQ", "IR", "IS", "IT", "JE", "JM", "JO", "JP", "KE", "KG", "KH", "KI", "KM", "KN", "KP",
	"KR", "KW", "KY", "KZ", "LA", "LB", "LC", "LI", "LK", "LR", "LS", "LT", "LU", "LV", "LY",
	"MA", "MC", "MD", "ME", "MF", "MG", "MH", "MK", "ML", "MM", "MN", "MO", "MP", "MQ", "MR",
	"MS", "MT", "MU", "MV", "MW", "MX", "MY", "MZ", "NA", "NC", "NE", "NF", "NG", "NI", "NL",
	"NO", "NP", "NR", "NU", "NZ", "OM", "PA", "PE", "PF", "PG", "PH", "PK", "PL", "PM", "PN",
	"PR", "PS", "PT", "PW", "PY", "QA", "RE", "RO", "RS", "RU", "RW", "SA", "SB", "SC", "SD",
	"SE", "SG", "SH", "SI", "SJ", "SK", "SL", "SM", "SN", "SO", "SR", "ST", "SV", "SY", "SZ",
	"TC", "TD", "TF", "TG", "TH", "TJ", "TK", "TL", "TM", "TN", "TO", "TR", "TT", "TV", "TW",
	"TZ", "UA", "UG", "UM", "US", "UY", "UZ", "VA", "VC", "VE", "VG", "VI", "VN", "VU", "WF",
	"WS", "YE", "YT", "ZA", "ZM", "ZW", "??",
};
//---------------------------------------------------------------------------

IP2CC::IP2CC() {
    ui32Count = 0;
    ui32Size = 65536;

#ifdef _WIN32
	FILE * ip2country = fopen((PATH + "\\cfg\\ip-to-country.csv").c_str(), "r");
#else
	FILE * ip2country = fopen((PATH + "/cfg/ip-to-country.csv").c_str(), "r");
#endif

    if(ip2country == NULL) {
		ui32RangeFrom = NULL;
        ui32RangeTo = NULL;
        ui8RangeCI = NULL;

        return;
    }

#ifdef _WIN32
    ui32RangeFrom = (uint32_t *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, ui32Size * sizeof(uint32_t));
#else
	ui32RangeFrom = (uint32_t *) calloc(ui32Size, sizeof(uint32_t));
#endif

    if(ui32RangeFrom == NULL) {
		AppendSpecialLog("Cannot create IP2CC::ui32RangeFrom!");
		return;
    }

#ifdef _WIN32
    ui32RangeTo = (uint32_t *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, ui32Size * sizeof(uint32_t));
#else
	ui32RangeTo = (uint32_t *) calloc(ui32Size, sizeof(uint32_t));
#endif

    if(ui32RangeTo == NULL) {
		AppendSpecialLog("Cannot create IP2CC::ui32RangeTo!");
		return;
    }

#ifdef _WIN32
    ui8RangeCI = (uint8_t *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, ui32Size * sizeof(uint8_t));
#else
	ui8RangeCI = (uint8_t *) calloc(ui32Size, sizeof(uint8_t));
#endif

    if(ui8RangeCI == NULL) {
		AppendSpecialLog("Cannot create IP2CC::ui8RangeCI!");
		return;
	}

    char sLine[1024];

    while(fgets(sLine, 1024, ip2country) != NULL) {
        if(sLine[0] != '\"') {
            continue;
        }

        if(ui32Count == ui32Size) {
            ui32Size += 512;

#ifdef _WIN32
			ui32RangeFrom = (uint32_t *) HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)ui32RangeFrom, ui32Size * sizeof(uint32_t));
#else
			ui32RangeFrom = (uint32_t *) realloc(ui32RangeFrom, ui32Size * sizeof(uint32_t));
#endif
            if(ui32RangeFrom == NULL) {
    			string sDbgstr = "[BUF] Cannot reallocate "+string(ui32Size)+
    				" bytes of memory in IP2CC::IP2CC for ui32RangeFrom!";
#ifdef _WIN32
    			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
    			AppendSpecialLog(sDbgstr);
                return;
    		}

#ifdef _WIN32
			ui32RangeTo = (uint32_t *) HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)ui32RangeTo, ui32Size * sizeof(uint32_t));
#else
			ui32RangeTo = (uint32_t *) realloc(ui32RangeTo, ui32Size * sizeof(uint32_t));
#endif
            if(ui32RangeTo == NULL) {
    			string sDbgstr = "[BUF] Cannot reallocate "+string(ui32Size)+
    				" bytes of memory in IP2CC::IP2CC for ui32RangeTo!";
#ifdef _WIN32
    			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
    			AppendSpecialLog(sDbgstr);
                return;
    		}

#ifdef _WIN32
            ui8RangeCI = (uint8_t *) HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)ui8RangeCI, ui32Size * sizeof(uint8_t));
#else
			ui8RangeCI = (uint8_t *) realloc(ui8RangeCI, ui32Size * sizeof(uint8_t));
#endif
            if(ui8RangeCI == NULL) {
    			string sDbgstr = "[BUF] Cannot reallocate "+string(ui32Size)+
    				" bytes of memory in IP2CC::IP2CC for ui8RangeCI!";
#ifdef _WIN32
    			sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
    			AppendSpecialLog(sDbgstr);
                return;
    		}
        }

        char * sStart = sLine+1;
        uint8_t ui8d = 0;
        
        size_t iLineLen = strlen(sLine);

		for(size_t ui16i = 1; ui16i < iLineLen; ui16i++) {
            if(sLine[ui16i] == '\"') {
                sLine[ui16i] = '\0';
                if(ui8d == 0) {
                    ui32RangeFrom[ui32Count] = strtoul(sStart, NULL, 10);
                } else if(ui8d == 1) {
                    ui32RangeTo[ui32Count] = strtoul(sStart, NULL, 10);
                } else {
                    for(uint8_t ui8i = 0; ui8i < 246; ui8i++) {
                        if(((uint16_t *)CountryCodes[ui8i])[0] == ((uint16_t *)sStart)[0]) {
                            ui8RangeCI[ui32Count] = ui8i;
                            break;
                        }
                    }

                    break;
                }

                ui8d++;
                ui16i += (uint16_t)2;
                sStart = sLine+ui16i+1;
                
            }
        }

        ui32Count++;
    }

	fclose(ip2country);
}
//---------------------------------------------------------------------------
	
IP2CC::~IP2CC() {
    if(ui32RangeFrom != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)ui32RangeFrom) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate IP2CC::ui32RangeFrom! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
            AppendSpecialLog(sDbgstr);
        }
#else
		free(ui32RangeFrom);
#endif
    }

    if(ui32RangeTo != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)ui32RangeTo) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate IP2CC::ui32RangeTo! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
            AppendSpecialLog(sDbgstr);
        }
#else
		free(ui32RangeTo);
#endif
    }

    if(ui8RangeCI != NULL) {
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)ui8RangeCI) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate IP2CC::ui8RangeCI! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
            AppendSpecialLog(sDbgstr);
        }
#else
		free(ui8RangeCI);
#endif
    }
}
//---------------------------------------------------------------------------

const char * IP2CC::Find(const uint32_t &ui32Hash, const bool &bCountryName) {
	uint32_t ui32i = 0;
	for(; ui32i < ui32Count; ui32i++) {
        if(ui32RangeFrom[ui32i] < ui32Hash && ui32RangeTo[ui32i] > ui32Hash) {
            if(bCountryName == false) {
                return CountryCodes[ui8RangeCI[ui32i]];
            } else {
                return CountryNames[ui8RangeCI[ui32i]];
            }
        }
    }

    if(bCountryName == false) {
        return CountryCodes[246];
    } else {
        return CountryNames[246];
    }
}
//---------------------------------------------------------------------------

uint8_t IP2CC::Find(const uint32_t &ui32Hash) {
	uint32_t ui32i = 0;
	for(; ui32i < ui32Count; ui32i++) {
        if(ui32RangeFrom[ui32i] < ui32Hash && ui32RangeTo[ui32i] > ui32Hash) {
            return ui8RangeCI[ui32i];
        }
    }

    return 246;
}
//---------------------------------------------------------------------------

const char * IP2CC::GetCountry(const uint8_t &ui8dx, const bool &bCountryName) {
    if(bCountryName == false) {
        return CountryCodes[ui8dx];
    } else {
        return CountryNames[ui8dx];
    }
}
//---------------------------------------------------------------------------
