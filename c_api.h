 /*************************************************** 
    Copyright (C) 2018  Steffen Ochs

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

    LITERATUR:
    - https://de.wikipedia.org/wiki/Liste_der_HTTP-Headerfelder
    
 ****************************************************/

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Device JSON Object - Send everytime when connect to API
void deviceObj(JsonObject  &jObj) {
  
  jObj["device"] = "nano";
  jObj["serial"] = String(ESP.getChipId(), HEX);
  
  if (sys.hwversion == 2) jObj["hw_version"] =  String("V1+");
  else  jObj["hw_version"] = String("V")+String(sys.hwversion);

  jObj["sw_version"] = FIRMWAREVERSION;
  jObj["api_version"] = SERVERAPIVERSION;
  //jObj["api_token"] = iot.CL_token;     // im CloudObj direkt
  jObj["language"] = sys.language;
  //system["item"] = sys.item;
       
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// System JSON Object
void systemObj(JsonObject  &jObj, bool settings = false) {

  jObj["time"]    = String(now());
  jObj["unit"]    = sys.unit;
  
  if (!settings) {
    jObj["soc"]     = battery.percentage;
    jObj["charge"]  = battery.charge;
    jObj["rssi"]    = wifi.rssi;
  } else {  
    jObj["ap"] =         sys.apname;
    jObj["host"] =       sys.host;
    jObj["language"] =   sys.language;
    jObj["fastmode"] =   sys.fastmode;
    jObj["version"] =    FIRMWAREVERSION;
    jObj["getupdate"] =  update.version;
    jObj["autoupd"] =    update.autoupdate;
    if (sys.hwversion == 2) jObj["hwversion"] =  String("V1+");
    else  jObj["hwversion"] =  String("V")+String(sys.hwversion);
    //jObj["advanced"] =  sys.advanced;
  }

}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Channel JSON Array
void channelAry(JsonArray  &jAry, int cc) {

  int j = 0;
  if (cc < CHANNELS) j = cc-1;    // nur ein Channel

  for (int i = j; i < cc; i++) {
  JsonObject& data = jAry.createNestedObject();
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

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Pitmaster JSON Object
void pitObj(JsonObject  &jObj) {
  
  jObj["channel"] = pitMaster[0].channel+1;
  jObj["pid"]     = pitMaster[0].pid;
  jObj["value"]   = (int)pitMaster[0].value;
  jObj["set"]     = pitMaster[0].set;
  switch (pitMaster[0].active) {
    case PITOFF:   jObj["typ"] = "off";    break;
    case DUTYCYCLE: // show manual
    case MANUAL:   jObj["typ"] = "manual"; break;
    case AUTO:     jObj["typ"] = "auto";   break;
    case AUTOTUNE: jObj["typ"] = "autotune"; break;
  }
  
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Pitmaster JSON Array
void pitAry(JsonArray  &jAry, int cc) {

  String sc[2] = {"#ff0000", "#FE2EF7"};
  String vc[2] = {"#000000", "#848484"};

  int j = 0;
  if (cc < PITMASTERSIZE) j = cc-1;    // nur ein bestimmter Pitmaster
      
  for (int i = j; i < cc; i++) {  // PITMASTERSIZE
    JsonObject& ma = jAry.createNestedObject();
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

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// PID JSON Array
void pidAry(JsonArray  &jAry, int cc) {

  int j = 0;
  if (cc < pidsize) j = cc-1;    // nur ein bestimmtes PID-Profil
      
  for (int i = j; i < cc; i++) {  // pidsize
    JsonObject& _pid = jAry.createNestedObject();
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

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// IoT JSON Object
void iotObj(JsonObject  &jObj) {
  
  jObj["TSwrite"] =   iot.TS_writeKey; 
  jObj["TShttp"] =    iot.TS_httpKey;
  jObj["TSuser"] =    iot.TS_userKey;
  jObj["TSchID"] =    iot.TS_chID;
  jObj["TSshow8"] =   iot.TS_show8;
  jObj["TSint"] =     iot.TS_int;
  jObj["TSon"] =      iot.TS_on;
  jObj["PMQhost"] =   iot.P_MQTT_HOST;
  jObj["PMQport"] =   iot.P_MQTT_PORT;
  jObj["PMQuser"] =   iot.P_MQTT_USER;
  jObj["PMQpass"] =   iot.P_MQTT_PASS;
  jObj["PMQqos"] =    iot.P_MQTT_QoS;
  jObj["PMQon"] =     iot.P_MQTT_on;
  jObj["PMQint"] =    iot.P_MQTT_int;
  jObj["CLon"] =      iot.CL_on;
  jObj["CLtoken"] =   iot.CL_token;
  jObj["CLint"] =     iot.CL_int;

}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// FCM JSON Array
void fcmAry(JsonArray  &jAry, int cc = 3) {

  int j = 0;
  if (cc < 3) j = cc-1;    // nur ein bestimmtes Profil
      
  for (int i = j; i < cc; i++) {
    JsonObject& _fcm = jAry.createNestedObject();
    _fcm["id"] =    i;
    _fcm["on"] = (byte) true;
    //_fcm["token"] = "cerAGIyShJk:APA91bGX6XYvWm7W-KQN1FUw--IDiceKfKnpa0AZ3B2gNhldbkNkz7c1-Js0ma5QA8v2nBcZsf7ndPEWBGfRogHU6RzOI08IAhOyL5cXpUeAKDOTaO5O6XMHq89IHh8UaycRi4evFMbM";
    _fcm["pseudo"] = "AdminSamsung";
  }
 
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Update JSON Object
void extObj(JsonObject  &jObj) {
  
  jObj["on"]    =   pushd.on;
  jObj["token"] =   pushd.token;
  jObj["id"] =      pushd.id;
  jObj["repeat"] =  pushd.repeat;
  jObj["service"] = pushd.service;
    
   JsonArray& _noteservice = jObj.createNestedArray("services");
   _noteservice.add("telegram");   // 0
   _noteservice.add("pushover");   // 1
   
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Update JSON Object
void updateObj(JsonObject  &jObj) {
  
  jObj["available"] = true;
  
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Alexa JSON Object
void alexaObj(JsonObject  &jObj) {
  
  jObj["task"] = "save";    // save or delete
  jObj["token"] = "xxx";
  
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// URL JSON Object
void urlObj(JsonObject  &jObj) {

  for (int i = 0; i < NUMITEMS(serverurl); i++) {
  
    JsonObject& _obj = jObj.createNestedObject(servertyp[i]);
    _obj["host"] =  serverurl[i].host;
    _obj["page"] =  serverurl[i].page;
  }
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// DATA JSON Object
void dataObj(JsonObject  &jObj, bool cloud) {

  // SYSTEM
  JsonObject& _system = jObj.createNestedObject("system");
  systemObj(_system);

  // CHANNEL
  JsonArray& _channel = jObj.createNestedArray("channel");
  channelAry(_channel, CHANNELS);

  // PITMASTER
  if (cloud) {
    JsonObject& _master = jObj.createNestedObject("pitmaster");
    pitObj(_master);
  } else {    
    JsonArray& _master = jObj.createNestedArray("pitmaster");
    pitAry(_master, PITMASTERSIZE);
  }
  
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// SETTINGS JSON Object
void settingsObj(JsonObject  &jObj) {

  // SYSTEM
  JsonObject& _system = jObj.createNestedObject("system");
  systemObj(_system, true);

  JsonArray& _hw = jObj.createNestedArray("hardware");
  _hw.add(String("V1"));
  if (sys.hwversion > 1) _hw.add(String("V1+"));

  JsonObject& api = jObj.createNestedObject("api");
  api["version"] = APIVERSION;

  // SENSORS
  JsonArray& _typ = jObj.createNestedArray("sensors");
  for (int i = 0; i < SENSORTYPEN; i++) {
    _typ.add(ttypname[i]);
  }
  
  // PID-PROFILS
  JsonArray& _pid = jObj.createNestedArray("pid");
  pidAry(_pid, pidsize);

  // AKTORS
  JsonArray& _aktor = jObj.createNestedArray("aktor");
  _aktor.add("SSR");
  _aktor.add("FAN");
  _aktor.add("SERVO");
  if (sys.damper) _aktor.add("DAMPER"); 
  
  // IOT
  JsonObject& _iot = jObj.createNestedObject("iot");
  iotObj(_iot);

  // NOTES
  JsonObject& _note = jObj.createNestedObject("notes");  
    JsonArray& _firebase = _note.createNestedArray("fcm");
    //fcmAry(_note);
    
    JsonObject& _ext = _note.createNestedObject("ext");
    extObj(_ext);  
  
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CLOUD JSON Object - Level 1
void cloudObj(JsonObject  &jObj) {

  jObj["task"] = "save";
  jObj["api_token"] = iot.CL_token; 

  JsonArray& data = jObj.createNestedArray("data");

 #ifdef MEMORYCLOUD
  if (cloudcount > 0) {
    long cur = now();

    for (int i = 0; i < cloudcount; i++) {
      JsonObject& _obj = data.createNestedObject();
      parseLog(_obj, i, (cur-((cloudcount-i)*3)));  
    }

    cloudcount = 0;
  }
  #endif
  
  //for (int i = 0; i < 3; i++) {  
  JsonObject& _obj = data.createNestedObject();
  dataObj(_obj, true); 
  //}

}


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
// Hauptprogramm API - JSON Generator
String apiData(int typ) {

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  if (typ == APIDATA || typ == APISETTINGS) {
    // interne Kommunikation mit dem Webinterface
  } else {
    JsonObject& device = root.createNestedObject("device");
    deviceObj(device);
  }

  switch (typ) {

    case APIUPDATE: {
      JsonObject& update = root.createNestedObject("update");
      updateObj(update);
      
      JsonObject& url = root.createNestedObject("url");
      urlObj(url);
      break;
    }

    case APICLOUD: {
      JsonObject& cloud = root.createNestedObject("cloud");
      cloudObj(cloud);
      break;
    }

    case APIDATA: {
      dataObj(root, false);
      break;
    }

    case APISETTINGS: {
      settingsObj(root);
      break;
    }

    case APINOTE: {
      JsonObject& note = root.createNestedObject("notification");
      noteObj(note);
      break;
    }

    case APIALEXA: {
      JsonObject& alexa = root.createNestedObject("alexa");
      alexaObj(alexa);
      break;
    }
  }
  
  String jsonStr;
  root.printTo(jsonStr);
  
  return jsonStr;
}


// Bekommen wir das zusammengefasst in einen Client für Updatecheck, Cloud und Notes?

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
    printClient(serverurl[CLOUDLINK].page.c_str(),CLIENTERRROR);
    //printClient(SAVEDATALINK,CLIENTERRROR);
    DataClient = NULL;
    delete client;
  }, NULL);

  DataClient->onConnect([](void * arg, AsyncClient * client){

   DataClient->onError(NULL, NULL);

   client->onDisconnect([](void * arg, AsyncClient * c){
    printClient(serverurl[CLOUDLINK].page.c_str() ,DISCONNECT);
    //printClient(SAVEDATALINK ,DISCONNECT);
    DataClient = NULL;
    delete c;
   }, NULL);

   //send the request
   //printClient(SAVEDATALINK,SENDTO);
   //String message = cloudData(true);   
   //String adress = createCommand(POSTMETH,NOPARA,SAVEDATALINK,CLOUDSERVER,message.length());
   printClient(serverurl[CLOUDLINK].page.c_str(),SENDTO);
   String message = apiData(APICLOUD);  //cloudData(true);   //
   String adress = createCommand(POSTMETH,NOPARA,serverurl[CLOUDLINK].page.c_str(),serverurl[CLOUDLINK].host.c_str(),message.length());
   adress += message;

   client->write(adress.c_str());
   //Serial.println(adress);
      
  }, NULL);

  if(!DataClient->connect(serverurl[CLOUDLINK].host.c_str(), 80)){
   //printClient(SAVEDATALINK ,CONNECTFAIL);
   printClient(serverurl[CLOUDLINK].page.c_str() ,CONNECTFAIL);
   AsyncClient * client = DataClient;
   DataClient = NULL;
   delete client;
  }    
}



// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Read time stamp from HTTP Header
void readUTCfromHeader(String payload) {

  int index = payload.indexOf("Date: ");
  if (index > -1) {
            
    char date_string[27];
    for (int i = 0; i < 26; i++) {
      char c = payload[index+i+6];
      date_string[i] = c;
    }

    tmElements_t tmx;
    string_to_tm(&tmx, date_string);
    setTime(makeTime(tmx));

    IPRINTP("UTC: ");
    DPRINTLN(digitalClockDisplay(now()));
  }
}

int log_length; 

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Read content length from HTTP Header
void readContentLengthfromHeader(String payload, int len) {

  log_length = 0;
  int index = payload.indexOf("Content-Length: ");
  if (index > -1) {
           
    payload = payload.substring(index+16,len);            // "Content-Length:" entfernen     
    payload = payload.substring(0,payload.indexOf("\n")); // Ende der Zeile
    log_length = payload.toInt();
    Serial.println(log_length);
  }
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Read new location from 302 HTTP Header
void readLocation(String payload, int len) {

  int index = payload.indexOf("Location: ");
  if (index > -1) {
    payload = payload.substring(index+10,len);            // "Location" entfernen     
    payload = payload.substring(0,payload.indexOf("\n")); // Ende des Links
    Serial.println(payload);
            
    index = payload.indexOf("?");       // Eventuelle Anhänge entfernen
    if (index > -1) payload = payload.substring(0,index);
    len = payload.length();
    index = payload.indexOf("://");     // http entfernen
    if (index > -1) payload = payload.substring(index+3,len);
    index = payload.indexOf("/");

    if (index > -1) {
      serverurl[APILINK].host = payload.substring(0,index);
      serverurl[APILINK].page = payload.substring(index,len);
      setconfig(eSERVER,{});   // für Serverlinks
      sys.restartnow = true;
    }
  }
}

bool apicontent;
static AsyncClient * apiClient = NULL;

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Send to API
bool sendAPI(int check){

  if (check == 0) {
    if(apiClient) return false;                 //client already exists

    apiClient = new AsyncClient();
    if(!apiClient)  return false;               //could not allocate client

    apiClient->onError([](void * arg, AsyncClient * client, int error){
    //printClient(THINGHTTPLINK,CLIENTERRROR);
      apiClient = NULL;
      delete client;
    }, NULL);
    
  } else if (check == 2) {          // Senden ueber Server
    
    apiClient->onConnect([](void * arg, AsyncClient * client){
    
      printClient(serverurl[APILINK].page.c_str() ,CLIENTCONNECT);
      apicontent = false;
      
      apiClient->onError(NULL, NULL);
      
      client->onDisconnect([](void * arg, AsyncClient * c){
        printClient(serverurl[APILINK].page.c_str() ,DISCONNECT);
        apiClient = NULL;
        delete c;
      }, NULL);

      client->onData([](void * arg, AsyncClient * c, void * data, size_t len){

          //File configFile;
          String payload((char*)data);
          //Serial.println(payload);
          //Serial.println(len);

          if (payload.indexOf("HTTP/1.1") > -1) {             // Time Stamp
            readUTCfromHeader(payload);
          }
          
          if ((payload.indexOf("200 OK") > -1)) {             // 200 Header
            readContentLengthfromHeader(payload, len);
            apicontent = true;
            return;
          
          } else if (payload.indexOf("302 Found") > -1) {     // 302 Header: new API-Links 
            readLocation(payload, len);
          
          } else if (apicontent) {                            // Body: 1 part
            apicontent = false;
            bodyWebHandler.setServerURL((uint8_t*)data);      // ist das der komplette inhalt?

            //configFile = SPIFFS.open("/log.txt", "w");
            //configFile.print((char*)data);
            //configFile.close();
            
            log_length -= len;
            //Serial.println(log_length);
            
          } else if (log_length > 0) {                        // Body: current part

            //configFile = SPIFFS.open("/log.txt", "a");
            //configFile.print((char*)data);
            //configFile.close();
            
            log_length -= len;
            //Serial.println(log_length);
            
          }
           
      }, NULL);

      //send the request
      printClient(serverurl[APILINK].page.c_str() ,SENDTO);
      String message = apiData(APIUPDATE);
      String adress = createCommand(POSTMETH,NOPARA,serverurl[APILINK].page.c_str(),serverurl[APILINK].host.c_str(),message.length());
      adress += message;
      client->write(adress.c_str());
      Serial.println(adress);

    }, NULL);

    if(!apiClient->connect(serverurl[APILINK].host.c_str(), 80)){
        printClient(serverurl[APILINK].page.c_str() ,CONNECTFAIL);
        AsyncClient * client = apiClient;
        apiClient = NULL;
        delete client;
    }
  }
  return true;    // Nachricht kann gesendet werden
}




// update.autoupdate einarbeiten

void check_api() {

  if (update.state == -1 || update.state == 2) {  // -1 = check, 2 = check after restart during Update
    if((wifi.mode == 1)) {
  
      if (sendAPI(0)) sendAPI(2);

    } else update.get = "false";
    
    if (update.state == -1) update.state = 0;         // von check (-1) auf ruhend (0) wechseln
    else if (update.state == 2) update.state = 3;     // Update fortführen nach Restart
    // kein Speichern im EE, Zustand -1 ist nur temporär
  } 
}




