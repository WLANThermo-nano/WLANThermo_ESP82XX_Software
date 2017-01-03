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
// Copy of Example NTPClient
unsigned long sendNTPpacket(IPAddress& address) {
  
  Serial.println("sending NTP packet...");
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
// Get current time
// Copy of Example NTPClient
void get_ntp_time() {
  
  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP); 

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);
  
  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("no packet yet");
  }
  else {
    Serial.print("packet received, length=");
    Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);


    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');
    if ( ((epoch % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ( (epoch % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(epoch % 60); // print the second
  }
}

