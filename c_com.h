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

    // Umsortieren
    for (int i = 0;i<index+1;i++) {
      *buffer++;
    }
    uint8_t * PM_buffer = reinterpret_cast<uint8_t *>(buffer);

    // ADD WIFI SETTINGS
    if (command == "setnetwork") {
       bodyWebHandler.setNetwork(PM_buffer);
       return;
    }

    // SET THINGSPEAK KEYs
    else if (command == "setcharts") {
      //bodyWebHandler.setChart(PM_buffer);
      return;
    }

    // SET SYSTEM
    else if (command == "setsystem") {
      //bodyWebHandler.setSystem(PM_buffer);
      return;
    }

    // SET CHANNELS
    else if (command == "setchannels") {
      //bodyWebHandler.setChannels(PM_buffer);
      return;
    }

    // SET PITMASTER
    else if (command == "setpitmaster") {
      //handleSetPitmaster(request,PM_buffer); 
      return;    
    }

    // SET PITMASTER MANUEL
    else if (command == "setmanual") {
      pitmaster.active = true;
      pitmaster.manual = true;
      String val(buffer);
      pitmaster.value = val.toInt();
      return;
    }   

     // UPDATE auf bestimmte Version
    else if (command == "update") {
      String payload((char*)buffer);
      if (payload.indexOf("v") == 0) {
        sys.getupdate = payload;  // kein Speichern, da während des Updates eh gespeichert wird
        sys.update = 1;  
      } else  DPRINTPLN("[INFO]\tUpdateversion nicht erkannt!");
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
      nanoWebHandler.handleData(false);
      return;
    }
  
    else if (str == "settings") {
      nanoWebHandler.handleSettings(false);
      return;
    }
  
    else if (str == "networklist") {
      nanoWebHandler.handleWifiResult(false);
      return;
    }
    
    else if (str == "networkscan") {
      nanoWebHandler.handleWifiScan(false);
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
      pitmaster.manual = true;
      pitmaster.value = 50;
      //pitmaster.pid = 1;
      return;
    }

    else if (str == "pittest2") {
      pitmaster.active = true;
      pitmaster.manual = true;
      pitmaster.value = 100;
      return;
    }
  
    else if (str == "configreset") {
      nanoWebHandler.configreset();
      return;
    }

    else if (str == "piepser") {
      Serial.println("Piepsertest");
      piepserON();
      delay(1000);
      piepserOFF();
      return;
    }

    else if (str == "getSSID") {
      Serial.println(WiFi.SSID());
      return;
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
      return;
    }

    // Reset PITMASTER PID
    else if (str == "setPID") {
      set_pid();  // Default PID-Settings
      if (setconfig(ePIT,{})) DPRINTPLN("[INFO]\tReset pitmaster config");
      return;
    }

    // AUTOTUNE
    else if (str == "autotune") {
      startautotunePID(5, true, 40, 40L*60L*1000L);
      return;
    }

    // STOP PITMASTER
    else if (str == "stop") {
      disableAllHeater();
      setconfig(ePIT,{});
      return;
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

    // GET MAC
    else if (str == "mac") {
      DPRINTLN(getMacAddress());
      return;
    }

    // Test
    else if (str == "sendSetting") {
      //sendSettings();
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




