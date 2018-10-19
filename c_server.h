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
// Notification JSON Object
void noteObj(JsonObject  &jObj) {

  jObj["task"] = "Alert";

  // Kanäle & Message
  JsonArray& _ch = jObj.createNestedArray("channels");
  JsonArray& _message = jObj.createNestedArray("messages");

  for (int i = 0; i < CHANNELS; i++) {
    if (ch[i].alarm == 1 || ch[i].alarm == 3){      // push or all
      if (notification.index & (1<<i)) {            // ALARM AT CHANNEL i
        _ch.add(i+1);                               // Füge Kanal hinzu
        _message.add(notification.limit & (1<<i));  // Füge Art hinzu
      }
    }
  }

  // an welche Dienste soll geschickt werden?
  JsonArray& services = jObj.createNestedArray("services");
/*
  if (notification.temp1.length() == 30 || notification.token.length() == 30) {
    JsonObject& _obj2 = services.createNestedObject();
    _obj2["service"] =  "pushover";
    if (notification.temp1.length() == 30) {
      _obj1["key1"] =  notification.temp1;
      _obj2["key2"] =  notification.temp2;
    } else {
      _obj1["key1"] =  notification.token;
      _obj2["key2"] =  notification.id;
    }
  
  } else if (notification.temp1.length() == 40 || notification.token.length() == 40) {
    JsonObject& _obj3 = services.createNestedObject();
    _obj3["service"] =  "prowl";
  } else  
  
    JsonObject& _obj1 = services.createNestedObject();
    _obj1["service"] =  "telegram";
    _obj1["key1"] =  "xxx";
    _obj1["key2"] =  "xxx";
    
    JsonObject& _obj3 = services.createNestedObject();
    _obj3["service"] =  "mail";
    _obj3["adress"] =  "xxx";
 */ 
  
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
String cloudData(bool cloud, bool get_sys, uint8_t get_ch, uint8_t get_pit) {

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  if (get_sys) {
    JsonObject& system = root.createNestedObject("system");

    system["time"] = String(now());
    system["soc"] = battery.percentage;
    system["charge"] = battery.charge;
    system["rssi"] = wifi.rssi;
    system["unit"] = sys.unit;
    //system["item"] = sys.item;
    //system["sn"] = String(ESP.getChipId(), HEX);
    if (cloud) {
      system["serial"] = String(ESP.getChipId(), HEX);
      system["api_token"] = iot.CL_token; 
    }
  }

  if (get_ch) {
    JsonArray& channel = root.createNestedArray("channel");

    int j = 0;
    if (get_ch < CHANNELS) j = get_ch-1;
    //float temo[8] = {68.8, 67.4, 57.6, 110.3, 289.1, 16.1, 1, 1};

    for (int i = j; i < get_ch; i++) {
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
  }

  if (get_pit) {
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

      String sc[2] = {"#ff0000", "#FE2EF7"};
      String vc[2] = {"#000000", "#848484"};
      
      JsonArray& master = root.createNestedArray("pitmaster");

      for (int i = 0; i < get_pit; i++) {  // PITMASTERSIZE
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
    }
  }


    /*
    if (htu.exist()) {
      JsonObject& _htu = root.createNestedObject("htu");
      _htu["temp"] = htu.temp();
      _htu["hum"] = htu.hum();
    }
    */

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
String cloudSettings(bool get_sys, bool get_sen, uint8_t get_pid, bool get_akt, bool get_iot, bool get_not) {

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  
  if (get_sys) {
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
    if (sys.hwversion == 2)
      _system["hwversion"] =  String("V1+");
    else 
      _system["hwversion"] =  String("V")+String(sys.hwversion);
    //_system["advanced"] =  sys.advanced;

    JsonArray& _hw = root.createNestedArray("hardware");
    _hw.add(String("V")+String(1));
    if (sys.hwversion > 1) _hw.add(String("V1+"));

    JsonObject& api = root.createNestedObject("api");
    api["version"] = APIVERSION;
  }

  if (get_sen) {
    JsonArray& _typ = root.createNestedArray("sensors");
    for (int i = 0; i < SENSORTYPEN; i++) {
     _typ.add(ttypname[i]);
    }
  }

  if (get_pid) {
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
      _pid["opl"] =  pid[i].opl;
    }
  }

  if (get_akt) {
    JsonArray& _aktor = root.createNestedArray("aktor");
    _aktor.add("SSR");
    _aktor.add("FAN");
    _aktor.add("SERVO");
    if (sys.damper) _aktor.add("DAMPER"); 
  }

  if (get_iot) {
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
    _iot["CLon"] =      iot.CL_on;
    _iot["CLtoken"] =   iot.CL_token;
    _iot["CLint"] =     iot.CL_int;
  }
  
  if (get_not) {
    JsonObject& _note = root.createNestedObject("notes");
      
      JsonArray& _firebase = _note.createNestedArray("fcm");
      for (int i = 0; i < 3; i++) {
        JsonObject& _fcm = _firebase.createNestedObject();
        _fcm["id"] =    i;
        _fcm["on"] = (byte) true;
        //_fcm["token"] = "cerAGIyShJk:APA91bGX6XYvWm7W-KQN1FUw--IDiceKfKnpa0AZ3B2gNhldbkNkz7c1-Js0ma5QA8v2nBcZsf7ndPEWBGfRogHU6RzOI08IAhOyL5cXpUeAKDOTaO5O6XMHq89IHh8UaycRi4evFMbM";
        _fcm["pseudo"] = "AdminSamsung";
      }

    JsonObject& _ext = _note.createNestedObject("ext");
      _ext["on"]    =   pushd.on;
      _ext["token"] =   pushd.token;
      _ext["id"] =      pushd.id;
      _ext["repeat"] =  pushd.repeat;
      _ext["service"] = pushd.service;
    
      JsonArray& _noteservice = _ext.createNestedArray("services");
        _noteservice.add("telegram");   // 0
        _noteservice.add("pushover");   // 1
  }

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
    sys.pitsupply = true;
    setconfig(eSYSTEM,{});
    request->send(200, TEXTPLAIN, "v2");
  });

/*
  server.on("/temperatur",[](AsyncWebServerRequest *request){
    request->send(200, APPLICATIONJSON, cloudData(0,0,5,0));
  });
*/

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

/*
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
*/
  server.on("/newtoken",[](AsyncWebServerRequest *request){
    ESP.wdtDisable(); 
    iot.CL_token = newToken();
    setconfig(eTHING,{});
    lastUpdateCloud = 0; // Daten senden forcieren
    ESP.wdtEnable(10);
    request->send(200, TEXTPLAIN, iot.CL_token);
  });

  server.on("/setDC",[](AsyncWebServerRequest *request) { 
      if(request->hasParam("aktor")&&request->hasParam("dc")&&request->hasParam("val")){
        ESP.wdtDisable(); 
        bool dc = request->getParam("dc")->value().toInt();
        byte aktor = request->getParam("aktor")->value().toInt();
        int val = request->getParam("val")->value().toInt();        // Value * 10
        byte id = 0;  // Pitmaster1
        if (aktor == SERVO && sys.hwversion > 1) bodyWebHandler.setservoV2(true);
        if (val >= SERVOPULSMIN*10 && val <= SERVOPULSMAX*10 && aktor == SERVO) val = getDC(val);
        else val = constrain(val,0,1000);
        DC_start(dc, aktor, val, id);  
        IPRINTP("DC-Test: ");
        DPRINTLN(val/10.0);
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

  //setWebSocket();
      
  server.begin();
  IPRINTPLN("HTTP server started");
  MDNS.addService("http", "tcp", 80);
}

