#ifndef APP_DATA_H__
#define APP_DATA_H__

#include "etl/enum_type.h"
#include "esparrag_common.h"
#include "esparrag_data.h"
#include "esparrag_database.h"

struct CONFIG_ID
{
    enum enum_type
    {
        AP_SSID,
        AP_PASSWORD,
        STA_SSID,
        STA_PASSWORD,

        NUM
    };

    ETL_DECLARE_ENUM_TYPE(CONFIG_ID, uint16_t)
    ETL_ENUM_DEFAULT(AP_SSID)
    ETL_ENUM_DEFAULT(AP_PASSWORD)
    ETL_ENUM_DEFAULT(STA_SSID)
    ETL_ENUM_DEFAULT(STA_PASSWORD)
    ETL_END_ENUM_TYPE
};

struct STATUS_ID
{
    enum enum_type
    {
        WIFI_STATE,
        SOME_NUM,

        NUM
    };

    ETL_DECLARE_ENUM_TYPE(STATUS_ID, uint16_t)
    ETL_ENUM_DEFAULT(WIFI_STATE)
    ETL_ENUM_DEFAULT(SOME_NUM)
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

class AppData
{
public:
    inline static Database Config{"config_db",
                                  Data<CONFIG_ID::AP_SSID, const char *>{"suannai_esp32"},
                                  Data<CONFIG_ID::AP_PASSWORD, const char *>{"11112222"},
                                  Data<CONFIG_ID::STA_SSID, const char *>{""},
                                  Data<CONFIG_ID::STA_PASSWORD, const char *>{""}};

    inline static Database Status{"status_db",
                                  Data<STATUS_ID::WIFI_STATE, uint8_t>{WIFI_OFFLINE, WIFI_NUM - 1, WIFI_OFFLINE, false},
                                  Data<STATUS_ID::SOME_NUM, uint8_t>{0, 250, 50, false}};
};

#endif