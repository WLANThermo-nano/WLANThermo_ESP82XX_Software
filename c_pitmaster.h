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

int pidMax = 100;      // Maximum (PWM) value, the heater should be set


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


struct AutoTune {
   bool storeValues;
   float temp;             // BETRIEBS-TEMPERATUR
   int  maxCycles;        // WIEDERHOLUNGEN
   int cycles;            // CURRENT WIEDERHOLUNG
   int heating;            // HEATING FLAG
   uint32_t t1;            // ZEITKONSTANTE 1
   uint32_t t2;           // ZEITKONSTANTE 2
   int32_t t_high;        // FLAG HIGH
   int32_t t_low;         // FLAG LOW
   int32_t bias;
   int32_t d;
   float Ku;
   float Tu;
   float Kp;
   float Ki;
   float Kd;
   float maxTemp;
   float minTemp;
   bool initialized;
};

AutoTune autotune;

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



void disableAllHeater() {
  // Anschlüsse ausschalten
  digitalWrite(PITMASTER1, LOW);
  pitmaster.value = 0;
  pitmaster.event = false;
  pitmaster.msec = 0;
  
}

static inline float min(float a,float b)  {
        if(a < b) return a;
        return b;
}

static inline float max(float a,float b)  {
        if(a < b) return b;
        return a;
}


void startautotunePID(float temp, int maxCycles, bool storeValues)  {

    if (autotune.initialized) {
  
      autotune.cycles = 0;           // Durchläufe
      autotune.heating = true;      // Flag
      autotune.temp = temp;
      autotune.maxCycles = constrain(maxCycles, 5, 20);
      autotune.storeValues = storeValues;
  
      uint32_t temp_millis = millis();
      autotune.t1 = temp_millis;    // Zeitpunkt t1
      autotune.t2 = temp_millis;    // Zeitpunkt t2
      autotune.t_high = 0;
      autotune.bias = pidMax/2;         // Startwert = halber Wert
      autotune.d = pidMax/2;          // Startwert = halber Wert
    
      autotune.Kp = 0;
      autotune.Ki = 0; 
      autotune.Kd = 0;
      autotune.maxTemp = 20; 
      autotune.minTemp = 20;
  
      Serial.println("[INFO]\t Autotune started!");
 
      disableAllHeater();         // switch off all heaters.
      
      pitmaster.manuel = pidMax;   // Aktor einschalten
    }
    
    autotune.initialized = true;
    pitmaster.active = true;
    
}

float autotunePID() {

  float currentTemp = ch[pitmaster.channel].temp;
  unsigned long time = millis();
  autotune.maxTemp = max(autotune.maxTemp,currentTemp);
  autotune.minTemp = min(autotune.minTemp,currentTemp);
  float y;
  
  // Soll während aufheizen ueberschritten --> switch heating off  
  if (autotune.heating == true && currentTemp > autotune.temp)  {
    if(time - autotune.t2 > 2500)  {    // warum Wartezeit und wieviel
                
      autotune.heating = false;
      y = (autotune.bias - autotune.d);     // Aktorwert
                
      autotune.t1 = time;
      autotune.t_high = autotune.t1 - autotune.t2;
      autotune.maxTemp = autotune.temp;
    }
  }

  // Soll während abkühlen unterschritten --> switch heating on
  if (autotune.heating == false && currentTemp < autotune.temp) {
    if(time - autotune.t1 > 5000)  {
                
      autotune.heating = true;
      autotune.t2 = time;
      autotune.t_low = autotune.t2 - autotune.t1; // half wave length
                
      if(autotune.cycles > 0)  {
          
        autotune.bias += (autotune.d*(autotune.t_high - autotune.t_low))/(autotune.t_low + autotune.t_high);
        autotune.bias = constrain(autotune.bias, 20 ,pidMax - 20);
                    
        if (autotune.bias > pidMax/2)  autotune.d = pidMax - 1 - autotune.bias;
        else autotune.d = autotune.bias;

        if(autotune.cycles > 2)  {
            
        // Parameter according Ziegler¡§CNichols method: http://en.wikipedia.org/wiki/Ziegler%E2%80%93Nichols_method
                        
          autotune.Ku = (4.0 * autotune.d) / (3.14159*(autotune.maxTemp-autotune.minTemp));
          autotune.Tu = ((float)(autotune.t_low + autotune.t_high)/1000.0);
          Serial.print("Ku: "); Serial.println(autotune.Ku);
          Serial.print("Tu: "); Serial.println(autotune.Tu);
                        
          autotune.Kp = 0.6*autotune.Ku;
          autotune.Ki = 2*autotune.Kp/autotune.Tu;
          autotune.Kd = autotune.Kp*autotune.Tu*0.125;
                        
          Serial.print("Kp: "); Serial.println(autotune.Kp);
          Serial.print("Ki: "); Serial.println(autotune.Ki);
          Serial.print("Kd: "); Serial.println(autotune.Kd);
            
        }
      }
      y = (autotune.bias + autotune.d);
      autotune.cycles++;
      autotune.minTemp = autotune.temp;
    }
  }
    
  if (currentTemp > (autotune.temp + 40))  {   // FEHLER
    Serial.println("[FEHLER]\t Autotune failure: Overtemperature");
    disableAllHeater();
    autotune.initialized = false;
    return 0;
  }
    
  if (((time - autotune.t1) + (time - autotune.t2)) > (10L*60L*1000L*2L)) {   // 20 Minutes
        
    Serial.println("[INFO]\t Autotune failure: TIMEOUT");
    disableAllHeater();
    autotune.initialized = false;
    return 0;
  }
    
  if (autotune.cycles > autotune.maxCycles) {       // FINISH
            
    Serial.println("[INFO]\t Autotune finished!");
    disableAllHeater();
    autotune.initialized = false;
            
    if (autotune.storeValues)  {
      Serial.println(autotune.Kp);
      Serial.println(autotune.Ki);
      Serial.println(autotune.Kd);

    }
    return 0;
  }

  return y;       
}



// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Control - Manuell PWM
void pitmaster_control() {
  // ESP PWM funktioniert nur bis 10 Hz Trägerfrequenz stabil, daher eigene Taktung
  if (pitmaster.active == 1) {

    // Ende eines HIGH-Intervalls, wird durch pit_event nur einmal pro Intervall durchlaufen
    if ((millis() - pitmaster.last > pitmaster.msec) && pitmaster.event) {
      digitalWrite(PITMASTER1, LOW);
      pitmaster.event = false;
    }

    // neuen Stellwert bestimmen und ggf HIGH-Intervall einleiten
    if (millis() - pitmaster.last > pid[pitmaster.typ].pause) {
  
      float y;

      if (autotune.initialized)       y = autotunePID();
      else if (pitmaster.manuel > 0)  y = pitmaster.manuel;
      else                            y = PID_Regler();
      
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










