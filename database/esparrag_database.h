#ifndef ESPARRAG_DB__
#define ESPARRAG_DB__

#include "esparrag_common.h"
#include "config_entry.h"
#include "config_table.h"
#include <utility>
#include "etl/vector.h"
#include "etl/delegate.h"
#include "etl/bitset.h"
#include "esparrag_nvs.h"

static constexpr int MAX_DB_SUBSCRIBERS = 5;
using dirty_list_t = etl::bitset<CONFIG_ID::NUM>;
using config_change_cb = etl::delegate<void(const dirty_list_t &)>;

class ConfigDB
{
public:
    ConfigDB();
    ~ConfigDB() { ESPARRAG_ASSERT(false); }

    eResult Init(bool reset = false);
    void Commit();
    void Reset();

    template<size_t... CONFIGS>
    void Subscribe(config_change_cb CB);

    template <class VAL_T>
    eResult Set(CONFIG_ID id, VAL_T val);
    eResult Set(CONFIG_ID id, const char *val);

    template <class VAL_T>
    void Get(CONFIG_ID id, VAL_T &val) const;
    void Get(CONFIG_ID id, const char *&val) const;

private:
    const char *configKey(size_t id);
    void notify_subscribers();

    static configEntry *m_configs[];
    dirty_list_t m_dirty_list;
    etl::vector<std::pair<dirty_list_t, config_change_cb>, MAX_DB_SUBSCRIBERS> m_subscribers;
    NVS m_nvs;
    bool m_isInitialized = false;

    ConfigDB &operator=(const ConfigDB &) = delete;
    ConfigDB(const ConfigDB &) = delete;
};

//Template functions implementation
template<size_t... CONFIGS>
void ConfigDB::Subscribe(config_change_cb CB)
{
    ESPARRAG_ASSERT(m_subscribers.size() != m_subscribers.capacity());
    dirty_list_t bitset;
    (bitset.set(CONFIGS), ...);
    m_subscribers.push_back(std::make_pair(bitset, CB));
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

template <class VAL_T>
void ConfigDB::Get(CONFIG_ID id, VAL_T &val) const
{
    ESPARRAG_ASSERT(m_isInitialized);
    return m_configs[id]->get(val);
}

#endif