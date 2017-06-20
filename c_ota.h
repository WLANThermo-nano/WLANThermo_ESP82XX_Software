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
// Check if there is http update
void check_http_update() {

  if((wifiMulti.run() == WL_CONNECTED && sys.autoupdate)) {
    HTTPClient http;

    String adress = F("http://nano.wlanthermo.de/update.php?software=");
    adress += FIRMWAREVERSION;
    adress += F("&hardware=v");
    adress += String(sys.hwversion);
    adress += F("&serial=");
    adress += String(ESP.getChipId(), HEX);
    adress += F("&checkUpdate=true");

    DPRINTPLN("[INFO]\tCheck HTTP Update");

    http.begin(adress); //HTTP
       
    int httpCode = http.GET();

    // httpCode will be negative on error
    if(httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      DPRINTF("[HTTP]\tGET... code: %d\n", httpCode);

      // file found at server
      if(httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        DPRINTP("[HTTP]\tReceive: ");
        DPRINTLN(payload);
        sys.getupdate = payload;
      }
    } else {
      DPRINTF("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      sys.getupdate = "false";
    }
    http.end();
  } else sys.getupdate = "false";
  if (sys.update == -1) sys.update = 0;
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Do http update
void do_http_update() {
  
  if((wifiMulti.run() == WL_CONNECTED)) {

    if (sys.update == 3){
      sys.update = 0;
      modifyconfig(eSYSTEM,{});
      displayblocked = true;
      question.typ = OTAUPDATE;
      drawQuestion(0);
      DPRINTPLN("[INFO]\tUPDATE FINISHED");
      return;
    }

    String adress = F("http://nano.wlanthermo.de/update.php?software=");
    adress += FIRMWAREVERSION;
    adress += F("&hardware=v");
    adress += String(sys.hwversion);
    adress += F("&serial=");
    adress += String(ESP.getChipId(), HEX);

    if (sys.updatecount < 2) sys.updatecount++;   // eine Wiederholung
    else  {
      sys.update = 0;
      modifyconfig(eSYSTEM,{});
      DPRINTPLN("[INFO]\tUPDATE_CANCELED");
      displayblocked = false;
      sys.updatecount = 0;
      return;
    }

    displayblocked = true;
    t_httpUpdate_return ret;
    
    if (sys.update == 1) {
      sys.update = 2;  // Nächster Updatestatus
      drawUpdate("Webinterface");
      modifyconfig(eSYSTEM,{});                                      // SPEICHERN
      DPRINTPLN("[INFO]\tDo SPIFFS Update ...");
      ret = ESPhttpUpdate.updateSpiffs(adress + "&getcurrentSpiffs=true");
    
    } else if (sys.update == 2) {
      sys.update = 3;
      drawUpdate("Firmware");
      modifyconfig(eSYSTEM,{});                                      // SPEICHERN
      DPRINTPLN("[INFO]\tDo Firmware Update ...");
      ret = ESPhttpUpdate.update(adress + "&getcurrentFirmware=true");
    
    } 
    
    switch(ret) {
        case HTTP_UPDATE_FAILED:
          DPRINTF("[HTTP]\tUPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          DPRINTPLN("");
          if (sys.update == 2) sys.update = 1;  // Spiffs wiederholen
          else  sys.update = 2;                 // Firmware wiederholen
          //modifyconfig(eSYSTEM,{});
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
  }
}



