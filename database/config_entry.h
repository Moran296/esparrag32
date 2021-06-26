#ifndef ESPARRAG_CONFIG_ENTRY__
#define ESPARRAG_CONFIG_ENTRY__
#include "esparrag_common.h"
#include "esparrag_log.h"
#include "esparrag_nvs.h"
#include "etl/string.h"

class ConfigDB;

class configEntry
{
public:
    virtual void reset(bool &isChanged) = 0;
    virtual eResult write_to_flash(NVS &nvs, const char *key) = 0;
    virtual eResult read_from_flash(NVS &nvs, const char *key) = 0;

    //setters
    virtual eResult set(bool val, bool &isChanged) { ESPARRAG_ASSERT(false); }
    virtual eResult set(uint8_t val, bool &isChanged){ESPARRAG_ASSERT(false)};
    virtual eResult set(uint16_t val, bool &isChanged){ESPARRAG_ASSERT(false)};
    virtual eResult set(int16_t val, bool &isChanged){ESPARRAG_ASSERT(false)};
    virtual eResult set(uint32_t val, bool &isChanged){ESPARRAG_ASSERT(false)};
    virtual eResult set(int32_t val, bool &isChanged){ESPARRAG_ASSERT(false)};
    virtual eResult set(uint64_t val, bool &isChanged) { ESPARRAG_ASSERT(false); }
    virtual eResult set(double val, bool &isChanged) { ESPARRAG_ASSERT(false); }
    virtual eResult set(const char *val, bool &isChanged) { ESPARRAG_ASSERT(false); }
    //getters
    virtual void get(bool &val) { ESPARRAG_ASSERT(false); }
    virtual void get(uint8_t &val){ESPARRAG_ASSERT(false)};
    virtual void get(uint16_t &val){ESPARRAG_ASSERT(false)};
    virtual void get(int16_t &val){ESPARRAG_ASSERT(false)};
    virtual void get(uint32_t &val){ESPARRAG_ASSERT(false)};
    virtual void get(int32_t &val){ESPARRAG_ASSERT(false)};
    virtual void get(uint64_t &val) { ESPARRAG_ASSERT(false); }
    virtual void get(double &val) { ESPARRAG_ASSERT(false); }
    virtual void get(const char *&val) { ESPARRAG_ASSERT(false); }
};

template <typename T, T MIN, T MAX, T DEFAULT, bool PERSISTENT = true, size_t SIZE = sizeof(T)>
class IntegralEntry : public configEntry
{
public:
    eResult set(T value, bool &isChanged) override
    {
        isChanged = false;
        if (value > max_ || value < min_)
            return eResult::ERROR_INVALID_PARAMETER;

        if (value == value_)
            return eResult::SUCCESS;

        isChanged = true;
        value_ = value;
        return eResult::SUCCESS;
    }

    void get(T &value) override
    {
        value = value_;
    }

    void reset(bool &isChanged)
    {
        set(default_, isChanged);
    }

    eResult write_to_flash(NVS &nvs, const char *key)
    {
        if (persistent_)
            return nvs.SetBlob(key, &value_, size_);

        return eResult::SUCCESS;
    }

    eResult read_from_flash(NVS &nvs, const char *key) override
    {
        size_t actual = 0;
        eResult res = eResult::SUCCESS;
        if (persistent_)
        {
            res = nvs.GetBlob(key, &value_, SIZE, actual);
            if (res == eResult::ERROR_FLASH_NOT_FOUND)
                res = eResult::SUCCESS;
        }

        return res;
    }

private:
    T min_ = MIN;
    T max_ = MAX;
    T default_ = DEFAULT;
    T value_ = DEFAULT;
    size_t size_ = SIZE;
    bool persistent_ = PERSISTENT;
};

template <size_t SIZE, bool PERSISTENT = true>
class StringEntry : public configEntry
{
public:
    StringEntry(const char *defaultVal)
    {
        ESPARRAG_ASSERT(strlen(defaultVal) < SIZE);
        value_.assign(defaultVal);
        default_.assign(defaultVal);
    }

    eResult set(const char *value, bool &isChanged) override
    {
        ESPARRAG_ASSERT(value != nullptr);

        isChanged = false;
        if (strlen(value) > SIZE)
            return eResult::ERROR_INVALID_PARAMETER;

        if (value_ == value)
            return eResult::SUCCESS;

        isChanged = true;
        value_.assign(value);
        return eResult::SUCCESS;
    }

    void get(const char *&value) override
    {
        value = value_.c_str();
    }

    void reset(bool &isChanged)
    {
        set(default_.c_str(), isChanged);
    }

    eResult write_to_flash(NVS &nvs, const char *key) override
    {
        if (persistent_)
        {
            ESPARRAG_LOG_INFO("writng...");
            return nvs.SetString(key, value_.c_str());
        }

        return eResult::SUCCESS;
    }

    eResult read_from_flash(NVS &nvs, const char *key) override
    {
        eResult res = eResult::SUCCESS;
        if (persistent_)
        {
            res = nvs.GetString(key, (char *)value_.c_str(), SIZE);
            if (res == eResult::ERROR_FLASH_NOT_FOUND)
                res = eResult::SUCCESS;
        }

        return eResult::SUCCESS;
    }

private:
    etl::string<SIZE> default_;
    etl::string<SIZE> value_;
    size_t size_ = SIZE;
    bool persistent_ = PERSISTENT;
};

#endif
