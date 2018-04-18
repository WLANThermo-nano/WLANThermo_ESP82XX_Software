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

 // HELP: https://github.com/bblanchon/ArduinoJson

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initalize EEPROM
void setEE() {
  
  // EEPROM Sector: 0xFB, ab Sector 0xFC liegen System Parameter
  
  IPRINTP("EEPROM: 0x"); // letzter Sector von APP2
  DPRINT((((uint32_t)&_SPIFFS_end - 0x40200000) / SPI_FLASH_SEC_SIZE),HEX);
  DPRINTP(" (");
  DPRINT(EEPROM_SIZE, DEC);  // ESP.getFreeSketchSpace()
  DPRINTPLN("B)");
  
  EEPROM.begin(EEPROM_SIZE);

  // WIFI SETTINGS:         0    - 300
  // SYSTEM SETTINGS:       300  - 680
  // CHANNEL SETTINGS:      680  - 1180
  // THINGSPEAK SETTINGS:   1180 - 1600
  // PITMASTER SETTINGS:    1600 - 2300
  
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Write to EEPROM
void writeEE(const char* json, int len, int startP) {
  
  IPRINTP("wEE: (");
  DPRINT(len);
  DPRINTP(") ");
  DPRINTLN(json);
  
  for (int i = startP; i < (startP+len); ++i) {
    EEPROM.write(i, json[i-startP]);
  } 
  EEPROM.commit();
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Read from EEPROM
void readEE(char *buffer, int len, int startP) {
  
  for (int i = startP; i < (startP+len); ++i) {
    buffer[i-startP] = char(EEPROM.read(i));
  }
  
  IPRINTP("rEE: ");
  DPRINTLN(buffer);
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Clear EEPROM
void clearEE(int len, int startP) {  
  
  IPRINTF("cEE: %u to: %u\r\n", startP, startP+len-1); 

  for (int i = startP; i < (startP+len); ++i) {
    EEPROM.write(i, 0);
  } 
  EEPROM.commit();
}



// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Check Memory Sectors
void check_sector() {

  // https://github.com/esp8266/Arduino/blob/master/cores/esp8266/Esp.cpp
  freeSpaceStart = (ESP.getSketchSize() + FLASH_SECTOR_SIZE - 1) & (~(FLASH_SECTOR_SIZE - 1));
  freeSpaceEnd = (uint32_t)&_SPIFFS_start - 0x40200000 - FLASH_SECTOR_SIZE;
  log_sector = freeSpaceStart/SPI_FLASH_SEC_SIZE;

  IPRINTP("FV: ");
  DPRINTLN(FIRMWAREVERSION);
  IPRINTP("SKETCH: 0x01 (");
  DPRINT((ESP.getSketchSize() + FLASH_SECTOR_SIZE - 1)/1024);
  DPRINTPLN("K)");
  //IPRINTP("DATALG: 0x");
  //DPRINT(log_sector,HEX);
  //DPRINTP(" (");
  //DPRINT((freeSpaceEnd - freeSpaceStart)/1024, DEC);  // ESP.getFreeSketchSpace()
  //DPRINTPLN("K)");
    
  //DPRINTLN(ESP.getFlashChipRealSize()/1024);
  //DPRINTLN(ESP.getFlashChipSize() - 0x4000);
    
}




