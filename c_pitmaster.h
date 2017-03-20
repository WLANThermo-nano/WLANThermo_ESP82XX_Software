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

struct PID {
  String name;
  float Kp;                     // P-Konstante oberhalb pid_switch
  float Ki;                     // I-Konstante oberhalb pid_switch
  float Kd;                     // D-Konstante oberhalb pid_switch
  float Kp_a;                   // P-Konstante unterhalb pid_switch
  float Ki_a;                   // I-Konstante unterhalb pid_switch
  float Kd_a;                   // D-Konstante unterhalb pid_switch
  int Ki_min;                   // Minimalwert I-Anteil
  int Ki_max;                   // Maximalwert I-Anteil
  float pswitch;                // Umschaltungsgrenze
  int pause;                    // Regler Intervall
  bool freq;
  float esum;                   // Startbedingung I-Anteil
  float elast;                  // Startbedingung D-Anteil
};

PID pid[PITMASTERSIZE];

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Set Pitmaster Pin
void set_pitmaster() {
  
  pinMode(PITMASTER1, OUTPUT);
  digitalWrite(PITMASTER1, LOW);

  pitmaster.typ = 0;
  pitmaster.channel = 0;
  pitmaster.set = ch[pitmaster.channel].min;
  pitmaster.active = false;
  pitmaster.value = 0;
  pitmaster.manuel = 0;
  pitmaster.event = false;
  pitmaster.msec = 0;

}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// PID
float PID_Regler(){ 

  float x = ch[pitmaster.channel].temp;         // IST
  float w = pitmaster.set;                      // SOLL
  byte ii = pitmaster.typ;
  
  // PID Parameter
  float kp, ki, kd;
  if (x > (pid[ii].pswitch * w)) {
    kp = pid[ii].Kp;
    ki = pid[ii].Ki;
    kd = pid[ii].Kd;
  } else {
    kp = pid[ii].Kp_a;
    ki = pid[ii].Ki_a;
    kd = pid[ii].Kd_a;
  }

  // Abweichung bestimmen
  //float e = w - x;                          
  int diff = (w -x)*10;
  float e = diff/10.0;              // nur Temperaturunterschiede von >0.1°C beachten
  
  // Proportional-Anteil
  float p_out = kp * e;                     
  
  // Differential-Anteil
  float edif = (e - pid[ii].elast)/(pid[ii].pause/1000.0);   
  pid[ii].elast = e;
  float d_out = kd * edif;                  

  // Integral-Anteil
  // Anteil nur erweitert, falls Bregrenzung nicht bereits erreicht
  if ((p_out + d_out) < PITMAX) {
    pid[ii].esum += e * (pid[ii].pause/1000.0);             
  }
  // ANTI-WIND-UP (sonst Verzögerung)
  // Limits an Ki anpassen: Ki*limit muss y_limit ergeben können
  if (pid[ii].esum * ki > pid[ii].Ki_max) pid[ii].esum = pid[ii].Ki_max/ki;
  else if (pid[ii].esum * ki < pid[ii].Ki_min) pid[ii].esum = pid[ii].Ki_min/ki;
  float i_out = ki * pid[ii].esum;
                    
  // PID-Regler berechnen
  float y = p_out + i_out + d_out;  
  y = constrain(y,PITMIN,PITMAX);           // Auflösung am Ausgang ist begrenzt            
  
  #ifdef DEBUG
  Serial.println("{INFO]\tPID:" + String(y,1) + "\tp:" + String(p_out,1) + "\ti:" + String(i_out,2) + "\td:" + String(d_out,1));
  #endif
  
  return y;
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Control - Manuell PWM
void pitmaster_control() {
  // ESP PWM funktioniert nur bis 10 Hz Trägerfrequenz stabil, daher eigene Taktung
  if (pitmaster.active) {

    // Ende eines HIGH-Intervalls, wird durch pit_event nur einmal pro Intervall durchlaufen
    if ((millis() - pitmaster.last > pitmaster.msec) && pitmaster.event) {
      digitalWrite(PITMASTER1, LOW);
      pitmaster.event = false;
    }

    // neuen Stellwert bestimmen und ggf HIGH-Intervall einleiten
    if (millis() - pitmaster.last > pid[pitmaster.typ].pause) {
  
      float y;

      if (pitmaster.manuel > 0) y = pitmaster.manuel;
      else y = PID_Regler();
      pitmaster.value = y;

      if (pid[pitmaster.typ].freq)
        analogWrite(PITMASTER1,map(y,0,100,0,1024));
      else {
        pitmaster.msec = map(y,0,100,0,pid[pitmaster.typ].pause); 
        if (pitmaster.msec > 0) digitalWrite(PITMASTER1, HIGH);
        if (pitmaster.msec < pid[pitmaster.typ].pause) pitmaster.event = true;  // außer bei 100%
      }
    
      pitmaster.last = millis();
    }
  } else {
    digitalWrite(PITMASTER1, LOW);
    pitmaster.value = 0;
    pitmaster.event = false;
    pitmaster.msec = 0;
  }
  
}




