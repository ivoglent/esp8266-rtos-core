# ESP8266 RTOS SDK APP
## Setup env
``` 
export IDF_PATH=~/esp/esp8266/ESP8266_RTOS_SDK && \
export PATH=$PATH:$HOME/esp/esp8266/xtensa-lx106-elf/bin:$HOME/anaconda3/bin && \
export ESPPORT=/dev/cu.usbserial-0001 && \
. ~/esp/esp8266/ESP8266_RTOS_SDK/export.sh


```
## Commands
Generate image
```
rm -rf ./data.bin && \
python spiffsgen.py --aligned-obj-ix-tables --page-size 256 0x7C000 ${PWD}/data ${PWD}/data.bin && \
python ${HOME}/esp/esp8266/ESP8266_RTOS_SDK/components/esptool_py/esptool/esptool.py --chip esp8266 --port /dev/cu.usbserial-0001 --baud 115200  write_flash --flash_mode dio --flash_freq 40m --flash_size 4MB 0x310000 ${PWD}/data.bin

```

Flash image
``` 
python ${HOME}/esp/esp8266/ESP8266_RTOS_SDK/components/esptool_py/esptool/esptool.py --chip esp8266 --port /dev/cu.usbserial-0001 --baud 115200  write_flash --flash_mode dio --flash_freq 40m --flash_size 4MB 0x310000 ${PWD}/data.bin

```

Test
```
./mkspiffs --unpack ./dump_2 data.bin
```

Setup MQTT
```
setup_mqtt mqtt://172.16.0.234 clientId homeId  test Test@123
```

Change target
``` 
make change-target target=esp01
```


Flash and monitor
``` 
ESPPORT=/dev/cu.usbserial-14330 ESPBAUD=115200 make flash monitor 
```