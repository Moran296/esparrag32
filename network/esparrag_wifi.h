#ifndef ESPARRAGOS_WIFI__
#define ESPARRAGOS_WIFI__

#include "esparrag_common.h"
#include "esparrag_database.h"
#include "esp_netif.h"
#include "esp_event.h"

class Wifi
{
public:
    Wifi(ConfigDB &db);
    eResult Init();

private:
    eResult provision();
    eResult ap_start();
    eResult sta_start(const char *ssid, const char *password);
    eResult sta_connect();
    bool ValidityCheck(const char *ssid, const char *password) const;
    void disconnect();

    esp_netif_t *ap_netif;
    esp_netif_t *sta_netif;
    ConfigDB &m_db;
    bool internet_connected = false;
    bool failed_connection = false;

    static void eventHandler(void *event_handler_arg,
                             esp_event_base_t event_base,
                             int32_t event_id,
                             void *event_data);

    void dbConfigChange(const dirty_list_t &list);
};

#endif
