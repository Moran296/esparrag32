#ifndef ESPARRAG_CONFIG_TABLE__
#define ESPARRAG_CONFIG_TABLE__
#include "etl/enum_type.h"
#include "esparrag_common.h"

struct CONFIG_ID
{
    enum enum_type
    {
        AP_SSID,
        AP_PASSWORD,
        STA_SSID,
        STA_PASSWORD,
        WIFI_STATE,
        SOME_NUM,

        NUM
    };

    ETL_DECLARE_ENUM_TYPE(CONFIG_ID, size_t)
    ETL_ENUM_DEFAULT(AP_SSID)
    ETL_ENUM_DEFAULT(AP_PASSWORD)
    ETL_ENUM_DEFAULT(STA_SSID)
    ETL_ENUM_DEFAULT(STA_PASSWORD)
    ETL_ENUM_DEFAULT(WIFI_STATE)
    ETL_ENUM_DEFAULT(SOME_NUM)
    ETL_END_ENUM_TYPE
};

struct WIFI_STATE
{
    enum enum_type
    {
        OFFLINE,
        AP,
        CONNECTING,
        STA,

        NUM
    };

    ETL_DECLARE_ENUM_TYPE(WIFI_STATE, uint8_t)
    ETL_ENUM_DEFAULT(OFFLINE)
    ETL_ENUM_DEFAULT(AP)
    ETL_ENUM_DEFAULT(CONNECTING)
    ETL_ENUM_DEFAULT(STA)
    ETL_END_ENUM_TYPE
};

#endif