#ifndef APP_DATA_H__
#define APP_DATA_H__

#include "etl/enum_type.h"
#include "etl/string.h"
#include "esparrag_common.h"
#include "esparrag_data.h"
#include "esparrag_database.h"

struct eConfig
{
    enum enum_type
    {
        AP_SSID,
        AP_PASSWORD,
        STA_SSID,
        STA_PASSWORD,

        NUM
    };

    ETL_DECLARE_ENUM_TYPE(eConfig, uint16_t)
    ETL_ENUM_DEFAULT(AP_SSID)
    ETL_ENUM_DEFAULT(AP_PASSWORD)
    ETL_ENUM_DEFAULT(STA_SSID)
    ETL_ENUM_DEFAULT(STA_PASSWORD)
    ETL_END_ENUM_TYPE
};

struct eStatus
{
    enum enum_type
    {
        MY_NAME,
        WIFI_STATE,
        MQTT_STATE,
        STA_IP,
        BROKER_IP,

        NUM
    };

    ETL_DECLARE_ENUM_TYPE(eStatus, uint16_t)
    ETL_ENUM_DEFAULT(WIFI_STATE)
    ETL_ENUM_DEFAULT(BROKER_IP)
    ETL_END_ENUM_TYPE
};

enum eWifiState
{
    WIFI_OFFLINE,
    WIFI_AP,
    WIFI_CONNECTING,
    WIFI_STA,

    WIFI_NUM
};

enum eMqttState
{
    MQTT_OFFLINE,
    MQTT_CONNECTING,
    MQTT_CONNECTED,
    MQTT_SUBSCRIBED,

    MQTT_NUM
};

class Settings
{
public:
    inline static Database Config{"config_db",
                                  Data<eConfig::AP_SSID, const char *>{"suannai_esp32"},
                                  Data<eConfig::AP_PASSWORD, const char *>{"11112222"},
                                  Data<eConfig::STA_SSID, const char *>{""},
                                  Data<eConfig::STA_PASSWORD, const char *>{""}};

    inline static Database Status{"status_db",
                                  Data<eStatus::MY_NAME, const char *>{"", false},
                                  Data<eStatus::WIFI_STATE, uint8_t>{WIFI_OFFLINE, WIFI_NUM - 1, WIFI_OFFLINE, false},
                                  Data<eStatus::MQTT_STATE, uint8_t>{MQTT_OFFLINE, MQTT_NUM - 1, MQTT_OFFLINE, false},
                                  Data<eStatus::STA_IP, const char *>{"", false},
                                  Data<eStatus::BROKER_IP, const char *>{"", false}};
};

#endif