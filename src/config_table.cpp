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
    INTEGRAL_CONFIG(CONFIG_ID::WIFI_STATUS, uint8_t, WIFI_STATUS::OFFLINE, WIFI_STATUS::NUM, WIFI_STATUS::OFFLINE, false),
    // ABOVE ARE LIBRARY CONFIGS, MUST NOT BE TOUCHED. USER CAN ADD OWN CONFIGS DOWN
};
