cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers")
file(GLOB app_sources
        *.cpp
        src/*.cpp
        src/system/*.cpp
        src/system/mqtt/*.cpp
        src/system/wifi/WifiService.cpp
        src/system/console/Console.cpp
        src/system/config/*.cpp
        src/system/storage/Storable.cpp
        src/system/ota/*.cpp
        src/system/ota/*.c
        src/system/command/*.cpp
        src/system/smart_config/*.cpp
        src/system/mdns/*.cpp
        )

idf_component_register(
        SRCS ${app_sources}
        INCLUDE_DIRS "src"
        REQUIRES
            esp8266
            esp_event
            nvs_flash
            mqtt
            spiffs
            json
            esp_https_ota
            console
            app_update
            esp_http_server
            mdns
)

#spiffs_create_partition_image(spiffs ../data)