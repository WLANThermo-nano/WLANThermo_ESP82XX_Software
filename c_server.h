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

struct myRequest {
  String host;
  String url;
  String method;
  String response;
  AsyncWebServerRequest *request;
};

myRequest myrequest;

static AsyncClient * aClient = NULL;

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Check if there is http update
void getRequest() {

  if(aClient) return;                 //client already exists

  aClient = new AsyncClient();
  if(!aClient)  return;               //could not allocate client

  aClient->onError([](void * arg, AsyncClient * client, int error){
    aClient = NULL;
    delete client;
  }, NULL);

  aClient->onConnect([](void * arg, AsyncClient * client){

   aClient->onError(NULL, NULL);

   client->onDisconnect([](void * arg, AsyncClient * c){
    DPRINTPLN("[INFO]\tDisconnect myRequest Client");
    //Serial.println(myrequest.response);
    //myrequest.request->send(404);
    myrequest.request->send(200, "text/plain", myrequest.response);
    myrequest.response = "";
    aClient = NULL;
    delete c;
   }, NULL);

   client->onData([](void * arg, AsyncClient * c, void * data, size_t len){
    String payload((char*)data);
    myrequest.response += payload; 
    Serial.println("[INFO]\tResponse external Request");
   }, NULL);

   //send the request
   DPRINTPLN("[INFO]\tSend external Request:");
   String url;
   if (myrequest.method == "POST") url += F("POST ");
   else  url += F("GET ");
   url += myrequest.url;
   url += F(" HTTP/1.1\n");
   url += F("Host: ");
   url += myrequest.host;
   //Serial.println(url);
   url += F("\n\n");
   
   client->write(url.c_str());
    
 }, NULL);

 if(!aClient->connect(myrequest.host.c_str(), 80)){
   Serial.println("[INFO]\MyRequest Client Connect Fail");
   AsyncClient * client = aClient;
   aClient = NULL;
   delete client;
 }    
}



// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
String serverLog() {

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  root["SN"] = String(ESP.getChipId(), HEX);
  JsonArray& _logs = root.createNestedArray("logs");

  if (log_count > 9) {
    
    for (int i = 0; i < 10; i++) {
      if (mylog[i].modification) {    // nur bei Aenderung wird das Log benutzt
        JsonObject& _log = _logs.createNestedObject();
        _log["time"] = mylog[i].timestamp;
        _log["battery"] = mylog[i].battery;
        _log["pit_set"] = (mylog[i].soll==NULL)?NULL:mylog[i].soll/10.0;
        _log["pit_value"] = mylog[i].pitmaster; 
        _log["ch1"] = (mylog[i].tem[0]==NULL)?999:mylog[i].tem[0]/10.0;
        _log["ch2"] = (mylog[i].tem[1]==NULL)?999:mylog[i].tem[1]/10.0;
        _log["ch3"] = (mylog[i].tem[2]==NULL)?999:mylog[i].tem[2]/10.0;
        _log["ch4"] = (mylog[i].tem[3]==NULL)?999:mylog[i].tem[3]/10.0;
        _log["ch5"] = (mylog[i].tem[4]==NULL)?999:mylog[i].tem[4]/10.0;
        _log["ch6"] = (mylog[i].tem[5]==NULL)?999:mylog[i].tem[5]/10.0;
        _log["ch7"] = (mylog[i].tem[6]==NULL)?999:mylog[i].tem[6]/10.0;
        _log["ch8"] = (mylog[i].tem[7]==NULL)?999:mylog[i].tem[7]/10.0;
      } 
    }
  }
 
  String json;
  root.printTo(json);
  
 /*
 
  String json = "{\"SN\":\"";
  json += String(ESP.getChipId(), HEX);
  json += "\",\"logs\":[";
  
  if (log_count > 9) {
    
    for (int i = 0; i < 10; i++) {
      if (mylog[i].modification) {    // nur bei Aenderung wird das Log benutzt
        if (i > 0) json += ",";
        json += "{\"time\":";
        json += mylog[i].timestamp;
        json += ",\"battery\":";
        json += mylog[i].battery;
        json += ",\"pit_set\":";
        if (mylog[i].soll=!NULL) json += mylog[i].soll/10.0;
        json += ",\"pit_value\":";
        json += mylog[i].pitmaster; 
        for (int j = 0; j < 8; j++) {
          json += ",\"ch";
          json += String(j);
          json += "\":";
          if (mylog[i].tem[j]!=NULL)  json += mylog[i].tem[j]/10.0;
        }
        json += "}";
      } 
    }
  }
  json += "]}";

  */
  return json;
}


static AsyncClient * LogClient = NULL;

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
void sendServerLog() {

  if(LogClient) return;                 //client already exists

  LogClient = new AsyncClient();
  if(!LogClient)  return;               //could not allocate client

  LogClient->onError([](void * arg, AsyncClient * client, int error){
    printClient(SAVELOGSLINK,CLIENTERRROR);
    LogClient = NULL;
    delete client;
  }, NULL);

  LogClient->onConnect([](void * arg, AsyncClient * client){

   LogClient->onError(NULL, NULL);

   client->onDisconnect([](void * arg, AsyncClient * c){
    printClient(SAVELOGSLINK ,DISCONNECT);
    LogClient = NULL;
    delete c;
   }, NULL);

   client->onData([](void * arg, AsyncClient * c, void * data, size_t len){
    String payload((char*)data);
    serverAnswer(payload, len);
   }, NULL);

   //send the request
   printClient(SAVELOGSLINK,SENDTO); 

   String message = serverLog(); 
   String adress = createCommand(POSTMETH,NOPARA,SAVELOGSLINK,NANOSERVER,message.length());
   adress += message;
   client->write(adress.c_str());
   //Serial.println(adress);
      
 }, NULL);

 if(!LogClient->connect(NANOSERVER, 80)){
   printClient(SAVELOGSLINK ,CONNECTFAIL);
   AsyncClient * client = LogClient;
   LogClient = NULL;
   delete client;
 }    
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
String cloudData() {

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& system = root.createNestedObject("system");

    system["time"] = String(now());
    system["soc"] = battery.percentage;
    system["charge"] = !battery.charge;
    system["rssi"] = rssi;
    system["unit"] = temp_unit;
    //system["sn"] = String(ESP.getChipId(), HEX);

    JsonArray& channel = root.createNestedArray("channel");

    for (int i = 0; i < CHANNELS; i++) {
    JsonObject& data = channel.createNestedObject();
      data["number"]= i+1;
      data["name"]  = ch[i].name;
      data["typ"]   = ch[i].typ;
      data["temp"]  = limit_float(ch[i].temp, i);
      data["min"]   = ch[i].min;
      data["max"]   = ch[i].max;
      data["alarm"] = ch[i].alarm;
      data["color"] = ch[i].color;
    }
  
    JsonObject& master = root.createNestedObject("pitmaster");

    master["channel"] = pitmaster.channel+1;
    master["pid"] = pitmaster.pid;
    master["value"] = (int)pitmaster.value;
    master["set"] = pitmaster.set;
    if (pitmaster.active)
      if (autotune.initialized)  master["typ"] = "autotune";
      else if (pitmaster.manual) master["typ"] = "manual";
      else  master["typ"] = "auto";
    else master["typ"] = "off";  

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
    LogClient = NULL;
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
   String message = cloudData();   
   String adress = createCommand(GETMETH,SAVEDATA,SAVEDATALINK,CLOUDSERVER,message.length());
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
void server_setup() {

  MDNS.begin(sys.host.c_str());  // siehe Beispiel: WiFi.hostname(host); WiFi.softAP(host);
  DPRINTP("[INFO]\tOpen http://");
  DPRINT(sys.host);
    
  server.addHandler(&nanoWebHandler);
  server.addHandler(&bodyWebHandler);
  //server.addHandler(&logHandler);
    
  server.on("/help",HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("https://github.com/WLANThermo-nano/WLANThermo_nano_Software/blob/master/README.md");
  }).setFilter(ON_STA_FILTER);
    
      
  server.on("/info",[](AsyncWebServerRequest *request){
    FSInfo fs_info;
    SPIFFS.info(fs_info);
    request->send(200,"","totalBytes:" +String(fs_info.totalBytes) + "\n"
      +"usedBytes: " + String(fs_info.usedBytes)+ "\n"
      +"heap: "+String(ESP.getFreeHeap()) + "\n"
      +"sn: "+String(ESP.getChipId(), HEX) + "\n"
      +"batmin: "+String(battery.min) + "\n"
      +"batmax: "+String(battery.max)
      +"bat: "+String(battery.percentage));
  });

  server.on("/god",[](AsyncWebServerRequest *request){
    sys.god =!sys.god;
    setconfig(eSYSTEM,{});
    if (sys.god) request->send(200, "text/plain", "GodMode aktiviert.");
    else request->send(200, "text/plain", "GodMode deaktiviert.");
  });

  server.on("/clearplot",[](AsyncWebServerRequest *request){
    log_count = 0; //TEST
    request->send(200, "text/plain", "true");
  });

  server.on("/v2",[](AsyncWebServerRequest *request){
    sys.hwversion = 2;
    setconfig(eSYSTEM,{});
    request->send(200, "text/plain", "v2");
  });

  server.on("/pitsupply",[](AsyncWebServerRequest *request){
    if (sys.hwversion > 1 && !sys.pitsupply) {
      sys.pitsupply = true;
      request->send(200, "text/plain", "aktiviert");
    } else {
      sys.pitsupply = false;
      request->send(200, "text/plain", "deaktiviert");
    }
  });
   
  server.on("/startlog",[](AsyncWebServerRequest *request){
    chart.on = true;
    request->send(200, "text/plain", "true");
  });

  server.on("/newtoken",[](AsyncWebServerRequest *request){
    ESP.wdtDisable(); 
    iot.CL_token = newToken();
    setconfig(eTHING,{});
    lastUpdateCloud = 0; // Daten senden forcieren
    ESP.wdtEnable(10);
    request->send(200, "text/plain", iot.CL_token);
  });
  

  server.on("/setDC",[](AsyncWebServerRequest *request) { 
      if(request->hasParam("aktor")&&request->hasParam("dc")&&request->hasParam("val")){
        ESP.wdtDisable(); 
        bool dc = request->getParam("dc")->value().toInt();
        byte aktor = request->getParam("aktor")->value().toInt();
        int val = request->getParam("val")->value().toInt();
        DC_control(dc, aktor, val);
        DPRINTP("[INFO]\tDC-Test: ");
        DPRINTLN(val);
        ESP.wdtEnable(10);
        request->send(200, "text/plain", "true");
      } else request->send(200, "text/plain", "false");
  });

  server.on("/getRequest",[](AsyncWebServerRequest *request) { 
      if(request->hasParam("url")&&request->hasParam("method")&&request->hasParam("host")){
        //ESP.wdtDisable(); 
        //Serial.println(ESP.getFreeHeap());
        Serial.println("[REQUEST]\t/getRequest");
        myrequest.url = request->getParam("url")->value();
        myrequest.host = request->getParam("host")->value();
        myrequest.method = request->getParam("method")->value();
        myrequest.request = request;
        getRequest(); 
        //ESP.wdtEnable(10);
      } else request->send(200, "text/plain", "false");
  });

  server.on("/autotune",[](AsyncWebServerRequest *request) { 
      if(request->hasParam("cycle")&&request->hasParam("over")&&request->hasParam("timelimit")){
        ESP.wdtDisable(); 
        Serial.println(ESP.getFreeHeap());
        long limit = request->getParam("timelimit")->value().toInt();
        int over = request->getParam("over")->value().toInt();
        int cycle = request->getParam("cycle")->value().toInt();
        startautotunePID(cycle, true, over, limit);
        ESP.wdtEnable(10);
        request->send(200, "text/plain", "true");
      } else request->send(200, "text/plain", "false");
  });

  // to avoid multiple requests to ESP
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html"); // gibt alles im Ordner frei
    
  // 404 NOT found: called when the url is not defined here
  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404);
  });
      
  server.begin();
  DPRINTPLN("[INFO]\tHTTP server started");
  MDNS.addService("http", "tcp", 80);
}

