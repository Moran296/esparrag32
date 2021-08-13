#include "esparrag_mqtt.h"
#include "esparrag_log.h"

void MqttClient::mqttEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    MqttClient *client = reinterpret_cast<MqttClient *>(arg);
    esp_mqtt_event_handle_t event = reinterpret_cast<esp_mqtt_event_handle_t>(event_data);

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_BEFORE_CONNECT:
        ESPARRAG_LOG_INFO("MQTT_EVENT_BEFORE_CONNECT");
        client->updateCloudState(eMqttState::MQTT_CONNECTING);
        break;
    case MQTT_EVENT_CONNECTED:
        ESPARRAG_LOG_INFO("MQTT_EVENT_CONNECTED");
        client->updateCloudState(eMqttState::MQTT_CONNECTED);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESPARRAG_LOG_INFO("MQTT_EVENT_DISCONNECTED");
        client->updateCloudState(eMqttState::MQTT_OFFLINE);
        break;
    case MQTT_EVENT_SUBSCRIBED:
        client->updateCloudState(eMqttState::MQTT_SUBSCRIBED);
        ESPARRAG_LOG_INFO("MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESPARRAG_LOG_INFO("MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESPARRAG_LOG_INFO("MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESPARRAG_LOG_INFO("MQTT_EVENT_DATA");
        client->handleData(event);
        break;
    case MQTT_EVENT_ERROR:
        ESPARRAG_LOG_INFO("MQTT_EVENT_ERROR");
        break;
    default:
        ESPARRAG_ASSERT(false);
        break;
    }
}
void MqttClient::Init()
{
    auto changeCB = DB_PARAM_CALLBACK(Settings::Status)::create<MqttClient, &MqttClient::init>(*this);
    Settings::Status.Subscribe<eStatus::BROKER_IP>(changeCB);
}

void MqttClient::init(DB_PARAM_DIRTY_LIST(Settings::Status) list)
{
    constexpr int MQTT_PORT = 1883;
    esp_mqtt_client_config_t config{};
    const char *mqttIP = nullptr;
    char mqttHost[50]{};

    if (m_client != nullptr)
    {
        return;
    }

    Settings::Status.Get<eStatus::BROKER_IP>(mqttIP);
    if (!mqttIP)
    {
        return;
    }

    if (strlen(mqttIP) == 0)
    {
        return;
    }

    snprintf(mqttHost, sizeof(mqttHost), "mqtt://%s:%d", mqttIP, MQTT_PORT);
    const char *name = nullptr;
    Settings::Config.Get<eConfig::DEV_NAME>(name);

    config.port = MQTT_PORT;
    config.uri = mqttHost;
    config.host = mqttHost;
    config.client_id = name;
    config.username = name;

    m_client = esp_mqtt_client_init(&config);
    if (!m_client)
    {
        ESPARRAG_LOG_ERROR("could not create mqtt client");
        return;
    }

    esp_err_t err = esp_mqtt_client_register_event(m_client, MQTT_EVENT_ANY, mqttEventHandler, this);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("could not register event handler");
        return;
    }

    err = esp_mqtt_client_start(m_client);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("could not start mqtt");
        return;
    }

    ESPARRAG_LOG_INFO("mqtt initiated");
}

void MqttClient::updateCloudState(eMqttState state)
{
    Settings::Status.Set<eStatus::MQTT_STATE, uint8_t>(state);
    Settings::Status.Commit();
}

void MqttClient::handleData(esp_mqtt_event_t *event)
{
}

const char *MqttClient::constructFullTopic(const char *topic)
{
    return nullptr;
}
