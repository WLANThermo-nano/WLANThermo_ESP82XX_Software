 /*************************************************** 
    Copyright (C) 2016  Steffen Ochs, Phantomias2006

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

#ifdef MPR
#define MPR121_I2CADDR_DEFAULT 0x5A

#define MPR121_TOUCHSTATUS_L    0x00      // Touch Status Registers (0x00~0x01)
#define MPR121_FILTDATA_0L      0x04      // Electrode Data Register (0x04~0x1D) 
#define MPR121_BASELINE_0       0x1E      // Baseline Value Register (0x1E~0x2A)

#define MPR121_TOUCHTH_0        0x41      // Touch and Release Threshold (0x41~0x5A)
#define MPR121_RELEASETH_0      0x42
#define MPR121_DEBOUNCE         0x5B      // Debounce Register (0x5B)

#define MPR121_CONFIG1          0x5C      // AFE Configuration Register (0x5C, 0x5D)
#define MPR121_CONFIG2          0x5D
#define MPR121_CHARGECURR_0     0x5F      // Individual Charge Current Register (0x5F~0x6B)
#define MPR121_CHARGETIME_1     0x6C      // Individual Charge Time Register (0x6C~0x72)
#define MPR121_ECR              0x5E      // Electrode Configuration Register (ECR,0x5E)
 
#define MPR121_AUTOCONFIG0      0x7B      // Auto Configuration Registers (0x7B~0x7F)
#define MPR121_AUTOCONFIG1      0x7C
#define MPR121_UPLIMIT          0x7D
#define MPR121_LOWLIMIT         0x7E
#define MPR121_TARGETLIMIT      0x7F

#define MPR121_GPIOCTL0        0x73      // GPIO MODE Part 1
#define MPR121_GPIOCTL1        0x74      // GPIO MODE Part 2
#define MPR121_GPIODAT         0x75      // GPIO DATA
#define MPR121_GPIODIR          0x76      // GPIO INPUT/OUTPUT
#define MPR121_GPIOEN           0x77      // GPIO DISABLE/ENABLE
#define MPR121_GPIOSET          0x78
#define MPR121_GPIOCLR          0x79      // GPIO CLEAR
#define MPR121_GPIOTOGGLE       0x7A

class MPR121 {

  private:

    int8_t _i2caddr;
    uint16_t _pushed;
    bool _exist;

    uint8_t readRegister8(uint8_t reg) {
      Wire.beginTransmission(_i2caddr);
      Wire.write(reg);
      Wire.endTransmission(false);
      while (Wire.requestFrom(_i2caddr, 1) != 1);
      return ( Wire.read());
    }

    uint16_t readRegister16(uint8_t reg) {
      Wire.beginTransmission(_i2caddr);
      Wire.write(reg);
      Wire.endTransmission(false);
      while (Wire.requestFrom(_i2caddr, 2) != 2);
      uint16_t v = Wire.read();
      v |=  ((uint16_t) Wire.read()) << 8;
      return v;
    }

    void writeRegister(uint8_t reg, uint8_t value) {
      Wire.beginTransmission(_i2caddr);
      Wire.write((uint8_t)reg);
      Wire.write((uint8_t)(value));
      Wire.endTransmission();
    }

    void controlBaseline() {
      // Baseline Filtering Control Register
      writeRegister(0x2B, 0x01);          // MHD Rising, default: 0x01, Maximum Half Delta
      writeRegister(0x2C, 0x01);          // NHD Amount Rising, default: 0x01, Noise Half Delta
      writeRegister(0x2D, 0x0E);          // NCL Rising, default: 0x00, Noise Count Limit
      writeRegister(0x2E, 0x00);          // FDL Rising, default: 0x00, Filter Delay Count Limit

      writeRegister(0x2F, 0x01);          // MHD Falling, default: 0x01
      writeRegister(0x30, 0x05);          // NHD Amount Falling, default: 0x01
      writeRegister(0x31, 0x01);          // NCL Falling, default: 0xFF
      writeRegister(0x32, 0x00);          // FDL Falling, default: 0x02

      writeRegister(0x33, 0x00);          // NHD Touched
      writeRegister(0x34, 0x00);          // NCL Touched
      writeRegister(0x35, 0x00);          // FDL Touched 
    }
    

  public:

    bool exist() {
      return _exist;
    }
    

    void setThresholds(uint8_t touch, uint8_t release) {
      for (uint8_t i=0; i<12; i++) {
        writeRegister(MPR121_TOUCHTH_0 + 2*i, touch);
        writeRegister(MPR121_RELEASETH_0 + 2*i, release);
      }
    }

    uint16_t  filteredData(uint8_t t) {                   // 2te filtered electrode data
      if (t > 12) return 0;
      return readRegister16(MPR121_FILTDATA_0L + t*2);
    }

    uint16_t  baselineData(uint8_t t) {
      if (t > 12) return 0;
      uint16_t bl = readRegister8(MPR121_BASELINE_0 + t);
      return (bl << 2);
    }

    void  touched(void) {
      uint16_t t = readRegister16(MPR121_TOUCHSTATUS_L);
      _pushed = t & 0x0FFF;
    }

    byte pushbutton(uint8_t pin) {
      if (pin == 1) pin = 2;
      uint16_t cur = _pushed;
      return (cur & _BV(pin)); 
    }

    void ledOn(uint8_t pin) {
      allOff();
      if (pin > 5) pin = pin-6;
      writeRegister(MPR121_GPIOSET, _BV(pin));
    }

    void ledON(uint8_t pin) {
      writeRegister(MPR121_GPIOSET, _BV(pin));
    }

    void ledOFF(uint8_t pin) {
      writeRegister(MPR121_GPIOCLR, _BV(pin));
    }

    void ledToogle(uint8_t pin) {
      writeRegister(MPR121_GPIOTOGGLE, _BV(pin));
    }

    void allOff() {
      writeRegister(MPR121_GPIOCLR, 0x3F);
    }

    void allOn() {
      for (int i;i<6;i++) {
        writeRegister(MPR121_GPIODAT, _BV(i));
        Serial.println(_BV(i), BIN);
        delay(100);
      }
      allOff();
    }

    boolean begin(uint8_t i2caddr = MPR121_I2CADDR_DEFAULT) {
      
      //Wire.begin(SDA, SCL);
      _i2caddr = i2caddr;
      _exist = false;

      Wire.beginTransmission(_i2caddr);             // CHECK I²C ADRESS
      byte error = Wire.endTransmission();
      if (error != 0) return _exist; 
      
      writeRegister(0x80, 0x63);                    // MPR121_SOFTRESET
      delay(1);
      writeRegister(MPR121_ECR, 0x0);               // STOP MODE

      uint8_t c = readRegister8(MPR121_CONFIG2);    // CHECK INITIAL VALUE
      if (c != 0x24) return _exist;

      setThresholds(30, 20);                 // Touch = 12, Release = 6  // Abweichung zur Baseline
      controlBaseline();

      writeRegister(MPR121_DEBOUNCE, 0);
      writeRegister(MPR121_CONFIG1, 0x30); // default, 48uA charge current
      writeRegister(MPR121_CONFIG2, 0x20); // 0.5uS encoding, 1ms period

      //  writeRegister(MPR121_AUTOCONFIG0, 0x8F);

      //  writeRegister(MPR121_UPLIMIT, 150);
      //  writeRegister(MPR121_TARGETLIMIT, 100); // should be ~400 (100 shifted)
      //  writeRegister(MPR121_LOWLIMIT, 50);


      writeRegister(MPR121_GPIOEN, 0xFF);       // SET GPIO (EL4 - EL11)
      writeRegister(MPR121_GPIODIR, 0xFF);      // SET GPIO OUTPUT
      writeRegister(MPR121_GPIOCTL0, 0xFF);    // SET GPIO CMOS OUTPUT
      writeRegister(MPR121_GPIOCTL1, 0xFF);
      
      // enable electrode 0 and 1
      // start with first 5 bits of baseline tracking
      writeRegister(MPR121_ECR, 0x84);  // RUN MODE
      
      _exist = true;

    return _exist;
  }
  
};

MPR121 Touch;

#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize Buttons
void set_button() {
  
  for (int i = 0; i < NUMBUTTONS; i++) pinMode(buttonPins[i],INPUTMODE);

  #ifdef MPR
  
  if (!Touch.begin(0x5A)) {
    IPRINTPLN("No MPR121!");
  } else {
    IPRINTPLN("MPR121 found!");
    Touch.allOn();
    //Touch.allOff();
  }

  #endif

}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Dedect Button Input
static inline boolean button_input() {
  // Rückgabewert false ==> Prellzeit läuft, Taster wurden nicht abgefragt
  // Rückgabewert true ==> Taster wurden abgefragt und Status gesetzt

  static unsigned long lastRunTime;
  unsigned long now = millis();
  
  
  if (now - lastRunTime < PRELLZEIT) return false; // Prellzeit läuft noch

  lastRunTime = now;
  #ifdef MPR
  if (Touch.exist() && !digitalRead(buttonPins[0])) Touch.touched();  // Stand am MPR abfragen
  #endif
  
  for (int i=0;i<NUMBUTTONS;i++)
  {
    byte curState;
    #ifdef MPR
    if (Touch.exist()) {                    // Pushbutton per MPR121
      curState = Touch.pushbutton(i);
    } else {                              // real pushbutton
      curState = digitalRead(buttonPins[i]);
      if (INPUTMODE==INPUT_PULLUP) curState=!curState; // Vertauschte Logik bei INPUT_PULLUP
    }

    #else
      curState = digitalRead(buttonPins[i]);
      if (INPUTMODE==INPUT_PULLUP) curState=!curState; // Vertauschte Logik bei INPUT_PULLUP
    #endif
    
    if (buttonResult[i]>=SHORTCLICK) buttonResult[i]=NONE; // Letztes buttonResult löschen
    if (curState!=buttonState[i]) // Flankenwechsel am Button festgestellt
    {
      //Serial.println(curState);
      //if (curState) Touch.ledON(i+6);
      //else Touch.ledOFF(i+6);
      //Touch.ledToogle(i+6);
      if (curState)   // Taster wird gedrückt, Zeit merken
      {
        if (buttonResult[i]==FIRSTUP && now-buttonDownTime[i]<DOUBLECLICKTIME)
          buttonResult[i]=DOUBLECLICK;
        else
        { 
          buttonDownTime[i]=now;
          buttonResult[i]=FIRSTDOWN;
        }
      }
      else  // Taster wird losgelassen
      {
        if (buttonResult[i]==FIRSTDOWN) buttonResult[i]=FIRSTUP;
        if (now-buttonDownTime[i]>=LONGCLICKTIME) buttonResult[i]=LONGCLICK;
      }
    }
    else // kein Flankenwechsel, Up/Down Status ist unverändert
    {
      if (buttonResult[i]==FIRSTUP && now-buttonDownTime[i]>DOUBLECLICKTIME)
        buttonResult[i]=SHORTCLICK;
    }
    buttonState[i]=curState;
  } // for
  return true;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Response Button Status
static inline void button_event() {

  static unsigned long lastMupiTime;    // Zeitpunkt letztes schnelles Zeppen
  int mupi = 0;
  bool event[3] = {0, 0, 0};
  int b_counter;
  
  // Bearbeitungsmodus aktivieren/deaktivieren
  if (buttonResult[0]==DOUBLECLICK) {
    
    if (inWork) {
      inWork = false;                     // Bearbeitung verlassen
      flashinwork = true;                 // Dauerhafte Symbolanzeige
      event[0] = 1;
      event[1] = 0;
      event[2] = 1;                       // Endwert setzen
    } else {
      switch (inMenu) {
      
        case TEMPKONTEXT:                 // Temperaturwerte bearbeiten
          inWork = true;
          break;

        case PITSUB:                      // Pitmasterwerte bearbeiten
          inWork = true;
          break;

        case SYSTEMSUB:                   // Systemeinstellungen bearbeiten
          inWork = true;
          break;

      }
      event[0] = 1;
      event[1] = 1;         // Startwert setzen
      event[2] = 0;
      
    }
  }

  // Bei LONGCLICK rechts großer Zahlensprung jedoch gebremst
  if (buttonResult[0] == FIRSTDOWN && (millis()-buttonDownTime[0]>400)) {
    
    if (inWork) {
      mupi = 10;
      if (millis()-lastMupiTime > 200) {
        event[0] = 1;
        lastMupiTime = millis();
      }
    } else {
      buttonResult[0] = NONE;     // Manuelles Zurücksetzen um Überspringen der Menus zu verhindern
      switch (inMenu) {

        case MAINMENU:                    // Menu aufrufen
          inMenu = menu_count;
          displayblocked = false;
          b_counter = framepos[menu_count];
          current_frame = subframepos[menu_count];
          ui.switchToFrame(b_counter);
          return;
      
        case TEMPSUB:                     // Main aufrufen
          displayblocked = true;
          drawMenu();
          inMenu = MAINMENU;
          return;

        case PITSUB:                      // Main aufrufen
          if (isback) {
            displayblocked = true;
            drawMenu();
            inMenu = MAINMENU;
            setconfig(ePIT,{});
            isback = 0;
          }
          return;

        case SYSTEMSUB:                   // Main aufrufen
          if (isback) {
            displayblocked = true;
            drawMenu();
            inMenu = MAINMENU;
            setconfig(eSYSTEM,{});
            isback = 0;
          }
          return;

        case TEMPKONTEXT:                 // Temperaturmenu aufrufen
          if (isback) {
            b_counter = 0;
            setconfig(eCHANNEL,{});      // Am Ende des Kontextmenu Config speichern
            inMenu = TEMPSUB;
            ui.switchToFrame(b_counter);
            isback = 0;
          }
          return;
          

      }
    }
  }


  // Bei LONGCLICK links großer negativer Zahlensprung jedoch gebremst
  if (buttonResult[1] == FIRSTDOWN && (millis()-buttonDownTime[1]>400)) {

    if (inWork) {
      mupi = -10;
      if (millis()-lastMupiTime > 200) {
        event[0] = 1;
        lastMupiTime = millis();
      }
    } else {
      
      buttonResult[1] = NONE;     // Manuelles Zurücksetzen um Überspringen der Menus zu verhindern
      switch (inMenu) {
        case TEMPSUB:                     // Temperaturkontextmenu aufgerufen
          inMenu = TEMPKONTEXT;
          current_frame = subframepos[0];
          ui.switchToFrame(framepos[0]+1);
          return;

        case MAINMENU:                    // Menu aufrufen
          b_counter = 0;
          displayblocked = false;
          inMenu = TEMPSUB;
          ui.switchToFrame(b_counter);
          return;

        case PITSUB:                      // Main aufrufen
          displayblocked = true;
          drawMenu();
          inMenu = MAINMENU;
          setconfig(ePIT,{});
          return;

        case SYSTEMSUB:                   // Main aufrufen
          displayblocked = true;
          drawMenu();
          inMenu = MAINMENU;
          setconfig(eSYSTEM,{});
          return;

        case TEMPKONTEXT:                 // Temperaturmenu aufrufen
          b_counter = 0;
          setconfig(eCHANNEL,{});      // Am Ende des Kontextmenu Config speichern
          inMenu = TEMPSUB;
          ui.switchToFrame(b_counter);
          return;

      }
    }
  }


  // Button rechts Shortclick: Vorwärts / hochzählen / Frage mit Ja beantwortet
  if (buttonResult[0] == SHORTCLICK) {

    if (question.typ > 0) {
      // Frage wurde mit YES bestätigt
      switch (question.typ) {
        case CONFIGRESET:
          set_channels(1);
          setconfig(eCHANNEL,{});
          loadconfig(eCHANNEL,0);
          break;

        case HARDWAREALARM:
          ch[question.con].showalarm = 0;
          break;

        case TUNE:
          autotune.keepup = true;
          break;

      }
      question.typ = NO;
      displayblocked = false;
      return;
    }
    
    isback = 0;
    if (inWork) {

      // Bei SHORTCLICK kleiner Zahlensprung
      mupi = 1;
      event[0] = 1;
      
    } else {

      b_counter = ui.getCurrentFrameCount();
      int i = 0;
    
      switch (inMenu) {
      
        case MAINMENU:                     // Menu durchwandern
          if (menu_count < 2) menu_count++;
          else menu_count = 0;
          drawMenu();
          break;

        case TEMPSUB:                     // Temperaturen durchwandern
          if (!sys.fastmode) {
            current_ch++;
            if (current_ch > MAXCOUNTER) current_ch = MINCOUNTER;
          }
          else {
            do {
              current_ch++;
              i++;
              if (current_ch > MAXCOUNTER) current_ch = MINCOUNTER;
            } while ((ch[current_ch].temp==INACTIVEVALUE) && (i<CHANNELS)); 
          }
          ui.setFrameAnimation(SLIDE_LEFT);
          ui.transitionToFrame(0);      // Refresh
          break;

        case TEMPKONTEXT:
          b_counter = framepos[3];
          if (current_frame < subframepos[1]-1) current_frame++;
          else current_frame = subframepos[0];
          if (current_frame == subframepos[1]-1) {
            isback = 1;
            b_counter = framepos[4];        // BACK-Page
          }
          ui.switchToFrame(b_counter);
          break;

        case PITSUB:
        case SYSTEMSUB:
          b_counter = framepos[inMenu];
          if (current_frame < subframepos[inMenu+1]-1) current_frame++;
          else  current_frame = subframepos[inMenu];
          if (current_frame == subframepos[inMenu+1]-1) {
            isback = 1;
            b_counter = framepos[4];        // BACK-Page
          }
          ui.switchToFrame(b_counter);
          break;
      }
      return;
    }
  }
  
  
  // Button links Shortklick: Rückwärts / runterzählen / Frage mit Nein beantwortet
  if (buttonResult[1] == SHORTCLICK) {

    // Frage wurde verneint -> alles bleibt beim Alten
    if (question.typ > 0) {

      switch (question.typ) {

        case HARDWAREALARM:
          return;

      }
      
      question.typ = NO;
      displayblocked = false;
      return;
    }

    isback = 0;
    if (inWork) {

      mupi = -1;
      event[0] = 1;
    
    } else {
    
      b_counter = ui.getCurrentFrameCount();
      int j = CHANNELS;
    
      switch (inMenu) {

        case MAINMENU:                     
          if (menu_count > 0) menu_count--;
          else menu_count = 2;
          drawMenu();
          break;
        
        case TEMPSUB: 
          if (!sys.fastmode) {
            current_ch--;
            if (current_ch < MINCOUNTER) current_ch = MAXCOUNTER;
          } else {
            do {
              current_ch--;
              j--;
              if (current_ch < MINCOUNTER) current_ch = MAXCOUNTER;
            } while ((ch[current_ch].temp==INACTIVEVALUE) && (j > 0)); 
          }
          ui.setFrameAnimation(SLIDE_RIGHT);
          ui.transitionToFrame(0);      // Refresh
          break;

        case TEMPKONTEXT:
          b_counter = framepos[3];
          if (current_frame > subframepos[0])
            current_frame--;
          else {
            current_frame = subframepos[1]-1;
            isback = 1;
            b_counter = framepos[4];
          }
          ui.switchToFrame(b_counter);
          break;

        case PITSUB:
        case SYSTEMSUB:
          b_counter = framepos[inMenu];
          if (current_frame > subframepos[inMenu])
            current_frame--;
          else {
            current_frame = subframepos[inMenu+1]-1;
            isback = 1;
            b_counter = framepos[4];
          }
          ui.switchToFrame(b_counter);
          break;
      }
      return;
    }
  }

#ifdef MPR
  if ((inMenu == TEMPSUB || inMenu == TEMPKONTEXT) && !displayblocked && flashinwork) Touch.ledOn(current_ch);
  else Touch.allOff();
#endif

  // EVENT ---------------------------------------------------------
  if (event[0]) {  
    switch (current_frame) {
        
      case 1:  // Upper Limit
        if (event[1]) tempor = ch[current_ch].max;
        tempor += (0.1*mupi);
        if (sys.unit == "C") {
          if (tempor > OLIMITMAX) tempor = OLIMITMIN;
          else if (tempor < OLIMITMIN) tempor = OLIMITMAX;
        } else {
          if (tempor > OLIMITMAXF) tempor = OLIMITMINF;
          else if (tempor < OLIMITMINF) tempor = OLIMITMAXF;
        }
        if (event[2]) ch[current_ch].max = tempor;
        break;
          
      case 2:  // Lower Limit
        if (event[1]) tempor = ch[current_ch].min;
        tempor += (0.1*mupi);
        if (sys.unit == "C") {
          if (tempor > ULIMITMAX) tempor = ULIMITMIN;
          else if (tempor < ULIMITMIN) tempor = ULIMITMAX;
        } else {
          if (tempor > ULIMITMAXF) tempor = ULIMITMINF;
          else if (tempor < ULIMITMINF) tempor = ULIMITMAXF;
        }
        if (event[2]) ch[current_ch].min = tempor;
        break;
          
      case 3:  // Typ
        if (mupi == 10) mupi = 1;
        if (event[1]) tempor = ch[current_ch].typ;
        tempor += mupi;
        if (tempor > (SENSORTYPEN-1)) tempor = 0;
        else if (tempor < 0) tempor = SENSORTYPEN-1;
        if (event[2]) ch[current_ch].typ = tempor;
        break;
        
      case 4:  // Alarm
        if (mupi == 10) mupi = 1;
        if (event[1]) tempor = ch[current_ch].alarm;
        tempor += mupi;
        if (tempor > (ALARM_ALL)) tempor = ALARM_OFF;
        else if (tempor < ALARM_OFF) tempor = ALARM_ALL;
        if (event[2]) ch[current_ch].alarm = tempor;
        break;
        
      case 6:  // Pitmaster Typ
        if (mupi == 10) mupi = 1;
        if (event[1]) tempor = pitMaster[0].pid; 
        tempor += mupi;
        if (tempor > pidsize-1) tempor = 0;
        else if (tempor < 0) tempor = pidsize-1;
        if (event[2]) pitMaster[0].pid = tempor;
        break;
        
      case 7:  // Pitmaster Channel
        if (mupi == 10) mupi = 1;
        if (event[1]) tempor = pitMaster[0].channel;
        tempor += mupi;
        if (tempor > CHANNELS-1) tempor = 0;
        else if (tempor < 0) tempor = CHANNELS-1;
        if (event[2]) pitMaster[0].channel = tempor;
        break;
        
      case 8:  // Pitmaster Set
        if (event[1]) tempor = pitMaster[0].set;
        tempor += (0.1*mupi);
        if (tempor > PITMASTERSETMAX) tempor = PITMASTERSETMIN;
        else if (tempor < PITMASTERSETMIN) tempor = PITMASTERSETMAX;
        if (event[2]) pitMaster[0].set = tempor;
        break;
        
      case 9:  // Pitmaster Active
        if (event[1]) tempor = pitMaster[0].active;
        if (mupi) {
          if (tempor > 0) tempor = 0;
          else tempor = AUTO;
        }
        if (event[2]) {
          pitMaster[0].active = tempor;
          setconfig(ePIT,{});
        }
        break;

      case 11: // SSID -> Clear Wifi
        //setconfig(eWIFI,{}); // clear Wifi settings
        // Hinweis notwendig
        break;
        
      case 14:  // Unit Change
        if (event[1]) {
          if (sys.unit == "F") tempor = 1;
        }
        if (mupi) tempor = !tempor;
        if (event[2]) {
          String unit;
          if (tempor) unit = "F";
          else unit = "C";
          if (unit != sys.unit) {
            sys.unit = unit;
            transform_limits();                             // Transform Limits
            setconfig(eCHANNEL,{});                      // Save Config
            get_Temperature();                              // Update Temperature
          }
        }
        break;

      case 15:  // UPDATE
        sys.getupdate = FIRMWAREVERSION;
        sys.update = 1;
        break;

      default:
        if (event[1]) inWork = false;     // kein bearbeitbares Attribut
        break;
     
    }
  }
  
}






