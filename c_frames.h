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
 
byte flash = 0;                       // Flash Battery Symbol in Status Row


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Frame while system start 
void drawConnect() {
    
    display.clear();
    display.setColor(WHITE);
    
    // Draw Logo
    display.drawXbm(7, 4, nano_width, nano_height, xbmnano);
    display.display();
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Frame while Loading
void drawLoading() {

  display.clear();
  display.setColor(WHITE); 
  
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  
  if (!battery.charge) {
    display.fillRect(18,3,2,4); //Draw battery end button
    display.fillRect(16,8,1,1); //Untere Ecke
    display.drawRect(0,1,16,7); //Draw Outline
    display.setColor(BLACK);
    display.fillRect(4,0,8,10); //Untere Ecke
    display.setColor(WHITE);
    display.drawXbm(4, 0, 8, 10, xbmcharge);
    display.fillRect(2,3,6,4);  // Draw Battery Status
    display.drawString(64, 30, "WIRD GELADEN...");
    
  } else {
    display.fillRect(18,3,2,4); //Draw battery end button
    display.fillRect(16,8,1,1); //Untere Ecke
    display.drawRect(0,1,16,7); //Draw Outline
    display.fillRect(2,3,MAXBATTERYBAR,4);  // Draw Battery Status
    display.drawString(64, 30, "LADEN BEENDET");
    
  }

  display.display();

}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Frame while Question
void drawQuestion(int counter) {
    
    display.clear();
    display.setColor(WHITE);
    
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);

    bool b0 = true;
    bool b1 = true;
    
    switch (question.typ) {                   // Which Question?

      case CONFIGRESET:
        display.drawString(32,3,"Reset Config?");
        break;

      case HARDWAREALARM:
        String text = "ALARM! Kanal ";
        text += String(counter+1);
        display.drawString(25,3,text);
        display.drawString(40,18,"Stoppen?");
        b1 = false;
        break;
    }

    display.setFont(ArialMT_Plain_16);
    
    if (b1) display.drawString(10,40,"NO");
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    if (b0) display.drawString(118,40,"YES");
    display.display();
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Frame while Menu
void drawMenu() {
    
    display.clear();
    display.setColor(WHITE);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);

    // Sandwich
    display.drawLine(3, 3, 13, 3);
    display.drawLine(3, 7, 13, 7);
    display.drawLine(3, 11, 13, 11);

    display.drawXbm(17,41,arrow_height,arrow_width,xbmarrow2); 
    display.drawXbm(17,27,arrow_height,arrow_width,xbmarrow1);

    display.drawString(50,2,"MENU");
    display.setFont(ArialMT_Plain_16);

    switch (menu_count) {
      
      case 0:   // Temperature
        //display.fillRect(0, 18, 128, 14);
        display.drawString(30,27,"Temperatur");
        break;

      case 1:   // Pitmaster
        //display.fillRect(0, 33, 128, 14);
        display.drawString(30,27,"Pitmaster");
        break;

      case 2:   // System
        //display.fillRect(0, 48, 128, 14);
        display.drawString(30,27,"System");
        break;
    }
    
    display.display();
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Status Row

void gBattery(OLEDDisplay *display, OLEDDisplayUiState* state) {

  int battPixel = (battery.percentage*MAXBATTERYBAR)/100;  
  flash = !flash; //Toggle flash flag for icon blinking later
  
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(Noto_Sans_8);
  
  if (pitmaster.active)
    if (autotune.initialized)
      display->drawString(33,0, "A  " + String(pitmaster.set,1) + " / " + String(pitmaster.value,0) + "%");
    else
      display->drawString(33,0, "P  " + String(pitmaster.set,1) + " / " + String(pitmaster.value,0) + "%");
  else  display->drawString(24,0,String(battery.percentage)); 
  
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  if (isAP == 1)  display->drawString(128,0,"AP");
  else if (isAP == 2) display->drawString(128,0,"NO");
  else if (isAP == 0)  {
    //display->drawString(128,0,String(rssi)+" dBm");
    display->fillRect(116,8,2,1); //Draw ground line
    display->fillRect(120,8,2,1); //Draw ground line
    display->fillRect(124,8,2,1); //Draw ground line

    if (rssi > -100) display->fillRect(116,5,2,3); //Draw 1 line
    if (rssi > -85) display->fillRect(120,3,2,5); //Draw 2 line
    if (rssi > -70) display->fillRect(124,1,2,7); //Draw 3 line
  }

  //display->drawString(80,0,String(map(pit_y,0,pit_pause,0,100)) + "%");

  if (!INACTIVESHOW) display->drawString(100,0,"F");
  
  if (flash && battery.percentage < 10) {} // nothing for flash effect
  else if (!battery.charge) {
    display->fillRect(18,3,2,4); //Draw battery end button
    display->fillRect(16,8,1,1); //Untere Ecke
    display->drawRect(0,1,16,7); //Draw Outline
    display->setColor(BLACK);
    display->fillRect(4,0,8,10); //Untere Ecke
    display->setColor(WHITE);
    display->drawXbm(4, 0, 8, 10, xbmcharge);
    display->fillRect(2,3,6,4);  // Draw Battery Status
  }
  else {
  display->fillRect(18,3,2,4); //Draw battery end button
  display->fillRect(16,8,1,1); //Untere Ecke
  display->drawRect(0,1,16,7); //Draw Outline
  display->fillRect(2,3,battPixel,4);  // Draw Battery Status
  }
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Main Frames

void drawTemp(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->drawXbm(x+19,18+y,20,36,xbmtemp);                            // Symbol
  display->fillRect(x+27,y+43-ch[current_ch].match,4,ch[current_ch].match);   // Current level
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(19+x, 20+y, String(current_ch+1));                // Channel
  display->drawString(114+x, 20+y, ch[current_ch].name);    // Channel Name
  display->setFont(ArialMT_Plain_16);
  if (ch[current_ch].isalarm && !pulsalarm) {
    if (ch[current_ch].temp!=INACTIVEVALUE) {
      display->drawString(114+x, 36+y, String(ch[current_ch].temp,1)+ " °" + temp_unit); // Channel Temp
    } else display->drawString(114+x, 36+y, "OFF");
  } else if (!ch[current_ch].isalarm) {
    if (ch[current_ch].temp!=INACTIVEVALUE) {
      display->drawString(114+x, 36+y, String(ch[current_ch].temp,1)+ " °" + temp_unit); // Channel Temp
    } else display->drawString(114+x, 36+y, "OFF");
  }

  if (pitmaster.active) {
    if (current_ch == pitmaster.channel) {
      display->setFont(ArialMT_Plain_10);
      if (autotune.initialized) display->drawString(44+x, 31+y, "A");
      else display->drawString(44+x, 31+y, "P");
      int _cur = ch[current_ch].temp*10;
      int _set = pitmaster.set*10; 
      if (_cur > _set)
        display->drawXbm(x+37,24+y,arrow_height,arrow_width,xbmarrow2); 
      else if (_cur < _set) 
        display->drawXbm(x+37,24+y,arrow_height,arrow_width,xbmarrow1);
      else display->drawXbm(x+37,24+y,arrow_width,arrow_height,xbmarrow);
    }
  }

}

void drawlimito(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  if (flashinwork)    {
    display->drawXbm(x+19,18+y,20,36,xbmtemp);         // Symbol
    display->fillRect(x+27,y+43-ch[current_ch].match,4,ch[current_ch].match);   // Current level
  }
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(19+x, 20+y, String(current_ch+1));                // Channel
  display->drawLine(33+x,25+y,50,25);
  if (inWork) display->drawString(104+x, 19+y, String(tempor,1)+ " °" + temp_unit);
  else display->drawString(104+x, 19+y, String(ch[current_ch].max,1)+ " °" + temp_unit);  // Upper Limit 
}


void drawlimitu(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  if (flashinwork)    {
    display->drawXbm(x+19,18+y,20,36,xbmtemp);                            // Symbol
    display->fillRect(x+27,y+43-ch[current_ch].match,4,ch[current_ch].match);   // Current level
  }
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(19+x, 20+y, String(current_ch+1));                // Channel
  display->drawLine(33+x,39+y,50,39); 
  if (inWork) display->drawString(104+x, 34+y, String(tempor,1)+ " °" + temp_unit);
  else display->drawString(104+x, 34+y, String(ch[current_ch].min,1)+ " °" + temp_unit);  // Lower Limit
}

void drawtyp(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  if (flashinwork)    {
    display->drawXbm(x+19,18+y,20,36,xbmtemp);                            // Symbol
    display->fillRect(x+27,y+43-ch[current_ch].match,4,ch[current_ch].match);   // Current level
  }
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(19+x, 20+y, String(current_ch+1));                // Channel
  display->drawString(114+x, 20+y, "TYP:");                       
  if (inWork) display->drawString(114+x, 36+y, ttypname[(int) tempor]);
  else display->drawString(114+x, 36+y, ttypname[ch[current_ch].typ]);            // Typ
}

void drawalarm(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  if (flashinwork)    {
    display->drawXbm(x+19,18+y,20,36,xbmtemp);                            // Symbol
    display->fillRect(x+27,y+43-ch[current_ch].match,4,ch[current_ch].match);   // Current level
  }
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(19+x, 20+y, String(current_ch+1));                // Channel
  display->drawString(114+x, 20+y, "ALARM:");           
  if (inWork && tempor) display->drawString(114+x, 36+y, "YES");
  else if (!inWork && ch[current_ch].alarm) display->drawString(114+x, 36+y, "YES");
  else display->drawString(114+x, 36+y, "NO");   // Alarm
}

void drawpit1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  if (flashinwork) display->drawXbm(x+21,20+y,pit_width,pit_height,xbmpit);           // Symbol
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(114+x, 20+y, "PITMASTER:");           
  if (inWork) display->drawString(114+x, 36+y, pid[(int) tempor].name);
  else display->drawString(114+x, 36+y, pid[pitmaster.typ].name);
}

void drawpit2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  if (flashinwork)  display->drawXbm(x+21,20+y,pit_width,pit_height,xbmpit);          // Symbol
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(114+x, 20+y, "CHANNEL:");           
  if (inWork) display->drawString(114+x, 36+y, String((int)tempor +1));
  else  display->drawString(114+x, 36+y, String(pitmaster.channel+1));
}

void drawpit3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  if (flashinwork)  display->drawXbm(x+21,20+y,pit_width,pit_height,xbmpit);          // Symbol
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(114+x, 20+y, "SET:");           
  if (inWork) display->drawString(114+x, 36+y, String(tempor,1)+ " °" + temp_unit);
  else  display->drawString(114+x, 36+y, String(pitmaster.set,1)+ " °" + temp_unit);
}

void drawpit4(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  if (flashinwork)   display->drawXbm(x+21,20+y,pit_width,pit_height,xbmpit);         // Symbol
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(114+x, 20+y, "ACTIVE:");           
  if (inWork && tempor) display->drawString(114+x, 36+y, "YES");
  else if (!inWork && pitmaster.active) display->drawString(114+x, 36+y, "YES");
  else display->drawString(114+x, 36+y, "NO");  
}

void drawsys1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {

  display->drawXbm(x+5,22+y,sys_width,sys_height,xbmsys);
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  
  if (isAP == 1) {
    display->drawString(120, 20, "AP-SSID:");
    display->drawString(120, 36, sys.apname);
  }
  else if (isAP == 0) {
    display->drawString(120, 20, "SSID:");
    display->drawString(120, 36, WiFi.SSID());
  }
  else if (isAP == 2) {
    display->drawString(120, 20, "SSID:");
    display->drawString(120, 36, "");
  }
}

void drawsys2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {

  display->drawXbm(x+5,22+y,sys_width,sys_height,xbmsys);
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(120, 20, "IP:");
  
  if (isAP == 1) display->drawString(120, 36, WiFi.softAPIP().toString());
  else if (isAP == 0) display->drawString(120, 36, WiFi.localIP().toString());
  else if (isAP == 2) display->drawString(120, 36, "");
}

void drawsys3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {

  display->drawXbm(x+5,22+y,sys_width,sys_height,xbmsys);
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(120, 20, "HOST-NAME:");
  display->drawString(120, 36, sys.host);
}

void drawsys4(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  if (flashinwork)   display->drawXbm(x+5,22+y,sys_width,sys_height,xbmsys);
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(114+x, 20+y, "UNIT:");
  if (inWork && tempor) display->drawString(114+x, 36+y, "°F");
  else if (!inWork) display->drawString(114+x, 36+y, "°" + temp_unit);
  else display->drawString(114+x, 36+y, "°C");
}

void drawsys5(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  if (flashinwork)   display->drawXbm(x+5,22+y,sys_width,sys_height,xbmsys);
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(114+x, 20+y, "HW-ALARM:");
  if (inWork && tempor) display->drawString(114+x, 36+y, "YES");
  else if (!inWork && sys.hwalarm) display->drawString(114+x, 36+y, "YES");
  else display->drawString(114+x, 36+y, "NO");
}

void drawsys6(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  
  display->drawXbm(x+5,22+y,sys_width,sys_height,xbmsys);
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  
  // Draw Version
  display->drawString(114+x, 20+y, "FIRMWARE:");
  display->drawString(114+x,36+y,FIRMWAREVERSION);
}

void drawsys7(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  if (flashinwork)   display->drawXbm(x+5,22+y,sys_width,sys_height,xbmsys);
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  
  // Draw Fastmode
  display->drawString(114+x, 20+y, "FASTMODE:");
  if (inWork && tempor) display->drawString(114+x, 36+y, "NO");
  else if (!inWork && INACTIVESHOW) display->drawString(114+x, 36+y, "NO");
  else display->drawString(114+x, 36+y, "YES");
}

void drawback(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->drawXbm(x+5,22+y,back_width,back_height,xbmback);
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(100+x, 27+y, "BACK");
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialising Frames

// this array keeps function pointers to all frames
// frames are the single views that slide from right to left
FrameCallback frames[] = { drawTemp, drawlimito, drawlimitu, drawtyp, drawalarm, drawback,
                           drawpit1, drawpit2, drawpit3, drawpit4, drawback,
                           drawsys1, drawsys2, drawsys3, drawsys4, drawsys5, 
                           drawsys6, drawsys7, drawback};  // drawFrame3
                           // drawback immer als letztes

// how many frames are there?
// int frameCount           // siehe c_init.h 

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { gBattery };
int overlaysCount = 1;


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Configuration OLEDDisplay

void set_OLED() {
  
  // The ESP is capable of rendering 60fps in 80Mhz mode
  // but that won't give you much time for anything else
  // run it in 160Mhz mode or just set it to 30 fps
  ui.setTargetFPS(30);

  // Customize the active and inactive symbol
  //ui.setActiveSymbol(activeSymbol);
  //ui.setInactiveSymbol(inactiveSymbol);

  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  //ui.setIndicatorPosition(TOP);

  // Defines where the first frame is located in the bar.
  //ui.setIndicatorDirection(LEFT_RIGHT);

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);

  // Add frames
  ui.setFrames(frames, frameCount);

  // Add overlays
  ui.setOverlays(overlays, overlaysCount);

  ui.setTimePerFrame(10000);
  ui.setTimePerTransition(300);
  ui.disableAutoTransition();
  ui.disableIndicator();

  // Initialising the UI will init the display too.
  ui.init();

  question.typ = NO;
  question.con = 0;

  display.flipScreenVertically();

  display.clear();
  display.display();

}


