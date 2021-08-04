#include "esparrag_wifi.h"
#include "string.h"
#include "esp_wifi.h"

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

    auto changeCB = DB_PARAM_CALLBACK(Settings::Config)::create<Wifi, &Wifi::dbConfigChange>(*this);
    Settings::Config.Subscribe<CONFIG_ID::STA_SSID,
                               CONFIG_ID::AP_SSID,
                               CONFIG_ID::AP_PASSWORD>(changeCB);

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

    uint8_t mode = 0;
    Settings::Status.Get<STATUS_ID::WIFI_STATE>(mode);

    if (mode == WIFI_STA && internet_connected)
        return eResult::SUCCESS;

    if (mode == WIFI_STA)
    {
        sta_connect();
    }

    //check for sta credentials in database
    const char *sta_ssid = nullptr;
    const char *sta_password = nullptr;
    Settings::Config.Get<CONFIG_ID::STA_SSID, CONFIG_ID::STA_PASSWORD>(sta_ssid, sta_password);
    if (strlen(sta_ssid))
        ESPARRAG_LOG_WARNING("ssid %s", sta_ssid);
    if (strlen(sta_password))
        ESPARRAG_LOG_WARNING("pass %s", sta_password);
    if (ValidityCheck(sta_ssid, sta_password) == true)
    {
        if (mode == WIFI_AP)
        {
            ESPARRAG_LOG_INFO("disconnect");
            disconnect();
        }

        ESPARRAG_LOG_INFO("credentials found. starting sta");
        sta_start(sta_ssid, sta_password);
    }

    else if (mode == WIFI_OFFLINE)
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

    err = esp_wifi_set_config(WIFI_IF_STA, &config);
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
    eWifiState state = WIFI_OFFLINE;
    Settings::Status.Set<STATUS_ID::WIFI_STATE>((uint8_t)state);
    Settings::Status.Commit();
}

eResult Wifi::ap_start()
{
    const char *ssid = nullptr;
    const char *password = nullptr;
    Settings::Config.Get<CONFIG_ID::AP_SSID, CONFIG_ID::AP_PASSWORD>(ssid, password);
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

    err = esp_wifi_set_config(WIFI_IF_AP, &config);
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

    ESPARRAG_LOG_INFO("starting ap.....");
    return eResult::SUCCESS;
}

void Wifi::dbConfigChange(DB_PARAM_DIRTY_LIST(Settings::Config) list)
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

            eWifiState state = WIFI_STA;
            Settings::Status.Set<STATUS_ID::WIFI_STATE>((uint8_t)state);
            Settings::Status.Commit();

            retries = 0;
        }
        else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
        {
            wifi->internet_connected = false;
            ESPARRAG_LOG_INFO("STA DISCONNECTED, reason = %d", ((wifi_event_sta_disconnected_t *)event_data)->reason);
            int max_reconnections = 5;
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
            ESPARRAG_LOG_INFO("AP STARTED");
            eWifiState state = WIFI_AP;
            Settings::Status.Set<STATUS_ID::WIFI_STATE>((uint8_t)state);
            Settings::Status.Commit();
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
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ESPARRAG_LOG_INFO("GOT IP");
        ip_event_got_ip_t *ipevent = (ip_event_got_ip_t *)event_data;
        char newip[20]{};
        sprintf(newip, IPSTR, IP2STR(&ipevent->ip_info.ip));
        Settings::Status.Set<STATUS_ID::STA_IP>(newip);
        Settings::Status.Commit();
    }
}