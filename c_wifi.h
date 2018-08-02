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

    QUELLE:
    - Wifi Event Handling: https://github.com/esp8266/Arduino/pull/2119
    https://github.com/kzyapkov/Arduino-ESP8266/blob/master/doc/esp8266wifi/generic-class.md
    
 ****************************************************/

/*
WiFiEventHandler onStationModeConnected(std::function<void(const WiFiEventStationModeConnected&)>);
WiFiEventHandler onStationModeDisconnected(std::function<void(const WiFiEventStationModeDisconnected&)>);
WiFiEventHandler onStationModeAuthModeChanged(std::function<void(const WiFiEventStationModeAuthModeChanged&)>);
WiFiEventHandler onStationModeGotIP(std::function<void(const WiFiEventStationModeGotIP&)>);
WiFiEventHandler onStationModeDHCPTimeout(std::function<void(void)>);
WiFiEventHandler onSoftAPModeStationConnected(std::function<void(const WiFiEventSoftAPModeStationConnected&)>);
WiFiEventHandler onSoftAPModeStationDisconnected(std::function<void(const WiFiEventSoftAPModeStationDisconnected&)>);
*/
// 0 = WIFI_OFF, 1 = WIFI_STA, 2 = WIFI_AP, 3 = WIFI_AP_STA
long timerAPstart;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Configuration + Start AP-Mode
void set_AP() {
  
    WiFi.mode(WIFI_AP_STA);     // während AP wird ständig versucht mit STA zu verbinden
    //WiFi.mode(WIFI_AP);       // nur AP, keine Verbindungsversuche
    delay(100);                 // sauberes Umschalten

    IPAddress local_IP(192,168,66,1), gateway(192,168,66,1), subnet(255,255,255,0);
    WiFi.softAPConfig(local_IP, gateway, subnet);
    WiFi.softAP(sys.apname.c_str(), APPASSWORD, 5);   // Channel 5

    IPRINTP("AP: "); DPRINTLN(sys.apname);
    IPRINTP("AP IP: "); DPRINTLN(WiFi.softAPIP());
    
    //wifi.mode = 2;                    // WiFi-Mode = AP
    wifi.mode = 6;  //0                    // Wifi-Mode erstmal OFF
    wifi.disconnectAP = false;        // wait with disconnect

    // Use Autoreconnection after this for Connection with WiFi
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// WiFi Handler
void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  
  DPRINTLN();
  IPRINTP("STA: "); DPRINTLN(WiFi.SSID());
  IPRINTP("IP: "); DPRINTLN(WiFi.localIP());

  if (WiFi.getMode() > 1) wifi.disconnectAP = true;             // Close AP-Mode
  wifi.mode = 1;                                                // WiFi-Mode = STA
  
  connectToMqtt();                 // Start MQTT

  // Änderung an den Wifidaten: muss sortiert und gespeichert werden?
  if (holdssid.hold && WiFi.SSID() == holdssid.ssid) {
    sys.control = 3;               // neue Wifi-Daten erhalten und verbunden
  } else if (WiFi.SSID() != wifi.savedssid[0]){
    sys.control = 2;              // nach Verbindungsverlust neues Netz aus dem Speicher benutzt: Speicher umsortieren
  }
  
  holdssid.hold = 0;            // Handler für neues Wifi zurücksetzen
  holdssid.connect = 0;         // Handler für neues Wifi zurücksetzen
  wifi.revive = false;

  timerAPstart = 0;
  wifi.savecount = 0;           // Liste beim nächsten Verlust von vorne rotieren (liste wurde umsortiert)
  
  check_http_update();
  
  if (question.typ == SYSTEMSTART) {
    displayblocked = false;       // Close Start Screen
    question.typ = NO;
  }
  
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("wifi: disconnect");
  
  //if (WiFi.getMode() == 3)  wifi.mode = 2;    // AP
  //else wifi.mode = 0;                         // No Wifi
  wifi.mode = 6; 

  // Anmeldeversuch fehlgeschlagen
  if (holdssid.hold == 2 && (millis() - holdssid.connect > 5000)) {
    wifi.revive = true;      // Nach fehlgeschlagenem Versuch alte Verbindung wiederbeleben
    holdssid.hold == 0;
  }
  //pmqttClient.disconnect();
}

void onsoftAPDisconnect(const WiFiEventSoftAPModeStationDisconnected& event) {
  Serial.print("NO AP: ");
  Serial.println(WiFi.getMode());
}

void onDHCPTimeout() {
  //Serial.println("nicht verbunden");
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Configuration WiFi
void set_wifi() {

  WiFi.disconnect();    // wenn das, dann kein reconnection nach neustart
  //WiFi.persistent(false); // wenn das davor, wird Flash nicht bei disconnect gelöscht
  WiFi.persistent(false);  // damit werden aber auch nie Daten in den Wifi Flash gespeichert

  // Include WiFi Handler
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
  softAPDisconnectHandler = WiFi.onSoftAPModeStationDisconnected(onsoftAPDisconnect);

  WiFi.hostname(sys.host);
  IPRINTLN("Hostname: " + sys.host);
  
  holdssid.hold = 0;
  holdssid.connect = false;
  wifi.savecount = 0;
  wifi.revive = false;

  wifi.reconnecttime = 0; // Verbindungsaufbau

  if (checkResetInfo()) {
    question.typ = SYSTEMSTART;
    drawConnect();                          // Start screen
  }
  
  set_AP();                               // Start AP-Mode
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Connect WiFi with saved Data
void connectWiFi() {

    WiFi.begin(holdssid.ssid.c_str(), holdssid.pass.c_str());
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Get RSSI
void get_rssi() {
  
  wifi.rssi = WiFi.RSSI();
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Stop AP-Mode
void stopAP() {
  //WiFi.softAPdisconnect();    // sollte eigentlich auch stoppen
  //WiFi.disconnect();          // nicht benötigt
  WiFi.mode(WIFI_STA);
  delay(100);
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Take AP-Mode
void takeAP() {
  WiFi.mode(WIFI_AP);
  delay(100);
  Serial.println("wifi: AP");
  wifi.mode = 2;
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ModifyWifi
void modifywifi(bool neu) {

  if (neu || ((wifi.savedlen > 1) && (WiFi.SSID() != wifi.savedssid[0]))) { 
  
    if (!modifyconfig(eWIFI, neu)) {
      IPRINTPLN("f:wifi");        // Failed to save
    } else {
      IPRINTPLN("s:Wifi");        // Saved
      loadconfig(eWIFI,0);          // temporären Speicher aktualisieren
    } 
  }
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// MultiWifi
void multiwifi() {

 
    if (millis() - wifi.reconnecttime > 15000 || wifi.reconnecttime == 0) {    // Wifi Daten vorhanden
    // wifi.reconnecttime == 0 bei Systemstart

      wifi.reconnecttime = millis();
      Serial.println("wifi: multi");
    
      if (wifi.savecount < wifi.savedlen) {     // Daten durchlaufen
        
        holdssid.ssid = wifi.savedssid[wifi.savecount];
        holdssid.pass = wifi.savedpass[wifi.savecount];
        Serial.print("TRY: ");
        Serial.println(holdssid.ssid);
        connectWiFi();
        wifi.savecount++;
        
      } else {
        // Liste einmal durchgegangen, dann AP starten
        // Listenzähler nicht zurücksetzen, sonst Verbindungsversuch bei Aufruf von Wifiscan (Idle status)
        wifi.takeAP = true;
      }
    } //"Warte"
 
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// WiFi Monitoring
void wifimonitoring() {

  // Verzögerte Speicher-Aktion:  
  switch (sys.control) {

    case 2:         // gespeicherte Daten verwendet
      modifywifi(false);           // Umsortierung des Wifi-Speichers einleiten
      //wifi.savecount = 0;          // Wifi Liste Counter zurücksetzen   
      break;

    case 3:                        // neue Daten wurden verwendet
      modifywifi(true);            // Daten im Wifi-Speicher ablegen, falls nicht doppelt
      break;       
  }

  sys.control = 0;

  /*

  // Systemstart ohne Wifi-Daten
  // auch wenn in AP und dann Wifiscan ausgeführt
  if (WiFi.status() == WL_IDLE_STATUS) {
    multiwifi();
    Serial.println(connectionStatus(WiFi.status()));
  }

  // Systemstart mit Wifi-Daten oder
  // Verbindung verloren, ESP versucht selbst reconnect, aber wenn mehr als ein Netz im Speicher dann nach 10s rotieren
  // oder wenn nach gewisser Zeit keine Verbindung, dann nur noch AP
  if ((WiFi.status() == WL_NO_SSID_AVAIL) || wifi.revive) { // Network not availible
    //if (timerAPstart == 0) timerAPstart = millis();
    
    //if (wifi.savedlen > 0 && (millis() - timerAPstart < 60000))  
    if (!wifi.takeAP)   
      multiwifi();
    else {
      WiFi.mode(WIFI_AP);
      delay(100);
      Serial.println("Nur AP");
      wifi.revive = 0;
      wifi.mode = 2;
    } 
      
  } */

  if (wifi.mode == 6) {

    if (!wifi.takeAP)   
      multiwifi();
    else {
      takeAP();
      wifi.revive = 0;
      
    } 
    
  }

  // nach Verbindungsversuch zuerst Status "Disconnect" (3x) danach wechsel in "No SSID".
  // Status wenn komplett in AP: "Disconnect"

  // nach 5 min nochmal multiwifi durchführen

  // Bei Eingabe neuer Wifi-Daten (setnetwork)
  if (holdssid.hold == 1) {                               // neue Verbindung
    if (millis() - holdssid.connect > 1000) {             // mit Verzögerung um den Request zu beenden
      holdssid.hold = 2;                                  // anzeigen, dass neue Verbindung
      Serial.println("wifi: new");
      wifi.mode = 7;
      connectWiFi();
    }

  } else if (holdssid.hold == 2) {
    if (millis() - holdssid.connect > 10000)   
      wifi.mode = 6;

  // Wifi-Stop-Prozess (zweistufig)
  } else if (wifi.mode == 3 || wifi.mode == 4) {    // stop wifi
    stop_wifi();

  // AP nach erfolgreicher Verbindung zur STA abschalten
  } else if (wifi.mode == 1 & wifi.disconnectAP) {                    // AP abschalten
    uint8_t client_count = wifi_softap_get_station_num();
    if (!client_count) {
      wifi.disconnectAP = false;
      stopAP();
      IPRINTPLN("wifi: STA");
    }
  }
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Stop WiFi
void stop_wifi() {

  if (wifi.mode == 4) {
    wifi.turnoffAPtimer = millis();
    wifi.mode = 3;
    return;
  }
  
  if (millis() - wifi.turnoffAPtimer > 1000) {
    IPRINTPLN("Stop Wifi");
    pmqttClient.disconnect();
    wifi_station_disconnect();
    wifi_set_opmode(NULL_MODE);
    wifi_set_sleep_type(MODEM_SLEEP_T);
    wifi_fpm_open();
    wifi_fpm_do_sleep(0xFFFFFFF);
    //WiFi.disconnect();
    //delay(100); // leider notwendig
    //WiFi.mode(WIFI_OFF);
    delay(100); // leider notwendig

    wifi.mode = 0;
  }
}



// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
String connectionStatus ( int which )
{
    switch ( which )
    {
        case WL_CONNECTED:
            return "Connected";
            break;

        case WL_NO_SSID_AVAIL:
            return "Network not availible";
            break;

        case WL_CONNECT_FAILED:
            return "Wrong password";
            break;

        case WL_IDLE_STATUS:
            return "Idle status";
            break;

        case WL_DISCONNECTED:
            return "Disconnected";
            break;

        default:
            return "Unknown";
            break;
    }
}

void EraseWiFiFlash() {

  //CONFIG_WIFI_SECTOR 0x7E
  // https://github.com/igrr/atproto/blob/master/target/esp8266/config_store.c

  noInterrupts();
  if (spi_flash_erase_sector(0x7E) == SPI_FLASH_RESULT_OK)
  {
    Serial.println(F("[FLASH] Wifi clear"));
    delay(10);
  }
  interrupts();
}

 
