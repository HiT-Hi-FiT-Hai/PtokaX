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
#include "ClientTagManager.h"
//---------------------------------------------------------------------------
#include "utility.h"
//---------------------------------------------------------------------------
ClientTagMan *ClientTagManager = NULL;
//---------------------------------------------------------------------------

ClientTagMan::ClientTagMan() {
    cliTags = (ClientTag *) calloc(256, sizeof(ClientTag));
    if(cliTags == NULL) {
		string sDbgstr = "[BUF] Cannot allocate cliTags!";
		AppendSpecialLog(sDbgstr);
    	exit(EXIT_FAILURE);
    }

    Load();
}
//---------------------------------------------------------------------------
	
ClientTagMan::~ClientTagMan() {
    free(cliTags);
    cliTags = NULL;
}
//---------------------------------------------------------------------------

void ClientTagMan::Load() {
	TiXmlDocument doc((PATH+"/cfg/ClientTags.xml").c_str());
    if(doc.LoadFile()) {
        TiXmlHandle cfg(&doc);
        TiXmlNode *clienttags = cfg.FirstChild("ClientTags").Node();
		if(clienttags != NULL) {
        	int i = 0;
			TiXmlNode *child = NULL;
			while((child = clienttags->IterateChildren(child)) != NULL) {
				TiXmlNode *clienttag = child->FirstChild("ClientTag");

				if(clienttag == NULL ||
                    (clienttag = clienttag->FirstChild()) == NULL) {
					continue;
				}

                const char *patt = clienttag->Value();
                
				if((clienttag = child->FirstChild("ClientName")) == NULL ||
                    (clienttag = clienttag->FirstChild()) == NULL) {
					continue;
				}

                const char *name = clienttag->Value();
                
				strcpy(cliTags[i].TagPatt, patt);
				cliTags[i].PattLen = strlen(cliTags[i].TagPatt);
				strcpy(cliTags[i].CliName, name);
				i++;
            }
        }
    } else {
		TiXmlDocument doc((PATH+"/cfg/ClientTags.xml").c_str());
		doc.InsertEndChild(TiXmlDeclaration("1.0", "windows-1252", "yes"));
		TiXmlElement clienttags("ClientTags");
		const char* ClientPatts[] = { "++", "DCGUI", "DC", "oDC", "DC:PRO", "QuickDC", "LDC++", "R2++", "Goofy++", "PWDC++", "BDC++", "zK++" };
		const char* ClientNames[] = { "DC++", "Valknut", "NMDC2", "oDC", "DC:PRO", "QuickDC", "LDC++", "R2++", "Goofy++", "PWDC++", "BDC++", "zK++" };
		for(unsigned int i = 0; i < 12; i++) {
			TiXmlElement clientpatt("ClientTag");
			clientpatt.InsertEndChild(TiXmlText(ClientPatts[i]));

			TiXmlElement clientname("ClientName");
			clientname.InsertEndChild(TiXmlText(ClientNames[i]));

			TiXmlElement clienttag("Client");
			clienttag.InsertEndChild(clientpatt);
			clienttag.InsertEndChild(clientname);

			clienttags.InsertEndChild(clienttag);

			strcpy(cliTags[i].TagPatt, ClientPatts[i]);
			cliTags[i].PattLen = strlen(cliTags[i].TagPatt);
			strcpy(cliTags[i].CliName, ClientNames[i]);
		}
		doc.InsertEndChild(clienttags);
		doc.SaveFile();
    }
}
//---------------------------------------------------------------------------
