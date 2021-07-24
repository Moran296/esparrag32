#ifndef ESPARRAG_NVS__
#define ESPARRAG_NVS__

#include "etl/array.h"
#include "esparrag_common.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class NVS
{
public:
    NVS(const char *nvs_namespace);

    eResult GetString(const char *key, char *value, size_t maxLen);
    eResult GetUint32(const char *key, uint32_t &value);
    eResult SetString(const char *key, const char *value);
    eResult SetUint32(const char *key, const uint32_t &value);
    eResult SetBlob(const char *key, const void *value, size_t len);
    eResult GetBlob(const char *key, void *value, size_t maxLen, size_t &actualLen);
    eResult Erase(const char *key);
    eResult Erase();
    eResult Commit();

private:
    static bool s_isInitialized;
    static eResult init();
    SemaphoreHandle_t m_mutex;
    nvs_handle_t m_handle;

    NVS(const NVS &) = delete;
    NVS &operator=(const NVS &) = delete;
};

#endif
