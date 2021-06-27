#include "esparrag_wifi.h"
#include "config_table.h"
#include "string.h"
#include "esp_wifi.h"

Wifi::Wifi(ConfigDB &db) : m_db(db) {}

eResult Wifi::Init()
{
    esp_err_t err = 0;
    err = esp_netif_init();
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("wifi init failed in line %d. err %d", __LINE__, err);
        return eResult::ERROR_WIFI;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("wifi init failed in line %d. err %d", __LINE__, err);
        return eResult::ERROR_WIFI;
    }

    err = esp_event_handler_instance_register(ESP_EVENT_ANY_BASE,
                                              ESP_EVENT_ANY_ID,
                                              &eventHandler,
                                              this,
                                              NULL);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("wifi init failed in line %d. err %d", __LINE__, err);
        return eResult::ERROR_WIFI;
    }

    dirty_list_t relevant_configs;
    relevant_configs.set(CONFIG_ID::STA_SSID);
    relevant_configs.set(CONFIG_ID::AP_SSID);
    relevant_configs.set(CONFIG_ID::AP_PASSWORD);
    m_db.Subscribe(relevant_configs, config_change_cb::create<Wifi, &Wifi::dbConfigChange>(*this));

    ap_netif = esp_netif_create_default_wifi_ap();
    ESPARRAG_ASSERT(ap_netif != nullptr);
    sta_netif = esp_netif_create_default_wifi_sta();
    ESPARRAG_ASSERT(sta_netif != nullptr);

    wifi_init_config_t initConf = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&initConf);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("wifi init failed in line %d. err %d", __LINE__, err);
        return eResult::ERROR_WIFI;
    }

    return provision();
}

eResult Wifi::provision()
{
    ESPARRAG_LOG_INFO("provisioning...");

    uint8_t getter = 0;
    m_db.Get(CONFIG_ID::WIFI_STATUS, getter);
    WIFI_STATUS mode(getter);
    if (mode == WIFI_STATUS::STA && internet_connected)
        return eResult::SUCCESS;

    if (mode == WIFI_STATUS::STA)
    {
        sta_connect();
    }

    //check for sta credentials in database
    const char *sta_ssid = nullptr;
    const char *sta_password = nullptr;
    m_db.Get(CONFIG_ID::STA_SSID, sta_ssid);
    m_db.Get(CONFIG_ID::STA_PASSWORD, sta_password);
    if (ValidityCheck(sta_ssid, sta_password) == true)
    {
        if (mode == WIFI_STATUS::AP)
        {
            ESPARRAG_LOG_INFO("disconnect");
            disconnect();
        }

        ESPARRAG_LOG_INFO("credentials found. starting sta");
        sta_start(sta_ssid, sta_password);
    }

    else if (mode == WIFI_STATUS::OFFLINE)
    {
        ESPARRAG_LOG_INFO("starting ap");
        ap_start();
    }

    return eResult::SUCCESS;
}

bool Wifi::ValidityCheck(const char *ssid, const char *password) const
{
    if (!ssid || !password)
        return false;

    uint8_t ssidlen = strlen(ssid);
    uint8_t passlen = strlen(password);

    if (ssidlen == 0 || ssidlen > 19)
        return false;
    if (passlen > 0 && passlen < 8)
        return false;
    if (passlen > 19)
        return false;

    return true;
}

eResult Wifi::sta_start(const char *ssid, const char *password)
{
    esp_err_t err = ESP_OK;
    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("wifi sta connect failed in line %d. err %d", __LINE__, err);
        return eResult::ERROR_WIFI;
    }
    wifi_config_t config;
    memset(&config, 0, sizeof(config));
    strlcpy((char *)config.sta.ssid, ssid, sizeof(config.sta.ssid));
    strlcpy((char *)config.sta.password, password, sizeof(config.sta.password));
    config.sta.bssid_set = false;

    err = esp_wifi_set_config(ESP_IF_WIFI_STA, &config);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("wifi sta connect failed in line %d. err %d", __LINE__, err);
        return eResult::ERROR_WIFI;
    }
    err = esp_wifi_start();
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("wifi sta connect failed in line %d. err %d", __LINE__, err);
        return eResult::ERROR_WIFI;
    }

    ESPARRAG_LOG_INFO("sta starting");
    return eResult::SUCCESS;
}

eResult Wifi::sta_connect()
{
    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("wifi sta connect failed in line %d. err %d", __LINE__, err);
        return eResult::ERROR_WIFI;
    }

    return eResult::SUCCESS;
}

void Wifi::disconnect()
{
    esp_wifi_disconnect();
    esp_wifi_stop();
    //should we switch to offline?
    m_db.Set(CONFIG_ID::WIFI_STATUS, (uint8_t)WIFI_STATUS::OFFLINE);
}

eResult Wifi::ap_start()
{
    const char *ssid = nullptr;
    const char *password = nullptr;
    m_db.Get(CONFIG_ID::AP_SSID, ssid);
    m_db.Get(CONFIG_ID::AP_PASSWORD, password);
    ESPARRAG_LOG_INFO("ap ssid %s password %s", ssid, password);
    ESPARRAG_ASSERT(ValidityCheck(ssid, password) == true);

    esp_err_t err = ESP_OK;
    err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("wifi sta connect failed in line %d. err %d", __LINE__, err);
        return eResult::ERROR_WIFI;
    }
    wifi_config_t config;
    memset(&config, 0, sizeof(config));
    strlcpy((char *)config.ap.ssid, ssid, sizeof(config.ap.ssid));
    strlcpy((char *)config.ap.password, password, sizeof(config.ap.password));
    config.ap.max_connection = 4;
    config.ap.ssid_len = strlen(ssid);
    config.ap.channel = 1;
    config.ap.authmode = WIFI_AUTH_WPA2_PSK;

    err = esp_wifi_set_config(ESP_IF_WIFI_AP, &config);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("wifi sta connect failed in line %d. err %d", __LINE__, err);
        return eResult::ERROR_WIFI;
    }

    err = esp_wifi_start();
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("wifi sta connect failed in line %d. err %d", __LINE__, err);
        return eResult::ERROR_WIFI;
    }

    ESPARRAG_LOG_INFO("ap start");
    return eResult::SUCCESS;
}

void Wifi::dbConfigChange(const dirty_list_t &list)
{
    provision();
}

void Wifi::eventHandler(void *event_handler_arg,
                        esp_event_base_t event_base,
                        int32_t event_id,
                        void *event_data)
{
    ESPARRAG_LOG_INFO("WIFI event %d", event_id);
    Wifi *wifi = reinterpret_cast<Wifi *>(event_handler_arg);
    static int retries = 0;

    if (event_base == WIFI_EVENT)
    {
        if (event_id == WIFI_EVENT_STA_START)
        {
            ESPARRAG_LOG_INFO("STA STARTED");
            esp_netif_set_hostname(wifi->sta_netif, "whatever");
            wifi->sta_connect();
        }

        else if (event_id == WIFI_EVENT_STA_CONNECTED)
        {
            ESPARRAG_LOG_INFO("STA CONNECTED");
            wifi->internet_connected = true;
            wifi->m_db.Set(CONFIG_ID::WIFI_STATUS, (uint8_t)WIFI_STATUS::STA);
            wifi->m_db.Commit();
            retries = 0;
        }
        else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
        {
            wifi->internet_connected = false;
            ESPARRAG_LOG_INFO("STA DISCONNECTED, reason = %d", ((wifi_event_sta_disconnected_t *)event_data)->reason);
            int max_reconnections = 0;
            ESPARRAG_ASSERT(max_reconnections != 0);

            if (retries <= max_reconnections)
            {
                ESPARRAG_LOG_INFO("connection retry. %d out of %d", retries, max_reconnections);
                wifi->sta_connect();
                retries++;
            }
            else
            {
                wifi->disconnect();
                wifi->ap_start();
            }
        }
        else if (event_id == WIFI_EVENT_AP_START)
        {
            ESPARRAG_LOG_INFO("AP START");
            wifi->m_db.Set(CONFIG_ID::WIFI_STATUS, (uint8_t)WIFI_STATUS::AP);
            wifi->m_db.Commit();
        }
        else if (event_id == WIFI_EVENT_AP_STOP)
        {
            ESPARRAG_LOG_INFO("AP STOP");
        }
        else if (event_id == WIFI_EVENT_AP_STACONNECTED)
        {
            ESPARRAG_LOG_INFO("CLIENT CONNECTED TO AP");
        }
        else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
        {
            ESPARRAG_LOG_INFO("CLIENT DISCONNECTED FROM AP");
        }
    }
}