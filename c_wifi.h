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
    0.1.00 - 2016-12-30 initial version
    0.2.00 - 2017-01-03 change NTP time communication
    
 ****************************************************/


ESP8266WiFiMulti wifiMulti;

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

// Initialize NTP
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Connect WiFi

void set_wifi() {

  char apname[] = APNAME;
  char appass[] = APPASSWORD;

  IPAddress local_IP(192,168,66,1);
  IPAddress gateway(192,168,66,1);
  IPAddress subnet(255,255,255,0);

  WiFi.hostname(host);

  #ifdef DEBUG
    Serial.println("[INFO]\tHostname: " + host);
  #endif

  WiFi.mode(WIFI_STA);
  
  #ifdef DEBUG
  Serial.print("[INFO]\tConnecting");
  #endif

  holdssid.hold = false;

  // Add Wifi Settings
  for (int i = 0; i < lenwifi; i++) {
    wifiMulti.addAP(wifissid[i].c_str(), wifipass[i].c_str());
  }
  
  drawConnect();
  int counter = 0;
  while (wifiMulti.run() != WL_CONNECTED && counter < 8) {
    delay(500);
    #ifdef DEBUG
      Serial.print(".");
    #endif
    counter++;
  }

  #ifdef DEBUG
    Serial.println();
  #endif
  
  if (WiFi.status() == WL_CONNECTED) {

    #ifdef DEBUG
      Serial.print("[INFO]\tWiFi connected to: ");
      Serial.println(WiFi.SSID());
      Serial.print("[INFO]\tIP address: ");
      Serial.println(WiFi.localIP());
    #endif
    
    isAP = 0;

    WiFi.setAutoReconnect(true); //Automatisch neu verbinden falls getrennt
 
    udp.begin(2390);  // localPort = 2390;

    #ifdef DEBUG
      Serial.print("[INFO]\tStarting UDP: Local port ");
      Serial.println(udp.localPort());
    #endif
    
  }
  else {

    WiFi.mode(WIFI_AP_STA);

    #ifdef DEBUG
      Serial.print("[INFO]\tConfiguring access point: ");
      Serial.print(APNAME);
      Serial.println(" ...");
    #endif
    
    WiFi.softAPConfig(local_IP, gateway, subnet);
    WiFi.softAP(apname, appass, 5);  // Channel 5

    #ifdef DEBUG
      Serial.print("[INFO]\tAP IP address: ");
      Serial.println(WiFi.softAPIP());
    #endif
    
    isAP = 1;
    disconnectAP = false;
    
  }
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Get RSSI
void get_rssi() {
  
  rssi = WiFi.RSSI();
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address) {
  
  #ifdef DEBUG
    Serial.println("[INFO]\tSending NTP packet...");
  #endif
  
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

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Get NTP time
time_t getNtpTime() {
  
  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP); 

  while (udp.parsePacket() > 0) ; // discard any previously received packets
  
  sendNTPpacket(timeServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 4000) {
    int size = udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      #ifdef DEBUG
        Serial.println("[INFO]\tReceive NTP Response");
      #endif
      udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  #ifdef DEBUG
    Serial.println("[INFO]\tNo NTP Response!");
  #endif
  return 0; // return 0 if unable to get the time
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Show time

String printDigits(int digits){
  String com;
  if(digits < 10) com = "0";
  com += String(digits);
  return com;
}

String digitalClockDisplay(){

  String zeit;
  zeit += printDigits(hour())+":";
  zeit += printDigits(minute())+":";
  zeit += printDigits(second())+" ";
  zeit += String(day())+".";
  zeit += String(month())+".";
  zeit += String(year());
  return zeit;
}


#define FPM_SLEEP_MAX_TIME 0xFFFFFFF
bool awaking = false;


void WIFI_Connect(const char* data[2]) {

  // http://www.esp8266.com/viewtopic.php?f=32&t=8286
  
  //WiFi.disconnect();
  Serial.println("Verbinden mit neuer SSID");
  //WiFi.mode(WIFI_AP_STA);
  WiFi.begin(data[0], data[1]);


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


void stop_wifi() {
  
  #ifdef DEBUG
    Serial.println("[INFO]\tStop Wifi");
  #endif
  
  wifi_station_disconnect();
  wifi_set_opmode(NULL_MODE);
  wifi_set_sleep_type(MODEM_SLEEP_T);
  wifi_fpm_open();
  wifi_fpm_do_sleep(FPM_SLEEP_MAX_TIME);
  delay(100); // leider notwendig

  isAP = 2;
}

void reconnect_wifi() {

  // wake up to use WiFi again
  wifi_fpm_do_wakeup();
  wifi_fpm_close();
  wifi_set_opmode(STATION_MODE);
  wifi_station_connect();
}

int scan_wifi() {

  #ifdef DEBUG
    Serial.print("[INFO]\tWifi Scan: ");
  #endif
  
  int n = WiFi.scanNetworks(false, false);
  // Keine HIDDEN NETWORKS SCANNEN

  // https://github.com/esp8266/Arduino/blob/master/doc/esp8266wifi/scan-class.md#scannetworks
  // https://github.com/esp8266/Arduino/blob/master/doc/esp8266wifi/station-class.md#setautoreconnect
  #ifdef DEBUG
    Serial.print(n);
    Serial.println(" network(s) found");
  #endif
  
  return n;
  
}


struct station_info *stat_info;
struct ip_addr *IPaddress;
IPAddress address;

void dumpClients()
{
  // https://github.com/esp8266/Arduino/issues/2681
  //http://www.esp8266.com/viewtopic.php?f=32&t=5669&sid=a9f40b382551435102f1b5ea3b6ef37c&start=8
  
  Serial.print(" Clients:\r\n");
  stat_info = wifi_softap_get_station_info();
  //uint8_t client_count = wifi_softap_get_station_num()
  while (stat_info != NULL)
  {
    IPaddress = &stat_info->ip;
    address = IPaddress->addr;
    Serial.print("\t");
    Serial.print(address);
    Serial.print("\r\n");
    stat_info = STAILQ_NEXT(stat_info, next);
  } 
}


