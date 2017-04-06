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
  #ifdef KTYPE
    // CS notwendig, da nur bei CS HIGH neue Werte im Chip gelesen werden
    pinMode(THERMOCOUPLE_CS, OUTPUT);
    digitalWrite(THERMOCOUPLE_CS, HIGH);
  #endif

  // Piepser
  pinMode(MOSI, OUTPUT);
  analogWriteFreq(4000);

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


#ifdef KTYPE

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Reading Temperature KTYPE
double get_thermocouple(void) {

  long dd = 0;
  
  // Communication per I2C Pins but with CS
  digitalWrite(THERMOCOUPLE_CS, LOW);                    // START
  for (uint8_t i=32; i; i--){
    dd = dd <<1;
    if (twi_read_bit())  dd |= 0x01;
  }
  digitalWrite(THERMOCOUPLE_CS, HIGH);                   // END

  // Invalid Measurement
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

  // Temperature in Celsius
  double vv = dd;
  vv *= 0.25;
  return vv;
}

#endif


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize Charge Detection
void set_batdetect(boolean stat) {

  if (stat)  pinMode(CHARGEDETECTION, INPUT_PULLDOWN_16);
  else pinMode(CHARGEDETECTION, INPUT);
}

uint32_t vol_sum = 0;
int vol_count = 0;

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Reading Battery Voltage
void get_Vbat() {
  
  // Digitalwert transformiert in Batteriespannung in mV
  int voltage = analogRead(ANALOGREADBATTPIN);
  //charge = digitalRead(CHARGEDETECTION);

  String stat;
  byte curStateNone = digitalRead(CHARGEDETECTION);
  set_batdetect(HIGH);
  stat += String(curStateNone);
  byte curStatePull = digitalRead(CHARGEDETECTION);
  set_batdetect(LOW);
  stat += String(curStatePull);
  //ch[0].name = stat;
  /*
  if (curStateNone != curStatePull) {
    Serial.println("ungleich");
    ch[0].name = "UN";
    charge = HIGH;
  } else {
    if (curStateNone)
    
    ch[0].name = "Kanal 1";
  }*/
  
  charge = curStateNone;
  
  // Standby erkennen
  if (voltage < 10) {
    stby = true;
    return;
  }
  else stby = false;

  // Transformation Digitalwert in Batteriespannung
  voltage = voltage * BATTDIV; 

  // Batteriespannung wird in einen Buffer geschrieben da die gemessene
  // Spannung leicht schwankt, aufgrund des aktuellen Energieverbrauchs
  // wird die Batteriespannung als Mittel (Median) aus 20 Messungen ausgegeben

  vol_sum += voltage;
  vol_count++;
  
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Calculate SOC
void cal_soc() {

  // mittlere Batteriespannung aus dem Buffer lesen und in Prozent umrechnen
  int voltage;
  
  if (vol_count > 0) voltage = vol_sum / vol_count;
  else voltage = 0;
  median_add(voltage);
  voltage = median_average();
  
  BatteryPercentage = ((voltage - BATTMIN)*100)/(BATTMAX - BATTMIN);
  
  // Schwankungen verschiedener Batterien ausgleichen
  if (BatteryPercentage > 100) BatteryPercentage = 100;
  
  #ifdef DEBUG
    Serial.printf("[INFO]\tBattery voltage: %umV\tcharge: %u%%\r\n", voltage, BatteryPercentage); 
  #endif

  // Abschaltung des Systems bei <0% Akkuleistung
  if (BatteryPercentage < 0) {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display.drawString(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/3, "LOW BATTERY");
    display.drawString(DISPLAY_WIDTH/2, 2*DISPLAY_HEIGHT/3, "PLEASE SWITCH OFF");
    display.display();
    ESP.deepSleep(0);
    delay(100); // notwendig um Prozesse zu beenden
  }

  vol_sum = 0;
  vol_count = 0;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize Hardware Alarm
void set_piepser() {

  // Hardware-Alarm bereit machen
  pinMode(MOSI, OUTPUT);
  analogWriteFreq(4000);
  //analogWriteFreq(500);
  doAlarm = false;
  
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
//Control Hardware Alarm
void controlAlarm(bool action){

  
  bool setalarm = false;

  for (int i=0; i < CHANNELS; i++) {
    if (ch[i].alarm && ch[i].isalarm) {
      setalarm = true;
      if (!isAP) {
        #ifdef THINGSPEAK
          //if (ch[i].temp > ch[i].max) sendMessage(i+1,1);
          //else if (ch[i].temp < ch[i].min) sendMessage(i+1,0);
        #endif
      }
    }
  }

  if (doAlarm && setalarm) {
    analogWrite(MOSI,512);
    displayblocked = true;
    question = HARDWAREALARM;
    drawQuestion();
  }
  else {
    analogWrite(MOSI,0);
  }  
  
}







