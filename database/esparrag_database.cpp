#include "esparrag_database.h"
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
    notify_subscribers();
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

void ConfigDB::notify_subscribers()
{
    for (auto &[list, cb] : m_subscribers)
    {
        dirty_list_t intersected_bits = m_dirty_list & list;
        if (intersected_bits.any())
        {
            cb(m_dirty_list);
        }
    }

    m_dirty_list.reset();
}

const char *ConfigDB::configKey(size_t id)
{
    static char key[10] = "";
    memset(key, 0, sizeof(key));
    sprintf(key, "c_%d", id);
    return key;
}
