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

#ifdef OTA

#include <ArduinoOTA.h>           // OTA

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Configuration OTA
void set_ota(){

  ArduinoOTA.setHostname((const char *)host.c_str());

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

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
void check_ota_sector() {

  // https://github.com/esp8266/Arduino/blob/master/cores/esp8266/Esp.cpp
  freeSpaceStart = (ESP.getSketchSize() + FLASH_SECTOR_SIZE - 1) & (~(FLASH_SECTOR_SIZE - 1));
  freeSpaceEnd = (uint32_t)&_SPIFFS_start - 0x40200000;

  Serial.print("[INFO]\tInitalize SKETCH at Sector: 0x01 (");
  Serial.print((ESP.getSketchSize() + FLASH_SECTOR_SIZE - 1)/1024);
  Serial.println("K)");
  Serial.print("[INFO]\tInitalize DATALG at Sector: 0x");
  Serial.print(freeSpaceStart/SPI_FLASH_SEC_SIZE,HEX);
  Serial.print(" (");
  Serial.print((freeSpaceEnd - freeSpaceStart)/1024, DEC);  // ESP.getFreeSketchSpace()
  Serial.println("K)");
  log_sector = freeSpaceStart/SPI_FLASH_SEC_SIZE;
    
  //uint32_t _sectorStart = (ESP.getSketchSize() / SPI_FLASH_SEC_SIZE) + 1;
  //Serial.println(ESP.getFlashChipRealSize()/1024);
  //Serial.println(ESP.getFlashChipSize() - 0x4000);
    
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Test zum Speichern von Datalog

//unsigned char meinsatz[64] = "Ich nutze ab jetzt den Flash Speicher für meine Daten!\n";
//unsigned char meinflash[64];

void write_flash(uint32_t _sector) {

  //_sector = 0xD6;            // 0x81 bis 0xD6
  noInterrupts();
  if(spi_flash_erase_sector(_sector) == SPI_FLASH_RESULT_OK) {  // ESP.flashEraseSector
    spi_flash_write(_sector * SPI_FLASH_SEC_SIZE, (uint32 *) mylog, sizeof(mylog));  //ESP.flashWrite
    #ifdef DEBUG
    Serial.print("[INFO]\tSpeicherung im Sector: ");
    Serial.println(_sector, HEX);
    #endif
  } else {
    #ifdef DEBUG
    Serial.println("[INFO]\tFehler beim Speichern im Flash");
    #endif
  }
  interrupts(); 
}

void read_flash(uint32_t _sector) {

  noInterrupts();
  spi_flash_read(_sector * SPI_FLASH_SEC_SIZE, (uint32 *) archivlog, sizeof(archivlog));  //ESP.flashRead
  //spi_flash_read(_sector * SPI_FLASH_SEC_SIZE, (uint32 *) meinflash, sizeof(meinflash));
  interrupts();
}

#endif



