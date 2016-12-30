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
    
 ****************************************************/


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Calculate Temperature from ADC-Bytes
float calcT(int r, byte typ){ 

  float Rmess = 47;
  float a, b, c, Rn;

  switch (typ) {
  case 0:  // Maverik
    Rn = 1000; a = 0.003358; b = 0.0002242; c = 0.00000261;
    break; 
  case 1:  // Fantast-Neu
    Rn = 220; a = 0.00334519; b = 0.000243825; c = 0.00000261726;
    break; 
  case 2:  // NTC 100K6A1B (lila Kopf)
    Rn = 100; a = 0.00335639; b = 0.000241116; c = 0.00000243362; 
    break; 
  case 3:  // NTC 100K (braun/schwarz/gelb/gold)
    Rn = 100; a = 0.003354016; b = 0.0002460380; c = 0.00000340538; 
    break;
  case 4:  // SMD NTC 100K (ATC Semitec 104GT-2)
    Rn = 0.001; a = 0.0006558853; b = 0.0002343567; c = 0.0;  
    // Rn = 0.001 zur Anpassung der Steinhart-Hart eq.
    break; 
  case 5:  // NTC 5K3A1B (orange Kopf)
    Rn = 5; a = 0.0033555; b = 0.0002570; c = 0.00000243;  
    break; 
   
  default:  
    return 0;
  }
  
  float Rt = Rmess*((4096.0/(4096-r)) - 1);
  float v = log(Rt/Rn);
  float erg = (1/(a + b*v + c*v*v)) - 273;
  
  return (erg>-10)?erg:0x00;
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Reading Temperature ADC
void get_Temperature() {

  // Read NTC Channels
  for (int i=0; i < CHANNELS; i++)  {

    // Wenn KTYPE existiert, gibt es nur 4 anschließbare NTC. 
    // KTYPE wandert dann auf Kanal 5
    #if KTYPE
    if (i == 4) {
      temp[i] = get_thermocouple();
      continue;
    }
    #endif

    if (CHANNELS > 3 && i == CHANNELS-1) {
      // Letzter Kanal ist immer Umgebungstemperatur und der ist Kanal 7
      temp[i] = calcT(get_adc_average(6),ttyp[i]);
    }
    // NTC der Reihe nach auslesen
    else  {
      temp[i] = calcT(get_adc_average(i),ttyp[i]);
    }
  }


  // Temperature monitoring  
  for (int i=0; i < CHANNELS; i++){

    // Show limits in OLED
    if (tmax[i] > tmin[i]) {
      match[i] = map(temp[i],tmin[i],tmax[i],3,18);
      match[i] = constrain(match[i], 0, 20);
    }
    else match[i] = 0;

    // Check limit exceeded
    if (temp[i] > tmax[i] || temp[i] < tmin[i]) alarm[i] = true;
    else alarm[i] = false;
  }
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize Temperature Channels
void set_Channels() {

  // Grundwerte einrichten
  for (int i=0; i<CHANNELS; i++) {
    temp[i] = 0.0;
    match[i] = 0;
    alarm[i] = false;
  }

  // Wenn KTYPE muss Kanal 5 auch KTYPE sein
  #if KTYPE
  ttyp[4] = 6;
  #endif

}


