
/***************************************************
 Copyright (C) 2017  Steffen Ochs, Holger Imbery
 
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


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Send data to private MQTT Broker
void sendpmqtt() {
     
    if (pmqttClient.connected()) {
        
        unsigned long vorher = millis();
        
        for (int i = 0; i < 8; i++)  {
            if (ch[i].temp != INACTIVEVALUE) {
                String adress = F("WLanThermo/");
                adress += sys.host;
                adress += F("/status/");
                adress += "temp";
                adress += String(i+1);
                String postStr = String(ch[i].temp,1);
                pmqttClient.publish(adress.c_str(), charts.P_MQTT_QoS, false, postStr.c_str());
            }
        }
        
        String adress = F("WLanThermo/");
        adress += sys.host;
        adress += F("/status/");
        adress += "voltage";
        String postStr = String(battery.percentage);
        pmqttClient.publish(adress.c_str(), charts.P_MQTT_QoS, false, postStr.c_str());
        
        DPRINTF("[INFO]\tPublish to MQTTbroker at QoS 0: %ums\r\n", millis()-vorher);
        
    } else {
      DPRINTPLN("[INFO]\tNot Connected to MQTT Broker");
      pmqttClient.connect();
    }
}




