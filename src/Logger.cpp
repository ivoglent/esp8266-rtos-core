#include "Logger.h"

static uint64_t loggerUnixTimestamp = 0;
std::string unixTimestampToDateString(time_t timestamp) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm timeinfo;
    localtime_r(&timestamp, &timeinfo);
    char buffer[9];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);

    char ms[4];
    snprintf(ms, sizeof(ms), "%03d", (uint) tv.tv_usec % 1000);

    std::stringstream ss;
    ss << buffer;
    ss << ".";
    ss << ms;
    return ss.str();
}

int32_t getTime() {
    /*uint64_t currentTime = loggerUnixTimestamp / 1000 + esp_timer_get_time() / 1000000;
    auto timeString = unixTimestampToDateString((time_t) currentTime);
    memcpy(time, timeString.c_str(), 13);
    time[13] = '\0';*/
    return  esp_timer_get_time() / 1000;
}

__attribute__((unused)) void setLoggerUnixTimestamp(uint64_t time) {
    loggerUnixTimestamp = time;
}

