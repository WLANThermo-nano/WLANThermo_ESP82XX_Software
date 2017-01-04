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
const int timeZone = 1;     // Central European Time

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Connect WiFi

void set_wifi() {

  char apname[] = APNAME;
  char appass[] = APPASSWORD;

  IPAddress local_IP(192,168,66,1);
  IPAddress gateway(192,168,66,1);
  IPAddress subnet(255,255,255,0);

  WiFi.mode(WIFI_STA);
  
  #ifdef DEBUG
  Serial.print("Connecting");
  #endif

  // Add Wifi Settings
  for (int i = 0; i < lenwifi; i++) {
    wifiMulti.addAP(wifissid[i].c_str(), wifipass[i].c_str());
  }

  int counter = 0;
  while (wifiMulti.run() != WL_CONNECTED && counter < 8) {
    delay(500);
    #ifdef DEBUG
      Serial.print(".");
    #endif
    drawConnect(3, counter % 3);
    counter++;
  }

  #ifdef DEBUG
    Serial.println();
  #endif
  
  if (WiFi.status() == WL_CONNECTED) {

    #ifdef DEBUG
      Serial.print("WiFi connected to: ");
      Serial.println(WiFi.SSID());
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    #endif
    
    isAP = false;

    udp.begin(2390);  // localPort = 2390;

    #ifdef DEBUG
      Serial.println("Starting UDP");
      Serial.print("Local port: ");
      Serial.println(udp.localPort());
    #endif
    
  }
  else {

    WiFi.mode(WIFI_AP);

    #ifdef DEBUG
      Serial.print("Configuring access point: ");
      Serial.print(APNAME);
      Serial.println(" ...");
    #endif
    
    WiFi.softAPConfig(local_IP, gateway, subnet);
    WiFi.softAP(apname, appass, 5);  // Channel 5

    #ifdef DEBUG
      Serial.print("AP IP address: ");
      Serial.println(WiFi.softAPIP());
    #endif
    
    isAP = true;
    
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
    Serial.println("sending NTP packet...");
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

  #ifdef DEBUG
    Serial.println("Transmit NTP Request");
  #endif
  
  sendNTPpacket(timeServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      #ifdef DEBUG
        Serial.println("Receive NTP Response");
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
    Serial.println("No NTP Response!");
  #endif
  return 0; // return 0 if unable to get the time
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Show time

void printDigits(int digits){
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void digitalClockDisplay(){
  
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(".");
  Serial.print(month());
  Serial.print(".");
  Serial.print(year()); 
  Serial.println(); 
}

