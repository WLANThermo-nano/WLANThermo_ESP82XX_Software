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
String cloudData(bool cloud) {

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& system = root.createNestedObject("system");

    system["time"] = String(now());
    system["soc"] = battery.percentage;
    system["charge"] = battery.charge;
    system["rssi"] = wifi.rssi;
    system["unit"] = sys.unit;
    //system["sn"] = String(ESP.getChipId(), HEX);
    if (cloud) {
      system["serial"] = String(ESP.getChipId(), HEX);
      system["api_token"] = iot.CL_token; 
    }

    JsonArray& channel = root.createNestedArray("channel");

    //float temo[8] = {68.8, 67.4, 57.6, 110.3, 289.1, 16.1, 1, 1};

    for (int i = 0; i < CHANNELS; i++) {
    JsonObject& data = channel.createNestedObject();
      data["number"]= i+1;
      data["name"]  = ch[i].name;
      data["typ"]   = ch[i].typ;
      data["temp"]  = limit_float(ch[i].temp, i);  // temo[i];//
      data["min"]   = ch[i].min;
      data["max"]   = ch[i].max;
      data["alarm"] = ch[i].alarm;
      data["color"] = ch[i].color;
    }
/*
    if (cloud) {
      JsonObject& master = root.createNestedObject("pitmaster");

      master["channel"] = pitMaster[0].channel+1;
      master["pid"] = pitMaster[0].pid;
      master["value"] = (int)pitMaster[0].value;
      master["set"] = pitMaster[0].set;
      switch (pitMaster[0].active) {
        case PITOFF:   master["typ"] = "off";    break;
        case DUTYCYCLE: // show manual
        case MANUAL:   master["typ"] = "manual"; break;
        case AUTO:     master["typ"] = "auto";   break;
        case AUTOTUNE: master["typ"] = "autotune"; break;
      }
    } else { 

 */   

      String sc[2] = {"#ff0000", "#FE2EF7"};
      String vc[2] = {"#000000", "#848484"};
      
      JsonArray& master = root.createNestedArray("pitmaster");

      for (int i = 0; i < 1; i++) {  // PITMASTERSIZE
        JsonObject& ma = master.createNestedObject();
        ma["id"] = i;
        ma["channel"] = pitMaster[i].channel+1;
        ma["pid"] = pitMaster[i].pid;
        ma["value"] = (int)pitMaster[i].value;
        ma["set"] = pitMaster[i].set;
        switch (pitMaster[i].active) {
          case PITOFF:   ma["typ"] = "off";    break;
          case DUTYCYCLE: // show manual
          case MANUAL:   ma["typ"] = "manual"; break;
          case AUTO:     ma["typ"] = "auto";   break;
          case AUTOTUNE: ma["typ"] = "autotune"; break;
        } 
        ma["set_color"] = sc[i];
        ma["value_color"] = vc[i];
      }
  //  }

  


    
    if (htu.exist()) {
      JsonObject& _htu = root.createNestedObject("htu");
      _htu["temp"] = htu.temp();
      _htu["hum"] = htu.hum();
    }

    //JsonObject& api = root.createNestedObject("api");
    //api["version"] = APIVERSION;

    String jsonStr;
    root.printTo(jsonStr);
  
    return jsonStr;
}


static AsyncClient * DataClient = NULL;

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
void sendDataCloud() {

  if(DataClient) {
    AsyncClient * client = DataClient;
    DataClient = NULL;
    delete client;
    return;                 //client already exists
  }

  DataClient = new AsyncClient();
  if(!DataClient)  return;               //could not allocate client

  DataClient->onError([](void * arg, AsyncClient * client, int error){
    printClient(SAVEDATALINK,CLIENTERRROR);
    DataClient = NULL;
    delete client;
  }, NULL);

  DataClient->onConnect([](void * arg, AsyncClient * client){

   DataClient->onError(NULL, NULL);

   client->onDisconnect([](void * arg, AsyncClient * c){
    printClient(SAVEDATALINK ,DISCONNECT);
    DataClient = NULL;
    delete c;
   }, NULL);

   //send the request
   printClient(SAVEDATALINK,SENDTO);
   String message = cloudData(true);   
   String adress = createCommand(POSTMETH,NOPARA,SAVEDATALINK,CLOUDSERVER,message.length());
   adress += message;

   client->write(adress.c_str());
   //Serial.println(adress);
      
  }, NULL);

  if(!DataClient->connect(CLOUDSERVER, 80)){
   printClient(SAVEDATALINK ,CONNECTFAIL);
   AsyncClient * client = DataClient;
   DataClient = NULL;
   delete client;
  }    
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
String cloudSettings() {

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& _system = root.createNestedObject("system");

  _system["time"] =       String(now());
  _system["ap"] =         sys.apname;
  _system["host"] =       sys.host;
  _system["language"] =   sys.language;
  _system["unit"] =       sys.unit;
  _system["fastmode"] =   sys.fastmode;
  _system["version"] =    FIRMWAREVERSION;
  _system["getupdate"] =  sys.getupdate;
  _system["autoupd"] =    sys.autoupdate;
  _system["hwversion"] =  String("V")+String(sys.hwversion);
  //_system["advanced"] =  sys.advanced;
  
  JsonArray& _typ = root.createNestedArray("sensors");
  for (int i = 0; i < SENSORTYPEN; i++) {
    _typ.add(ttypname[i]);
  }

  JsonArray& _pit = root.createNestedArray("pid");
  for (int i = 0; i < pidsize; i++) {
    JsonObject& _pid = _pit.createNestedObject();
    _pid["name"] =    pid[i].name;
    _pid["id"] =      pid[i].id;
    _pid["aktor"] =   pid[i].aktor;
    _pid["Kp"] =      limit_float(pid[i].Kp, -1);
    _pid["Ki"] =      limit_float(pid[i].Ki, -1);
    _pid["Kd"] =      limit_float(pid[i].Kd, -1);
    _pid["Kp_a"] =    limit_float(pid[i].Kp_a, -1);
    _pid["Ki_a"] =    limit_float(pid[i].Ki_a, -1);
    _pid["Kd_a"] =    limit_float(pid[i].Kd_a, -1);
    _pid["DCmmin"] =  pid[i].DCmin;
    _pid["DCmmax"] =  pid[i].DCmax;
  }

  JsonArray& _aktor = root.createNestedArray("aktor");
  _aktor.add("SSR");
  _aktor.add("FAN");
  _aktor.add("SERVO");
  if (sys.damper) _aktor.add("DAMPER"); 

  JsonObject& _iot = root.createNestedObject("iot");
  _iot["TSwrite"] =   iot.TS_writeKey; 
  _iot["TShttp"] =    iot.TS_httpKey;
  _iot["TSuser"] =    iot.TS_userKey;
  _iot["TSchID"] =    iot.TS_chID;
  _iot["TSshow8"] =   iot.TS_show8;
  _iot["TSint"] =     iot.TS_int;
  _iot["TSon"] =      iot.TS_on;
  _iot["PMQhost"] =   iot.P_MQTT_HOST;
  _iot["PMQport"] =   iot.P_MQTT_PORT;
  _iot["PMQuser"] =   iot.P_MQTT_USER;
  _iot["PMQpass"] =   iot.P_MQTT_PASS;
  _iot["PMQqos"] =    iot.P_MQTT_QoS;
  _iot["PMQon"] =     iot.P_MQTT_on;
  _iot["PMQint"] =    iot.P_MQTT_int;
  _iot["TGon"]    =   iot.TG_on;
  _iot["TGtoken"] =   iot.TG_token;
  _iot["TGid"] =      iot.TG_id;
  _iot["CLon"] =      iot.CL_on;
  _iot["CLtoken"] =   iot.CL_token;
  _iot["CLint"] =     iot.CL_int;

  JsonArray& _hw = root.createNestedArray("hardware");
  _hw.add(String("V")+String(1));
  if (sys.hwversion > 1) _hw.add(String("V")+String(2));

  JsonArray& _noteservice = root.createNestedArray("notification");
  _noteservice.add("telegram");
  _noteservice.add("pushover");

  JsonObject& api = root.createNestedObject("api");
  api["version"] = APIVERSION;

  String jsonStr;
  root.printTo(jsonStr);
  
  return jsonStr;
  
}



// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
void server_setup() {

  MDNS.begin(sys.host.c_str());  // siehe Beispiel: WiFi.hostname(host); WiFi.softAP(host);
    
  server.addHandler(&nanoWebHandler);
  server.addHandler(&bodyWebHandler);
  //server.addHandler(&logHandler);
    
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
      +"batlimit: "+String(battery.min) + " | " + String(battery.max) + "\n"
      +"bat: "       + String(battery.voltage) + " | " + String(battery.sim) + " | " + String(battery.simc) + "\n"
      +"batstat: "  + String(battery.state) + " | " +String(battery.setreference) + "\n"
      +"ssid: "     + ssidstr + "\n"
      +"wifimode: " + String(WiFi.getMode()) + "\n"
      +"mac:" + String(getMacAddress())
      );
  });

  server.on("/setbattmax",[](AsyncWebServerRequest *request){
    if (request->method() == HTTP_GET) {
      request->send(200, "text/html", "<form method='POST' action='/setbattmax'>Maximale Batteriespannung in mV eingeben: <input type='number' name='battmax'><br><br><input type='submit' value='Speichern'></form>");
    } else if (request->method() == HTTP_POST) {
      if(!request->authenticate(sys.www_username, sys.www_password.c_str()))
        return request->requestAuthentication();
      if (request->hasParam("battmax", true)) { 
        int battmax = request->getParam("battmax", true)->value().toInt(); 
        battery.max = constrain(battmax,BATTMIN, 4200);
        setconfig(eSYSTEM,{});
        request->send(200, TEXTPLAIN, "Gespeichert");
      }
    } else request->send(500, TEXTPLAIN, BAD_PATH);
  });

  server.on("/nobattery",[](AsyncWebServerRequest *request){
    sys.god ^= (1<<1);    // XOR
    setconfig(eSYSTEM,{});
    if (sys.god & (1<<1)) request->send(200, TEXTPLAIN, TEXTON);
    else request->send(200, TEXTPLAIN, TEXTOFF);
  });

  server.on("/god",[](AsyncWebServerRequest *request){
    sys.god ^= (1<<0);    // XOR
    setconfig(eSYSTEM,{});
    if (sys.god & (1<<0)) request->send(200, TEXTPLAIN, TEXTON);
    else request->send(200, TEXTPLAIN, TEXTOFF);
  });

  server.on("/v2",[](AsyncWebServerRequest *request){
    sys.hwversion = 2;
    setconfig(eSYSTEM,{});
    request->send(200, TEXTPLAIN, "v2");
  });

  server.on("/pitsupply",[](AsyncWebServerRequest *request){
    if (sys.hwversion > 1 && !sys.pitsupply) {
      sys.pitsupply = true;
      setconfig(eSYSTEM,{});
      request->send(200, TEXTPLAIN, TEXTON);
    } else {
      sys.pitsupply = false;
      setconfig(eSYSTEM,{});
      request->send(200, TEXTPLAIN, TEXTOFF);
    }
  });

  server.on("/damper",[](AsyncWebServerRequest *request){
    if (request->method() == HTTP_GET) {
      request->send(200, "text/html", "<form method='POST' action='/damper'>Beim Hinzufuegen es Dampers werden die PID-Profile zurueckgesetzt: <br><br><input type='submit' value='Hinzufuegen'></form>");
    } else if (request->method() == HTTP_POST) {
      if(!request->authenticate(sys.www_username, sys.www_password.c_str()))
        return request->requestAuthentication();
      sys.damper = true;
      sys.hwversion = 2;  // Damper nur mit v2 Konfiguration
      set_pid(1);         // es wird ein Servo gebraucht
      setconfig(ePIT,{});
      setconfig(eSYSTEM,{});
      request->send(200, TEXTPLAIN, TEXTADD);
    } else request->send(500, TEXTPLAIN, BAD_PATH);
  });

  server.on("/servo",[](AsyncWebServerRequest *request){
    set_pid(1);
    setconfig(ePIT,{});
    request->send(200, TEXTPLAIN, TEXTADD);
  });

  server.on("/stop",[](AsyncWebServerRequest *request){
    //disableAllHeater();
    pitMaster[0].active = PITOFF;
    setconfig(ePIT,{});
    request->send(200, TEXTPLAIN, "Stop pitmaster");
  });

  server.on("/typk",[](AsyncWebServerRequest *request){
    if (sys.hwversion == 1 && !sys.typk) {
      sys.typk = true;
      set_sensor();
      request->send(200, TEXTPLAIN, TEXTON);
    } else {
      sys.typk = false;
      request->send(200, TEXTPLAIN, TEXTOFF);
    }
    setconfig(eSYSTEM,{});  // Speichern
  });
   
  server.on("/restart",[](AsyncWebServerRequest *request){
    sys.restartnow = true;
    request->redirect("/");
  }).setFilter(ON_STA_FILTER);

  server.on("/ampere",[](AsyncWebServerRequest *request){
    ch[5].typ = 11;
    setconfig(eCHANNEL,{});
    request->send(200, TEXTPLAIN, TEXTTRUE);
  });

  server.on("/ohm",[](AsyncWebServerRequest *request){
    ch[0].typ = 12;
    setconfig(eCHANNEL,{});
    request->send(200, TEXTPLAIN, TEXTTRUE);
  });

  server.on("/newtoken",[](AsyncWebServerRequest *request){
    ESP.wdtDisable(); 
    iot.CL_token = newToken();
    setconfig(eTHING,{});
    lastUpdateCloud = 0; // Daten senden forcieren
    ESP.wdtEnable(10);
    request->send(200, TEXTPLAIN, iot.CL_token);
  });
  
  server.on("/message",[](AsyncWebServerRequest *request) { 
      if(request->hasParam("token")&&request->hasParam("id")){
        ESP.wdtDisable(); 
        notification.temp1 = request->getParam("token")->value();
        notification.temp2 = request->getParam("id")->value();
        notification.type = 1;    // Verbindungstest
        ESP.wdtEnable(10);
        request->send(200, TEXTPLAIN, TEXTTRUE);
      } else request->send(200, TEXTPLAIN, TEXTFALSE);
  });

  server.on("/setDC",[](AsyncWebServerRequest *request) { 
      if(request->hasParam("aktor")&&request->hasParam("dc")&&request->hasParam("val")){
        ESP.wdtDisable(); 
        bool dc = request->getParam("dc")->value().toInt();
        byte aktor = request->getParam("aktor")->value().toInt();
        int val = request->getParam("val")->value().toInt();        // Value * 10
        byte id = 0;  // Pitmaster1
        if (val >= SERVOPULSMIN*10 && val <= SERVOPULSMAX*10 && aktor == SERVO) val = getDC(val);
        else val = constrain(val,0,1000);
        DC_start(dc, aktor, val, id);  
        IPRINTP("DC-Test: ");
        DPRINTLN(val/10.0);
        ESP.wdtEnable(10);
        request->send(200, TEXTPLAIN, TEXTTRUE);
      } else request->send(200, TEXTPLAIN, TEXTFALSE);
  });

  server.on("/autotune",[](AsyncWebServerRequest *request) { 
      if(request->hasParam("cycle")&&request->hasParam("over")&&request->hasParam("timelimit")){
        ESP.wdtDisable(); 
        long limit = request->getParam("timelimit")->value().toInt();
        int over = request->getParam("over")->value().toInt();
        int cycle = request->getParam("cycle")->value().toInt();
        byte id = 0;  // Pitmaster1
        startautotunePID(cycle, true, over, limit, id);
        ESP.wdtEnable(10);
        request->send(200, TEXTPLAIN, TEXTTRUE);
      } else request->send(200, TEXTPLAIN, TEXTFALSE);
  });

  server.on("/setadmin",[](AsyncWebServerRequest *request) { 
      if (request->method() == HTTP_GET) {
        request->send(200, "text/html", "<form method='POST' action='/setadmin'>Neues Password eingeben (max. 10 Zeichen): <input type='text' name='wwwpassword'><br><br><input type='submit' value='Change'></form>");
      } else if (request->method() == HTTP_POST) {
        if(!request->authenticate(sys.www_username, sys.www_password.c_str()))
          return request->requestAuthentication();
        if (request->hasParam("wwwpassword", true)) { 
          String password = request->getParam("wwwpassword", true)->value();
          if (password.length() < 11) {
            sys.www_password = password;
            setconfig(eSYSTEM,{});
            request->send(200, TEXTPLAIN, TEXTTRUE);
          }
          else request->send(200, TEXTPLAIN, TEXTFALSE);
        }
      } else request->send(500, TEXTPLAIN, BAD_PATH);

  });

  // to avoid multiple requests to ESP
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html"); // gibt alles im Ordner frei
    
  // 404 NOT found: called when the url is not defined here
  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404);
  });
      
  server.begin();
  IPRINTPLN("HTTP server started");
  MDNS.addService("http", "tcp", 80);
}

