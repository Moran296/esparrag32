#include "database.h"
#include "esp_log.h"

#define TAG "DB"
#include "esparrag_log.h"

static constexpr const char *db_nvs_namespace = "DB_namespace";
ConfigDB::ConfigDB() : m_nvs(db_nvs_namespace)
{
}

eResult ConfigDB::Init(bool reset)
{
    eResult res = eResult::SUCCESS;
    if (reset)
    {
        m_isInitialized = true;
        return eResult::SUCCESS;
    }

    for (size_t i = 0; i < CONFIG_ID::NUM; i++)
    {
        res = m_configs[i]->read_from_flash(m_nvs, configKey(i));
        if (res != eResult::SUCCESS)
        {
            ESPARRAG_LOG_ERROR("failed reading config %s from flash", CONFIG_ID(i).c_str());
        }
    }

    ESPARRAG_LOG_INFO("database initiated");
    m_isInitialized = true;
    return res;
}

template <class VAL_T>
eResult ConfigDB::Set(CONFIG_ID id, VAL_T val)
{
    ESPARRAG_ASSERT(m_isInitialized);
    eResult res = eResult::SUCCESS;
    bool isChanged = false;
    res = m_configs[id]->set(val, isChanged);
    if (res != eResult::SUCCESS)
    {
        ESPARRAG_LOG_ERROR("set config failed");
        return res;
    }

    if (isChanged)
        m_dirty_list.set(id);

    return eResult::SUCCESS;
}

eResult ConfigDB::Set(CONFIG_ID id, const char *val)
{
    ESPARRAG_ASSERT(m_isInitialized);
    eResult res = eResult::SUCCESS;
    bool isChanged = false;
    res = m_configs[id]->set(val, isChanged);
    if (res != eResult::SUCCESS)
    {
        ESPARRAG_LOG_ERROR("set config failed");
        return res;
    }

    if (isChanged)
        m_dirty_list.set(id);

    return eResult::SUCCESS;
}

template <class VAL_T>
void ConfigDB::Get(CONFIG_ID id, VAL_T &val) const
{
    ESPARRAG_ASSERT(m_isInitialized);
    return m_configs[id]->get(val);
}

void ConfigDB::Get(CONFIG_ID id, const char *&val) const
{
    ESPARRAG_ASSERT(m_isInitialized);
    return m_configs[id]->get(val);
}

void ConfigDB::Commit()
{
    ESPARRAG_ASSERT(m_isInitialized);
    eResult res = eResult::SUCCESS;
    for (size_t i = 0; i < CONFIG_ID::NUM; i++)
    {
        if (m_dirty_list[i])
        {
            res = m_configs[i]->write_to_flash(m_nvs, configKey(i));
            if (res != eResult::SUCCESS)
            {
                ESPARRAG_LOG_ERROR("failed to write config");
            }
        }
    }

    m_nvs.Commit();
    notify_observers(m_dirty_list);
    m_dirty_list.reset();
}

void ConfigDB::Reset()
{
    ESPARRAG_ASSERT(m_isInitialized);
    bool isChanged = false;

    for (size_t i = 0; i < CONFIG_ID::NUM; i++)
    {
        m_configs[i]->reset(isChanged);
        if (isChanged)
        {
            m_dirty_list.set(i);
        }
    }

    Commit();
}

const char *ConfigDB::configKey(size_t id)
{
    static char key[10] = "";
    memset(key, 0, sizeof(key));
    sprintf(key, "c_%d", id);
    return key;
}
