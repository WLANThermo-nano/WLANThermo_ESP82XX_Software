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

  // Commando auslesen
  String str(buffer);
  int index = str.indexOf(':');

  // Falls zusätzliche Attribute vorhanden
  if (index > 0) {
    String command = str.substring(0,index);
    DPRINTP("[INFO]\tSerial Command: ");
    DPRINTLN(command);
    
    for (int i = 0;i<index+1;i++) {
    *buffer++;
    }
    
    //Serial.println(buffer);
    uint8_t * PM_buffer = reinterpret_cast<uint8_t *>(buffer);

    // ADD WIFI SETTINGS
    if (command == "setnetwork") {
       AsyncWebServerRequest *request;
       handleSetNetwork(request,PM_buffer);
       return;
    }

    // SET THINGSPEAK KEYs
    else if (command == "setcharts") {
      AsyncWebServerRequest *request;
      handleSetChart(request,PM_buffer);
      return;
    }

    // SET SYSTEM
    else if (command == "setsystem") {
      AsyncWebServerRequest *request;
      handleSetSystem(request,PM_buffer);
      return;
    }

    // SET CHANNELS
    else if (command == "setchannels") {
      AsyncWebServerRequest *request;
      handleSetChannels(request,PM_buffer);
      return;
    }

    // AUTOTUNE
    else if (command == "autotune") {
      //startautotunePID(json["data"][0], json["data"][1]);
      return;
    }

    // ADD PITMASTER PID
    else if (command == "addpid") {
      AsyncWebServerRequest *request;
      handleAddPitmaster(request,PM_buffer);     
      return;
    }

    // SET PITMASTER
    else if (command == "setpitmaster") {
      AsyncWebServerRequest *request;
      handleSetPitmaster(request,PM_buffer); 
      return;    
    }

    // SET PITMASTER MANUEL
    else if (command == "setmanuel") {
      pitmaster.active = true;
      pitmaster.manuel = true;
      String val(buffer);
      pitmaster.value = val.toInt();
      return;
    }    
  
  } else {
  
  
    // GET HELP
    if (str == "help") {
      Serial.println();
      Serial.println(F("Syntax: \"command\":{\"Attribut\":\"Value\"]}"));
      Serial.println(F("Possible commands without additional attributs"));
      Serial.println(F("restart\t\t-> Restart ESP"));
      Serial.println(F("data\t\t-> Read data.json"));
      Serial.println(F("settings\t-> Read settings.json"));
      Serial.println(F("networklist\t-> Get Networks"));
      Serial.println(F("networkscan\t-> Start Network Scan"));
      Serial.println(F("clearwifi\t-> Reset WIFI Settings"));
      Serial.println(F("stopwifi\t-> Stop WIFI"));
      Serial.println();
      return;
    }
    
    else if (str == "data") {
      AsyncWebServerRequest *request;
      handleData(request, false);
      return;
    }
  
    else if (str == "settings") {
      AsyncWebServerRequest *request;
      handleSettings(request, false);
      return;
    }
  
    else if (str == "networklist") {
      AsyncWebServerRequest *request;
      handleWifiResult(request, false);
      return;
    }
    
    else if (str == "networkscan") {
      AsyncWebServerRequest *request;
      handleWifiScan(request, false);
      return;
    }

    else if (str == "clearwifi") {
      setconfig(eWIFI,{}); // clear Wifi settings
      return;
    }

    else if (str == "stopwifi") {
      isAP = 3; // Turn Wifi off
      return;
    }
  
    else if (str == "pittest") {
      pitmaster.active = true;
      pitmaster.manuel = true;
      pitmaster.value = 90;
      return;
    }
  
    else if (str == "configreset") {
      setconfig(eCHANNEL,{});
      loadconfig(eCHANNEL);
      set_Channels();
      setconfig(eSYSTEM,{});
      loadconfig(eSYSTEM);
      return;
    }

    else if (str == "piepsertest") {
      Serial.println("Piepsertest");
      piepserON();
      delay(1000);
      piepserOFF();
      return;
    }

    else if (str == "getSSID") {
      Serial.println(WiFi.SSID());
    }

    else if (str == "getTS") {
      Serial.println(charts.TSwriteKey);
      Serial.println(charts.TShttpKey);
      Serial.println(charts.TSchID);
    }

    // RESTART SYSTEM
    else if (str == "restart") {
      ESP.restart();
    }

    // LET ESP SLEEP
    else if (str == "sleep") {
      display.displayOff();
      ESP.deepSleep(0);
      delay(100); // notwendig um Prozesse zu beenden
    }

    // GET FIRMWAREVERSION
    else if (str == "getVersion") {
      Serial.println(FIRMWAREVERSION);
    }

    // Reset PITMASTER PID
    else if (str == "setPID") {
      set_pid();  // Default PID-Settings
      if (setconfig(ePIT,{})) DPRINTPLN("[INFO]\tReset pitmaster config");
    }

    // HTTP UPDATE
    else if (str == "update") {
      sys.update = 1;
      return;
    }

    // CHECK HTTP UPDATE
    else if (str == "checkupdate") {
      sys.update = -1;
      return;
    }
  }

  DPRINTP("[INFO]\tYou entered: >");
  DPRINT(buffer);
  DPRINTPLN("<");
  DPRINTPLN("[INFO]\tUnkwown command");     // Befehl nicht erkannt  
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




