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

    LITERATUR:
    - https://de.wikipedia.org/wiki/Liste_der_HTTP-Headerfelder
    
 ****************************************************/


// see: https://github.com/me-no-dev/ESPAsyncTCP/issues/18
static AsyncClient * tssettingclient = NULL;
static AsyncClient * tsdataclient = NULL;           // Telegram Channel Data
static AsyncClient * tsalarmclient = NULL;


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize IoT
void set_iot(bool init) {
  
   if (init) {                // clear all
    iot.TS_writeKey = "";     
    iot.TS_httpKey = "";       
    iot.TS_userKey = "";     
    iot.TS_chID = ""; 
    
    iot.P_MQTT_USER = "";
    iot.P_MQTT_PASS = ""; 
    iot.P_MQTT_QoS = 0;
   }
   
   iot.TS_show8 = false;        
   iot.TS_int = INTERVALCOMMUNICATION/4;
   iot.TS_on = false;
   
   iot.P_MQTT_on = false;
   iot.P_MQTT_HOST = "192.168.2.1";
   iot.P_MQTT_PORT = 1883;
   iot.P_MQTT_int = INTERVALCOMMUNICATION/4;
   
   iot.CL_on = false;
   iot.CL_token = newToken();
   iot.CL_int = INTERVALCOMMUNICATION/4;
}

void set_push() {
  pushd.on = 0;
  pushd.token = "";
  pushd.id = "";
  pushd.repeat = 1;
  pushd.service = 0;
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Create Notification Message
String createNote() {

  String postStr;

  if (notification.type > 0) {    // Test Message
    postStr += F("&msg=");
    postStr += F("up");
    postStr += F("&ch=1");
    notification.type = 0;
    
  } else {
    bool limit = notification.limit & (1<<notification.ch);
    postStr += F("&msg=");
    postStr += (limit)?F("up"):F("down");
    postStr += F("&ch=");
    postStr += String(notification.ch+1);    
  } 
  return postStr;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Send Notification
bool sendNote(int check){

  if (check == 0) {
    if(tsalarmclient) return false;                 //client already exists

    tsalarmclient = new AsyncClient();
    if(!tsalarmclient)  return false;               //could not allocate client

    tsalarmclient->onError([](void * arg, AsyncClient * client, int error){
      printClient(THINGHTTPLINK,CLIENTERRROR);
      tsalarmclient = NULL;
      delete client;
    }, NULL);
    
  } else if (check == 2) {          // Senden ueber Server
    
    tsalarmclient->onConnect([](void * arg, AsyncClient * client){
    
      tsalarmclient->onError(NULL, NULL);

      client->onDisconnect([](void * arg, AsyncClient * c){
        printClient(SENDNOTELINK ,DISCONNECT);
        tsalarmclient = NULL;
        delete c;
      }, NULL);

      //send the request
      printClient(SENDNOTELINK,SENDTO);
      String adress = createCommand(GETMETH,SENDNOTE,SENDNOTELINK,MESSAGESERVER,0);
      client->write(adress.c_str());
      Serial.println(adress);
    }, NULL);

    if(!tsalarmclient->connect(MESSAGESERVER, 80)){
      printClient(SENDNOTELINK ,CONNECTFAIL);
      AsyncClient * client = tsalarmclient;
      tsalarmclient = NULL;
      delete client;
    }
  }
  return true;    // Nachricht kann gesendet werden
}

