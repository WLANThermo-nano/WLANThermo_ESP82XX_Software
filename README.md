# Nemesis

ESP8582 based WLAN Thermometer Nano

Functional Description: https://github.com/WLANThermo-nano/WLANThermo_nano_Software/wiki

This works with the ESP8266 Arduino platform with a recent stable release(2.0.0 or newer) 
https://github.com/esp8266/Arduino and the Arduino Time Library https://github.com/PaulStoffregen/Time.

In addition, the SSD1306 library must be embedded in the Arduino IDE libaries folder. A modification of squix78's library https://github.com/squix78/esp8266-oled-ssd1306 is used.

In addition, the ESPAsyncWebServer library and the ESPAsyncTCP library must be embedded in the Arduino IDE libaries folder. The original versions of me-no-dev's library https://github.com/me-no-dev/ESPAsyncWebServer and https://github.com/me-no-dev/ESPAsyncTCP are used.

##Troubleshooting

If the compiler exits with an error similar to "WebHandlers.cpp:67:64: error: 'strftime' was not declared in this scope", follow these steps:

* Locate  folder `/Library/Arduino15/packages/esp8266/tools/xtensa-lx106-elf-gcc/1.20.0-26-gb404fb9-2/xtensa-lx106-elf/include` 
* make a copy of `time.h` and name it  `_time.h`
* Locate and open `libaries\ESPAsyncWebServer\src\WebHandlerImpl.h`
* Replace `#include <time.h>` to `#include <_time.h>`

see https://github.com/me-no-dev/ESPAsyncWebServer/issues/60
