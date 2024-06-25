#pragma once

#include <cstdint>
#include "../../Core.h"
#include "../../Utils.h"
#define WRITE_FILE_PART_SIZE  256
extern "C" {
#include <cJSON.h>
}

class Storable {
public:
    virtual void parseFromJson(cJSON *json);

    virtual std::string toString() const;

    virtual std::string toJsonString() const;
};

template<typename T>
class Storage {
private:
    std::string _readFile(const char *filePath) {
        auto f = fopen(filePath, "r");
        if (f == nullptr) {
            esp_loge(file, "Failed to open file for reading");
            return "";
        }
        std::string content;
        char buf[64];
        while (fgets(buf, 64, f) != nullptr) {
            content.append(buf);
        }
        fclose(f);
        return content;
    }

    bool _writeFilePart(FILE *file, const char *content, size_t len) {
        size_t res = fwrite(content, 1, len, file);
        if (res == len) {
            return true;  // Successfully wrote the part
        } else {
            return false; // Write failed
        }
    }

    bool _writeFile(const char *content, const char *filePath) {
        auto file = fopen(filePath, "w+");
        if (!file) {
            esp_loge(file, "Failed to open file for writing: %s", filePath);
            return false;
        }

        const size_t chunkSize = 1024; // Set the desired chunk size
        size_t contentLength = strlen(content);

        size_t bytesWritten = 0;
        bool success = true;

        while (bytesWritten < contentLength) {
            size_t len = contentLength - bytesWritten;
            if (len > chunkSize) {
                len = chunkSize;
            }

            if (!_writeFilePart(file, content + bytesWritten, len)) {
                success = false; // Write failed
                break;
            }

            bytesWritten += len;
            vTaskDelay(10);
        }
        fclose(file);
        if (success) {
            esp_logi(file, "File written successfully: %s!", filePath);
        } else {
            esp_loge(file, "File written error: %s!", filePath);
        }


        return success;
    }


public:
    T *read(const char *filename) {
        esp_logi(file, "Reading content of: %s", filename);
        std::string data = _readFile(filename);
        if (!data.empty()) {
            auto json = cJSON_Parse(data.data());
            if (json) {
                T *obj(new T());
                if (instanceof<Storable>(obj)) {
                    obj->parseFromJson(json);
                    esp_logi(file, "Loaded object from file: %s", obj->toString().c_str());
                } else {
                    fromJson(json, *obj);
                }
                cJSON_Delete(json);
                return obj;
            }
            esp_logw(file, "Can not parse the configuration file content!");
        }

        return nullptr;
    }

    void remove(const char *filename) {
        // Delete it if it exists
        unlink(filename);
    }

    std::string readString(const char *filename) {
        return _readFile(filename);
    }

    void write(T *obj, const char *filename) {
        if (instanceof<Storable>(obj)) {
            esp_logi(file, "Writing content to file: %s", filename);
            _writeFile(obj->toJsonString().c_str(), filename);
        } else {
            esp_logi(file, "Not supported type");
        }
    }

    void write(const std::string &content, const char *filename) {
        esp_logi(file, "Writing content of to file: %s", filename);
        _writeFile(content.c_str(), filename);
    }
};
