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


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Calculate Temperature from ADC-Bytes
float calcT(int r, byte typ){ 

  float Rmess = 47;
  float a, b, c, Rn;

  // kleine Abweichungen an GND verursachen Messfehler von wenigen Digitalwerten
  // daher werden nur Messungen mit einem Digitalwert von mind. 10 ausgewertet,
  // das entspricht 5 mV
  if (r < 10) return INACTIVEVALUE;        // Kanal ist mit GND gebrückt

  switch (typ) {
  case 0:  // Maverik
    Rn = 1000; a = 0.003358; b = 0.0002242; c = 0.00000261;
    break; 
  case 1:  // Fantast-Neu
    Rn = 220; a = 0.00334519; b = 0.000243825; c = 0.00000261726;
    break; 
  case 2:  // Fantast
    Rn = 50.08; a = 3.3558340e-03; b = 2.5698192e-04; c = 1.6391056e-06;
    break; 
  case 3:  // iGrill2
    Rn = 99.61 ; a = 3.3562424e-03; b = 2.5319218e-04; c = 2.7988397e-06;
    break; 
  case 4:  // ET-73
    Rn = 200; a = 0.00335672; b = 0.000291888; c = 0.00000439054; 
    break;
  case 5:  // PERFEKTION
    Rn = 200.1; a =  3.3561990e-03; b = 2.4352911e-04; c = 3.4519389e-06;  
    break; 
  case 6:  // 50K 
    Rn = 50.0; a = 3.35419603e-03; b = 2.41943663e-04; c = 2.77057578e-06;
    //Rn = 5; a = 0.0033555; b = 0.0002570; c = 0.00000243; // NTC 5K3A1B (orange Kopf)
    break; 
  case 7: // INKBIRD
    Rn = 48.59; a = 3.3552456e-03; b = 2.5608666e-04; c = 1.9317204e-06;
    //Rn = 48.6; a = 3.35442124e-03; b = 2.56134397e-04; c = 1.9536396e-06;
    //Rn = 48.94; a = 3.35438959e-03; b = 2.55353377e-04; c = 1.86726509e-06;
    break;
  case 8: // NTC 100K6A1B (lila Kopf)
    Rn = 100; a = 0.00335639; b = 0.000241116; c = 0.00000243362; 
    break;
  case 9: // Weber_6743
    Rn = 102.315; a = 3.3558796e-03; b = 2.7111149e-04; c = 3.1838428e-06; 
    break;
  case 10: // Santos
    Rn = 200.82; a = 3.3561093e-03; b = 2.3552814e-04; c = 2.1375541e-06; 
    break;
  
  #ifdef AMPERE
  case 11:
    
    //Rn = ((r * 2.048 )/ 4096.0)*1000.0;
    //Serial.println(ampere);
    return ampere;
  
  case 12:
    //Rn = ((r * 2.048 )/ 4096.0)*1000.0;
    //Serial.println(r);
    return Rmess*((4096.0/(4096-r)) - 1);
  #endif
  
  // 20K: Rn = 20.0; a = 3.35438355e-03; b = 2.41848755e-04; c = 2.77972882e-06;
  // 50K: Rn = 50.0; a = 3.35419603e-03; b = 2.41943663e-04; c = 2.77057578e-06;
   
  default:  
    return INACTIVEVALUE;
  }
  
  float Rt = Rmess*((4096.0/(4096-r)) - 1);
  float v = log(Rt/Rn);
  float erg = (1/(a + b*v + c*v*v)) - 273.15;
  
  return (erg>-31)?erg:INACTIVEVALUE;
}



// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Reading Temperature ADC
void get_Temperature() {
  // Read NTC Channels
  for (int i=0; i < sys.ch; i++)  {

    float value;
/* 
    // NTC der Reihe nach auslesen
    if (MAX1161x_ADDRESS == MAX11613_ADDRESS && i<4 && i!=0) {
      if (i == ci) {
        value = calcT(get_adc_average(3-i),ch[i].typ);
        //Serial.println(value);
      } else value = ch[i].temp;
      
    }
    else */ if (MAX1161x_ADDRESS == MAX11615_ADDRESS)    value = calcT(get_adc_average(i),ch[i].typ);
    else value = INACTIVEVALUE;
 
    // Temperatursprung außerhalb der Grenzen macht keinen Sinn
    if (ch[i].temp == INACTIVEVALUE && (value < -15.0 || value > 300.0)) value = INACTIVEVALUE;  // wrong typ
 
    // Wenn KTYPE existiert, gibt es nur 4 anschließbare NTC. 
    // KTYPE wandert dann auf Kanal 5
    if (sys.typk && sys.hwversion == 1) {
      if (i == 4) value = get_thermocouple(false);
      if (i == 5) value = get_thermocouple(true);
      //if (i == 5) value = INACTIVEVALUE;
    }

    // Umwandlung C/F
    if ((sys.unit == "F") && value!=INACTIVEVALUE) {  // Vorsicht mit INACTIVEVALUE
      value *= 9.0;
      value /= 5.0;
      value += 32;
    }

    // Temperature Average Buffer by Pitmaster
    if (sys.transform) {
      if (value != INACTIVEVALUE) {
        mem_add(value, i);
      } else {
        mem_clear(i);
      }
      value = mem_a(i);
    }
    
    ch[i].temp = value;
    
    int max = ch[i].max;  // nur für Anzeige
    int min = ch[i].min;  // nur für Anzeige
   
    // Show limits in OLED  
    if ((max > min) && value!=INACTIVEVALUE) {
      int match = map((int)value,min,max,3,18);
      ch[i].match = constrain(match, 0, 20);
    }
    else ch[i].match = 0;
    
  }

  // Open Lid Detection
  open_lid();

}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize Temperature Channels
void set_channels(bool init) {

  // Grundwerte einrichten
  for (int i=0; i<sys.ch; i++) {
        
    ch[i].temp = INACTIVEVALUE;
    ch[i].match = 0;
    ch[i].isalarm = false;
    ch[i].showalarm = 0;

    if (init) {
      ch[i].name = ("Kanal " + String(i+1));
      ch[i].typ = 0;
    
      if (sys.unit == "F") {
        ch[i].min = ULIMITMINF;
        ch[i].max = OLIMITMINF;
      } else {
        ch[i].min = ULIMITMIN;
        ch[i].max = OLIMITMIN;
      }
  
      ch[i].alarm = false; 
      ch[i].color = colors[i];
    }
  }
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Transform Channel Limits
void transform_limits() {
  
  float max;
  float min;
  
  for (int i=0; i < sys.ch; i++)  {
    max = ch[i].max;
    min = ch[i].min;

    if (sys.unit == "F") {               // Transform to °F
      max *= 9.0;
      max /= 5.0;
      max += 32;
      min *= 9.0;
      min /= 5.0;
      min += 32; 
    } else {                              // Transform to °C
      max -= 32;
      max *= 5.0;
      max /= 9.0;
      min -= 32;
      min *= 5.0;
      min /= 9.0;
    }

    ch[i].max = max;
    ch[i].min = min;
  }
}

