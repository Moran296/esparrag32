#include "esparrag_wifi.h"
#include "esp_wifi.h"
#include <cstring>
#include <cstdlib>
#include "esparrag_log.h"

using namespace WifiFSM;


void Wifi::eventHandler(void *event_handler_arg,
                        esp_event_base_t event_base,
                        int32_t event_id,
                        void *event_data)
{
    ESPARRAG_LOG_INFO("WIFI event %d\n", event_id);
    Wifi *wifi = reinterpret_cast<Wifi *>(event_handler_arg);

    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
            case WIFI_EVENT_STA_START:
                wifi->Dispatch(EVENT_StaStart{});
                break;

            case WIFI_EVENT_STA_CONNECTED:
                wifi->Dispatch(EVENT_StaConnected{});
                break;

            case WIFI_EVENT_STA_DISCONNECTED:
                wifi->Dispatch(EVENT_LoseConnection{});
                break;

            case WIFI_EVENT_AP_START:
                ESPARRAG_LOG_INFO("AP started successfully\n");
                break;

            case WIFI_EVENT_AP_STOP:
                wifi->Dispatch(EVENT_APStop{});
                break;

            case WIFI_EVENT_AP_STACONNECTED:
                wifi->Dispatch(EVENT_UserConnected{});
                break;

            case WIFI_EVENT_AP_STADISCONNECTED:
                wifi->Dispatch(EVENT_LoseConnection{});
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        wifi->Dispatch(EVENT_GotIP{});
    }
}

Wifi::Wifi()
{
}

void Wifi::Init()
{
    esp_err_t err = 0;
    err = esp_netif_init();
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("wifi init failed in line %d. err %d\n", __LINE__, err);
        return;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("wifi init failed in line %d. err %d\n", __LINE__, err);
        return;
    }

    err = esp_event_handler_instance_register(ESP_EVENT_ANY_BASE,
                                              ESP_EVENT_ANY_ID,
                                              &eventHandler,
                                              this,
                                              NULL);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("wifi init failed in line %d. err %d\n", __LINE__, err);
        return;
    }

    ap_netif = esp_netif_create_default_wifi_ap();
    configASSERT(ap_netif != nullptr);
    sta_netif = esp_netif_create_default_wifi_sta();
    configASSERT(sta_netif != nullptr);

    wifi_init_config_t initConf = WIFI_INIT_CONFIG_DEFAULT();
    initConf.nvs_enable = false;
    err = esp_wifi_init(&initConf);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("wifi init failed in line %d. err %d\n", __LINE__, err);
        return;
    }

    Start();
    return;
}

bool Wifi::Connect(const SSID_T& ssid, const PASSWORD_T& password) {
    if (ssid.empty() || password.empty()) {
        return false;
    }

    if (IsInState<STATE_Connected>()) {
        ESPARRAG_LOG_ERROR("already connected. call disconnect first\n");
        return false;
    }

    m_ssid = ssid;
    m_password = password;

    ESPARRAG_LOG_INFO("*connecting....*\n");
    Dispatch(EVENT_StaConnect{});
    return true;
}    

bool Wifi::SwitchToAP() {
    if (IsInState<STATE_AP>()) {
        ESPARRAG_LOG_WARNING("already AP\n");
        return false;
    }

    Dispatch(EVENT_APStart{});
    return true;
}

bool Wifi::Disconnect() {
    if (IsInState<STATE_Offline>()) {
        ESPARRAG_LOG_WARNING("already offline\n");
        return false;
    }

    Dispatch(EVENT_Disconnect{});
    return true;
}

// ENTRY FUNCTIONS
void Wifi::on_entry(STATE_Offline&) {
    ESPARRAG_LOG_INFO("wifi Offline\n");
}

void Wifi::on_entry(STATE_AP&) {
    ESPARRAG_LOG_INFO("wifi is ap\n");
}

void Wifi::on_entry(STATE_Connecting& state) {
    ESPARRAG_LOG_INFO("wifi is trying to connect......\n");
    sta_start(); 
}

void Wifi::on_entry(STATE_Connected&) {
    ESPARRAG_LOG_INFO("wifi connected\n");
}

//OFFLINE
auto Wifi::on_event(STATE_Offline &, EVENT_APStart &) {
    ESPARRAG_LOG_INFO("state offline got AP Start event\n");
    ap_start();
    return STATE_AP{};
}

auto Wifi::on_event(STATE_Offline &, EVENT_StaConnect &event) {
    ESPARRAG_LOG_INFO("state offline got StaConnect event\n");
    return STATE_Connecting{};
}

//AP
auto Wifi::on_event(STATE_AP &, EVENT_APStart &) {
    ESPARRAG_LOG_INFO("state AP got APStart event\n");
    return STATE_AP{};
}

auto Wifi::on_event(STATE_AP &, EVENT_APStop &) {
    ESPARRAG_LOG_INFO("state AP got APStop event\n");
    return STATE_Offline{};
}

auto Wifi::on_event(STATE_AP &, EVENT_StaConnect &event) {
    ESPARRAG_LOG_INFO("state AP got StaConnect event\n");
    disconnect();
    return STATE_Connecting{};
}

auto Wifi::on_event(STATE_AP &, EVENT_UserConnected &) {
    ESPARRAG_LOG_INFO("state AP got UserConnected event\n");
    return std::nullopt;
}

//Connecting
auto Wifi::on_event(STATE_Connecting &, EVENT_LoseConnection &) {
    ESPARRAG_LOG_INFO("state Connecting got LoseConnection event\n");
    return std::nullopt;
}

auto Wifi::on_event(STATE_Connecting &, EVENT_StaStart&) {
    ESPARRAG_LOG_INFO("state Connecting got StaStart event\n");
    sta_connect();
    return std::nullopt;
}

auto Wifi::on_event(STATE_Connecting &, EVENT_StaConnected &) {
    ESPARRAG_LOG_INFO("state Connecting got connected event\n");
    return STATE_Connected{};
}

auto Wifi::on_event(STATE_Connecting &, EVENT_GotIP &) {
    ESPARRAG_LOG_INFO("state Connecting got GotIP event\n");
    return STATE_Connected{};
}

//Connected
auto Wifi::on_event(STATE_Connected &, EVENT_LoseConnection &) {
    ESPARRAG_LOG_INFO("state Connected got LoseConnection event\n");
    return STATE_Connecting{};
}

auto Wifi::on_event(STATE_Connected &, EVENT_Disconnect &) {
    ESPARRAG_LOG_INFO("state Connected got Disconnect event\n");
    disconnect();
    return STATE_Offline{};
}

auto Wifi::on_event(STATE_Connected &, EVENT_GotIP &) {
    ESPARRAG_LOG_INFO("state Connected got GotIP event\n");
    return std::nullopt;
}

// Private functions

bool Wifi::sta_start()
{
    esp_err_t err = ESP_OK;
    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("wifi sta connect failed in line %d. err %d\n", __LINE__, err);
        return false;
    }
    wifi_config_t config;
    memset(&config, 0, sizeof(config));
    strlcpy((char *)config.sta.ssid, m_ssid.data(), sizeof(config.sta.ssid));
    strlcpy((char *)config.sta.password, m_password.data(), sizeof(config.sta.password));
    config.sta.bssid_set = false;
    config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;

    err = esp_wifi_set_config(WIFI_IF_STA, &config);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("wifi sta connect failed in line %d. err %d\n", __LINE__, err);
        return false;
    }

    err = esp_wifi_start();
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("wifi sta connect failed in line %d. err %d\n", __LINE__, err);
        return false;
    }

    return true;
}

bool Wifi::sta_connect()
{
    ESPARRAG_LOG_INFO("sta connecting.....\n");
    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("wifi sta connect failed in line %d. err %d\n", __LINE__, err);
        return false;
    }

    return true;
}

void Wifi::disconnect()
{
    esp_wifi_disconnect();
    esp_wifi_stop();
}

bool Wifi::ap_start()
{
    ESPARRAG_LOG_INFO("ap ssid %s password %s\n", AP_SSID, AP_PASSWORD);

    esp_err_t err = ESP_OK;
    err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("wifi sta connect failed in line %d. err %d\n", __LINE__, err);
        return false;
    }
    wifi_config_t config;
    memset(&config, 0, sizeof(config));
    strlcpy((char *)config.ap.ssid, AP_SSID, sizeof(config.ap.ssid));
    strlcpy((char *)config.ap.password, AP_PASSWORD, sizeof(config.ap.password));
    config.ap.max_connection = 4;
    config.ap.ssid_len = strlen(AP_SSID);
    config.ap.channel = 1;
    config.ap.authmode = WIFI_AUTH_WPA2_PSK;

    err = esp_wifi_set_config(WIFI_IF_AP, &config);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("wifi ap start failed in line %d. err %d\n", __LINE__, err);
        return false;
    }

    ESPARRAG_LOG_INFO("starting ap.....\n");
    err = esp_wifi_start();
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("wifi sta connect failed in line %d. err %d\n", __LINE__, err);
        return false;
    }

    return true;
}
