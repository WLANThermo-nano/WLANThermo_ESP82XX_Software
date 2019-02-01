 /*************************************************** 
    Copyright (C) 2018  Steffen Ochs

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

    NOTE:
    - Nano V2: MISO > Supply Switch; CLK > PIT2

 ****************************************************/

#ifndef C_CONSTS_H_
#define C_CONSTS_H_

// HARDWARE
#define FIRMWAREVERSION  "v1.0.4"
#define GUIAPIVERSION    "1"
#define SERVERAPIVERSION "1"

// CHANNELS
#define MAXCHANNELS 8                     // UPDATE AUF HARDWARE 4.05
#define INACTIVEVALUE  999             // NO NTC CONNECTED
#define SENSORTYPEN    11               // NUMBER OF SENSORS
#define LIMITUNTERGRENZE -31           // MINIMUM LIMIT
#define LIMITOBERGRENZE 999            // MAXIMUM LIMIT
#define ULIMITMIN 10.0
#define ULIMITMAX 150.0
#define OLIMITMIN 35.0
#define OLIMITMAX 200.0
#define ULIMITMINF 50.0
#define ULIMITMAXF 302.0
#define OLIMITMINF 95.0
#define OLIMITMAXF 392.0
#define MAX11615_ADDRESS 0x33
#define MAX11613_ADDRESS 0x34
#define MEM_SIZE 10                  // Temperature Memory Buffer

// BATTERY
#define BATTMIN 3550                  // MINIMUM BATTERY VOLTAGE in mV
#define BATTMAX 4180                  // MAXIMUM BATTERY VOLTAGE in mV 
#define ANALOGREADBATTPIN 0           // INTERNAL ADC PIN
#define BATTDIV 5.9F
#define CHARGEDETECTION 16              // LOAD DETECTION PIN
#define CORRECTIONTIME 60000
#define BATTERYSTARTUP 10000

// OLED
#define OLED_ADRESS 0x3C              // OLED I2C ADRESS
#define MAXBATTERYBAR 13

// TIMER (bezogen auf 250 ms Timer) (x mal 250)
#define INTERVALSENSOR 4              // 1 s
#define INTERVALCOMMUNICATION 120     // 30 s
#define INTERVALBATTERYSIM 120        // 30 s
#define FLASHINWORK 2                 // 500 ms

// BUS
#define SDA 0
#define SCL 2
#define THERMOCOUPLE_CS 12          // Nur Test-Versionen, Konflikt Pitsupply

// BUTTONS
#define btn_r  4                    // Pullup vorhanden
#define btn_l  5                    // Pullup vorhanden
#define INPUTMODE INPUT_PULLUP      // INPUT oder INPUT_PULLUP
#define PRELLZEIT 5                 // Prellzeit in Millisekunden   
#define DOUBLECLICKTIME 400         // Längste Zeit für den zweiten Klick beim DOUBLECLICK
#define LONGCLICKTIME 600           // Mindestzeit für einen LONGCLICK
#define MINCOUNTER 0                // OLED FRAMES COUNTER LIMIT

// WIFI
#define APNAME      "NANO-AP"
#define APPASSWORD  "12345678"
#define HOSTNAME    "NANO-"

// FILESYSTEM
#define CHANNELJSONVERSION 4        // FS VERSION
#define EEPROM_SIZE 2816            // EEPROM SIZE
#define EEWIFIBEGIN         0
#define EEWIFI              300
#define EESYSTEMBEGIN       EEWIFIBEGIN+EEWIFI
#define EESYSTEM            380
#define EECHANNELBEGIN      EESYSTEMBEGIN+EESYSTEM
#define EECHANNEL           500
#define EETHINGBEGIN        EECHANNELBEGIN+EECHANNEL
#define EETHING             420
#define EEPITMASTERBEGIN    EETHINGBEGIN+EETHING
#define EEPITMASTER         700
#define EEPUSHBEGIN         EEPITMASTERBEGIN+EEPITMASTER
#define EEPUSH              512         

// PITMASTER
#define PITMASTER0IO1 15               // PITMASTER PIN
#define PITMASTER0IO2 14               // CLK // ab Platine V7.2
#define PITSUPPLY  12               // MISO // ab Platine V9.3
#define PITMIN 0                    // LOWER LIMIT SET
#define PITMAX 100                  // UPPER LIMIT SET
#define PITMASTERSIZE 1             // PITMASTER SETTINGS LIMIT
#define PIDSIZE 3
#define PITMASTERSETMIN 50
#define PITMASTERSETMAX 200
#define SERVOPULSMIN 550  // 25 Grad    // 785
#define SERVOPULSMAX 2250


#define PRODUCTNUMBERLENGTH 11

// API
#define APISERVER "api.wlanthermo.de"
#define CHECKAPI "/"

//#define APISERVER "nano.norma.uberspace.de"
//#define CHECKAPI "/api/index.php"


// THINGSPEAK
#ifdef THINGSPEAK
#define THINGSPEAKSERVER "api.thingspeak.com"
#define SENDTSLINK "/update.json"
#define THINGHTTPLINK "/apps/thinghttp/send_request"
#endif

#endif /* C_CONSTS_H_ */
