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
    0.1.00 - 2017-01-06 initial version
    
 ****************************************************/

#define PITMASTER1 15

                
int pit_max = 100;              // Obergrenze Stellwert
int pit_min = 0;                // Untergrenze Stellwert
unsigned long lastPitmaster;
bool pit_event = false;


struct PID {
  float Kp;                     // P-Konstante oberhalb pid_switch
  float Ki;                     // I-Konstante oberhalb pid_switch
  float Kd;                     // D-Konstante oberhalb pid_switch
  float Kp_a;                   // P-Konstante unterhalb pid_switch
  float Ki_a;                   // I-Konstante unterhalb pid_switch
  float Kd_a;                   // D-Konstante unterhalb pid_switch
  int Ki_min;                   // Minimalwert I-Anteil
  int Ki_max;                   // Maximalwert I-Anteil
  float esum;                   // Startbedingung I-Anteil
  float elast;                  // Startbedingung D-Anteil
  float pid_switch;             // Umschaltungsgrenze
};

PID p1 = {3.8, 0.01, 128, 6.2, 0.001, 5, 0, 95, 0, 0, 0.9};

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Set Pitmaster Pin
void set_pitmaster() {
  
  pinMode(PITMASTER1, OUTPUT);
  digitalWrite(PITMASTER1, LOW);

  //analogWriteRange(255);
  //analogWriteFreq(10);   // <10 funktioniert nicht stabil
}




// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// PID
float PID_Regler (float x, float w){   // x:IST,  w:SOLL

  // PID Parameter
  float kp, ki, kd;
  if (x > (p1.pid_switch * w)) {
    kp = p1.Kp;
    ki = p1.Ki;
    kd = p1.Kd;
  } else {
    kp = p1.Kp_a;
    ki = p1.Ki_a;
    kd = p1.Kd_a;
  }

  // Abweichung bestimmen
  //float e = w - x;                          
  int diff = (w -x)*10;
  float e = diff/10.0;              // nur Temperaturunterschiede von >0.1°C beachten
  
  // Proportional-Anteil
  float p_out = kp * e;                     
  
  // Differential-Anteil
  float edif = (e - p1.elast)/(pit_pause/1000.0);   
  p1.elast = e;
  float d_out = kd * edif;                  

  // Integral-Anteil
  // Anteil nur erweitert, falls Bregrenzung nicht bereits erreicht
  if ((p_out + d_out) < pit_max) {
    p1.esum += e * (pit_pause/1000.0);             
  }
  // ANTI-WIND-UP (sonst Verzögerung)
  // Limits an Ki anpassen: Ki*limit muss y_limit ergeben können
  if (p1.esum * ki > p1.Ki_max) p1.esum = p1.Ki_max/ki;
  else if (p1.esum * ki < p1.Ki_min) p1.esum = p1.Ki_min/ki;
  float i_out = ki * p1.esum;
                    
  // PID-Regler berechnen
  float y = p_out + i_out + d_out;  
  y = constrain(y,pit_min,pit_max);           // Auflösung am Ausgang ist begrenzt            
  
  #ifdef DEBUG
  Serial.println("{INFO]\tPID:" + String(y,1) + "\tp:" + String(p_out,1) + "\ti:" + String(i_out,2) + "\td:" + String(d_out,1));
  #endif
  
  return y;
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Control - Manuell PWM
void pitmaster_control() {
  // ESP PWM funktioniert nur bis 10 Hz Trägerfrequenz stabil, daher eigene Taktung

  // Ende eines HIGH-Intervalls, wird durch pit_event nur einmal pro Intervall durchlaufen
  if ((millis() - lastPitmaster > pit_y) && pit_event) {
    digitalWrite(PITMASTER1, LOW);
    pit_event = false;
  }

  // neuen Stellwert bestimmen und ggf HIGH-Intervall einleiten
  if (millis() - lastPitmaster > pit_pause) {
  
    float y;

    if (pit_manuell > 0) y = pit_manuell;
    else y = PID_Regler(ch[0].temp, 30.0);

    //Serial.println(y);
    pit_y = map(y,0,100,0,pit_pause); 
    //analogWrite(PITMASTER1,y);
 
    //Serial.println(pit_y);  

    if (pit_y > 0) digitalWrite(PITMASTER1, HIGH);
    if (pit_y < pit_pause) pit_event = true;
    
    lastPitmaster = millis();
  }
  
}




