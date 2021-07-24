#include "esparrag_nvs.h"
#include "lock.h"
#include "nvs_flash.h"

#define TAG "DRV_FLASH"
#include "esparrag_log.h"

bool NVS::s_isInitialized = false;

NVS::NVS(const char *nvs_namespace)
{
    if (!s_isInitialized)
        init();

    m_mutex = xSemaphoreCreateMutex();
    ESPARRAG_ASSERT(m_mutex != nullptr);

    esp_err_t err = nvs_open(nvs_namespace, NVS_READWRITE, &m_handle);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("NVS open failed. esp err = %d", err);
        ESPARRAG_ASSERT(false);
    }

    ESPARRAG_LOG_INFO("NVS %s opened", nvs_namespace);
}

eResult NVS::init()
{
    esp_err_t err = ESP_OK;
    if (s_isInitialized)
    {
        ESPARRAG_LOG_WARNING("trying to double init NVS");
        return eResult::ERROR_INVALID_STATE;
    }

    err = nvs_flash_init();
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("failed init of nvs err %d", err);
        ESPARRAG_ASSERT(false);
    }

    s_isInitialized = true;
    return eResult::SUCCESS;
}

eResult NVS::GetUint32(const char *key, uint32_t &value)
{
    Lock lock(m_mutex);

    esp_err_t err = nvs_get_u32(m_handle, key, &value);
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            return eResult::ERROR_FLASH_NOT_FOUND;
        }

        ESPARRAG_LOG_ERROR("NVS get uint32 failed. esp err = %d", err);
        return eResult::ERROR_FLASH;
    }

    return eResult::SUCCESS;
}

eResult NVS::SetUint32(const char *key, const uint32_t &value)
{
    Lock lock(m_mutex);

    esp_err_t err = nvs_set_u32(m_handle, key, value);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("NVS set uint32 failed. esp err = %d", err);
        return eResult::ERROR_FLASH;
    }

    return eResult::SUCCESS;
}

eResult NVS::GetString(const char *key, char *value, size_t maxLen)
{
    Lock lock(m_mutex);

    size_t requiredLen = 0;
    esp_err_t err = nvs_get_str(m_handle, key, nullptr, &requiredLen);
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            return eResult::ERROR_FLASH_NOT_FOUND;
        }

        ESPARRAG_LOG_ERROR("NVS get string failed. esp err = %d", err);
        return eResult::ERROR_FLASH;
    }

    if (requiredLen > maxLen)
    {
        ESPARRAG_LOG_ERROR("read string, buffer size = %d is smaller than value len = %d", maxLen, requiredLen);
        return eResult::ERROR_INVALID_PARAMETER;
    }

    err = nvs_get_str(m_handle, key, value, &requiredLen);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("NVS get string failed. esp err = %d", err);
        return eResult::ERROR_FLASH;
    }

    return eResult::SUCCESS;
}

eResult NVS::SetString(const char *key, const char *value)
{
    Lock lock(m_mutex);

    esp_err_t err = nvs_set_str(m_handle, key, value);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("NVS set string failed. esp err = %d", err);
        return eResult::ERROR_FLASH;
    }

    return eResult::SUCCESS;
}

eResult NVS::Erase()
{
    Lock lock(m_mutex);

    esp_err_t err = nvs_erase_all(m_handle);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("NVS erase failed. esp err = %d", err);
        return eResult::ERROR_FLASH;
    }
    return eResult::SUCCESS;
}

eResult NVS::Erase(const char *key)
{
    Lock lock(m_mutex);

    esp_err_t err = nvs_erase_key(m_handle, key);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("NVS erase failed. esp err = %d", err);
        return eResult::ERROR_FLASH;
    }
    return eResult::SUCCESS;
}

eResult NVS::SetBlob(const char *key, const void *value, size_t len)
{
    Lock lock(m_mutex);

    esp_err_t err = nvs_set_blob(m_handle, key, value, len);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("NVS set blob failed. esp err = %d", err);
        return eResult::ERROR_FLASH;
    }

    ESPARRAG_LOG_DEBUG("NVS set blob success");
    return eResult::SUCCESS;
}

eResult NVS::GetBlob(const char *key, void *value, size_t maxLen, size_t &actualLen)
{
    esp_err_t err = ESP_OK;
    Lock lock(m_mutex);

    err = nvs_get_blob(m_handle, key, NULL, &actualLen);
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            return eResult::ERROR_FLASH_NOT_FOUND;
        }

        ESPARRAG_LOG_ERROR("NVS get size of blob operation failed. esp err = %d", err);
        return eResult::ERROR_FLASH;
    }

    if (actualLen > maxLen)
    {
        ESPARRAG_LOG_ERROR("size of requested blob is bigger than max requested");
        return eResult::ERROR_INVALID_PARAMETER;
    }

    err = nvs_get_blob(m_handle, key, value, &actualLen);
    if (err != ESP_OK)
    {
        return eResult::ERROR_FLASH;
        ESPARRAG_LOG_ERROR("NVS get blob operation failed. esp err = %d", err);
    }

    ESPARRAG_LOG_DEBUG("NVS get blob len = %d success", actualLen);
    return eResult::SUCCESS;
}

eResult NVS::Commit()
{
    Lock lock(m_mutex);

    esp_err_t err = nvs_commit(m_handle);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("commit change to nvs failed, err %d", err);
        return eResult::ERROR_FLASH;
    }

    return eResult::SUCCESS;
}
