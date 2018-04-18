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
    
    wifi.mode = 2;                    // WiFi-Mode = AP
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
  
  check_http_update();
  
  if (question.typ == SYSTEMSTART) {
    displayblocked = false;       // Close Start Screen
    question.typ = NO;
  }
  
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("nicht verbunden");
  
  if (WiFi.getMode() == 3)  wifi.mode = 2;    // AP
  else wifi.mode = 0;                         // No Wifi

  if (holdssid.hold == 2 && (millis() - holdssid.connect > 5000)) {
    wifi.revive = true;      // Nach fehlgeschlagenem Versuch wiederbeleben
    holdssid.hold == 0;
  } else {
    
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
// MultiWifi
void multiwifi() {
  
  if (wifi.savedlen > 0) {  // mehr als 1 Datensatz
    if (millis() - wifi.reconnecttime > 10000 || wifi.reconnecttime == 0) {    // Wifi Daten vorhanden
      if (wifi.savecount < wifi.savedlen) {     // Daten durchlaufen
        Serial.print("TRY: ");
        holdssid.ssid = wifi.savedssid[wifi.savecount];
        holdssid.pass = wifi.savedpass[wifi.savecount];
        Serial.println(holdssid.ssid);
        connectWiFi();
        wifi.savecount++;
      } else {
        wifi.reconnecttime = millis();
        wifi.savecount = 0;   // zurücksetzen
      }
    } //else Serial.println("Warte");
  } 
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
// WiFi Monitoring
void wifimonitoring() {

  // Verzögerte Speicher-Aktion:  
  switch (sys.control) {

    case 2:         // gespeicherte Daten verwendet
      modifywifi(false);           // Umsortierung des Wifi-Speichers einleiten
      wifi.savecount = 0;          // Wifi Liste Counter zurücksetzen   
      break;

    case 3:                        // neue Daten wurden verwendet
      modifywifi(true);            // Daten im Wifi-Speicher ablegen, falls nicht doppelt
      break;       
  }

  sys.control = 0;

  // Systemstart
  if (WiFi.status() == WL_IDLE_STATUS) {
    multiwifi();
    //Serial.println(connectionStatus(WiFi.status()));
  }
  
  // Verbindung verloren, ESP versucht selbst reconnect, aber wenn mehr als ein Netz im Speicher dann rotieren
  if ((WiFi.status() == WL_NO_SSID_AVAIL) || wifi.revive) { // Network not availible
    if (wifi.savedlen > 1)    // mehr als ein Wifi im Speicher
      multiwifi();
  }
  
  if (holdssid.hold == 1) {                               // neue Verbindung
    if (millis() - holdssid.connect > 1000) {             // mit Verzögerung um den Request zu beenden
      //holdssid.connect = 0;
      holdssid.hold = 2;
      connectWiFi();
    }
    
  } else if (wifi.mode == 3 || wifi.mode == 4) {    // stop wifi
    stop_wifi();
    
  } else if (wifi.mode == 1 & wifi.disconnectAP) {                    // AP abschalten
    uint8_t client_count = wifi_softap_get_station_num();
    if (!client_count) {
      wifi.disconnectAP = false;
      stopAP();
      IPRINTPLN("AP: Closed");
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

 
