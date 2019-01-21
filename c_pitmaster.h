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

#define PM_DEBUG              // ENABLE SERIAL AUTOTUNE DEBUG MESSAGES

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

#define OPL_FALL  97              // OPEN LID LIMIT FALLING
#define OPL_RISE  100             // OPEN LID LIMIT RISING
#define OPL_PAUSE 300             // OPEN LID PAUSE

#define PIDKIMAX  95              // ANTI WINDUP LIMIT MAX
#define PIDKIMIN  0               // ANTI WINDUP LIMIT MIN

#define ATOVERTEMP 30             // AUTOTUNE OVERTEMPERATURE LIMIT
#define ATTIMELIMIT 120L*60000L   // AUTOTUNE TIMELIMIT

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Open Lid
void open_lid_init() {

  // nur Pitmaster 0
  opl.detected = false;
  for (int i = 0; i < 5; i++) {
    opl.ref[i] = 0;
  }
  opl.temp = 0;
  opl.count = 0;
}


void open_lid() {

  if (pitMaster[0].active == AUTO && pid[pitMaster[0].pid].opl) {
    opl.ref[0] = opl.ref[1];
    opl.ref[1] = opl.ref[2];
    opl.ref[2] = opl.ref[3];
    opl.ref[3] = opl.ref[4];
    opl.ref[4] = ch[pitMaster[0].channel].temp;

    float temp_ref = (opl.ref[0] + opl.ref[1] + opl.ref[2]) / 3.0;

    // erkennen ob Temperatur wieder eingependelt oder Timeout
    if (opl.detected) {  // Open Lid Detected
       
      opl.count--;

      // extremes Überschwingen vermeiden
      if (opl.temp > pitMaster[0].set && ch[pitMaster[0].channel].temp < pitMaster[0].set) opl.temp = pitMaster[0].set;
    
      if (opl.count <= 0)  // Timeout
        opl.detected = false;
      
      else if (ch[pitMaster[0].channel].temp > (opl.temp * (OPL_RISE/100.0)))    // Lid Closed
        opl.detected = false;
      
    } else if (ch[pitMaster[0].channel].temp < (temp_ref * (OPL_FALL/100.0))) {    // Opened lid detected!
      // Wenn Temp innerhalb der letzten beiden Messzyklen den falling Wert unterschreitet
    
      opl.detected = true;
      //opl.temp = opl.ref[0];  // war bsiher pit_now, das ist aber schon zu niedrig     
      opl.temp = opl.ref[0];         
      opl.count = OPL_PAUSE / (INTERVALSENSOR/4.0);    // bezogen auf 250 ms Timer

      Serial.println("OPL");
    } 
    
  } else  {
    opl.detected = false;
  }
}


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

  pitMaster[id].resume = 1;   // aktuell immer wiederbeleben, eventuell bei bewusstem ON/OFF abschalten
  if (!pitMaster[id].resume) pitMaster[id].active = PITOFF; 

  if (pitMaster[id].active != MANUAL) pitMaster[id].value = 0;  // vll auch noch in disableHeater
  disableHeater(id, true);

}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Set Pitmaster Pin
void set_pitmaster(bool init) {

  // IOs Pitmaster0
  pinMode(PITMASTER0IO1, OUTPUT);
  digitalWrite(PITMASTER0IO1, LOW);
  pitMaster[0].io[0] = PITMASTER0IO1;
  pinMode(PITMASTER0IO2, OUTPUT);
  digitalWrite(PITMASTER0IO2, LOW);
  pitMaster[0].io[1] = PITMASTER0IO2;

  // Initialize Pitmaster0
  init_pitmaster(init, 0);

  // v2: pitsupply enable, v1 conflict typ k
  if (sys.hwversion > 1) {  
    pinMode(PITSUPPLY, OUTPUT);
    digitalWrite(PITSUPPLY, LOW);
  }

  // Initialize Open Lid Pitmaster0
  open_lid_init();

}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Set Default PID-Settings
void set_pid(byte index) {

  // raus: Kp_a, Ki_a, Kp_a, Ki_min, Ki_max, Switch,
  
  pidsize = 3; //3;
  //        Name,      Nr, Aktor,  Kp,    Ki,  Kd, DCmin, DCmax, JP...
  pid[0] = {"SSR SousVide", 0, 0, 104,   0.2,   0,  0, 100, 100};
  pid[1] = {"TITAN 50x50",  1, 1, 3.8,  0.01, 128, 25, 100, 70};
  pid[2] = {"Kamado 50x50", 2, 1, 7.0, 0.019, 630, 25, 100, 70};

  if (index)  pid[2] = {"Servo", 2, 2, 12.0, 0.09, 0, 20, 80, 100};

}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// PID
float PID_Regler(byte id){ 

  // see: http://rn-wissen.de/wiki/index.php/Regelungstechnik
  // see: http://www.ni.com/white-paper/3782/en/

  float x = ch[pitMaster[id].channel].temp;         // IST
  float w = pitMaster[id].set;                      // SOLL
  byte ii = pitMaster[id].pid;                      // PID
  
  // PID Parameter
  float kp, ki, kd;
  kp = pid[ii].Kp;
  ki = pid[ii].Ki;
  kd = pid[ii].Kd;
  
  int32_t diff;
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

  // JUMP DROSSEL
  pid[ii].jumpth = (w * 0.05);
  if (pid[ii].jumpth > (100.0/kp)) pid[ii].jumpth = 100.0/kp;
  Serial.println(pid[ii].jumpth);
  if (diff > pid[ii].jumpth) pitMaster[id].jump = true;  // Memory bis Soll erreicht
  else if (diff <= 0) pitMaster[id].jump = false;
    
  //float e = w - x;                          
  
  // Proportional-Anteil
  float p_out = kp * e;                     
  
  // Differential-Anteil
  float edif = (e - pitMaster[id].elast)/(pitMaster[id].pause/1000.0);   
  pitMaster[id].elast = e;
  float d_out = kd * edif; 
  if ((x-w) > 0) d_out = 0;       // Begrenzung auf Untertemperaturbereich

  // i-Anteil wechsl: https://github.com/WLANThermo/WLANThermo_v2/blob/b7bd6e1b56fe5659e8750c17c6dd1cd489872f6c/software/usr/sbin/wlt_2_pitmaster.py
  // Integral-Anteil
  float i_out;
  if (ki != 0) {
    
    // Sprünge im Reglerausgangswert bei Anpassung von Ki vermeiden
    if (ki != pitMaster[id].Ki_alt) {
      pitMaster[id].esum = (pitMaster[id].esum * pitMaster[id].Ki_alt) / ki;
      pitMaster[id].Ki_alt = ki;
    }
    
    // Anti-Windup I-Anteil
    // Keine Erhöhung I-Anteil wenn Regler bereits an der Grenze ist
    if (p_out < PITMAX) { //if ((p_out + d_out) < PITMAX) {
      pitMaster[id].esum += e * (pitMaster[id].pause/1000.0);
    }
    
    // Anti-Windup I-Anteil (Limits)
    if (pitMaster[id].esum * ki > PIDKIMAX) pitMaster[id].esum = PIDKIMAX/ki;
    else if (pitMaster[id].esum * ki < PIDKIMIN) pitMaster[id].esum = PIDKIMIN/ki;
    
    i_out = ki * pitMaster[id].esum;
    
  } else {
    // Historie vergessen, da wir nach Ki = 0 von 0 aus anfangen
    pitMaster[id].esum = 0;
    i_out = 0;
    pitMaster[id].Ki_alt = 0;
  }
                    
  // PID-Regler berechnen
  float y = p_out + i_out + d_out;  
  y = constrain(y,PITMIN,PITMAX);           // Auflösung am Ausgang ist begrenzt            
  
  PMPRINTLN("[PM]\tPID:" + String(y,1) + "\tp:" + String(p_out,1) + "\ti:" + String(i_out,2) + "\td:" + String(d_out,1));
  
  return y;
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Control - Pitmaster 12V Supply  only Pitmaster0
void pitsupply(bool out, byte id) {

  if (id == 0 && sys.hwversion > 1) {
    //if () out = HIGH;  // SSR || FAN
    digitalWrite(PITSUPPLY, out);
    sys.transform = out;
  }
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Clear PID - reset PID store value
void clear_PID_Regler(byte id) {
  
  pitMaster[id].esum  = 0;
  pitMaster[id].elast = 0;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// TURN PITMASTER OFF
void disableHeater(byte id, bool hold) {
  // Anschlüsse ausschalten

  if (!pitMaster[id].disabled) {

    for (int ii = 0; ii < 2; ii++) {
  
      PMPRINTF("OFF: %u,\t %u,\t %u\r\n", id, pitMaster[id].io[ii], pitMaster[id].aktor[ii]);

      // https://github.com/esp8266/Arduino/issues/2175
      // erst ab v2.4.0
      // pinMode(pitMaster[id].io, OUTPUT);      
      // digitalWrite(pitMaster[id].io,LOW);     // automatic reset Output Mode
  
      if (pitMaster[id].aktor[ii] == FAN || pitMaster[id].aktor[ii] == FAN2) 
        analogWrite(pitMaster[id].io[ii], LOW);  // FAN
      else digitalWrite(pitMaster[id].io[ii], LOW); // SSR or Servo

    }
    
    pitsupply(0, id); // 12V Supply abschalten, falls nötig

    if (!hold) {
      pitMaster[id].active = PITOFF;      // turn off
      pitMaster[id].value = 0;            // reset value
    }
    
    pitMaster[id].event = false;        // reset ssr event
    pitMaster[id].msec = 0;             // reset time variable
    pitMaster[id].stakt = 0;            // disable servo event

    pitMaster[id].jump = false;

    clear_PID_Regler(id);               // rest pid

    pitMaster[id].disabled = true;
  }
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void disableAllHeater() {
  disableHeater(0);
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

float maxvalue(float a,float b) {
    return a>b?a:b;
}

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// START AUTOTUNE
void startautotune(byte id) {

  // only Pitmaster 0
  if (id == 0) {

    for (int i = 0; i < 3; i++) {         // CLEAR VECTORS
      autotune.temp[i] = 0;
      autotune.time[i] = 0;  
    }

    autotune.value = 0;
    autotune.set = pitMaster[id].set * 0.9;   // ist INT damit nach unten gerundet  // SET TEMPERTURE: 10% weniger als Reserve
    float currenttemp = ch[pitMaster[id].channel].temp;

    // macht Autotune überhaupt Sinn?
    if (autotune.set - currenttemp > (pitMaster[id].set * 0.05)) {  // mindestens 5% von Set
      disableAllHeater();                    // SWITCH OF HEATER
      pitMaster[id].active = AUTOTUNE;                  
      autotune.run = 2;                     // AUTOTUNE INITALIZED
      autotune.max = pid[pitMaster[id].pid].jumppw;
      PMPRINTPLN("[AT]\t Start!");
    } else {
      pitMaster[id].active = AUTO;          // sollte eh schon
      autotune.run = 0;
    }

    pid[pitMaster[id].pid].autotune = false;  // Zurücksetzen
  }
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// STOP AUTOTUNE
void stopautotune(byte id) {

  if (id == 0) {
  
    autotune.value = 0;
    autotune.run = 0;
  
    if ((autotune.stop == 1)) {         // sauber beendet
      pitMaster[id].active = AUTO;      // Pitmaster in AUTO fortsetzen
      PMPRINTPLN("Autotune beendet");
    } else pitMaster[id].active = PITOFF;

    setconfig(ePIT,{});     // save
  
    question.typ = TUNE;
    drawQuestion(autotune.stop);
    autotune.stop = 0;
  }
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// AUTOTUNE
float autotunePID(byte id) {

  // http://www.matthias-trier.de/EUFH_MSR_Vertriebsing_Reglerauswahl_140411.pdf

  float currentTemp = ch[pitMaster[id].channel].temp;
  unsigned long time = millis();

    // Startbedingungen herstellen  
    if (autotune.run == 2) {
      
      // Phase A1 - Aktor auf MAX einschalten
      autotune.value = autotune.max;
      autotune.run = 3;
    
    } else if (autotune.run == 3) {

      // Phase A2 - Start Steigungstest
      if (autotune.temp[0] == 0) {
        autotune.temp[0] = currentTemp;
        autotune.time[0] = time;
        //Serial.print("0: "); Serial.print(autotune.time[0]); Serial.print(" | "); Serial.println(autotune.temp[0]);

      // Vmax bestimmen
      } else if (autotune.temp[1] == 0 && currentTemp < autotune.set) {
        float vmax;
        vmax = 1000.0*(currentTemp - autotune.temp[0])/(time - autotune.time[0]);
        if (vmax > autotune.vmax) {
          autotune.vmax = vmax;
          PMPRINTP("vmax: "); PMPRINTLN(vmax);
        }
        autotune.temp[0] = currentTemp;
        autotune.time[0] = time;

      // Phase A3 - Ende Steigung, Anfang Überschwinger
      } else if (autotune.temp[1] == 0 && currentTemp > autotune.set) {
        autotune.value = 0; // Pitmaster ruhen
        autotune.temp[1] = currentTemp;
        autotune.temp[2] = currentTemp;
        autotune.time[1] = time;
        //Serial.print("1: "); Serial.print(autotune.time[1]); Serial.print(" | "); Serial.println(autotune.temp[1]);

      // Während Überschwinger
      } else if (autotune.temp[1] > 0 && currentTemp > autotune.temp[2]) {
        autotune.temp[2] = currentTemp;   // Temperatur nachziehen

      // Phase A4 - Ende Überschwinger (Schwankungen ausgleichen, eventuell abhängig vom Aktor machen) 
      } else if (autotune.temp[1] > 0 && currentTemp*1.01 < autotune.temp[2]) {
        autotune.time[2] = time;
        //Serial.print("2: "); Serial.print(autotune.time[2]); Serial.print(" | "); Serial.println(autotune.temp[2]);
        autotune.value = 25;                    // Teillast müsste reichen um eine wirkung zu sehen
        autotune.run = 4;
      }
      
    } else {

      if (currentTemp > autotune.temp[2]) {   // Totzeit bestimmen, wenn Temperatur wieder steigt
        
        // Berechnung der Verzugszeit Tt = Tu 
        uint32_t Tt = (millis() - autotune.time[2]) / 1000;      // Wiederanlaufzeit
        PMPRINTLN(Tt);
           
        // dT = Überschwinger nach Abschaltung
        float dT = 1.5 * (autotune.temp[2] - autotune.temp[1]);
        PMPRINTLN(dT);
        
        // Proportionalbereich Xp aus Anstiegsgeschwindigkeit und Verzugszeit
        float Xp = 0.83 * Tt * autotune.vmax * (PITMAX/autotune.max);
        float K = 100.0 / Xp;                         
        K = maxvalue(K, 100.0/dT);
        autotune.Kp = constrain((uint32_t) (K*10.0), 0, 1000)/10.0;   // Kp > 100 unsinnig
    
        // Integralzeit: Zeit zum Erreichen vom P-Anteil
        // Tn1 = (dT * 4.0)/autotune.vmax;
        // Tn2 = Tt * 4;                    // mehr Verzögerung durch langsames Anwachsen
        float Tn = maxvalue(((dT * 4.0)/autotune.vmax), (Tt * 4));
        PMPRINTLN(Tn);
        K = autotune.Kp / Tn;           // Ki = Kp / (Tn)
        autotune.Ki = constrain((uint32_t) (K*100.0), 0, 100)/100.0;            // Ki > 1 ist Unsinn

        // D-Anteil über Verhältnis Überschwinger und Anstiegsgeschwindigkeit (Brems-Funktion)
        K = 7.0 * dT;           // Faktor gibt Agressivität der Bremsfunktion vor
        K = constrain((uint16_t) (K), 0, 100);      // max 100% bei max. Geschwindigkeit, sonst stocken (i beachten)
        autotune.Kd = (uint16_t) (K / autotune.vmax);
        
        PMPRINTP("[AT]\tKp: ");
        PMPRINT(autotune.Kp);
        PMPRINTP(" Ki: ");
        PMPRINT(autotune.Ki);
        PMPRINTP(" Kd: ");
        PMPRINTLN(autotune.Kd);

        byte ii = pitMaster[id].pid;
        pid[ii].Kp = autotune.Kp;  
        pid[ii].Ki = autotune.Ki;   
        pid[ii].Kd = autotune.Kd;                    
        //pid[ii].Kp_a = 0;                
        //pid[ii].Ki_a = 0;                   
        //pid[ii].Kd_a = 0;

        PMPRINTPLN("[AT]\tFinished!");
        //disableHeater();
        autotune.stop = 1;
      }
    }
  
    if (currentTemp > (autotune.set + ATOVERTEMP))  {   // FEHLER
      PMPRINTPLN("f:AT OVERTEMP");
      disableHeater(id);
      autotune.stop = 2;
      return 0;
    }
    
    if ((time - autotune.time[0]) > ATTIMELIMIT) {   // 20 Minutes
      PMPRINTPLN("f:AT TIMEOUT");
      disableHeater(id);
      autotune.stop = 3;
      return 0;
    }

  return autotune.value; 
       
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Start Duty Cycle Test
void DC_start(bool dc, byte aktor, int val, byte id) {

  if (pitMaster[id].active != DUTYCYCLE) {
    dutyCycle[id].dc = dc;                  // min or max
    dutyCycle[id].aktor = aktor;            // test actor
    dutyCycle[id].value = val;              // test value
    dutyCycle[id].timer = millis();         // shutdown timer
    pitMaster[id].last = 0;                 // react now

    // save current state
    switch (pitMaster[id].active) {
      case PITOFF: dutyCycle[id].saved = PITOFF; break;
      case MANUAL: dutyCycle[id].saved = pitMaster[id].value; break;
      case AUTOTUNE: 
        dutyCycle[id].saved = PITOFF; // stop autotune
        autotune.stop = 2;
        break;
      case AUTO: dutyCycle[id].saved = -1; break;
    }
    disableHeater(id);                       // stop current actor
    PMPRINTPLN("[DC]\tStart!");
    pitMaster[id].active = DUTYCYCLE;       // set duty cycle state
  }
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Stop Duty Cycle Test
void DC_stop(byte id) {
  
  if (pitMaster[id].active == DUTYCYCLE && (millis() - dutyCycle[id].timer > 10000)) {
    disableHeater(id);   // stop test actor
    if (dutyCycle[id].saved == 0) {               // off
      pitMaster[id].active = PITOFF;
    } else if (dutyCycle[id].saved > 0) {
      pitMaster[id].value = dutyCycle[id].saved;  // manual
      pitMaster[id].active = MANUAL;
    } else pitMaster[id].active = AUTO;           // auto
    PMPRINTPLN("[DC]\tStop!");
  }
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Aktor Limits
void aktor_limits(byte id, byte pio) {

  // limits from global actor
  uint16_t dcmin = pid[pitMaster[id].pid].DCmin*10;    // 1. Nachkommastelle 
  uint16_t dcmax = pid[pitMaster[id].pid].DCmax*10;    // 1. Nachkommastelle

  // overwrite, if DUTYCYCLE-Test (maximum range)
  if (pitMaster[id].active == DUTYCYCLE) {
    dcmin = 0;
    dcmax = 1000;                               // 1. Nachkommastelle
  }

  byte aktor = pitMaster[id].aktor[pio];

  // convert duty cycle (%) into actor dependent range
  switch (aktor) {

    case NOAR:    // NO AKTOR
      break;
      
    case SSR:     // SSR
      pitMaster[id].dcmin = map(dcmin,0,1000,0,pitMaster[id].pause);
      pitMaster[id].dcmax = map(dcmax,0,1000,0,pitMaster[id].pause);
      break;

    case FAN2:    // SERVO-SUPPORT (need maximum range)
      dcmin = 0;
      dcmax = 1000; 
    case FAN:     // FAN
      pitMaster[id].dcmin = map(dcmin,0,1000,0,1024);
      pitMaster[id].dcmax = map(dcmax,0,1000,0,1024);
      break;   

    case SERVO2:  // DAMPER-SERVO (need servo puls interval)
      dcmin = pid[2].DCmin*10;;
      dcmax = pid[2].DCmax*10;
    case SERVO:   // SERVO
      pitMaster[id].dcmin = map(dcmin,0,1000,SERVOPULSMIN,SERVOPULSMAX);
      pitMaster[id].dcmax = map(dcmax,0,1000,SERVOPULSMIN,SERVOPULSMAX);
      break;   
  }
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Pitmaster Pause
void check_pit_pause(byte id) {

  // Global Pitmaster Aktor from PID-Profil
  byte aktor = pid[pitMaster[id].pid].aktor;

  // overwrite, if DUTYCYCLE-Test
  if (pitMaster[id].active == DUTYCYCLE) {
    aktor = dutyCycle[id].aktor;
  }

  // Initialize
  pitMaster[id].aktor[0] = NOAR;    // IO1
  pitMaster[id].aktor[1] = NOAR;    // IO2
  pitMaster[id].pause = 1000;       // Control Time

  // select local aktor by hardwareversion
  switch (aktor) {

    case SSR:  
      pitMaster[id].aktor[0] = SSR; 
      pitMaster[id].pause = 2000;
      break;
              
    case FAN: 
      pitMaster[id].aktor[0] = FAN;
      break;   
      
    case SERVO: 
      if (sys.hwversion == 1) pitMaster[id].aktor[0] = SERVO;
      else if (sys.hwversion > 1) {
        pitMaster[id].aktor[0] = FAN2;   // Spannungsversorgung des Servos
        pitMaster[id].aktor[1] = SERVO;
      }
      break;   

    case DAMPER: 
      if (sys.hwversion > 1) pitMaster[id].aktor[0] = FAN;
      if (sys.hwversion > 1) pitMaster[id].aktor[1] = SERVO2;
      break;
  }
  
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
// Control - Pitmaster
void pitmaster_control(byte id) {

  // Control Autotune
  if (autotune.stop > 0) stopautotune(id);
  else if (autotune.run == 1) startautotune(id);

  // Stop Duty Cylce Test
  DC_stop(id);
  
  // ESP PWM funktioniert nur bis 10 Hz Trägerfrequenz stabil, daher eigene Taktung
  if (pitMaster[id].active > 0) {

    pitMaster[id].disabled = false;   // pitmaster is running

    // Check Pitmaster Pause
    check_pit_pause(id);

    // SSR: Ende eines HIGH-Intervalls, wird durch pit_event nur einmal pro Intervall durchlaufen
    if (pitMaster[id].event && (millis() - pitMaster[id].last > pitMaster[id].msec)) {
      digitalWrite(pitMaster[id].io[0], LOW); // SSR nur an IO1
      pitMaster[id].event = false;
    }

    // neuen Stellwert bestimmen
    if (millis() - pitMaster[id].last > pitMaster[id].pause) {
     
      pitMaster[id].last = millis();
      byte aktor;

      // PITMASTER TYP
      switch (pitMaster[id].active) {

        case DUTYCYCLE:
          aktor = dutyCycle[id].aktor;
          // Startanlauf: bei Servo beide male zuerst in die Mitte, bei Fan nur unten
          if (millis() - dutyCycle[id].timer < 1000) {
            // es fehlt noch DAMPER
            if ((aktor == FAN && !dutyCycle[id].dc) || aktor == SERVO) pitMaster[id].value = 50;
          } else pitMaster[id].value = dutyCycle[id].value/10.0;
          pitMaster[id].timer0 = 0;     // Überbrückung Anlauf-Prozess
          break;

        case AUTOTUNE:
          pitMaster[id].value = autotunePID(id);
          break;

        case AUTO:
          if (!opl.detected)  pitMaster[id].value = PID_Regler(id);      
          else pitMaster[id].value = 0; 
          break;

        case MANUAL:    
          // falls manual wird value vorgegeben
          break;   

        case MYAUTO:
          //myPitmaster();
          break;
      }

      float value;

      for (byte pp = 0; pp < 2; pp++) {      // pro Pitmaster jeweils 2 IO

        // Initalize and caching actor limits
        aktor_limits(id, pp);
        aktor = pitMaster[id].aktor[pp];
        
        value = pitMaster[id].value;
        // JUMP DROSSEL
        if (pitMaster[id].jump && (value > pid[pitMaster[id].pid].jumppw))
          value = pid[pitMaster[id].pid].jumppw;

        // PITMASTER AKTOR
        switch (aktor) {

          case SSR:       // nur bei IO1
            pitsupply(1, id);   // immer 12V Supply
            pitMaster[id].msec = map(value,0,100,pitMaster[id].dcmin,pitMaster[id].dcmax); 
            PMPRINTF("SSR: %u,\t %u,\t %ums\r\n", id, pitMaster[id].io[pp], pitMaster[id].msec);
            if (pitMaster[id].msec > 0) digitalWrite(pitMaster[id].io[pp], HIGH);
            if (pitMaster[id].msec < pitMaster[id].pause) pitMaster[id].event = true;  // außer bei 100%
            break;

          case FAN2:      // SERVO-SUPPORT (IO1 = FAN2; IO2 = SERVO)
            value = 50;   // 6V
          case FAN:  
            pitsupply(sys.pitsupply, id);   // 12V Supply nur falls aktiviert
            PMPRINTF("FAN: %u,\t %u,\t %u%%\r\n", id, pitMaster[id].io[pp], (int)value);
            if (value == 0) {   
              analogWrite(pitMaster[id].io[pp], 0);  // bei 0 soll der Lüfter auch stehen
              pitMaster[id].timer0 = millis();  
            } else {
              // Anlaufhilfe: ein Zyklus auf 30% wenn von 0% kommend
              if (millis() - pitMaster[id].timer0 < pitMaster[id].pause*2 && value < 30) { 
                analogWrite(pitMaster[id].io[pp],map(30,0,100,pitMaster[id].dcmin,pitMaster[id].dcmax));   // BOOST
              } else {
                analogWrite(pitMaster[id].io[pp],map(value,0,100,pitMaster[id].dcmin,pitMaster[id].dcmax));
              }
            }
            break;

          case SERVO2:    // DAMPER-SERVO-CONTROL          
            if (value > 0) value = 100;
          case SERVO:   // Achtung bei V2 mit den 12V bei Anschluss an Stromversorgung
          
            uint16_t smsec = mapfloat(value,0,100,pitMaster[id].dcmin,pitMaster[id].dcmax);

            // kleine Bewegungen vermeiden (in msec)
            if (abs(pitMaster[id].msec - smsec) > 40) pitMaster[id].nmsec = smsec;
            PMPRINTF("SVO: %u,\t %u,\t %uus\r\n", id, pitMaster[id].io[pp], pitMaster[id].nmsec);
            // Servosteuerung aktivieren
            if (pitMaster[id].stakt == 0) pitMaster[id].stakt = millis();
            // Hardware Timer timer0 / timer1 blocked by STA
            // Software Timer os_timer no us (only with USE_US_TIMER but destroyed millis()/micros()
            break;   
        }
      } 
    }
    
  } else {    // TURN OFF PITMASTER
    disableHeater(id);
    if(id == 0) open_lid_init();
  }
}


byte servointerrupt;
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Control - Manuell PWM Servo
void updateServo() {

  servointerrupt = 1;

  for (int id = 0; id < PITMASTERSIZE; id++) {

    byte ii;
    if (pitMaster[id].aktor[0] == SERVO) ii = 0;
    else ii = 1;

    if (pitMaster[id].stakt > 0) {                        // Servo aktiviert
      if (millis() - pitMaster[id].stakt > 19)   {    // 50 Hz Takt
        pitMaster[id].stakt = millis(); 
        
        delayMicroseconds(5); // weniger Zittern am Servo

        // Servoposition langsam annähern
        if (pitMaster[id].msec == 0) pitMaster[id].msec = pitMaster[id].nmsec;
        else if (pitMaster[id].nmsec - pitMaster[id].msec > 25) pitMaster[id].msec += 25;
        else if (pitMaster[id].msec - pitMaster[id].nmsec > 25) pitMaster[id].msec -= 25;
        else pitMaster[id].msec = pitMaster[id].nmsec;

        // Reihenfolge wichtig
        //noInterrupts();   // erzeugt Störungen im StepUp
        digitalWrite(pitMaster[id].io[ii], HIGH);
        pitMaster[id].timer0 = micros();
        while (micros() - pitMaster[id].timer0 < pitMaster[id].msec) {}  //delayMicroseconds() ist zu ungenau
        digitalWrite(pitMaster[id].io[ii], LOW);
        //interrupts();
        
        servointerrupt = servointerrupt*1;        // Signal abgeschlossen, frei für Rest
      } else servointerrupt = servointerrupt*0;   // Signal noch nicht abgeschlossen
    } else servointerrupt = servointerrupt*1;     // nichts zum abschließen
  
  }

  // jeder Servo braucht einen eigenen Timer, ACHTUNG: Blocktime x ServoAnzahl darf nicht zu groß werden
}







