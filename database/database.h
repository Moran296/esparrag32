#ifndef ESPARRAG_DB__
#define ESPARRAG_DB__

#include "esparrag_common.h"
#include "config_entry.h"
#include "config_table.h"
#include <utility>
#include "etl/vector.h"
#include "etl/observer.h"
#include "etl/bitset.h"
#include "esparrag_nvs.h"

static constexpr int MAX_DB_OBSERVERS = 5;
using dirty_list_t = etl::bitset<CONFIG_ID::NUM>;
using DB_OBSERVER = etl::observer<dirty_list_t>;

class ConfigDB : etl::observable<DB_OBSERVER, MAX_DB_OBSERVERS>
{
public:
    ConfigDB();
    ~ConfigDB() { ESPARRAG_ASSERT(false); }

    eResult Init(bool reset = false);

    template <class VAL_T>
    eResult Set(CONFIG_ID id, VAL_T val);
    eResult Set(CONFIG_ID id, const char *val);

    template <class VAL_T>
    void Get(CONFIG_ID id, VAL_T &val) const;
    void Get(CONFIG_ID id, const char *&val) const;

    void Commit();
    void Reset();

private:
    const char *configKey(size_t id);

    static configEntry *m_configs[];
    dirty_list_t m_dirty_list;
    NVS m_nvs;
    bool m_isInitialized = false;

    ConfigDB &operator=(const ConfigDB &) = delete;
    ConfigDB(const ConfigDB &) = delete;
};

#endif