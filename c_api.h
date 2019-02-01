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


    UPDATEPROZESS:
    - update.version wird von der API befüllt
    - update.get wird vom User oder GUI befüllt
    - wenn update.get == update.version, dann kann das Update direkt gestartet werden update.state = 1
    - wenn update.get unbekannt ist, dann zuerst über API anfragen mit update.state = -1
    - dabei muss das "udapte" JSON mit update.get befüllt werden
    - update.get wird im EE gespeichert und nach dem Neustart wieder aufgerufen
    - Dann wird speziell nach den Links von der Version in update.get gefragt


 ****************************************************/

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Device JSON Object - Send everytime when connect to API
void deviceObj(JsonObject  &jObj) {
  
  jObj["device"] = "nano";
  jObj["serial"] = String(ESP.getChipId(), HEX);
  if (sys.item != "") jObj["item"] = sys.item;
  
  //if (sys.hwversion == 2) jObj["hw_version"] =  String("v1+");
  //else  
  jObj["hw_version"] = String("v")+String(sys.hwversion);

  jObj["sw_version"] = FIRMWAREVERSION;
  jObj["api_version"] = SERVERAPIVERSION;
  jObj["language"] = sys.language;
        
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
    jObj["online"]  = sys.cloud_state;
  } else {  
    jObj["ap"] =         sys.apname;
    jObj["host"] =       sys.host;
    jObj["language"] =   sys.language;
    jObj["version"] =    FIRMWAREVERSION;
    jObj["getupdate"] =  update.version;
    jObj["autoupd"] =    update.autoupdate;
    if (sys.hwversion == 2) jObj["hwversion"] =  String("V1+");
    else  jObj["hwversion"] =  String("V")+String(sys.hwversion);
    jObj["god"] =        sys.god;
  }

}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Channel JSON Array
void channelAry(JsonArray  &jAry, int cc) {

  int i = 0;
  if (cc < sys.ch) i = cc-1;    // nur ein Channel

  for (i; i < cc; i++) {
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
// Pitmaster Types JSON Array
void pitTyp(JsonObject  &jObj) {

  JsonArray& _typ = jObj.createNestedArray("type");
  _typ.add("off");
  if(sys.pitmaster) {
    _typ.add("manual");
    _typ.add("auto");
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
    case AUTOTUNE: // show auto
    case AUTO:     jObj["typ"] = "auto";   break; 
  }
  
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Pitmaster JSON Array
void pitAry(JsonArray  &jAry, int cc) {

  String sc[2] = {"#ff0000", "#FE2EF7"};
  String vc[2] = {"#000000", "#848484"};

  int i = 0;
  if (cc < PITMASTERSIZE) i = cc-1;    // nur ein bestimmter Pitmaster
      
  for (i; i < cc; i++) {  // PITMASTERSIZE
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
      case AUTOTUNE: // show auto
      case AUTO:     ma["typ"] = "auto";   break;
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
    //_pid["Kp_a"] =    limit_float(pid[i].Kp_a, -1);
    //_pid["Ki_a"] =    limit_float(pid[i].Ki_a, -1);
    //_pid["Kd_a"] =    limit_float(pid[i].Kd_a, -1);
    _pid["DCmmin"] =  pid[i].DCmin;
    _pid["DCmmax"] =  pid[i].DCmax;
    _pid["opl"] =     pid[i].opl;
    _pid["tune"] =    pid[i].autotune;    // noch nicht im EE gespeichert
    _pid["jp"] =      pid[i].jumppw;
  }
 
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// IoT JSON Object
void iotObj(JsonObject  &jObj) {

  #ifdef THINGSPEAK
  jObj["TSwrite"] =   iot.TS_writeKey; 
  jObj["TShttp"] =    iot.TS_httpKey;
  jObj["TSuser"] =    iot.TS_userKey;
  jObj["TSchID"] =    iot.TS_chID;
  jObj["TSshow8"] =   iot.TS_show8;
  jObj["TSint"] =     iot.TS_int;
  jObj["TSon"] =      iot.TS_on;
  #endif
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
  jObj["CLurl"] =     "cloud.wlanthermo.de/index.html";

}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// FCM JSON Array
void fcmAry(JsonArray  &jAry, int cc = 3) {

  int i = 0;
  if (cc < 3) i = cc-1;    // nur ein bestimmtes Profil
      
  for (i; i < cc; i++) {
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

  // nur leeres Objekt, wird vom Server befüllt
  //jObj["available"] = true;

  // nach einer bestimmten Version fragen
  if (update.get != "false") jObj["version"] = update.get;
  
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

/*
  for (int i = 0; i < NUMITEMS(serverurl); i++) {
  
    JsonObject& _obj = jObj.createNestedObject(serverurl[i].typ);
    _obj["host"] =  serverurl[i].host;
    _obj["page"] =  serverurl[i].page;
  }
*/
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// DATA JSON Object
void dataObj(JsonObject  &jObj, bool cloud) {

  // SYSTEM
  JsonObject& _system = jObj.createNestedObject("system");
  systemObj(_system);

  // CHANNEL
  JsonArray& _channel = jObj.createNestedArray("channel");
  channelAry(_channel, sys.ch);

  //JsonObject& _master = jObj.createNestedObject("pitmaster");

  // PITMASTER  (Cloud kann noch kein Array verarbeiten)
  if (cloud) {
    //JsonObject& _master = jObj.createNestedObject("pitmaster");
   // pitObj(_master);
  if (sys.pitmaster) {
    JsonArray& _master = jObj.createNestedArray("pitmaster");
    pitAry(_master, PITMASTERSIZE);
  }
    
  } else {    
    JsonObject& _master = jObj.createNestedObject("pitmaster");
    pitTyp(_master);
    JsonArray& _pit = _master.createNestedArray("pm");
    pitAry(_pit, PITMASTERSIZE);
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
  api["version"] = GUIAPIVERSION;

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

  // History
  #ifdef MEMORYCLOUD
  if (cloudcount > 0) {
    long cur = now();
    cur -= (cloudcount)*(iot.CL_int/3);

    for (int i = 0; i < cloudcount; i++) {
      JsonObject& _obj = data.createNestedObject();
      parseLog(_obj, i, cur);
      cur += (iot.CL_int/3);  
    }

    cloudcount = 0;
  }
  #endif
  
  // aktuelle Werte  
  JsonObject& _obj = data.createNestedObject();
  dataObj(_obj, true); 

}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Notification JSON Object
void noteObj(JsonObject  &jObj) {

  jObj["lang"] = sys.language;

  if (iot.CL_on)
    jObj["api_token"] = iot.CL_token;

  // an welche Dienste soll geschickt werden?
  JsonArray& services = jObj.createNestedArray("services");

    JsonObject& _obj1 = services.createNestedObject();
    switch (pushd.service) {
      case 0: _obj1["service"] = F("telegram"); break;
      case 1: _obj1["service"] = F("pushover"); break;
      case 2: _obj1["service"] = F("prowl"); break;
    }
    _obj1["key1"] =  pushd.token;
    _obj1["key2"] =  pushd.id;

    // pushd.repeat;

  // Nachricht
  jObj["task"] = "alert"; 

  if (notification.type == 1) {
    jObj["message"] = "test";    
    if (pushd.on == 2) pushd.on = 3;    // alte Werte wieder herstellen  (Testnachricht)
    
  } else if (notification.type == 2) {
    jObj["message"] = "battery";
    
  } else {

    jObj["unit"] = sys.unit;
     
    /*
    // Kanäle & Message
    JsonArray& _ch = jObj.createNestedArray("channels");
    JsonArray& _message = jObj.createNestedArray("messages");

    for (int i = 0; i < sys.ch; i++) {
      Serial.println(ch[i].alarm);
      if (ch[i].alarm == 1 || ch[i].alarm == 3){      // push or all
      
        if (notification.index & (1<<i)) {            // ALARM AT CHANNEL i
          notification.index &= ~(1<<i);           // Kanal entfernen, sonst erneuter Aufruf
          _ch.add(i+1);                               // Füge Kanal hinzu
          bool limit = notification.limit & (1<<i);
          _message.add((limit)?F("up"):F("down"));
        }
      }
    }
    */

    bool limit = notification.limit & (1<<notification.ch);
    jObj["message"] = (limit)?F("up"):F("down");
    jObj["channel"] = notification.ch+1;
    notification.index &= ~(1<<notification.ch);           // Kanal entfernen, sonst erneuter Aufruf
    
    JsonArray& _temp = jObj.createNestedArray("temp");
    _temp.add(ch[notification.ch].temp);
    if (limit) 
      _temp.add(ch[notification.ch].max);
    else
      _temp.add(ch[notification.ch].min);
    


  }
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Hauptprogramm API - JSON Generator
String apiData(int typ) {

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  if (typ == APIDATA) {     //  || typ == APISETTINGS
    // interne Kommunikation mit dem Webinterface
  } else {
    JsonObject& device = root.createNestedObject("device");
    deviceObj(device);
  }

  switch (typ) {

    case NOAPI: {
      return "";
    }

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
      notification.type = 0;    // Zurücksetzen
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


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Read time stamp from HTTP Header
void readUTCfromHeader(String payload) {

  int index = payload.indexOf("Date: ");
  if (index > -1 && now() < 31536000) {  // Jahr 1971
            
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
int log_typ;

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Read content length from HTTP Header
void readContentLengthfromHeader(String payload, int len) {

  log_length = 0;
  int index = payload.indexOf("Content-Length: ");
  if (index > -1) {
     
    payload = payload.substring(index+16,len);            // "Content-Length:" entfernen     
    payload = payload.substring(0,payload.indexOf("\n")); // Ende der Zeile
    log_length = payload.toInt();
    
  }
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Check content typ from HTTP Header
void checkContentTypfromHeader(String payload, int len) {

  int index = payload.indexOf("Content-Type: ");
  if (index > -1) {
           
    payload = payload.substring(index+14,len);            // "Content-Length:" entfernen     
    payload = payload.substring(0,payload.indexOf("\n")); // Ende der Zeile

    if (payload.indexOf("json") > -1) log_typ = 1;        // JSON
    else if (payload.indexOf("text") > -1) log_typ = 2;   // TEXT
    else log_typ = 0;
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


enum {CONNECTFAIL, SENDTO, DISCONNECT, CLIENTCONNECT};

void printClient(const char* link, int arg) {

  switch (arg) {
    case CONNECTFAIL:   IPRINTP("f:"); break;    // Client Connect Fail
    case SENDTO:        IPRINTP("s:"); break;    // Client Send to
    case DISCONNECT:    IPRINTP("d:"); break;    // Disconnect Client
    case CLIENTCONNECT: IPRINTP("c:"); break;    // Client Connect
  }
  DPRINTLN(link);
}


int apicontent;
static AsyncClient * apiClient = NULL;

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Send to API
bool sendAPI(int check){

  if (check == 0) {
    if(apiClient) return false;                 //client already exists

    apiClient = new AsyncClient();
    if(!apiClient)  return false;               //could not allocate client

    apiClient->onError([](void * arg, AsyncClient * client, int error){
    IPRINTP("e:Client");
      apiClient = NULL;
      delete client;
    }, NULL);
    
  } else if (check == 2) {          // Senden ueber Server

    sys.server_state = 0;
    apiClient->onConnect([](void * arg, AsyncClient * client){
    
      //printClient(serverurl[urlindex].page.c_str(),CLIENTCONNECT);
      apicontent = false;
      
      apiClient->onError(NULL, NULL);
      
      client->onDisconnect([](void * arg, AsyncClient * c){
        //printClient(serverurl[urlindex].page.c_str() ,DISCONNECT);
        apiClient = NULL;
        delete c;
      }, NULL);

      client->onData([](void * arg, AsyncClient * c, void * data, size_t len){

          String payload((char*)data);
          //Serial.println(millis());
          //Serial.println(payload);
          //Serial.println(len);

          if (payload.indexOf("HTTP/1.1") > -1) {             // Time Stamp
            readUTCfromHeader(payload);
          }
          
          if ((payload.indexOf("200 OK") > -1)) {             // 200 Header
            sys.server_state = 1;                             // Server Communication: yes
            readContentLengthfromHeader(payload, len);
            checkContentTypfromHeader(payload, len);

            if (log_length > 0) {                             // Content available
              apicontent = 1;
              if (log_typ == 1 && payload.indexOf("{") > -1) {     // JSON: Body belongs to header
                apicontent = payload.indexOf("{")+1;          
                if ((len-(apicontent-1)) != log_length) 
                  Serial.println("[WARNING]: Content-Length unequal");
                Serial.println("Body belongs to header");
              } else  return;                                 // Header alone
            }
          
          } else if (payload.indexOf("302 Found") > -1) {     // 302 Header: new API-Links 
            readLocation(payload, len);
          }

          //} else if (payload.indexOf("500 Internal Server Error") > -1) {  // 500 Header: new API-Links 
          //  Serial.println("Fehler im Verbindungsaufbau");
          
          if (apicontent) {  // Body: 1 part
            if (log_typ == 1){    // JSON
              bodyWebHandler.setServerAPI((uint8_t*)data+(apicontent-1)); 
            }
            apicontent = 0;
            log_length -= len;                // Option das nicht alles auf einmal kommt bleibt offen
            //Serial.println(log_length);
            
          } else if (log_length > 0) {                        // Body: current part
            log_length -= len;                // leeren?
            //Serial.println(log_length);
            
          }
           
      }, NULL);

      //send the request
      //printClient(serverurl[urlindex].page.c_str() ,SENDTO);
      String message = apiData(apiindex);
      String adress = createCommand(POSTMETH,parindex,serverurl[urlindex].page.c_str(),serverurl[urlindex].host.c_str(),message.length());
      adress += message;
      client->write(adress.c_str());
      //Serial.println(adress);
      apiindex = NULL;
      urlindex = NULL;
      parindex = NULL;

    }, NULL);

    if(!apiClient->connect(serverurl[urlindex].host.c_str(), 80)){
        printClient(serverurl[urlindex].page.c_str() ,CONNECTFAIL);
        AsyncClient * client = apiClient;
        apiClient = NULL;
        delete client;
    }
  }
  return true;    // Nachricht kann gesendet werden
}



void check_api() {

  // bei wifi-connect wird update.state = -1 gesetzt

  if (update.state == -1 || update.state == 2) {  // -1 = check, 2 = check after restart during update
    if((wifi.mode == 1)) {
      //Serial.println("Verbindungsversuch API");
      if (sendAPI(0)) {             // blockt sich selber, so dass nur ein Client gleichzeitig offen
        apiindex = APIUPDATE;
        urlindex = APILINK;
        parindex = NOPARA;
        sendAPI(2);
      }

    // kommt nicht bei Systemstart zum Einsatz
    } else {                      // kein Internet, also Update stoppen
      if (update.state == -1)  update.get = "false";    // nicht während Update, da wird die Version gebraucht
    }
    
    if (update.state == -1) update.state = 0;         // von check (-1) auf ruhend (0) wechseln
    // kein Speichern im EE, Zustand -1 ist nur temporär
  } 
}




