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
    0.2.00 - 2016-12-30 implement ChannelData
    0.2.01 - 2017-01-04 optimize Thingspeak Communication
    
 ****************************************************/

#ifdef THINGSPEAK

WiFiClient THINGclient;;

void sendData() {

  unsigned long vorher = millis();
   
  String apiKey = THINGSPEAK_KEY;
  const char* server = "api.thingspeak.com";

  // Verbindungsaufbau: ~120 ms
  if (THINGclient.connect(server,80)) {

    String postStr = apiKey;

    for (int i=0; i < CHANNELS; i++)  {
      if (ch[i].temp!=INACTIVEVALUE) {
        postStr += "&";
        postStr += String(i+1);
        postStr += "=";
        postStr += String(ch[i].temp,1);
      }
    }
    postStr +="&8=";
    postStr += String(BatteryPercentage);

    // Sendedauer: ~115ms  
    THINGclient.print("POST /update HTTP/1.1\nHost: api.thingspeak.com\nConnection: close\nX-THINGSPEAKAPIKEY: "
                      +apiKey+"\nContent-Type: application/x-www-form-urlencoded\nContent-Length: "
                      +postStr.length()+"\n\n"+postStr);

    #ifdef DEBUG
      Serial.printf("[INFO]\tSend to Thingspeak: %ums\r\n", millis()-vorher); 
    #endif
  }

  THINGclient.stop();
        
}

void sendMessage(int ch, int count) {

  unsigned long vorher = millis();
   
  String apiKey = "N08GS23IX7J2EA0E";
  const char* server = "api.thingspeak.com";

  // Verbindungsaufbau: ~120 ms
  if (THINGclient.connect(server,80)) {
 
    // Sendedauer: ~115ms  

    //"GET /update?key=IJO0IVY7KD6PAA5E&field1=";

    // We now create a URI for the request
    String url = "/apps/thinghttp/send_request?api_key=";
    url += apiKey;
    url += "&message=";
    if (count) url += "hoch";
    else url += "niedrig";
    url += "&ch=";
    url += String(ch);
    
    //THINGclient.print("GET " + url + "&headers=false" + " HTTP/1.1\r\n" + "Host: " + server + "\r\n" + "Connection: close\r\n\r\n");
    THINGclient.print("GET " + url + " HTTP/1.1\r\n" + "Host: " + server + "\r\n" + "Connection: close\r\n\r\n");
  
  
  //delay(500);
 
  // Read all the lines of the reply from server and print them to Serial
  //while(THINGclient.available()){
    //String line = THINGclient.readStringUntil('\r');
    //Serial.println(line);
  //}

    #ifdef DEBUG
      Serial.printf("[INFO]\tSend to Thingspeak: %ums\r\n", millis()-vorher); 
    #endif
  }

  THINGclient.stop();
        
}


#endif

