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
    IPRINTP("Serial: ");
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

    else if (command == "setEE") {
      if (m24.exist()) {
        String payload((char*)buffer);
        if (payload.length() == PRODUCTNUMBERLENGTH) {
          char item[PRODUCTNUMBERLENGTH];
          payload.toCharArray(item, PRODUCTNUMBERLENGTH+1);
          m24.put(0,item);
          m24.get(0,item);
          IPRINTP("M24C02: ");
          Serial.println(item);
          if (item[0] == 'n') {   // Kennung
            String str(item);
            sys.item = str;
            piepserON();
            sys.piepoff_t = 6;
            //delay(1500);
            //piepserOFF();
          }
        }
      }
      return;
    }

     // UPDATE auf bestimmte Version
    else if (command == "update") {
      String payload((char*)buffer);
      if (payload.indexOf("v") == 0) {
        update.get = payload;  // kein Speichern, da während des Updates eh gespeichert wird
        if (update.get == update.version) update.state = 1;   // Version schon bekannt, direkt los
        else update.state = -1;                               // Version erst vom Server anfragen
      } else  {IPRINTPLN("Update unbekannt!");}
      return;    
    }

    // Battery MIN
    else if (command == "setbattmin") {
      String payload((char*)buffer);
      if (payload.length() == 4) {
        battery.min = payload.toInt();
        setconfig(eSYSTEM,{});
      }
      return;    
    }

    // Battery MAX
    else if (command == "setbattmax") {
      String payload((char*)buffer);
      if (payload.length() == 4) {
        battery.max = payload.toInt();
        setconfig(eSYSTEM,{});
      }
      return;    
    }
  
  } else {
  
    if (str == "getEE") {
      if (m24.exist()) {
        Serial.print("pn: ");
        Serial.println(sys.item);     
      }
      return;
    }

    else if (str == "data") {
      Serial.println(apiData(APIDATA));
      return;
    }
  
    else if (str == "settings") {
      Serial.println(apiData(APISETTINGS));
      return;
    }
  /*
    else if (str == "networklist") {
      nanoWebHandler.handleWifiResult(false);
      return;
    }
    
    else if (str == "networkscan") {
      nanoWebHandler.handleWifiScan(false);
      return;
    }
*/
    
    else if (str == "clearwifi") {
      setconfig(eWIFI,{}); // clear Wifi settings
      wifi.mode = 5;  // interner Speicher leeren
      sys.restartnow = true;
      return;
    }
  
    else if (str == "configreset") {
      nanoWebHandler.configreset();
      return;
    }
/*
    else if (str == "battery") {
      notification.type = 2;
      Serial.println("Test");
      return;
    }
*/
    // RESTART SYSTEM
    else if (str == "restart") {
      sys.restartnow = true;
      return;
    }

/*
    // LET ESP SLEEP
    else if (str == "sleep") {
      display.displayOff();
      ESP.deepSleep(0);
      delay(100); // notwendig um Prozesse zu beenden
    }
*/

    // STOP PITMASTER
    else if (str == "stop") {
      pitMaster[0].active = PITOFF;
      setconfig(ePIT,{});
      return;
    }
/*
    else if (str == "pittest") {
      pitMaster[0].active = AUTO;
      pitMaster[0].set = 110;
      pitMaster[0].channel = 1;
      pitMaster[0].pid = 0;
      pitMaster[1].active = AUTO;
      pitMaster[1].set = 30;
      pitMaster[1].channel = 2;
      pitMaster[1].pid = 1;
      
      return;
    }
*/

    // CHECK HTTP UPDATE
    else if (str == "checkupdate") {
      update.state = -1;
      return;
    }
    
    // Set V2
    else if (str == "v2") {
      sys.hwversion = 2;
      setconfig(eSYSTEM,{}); 
      return;
    }

    // Test Wifi Flash Clear
    else if (str == "erasewifi") {
      EraseWiFiFlash();
      return;
    }
    
  }

  IPRINTP("You entered: >");
  DPRINT(buffer);
  DPRINTPLN("<");
  DPRINTPLN("Unkwown command");     // Befehl nicht erkannt  
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




