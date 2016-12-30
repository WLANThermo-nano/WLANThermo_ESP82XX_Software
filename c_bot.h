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
    
 ****************************************************/

#ifdef THINGSPEAK

WiFiClient THINGclient;;

void sendData() {

  String apiKey = THINGSPEAK_KEY;
  const char* server = "api.thingspeak.com";

  if (THINGclient.connect(server,80)) { // "184.106.153.149" or api.thingspeak.com
   
    String postStr = apiKey;
    postStr +="&1=";
    postStr += String(ch[0].temp,1);
    postStr +="&2=";
    postStr += String(ch[1].temp,1);
    postStr +="&3=";
    postStr += String(ch[2].temp,1);
    postStr +="&4=";
    postStr += String(ch[3].temp,1);
    postStr +="&5=";
    postStr += String(ch[4].temp,1);
    postStr +="&6=";
    postStr += String(ch[5].temp,1);
    postStr +="&7=";
    postStr += String(ch[5].temp,1);
    postStr +="&8=";
    postStr += String(BatteryPercentage);

    // Sendedauer: 1s  
    THINGclient.print("POST /update HTTP/1.1\n");
    THINGclient.print("Host: api.thingspeak.com\n");
    THINGclient.print("Connection: close\n");
    THINGclient.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
    THINGclient.print("Content-Type: application/x-www-form-urlencoded\n");
    THINGclient.print("Content-Length: ");
    THINGclient.print(postStr.length());
    THINGclient.print("\n\n");
    THINGclient.print(postStr);
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
    IPAddress server(149,154,167,198);
    Serial.print(".... ");

    // Verbindung aufgebaut
    if (!TELEGRAMMclient.connect(server, 443)) {  
        Serial.println("connection failed");
        return mess;
    }

    // Command senden
    TELEGRAMMclient.println("GET /"+command);

    // wenn keine Antwort abbrechen
    unsigned long timeout = millis();
    while (TELEGRAMMclient.available() == 0) {
      if (millis() - timeout > 3000) {
        Serial.println(">>> Client Timeout !");
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
    Serial.println("SEND Message ");
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
            Serial.print("Message delivred: \"");
            Serial.print(text);
            Serial.println("\"");
            break;
          }
        }
    }
    if (sent==false) Serial.println("Message not delivered");
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
  
    Serial.print("GET Update Messages up to: ");
    
    String command="bot"+_token+"/getUpdates?offset="+offset;
    String mess=connectToTelegram(command);       //recieve reply from telegram.org
    
    //String mess = "";
    Serial.println(offset);
    
    //Serial.println(command);
    //Serial.println(mess);
    
    if (mess!="") {

      // Ergebnisse auslesen falls neue Message bekommen
      DynamicJsonBuffer jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject(mess);

      int ii = 0;
      while (root["result"][ii]["update_id"] != 0) {

        long up_id = root["result"][ii]["update_id"];
        
        Serial.print("new message: ");
        Serial.println(up_id);
        id = String(up_id+1);   // Nachrichten-Counter hochzaehlen
        
        strcpy(userData->name, root["result"][ii]["message"]["from"]["last_name"]);
        strcpy(userData->text, root["result"][ii]["message"]["text"]);
        strcpy(userData->chat_id, root["result"][ii]["message"]["chat"]["id"]);
        
        Serial.print("Name = ");
        Serial.println(userData->name);
        Serial.print("Chat = ");
        Serial.println(userData->chat_id);
        Serial.print("Text = ");
        Serial.println(userData->text);
        Serial.println();

        // Nachricht verarbeiten
        Bot_ExecMessages(userData);
        yield();    // Stability
        delay(500); // Stability
  
      
        ii++;
      }
      
      if (ii == 0) {
        Serial.println("no new messages");
        Serial.println();
        strcpy(userData->name, "0");
        strcpy(userData->chat_id, "0");
        strcpy(userData->text, "0");  
      }  
    }

    if (mess=="") {     
        Serial.println("failed to update");
        return;
    }

}
#endif


