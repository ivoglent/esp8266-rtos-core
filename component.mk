#
# Component Makefile
#

COMPONENT_SRCDIRS := src src/system src/system/command  src/system/wifi src/system/mqtt src/system/config src/system/console src/system/storage src/system/smart_config src/system/mdns

COMPONENT_ADD_INCLUDEDIRS := src src/system src/system/command src/system/ota src/system/wifi src/system/mqtt src/system/config src/system/console src/system/storage src/system/smart_config src/system/mdns

ifndef CONFIG_SUPPORT_OTA_UPDATE
COMPONENT_SRCDIRS += src/system/ota
COMPONENT_ADD_INCLUDEDIRS += src/system/ota
endif