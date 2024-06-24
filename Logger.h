#pragma once

#ifndef APP_LOG_LEVEL
#define APP_LOG_LEVEL 5
#endif

#define CONFIG_LOG_COLORS 1

#ifndef LOG_LOCAL_LEVEL
#define LOG_LOCAL_LEVEL 5
#endif

#include <ctime>
#include <esp_log.h>
#include "esp_timer.h"
#include <string>
#include <sstream>
#include <cstring>


#define log_format(letter, format)  LOG_COLOR_ ## letter "[" #letter "][%d]\033[0;34m[%6s]" LOG_RESET_COLOR ": " format LOG_RESET_COLOR
std::string unixTimestampToDateString(time_t timestamp);
int32_t getTime();

#define esp_log_level(level, tag, format, ...) do { \
        uint32_t time = getTime();                             \
        if (level==ESP_LOG_ERROR )          { esp_log_write(ESP_LOG_ERROR,      tag, log_format(E, format), (time), tag, ##__VA_ARGS__); } \
        else if (level==ESP_LOG_WARN )      { esp_log_write(ESP_LOG_WARN,       tag, log_format(W, format), (time), tag, ##__VA_ARGS__); } \
        else if (level==ESP_LOG_DEBUG )     { esp_log_write(ESP_LOG_DEBUG,      tag, log_format(D, format), (time), tag, ##__VA_ARGS__); } \
        else if (level==ESP_LOG_VERBOSE )   { esp_log_write(ESP_LOG_VERBOSE,    tag, log_format(V, format), (time), tag, ##__VA_ARGS__); } \
        else                                { esp_log_write(ESP_LOG_INFO,       tag, log_format(I, format), (time), tag, ##__VA_ARGS__); } \
    } while(0)

#define esp_log_level_local(level, tag, format, ...) do {               \
        if ( LOG_LOCAL_LEVEL >= level ) esp_log_level(level, tag, format, ##__VA_ARGS__); \
    } while(0)

#if APP_LOG_LEVEL >= 1
#define esp_loge(tag, format, ...) esp_log_level_local(ESP_LOG_ERROR,   #tag, format, ##__VA_ARGS__)
#else
#define esp_loge(tag, format, ...)
#endif

#if APP_LOG_LEVEL >= 2
#define esp_logw(tag, format, ...) esp_log_level_local(ESP_LOG_WARN,    #tag, format, ##__VA_ARGS__)
#else
#define esp_logw(tag, format, ...)
#endif


#if APP_LOG_LEVEL >= 3
#define esp_logi(tag, format, ...) esp_log_level_local(ESP_LOG_INFO,    #tag, format, ##__VA_ARGS__)
#else
#define esp_logi(tag, format, ...)
#endif

#if APP_LOG_LEVEL >= 4
#define esp_logd(tag, format, ...) esp_log_level_local(ESP_LOG_DEBUG,   #tag, format, ##__VA_ARGS__)
#else
#define esp_logd(tag, format, ...)
#endif

#if APP_LOG_LEVEL >= 5
#define esp_logv(tag, format, ...) esp_log_level_local(ESP_LOG_VERBOSE, #tag, format, ##__VA_ARGS__)
#else
#define esp_logv(tag, format, ...)
#endif
