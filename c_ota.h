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
    0.2.00 - 2017-01-04 add setHostname
    
 ****************************************************/

#ifdef OTA

#include <ArduinoOTA.h>           // OTA

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Configuration OTA
void set_ota(){

  String hostname = HOSTNAME;
  hostname += String(ESP.getChipId(), HEX);
  ArduinoOTA.setHostname((const char *)hostname.c_str());

  ArduinoOTA.onStart([]() {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display.drawString(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2 - 10, "OTA Update");
    display.display();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    display.drawProgressBar(4, 32, 120, 8, progress / (total / 100) );
    display.display();
  });

  ArduinoOTA.onEnd([]() {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display.drawString(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2, "Restart");
    display.display();
  });

  #ifdef DEBUG
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("Error[%u]: ", error);
	
		switch (error) {
			case OTA_AUTH_ERROR:
				Serial.println("Auth Failed");
				break;
			case OTA_BEGIN_ERROR:
				Serial.println("Connect Failed");
				break;
			case OTA_CONNECT_ERROR:
				Serial.println("Connect Failed");
				break;
			case OTA_RECEIVE_ERROR:
				Serial.println("Receive Failed");
				break;
			case OTA_END_ERROR:
				Serial.println("End Failed");
				break;
			default:
				Serial.println("OTA unknown ERROR");
				break;
		}
	});
  #endif

}


void check_ota_sector() {
  uint8_t pla = system_upgrade_userbin_check();
  Serial.print("Current Sector: ");
  Serial.println(pla);
}

unsigned char meinsatz[64] = "Ich nutze ab jetzt den Flash Speicher für meine Daten!\n";
unsigned char meinflash[64];

void write_flash() {
  
  spi_flash_erase_sector(0xD6);       // 0x81 bis 0xD6
  spi_flash_write(0xD6000, (uint32 *) meinsatz, sizeof(meinsatz));
  
}

void read_flash() {
  spi_flash_read(0xD6000, (uint32 *) meinflash, sizeof(meinflash));
  //os_printf("%s", meinflash); 
  for (int i=0; i < 64; i++) {
    char test = meinflash[i];
    Serial.print(test);
  }
  Serial.println("");
}
#endif



