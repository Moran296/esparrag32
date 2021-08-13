#include "esparrag_mdns.h"
#include "mdns.h"
#include "esparrag_log.h"
#include "esparrag_manager.h"

#define MQTT_SRV "_mqtt"
#define MQTT_PROTO "_tcp"

void Mdns::Init()
{
    Settings::Status.Subscribe<eStatus::WIFI_STATE>([](auto dirty_list)
                                                    { init(); });
}

void Mdns::init()
{
    uint8_t state;
    const char *brokerIp = nullptr;
    Settings::Status.Get<eStatus::BROKER_IP>(brokerIp);
    Settings::Status.Get<eStatus::WIFI_STATE>(state);

    if (strlen(brokerIp) != 0)
        return;

    if (state != eWifiState::WIFI_STA)
        return;

    esp_err_t err = mdns_init();
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("mdns server init failed. err %d", err);
        return;
    }

    FindBroker();
}

void Mdns::FindBroker()
{
    mdns_result_t *results = nullptr;
    esp_err_t err = mdns_query_ptr(MQTT_SRV, MQTT_PROTO, 1000, 1, &results);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("couldn't find broker. err %d", err);
        return;
    }
    if (results == nullptr)
    {
        ESPARRAG_LOG_ERROR("couldn't find broker. no results");
        return;
    }

    char address[20]{};
    snprintf(address, 20, IPSTR, IP2STR(&(results->addr->addr.u_addr.ip4)));
    ESPARRAG_LOG_ERROR("broker ip %s", address);
    Settings::Status.Set<eStatus::BROKER_IP>(address);
    Settings::Status.Commit();
}