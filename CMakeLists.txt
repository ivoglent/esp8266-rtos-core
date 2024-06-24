file(GLOB app_sources *.cpp system/*.cpp  system/mqtt/*.cpp
        system/wifi/WifiService.cpp
        system/console/Console.cpp
        system/config/*.cpp
        system/storage/Storable.cpp
        system/ota/*.cpp
        system/ota/*.c
        system/command/*.cpp
        components/tuya-switch/*.c
        components/tuya-switch/*.cpp
        )
#FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/*.cpp)

idf_component_register(
        SRCS ${app_sources}
        INCLUDE_DIRS ""
        REQUIRES
            esp_event
            nvs_flash
            mqtt
            spiffs
            json
            esp_https_ota
            console
            app_update
            esp_http_server
            #esp8266/driver
)

#spiffs_create_partition_image(spiffs ../data)