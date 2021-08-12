#include "esparrag_mqtt.h"

void MqttClient::mqttEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    eResult res = eResult::SUCCESS;
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
