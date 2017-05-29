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

    The AutotunePID elements of this program based on the work of 
    AUTHOR: Repetier
    PURPOSE: Repetier-Firmware/extruder.cpp

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
   uint32_t t0;
   uint32_t t1;            // ZEITKONSTANTE 1
   uint32_t t2;           // ZEITKONSTANTE 2
   int32_t t_high;        // FLAG HIGH
   int32_t t_low;         // FLAG LOW
   int32_t bias;
   int32_t d;
   float Kp;
   float Ki;
   float Kd;
   float Kp_a;
   float Ki_a;
   float Kd_a;
   float maxTemp;
   float minTemp;
   bool initialized;
   float value;
   float previousTemp;
   float maxTP;             // MAXIMALE STEIGUNG = WENDEPUNKT
   uint32_t tWP;            // ZEITPUNKT WENDEPUNKT  
   float TWP;               // TEMPERATUR WENDEPUNKT
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

  // see: http://rn-wissen.de/wiki/index.php/Regelungstechnik

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
  
  DPRINTLN("{INFO]\tPID:" + String(y,1) + "\tp:" + String(p_out,1) + "\ti:" + String(i_out,2) + "\td:" + String(d_out,1));
  
  return y;
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// AUTOTUNE
void disableAllHeater() {
  // Anschlüsse ausschalten
  digitalWrite(PITMASTER1, LOW);
  pitmaster.active = 0;
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

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// AUTOTUNE
void startautotunePID(int maxCycles, bool storeValues)  {

    if (!autotune.initialized) {
  
      autotune.cycles = 0;           // Durchläufe
      autotune.heating = true;      // Flag
      autotune.temp = pitmaster.set;
      autotune.maxCycles = constrain(maxCycles, 5, 20);
      autotune.storeValues = storeValues;
  
      uint32_t temp_millis = millis();
      autotune.t0 = temp_millis;    // Zeitpunkt t0
      autotune.t1 = temp_millis;    // Zeitpunkt t1
      autotune.t2 = temp_millis;    // Zeitpunkt t2
      autotune.t_high = 0;
      autotune.bias = pidMax/2;         // Startwert = halber Wert
      autotune.d = pidMax/2;          // Startwert = halber Wert

      float tem = ch[pitmaster.channel].temp; 
      autotune.Kp = 0;
      autotune.Ki = 0; 
      autotune.Kd = 0;
      autotune.maxTemp = tem; 
      autotune.minTemp = tem;
      autotune.previousTemp = tem;  // Startwert
      autotune.maxTP = 0.0;
      autotune.tWP = temp_millis;
      autotune.TWP = tem;
  
      DPRINTLN("[AUTOTUNE]\t Start!");
 
      disableAllHeater();         // switch off all heaters.
      
      autotune.value = pidMax;   // Aktor einschalten
    }
    
    autotune.initialized = true;
    pitmaster.active = true;
    
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// AUTOTUNE
float autotunePID() {

  /*
   Relay Feedback Test [Astrom & Hagglund, 1984
   Autotune Variation (ATV)
   see: https://www.google.de/?gws_rd=ssl#q=Relay+Feedback+PID
   
         
             t_high = t1-t2*
     +d       ____      ____      _
     bias--__|    |____|    |____|
     -d     t2*   t1   t2
   /                         t_low = t2-t1  
   
   */

  float currentTemp = ch[pitmaster.channel].temp;
  unsigned long time = millis();

  if (autotune.cycles == 0) { 
    float TP = (currentTemp - autotune.previousTemp)/ (float)pid[pitmaster.typ].pause;
    if (autotune.maxTP < TP) {
      autotune.maxTP = TP;
      autotune.tWP = time;
      autotune.TWP = (currentTemp + autotune.previousTemp)/2.0;
      DPRINT("[AUTOTUNE]\tWendepunktbestimmung: ");
      DPRINTLN(TP*1000);
    }
    autotune.previousTemp = currentTemp;
  }
  
  autotune.maxTemp = max(autotune.maxTemp,currentTemp);
  autotune.minTemp = min(autotune.minTemp,currentTemp);
  
  // Soll während aufheizen ueberschritten --> switch heating off  
  if (autotune.heating == true && currentTemp > autotune.temp)  {
    if(time - autotune.t2 > 3000)  {    // warum Wartezeit und wieviel
                
      autotune.heating = false;
      autotune.value = (autotune.bias - autotune.d);     // Aktorwert
                
      autotune.t1 = time;
      autotune.t_high = autotune.t1 - autotune.t2;
      autotune.maxTemp = autotune.temp;
      DPRINTLN("[AUTOTUNE]\tTemperature overrun!");
    }
  }
  
  // Soll während abkühlen unterschritten --> switch heating on
  else if (autotune.heating == false && currentTemp < autotune.temp) {
    if(time - autotune.t1 > 3000)  {
                
      autotune.heating = true;
      autotune.t2 = time;
      autotune.t_low = autotune.t2 - autotune.t1; // half wave length
      DPRINTLN("[AUTOTUNE]\tTemperature fall below!");

      if (autotune.cycles == 0) {

        // Bestimmung von Kp_a, Ki_a, Kd_a
        // Approximation der Strecke durch PT1Tt-Glied aus Sprungantwort

        uint32_t tWP1 = (autotune.TWP-autotune.minTemp)/autotune.maxTP;  // Zeitabschn. (ms) unter Wendetangente bis Ausgangstemperatur
        uint32_t tWP2 = (autotune.temp-autotune.TWP)/autotune.maxTP;     // Zeitabschn. (ms) oberhalb Wendetangente bis Zieltemperatur
        
        uint32_t Tt = (autotune.tWP - autotune.t0 - tWP1)/1000.0;      // Totzeit in sec
        uint32_t Tg = (tWP1 + tWP2)/1000.0;                            // Ausgleichszeit in sec

        float Ks = (autotune.temp - autotune.minTemp)/(autotune.bias + autotune.d);
        float Tn = 2 * Tt;            // Tn = 3.33 * Tt;
        float Tv = 0.5 * Tt;

        DPRINT("[AUTOTUNE]\tTt: ");
        DPRINT(Tt);
        DPRINT(" s, Tg: ");
        DPRINT(Tg);
        DPRINTLN(" s");

        autotune.Kp_a = 1.2/Ks * Tg/Tt;  // Kp_a = 0.9/Ks * Tg/Tt
        autotune.Ki_a = autotune.Kp_a/Tn;
        autotune.Kd_a = autotune.Kp_a*Tv;

        DPRINT("[AUTOTUNE]\tKp_a: ");
        DPRINT(autotune.Kp_a);
        DPRINT(" Ki_a: ");
        DPRINT(autotune.Ki_a);
        DPRINT(" Kd_a: ");
        DPRINTLN(autotune.Kd_a);
        
        
      } else {

        // Anpassung Bias
        autotune.bias += (autotune.d*(autotune.t_high - autotune.t_low))/(autotune.t_low + autotune.t_high);
        autotune.bias = constrain(autotune.bias, 5 ,pidMax - 5);  // Arbeitsbereich zwischen 5% und 95%
                    
        if (autotune.bias > pidMax/2)  autotune.d = pidMax - autotune.bias;
        else autotune.d = autotune.bias;
        
        DPRINT("[AUTOTUNE]\tbias: ");
        DPRINT(autotune.bias);
        DPRINT(" d: ");
        DPRINTLN(autotune.d);

        if(autotune.cycles > 2)  {

        // Parameter according Ziegler & Nichols method: http://en.wikipedia.org/wiki/Ziegler%E2%80%93Nichols_method
        // Ku = kritische Verstaerkung = 4*d / (pi*A)
        // Tu = kritische Periodendauer = t_low + t_high = (t2 - t1) + (t1 - t2*) = t2 - t2*
        // A = Amplitude der Schwingung = maxTemp-minTemp/2 // Division fehlt in der Quelle

          float A = (autotune.maxTemp - autotune.minTemp)/2.0;
          float Ku = (4.0 * autotune.d) / (3.14159 * A);
          float Tu = ((float)(autotune.t_low + autotune.t_high)/1000.0);
          
          DPRINT("[AUTOTUNE]\tKu: ");
          DPRINT(Ku);
          DPRINT(" Tu: ");
          DPRINT(Tu);
          DPRINT(" A: ");
          DPRINT(A);
          DPRINT(" t_max: ");
          DPRINT(autotune.maxTemp);
          DPRINT(" t_min: ");
          DPRINTLN(autotune.minTemp);

          float Tn = 0.5*Tu;
          float Tv = 0.125*Tu;
          
          autotune.Kp = 0.6*Ku;
          autotune.Ki = autotune.Kp/Tn;      // Ki = 1.2*Ku/Tu = Kp/Tn = Kp/(0.5*Tu) = 2*Kp/Tu
          autotune.Kd = autotune.Kp*Tv;      // Kd = 0.075*Ku*Tu = Kp*Tv = Kp*0.125*Tu
                        
          DPRINT("[AUTOTUNE]\tKp: ");
          DPRINT(autotune.Kp);
          DPRINT(" Ki: ");
          DPRINT(autotune.Ki);
          DPRINT(" Kd: ");
          DPRINTLN(autotune.Kd);
            
        }
      }
      autotune.value = (autotune.bias + autotune.d);
      DPRINTLN("[AUTOTUNE]\tNext cycle");
      autotune.cycles++;
      autotune.minTemp = autotune.temp;
    }
  }
    
  if (currentTemp > (autotune.temp + 40))  {   // FEHLER
    DPRINTLN("[ERROR]\tAutotune failure: Overtemperature");
    disableAllHeater();
    autotune.value = 0;
    autotune.initialized = false;
    return 0;
  }
    
  if (((time - autotune.t1) + (time - autotune.t2)) > (10L*60L*1000L*2L)) {   // 20 Minutes
        
    DPRINTLN("[ERROR]\tAutotune failure: TIMEOUT");
    disableAllHeater();
    autotune.value = 0;
    autotune.initialized = false;
    return 0;
  }
    
  if (autotune.cycles > autotune.maxCycles) {       // FINISH
            
    DPRINTLN("[AUTOTUNE]\tFinished!");
    disableAllHeater();
    autotune.value = 0;
    autotune.initialized = false;
            
    if (autotune.storeValues)  {
      
      pid[pitmaster.typ].Kp = autotune.Kp;
      pid[pitmaster.typ].Ki = autotune.Ki;
      pid[pitmaster.typ].Kd = autotune.Kd;

      
      pid[pitmaster.typ].Kp_a = autotune.Kp_a;
      pid[pitmaster.typ].Ki_a = autotune.Ki_a;
      pid[pitmaster.typ].Kd_a = autotune.Kd_a;
      DPRINTLN("[AUTOTUNE]\tParameters saved!");
    }
    return 0;
  }
  DPRINTLN("{INFO]\tAUTOTUNE: " + String(autotune.value,1) + " %");
  return autotune.value;       
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










