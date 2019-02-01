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
    
    HISTORY: Please refer Github History
    
 ****************************************************/


// https://github.com/adafruit/Adafruit_HTU21DF_Library/blob/master/Adafruit_HTU21DF.cpp

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize Sensors
void set_sensor() {

  // Piepser
  pinMode(MOSI, OUTPUT);
  analogWriteFreq(4000);

  
  if (sys.typk && sys.hwversion == 1) {
    // CS notwendig, da nur bei CS HIGH neue Werte im Chip gelesen werden
    pinMode(THERMOCOUPLE_CS, OUTPUT);
    digitalWrite(THERMOCOUPLE_CS, HIGH);
  }

  // MAX11615
  byte reg = 0xA0;    // A0 = 10100000
  // page 14
  // 1: setup mode
  // SEL:010 = Reference (Table 6)
  // external(1)/internal(0) clock
  // unipolar(0)/bipolar(1)
  // 0: reset the configuration register to default
  // 0: dont't care
 
  Wire.beginTransmission(MAX11615_ADDRESS);
  Wire.write(reg);
  byte error = Wire.endTransmission();
  IPRINTP("MAX1161x: ");
  if (error == 0) {
    DPRINTP("0x");
    DPRINTLN(MAX11615_ADDRESS, HEX);
    MAX1161x_ADDRESS = MAX11615_ADDRESS;
/*  } else {

    // MAX11613
    reg = 0xA0;    
    // page 14
    // 80 = 10000000 -> VCC (3V3) als Referenz, nur mit 0R wenn Doppelfühler, Toleranz wird größer
    // D0 = 11010000 -> interne Referenz, passt nicht überein, Werte zu tief
    // A0 = 10100000 -> externe Referenz, dann kein Doppelfühler
    // IC kann nur VCC = 3V3, nicht 2V
    // 1: setup mode
    // SEL[0:2] = Reference (Table 6)
    // external(1)/internal(0) clock
    // unipolar(0)/bipolar(1)
    // 0: reset the configuration register to default
    // 0: dont't care

    Wire.beginTransmission(MAX11613_ADDRESS);
    Wire.write(reg);
    error = Wire.endTransmission();

    if (error == 0) {
      DPRINTP("0x");
      DPRINTLN(MAX11613_ADDRESS, HEX);
      MAX1161x_ADDRESS = MAX11613_ADDRESS;
      if (sys.ch != 3) {
        sys.ch = 3;
        Serial.println("Umstellung auf LITE-3");
        setconfig(eSYSTEM,{});
      }
      sys.pitmaster = false;
 */   } else DPRINTPLN("No");
//  }
  
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Reading ADC-Channel Average
int get_adc_average (byte ch) {  
  // Get the average value for the ADC channel (ch) selected. 
  // MAX11613/15 samples the channel 8 times and returns the average.  
  // Setup byte required: 0xA0 

  byte config;
//Serial.println(ch);
  config = 0x21 + (ch << 1);   //00100001 + ch  // page 15 0x21
//Serial.println(config, BIN);
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


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize Charge Detection
void set_batdetect(boolean stat) {

  if (!stat)  pinMode(CHARGEDETECTION, INPUT_PULLDOWN_16);
  else pinMode(CHARGEDETECTION, INPUT);
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Reading Battery Voltage
void get_Vbat() {
  
  // Digitalwert transformiert in Batteriespannung in mV
  int voltage = analogRead(ANALOGREADBATTPIN);
//Serial.println(voltage);
  // CHARGE DETECTION
  //                LOAD        COMPLETE        SHUTDOWN
  // MCP:           LOW           HIGH           HIGH-Z 
  // curStateNone:  LOW           HIGH           HIGH
  // curStatePull:  LOW           HIGH           LOW
  //                0             3              1
  
  // Messung bei INPUT_PULLDOWN
  set_batdetect(LOW);
  bool curStatePull = digitalRead(CHARGEDETECTION);
  // Messung bei INPUT
  set_batdetect(HIGH);
  bool curStateNone = digitalRead(CHARGEDETECTION);
  // Ladeanzeige
  battery.charge = !curStateNone;
  //Serial.println(voltage);
  // Standby erkennen
  if (voltage < 20) {
    sys.stby = true;
    //return;
  }
  else sys.stby = false;

  // Transformation Digitalwert in Batteriespannung
  voltage = voltage * BATTDIV; 

  battery.state = curStateNone;
  battery.state |= curStatePull<<1;

  if (sys.god & (1<<1)) battery.state = 4;                   // Abschaltung der Erkennung

  switch (battery.state) {

    case 0:                                                    // LOAD
      if (battery.setreference != -1 && battery.setreference < 180) {                        // Referenz setzen
        battery.setreference = -1;
        setconfig(eSYSTEM,{});
      }
      break;

    case 1:                                                    // SHUTDOWN
      if (battery.setreference > 0 && battery.voltage > 0 && !sys.stby) {
        voltage = 4200;
        battery.sim = 4200;

        // Runterzählen
        if ((millis() - battery.correction) > CORRECTIONTIME) { 
          battery.setreference -= CORRECTIONTIME/1000;
          battery.setreference = constrain(battery.setreference, 0, 180);
          setconfig(eSYSTEM,{});                                  // SPEICHERN
          battery.correction = millis();
        }
      }

      if (battery.voltage > 3600) {

        if (battery.sim > 3600) {

          // Reduktion nach der Aufnahme eines neuen Werts
          if (battery.simc > MEDIAN_SIZE-1) { // angepasst an den Median
            battery.simc = 0;     // Zurücksetzen
            battery.sim -= 4; // -1 mV in 2 min  bei 88 mA  // 1500 mAh / 88 mA = 17h // 550 mV / 17h = 32 mV/h
          }

          // Aufnahme eines neuen Werts vor der Reduktion
          if (battery.simc < MEDIAN_SIZE-1 && battery.sim - battery.voltage > 5) voltage = battery.sim; // 9/10

        } else if (battery.simc > 1) battery.sim = battery.voltage - 1;  // Systemstart, etwas warten
        
      } else battery.simc = 0;

      break;

    case 3:                                                    // COMPLETE (vollständig)
      if (battery.setreference == -1 && voltage > 4150) {      // es wurde geladen, setze Referenz wenn Akku wirklich geladen
        battery.setreference = 180;                            // Referenzzeit setzen
        setconfig(eSYSTEM,{});
      } else if (millis() > 10000) voltage = 4200;             // 100% bei USB-Quelle, sofern USB nach Laden verbleibt
      break;

    case 4:                                                     // NO BATTERY
      voltage = 4200;
      battery.charge = false;
      break;
  }

  // Batteriespannung wird in einen Buffer geschrieben da die gemessene
  // Spannung leicht schwankt, aufgrund des aktuellen Energieverbrauchs
  // wird die Batteriespannung als Mittel aus mehreren Messungen ausgegeben
  // Während der Battery Initalisierung wird nicht in den Buffer geschrieben

  if (millis() < BATTERYSTARTUP*2) {
    vol_sum = voltage;                        // Battery Initalisierung
    vol_count = 1;
  } else {
    vol_sum += voltage;
    vol_count++;
  }
  
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Calculate SOC
void cal_soc() {

  // mittlere Batteriespannung aus dem Buffer lesen und in Prozent umrechnen
  int voltage;
  if (battery.state == 1 && millis() > BATTERYSTARTUP) battery.simc++; 
  
  if (vol_count > 0) {

    if (vol_count == 1) {                           // Battery Initialisierung
      if (battery.voltage < vol_sum)
        battery.voltage = vol_sum;                  // beim Start Messung anpassen
    } else {
      voltage = vol_sum / vol_count;
      median_add(voltage);
  
      //battery.voltage = voltage;
      battery.voltage = median_average(); 
    }
    
    battery.percentage = ((battery.voltage - battery.min)*100)/(battery.max - battery.min);
  
    // Schwankungen verschiedener Batterien ausgleichen
    if (battery.percentage > 100) {
      if (battery.charge) battery.percentage = 99;
      else battery.percentage = 100;
    }

    if (millis() > BATTERYSTARTUP) {
      IPRINTF("Battery voltage: %umV,\tcharge: %u%%\r\n", battery.voltage, battery.percentage); 
    }

    // Abschaltung des Systems bei <0% Akkuleistung
    if (battery.percentage < 0) {
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

}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize Hardware Alarm
void set_piepser() {

  // Hardware-Alarm bereit machen
  pinMode(MOSI, OUTPUT);
  analogWriteFreq(4000);
  //sys.hwalarm = false;
  
}

void piepserON() {
  analogWrite(MOSI,512);
  sys.piepoff_t = 2;
}

void piepserOFF() {
  if (sys.piepoff_t == 0) analogWrite(MOSI,0);
  else if (sys.piepoff_t > -2) sys.piepoff_t--;
}

void pbguard() {
  //analogWriteFreq(5);
  analogWrite(MOSI,1023);
  sys.piepoff_t = 2;        // für 2 Zyklen
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
//Control Hardware Alarm
void controlAlarm(){                // action dient zur Pulsung des Signals

  bool setalarm = false;

  for (int i=0; i < sys.ch; i++) {
    //if (ch[i].alarm > 0) {                              // CHANNEL ALARM ENABLED
                
      // CHECK LIMITS
      if ((ch[i].temp <= ch[i].max && ch[i].temp >= ch[i].min) || ch[i].temp == INACTIVEVALUE) {
        // everything is ok
        ch[i].isalarm = false;                      // no alarm
        ch[i].showalarm = 0;                    // no OLED note
        notification.index &= ~(1<<i);              // delete channel from index
        //notification.limit &= ~(1<<i);

      } else if (ch[i].isalarm && ch[i].showalarm > 0) {  // Alarm noch nicht quittiert
        // do summer alarm
        setalarm = true;                            

        // Show Alarm on OLED
        if (ch[i].showalarm == 2 && !displayblocked) {    // falls noch nicht angezeigt
          ch[i].showalarm = 1;                            // nur noch Summer
          question.typ = HARDWAREALARM;
          question.con = i;
          drawQuestion(i);
        }
      
      } else if ((!ch[i].isalarm || ch[i].repeatalarm) && ch[i].temp != INACTIVEVALUE) {
        // first rising limits

        ch[i].isalarm = true;                      // alarm

        if (ch[i].alarm == 1 || ch[i].alarm == 3) {   // push or all
          notification.index &= ~(1<<i);
          notification.index |= 1<<i;                // Add channel to index   
        
          if (ch[i].temp > ch[i].max) {
            notification.limit |= 1<<i;              // add upper limit
          }
          else if (ch[i].temp < ch[i].min) { 
            notification.limit &= ~(1<<i);           // add lower limit              
          }
        } 

        ch[i].repeatalarm = false;
        
        if (ch[i].alarm > 1) {                       // only if summer
          ch[i].showalarm = 2;                    // show OLED note first time
        }
      }
    //} else {                                      // CHANNEL ALARM DISABLED
    //  ch[i].isalarm = false;
    //  ch[i].showalarm = 0;
    //  notification.index &= ~(1<<i);              // delete channel from index
      //notification.limit &= ~(1<<i);
    //}   
  }

  // Hardware-Alarm-Variable: sys.hwalarm
  if (setalarm) piepserON(); 
}

#ifdef AMPERE

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Reading Ampere IC
unsigned long ampere_sum = 0;
unsigned long ampere_con = 0;
float ampere = 0;
unsigned long ampere_time;

void ampere_control() {
    
    ampere_sum += ((get_adc_average(5) * 2.048 )/ 4096.0)*1000.0;
    ampere_con++;

    if (millis()-ampere_time > 10*60*1000) {
      ampere_time = millis();
      ampere = ampere_sum/ampere_con;
      ampere_con = 0;
      ampere_sum = 0;
    } else if (millis() < 120000) {
      ampere = ampere_sum/ampere_con;
    }
}

#endif

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Reading Temperature KTYPE
double get_thermocouple(bool internal) {

  long dd = 0;
  
  // Communication per I2C Pins but with CS
  digitalWrite(THERMOCOUPLE_CS, LOW);                    // START
  for (uint8_t i=32; i; i--){
    dd = dd <<1;
    if (twi_read_bit())  dd |= 0x01;                     // needs #include "core_esp8266_si2c.c"
  }
  digitalWrite(THERMOCOUPLE_CS, HIGH);                   // END

  // Invalid Measurement
  if (dd & 0x7) {             // Abfrage von D0/D1/D2 (Fault)
    return INACTIVEVALUE; 
  }

  if (internal) {
    // Internal Reference
    double ii = (dd >> 4) & 0x7FF;     // Abfrage D4-D14
    ii *= 0.0625;
    if ((dd >> 4) & 0x800) ii *= -1;  // Abfrage D15 (Vorzeichen)
    return ii;
  }
  

  // Temperatur
  if (dd & 0x80000000) {    // Abfrage D31 (Vorzeichen)
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


// bei einem Neustart flag auf false, ebenfalls wenn voll geladen
// dann alle 5 min speichern und beim ersten speichern flag auf true

