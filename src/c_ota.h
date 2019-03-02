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


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// HTTP UPDATE


#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Do http update
void do_http_update() {

  // UPDATE beendet
  if (update.state == 4){
    question.typ = OTAUPDATE;
    drawQuestion(0);
    update.get = "false";
    update.state = 0;
    setconfig(eSYSTEM,{});  // Speichern
    update.state = -1;        // Neue Suche anstoßen
    IPRINTPLN("u:finish");  // Update finished
    return;
  }
  
  if((wifi.mode == 1)) {                 // nur bei STA
    if (update.get != "false") {

      // UPDATE Adresse
      String adress;

      if (update.state == 1 || update.state == 3) {              // nicht im Neuaufbau während Update
        // UPDATE 2x Wiederholen falls schief gelaufen
        if (update.count < 3) update.count++;   // Wiederholung
        else  {
          update.state = 0;
          setconfig(eSYSTEM,{});
          question.typ = OTAUPDATE;
          drawQuestion(0);
          IPRINTPLN("u:cancel");      // Update canceled
          displayblocked = false;
          update.count = 0;
          return;
        }
      }

      // UPDATE spiffs oder firmware
      displayblocked = true;
      t_httpUpdate_return ret;
    
      if (update.state == 1 && update.spiffsUrl != "") {  // erst wenn API abgefragt
        update.state = 2;  // Nächster Updatestatus
        drawUpdate("Webinterface");
        setconfig(eSYSTEM,{});                                      // SPEICHERN
        IPRINTPLN("u:SPIFFS ...");
        adress = update.spiffsUrl + adress;   // https://.... + adress
        Serial.println(adress);
        ret = ESPhttpUpdate.updateSpiffs(adress);

    
      } else if (update.state == 3 && update.firmwareUrl != "") {   // erst wenn API abgefragt
        update.state = 4;
        drawUpdate("Firmware");
        setconfig(eSYSTEM,{});                                      // SPEICHERN
        IPRINTPLN("u:FW ...");
        adress = update.firmwareUrl + adress;  // https://.... + adress
        Serial.println(adress);
        ret = ESPhttpUpdate.update(adress);
    
      } 

      // UPDATE Ereigniskontrolle
      switch(ret) {
        case HTTP_UPDATE_FAILED:
          DPRINTF("[HTTP]\tUPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          DPRINTPLN("");
          if (update.state == 2) update.state = 1;  // Spiffs wiederholen
          else  update.state = 3;                 // Firmware wiederholen
          //setconfig(eSYSTEM,{});
          drawUpdate("error");
          break;

        case HTTP_UPDATE_NO_UPDATES:
          DPRINTPLN("[HTTP]\tNO_UPDATES");
          displayblocked = false;
          break;

        case HTTP_UPDATE_OK:
          DPRINTPLN("[HTTP]\tUPDATE_OK");
          if (update.state == 2) ESP.restart();   // falls nach spiffs kein automatischer Restart durchgeführt wird
          break;
      }
    } else {
      if (update.state != 2) {    // nicht während Neustarts im Updateprozess
        IPRINTPLN("u:no");
        update.state = 0;   // Vorgang beenden
      }
    }
  }
}




/*
// FIRMWARE
#define FIRMWARESERVER "update.wlanthermo.de/getFirmware.php" 
// "nano.norma.uberspace.de/update/checkUpdate.php"
// SPIFFS
#define SPIFFSSERVER "update.wlanthermo.de/getSpiffs.php"   
// "nano.norma.uberspace.de/update/checkUpdate.php"
*/



