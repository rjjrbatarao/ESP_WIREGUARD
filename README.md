# ESP_WIREGUARD
esp8266/esp32 wireguard for arduino ide

## Enable ip forward
```
Needs IP Forwarding enable on esp32
ESP32
\tools\sdk\esp32\include\config\sdkconfig.h
#define CONFIG_LWIP_IP_FORWARD 1

on esp8266
tools lwip V2 Higher Bandwidth

## On esp8266 stack size must be adjusted to prevent bearssl incomplete response
C:\Users\lan\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\2.6.3\cores\esp8266\StackThunk.cpp
from
#define _stackSize (5900/4)
to
#define _stackSize (6200/4)
```
