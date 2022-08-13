# ESP_WIREGUARD
esp8266/esp32 wireguard for arduino ide

## To allow internet fromn esp Enable ip forward(has issue)
```
Needs IP Forwarding enable on esp32
ESP32
\tools\sdk\esp32\include\config\sdkconfig.h
#define CONFIG_LWIP_IP_FORWARD 1

on esp8266
tools lwip V2 Higher Bandwidth
```
## Issues ESP8266
```
incomplete response when used with client getting response from ssl website limited to 1300 bytes
```
## Issues ESP32
```
sometimes ip forwarding doesnt work over the vpn
```

