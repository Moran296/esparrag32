#ifndef APP_SETTINGS_H__
#define APP_SETTINGS_H__

#include "etl/enum_type.h"
#include "etl/string.h"
#include "esparrag_common.h"

struct eConfig
{
    enum enum_type
    {
        DEV_NAME,
        AP_PASSWORD,
        STA_SSID,
        STA_PASSWORD,

        NUM
    };

    ETL_DECLARE_ENUM_TYPE(eConfig, uint16_t)
    ETL_ENUM_DEFAULT(DEV_NAME)
    ETL_ENUM_DEFAULT(AP_PASSWORD)
    ETL_ENUM_DEFAULT(STA_SSID)
    ETL_ENUM_DEFAULT(STA_PASSWORD)
    ETL_END_ENUM_TYPE
};

struct eStatus
{
    enum enum_type
    {
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
};

#endif