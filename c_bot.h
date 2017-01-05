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

#endif


#ifdef TELEGRAM

WiFiClientSecure TELEGRAMMclient;

String _token = BOTTOKEN;
String id = "0";
 
struct UserData {
  char name[16];
  char chat_id[10];
  char text[32];
  };


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Connect to TELEGRAM
String connectToTelegram(String command)  {
    
    String mess="";
        
    // Connect to api.telegram.org       
    const char* server = "api.telegram.org";
 
    #ifdef DEBUG
    Serial.print(".... ");
    #endif
 
    // Verbindung aufgebaut
    if (!TELEGRAMMclient.connect(server, 443)) {  
        #ifdef DEBUG
        Serial.println("connection failed");
        #endif
        return mess;
    }

    // Command senden
    TELEGRAMMclient.println("GET /"+command);

    // wenn keine Antwort abbrechen
    unsigned long timeout = millis();
    while (TELEGRAMMclient.available() == 0) {
      if (millis() - timeout > 3000) {
        #ifdef DEBUG
        Serial.println(">>> Client Timeout !");
        #endif
        TELEGRAMMclient.stop();
        return mess;
      }
    }

    // ansonsten Antwort annehmen
    if(TELEGRAMMclient.available()){
      mess = TELEGRAMMclient.readString();
      //Serial.println(mess);
    }
    
    return mess;
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Send Data to TELEGRAM BOT
void sendMessage(String chat_id, String text, String reply_markup)  {

    bool sent=false;
    #ifdef DEBUG
    Serial.println("SEND Message ");
    #endif
    long sttime=millis();
    while (millis()<sttime+8000) {    // loop for a while to send the message
        String command="bot"+_token+"/sendMessage?chat_id="+chat_id+"&text="+text+"&reply_markup="+reply_markup;
        String mess=connectToTelegram(command);
        
        //Serial.println(mess);

        if (mess!="") {
          DynamicJsonBuffer json;
          JsonObject& ack = json.parseObject(mess);
          sent = ack["ok"];
        
          if (sent==true)   {
            #ifdef DEBUG
            Serial.print("Message delivred: \"");
            Serial.print(text);
            Serial.println("\"");
            #endif
            break;
          }
        }
    }
    #ifdef DEBUG
    if (sent==false) Serial.println("Message not delivered");
    #endif
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// React of TELEGRAM Message
void Bot_ExecMessages(struct UserData* userData) {

    String postStr;
  
    if (strcmp(userData->text, "/temp1")==0) {
        postStr = "Kanal 1: ";
        postStr += String(ch[0].temp,1);
        postStr += " °C";
        sendMessage(userData->chat_id, postStr, "");
    }

    else if (strcmp(userData->text, "/temp2")==0) {
        postStr = "Kanal 2: ";
        postStr += String(ch[1].temp,1);
        postStr += " °C";
        sendMessage(userData->chat_id, postStr, "");
    }
    
    else {
        postStr = "Häh?";
        sendMessage(userData->chat_id, postStr, "");
    }
  
    // All messages have been replied - reset new messages
    strcpy(userData->name, "0");
    strcpy(userData->chat_id, "0");
    strcpy(userData->text, "0");   
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Get TELEGRAM Update
void getUpdates(String offset, struct UserData* userData)  {
  
    #ifdef DEBUG
    Serial.print("GET Update Messages up to: ");
    #endif
    
    String command="bot"+_token+"/getUpdates?offset="+offset;
    String mess=connectToTelegram(command);       //recieve reply from telegram.org
    
    #ifdef DEBUG
    //String mess = "";
    Serial.println(offset);
    //Serial.println(command);
    //Serial.println(mess);
    #endif   
 
    if (mess!="") {

      // Ergebnisse auslesen falls neue Message bekommen
      DynamicJsonBuffer jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject(mess);

      int ii = 0;
      while (root["result"][ii]["update_id"] != 0) {

        long up_id = root["result"][ii]["update_id"];
        
        #ifdef DEBUG
        Serial.print("new message: ");
        Serial.println(up_id);
        #endif
       
        id = String(up_id+1);   // Nachrichten-Counter hochzaehlen
        
        strcpy(userData->name, root["result"][ii]["message"]["from"]["last_name"]);
        strcpy(userData->text, root["result"][ii]["message"]["text"]);
        strcpy(userData->chat_id, root["result"][ii]["message"]["chat"]["id"]);
        
        #ifdef DEBUG
        Serial.print("Name = ");
        Serial.println(userData->name);
        Serial.print("Chat = ");
        Serial.println(userData->chat_id);
        Serial.print("Text = ");
        Serial.println(userData->text);
        Serial.println();
        #endif

        // Nachricht verarbeiten
        Bot_ExecMessages(userData);
        yield();    // Stability
        delay(500); // Stability
  
      
        ii++;
      }
      
      if (ii == 0) {
       
        #ifdef DEBUG
        Serial.println("no new messages");
        Serial.println();
        #endif
        strcpy(userData->name, "0");
        strcpy(userData->chat_id, "0");
        strcpy(userData->text, "0");  
      }  
    }

    if (mess=="") {  
        #ifdef DEBUG
        Serial.println("failed to update");
        #endif
        return;
    }

}
#endif


