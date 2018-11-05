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

  if (pitMaster[id].active != MANUAL) pitMaster[id].value = 0;  // vll auch noch in disableHeater
  disableHeater(id, true);
  
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Open Lid
void open_lid_init() {

      // nur Pitmaster 1
      opl.detected = false;
      //opl.ref = {0.0, 0.0, 0.0, 0.0, 0.0};
      for (int i = 0; i < 5; i++) {
        opl.ref[i] = 0;
      }
      opl.temp = 0;
      opl.count = 0;
}

#define OPL_FALL  97
#define OPL_RISE  100
#define OPL_PAUSE 300

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
// Set Pitmaster Pin
void set_pitmaster(bool init) {
  
  //pitMaster[0].io = PITMASTER1;     // nur in disableHeater
  pinMode(PITMASTER1, OUTPUT);
  digitalWrite(PITMASTER1, LOW);
  init_pitmaster(init, 0);
  
  //pitMaster[1].io = PITMASTER2;     // nur in disableHeater
  pinMode(PITMASTER2, OUTPUT);
  digitalWrite(PITMASTER2, LOW);
  init_pitmaster(init, 1);

  if (sys.hwversion > 1) {  // v2: pitsupply enable
    pinMode(PITSUPPLY, OUTPUT);
    digitalWrite(PITSUPPLY, LOW);
    pitMaster[1].active = PITOFF; // damit der Switch funktioniert
    bodyWebHandler.setservoV2(0);
    bodyWebHandler.setdamperV2();
  } else {  // v1: only 1 pitmaster
    pitMaster[1].active = PITOFF;
  }

  open_lid_init();

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
  // see: http://www.ni.com/white-paper/3782/en/

  float x = ch[pitMaster[id].channel].temp;         // IST
  float w = pitMaster[id].set;                      // SOLL
  byte ii = pitMaster[id].pid;                      // PID
  
  // PID Parameter
  float kp, ki, kd;
  kp = pid[ii].Kp;
  ki = pid[ii].Ki;
  kd = pid[ii].Kd;
  
  uint32_t diff;
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
    if (pitMaster[id].esum * ki > pid[ii].Ki_max) pitMaster[id].esum = pid[ii].Ki_max/ki;
    else if (pitMaster[id].esum * ki < pid[ii].Ki_min) pitMaster[id].esum = pid[ii].Ki_min/ki;
    
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
void disableHeater(byte id, bool hold) {
  // Anschlüsse ausschalten

  if (!pitMaster[id].disabled) {
  
    PMPRINTF("OFF: %u,\t %u,\t %u\r\n", id, pitMaster[id].io, pid[pitMaster[id].pid].aktor);

    // https://github.com/esp8266/Arduino/issues/2175
    // erst ab v2.4.0
    //pinMode(pitMaster[id].io, OUTPUT);      
    //digitalWrite(pitMaster[id].io,LOW);     // automatic reset Output Mode
  
    if (pitMaster[id].pwm) analogWrite(pitMaster[id].io, LOW);  // FAN
    else digitalWrite(pitMaster[id].io, LOW); // SSR + Servo
    pitMaster[id].pwm = false;
    
    pitsupply(0, id); // 12V Supply abschalten, falls nötig

    // Reset IO-Ports
    if (id == 0) pitMaster[id].io = PITMASTER1;
    else if (id == 1) pitMaster[id].io = PITMASTER2;

    if (!hold) {
    pitMaster[id].active = PITOFF;      // turn off
    pitMaster[id].value = 0;            // reset value
    }
    pitMaster[id].event = false;        // reset ssr event
    pitMaster[id].msec = 0;             // reset time variable
    pitMaster[id].stakt = 0;            // disable Servo Control

    clear_PID_Regler(id);               // rest pid

    pitMaster[id].disabled = true;
  }
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
// START AUTOTUNE
void startautotune(byte id) {

  if (id == 0) {

    for (int i = 0; i < 3; i++) {         // CLEAR VECTORS
      autotune.temp[i] = 0;
      autotune.time[i] = 0;  
    }

    autotune.value = 0;
    autotune.set = pitMaster[id].set * 0.9;   // ist int damit nach unten gerundet  // SET TEMPERTURE: 10% weniger als Reserve
    float currenttemp = ch[pitMaster[id].channel].temp;

    // macht Autotune überhaupt Sinn?
    if (autotune.set - currenttemp > (pitMaster[id].set * 0.05)) {  // mindestens 5% von Set
      disableAllHeater();                    // SWITCH OF HEATER
      pitMaster[id].active = AUTOTUNE;                  
      autotune.run = 2;                     // AUTOTUNE INITALIZED
      autotune.max = PITMAX/2;
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
  
    if ((autotune.stop == 1)) { // sauber beendet
      pitMaster[id].active = AUTO;     // Pitmaster in AUTO fortsetzen
      Serial.println("Autotune beendet");
    } else pitMaster[id].active = PITOFF;

    setconfig(ePIT,{});
  
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
      
      // Phase A1
      autotune.value = autotune.max;                    // Aktor auf MAX einschalten
      autotune.run = 3;
    
    } else if (autotune.run == 3) {

      // Phase A2
      // Start Steigung
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
          Serial.print("vmax: ");
          Serial.println(vmax);
        }
        autotune.temp[0] = currentTemp;
        autotune.time[0] = time;

      // Ende Steigung, Anfang Überschwinger
      } else if (autotune.temp[1] == 0 && currentTemp > autotune.set) {
        autotune.value = 0; // Pitmaster ruhen
        autotune.temp[1] = currentTemp;
        autotune.temp[2] = currentTemp;
        autotune.time[1] = time;
        //Serial.print("1: "); Serial.print(autotune.time[1]); Serial.print(" | "); Serial.println(autotune.temp[1]);

      // Während Überschwinger
      } else if (autotune.temp[1] > 0 && currentTemp > autotune.temp[2]) {
        autotune.temp[2] = currentTemp;   // Temperatur nachziehen

      // Ende Überschwinger (Schwankungen ausgleichen, eventuell abhängig vom Aktor machen) 
      } else if (autotune.temp[1] > 0 && currentTemp*1.01 < autotune.temp[2]) {
        autotune.time[2] = time;
        //Serial.print("2: "); Serial.print(autotune.time[2]); Serial.print(" | "); Serial.println(autotune.temp[2]);
        autotune.value = autotune.max;                    // Aktor auf MAX einschalten
        autotune.run = 4;
      }
      
    } else {

      if (currentTemp > autotune.temp[2]) {   // Totzeit bestimmen, wenn Temperatur wieder steigt
        
        // Berechnung der Verzugszeit Tt = Tu 
        uint32_t Tt = (millis() - autotune.time[2]) / 1000;      // Wiederanlaufzeit
        Serial.println(Tt);
           
        // dT = Überschwinger nach Abschaltung
        float dT = 1.5 * (autotune.temp[2] - autotune.temp[1]);
        Serial.println(dT);
        
        // Proportionalbereich Xp aus Anstiegsgeschwindigkeit und Verzugszeit
        float Xp = 0.83 * Tt * autotune.vmax * (PITMAX/autotune.max);
        float K = 100.0 / Xp;                         
        autotune.Kp = constrain((uint32_t) (K*10.0), 0, (uint32_t) (1000.0/dT))/10.0;     // Kp < 100/dT
    
        K = 100.0 / (0.1*pitMaster[id].set);
        autotune.Kp_a = constrain((uint32_t) (K*10.0), 0, autotune.Kp*10.0)/10.0;  // nicht größer als Kp

        // Integralzeit: Zeit zum Erreichen vom P-Anteil
        float Tn_min = (dT * 4.0)/autotune.vmax;
        uint32_t Tn = Tt * 2;
        Tn = constrain(Tn, (uint32_t) Tn_min, 1000);
        Serial.println(Tn);
        K = autotune.Kp / Tn;           // Ki = Kp / (Tn)
        autotune.Ki = constrain((uint32_t) (K*100.0), 0, 100)/100.0;            // Ki > 1 ist Unsinn

        // D-Anteil über Zeitkonstante
        Kp = 10;
        dT = 20;
        Kp * 10°C = 100

        K = 5.0 * dT;
        K = constrain((uint32_t) (K*10.0), 0, 900)/10.0;      // max 90% bei max. Geschwindigkeit
        autotune.Kd = K / autotune.vmax;
        
        //K = autotune.Kp * Tn / 4.0;
        //autotune.Kd = constrain((uint32_t) (K*10.0), 0, 6000)/10.0;      // Kd > 600 ist Unsinn

        //autotune.Kd = 0;
        //autotune.Ki_a = 0;
        //autotune.Kd_a = 0;

        PMPRINTP("[AT]\tKp_a: ");
        PMPRINT(autotune.Kp_a);
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
        pid[ii].Kp_a = autotune.Kp_a;                
        pid[ii].Ki_a = 0;                   
        pid[ii].Kd_a = 0;

        PMPRINTPLN("[AT]\tFinished!");
        //disableHeater();
        autotune.stop = 1;
      }
    }
  
    if (currentTemp > (autotune.set + autotune.overtemp))  {   // FEHLER
      PMPRINTPLN("f:AT OVERTEMP");
      disableHeater(id);
      autotune.stop = 2;
      return 0;
    }
    
    if ((time - autotune.time[0]) > autotune.timelimit) {   // 20 Minutes
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
    dutyCycle[id].dc = dc;
    dutyCycle[id].aktor = aktor;
    dutyCycle[id].value = val;
    dutyCycle[id].timer = millis();

    pitMaster[id].last = 0;

    // save current state
    switch (pitMaster[id].active) {
      case PITOFF: dutyCycle[id].saved = PITOFF; break;
      case MANUAL: dutyCycle[id].saved = pitMaster[id].value; break;
      case AUTOTUNE: 
        dutyCycle[id].saved = PITOFF; // autotune abbrechen
        autotune.stop = 2;
        break;
      case AUTO: dutyCycle[id].saved = -1; break;
      case VOLTAGE: dutyCycle[id].saved = PITOFF; break;
    }

    // set duty cycle state
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
  } else if (pitMaster[id].active == VOLTAGE) {
    aktor = FAN;
    dcmin = 0;
    dcmax = 1000; 
  } else {
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
      pause = 1000;   // 50 Hz kompatibel
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

/*
struct Gcode {
    long time;          // Zeitvorgabe
    uint32_t last;      // letzter Pitmasterdurchlauf
    byte mode;          // uebergebener Pitmaster Mode
    byte val;           // uebergebener Wert
    bool hold;          // Timer angehalten, Temperatur noch nicht erreicht
}
Gcode gcode;

enum {GCODEM, GCODEP1, GCODEP2, GCODEP3, GCODEP4 GCODEOFF};


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// GCODE-Pitmaster
float gcode_pitmaster(byte id) {

  // Zeitvariable die angibt, ob neue Zeile gelesen wird oder noch nicht
  if (!gcode.hold && gcode.time != -1) {
    gcode.time -= millis() - gcode.last;
    if (gcode.time < 0) gcode.time = 0;
  }

  gcode.last = millis();

  // Befehle
  // Jeweils ein Befehl pro Zeile
  // K1 = Kanal 1 etc.                im Pitmaster speichern
  // A0 = Aktor 0 (SSR, FAN, SERVO)   im Pitmaster speichern
  // C0 = PID 0
  
  // M100 = Mode-Manuell 100%         eigene Variable (zwei: Modus und Wert)
  // P = Mode-Auto (P0 = normaler PID; P2 = PID mit Zeitverzögerung; P3 = manuell heizen; P2 = manuell kühlen
  // P2 = wenn ist > soll dann 100% sonst 0%
  
  // S120 = Solltemperatur 120        im Pitmaster speichern

  // Zeit am Ende
  // T1000 = Zeit 1000                eigene Variable
  // T0 = unendlich

  // Interpreter
  if {gcode.time <= 0) {

    while 1 {
      // Zeile lesen bis eine Zeitvorgabe kommt
      File f = SPIFFS.open("/pitmaster.gcode", "r");
      // wenn Ende gefunden dann ein GCODEOFF schicken und abschalten
      String command = f.readStringUntil('\n');
      command = command.substring(0,command.indexOf(';')); 
      Serial.println(command);
      f.close();

      char com = command.charAt(0);
      command = command.remove(0);
      int val = command.toInt();

      if (com == "K") pitMaster[id].channel = val -1;
      else if (com == "C") pitMaster[id].pid = val;
      else if (com == "A") pid[pitMaster[id].pid].aktor = val;
      else if (com == "M") {
        gcode.mode = GCODEM;
        gcode.val = val;
      }
      else if (com == "P") {
        gcode.mode = val+1;
        if (val == 2) gcode.hold = true; // Warten auf Temperatur erreicht
        // hier wird noch ein Abschalten des Hold benötigt, sowie ein grafischer Hinweis
      }
      else if (com == "S") pitMaster[id].set = val;
      else if (com == "T") {
        if (val == 0) gcode.time = -1;  // unendlich lang
        else gcode.time = val;
        break; // Leseschleife unterbrechen
      }
    }
  }

  // Pitmastervorgabe anhand des gewaehlten Modus
  switch (gcode.mode) {

    case GCODEM:
      return gcode.val;

    case GCODEP0:
      return PID_Regler(id);

    case GCODEP0:
      return PID_Regler(id);

    case GCODEP2:
      if (ch[pitMaster[id].channel].temp < pitMaster[id].set) return 100;
      else return 0;

    case GCODEP3:
      if (ch[pitMaster[id].channel].temp > pitMaster[id].set) return 100;
      else return 0;

    default:
      return 0;
  }
}

  
  // Schaltung Hauptschalter:


  // ist > soll, dann manuell 100%
  // ist < soll, dann manuell 0%

  
    return 0;
}

*/



// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Control - Manuell PWM
void pitmaster_control(byte id) {

  // Stop Autotune
  if (autotune.stop > 0) stopautotune(id);
  else if (autotune.run == 1) startautotune(id);

  // Stop Duty Cylce Test
  DC_stop(id);
  
  // ESP PWM funktioniert nur bis 10 Hz Trägerfrequenz stabil, daher eigene Taktung
  if (pitMaster[id].active > 0) {

    pitMaster[id].disabled = false;

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
          aktor = pid[pitMaster[id].pid].aktor;
          if (!opl.detected)
            pitMaster[id].value = PID_Regler(id);      //myPitmaster();
          else pitMaster[id].value = 0; 
          break;

        case MANUAL:    // falls manual wird value vorgegeben
          aktor = pid[pitMaster[id].pid].aktor;
          break;

        case VOLTAGE:    // falls voltage wird value vorgegeben
          aktor = FAN;
          //pitMaster[id].value = 0;
          break;

        //case GCODE:
          // aktor = ;                // GCODE
          // pitMaster[id].value = gcode_pitmaster(id);  // GCODE
         // break;
      }

      // PITMASTER AKTOR
      switch (aktor) {

        case SSR:  
          pitsupply(1, id);   // immer 12V Supply
          pitMaster[id].msec = map(pitMaster[id].value,0,100,pitMaster[id].dcmin,pitMaster[id].dcmax); 
          PMPRINTF("SSR: %u,\t %u,\t %ums\r\n", id, pitMaster[id].io, pitMaster[id].msec);
          if (pitMaster[id].msec > 0) digitalWrite(pitMaster[id].io, HIGH);
          if (pitMaster[id].msec < pitMaster[id].pause) pitMaster[id].event = true;  // außer bei 100%
          break;

        case DAMPER:
          // PITMASTER 1 = FAN
          // PITMASTER 2 = SERVO 
        case FAN:  
          pitsupply(sys.pitsupply, id);   // 12V Supply nur falls aktiviert
          pitMaster[id].pwm = true;      // kennzeichne pwm
          PMPRINTF("FAN: %u,\t %u,\t %u%%\r\n", id, pitMaster[id].io, (int)pitMaster[id].value);
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
          // PITMASTER 1 = VOLTAGE or fix VUSB
          // PITMASTER 2 = SERVO  (gedrehte Pitmaster)
          //pitsupply(0, id);  // keine 12V Supply
          
          uint16_t smsec = mapfloat(pitMaster[id].value,0,100,pitMaster[id].dcmin,pitMaster[id].dcmax);

          // kleine Bewegungen vermeiden
          if (abs(pitMaster[id].msec - smsec) > 40) { // in msec
            pitMaster[id].nmsec = smsec;
          }

          // vereinfachte Damper-Logik
          if (pid[pitMaster[0].pid].aktor == DAMPER) {      // Einfacher Damper
            pitMaster[id].nmsec = pitMaster[id].dcmin;
            if (pitMaster[0].value > 0) pitMaster[id].nmsec = pitMaster[id].dcmax;
          }
          PMPRINTF("SVO: %u,\t %u,\t %uus\r\n", id, pitMaster[id].io, pitMaster[id].nmsec);
          // Servosteuerung aktivieren
          if (pitMaster[id].stakt == 0) pitMaster[id].stakt = millis();
          
          // Hardware Timer timer0 / timer1 blocked by STA
          // Software Timer os_timer no us (only with USE_US_TIMER but destroyed millis()/micros()

          // Servo läuft nach restart nicht weiter
          break;   
      }
    }
  } else {    // TURN OFF PITMASTER
    disableHeater(id);
    if(id == 0) open_lid_init();
  }
  
}


byte servointerrupt;

void updateServo() {

  servointerrupt = 1;

  for (int id = 0; id < PITMASTERSIZE; id++) {

    if (pitMaster[id].stakt > 0) {                        // Servo aktiviert
      if (millis() - pitMaster[id].stakt > 19)   {    // 50 Hz Takt
        pitMaster[id].stakt = millis(); 
        //Serial.println(micros() - pitMaster[id].timer0);
        delayMicroseconds(5); // weniger Zittern am Servo

        // Servoposition langsam annähern
        if (pitMaster[id].msec == 0) pitMaster[id].msec = pitMaster[id].nmsec;
        else if (pitMaster[id].nmsec - pitMaster[id].msec > 25) pitMaster[id].msec += 25;
        else if (pitMaster[id].msec - pitMaster[id].nmsec > 25) pitMaster[id].msec -= 25;
        else pitMaster[id].msec = pitMaster[id].nmsec;

        // Reihenfolge wichtig
        //noInterrupts();   // erzeugt Störungen im StepUp
        digitalWrite(pitMaster[id].io, HIGH);
        pitMaster[id].timer0 = micros();
        while (micros() - pitMaster[id].timer0 < pitMaster[id].msec) {}  //delayMicroseconds() ist zu ungenau
        digitalWrite(pitMaster[id].io, LOW);
        //interrupts();
        
        servointerrupt = servointerrupt*1;        // Signal abgeschlossen, frei für Rest
      } else servointerrupt = servointerrupt*0;   // Signal noch nicht abgeschlossen
    } else servointerrupt = servointerrupt*1;     // nichts zum abschließen
  
  }

  // Abschalt-Delay wird so lange geblockt, bis alle Servos frei
  // aktuell immer nur ein Servo gleichzeitig, also kein Problem
  // könnte zum komplette blocken vom Delay kommen, aber bei nur einem Timer stimmt der zweite nicht mehr

}







