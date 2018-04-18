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

     // UPDATE auf bestimmte Version
    else if (command == "update") {
      String payload((char*)buffer);
      if (payload.indexOf("v") == 0) {
        sys.getupdate = payload;  // kein Speichern, da während des Updates eh gespeichert wird
        sys.update = 1;  
      } else  {IPRINTPLN("Update unbekannt!");}
      return;    
    }
  
  } else {
  
  
    // GET HELP
    if (str == "help") {
      Serial.println();
      Serial.println(F("Syntax: \"command\":{\"Attribut\":\"Value\"]}"));
      Serial.println();
      return;
    }

    else if (str == "data") {
      Serial.println(cloudData(false));
      return;
    }
  
    else if (str == "settings") {
      Serial.println(cloudSettings());
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
      wifi.mode = 5;
      sys.restartnow = true;
      return;
    }

    else if (str == "stopwifi") {
      wifi.mode = 3;  // Turn Wifi off
      return;
    }
  
    else if (str == "configreset") {
      nanoWebHandler.configreset();
      return;
    }

    else if (str == "piepser") {
      Serial.println("Piepsertest");
      piepserON();
      delay(2000);
      piepserOFF();
      return;
    }

    // RESTART SYSTEM
    else if (str == "restart") {
      sys.restartnow = true;
      return;
    }

    // LET ESP SLEEP
    else if (str == "sleep") {
      display.displayOff();
      ESP.deepSleep(0);
      delay(100); // notwendig um Prozesse zu beenden
    }

    // Reset PITMASTER PID
    else if (str == "setPID") {
      set_pid(0);  // Default PID-Settings
      if (setconfig(ePIT,{})) {IPRINTPLN("r:pm");}
      return;
    }

    // AUTOTUNE
    else if (str == "autotune") {
      startautotunePID(5, true, 40, 40L*60L*1000L, 0);
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




