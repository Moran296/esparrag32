#include "esparrag_mdns.h"
#include "mdns.h"
#include "esparrag_log.h"
#include "esparrag_manager.h"

#define MQTT_SRV "_mqtt"
#define MQTT_PROTO "_tcp"

bool Mdns::Init()
{
    esp_err_t err = mdns_init();
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("mdns server init failed. err %d", err);
        return false;
    }

    return true;
}

const char* Mdns::FindBroker()
{
    mdns_result_t *results = nullptr;
    esp_err_t err = mdns_query_ptr(MQTT_SRV, MQTT_PROTO, 1500, 1, &results);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("couldn't find broker. err %d", err);
        return nullptr;
    }
    if (results == nullptr)
    {
        ESPARRAG_LOG_ERROR("couldn't find broker. no results");
        return nullptr;
    }

    static char address[20]{};
    snprintf(address, 20, IPSTR, IP2STR(&(results->addr->addr.u_addr.ip4)));
    ESPARRAG_LOG_ERROR("broker ip %s", address);
    Settings::Status.SetAndCommit<eStatus::BROKER_IP>(address);
    return address;
}