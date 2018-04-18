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

//#define PM_DEBUG              // ENABLE SERIAL AUTOTUNE DEBUG MESSAGES

#ifdef PM_DEBUG
  #define PMPRINT(...)    Serial.print(__VA_ARGS__)
  #define PMPRINTLN(...)  Serial.println(__VA_ARGS__)
  #define PMPRINTP(...)   Serial.print(F(__VA_ARGS__))
  #define PMPRINTPLN(...) Serial.println(F(__VA_ARGS__))
  #define PMPRINTF(...)   Serial.printf(__VA_ARGS__)
  
#else
  #define PMPRINT(...)     //blank line
  #define PMPRINTLN(...)   //blank line 
  #define PMPRINTP(...)    //blank line
  #define PMPRINTPLN(...)  //blank line
  #define PMPRINTF(...)    //blank line
#endif


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Clear Pitmaster Settings
void init_pitmaster(bool init, byte id) {

  if (init) {
    pitMaster[id].pid = 0;
    pitMaster[id].channel = 0;
    pitMaster[id].set = PITMASTERSETMIN;
    pitMaster[id].active = PITOFF;
    //pitMaster[id].resume = 0;
  }

  pitMaster[id].resume = 1;   // später wieder raus
  if (!pitMaster[id].resume) pitMaster[id].active = PITOFF; 

  if (pitMaster[id].active != MANUAL) pitMaster[id].value = 0;
  pitMaster[id].event = false;
  pitMaster[id].msec = 0;
  pitMaster[id].pause = 1000;
  pitMaster[id].esum = 0;
  pitMaster[id].elast = 0;
  
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Set Pitmaster Pin
void set_pitmaster(bool init) {
  
  pitMaster[0].io = PITMASTER1;
  pinMode(PITMASTER1, OUTPUT);
  digitalWrite(PITMASTER1, LOW);
  init_pitmaster(init, 0);
  
  pitMaster[1].io = PITMASTER2;
  pinMode(PITMASTER2, OUTPUT);
  digitalWrite(PITMASTER2, LOW);
  init_pitmaster(init, 1);

  if (sys.hwversion > 1) {
    pinMode(PITSUPPLY, OUTPUT);
    digitalWrite(PITSUPPLY, LOW);
  }

}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Set Default PID-Settings
void set_pid(byte index) {

  // Name, Nr, Aktor, Kp, Ki, Kd, Kp_a, Ki_a, Kp_a, Ki_min, Ki_max, Switch, DCmin, DCmax, ...
  
  pidsize = 3; //3;
  pid[0] = {"SSR SousVide", 0, 0, 104, 0.2, 0, 20, 0,  0, 0, 95,  0.9, 0,  100};
  pid[1] = {"TITAN 50x50",  1, 1, 3.8, 0.01,   128, 6.2, 0.001, 5, 0, 95,  0.9, 25, 100};
  pid[2] = {"Kamado 50x50", 2, 1, 7.0, 0.019,  630, 6.2, 0.001, 5, 0, 95,  0.9, 25, 100};

  if (index)
    pid[2] = {"Servo", 2, 2, 12.0, 0.09, 0, 15, 0, 0, 0, 95, 0.9, 20, 80};

}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// PID
float PID_Regler(byte id){ 

  // see: http://rn-wissen.de/wiki/index.php/Regelungstechnik

  float x = ch[pitMaster[id].channel].temp;         // IST
  float w = pitMaster[id].set;                      // SOLL
  byte ii = pitMaster[id].pid;
  
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

  int diff;
  float e;

  // Abweichung bestimmen
  switch (pid[pitMaster[id].pid].aktor) {
    case SSR:           // SSR
      diff = (w -x)*100;
      e = diff/100.0;
    break;

    default:
      diff = (w -x)*10;
      e = diff/10.0;              // nur Temperaturunterschiede von >0.1°C beachten
  }
    
  //float e = w - x;                          
  
  // Proportional-Anteil
  float p_out = kp * e;                     
  
  // Differential-Anteil
  float edif = (e - pitMaster[id].elast)/(pitMaster[id].pause/1000.0);   
  pitMaster[id].elast = e;
  float d_out = kd * edif;                  

  // Integral-Anteil
  // Anteil nur erweitert, falls Begrenzung nicht bereits erreicht
  if ((p_out + d_out) < PITMAX) {
    pitMaster[id].esum += e * (pitMaster[id].pause/1000.0);             
  }
  // ANTI-WIND-UP (sonst Verzögerung)
  // Limits an Ki anpassen: Ki*limit muss y_limit ergeben können
  if (pitMaster[id].esum * ki > pid[ii].Ki_max) pitMaster[id].esum = pid[ii].Ki_max/ki;
  else if (pitMaster[id].esum * ki < pid[ii].Ki_min) pitMaster[id].esum = pid[ii].Ki_min/ki;
  float i_out = ki * pitMaster[id].esum;
                    
  // PID-Regler berechnen
  float y = p_out + i_out + d_out;  
  y = constrain(y,PITMIN,PITMAX);           // Auflösung am Ausgang ist begrenzt            
  
  PMPRINTLN("[PM]\tPID:" + String(y,1) + "\tp:" + String(p_out,1) + "\ti:" + String(i_out,2) + "\td:" + String(d_out,1));
  
  return y;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Clear PID
void clear_PID_Regler(byte id) {
  // PID zurücksetzen
  pitMaster[id].esum = 0;
  pitMaster[id].elast = 0;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Control - Pitmaster 12V Supply
void pitsupply(bool out, byte id) {

  if (pitMaster[id].io == PITMASTER1 && sys.hwversion > 1) {
    //if () out = HIGH;  // SSR || FAN
    //Serial.println(out);
    digitalWrite(PITSUPPLY, out);
  }
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// TURN PITMASTER OFF
void disableHeater(byte id) {
  // Anschlüsse ausschalten
  if (pid[pitMaster[id].pid].aktor == FAN || pid[pitMaster[id].pid].aktor == DAMPER) { // FAN
    analogWrite(pitMaster[id].io, LOW);
  } else {
    digitalWrite(pitMaster[id].io, LOW); // SSR
  }
    
  //digitalWrite(PITMASTER2, LOW);
  pitsupply(0, id); // 12V Supply abschalten, falls nötig
  
  pitMaster[id].active = PITOFF;
  pitMaster[id].value = 0;
  pitMaster[id].event = false;
  pitMaster[id].msec = 0;

  clear_PID_Regler(id);
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void disableAllHeater() {
  disableHeater(0);
  disableHeater(1);
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
static inline float min(float a,float b)  {
        if(a < b) return a;
        return b;
}

static inline float max(float a,float b)  {
        if(a < b) return b;
        return a;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// STOP AUTOTUNE
void stopautotune(byte id) {
  autotune.value = 0;
  autotune.initialized = false;

  if ((autotune.stop == 1) && autotune.storeValues) { // sauber beendet
      setconfig(ePIT,{});
      if (autotune.keepup) pitMaster[id].active = AUTO;     // Pitmaster in AUTO fortsetzen
      else pitMaster[id].active = PITOFF;
  } else pitMaster[id].active = PITOFF;
  
  question.typ = TUNE;
  drawQuestion(autotune.stop);
  autotune.stop = 0;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// START AUTOTUNE
void startautotunePID(int maxCyc, bool store, int over, long tlimit, byte id)  {
  
  autotune.cycles = 0;           // Durchläufe
  autotune.heating = true;      // Flag
  autotune.temp = pitMaster[id].set;
  autotune.maxCycles = constrain(maxCyc, 5, 20);
  autotune.storeValues = store;
  autotune.keepup = false;
  
  uint32_t tmi = millis();
  float tem = ch[pitMaster[id].channel].temp;

  // t0, t1, t2, t_high, bias, d, Kp, Ki, Kd, maxTemp, minTemp, pTemp, maxTP, tWP, TWP
  
  autotune.t0 = tmi;    // Zeitpunkt t0
  autotune.t1 = tmi;    // Zeitpunkt t1
  autotune.t2 = tmi;    // Zeitpunkt t2
  autotune.t_high = 0;
  autotune.bias = pidMax/2;         // Startwert = halber Wert
  autotune.d = pidMax/2;          // Startwert = halber Wert

  autotune.Kp = 0;
  autotune.Ki = 0; 
  autotune.Kd = 0;
  autotune.maxTemp = tem; 
  autotune.minTemp = tem;
  autotune.pTemp = tem;  // Startwert
  autotune.maxTP = 0.0;
  autotune.tWP = tmi;
  autotune.TWP = tem;

  autotune.overtemp = over; //40
  autotune.timelimit = tlimit; //(10L*60L*1000L*4L)
  
  PMPRINTPLN("[AT]\t Start!");
  
  disableHeater(id);         // switch off heaters.
  autotune.value = pidMax;   // Aktor einschalten
  autotune.initialized = true;  
  pitMaster[id].active = AUTOTUNE;
  autotune.start = tem;

}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// AUTOTUNE
float autotunePID(byte id) {

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

   // http://www.aaabbb.de/ControlTheory/ControlLoopTuningStepResponse.php

  // Show start on OLED
  if (autotune.start) {
    
    question.typ = TUNE;
    drawQuestion(0);
    autotune.start = false;
  }

  float currentTemp = ch[pitMaster[id].channel].temp;
  unsigned long time = millis();

  if (autotune.cycles == 0) { 
    float TP = (currentTemp - autotune.pTemp)/ (float)pitMaster[id].pause;
    if (autotune.maxTP < TP) {
      autotune.maxTP = TP;
      autotune.tWP = time;
      autotune.TWP = (currentTemp + autotune.pTemp)/2.0;
      PMPRINTP("[AT]\tWendepunktbestimmung: ");
      PMPRINTLN(TP*1000);
    }
    autotune.pTemp = currentTemp;
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
      PMPRINTPLN("[AT]\toverrun!");
    }
  }
  
  // Soll während abkühlen unterschritten --> switch heating on
  else if (autotune.heating == false && currentTemp < autotune.temp) {
    if(time - autotune.t1 > 3000)  {
                
      autotune.heating = true;
      autotune.t2 = time;
      autotune.t_low = autotune.t2 - autotune.t1; // half wave length
      PMPRINTPLN("[AT]\tfall below");

      if (autotune.cycles == 0) {

        uint32_t tWP1 = (autotune.TWP-autotune.minTemp)/autotune.maxTP;  // Zeitabschn. (ms) unter Wendetangente bis Ausgangstemperatur
        uint32_t tWP2 = (autotune.temp-autotune.TWP)/autotune.maxTP;     // Zeitabschn. (ms) oberhalb Wendetangente bis Zieltemperatur
        
        uint32_t Tt = (autotune.tWP - autotune.t0 - tWP1)/1000.0;      // Totzeit in sec
        uint32_t Tg = (tWP1 + tWP2)/1000.0;                            // Ausgleichszeit in sec

        float Ks = (autotune.temp - autotune.minTemp)/(autotune.bias + autotune.d);
        float Tn = 2 * Tt;            // Tn = 3.33 * Tt;
        float Tv = 0.5 * Tt;

        PMPRINTP("[AT]\tTt: ");
        PMPRINT(Tt);
        PMPRINTP(" s, Tg: ");
        PMPRINT(Tg);
        PMPRINTPLN(" s");

        autotune.Kp_a = (0.1*autotune.temp);
        autotune.Ki_a = 0;
        autotune.Kd_a = 0;

        PMPRINTP("[AT]\tKp_a: ");
        PMPRINT(autotune.Kp_a);
        PMPRINTP(" Ki_a: ");
        PMPRINT(autotune.Ki_a);
        PMPRINTP(" Kd_a: ");
        PMPRINTLN(autotune.Kd_a);
        
        
      } else {

        // Anpassung Bias
        autotune.bias += (autotune.d*(autotune.t_high - autotune.t_low))/(autotune.t_low + autotune.t_high);
        autotune.bias = constrain(autotune.bias, 5 ,pidMax - 5);  // Arbeitsbereich zwischen 5% und 95%
                    
        if (autotune.bias > pidMax/2)  autotune.d = pidMax - autotune.bias;
        else autotune.d = autotune.bias;
        
        PMPRINTP("[AT]\tbias: ");
        PMPRINT(autotune.bias);
        PMPRINTP(" d: ");
        PMPRINTLN(autotune.d);

        if(autotune.cycles > 2)  {

        // Parameter according Ziegler & Nichols method: http://en.wikipedia.org/wiki/Ziegler%E2%80%93Nichols_method
        // Ku = kritische Verstaerkung = 4*d / (pi*A)
        // Tu = kritische Periodendauer = t_low + t_high = (t2 - t1) + (t1 - t2*) = t2 - t2*
        // A = Amplitude der Schwingung = maxTemp-minTemp/2 // Division fehlt in der Quelle

          float A = (autotune.maxTemp - autotune.minTemp)/2.0;
          float Ku = (4.0 * autotune.d) / (3.14159 * A);
          float Tu = ((float)(autotune.t_low + autotune.t_high)/1000.0);
          
          PMPRINTP("[AT]\tKu: ");
          PMPRINT(Ku);
          PMPRINTP(" Tu: ");
          PMPRINT(Tu);
          PMPRINTP(" A: ");
          PMPRINT(A);
          PMPRINTP(" t_max: ");
          PMPRINT(autotune.maxTemp);
          PMPRINTP(" t_min: ");
          PMPRINTLN(autotune.minTemp);

          float Tn = 0.5*Tu;
          float Tv = 0.125*Tu;
          
          autotune.Kp = 0.6*Ku;
          autotune.Ki = autotune.Kp/Tn;      // Ki = 1.2*Ku/Tu = Kp/Tn = Kp/(0.5*Tu) = 2*Kp/Tu
          autotune.Kd = autotune.Kp*Tv;      // Kd = 0.075*Ku*Tu = Kp*Tv = Kp*0.125*Tu
                        
          PMPRINTP("[AT]\tKp: ");
          PMPRINT(autotune.Kp);
          PMPRINTP(" Ki: ");
          PMPRINT(autotune.Ki);
          PMPRINTP(" Kd: ");
          PMPRINTLN(autotune.Kd);
            
        }
      }
      autotune.value = (autotune.bias + autotune.d);
      PMPRINTPLN("[AT]\tNext cycle");
      autotune.cycles++;
      autotune.minTemp = autotune.temp;
    }
  }
    
  if (currentTemp > (autotune.temp + autotune.overtemp))  {   // FEHLER
    IPRINTPLN("f:AT OVERTEMP");
    disableHeater(id);
    autotune.stop = 2;
    return 0;
  }
    
  if (((time - autotune.t1) + (time - autotune.t2)) > autotune.timelimit) {   // 20 Minutes
        
    IPRINTPLN("f:AT TIMEOUT");
    disableHeater(id);
    autotune.stop = 2;
    return 0;
  }
    
  if (autotune.cycles > autotune.maxCycles) {       // FINISH
            
    PMPRINTPLN("[AT]\tFinished!");
    disableHeater(id);
    autotune.stop = 1;
            
    if (autotune.storeValues)  {
      
      pid[pitMaster[id].pid].Kp = autotune.Kp;
      pid[pitMaster[id].pid].Ki = autotune.Ki;
      pid[pitMaster[id].pid].Kd = autotune.Kd;
      pid[pitMaster[id].pid].Kp_a = autotune.Kp_a;
      pid[pitMaster[id].pid].Ki_a = autotune.Ki_a;
      pid[pitMaster[id].pid].Kd_a = autotune.Kd_a;
      DPRINTLN("[AT]\tParameters saved!");
    }
    return 0;
  }
  PMPRINTLN("AT: " + String(autotune.value,1) + " %");
  return autotune.value;       
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Start Duty Cycle Test
void DC_start(bool dc, byte aktor, int val, byte id) {

  if (pitMaster[id].active != DUTYCYCLE) {
    dutyCycle[id].dc = dc;
    dutyCycle[id].aktor = aktor;
    dutyCycle[id].value = val;
    dutyCycle[id].timer = millis();

    if (aktor == SERVO && sys.hwversion > 1) pitMaster[0].io = PITMASTER2;  // Servo-Spezial

    switch (pitMaster[id].active) {
      case PITOFF: dutyCycle[id].saved = PITOFF; break;
      case MANUAL: dutyCycle[id].saved = pitMaster[id].value; break;
      case AUTOTUNE: 
        dutyCycle[id].saved = PITOFF; // autotune abbrechen
        autotune.stop = 2;
        break;
      case AUTO: dutyCycle[id].saved = -1; break;
    }
    pitMaster[id].active = DUTYCYCLE;
  }
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Stop Duty Cycle Test
void DC_stop(byte id) {
  
  if (pitMaster[id].active == DUTYCYCLE && (millis() - dutyCycle[id].timer > 10000)) {
    if (dutyCycle[id].saved == 0) {           // off
      pitMaster[id].active = PITOFF;
    } else if (dutyCycle[id].saved > 0) {
      pitMaster[id].value = dutyCycle[id].saved;  // manual
      pitMaster[id].active = MANUAL;
    } else pitMaster[id].active = AUTO;       // auto
    PMPRINTLN("[DC]\tTest finished");
  }
}

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
 return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Control - Pitmaster Pause
void check_pit_pause(byte id) {

  int aktor;
  uint16_t dcmin, dcmax;
  
  if (pitMaster[id].active == DUTYCYCLE) {
    aktor = dutyCycle[id].aktor;
    dcmin = 0;
    dcmax = 1000;                               // 1. Nachkommastelle
  }
  else {
    aktor = pid[pitMaster[id].pid].aktor;
    dcmin = pid[pitMaster[id].pid].DCmin*10;    // 1. Nachkommastelle
    dcmax = pid[pitMaster[id].pid].DCmax*10;    // 1. Nachkommastelle
  }

  int pause;
  switch (aktor) {

    case SSR:  
      pause = 2000;   // 1/2 Hz, Netzsynchron    // myPitmaster-Anpassung
      pitMaster[id].dcmin = map(dcmin,0,1000,0,pitMaster[id].pause);
      pitMaster[id].dcmax = map(dcmax,0,1000,0,pitMaster[id].pause);
      break;
    case DAMPER:   
    case FAN:  
      pause = 1000;   // 1 Hz 
      pitMaster[id].dcmin = map(dcmin,0,1000,0,1024);
      pitMaster[id].dcmax = map(dcmax,0,1000,0,1024);
      break;   
    case SERVO:  
      pause = 20;   // 50 Hz
      pitMaster[id].dcmin = map(dcmin,0,1000,SERVOPULSMIN,SERVOPULSMAX);
      pitMaster[id].dcmax = map(dcmax,0,1000,SERVOPULSMIN,SERVOPULSMAX);
      break;   
  }

  pitMaster[id].pause = pause;
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// myPitmaster
int myPitmaster(Pitmaster pitmaster) {

  // veränderte Taktzeit durch Anpassung der PitmasterPause
  // Puffertemp < Ofentemp und Ofentemp > Grenze
  if (ch[pitmaster.channel].temp < ch[3].temp && ch[3].temp > pitmaster.set)        // ch[3] = OFEN
    return 100;
  else
    return 0;
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Control - Manuell PWM
void pitmaster_control(byte id) {

  // Stop Autotune
  if (autotune.stop > 0) stopautotune(id);

  // Stop Duty Cylce Test
  DC_stop(id);
  
  // ESP PWM funktioniert nur bis 10 Hz Trägerfrequenz stabil, daher eigene Taktung
  if (pitMaster[id].active > 0) {

    // Check Pitmaster Pause
    check_pit_pause(id);
  
    // SSR: Ende eines HIGH-Intervalls, wird durch pit_event nur einmal pro Intervall durchlaufen
    if ((millis() - pitMaster[id].last > pitMaster[id].msec) && pitMaster[id].event) {
      digitalWrite(pitMaster[id].io, LOW);
      pitMaster[id].event = false;
    }

    // neuen Stellwert bestimmen und ggf HIGH-Intervall einleiten
    if (millis() - pitMaster[id].last > pitMaster[id].pause) {
      
      pitMaster[id].last = millis();
      byte aktor;

      // PITMASTER TYP
      switch (pitMaster[id].active) {

        case DUTYCYCLE:
          aktor = dutyCycle[id].aktor;
          // Startanlauf: bei Servo beide male zuerst in die Mitte, bei Fan nur unten
          if (millis() - dutyCycle[id].timer < 1000) {
            if ((aktor == FAN && !dutyCycle[id].dc) || aktor == SERVO) pitMaster[id].value = 50;
          } else pitMaster[id].value = dutyCycle[id].value/10.0;
          pitMaster[id].timer0 = 0;     // Überbrückung Anlauf-Prozess
          break;

        case AUTOTUNE:
          pitMaster[id].value = autotunePID(id);
          aktor = pid[pitMaster[id].pid].aktor;
          break;

        case AUTO:
          pitMaster[id].value = PID_Regler(id);      //myPitmaster();
          aktor = pid[pitMaster[id].pid].aktor;
          break;

        case MANUAL:    // falls manual wird value vorgegeben
          aktor = pid[pitMaster[id].pid].aktor;
          break;
      }

      // PITMASTER AKTOR
      switch (aktor) {

        case SSR:  
          pitsupply(1, id);   // immer 12V Supply
          pitMaster[id].msec = map(pitMaster[id].value,0,100,pitMaster[id].dcmin,pitMaster[id].dcmax); 
          if (pitMaster[id].msec > 0) digitalWrite(pitMaster[id].io, HIGH);
          if (pitMaster[id].msec < pitMaster[id].pause) pitMaster[id].event = true;  // außer bei 100%
          break;

        case DAMPER:
          // PITMASTER 1 = FAN
          // PITMASTER 2 = SERVO 
        case FAN:  
          pitsupply(sys.pitsupply, id);   // 12V Supply nur falls aktiviert
          if (pitMaster[id].value == 0) {   
            analogWrite(pitMaster[id].io,0);  // bei 0 soll der Lüfter auch stehen
            pitMaster[id].timer0 = millis();  
          } else {
            // Anlaufhilfe: ein Zyklus auf 30% wenn von 0% kommend
            if (millis() - pitMaster[id].timer0 < pitMaster[id].pause*2 && pitMaster[id].value < 30) { 
              analogWrite(pitMaster[id].io,map(30,0,100,pitMaster[id].dcmin,pitMaster[id].dcmax));   // BOOST
            } else {
              analogWrite(pitMaster[id].io,map(pitMaster[id].value,0,100,pitMaster[id].dcmin,pitMaster[id].dcmax));
            }
          }
          break;
        
        case SERVO:   // Achtung bei V2 mit den 12V bei Anschluss an Stromversorgung
          pitsupply(0, id);  // keine 12V Supply
          pitMaster[id].msec = mapfloat(pitMaster[id].value,0,100,pitMaster[id].dcmin,pitMaster[id].dcmax);
          //Serial.println(pitMaster[id].msec);
          if (pid[pitMaster[0].pid].aktor == DAMPER) {      // Einfacher Damper
            pitMaster[id].msec = pitMaster[id].dcmin;
            if (pitMaster[0].value > 0) pitMaster[id].msec = pitMaster[id].dcmax;
          }
          pitMaster[id].timer0 = micros();
          noInterrupts();
          digitalWrite(pitMaster[id].io, HIGH);
          while (micros() - pitMaster[id].timer0 < pitMaster[id].msec) {}  //delayMicroseconds() ist zu ungenau
          digitalWrite(pitMaster[id].io, LOW);
          interrupts();
          break;   
      }
    }
  } else {    // TURN OFF PITMASTER
    disableHeater(id);
  }
  
}






