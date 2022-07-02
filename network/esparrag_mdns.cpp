#include "esparrag_mdns.h"
#include "mdns.h"
#include "esparrag_log.h"
#include "esp_wifi.h"

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

    if (mdns_hostname_set(DEVICE_NAME) != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("mdns hostname set failed");
        return false;
    }

    return true;
}

eResult Mdns::AdvertiseMqtt() {

    if (mdns_service_add(MQTT_SRV, MQTT_PROTO, "_tcp", 1883, NULL, 0) != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("mdns service add failed");
        return eResult::ERROR_GENERAL;
    }

    return eResult::SUCCESS;
}

void macToString(uint8_t *mac, char *str)
{
    sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

eResult Mdns::AdvertiseESPNOW() {

    uint8_t mac[6]{};
    char stringedMac[18]{};
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    macToString(mac, stringedMac);

    mdns_txt_item_t address = {"mac", stringedMac};
    if (mdns_service_add("_espnow", "_espnow", "_tcp", 4080, &address, 1) != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("mdns service add failed");
        return eResult::ERROR_GENERAL;
    }

    return eResult::SUCCESS;
}


EsparragResult<const char*> Mdns::FindBroker()
{
    mdns_result_t *results = nullptr;
    esp_err_t err = mdns_query_ptr(MQTT_SRV, MQTT_PROTO, 1500, 1, &results);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("couldn't find broker. err %d", err);
        return eResult::ERROR_CONNECTION_FAILURE;
    }
    if (results == nullptr)
    {
        ESPARRAG_LOG_ERROR("couldn't find broker. no results");
        return eResult::ERROR_NOT_FOUND;
    }

    static char address[20]{};
    snprintf(address, 20, IPSTR, IP2STR(&(results->addr->addr.u_addr.ip4)));
    ESPARRAG_LOG_INFO("broker ip %s", address);

    return address;
}