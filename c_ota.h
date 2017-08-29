 /*************************************************** 
    Copyright (C) 2016  Steffen Ochs

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    HISTORY: Please refer Github History
    
 ****************************************************/

 /*
 * Example:
 *
 * Check for new update
 * http://update.wlanthermo.de/checkUpdate.php?device="nano"&serial="Serialnummer"&hw_version="v1"&sw_version="currentVersion"
 * ----------------------------------------------------------------------------------------------------------------------------------------
 * Download Firmware-version XYZ
 * http://update.wlanthermo.de/checkUpdate.php?device="nano"serial="Serialnummer"&hw_version="v1"&sw_version="currentVersion"&getFirmware="XYZ"
 * ----------------------------------------------------------------------------------------------------------------------------------------
 * Download Spiffs-version XYZ
 * http://update.wlanthermo.de/checkUpdate.php?device="nano"serial="Serialnummer"&hw_version="v1"&sw_version="currentVersion"&getSpiffs="XYZ"
 * ----------------------------------------------------------------------------------------------------------------------------------------
 */ 

#ifdef OTA

  #include <ArduinoOTA.h>           // OTA

  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Configuration OTA
  void set_ota(){

    ArduinoOTA.setHostname((const char *)sys.host.c_str());

    ArduinoOTA.onStart([]() {
      display.clear();
      display.setFont(ArialMT_Plain_10);
      display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
      display.drawString(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2 - 10, "OTA Update");
      display.display();
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      display.drawProgressBar(4, 32, 120, 8, progress / (total / 100) );
      display.display();
    });

    ArduinoOTA.onEnd([]() {
      display.clear();
      display.setFont(ArialMT_Plain_10);
      display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
      display.drawString(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2, "Restart");
      display.display();
    });

    ArduinoOTA.onError([](ota_error_t error) {
	    DPRINTF("Error[%u]: ", error);
	
		  switch (error) {
			  case OTA_AUTH_ERROR:
			    DPRINTPLN("Auth Failed");
				  break;
			  case OTA_BEGIN_ERROR:
				  DPRINTPLN("Connect Failed");
				  break;
			  case OTA_CONNECT_ERROR:
				  DPRINTPLN("Connect Failed");
				  break;
			  case OTA_RECEIVE_ERROR:
				  DPRINTPLN("Receive Failed");
				  break;
			  case OTA_END_ERROR:
				  DPRINTPLN("End Failed");
				  break;
			  default:
				  DPRINTPLN("OTA unknown ERROR");
				  break;
		  }
	  });
  }

#endif

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Do http update
void do_http_update() {

  // UPDATE beendet
  if (sys.update == 3){
    question.typ = OTAUPDATE;
    drawQuestion(0);
    sys.getupdate = "false";
    sys.update = 0;
    setconfig(eSYSTEM,{});
    sys.update = -1;   // Neue Suche anstoßen
    DPRINTPLN("[INFO]\tUPDATE FINISHED");
    return;
  }
  
  if((isAP == 0)) {
    if (sys.getupdate != "false") {

      // UPDATE Adresse
      String adress = F("http://update.wlanthermo.de/checkUpdate.php?");
      adress += createParameter(SERIALNUMBER);
      adress += createParameter(DEVICE);
      adress += createParameter(HARDWAREVS);
      adress += createParameter(SOFTWAREVS);

      // UPDATE 1x Wiederholen falls schief gelaufen
      if (sys.updatecount < 2) sys.updatecount++;   // eine Wiederholung
      else  {
        sys.update = 0;
        setconfig(eSYSTEM,{});
        question.typ = OTAUPDATE;
        drawQuestion(0);
        DPRINTPLN("[INFO]\tUPDATE_CANCELED");
        displayblocked = false;
        sys.updatecount = 0;
        return;
      }

      // UPDATE spiffs oder firmware
      displayblocked = true;
      t_httpUpdate_return ret;
    
      if (sys.update == 1) {
        sys.update = 2;  // Nächster Updatestatus
        drawUpdate("Webinterface");
        setconfig(eSYSTEM,{});                                      // SPEICHERN
        DPRINTPLN("[INFO]\tDo SPIFFS Update ...");
        ret = ESPhttpUpdate.updateSpiffs(adress + "&getSpiffs=" + sys.getupdate);

    
      } else if (sys.update == 2) {
        sys.update = 3;
        drawUpdate("Firmware");
        setconfig(eSYSTEM,{});                                      // SPEICHERN
        DPRINTPLN("[INFO]\tDo Firmware Update ...");
        ret = ESPhttpUpdate.update(adress + "&getFirmware=" + sys.getupdate);
    
      } 

      // UPDATE Ereigniskontrolle
      switch(ret) {
        case HTTP_UPDATE_FAILED:
          DPRINTF("[HTTP]\tUPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          DPRINTPLN("");
          if (sys.update == 2) sys.update = 1;  // Spiffs wiederholen
          else  sys.update = 2;                 // Firmware wiederholen
          //setconfig(eSYSTEM,{});
          drawUpdate("error");
          break;

        case HTTP_UPDATE_NO_UPDATES:
          DPRINTPLN("[HTTP]\tNO_UPDATES");
          displayblocked = false;
          break;

        case HTTP_UPDATE_OK:
          DPRINTPLN("[HTTP]\tUPDATE_OK");
          if (sys.update == 2) ESP.restart();   // falls nach spiffs kein automatischer Restart durchgeführt wird
          break;
      }
    } else {
      DPRINTPLN("[INFO]\tKein UPDATE vorhanden");
      sys.update = 0;   // Vorgang beenden
    }
  }
}


// see: https://github.com/me-no-dev/ESPAsyncTCP/issues/18
static AsyncClient * updateClient = NULL;

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Check if there is http update
void check_http_update() {

  if (sys.update < 1) {
    if((isAP == 0 && sys.autoupdate)) {

      if(updateClient) return;                 //client already exists

      updateClient = new AsyncClient();
      if(!updateClient)  return;               //could not allocate client

      updateClient->onError([](void * arg, AsyncClient * client, int error){
        DPRINTF("[HTTP] GET... failed, error: %s\n", updateClient->errorToString(error));
        updateClient = NULL;
        delete client;
        sys.getupdate = "false";
      }, NULL);

      updateClient->onConnect([](void * arg, AsyncClient * client){

        printClient(CHECKUPDATELINK ,CLIENTCONNECT);
        
        updateClient->onError(NULL, NULL);

        client->onDisconnect([](void * arg, AsyncClient * c){
          printClient(CHECKUPDATELINK ,DISCONNECT);
          updateClient = NULL;
          delete c;
        }, NULL);

        client->onData([](void * arg, AsyncClient * c, void * data, size_t len){
          
          String payload((char*)data);
          if (payload.indexOf("200 OK") > -1) {
        
            // Date
            int index = payload.indexOf("Date: ");
            
            char date_string[27];
            for (int i = 0; i < 26; i++) {
              char c = payload[index+i+6];
              date_string[i] = c;
            }

            tmElements_t tmx;
            string_to_tm(&tmx, date_string);
            setTime(makeTime(tmx));

            DPRINTP("[INFO]\tUTC: ");
            DPRINTLN(digitalClockDisplay(now()));
            
            // Update
            DPRINTP("[HTTP]\tGET: ");
            index = payload.indexOf("\r\n\r\n");       // Trennung von Header und Body
            payload = payload.substring(index+7,len);      // Beginn des Body
            index = payload.indexOf("\r");                 // Ende Versionsnummer
            payload = payload.substring(0,index);

            if (payload == "false") {
              DPRINTPLN("Kein Update");
              sys.getupdate = payload;
            }
            else if (payload.indexOf("v") == 0) {
              DPRINTLN(payload);
              sys.getupdate = payload;
            } else {
              DPRINTPLN("Fehler");
              sys.getupdate = "false";
            }
            setconfig(eSYSTEM,{});    // Speichern
          }
           
        }, NULL);

        //send the request
        String adress = createCommand(GETMETH,CHECKUPDATE,CHECKUPDATELINK,UPDATESERVER,0);
        client->write(adress.c_str());
        //Serial.println(adress);
    
      }, NULL);

      if(!updateClient->connect(UPDATESERVER, 80)){
        printClient(CHECKUPDATELINK ,CONNECTFAIL);
        AsyncClient * client = updateClient;
        updateClient = NULL;
        delete client;
      }
      
    
    } else sys.getupdate = "false";
    if (sys.update == -1) sys.update = 0;
    // kein Speichern im EE, Zustand -1 ist nur temporär
  } 
}



