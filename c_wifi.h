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


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Connect WiFi

void set_wifi() {

  char appass[] = APPASSWORD;

  IPAddress local_IP(192,168,66,1);
  IPAddress gateway(192,168,66,1);
  IPAddress subnet(255,255,255,0);

  WiFi.hostname(sys.host);
  //WiFi.mode(WIFI_STA);
  
  DPRINTLN("[INFO]\tHostname: " + sys.host);
  DPRINTP("[INFO]\tConnecting");
  
  holdssid.hold = false;
  holdssid.connect = false;

  drawConnect();

  if (lenwifi > 0) {
    if (lenwifi > 1) {
  
      // Add Wifi Settings
      for (int i = 0; i < lenwifi; i++) {
        wifiMulti.addAP(wifissid[i].c_str(), wifipass[i].c_str());
      }
      DPRINTP("_Multi");
      int counter = 0;
      while (wifiMulti.run() != WL_CONNECTED && counter < 8) {
        delay(500);
        DPRINTP(".");
        counter++;
      }
    } else {
    
      WiFi.begin(wifissid[0].c_str(), wifipass[0].c_str());
      int counter = 0;
    
      while (WiFi.status() != WL_CONNECTED && counter < 20) {
        delay(500);
        DPRINTP(".");
        counter++;
      }
    }
  }
  
  
  if (WiFi.status() == WL_CONNECTED) {
        
    isAP = 0;

    WiFi.setAutoReconnect(true); //Automatisch neu verbinden falls getrennt
 
    udp.begin(2390);  // localPort = 2390;

    DPRINTP("[INFO]\tStarting UDP: Local port ");
    DPRINTLN(udp.localPort());
    
  }
  else {

    WiFi.mode(WIFI_AP_STA);

    DPRINTP("[INFO]\tConfiguring access point: ");
    DPRINT(sys.apname);
    DPRINTPLN(" ...");
    
    WiFi.softAPConfig(local_IP, gateway, subnet);
    WiFi.softAP(sys.apname.c_str(), appass, 5);  // Channel 5

    DPRINTP("[INFO]\tAP IP address: ");
    DPRINTLN(WiFi.softAPIP());
    
    isAP = 1;
    disconnectAP = false;
    
  }
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Get RSSI
void get_rssi() {
  
  rssi = WiFi.RSSI();
}

/*
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Send NTP request to the time server
void sendNTPpacket(IPAddress& address) {
  
  DPRINTPLN("[INFO]\tSending NTP packet...");
  
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Get NTP time
time_t getNtpTime() {

  IPAddress timeServerIP;   
  WiFi.hostByName("time.nist.gov", timeServerIP); 

  while (udp.parsePacket() > 0) ; // discard any previously received packets
  sendNTPpacket(timeServerIP);
  
  uint32_t beginWait = millis();
  while (millis() - beginWait < 4000) {
    int size = udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      DPRINTPLN("[INFO]\tReceive NTP Response");
      udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL;
    }
  }
  DPRINTPLN("[INFO]\tNo NTP Response!");
  return 0; // return 0 if unable to get the time
}
*/


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Connect Wifi after Settings Transmission
void WIFI_Connect() {

  // http://www.esp8266.com/viewtopic.php?f=32&t=8286
  
  //WiFi.disconnect();
  DPRINTPLN("[INFO]\tVerbinden mit neuer SSID");
  //WiFi.mode(WIFI_AP_STA);
  WiFi.begin(holdssid.ssid.c_str(), holdssid.pass.c_str());


  /*
  // führt zu einem SOFT WDT RESET, wenn die Zeit > 4000
  uint32_t beginWait = millis();
  while (millis() - beginWait < 2000) {
    if (WiFi.isConnected()) return 1;  
  }
  return 0;
  */

  /*
  // Funktioniert leider nicht im Request, führt zu Exception
  if (WiFi.waitForConnectResult() != WL_CONNECTED) return false;
  return true;
  */
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Scan possible SSIDs
int scan_wifi() {

  DPRINTP("[INFO]\tWifi Scan: ");
  
  int n = WiFi.scanNetworks(false, false);
  // Keine HIDDEN NETWORKS SCANNEN

  // https://github.com/esp8266/Arduino/blob/master/doc/esp8266wifi/scan-class.md#scannetworks
  // https://github.com/esp8266/Arduino/blob/master/doc/esp8266wifi/station-class.md#setautoreconnect
  // http://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/scan-class.html#scannetworksasync
  DPRINT(n);
  DPRINTPLN(" network(s) found");
  
  return n;
  
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// WiFi Monitoring
void wifimonitoring() {
  if (holdssid.connect) {
    if (millis() - holdssid.connect > 1000) {
      WIFI_Connect();
      holdssid.connect = false;
    }
  } else if (isAP == 3 || isAP == 4) {
    stop_wifi();
  } else if (WiFi.status() == WL_CONNECTED & isAP > 0) {
    // Verbindung neu hergestellt, entweder aus AP oder wegen Verbindungsverlust
    if (isAP == 1) disconnectAP = true;
    isAP = 0;
    
    DPRINTP("[INFO]\tWiFi connected to: ");
    DPRINTLN(WiFi.SSID());
    DPRINTP("[INFO]\tIP address: ");
    DPRINTLN(WiFi.localIP());

    question.typ = IPADRESSE;
    drawQuestion(0);

    if (holdssid.hold) {
      holdssid.hold = false;
      const char* data[2];
      data[0] = holdssid.ssid.c_str();
      data[1] = holdssid.pass.c_str();
      if (!modifyconfig(eWIFI,data)) 
        DPRINTPLN("[INFO]\tFailed to save wifi config");
      else  
        DPRINTPLN("[INFO]\tWifi config saved");
    }
    
    WiFi.setAutoReconnect(true); //Automatisch neu verbinden falls getrennt
    
  } else if (WiFi.status() != WL_CONNECTED & isAP == 0) {
    // Nicht verbunden
    DPRINTPLN("[INFO]\tWLAN-Verbindung verloren!");
    isAP = 2;

    // Verlust nach Verbindungsaufbauversuch
    if (holdssid.hold) {
      holdssid.hold = false;
      DPRINTPLN("[INFO]\tMit ehemaligem Wifi verbinden");
      WiFi.mode(WIFI_OFF);
      WiFi.mode(WIFI_STA);
      wifiMulti.run();        // mit vorherigem Wifi verbinden
    }
    
  } else if (isAP == 0 & disconnectAP) {
      uint8_t client_count = wifi_softap_get_station_num();
      if (!client_count) {
        disconnectAP = false;
        WiFi.mode(WIFI_STA);
        DPRINTPLN("[INFO]\tClient hat sich von AP getrennt -> AP abgeschaltet");
      }
  }
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Test
struct station_info *stat_info;
struct ip_addr *IPaddress;
IPAddress address;

void dumpClients()
{
  // https://github.com/esp8266/Arduino/issues/2681
  //http://www.esp8266.com/viewtopic.php?f=32&t=5669&sid=a9f40b382551435102f1b5ea3b6ef37c&start=8
  
  DPRINTP(" Clients:\r\n");
  stat_info = wifi_softap_get_station_info();
  //uint8_t client_count = wifi_softap_get_station_num()
  while (stat_info != NULL)
  {
    IPaddress = &stat_info->ip;
    address = IPaddress->addr;
    DPRINTP("\t");
    DPRINT(address);
    DPRINTP("\r\n");
    stat_info = STAILQ_NEXT(stat_info, next);
  } 
}


#define FPM_SLEEP_MAX_TIME 0xFFFFFFF


void stop_wifi() {

  if (isAP == 4) {
    isAPcount = millis();
    isAP = 3;
    return;
  }
  
  if (millis() - isAPcount > 1000) {
    DPRINTPLN("[INFO]\tStop Wifi");
    pmqttClient.disconnect();
    wifi_station_disconnect();
    wifi_set_opmode(NULL_MODE);
    wifi_set_sleep_type(MODEM_SLEEP_T);
    wifi_fpm_open();
    wifi_fpm_do_sleep(FPM_SLEEP_MAX_TIME);
    //WiFi.disconnect();
    //delay(100); // leider notwendig
    //WiFi.mode(WIFI_OFF);
    delay(100); // leider notwendig

    isAP = 2;
  }
}

void reconnect_wifi() {

  // wake up to use WiFi again
  //wifi_fpm_do_wakeup();
  //wifi_fpm_close();
  //wifi_set_opmode(STATION_MODE);
  //wifi_station_connect();

  WiFi.forceSleepWake();
  WiFi.mode(WIFI_STA);  
  wifi_station_connect();
  //WiFi.begin(ssid, password); 
}

