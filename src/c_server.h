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


// Beispiele:
// https://github.com/spacehuhn/wifi_ducky/blob/master/esp8266_wifi_duck/esp8266_wifi_duck.ino
// WebSocketClient: https://github.com/Links2004/arduinoWebSockets/issues/119


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
void server_setup() {

  //MDNS.begin(sys.host.c_str());  // siehe Beispiel: WiFi.hostname(host); WiFi.softAP(host);
  // verschoben in c_wifi

    
  server.addHandler(&nanoWebHandler);
  server.addHandler(&bodyWebHandler);
    
  server.on("/help",HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("https://github.com/WLANThermo-nano/WLANThermo_nano_Software/blob/master/README.md");
  }).setFilter(ON_STA_FILTER);
    
  server.on("/info",[](AsyncWebServerRequest *request){
    FSInfo fs_info;
    SPIFFS.info(fs_info);
    String ssidstr;
    for (int i = 0; i < wifi.savedlen; i++) {
        ssidstr += " ";
        ssidstr += String(i+1);
        ssidstr += ": "; 
        ssidstr += wifi.savedssid[i];
    }
    
    request->send(200,"","bytes: " + String(fs_info.usedBytes) + " | " + String(fs_info.totalBytes) + "\n"
      +"heap: "      + String(ESP.getFreeHeap()) + "\n"
      +"sn: "        + String(ESP.getChipId(), HEX) + "\n"
      +"pn: "        + sys.item + "\n"
      +"batlimit: "+String(battery.min) + " | " + String(battery.max) + "\n"
      +"bat: "       + String(battery.voltage) + " | " + String(battery.sim) + " | " + String(battery.simc) + "\n"
      +"batstat: "  + String(battery.state) + " | " +String(battery.setreference) + "\n"
      +"ssid: "     + ssidstr + "\n"
      +"wifimode: " + String(WiFi.getMode()) + "\n"
      +"mac:" + String(getMacAddress()) + "\n"
      +"rS: "      + String(ESP.getFlashChipRealSize()) + "\n"
      +"iS: "      + String(ESP.getFlashChipSize())
      );
  });

  #ifdef AMPERE
  server.on("/ampere",[](AsyncWebServerRequest *request){
    ch[5].typ = 12;
    setconfig(eCHANNEL,{});
    request->send(200, TEXTPLAIN, TEXTTRUE);
  });


  server.on("/ohm",[](AsyncWebServerRequest *request){
    ch[0].typ = 13;
    setconfig(eCHANNEL,{});
    request->send(200, TEXTPLAIN, TEXTTRUE);
  });
  #endif

  server.on("/setbattmin",[](AsyncWebServerRequest *request){
    battery.min -= 100;
    setconfig(eSYSTEM,{});
    request->send(200, TEXTPLAIN, "Done");
  });

  server.on("/servo",[](AsyncWebServerRequest *request){          // brauchen wir für alte Nanos
    set_pid(1);
    setconfig(ePIT,{});
    request->send(200, TEXTPLAIN, TEXTADD);
  });

  server.on("/stop",[](AsyncWebServerRequest *request){
    pitMaster[0].active = PITOFF;
    setconfig(ePIT,{});
    request->send(200, TEXTPLAIN, "Stop pitmaster");
  });

  server.on("/pbq",[](AsyncWebServerRequest *request){
    sys.god ^= (1<<5);
    setconfig(eSYSTEM,{});
    request->send(200, TEXTPLAIN, String((sys.god & (1<<5))?1:0));
  });

  server.on("/clientlog",[](AsyncWebServerRequest *request){
    sys.clientlog = true;
    update.state = -1;
    request->send(200, TEXTPLAIN, "aktiviert");
  });

  server.on("/restart",[](AsyncWebServerRequest *request){
    sys.restartnow = true;
    request->redirect("/");
  }).setFilter(ON_STA_FILTER);
   

  server.on("/newtoken",[](AsyncWebServerRequest *request){
    ESP.wdtDisable(); 
    iot.CL_token = newToken();
    setconfig(eTHING,{});
    lastUpdateCloud = 1; // Daten senden forcieren
    ESP.wdtEnable(10);
    request->send(200, TEXTPLAIN, iot.CL_token);
  });

/*
  server.on("/validateUser",[](AsyncWebServerRequest *request) { 
      if(request->hasParam("user")&&request->hasParam("passwd")){
        ESP.wdtDisable(); 
        String _user = request->getParam("user")->value();
        String _pw = request->getParam("passwd")->value();
        ESP.wdtEnable(10);
        if (_user == sys.www_username && _pw == sys.www_password)
          request->send(200, TEXTPLAIN, TEXTTRUE);
        else
          request->send(200, TEXTPLAIN, TEXTFALSE);
      } else request->send(200, TEXTPLAIN, TEXTFALSE);
  });
*/
 
  // to avoid multiple requests to ESP
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html"); // gibt alles im Ordner frei
    
  // 404 NOT found: called when the url is not defined here
  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404);
  });

  #ifdef WEBSOCKET
  setWebSocket();
  #endif
      
  server.begin();
  IPRINTPLN("HTTP server started");
  //MDNS.addService("http", "tcp", 80);  //verschoben in c_wifi.h
}

