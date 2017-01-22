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

    ANALOG/DIGITAL-WANDLUNG:
    - kleinster digitaler Sprung 1.06 V/1024 = 1.035 mV - eigentlich 1.0V/1024
    - Hinweis zur Abweichung: https://github.com/esp8266/Arduino/issues/2672
    -> ADC-Messspannung = Digitalwert * 1.035 mV
    - Spannungsteiler (47k / 10k) am ADC-Eingang zur 
    - Transformation von BattMin - BattMax in den Messbereich von 0 - 1.06V 
    -> Batteriespannung = ADC-Messspannung * (47+10)/10 
    -> Transformationsvariable Digitalwert-to-Batteriespannung: Battdiv = 1.035 mV * 5.7
    
    HISTORY:
    0.1.00 - 2016-12-30 initial version
    0.2.00 - 2017-01-04 add inactive channels
    
 ****************************************************/


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize Sensors
byte set_sensor() {

  // THERMOCOUPLE
  #if KTYPE
  pinMode(THERMOCOUPLE_CS, OUTPUT);
  digitalWrite(THERMOCOUPLE_CS, HIGH);
  SPI.begin();
  #endif

  // MAX1161x
  byte reg = 0xA0;    // A0 = 10100000
  // page 14
  // 1: setup mode
  // SEL2:0 = Reference (Table 6)
  // external(1)/internal(0) clock
  // unipolar(0)/bipolar(1)
  // 0: reset the configuration register to default
  // 0: dont't care

  Wire.beginTransmission(MAX1161x_ADDRESS);
  Wire.write(reg);
  byte error = Wire.endTransmission();
  return error;
  
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Reading ADC-Channel Average
int get_adc_average (byte ch) {  
  // Get the average value for the ADC channel (ch) selected. 
  // MAX11613/15 samples the channel 8 times and returns the average.  
  // Setup byte required: 0xA0 
  
  byte config = 0x21 + (ch << 1);   //00100001 + ch  // page 15 
  // 0: config mode
  // 01: SCAN = Converts the ch eight times
  // 0000: placeholder ch
  // 1: single-ended

  Wire.beginTransmission(MAX1161x_ADDRESS);
  Wire.write(config);  
  Wire.endTransmission();
  
  Wire.requestFrom(MAX1161x_ADDRESS, 2);
  word regdata = (Wire.read() << 8) | Wire.read();

  return regdata & 4095;
}


#if KTYPE

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Reading Temperature KTYPE
double get_thermocouple(void) {

  int32_t dd = 0;

  digitalWrite(THERMOCOUPLE_CS, LOW);
  delay(1);

  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));

  dd = SPI.transfer(0);
  dd <<= 8;
  dd |= SPI.transfer(0);
  dd <<= 8;
  dd |= SPI.transfer(0);
  dd <<= 8;
  dd |= SPI.transfer(0);

  SPI.endTransaction();

  digitalWrite(THERMOCOUPLE_CS, HIGH);

  if (dd & 0x7) {
    #ifdef DEBUG
    //Serial.println("No thermocouple!");
    #endif
    return INACTIVEVALUE; 
  }

  if (dd & 0x80000000) {
    // Negative value, drop the lower 18 bits and explicitly extend sign bits.
    dd = 0xFFFFC000 | ((dd >> 18) & 0x00003FFFF);
  }
  else {
    // Positive value, just drop the lower 18 bits.
    dd >>= 18;
  }
  
  double vv = dd;

  // Temperature in Celsius
  vv *= 0.25;
  return vv;
}

#endif


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Reading Battery Voltage
void get_Vbat() 
{
  // Digitalwert transformiert in Batteriespannung in mV
  int voltage = analogRead(ANALOGREADBATTPIN);

  // Ladevorgang erkennen
  if (voltage < 10) {
    LADEN = true;
    return;
  }
  else LADEN = false;

  // Transformation Digitalwert in Batteriespannung
  voltage = voltage * BATTDIV; 
       
  // Batteriespannung wird in Prozent umgerechnet und in einen Buffer geschrieben
  // da die gemessene Spannung leicht schwankt, wahrscheinlich je nach aktuellem Energieverbrauch
  // wird die Batteriespannung als Mittel (Median) aus 8 Messungen ausgegeben
  
  median_add(((voltage - BATTMIN)*100)/(BATTMAX - BATTMIN));
  BatteryPercentage = median_get();
  // Schwankungen verschiedener Batterien ausgleichen
  if (BatteryPercentage > 100) BatteryPercentage = 100;
  
  #ifdef DEBUG
    Serial.printf("[INFO]\tBattery voltage:%umV\tcharge:%u%%\r\n", voltage, BatteryPercentage); 
  #endif

  if (BatteryPercentage < 0) {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display.drawString(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/3, "LOW BATTERY");
    display.drawString(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/3, "PLEASE SWITCH OFF");
    display.display();
    //delay(5000);
    //display.displayOff();
    ESP.deepSleep(0);
    delay(100); // notwendig um Prozesse zu beenden
  }
}





