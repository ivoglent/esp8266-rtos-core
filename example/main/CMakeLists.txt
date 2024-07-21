file(GLOB app_sources
        *.cpp
        )
#FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/*.cpp)

idf_component_register(
        SRCS ${app_sources}
        INCLUDE_DIRS ""
        REQUIRES
            esp8266-rtos-core

)

#spiffs_create_partition_image(spiffs ../data)