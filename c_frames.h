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
    0.2.00 - 2016-12-30 implement ChannelData
    0.2.01 - 2017-01-04 add inactive/active channels and temperature unit
    
 ****************************************************/
 
#define MAXBATTERYBAR 13

byte flash = 0;                       // Flash Battery Symbol in Status Row


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Frame while system start 
void drawConnect(int count, int active) {
    
    display.clear();
    display.setColor(WHITE);
    
    // Draw Logo
    display.drawXbm(4, 20, 120, 39, xbmwlanthermo);

    // Draw Version
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.setFont(Noto_Sans_8);
    display.drawString(124,3,FIRMWAREVERSION);
    
    // Draw status
    for (int i = 0; i < count; i++) {
      const char *xbm;
      if (active == i) {
        xbm = active_bits;
      } else {
        xbm = inactive_bits;
      }
      display.drawXbm(64 - (12 * count / 2) + 12 * i, 5, 8, 8, xbm);
    }
    
    display.display();
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Frame while Loading
void drawLoading() {
  
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    
    display.drawString(10, 49, "LADE-LED BEACHTEN");

    // Lade-Batterie
    display.fillRect(93,21,4,16);   // Draw battery end button
    
    display.fillRect(32,18,17,2);   // Rahmen oben links
    display.fillRect(74,18,18,2);   // Rahmen oben rechts
    
    display.fillRect(32,38,26,2);   // Rahmen unten links
    display.fillRect(66,38,26,2);   // Rahmen unten rechts

    display.fillRect(32,18,2,21);   // Rahmen links
    display.fillRect(90,18,2,21);   // Rahmen rechts
    
    display.fillRect(51,17,21,13);  // Stecker Hauptteil
    display.fillRect(60,30,4,13);   // Stecker Kabel
    display.fillRect(54,7,3,9);     // Stecker Pin
    display.fillRect(66,7,3,9);     // Stecker Pin

    display.display();
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Frame while Question
void drawQuestion() {
    
    display.clear();
    display.setColor(WHITE);
    
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);

    bool b0 = true;
    bool b1 = true;
    
    switch (question) {                   // Which Question?

      case CONFIGRESET:
        display.drawString(32,3,"Reset Config?");
        break;

      case CHANGEUNIT:
        display.drawString(35,3,"Change Unit?");
        break;

      case HARDWAREALARM:
        display.drawString(35,3,"ALARM! Stop?");
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
// Status Row

void gBattery(OLEDDisplay *display, OLEDDisplayUiState* state) {

  int battPixel = (BatteryPercentage*MAXBATTERYBAR)/100;  
  flash = !flash; //Toggle flash flag for icon blinking later
  
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(Noto_Sans_8);
  display->drawString(24,0,String(BatteryPercentage));
  
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  if (isAP)  display->drawString(128,0,"AP");
  else  {
    //display->drawString(128,0,String(rssi)+" dBm");
    display->fillRect(116,8,2,1); //Draw ground line
    display->fillRect(120,8,2,1); //Draw ground line
    display->fillRect(124,8,2,1); //Draw ground line

    if (rssi > -100) display->fillRect(116,5,2,3); //Draw 1 line
    if (rssi > -85) display->fillRect(120,3,2,5); //Draw 2 line
    if (rssi > -70) display->fillRect(124,1,2,7); //Draw 3 line
  }

  //display->drawString(80,0,String(map(pit_y,0,pit_pause,0,100)) + "%");

  if (!INACTIVESHOW) display->drawString(85,0,"F");
  
  if (flash && BatteryPercentage < 10) {} // nothing for flash effect
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
  display->drawXbm(x+20,18+y,20,36,xbmtemp);                            // Symbol
  display->fillRect(x+28,y+43-ch[current_ch].match,4,ch[current_ch].match);   // Current level
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(20+x, 20+y, String(current_ch+1));                // Channel
  display->drawString(114+x, 20+y, ch[current_ch].name);    // Channel Name
  display->setFont(ArialMT_Plain_16);
  if (ch[current_ch].temp!=INACTIVEVALUE) {
    display->drawString(114+x, 36+y, String(ch[current_ch].temp,1)+ " °" + temp_unit); // Channel Temp
  } else display->drawString(114+x, 36+y, "OFF");

}

void drawlimito(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->drawXbm(x+20,18+y,20,36,xbmtemp);                            // Symbol
  display->fillRect(x+28,y+43-ch[current_ch].match,4,ch[current_ch].match);   // Current level
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(20+x, 20+y, String(current_ch+1));                // Channel
  display->drawString(104+x, 19+y, String(ch[current_ch].max,1)+ " °" + temp_unit);  // Upper Limit
  display->drawLine(33+x,25+y,50,25);
}


void drawlimitu(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->drawXbm(x+20,18+y,20,36,xbmtemp);                            // Symbol
  display->fillRect(x+28,y+43-ch[current_ch].match,4,ch[current_ch].match);   // Current level
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(20+x, 20+y, String(current_ch+1));                // Channel
  display->drawString(104+x, 34+y, String(ch[current_ch].min,1)+ " °" + temp_unit);  // Lower Limit
  display->drawLine(33+x,39+y,50,39);
}

void drawtyp(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->drawXbm(x+20,18+y,20,36,xbmtemp);                            // Symbol
  display->fillRect(x+28,y+43-ch[current_ch].match,4,ch[current_ch].match);   // Current level
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(20+x, 20+y, String(current_ch+1));                // Channel
  display->drawString(114+x, 20+y, "TYP:");                         
  display->drawString(114+x, 36+y, ttypname[ch[current_ch].typ]);            // Typ
}

void drawalarm(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->drawXbm(x+20,18+y,20,36,xbmtemp);                            // Symbol
  display->fillRect(x+28,y+43-ch[current_ch].match,4,ch[current_ch].match);   // Current level
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(20+x, 20+y, String(current_ch+1));                // Channel
  display->drawString(114+x, 20+y, "ALARM:");           
  if (ch[current_ch].alarm) display->drawString(114+x, 36+y, "JA");
  else display->drawString(114+x, 36+y, "NEIN");                        // Alarm
}

void drawwifi(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  //display.drawXbm(x + 7, y + 7, 50, 50, getIconFromString(weather.getIconTomorrow()));
  display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display->setFont(ArialMT_Plain_10);
  //display->drawString(90 + x, 20 + y, "");
  if (isAP) {
    display->drawString(DISPLAY_WIDTH/2 +x, DISPLAY_HEIGHT/3 +y, "IP Adresse: \n" + WiFi.softAPIP().toString()+ "\n SSID:" + APNAME);
  }
  else {
     display->drawString(DISPLAY_WIDTH/2 +x, DISPLAY_HEIGHT/2 +y, "IP Adresse: \n" + WiFi.localIP().toString());
  }
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialising Frames

// this array keeps function pointers to all frames
// frames are the single views that slide from right to left
FrameCallback frames[] = { drawTemp, drawlimito, drawlimitu, drawtyp, drawalarm};  // drawFrame3

// how many frames are there?
int frameCount = 5;   // 3

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

  ui.disableAutoTransition();
  ui.disableIndicator();

  // Initialising the UI will init the display too.
  ui.init();

  display.flipScreenVertically();

  display.clear();
  display.display();

}


