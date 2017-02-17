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
    
    HISTORY:
    0.1.00 - 2016-12-30 initial version: Example ESP8266WebServer/FSBrowser
    
 ****************************************************/

#include <ESP8266WebServer.h>   // https://github.com/esp8266/Arduino
#include <ESP8266mDNS.h>        

ESP8266WebServer server(80);    // declare webserver to listen on port 80
File fsUploadFile;              // holds the current upload

const char* www_username = "admin";
const char* www_password = "esp8266";

void buildlist();

String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path){
  Serial.println("handleFileRead: " + path);
  if(path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handleFileUpload(){
  if(server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    //Serial.print("handleFileUpload Data: "); Serial.println(upload.currentSize);
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile)
      fsUploadFile.close();
    Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
  }
}

// Delete File in FS
void handleFileDelete(){
  if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  Serial.println("handleFileDelete: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

// Create File in FS
void handleFileCreate(){
  if(server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  Serial.println("handleFileCreate: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(SPIFFS.exists(path))
    return server.send(500, "text/plain", "FILE EXISTS");
  File file = SPIFFS.open(path, "w");
  if(file)
    file.close();
  else
    return server.send(500, "text/plain", "CREATE FAILED");
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileList() {
  if(!server.hasArg("dir")) {server.send(500, "text/plain", "BAD ARGS"); return;}

  String path = server.arg("dir");
  Serial.println("handleFileList: " + path);
  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while(dir.next()){
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir)?"dir":"file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }

  output += "]";
  server.send(200, "text/json", output);
}

void buildDatajson(char *buffer, int len) {
  
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  JsonObject& system = root.createNestedObject("system");

  system["time"] = String(now());
  system["utc"] = timeZone;
  system["soc"] = BatteryPercentage;
  system["charge"] = false;
  system["rssi"] = rssi;
  system["unit"] = temp_unit;

  JsonArray& channel = root.createNestedArray("channel");

  for (int i = 0; i < CHANNELS; i++) {
  JsonObject& data = channel.createNestedObject();
    data["number"]= i+1;
    data["name"]  = "Kanal " + String(i+1);
    data["typ"]   = ch[i].typ;
    data["temp"]  = ch[i].temp;
    data["min"]   = ch[i].min;
    data["max"]   = ch[i].max;
    //data["set"]   = ch[i].soll;
    data["alarm"] = ch[i].alarm;
    data["color"] = ch[i].color;
  }
  
  JsonObject& master = root.createNestedObject("pitmaster");

  master["channel"] = 0;
  master["typ"] = "";
  master["value"] = 100;

  size_t size = root.measureLength() + 1;
  //Serial.println(size);
  
  if (size < len) {
    root.printTo(buffer, size);
  } else Serial.println("Buffer zu klein");
  
}

void buildSettingjson(char *buffer, int len) {

  String host = HOSTNAME;
  host += String(ESP.getChipId(), HEX);

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  JsonObject& system = root.createNestedObject("system");

  system["time"] = String(now());
  system["utc"] = timeZone;
  system["ap"] = APNAME;
  system["host"] = host;
  system["version"] = FIRMWAREVERSION;
  
  JsonArray& typ = root.createNestedArray("sensors");
  for (int i = 0; i < SENSORTYPEN; i++) {
    typ.add(ttypname[i]);
  }
    
  size_t size = root.measureLength() + 1;
  //Serial.println(size);
  
  if (size < len) {
    root.printTo(buffer, size);
  } else Serial.println("Buffer zu klein");
  
}

void handleData() {
  
  static char sendbuffer[1000];
  buildDatajson(sendbuffer, 1000);
  server.send(200, "text/json", sendbuffer);
}


void handleSettings() {

  static char sendbuffer[200];
  buildSettingjson(sendbuffer, 200);
  server.send(200, "text/json", sendbuffer);
}

void handleSetSettings() {

  if(!server.authenticate(www_username, www_password))
      return server.requestAuthentication();

  String inString = server.arg("channel"); 
  int channel = inString.toInt();
  
  if ((channel > 0) && (channel < CHANNELS+1)) {
    Serial.print("[POST]\tChannel: ");
    Serial.print(channel);

    inString = server.arg("typ");
    int typ = inString.toInt();

    if ((typ > -1) && (typ < SENSORTYPEN)) {
      Serial.print(", Typ: ");
      Serial.print(typ);
      ch[channel-1].typ = typ;
    }

    inString = server.arg("min");
    int min = inString.toInt();

    if ((min > 10) && (min < 90)) {
      Serial.print(", Min: ");
      Serial.print(min);
      ch[channel-1].min = min;
    }

    inString = server.arg("max");
    int max = inString.toInt();

    if ((max > 10) && (max < 90)) {
      Serial.print(", Max: ");
      Serial.print(max);
      ch[channel-1].max = max;
    }
    server.send(200, "text/json", "1");
  } else {
    server.send(200, "text/json", "0");
  }

}

void handleSetChannels() {
  
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(server.arg("plain"));
  if (!json.success()) Serial.println("Fehler");

  for (int i = 0; i < CHANNELS; i++) {
    JsonObject& _cha = json["channel"][i];
    //ch[i].name =  _cha["name"].asString();
    ch[i].typ =   _cha["typ"]; 
    ch[i].min =   _cha["min"]; 
    ch[i].max =   _cha["max"];
    Serial.println(ch[i].max);
    ch[i].alarm = _cha["alarm"]; 
    ch[i].color = _cha["color"].asString();
  }

  Serial.println("Hallo");
  //modifyconfig(eCHANNEL,"","");
  server.send(200, "text/json", "1");
}

void server_setup() {

    String host = HOSTNAME;
    host += String(ESP.getChipId(), HEX);
    MDNS.begin(host.c_str());
    Serial.print("[INFO]\tOpen http://");
    Serial.print(host);
    Serial.println("/data to see the current temperature");
  

    //SERVER INIT
    //list directory
    server.on("/list", HTTP_GET, handleFileList);

    //load editor
    server.on("/edit", HTTP_GET, [](){
        if(!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
    });

    //create file
    server.on("/edit", HTTP_PUT, handleFileCreate);

    //delete file
    server.on("/edit", HTTP_DELETE, handleFileDelete);

    //first callback is called after the request has ended with all parsed arguments
    //second callback handles file uploads at that location
    server.on("/edit", HTTP_POST, [](){ server.send(200, "text/plain", ""); }, handleFileUpload);

    server.on("/config", HTTP_GET, []() {
        server.send(200, "text/json", "config");
    });

    //called when the url is not defined here
    //use it to load content from SPIFFS
    server.onNotFound([](){
        if(!handleFileRead(server.uri()))
            server.send(404, "text/plain", "FileNotFound");
    });

    //get heap status, analog input value and all GPIO statuses in one json call
    server.on("/all", HTTP_GET, [](){
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();

        root["heap"] = ESP.getFreeHeap();
        root["analog"] = ch[0].temp;
        root["gpio"] = (uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16));

        size_t size = root.measureLength() + 1;
        char json[size];
        root.printTo(json, size);

        server.send(200, "text/json", json);
    });

    //list Temperature Data
    server.on("/data", HTTP_GET, handleData);

    //list Temperature Data
    //server.on("/grafik", HTTP_GET, buildlist);
    
    //list Setting Data
    server.on("/settings", HTTP_GET, handleSettings);

    //list Set Setting Data
    server.on("/setsettings", HTTP_POST, handleSetSettings);

    
    //list Set Setting Data
    server.on("/setchannels", HTTP_POST, handleSetChannels);

    
    // Auth
    server.on("/", [](){
    //if(!server.authenticate(www_username, www_password))
      //return server.requestAuthentication();
      if(!handleFileRead("/")) server.send(404, "text/plain", "FileNotFound");
      //server.send(200, "text/plain", "Login OK");
    });

    server.begin();
    #ifdef DEBUG
    Serial.println("[INFO]\tHTTP server started");
    #endif
    MDNS.addService("http", "tcp", 80);

}



unsigned long ulMeasCount=1;    // values already measured
unsigned long ulNoMeasValues=10; // size of array

unsigned long MakeList (bool bStream)
{
  unsigned long ulLength=0;
  
  // here we build a big list.
  // we cannot store this in a string as this will blow the memory   
  // thus we count first to get the number of bytes and later on 
  // we stream this out
  if (ulMeasCount>0) { 
    
    String sTable="";
    for (unsigned long li = 0; li < ulMeasCount; li++)  {
      
      // result shall be ['18:24:08 - 21.5.2015',21.10,49.00],
      unsigned long ulIndex=li%ulNoMeasValues;
      sTable += "['";
      sTable += "jetzt";//epoch_to_string(pulTime[ulIndex]).c_str();
      sTable += "',";
      sTable += 20; //pfTemp[ulIndex];
      sTable += ",";
      sTable += 20; //pfHum[ulIndex];
      sTable += "],\n";

      // play out in chunks of 1k
      if(sTable.length()>1024)  {
        if(bStream) {
          //pclient->print(sTable);
          server.send(200, "text/html", sTable);
        }
        ulLength += sTable.length();
        sTable = "";
      }
    }
    
    // remaining chunk
    if(bStream) {
      //pclient->print(sTable);
      server.send(200, "text/html", sTable);
    } 
    ulLength += sTable.length();  
  }
  
  return(ulLength);
}


String MakeHTTPHeader(unsigned long ulLength) {
  
  String sHeader;
  sHeader  = F("HTTP/1.1 200 OK\r\nContent-Length: ");
  sHeader += ulLength;
  sHeader += F("\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");
  return(sHeader);
}

String MakeHTTPFooter() {
  
  String sResponse;
  sResponse  = F("<FONT SIZE=-2><BR>Freies RAM=");
  sResponse += (uint32_t)system_get_free_heap_size();
  sResponse += F(" - Max. Datenpunkte=");
  sResponse += ulNoMeasValues;
  sResponse += F("<BR>Nano 2017<BR></body></html>");
  return(sResponse);
}


void buildlist()
{
    
  String sResponse,sResponse2,sHeader;
  unsigned long ulSizeList = MakeList(false); // get size of list first

  //sResponse  = F("<html>\n<head>\n<title>WLAN Thermo Nano Logger</title>\n<script type=\"text/javascript\" src=\"https://www.google.com/jsapi?autoload={'modules':[{'name':'visualization','version':'1','packages':['corechart']}]}\"></script>\n");
  //sResponse += F("<script type=\"text/javascript\"> google.setOnLoadCallback(drawChart);\nfunction drawChart() {var data = google.visualization.arrayToDataTable([\n['Zeit / UTC', 'Temp1', 'Temp2'],\n");    
  sResponse  = F("<html>\n<head>\n<title>WLAN Logger f&uuml;r Lufttemperatur und Feuchtigkeit</title>\n<script type=\"text/javascript\" src=\"https://www.google.com/jsapi?autoload={'modules':[{'name':'visualization','version':'1','packages':['corechart']}]}\"></script>\n");
  sResponse += F("<script type=\"text/javascript\"> google.setOnLoadCallback(drawChart);\nfunction drawChart() {var data = google.visualization.arrayToDataTable([\n['Zeit / UTC', 'Temperatur', 'Feuchtigkeit'],\n");    
    
  // part 2 of response - after the big list
  sResponse2  = F("]);\nvar options = {title: 'Verlauf',vAxes:{0:{viewWindowMode:'explicit',gridlines:{color:'black'},format:\"##.##\260C\"},1: {gridlines:{color:'transparent'},format:\"##,##%\"},},series:{0:{targetAxisIndex:0},1:{targetAxisIndex:1},},curveType:'none',legend:{ position: 'bottom'}};");
  sResponse2 += F("var chart = new google.visualization.LineChart(document.getElementById('curve_chart'));chart.draw(data, options);}\n</script>\n</head>\n");
  sResponse2 += F("<body>\n<font color=\"#000000\"><body bgcolor=\"#d0d0f0\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=yes\"><h1>WLAN Thermo Nano Logger</h1><BR>");
  sResponse2 += F("<BR>\n<div id=\"curve_chart\" style=\"width: 600px; height: 400px\"></div>");
  sResponse2 += MakeHTTPFooter().c_str();
    
  // Send the response to the client - delete strings after use to keep mem low
  //client.print(MakeHTTPHeader(sResponse.length()+sResponse2.length()+ulSizeList).c_str()); 
  //client.print(sResponse); sResponse="";
  //MakeList(true);
  //client.print(sResponse2);

  server.send(200, "text/html", MakeHTTPHeader(sResponse.length()+sResponse2.length()+ulSizeList).c_str());
  server.send(200, "text/html", sResponse); sResponse="";
  MakeList(true);
  server.send(200, "text/html", sResponse2);
   
}






