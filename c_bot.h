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

boolean thingspeakshowbattery = true;

#ifdef THINGSPEAK

  WiFiClient THINGclient;
  #define SERVER1 "api.thingspeak.com"

  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Send data to Thingspeak
  void sendData() {

    unsigned long vorher = millis(); 
    String apiKey = THINGSPEAK_KEY;
  
    // Sendedauer: ~120ms  
    if (THINGclient.connect(SERVER1,80)) {

      String postStr = apiKey;

      for (int i = 0; i < 7; i++)  {
        if (ch[i].temp != INACTIVEVALUE) {
          postStr += "&";
          postStr += String(i+1);
          postStr += "=";
          postStr += String(ch[i].temp,1);
        }
      }

      postStr +="&8=";
      if (thingspeakshowbattery) postStr += String(battery.percentage);  // Kanal 8 ist Batterie-Status
      else postStr += String(ch[7].temp,1);

      THINGclient.print("POST /update HTTP/1.1\nHost: api.thingspeak.com\nConnection: close\nX-THINGSPEAKAPIKEY: "
                        +apiKey+"\nContent-Type: application/x-www-form-urlencoded\nContent-Length: "
                        +postStr.length()+"\n\n"+postStr);

      DPRINTF("[INFO]\tSend to Thingspeak: %ums\r\n", millis()-vorher); 
    }

    THINGclient.stop();      
  }


  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Send Message to Telegram via Thingspeak
  void sendMessage(int ch, int count) {

    unsigned long vorher = millis();
    String apiKey = "xxx";

    // Sendedauer: ~120 ms
    if (THINGclient.connect(SERVER1,80)) {
 
      String url = "/apps/thinghttp/send_request?api_key=";
      url += apiKey;
      url += "&message=";
      if (count) url += "hoch";
      else url += "niedrig";
      url += "&ch=";
      url += String(ch);
    
      THINGclient.print("GET " + url + " HTTP/1.1\r\n" + "Host: " + SERVER1 
                        + "\r\n" + "Connection: close\r\n\r\n");
  
      DPRINTF("[INFO]\tSend to Thingspeak: %ums\r\n", millis()-vorher); 
    }

    THINGclient.stop(); 
  }

#endif

