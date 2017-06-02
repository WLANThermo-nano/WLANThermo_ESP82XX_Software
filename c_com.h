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

 // HELP: https://github.com/bblanchon/ArduinoJson


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// React to Serial Input 
void read_serial(char *buffer) {

  // GET HELP
  if (strcmp(buffer, "help")==0) {
    Serial.println();
    Serial.println("Syntax: {\"command\":\"xxx\",\"data\":[\"xxx\",\"xxx\"]}");
    Serial.println("Possible commands");
    Serial.println("restart    -> Restart ESP");
    Serial.println("getVersion -> Show Firmware Version Number");
    Serial.println("getSSID    -> Show current SSID");
    Serial.println("setWIFI    -> Reset wifi.json");
    Serial.println("           -> expected no data");
    Serial.println("addWIFI    -> Add new SSID to wifi.json");
    Serial.println("           -> expected data SSID and PASSWORD");
    Serial.println("setTS      -> Add THINGSPEAK KEY");
    Serial.println("           -> expected data KEY");
    Serial.println("getTS      -> Show THINGSPEAK KEY");
    Serial.println();
    return;
  }
  else if (strcmp(buffer, "data")==0) {
    AsyncWebServerRequest *request;
    handleData(request, false);
    return;
  }
  else if (strcmp(buffer, "settings")==0) {
    AsyncWebServerRequest *request;
    handleSettings(request, false);
    return;
  }
  else if (strcmp(buffer, "networklist")==0) {
    AsyncWebServerRequest *request;
    handleWifiResult(request, false);
    return;
  }
  else if (strcmp(buffer, "networkscan")==0) {
    AsyncWebServerRequest *request;
    handleWifiScan(request, false);
    return;
  }
  else if (strcmp(buffer, "activ")==0) {
    pitmaster.active = true;
    pitmaster.manuel = 90;
    return;
  }
  
  else if (strcmp(buffer, "configreset")==0) {
    setconfig(eCHANNEL,{});
    loadconfig(eCHANNEL);
    set_Channels();

    setconfig(eSYSTEM,{});
    loadconfig(eSYSTEM);
    return;
  }

  else if (strcmp(buffer, "log")==0) {
    StreamString output;
    getLog(&output,0);
    Serial.print(output);
    return;
  }

  else if (strcmp(buffer, "piepsertest")==0) {
    Serial.println("Piepsertest");
    piepserON();
    delay(1000);
    piepserOFF();
    return;
  }
  
  DPRINTP("You entered: >");
  DPRINT(buffer);
  DPRINTPLN("<");

  // Wenn nicht help dann json-Befehl auslesen
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buffer);
  
  if (!json.success()) {
    Serial.println("String invalid");
    return;
  }

  // new command
  const char* command = json["command"];

  if (!json.containsKey("command"))  {
    Serial.println("No command key");
    return;
  }

  // ADD WIFI SETTINGS
  if (strcmp(command, "addWIFI")==0) {
    const char* data[2];
    data[0] = json["data"][0];
    data[1] = json["data"][1];

    if (!modifyconfig(eWIFI,data)) DPRINTPLN("[INFO]\tFailed to save wifi config");
    else  DPRINTPLN("[INFO]\tWifi config saved");
    
  }

  // SET WIFI SETTINGS
  else if (strcmp(command, "setWIFI")==0) {
        
    if (setconfig(eWIFI,{})) DPRINTPLN("[INFO]\tReset wifi config");
  }

  // GET CURRENT WIFI SSID
  else if (strcmp(command, "getSSID")==0) {
    Serial.println(WiFi.SSID());
  }

  // SET THINGSPEAK KEY
  else if (strcmp(command, "setTS")==0) {

    const char* data[1]; 
    data[0] = json["data"][0];
    
    if (!setconfig(eTHING,data)) DPRINTPLN("[INFO]\tFailed to save Thingspeak config");
    else DPRINTPLN("[INFO]\tThingspeak config saved");
    
  }

  // GET THINGSPEAK KEY
  else if (strcmp(command, "getTS")==0) {
    Serial.println(THINGSPEAK_KEY);
  }

  // AUTOTUNE
  else if (strcmp(command, "autotune")==0) {
    startautotunePID(json["data"][0], json["data"][1]);
  }

  // RESTART SYSTEM
  else if (strcmp(command, "restart")==0) {
    ESP.restart();
  }

  // LET ESP SLEEP
  else if (strcmp(command, "sleep")==0) {
    display.displayOff();
    ESP.deepSleep(0);
    delay(100); // notwendig um Prozesse zu beenden
  }

  // GET FIRMWAREVERSION
  else if (strcmp(command, "getVersion")==0) {
    Serial.println(FIRMWAREVERSION);
  }

  // ADD PITMASTER PID
  else if (strcmp(command, "addPID")==0) {

    if (pidsize < PITMASTERSIZE) {

      pid[pidsize].name =    json["data"][0].asString();
      pid[pidsize].typ =     json["data"][1];
      pid[pidsize].Kp =      json["data"][2];  
      pid[pidsize].Ki =      json["data"][3];    
      pid[pidsize].Kd =      json["data"][4];                     
      pid[pidsize].Kp_a =    json["data"][5];                   
      pid[pidsize].Ki_a =    json["data"][6];                   
      pid[pidsize].Kd_a =    json["data"][7];                   
      pid[pidsize].Ki_min =  json["data"][8];                   
      pid[pidsize].Ki_max =  json["data"][9];                  
      pid[pidsize].pswitch = json["data"][10];                   
      pid[pidsize].reversal =json["data"][11];                   
      //pid[pidsize].DCmin =   json["data"][12];                   
      //pid[pidsize].DCmax =   json["data"][13];                   
      //pid[pidsize].SVmim =   json["data"][14];                  
      //pid[pidsize].SVmax =   json["data"][15];               
       
      pid[pidsize].esum =    0;             
      pid[pidsize].elast =   0;    
      pid[pidsize].id = pidsize;
      
      if (!modifyconfig(ePIT,{})) DPRINTPLN("[INFO]\tFailed to save pitmaster config");
      else {
        DPRINTPLN("[INFO]\tPitmaster config saved");
        pidsize++;   // Erhöhung von pidsize nur wenn auch gespeichert wurde
      }
    } else DPRINTPLN("[INFO]\tTo many pitmaster");
  }

  // SET PITMASTER PID
  else if (strcmp(command, "setPID")==0) {
    set_pid();  // Default PID-Settings
    if (setconfig(ePIT,{})) DPRINTPLN("[INFO]\tReset pitmaster config");
  }

  // SET PITMASTER MANUEL
  else if (strcmp(command, "setManuel")==0) {
    pitmaster.active = true;
    //pitmaster.typ = 1;
    pitmaster.manuel = json["data"][0];
    
  }

  else Serial.println("Unkwown command");     // Befehl nicht erkannt
    
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Put together Serial Input 
int readline(int readch, char *buffer, int len) {
  
  static int pos = 0;
  int rpos;

  if (readch > 0) {
    switch (readch) {
      case '\n': // Ignore new-lines
        break;
      case '\r': // Return on CR
        rpos = pos;
        pos = 0;  // Reset position index ready for next time
        return rpos;
      default:
        if (pos < len-1) {
          buffer[pos++] = readch;
          buffer[pos] = 0;
        }
    }
  }
  return -1;    // No end of line has been found, so return -1.
}




