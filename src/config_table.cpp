#include "config_entry.h"
#include "esparrag_database.h"
#include "config_table.h"
#include "etl/array.h"

#define INTEGRAL_CONFIG(ID, TYPE, MIN, MAX, DEFAULT, PERSISTENT) \
    [ID] = new IntegralEntry<TYPE, MIN, MAX, DEFAULT, PERSISTENT>
#define STRING_CONFIG(ID, SIZE, DEFAULT, PERSISTENT) \
    [ID] = new StringEntry<SIZE, PERSISTENT>(DEFAULT)

configEntry *ConfigDB::m_configs[] = {
    STRING_CONFIG(CONFIG_ID::AP_SSID, 20, "suannai_esp32", true),
    STRING_CONFIG(CONFIG_ID::AP_PASSWORD, 20, "11112222", true),
    STRING_CONFIG(CONFIG_ID::STA_SSID, 20, "", true),
    STRING_CONFIG(CONFIG_ID::STA_PASSWORD, 20, "", true),
    INTEGRAL_CONFIG(CONFIG_ID::WIFI_STATE, uint8_t, WIFI_STATE::OFFLINE, WIFI_STATE::NUM, WIFI_STATE::OFFLINE, false),
    INTEGRAL_CONFIG(CONFIG_ID::SOME_NUM, uint8_t, 0, 100, 50, false),
    // ABOVE ARE LIBRARY CONFIGS, MUST NOT BE TOUCHED. USER CAN ADD OWN CONFIGS DOWN
};
