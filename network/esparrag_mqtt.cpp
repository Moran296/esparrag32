#include "esparrag_mqtt.h"
#include "esparrag_log.h"
#include "esparrag_mdns.h"
#include "lock.h"

using namespace MqttFSM;

char MqttClient::m_payload[PAYLOAD_BUFFER_SIZE]{};
char MqttClient::m_topic[TOPIC_BUFFER_SIZE]{};
char EVENT_CONNECT::m_brokerIp[EVENT_CONNECT::MQTT_BROKER_IP_SIZE]{};

//===============================EVENT HANDLER ==================================================

void MqttClient::mqttEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    MqttClient *client = reinterpret_cast<MqttClient *>(arg);
    esp_mqtt_event_handle_t event = reinterpret_cast<esp_mqtt_event_handle_t>(event_data);

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_BEFORE_CONNECT:
        client->Dispatch(EVENT_BEFORE_CONNECT{});
        break;
    case MQTT_EVENT_CONNECTED:
        client->Dispatch(EVENT_CONNECTED{});
        break;
    case MQTT_EVENT_DISCONNECTED:
        client->Dispatch(EVENT_DISCONNECTED{});
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESPARRAG_LOG_INFO("MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        client->Dispatch(EVENT_SUBSCRIBED{});
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESPARRAG_LOG_INFO("MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        client->Dispatch(EVENT_PUBLISHED{});
        break;
    case MQTT_EVENT_DATA:
        {
            Lock lock(client->m_dataMutex);
            memset(m_topic, 0, sizeof(m_topic));
            memset(m_payload, 0, sizeof(m_payload));
            strlcpy(m_topic, event->topic, event->topic_len + 1);
            strlcpy(m_payload, event->data, event->data_len + 1);
        } 

        client->Dispatch(EVENT_INCOMING_DATA{});
        break;
    case MQTT_EVENT_ERROR:
        client->Dispatch(EVENT_ERROR{});
        break;
    default:
        ESPARRAG_ASSERT(false);
        break;
    }
}

//===============================================================================================
//===============================PUBLIC METHODS ==================================================
//===============================================================================================

MqttClient::MqttClient() : FsmTask(8000, 3, "mqttTask@esparrag") {}

void MqttClient::Init()
{
    m_dataMutex = xSemaphoreCreateMutex();
    Start(STATE_DISABLED{});
}

void MqttClient::On(const char *topic, mqtt_handler_callback callback)
{
    ESPARRAG_ASSERT(m_handlers.size() != m_handlers.capacity());
    ESPARRAG_ASSERT(callback);
    mqtt_event_handler_t handler{.cb = callback, .topic = constructFullTopic(topic), .isSubscribed = false};

    if (IsInState<STATE_CONNECTED>())
    {
        if (subscribe(constructFullTopic(topic)))
        {
            handler.isSubscribed = true;
        }
    }

    m_handlers.push_back(handler);
}

eResult MqttClient::Publish(const char *topic, const char *msg)
{
    return publish(constructFullTopic(topic), msg);
}

eResult MqttClient::Publish(const char *topic, cJSON *msg)
{
    memset(m_payload, 0, sizeof(m_payload));
    bool printed = cJSON_PrintPreallocated(msg, m_payload, PAYLOAD_BUFFER_SIZE, false);
    if (!printed)
    {
        ESPARRAG_LOG_ERROR("mqtt publish buffer too small");
        return eResult::ERROR_INVALID_STATE;
    }

    return publish(constructFullTopic(topic), m_payload);
}

eResult MqttClient::TryConnect(const char* brokerIp) {

    if (IsInState<STATE_CONNECTING>())
    {
        ESPARRAG_LOG_INFO("mqtt already connecting");
        return eResult::SUCCESS;
    }

    if (IsInState<STATE_CONNECTED>())
    {
        ESPARRAG_LOG_INFO("mqtt already connected");
        return eResult::SUCCESS;
    }

    if (!brokerIp || strlen(brokerIp) == 0)
    {
        ESPARRAG_LOG_ERROR("mqtt ip not set");
        return eResult::ERROR_INVALID_STATE;
    }


    Dispatch(EVENT_CONNECT(brokerIp));
    return eResult::SUCCESS;
}


//================================ENTRY FUNCTIONS ================================================

void MqttClient::on_entry(STATE_DISABLED& state) {
    ESPARRAG_LOG_INFO("entered %s", state.NAME);

}
void MqttClient::on_entry(STATE_CONNECTING& state) {
    ESPARRAG_LOG_INFO("entered %s", state.NAME);

}
void MqttClient::on_entry(STATE_CONNECTED& state) {
    ESPARRAG_LOG_INFO("entered %s", state.NAME);

    reSubscribe();
}


//===============================================================================================
//================================ STATE MACHINE ================================================
//===============================================================================================

using return_state_t = MqttClient::return_state_t;

// STATE_DISABLED

return_state_t MqttClient::on_event(STATE_DISABLED &state, EVENT_CONNECT &event) {
    ESPARRAG_LOG_INFO("%s got %s", state.NAME, event.NAME);

    return connect(EVENT_CONNECT::m_brokerIp) == true ? return_state_t {STATE_CONNECTING{}} : std::nullopt;
}

return_state_t MqttClient::on_event(STATE_DISABLED &state, EVENT_BEFORE_CONNECT &event) {
    ESPARRAG_LOG_INFO("%s got %s", state.NAME, event.NAME);

    return STATE_CONNECTING{};
}
    
// STATE_DISABLED

return_state_t MqttClient::on_event(STATE_CONNECTING &state, EVENT_CONNECTED &event) {
    ESPARRAG_LOG_INFO("%s got %s", state.NAME, event.NAME);

    return STATE_CONNECTED{};
}

return_state_t MqttClient::on_event(STATE_CONNECTING &state, EVENT_DISCONNECTED &event) {
    ESPARRAG_LOG_INFO("%s got %s", state.NAME, event.NAME);

    return std::nullopt;
}

return_state_t MqttClient::on_event(STATE_CONNECTING &state, EVENT_ERROR &event) {
    ESPARRAG_LOG_INFO("%s got %s", state.NAME, event.NAME);

    return std::nullopt;
}

// STATE_CONNECTED

return_state_t MqttClient::on_event(STATE_CONNECTED &state, EVENT_SUBSCRIBE &event) {
    ESPARRAG_LOG_INFO("%s got %s", state.NAME, event.NAME);

    return std::nullopt;
}

return_state_t MqttClient::on_event(STATE_CONNECTED &state, EVENT_SUBSCRIBED &event) {
    ESPARRAG_LOG_INFO("%s got %s", state.NAME, event.NAME);

    return std::nullopt;
}

return_state_t MqttClient::on_event(STATE_CONNECTED &state, EVENT_PUBLISHED &event) {
    ESPARRAG_LOG_INFO("%s got %s", state.NAME, event.NAME);

    return std::nullopt;
}

return_state_t MqttClient::on_event(STATE_CONNECTED &state, EVENT_ERROR &event) {
    ESPARRAG_LOG_INFO("%s got %s", state.NAME, event.NAME);

    return STATE_CONNECTING{};
}

return_state_t MqttClient::on_event(STATE_CONNECTED &state, EVENT_DISCONNECTED &event) {
    ESPARRAG_LOG_INFO("%s got %s", state.NAME, event.NAME);

    return STATE_CONNECTING{};
}

return_state_t MqttClient::on_event(STATE_CONNECTED &state, EVENT_INCOMING_DATA &event) {
    ESPARRAG_LOG_INFO("%s got %s", state.NAME, event.NAME);
    handleData();

    return std::nullopt;
}


//===============================================================================================

void MqttClient::handleData()
{
    Lock lock(m_dataMutex);
    mqtt_event_handler_t *handler = findHandler(m_topic);
    if (!handler)
    {
        ESPARRAG_LOG_ERROR("no handler for topic %s", m_topic);
        return;
    }

    // --handle request
    cJSON *jsonPayload = cJSON_Parse(m_payload);
    if (jsonPayload == nullptr)
    {
        ESPARRAG_LOG_WARNING("mqtt payload is not a valid json, reformatting");
        jsonPayload = cJSON_CreateObject();
        cJSON_AddStringToObject(jsonPayload, "payload", m_payload);
    }


    handler->cb(m_topic, jsonPayload);
}

eResult MqttClient::publish(const char *topic, const char *payload)
{
    int err = esp_mqtt_client_publish(m_client, topic, payload, strlen(payload), 0, false);
    if (err == -1)
    {
        ESPARRAG_LOG_ERROR("mqtt publish failed");
        return eResult::ERROR_GENERAL;
    }

    return eResult::SUCCESS;
}

MqttClient::mqtt_event_handler_t *MqttClient::findHandler(const char *topic)
{
    ESPARRAG_LOG_INFO("recieved topic: %s", topic);
    for (size_t i = 0; i < m_handlers.size(); i++)
    {
        if (m_handlers[i].topic == topic)
        {
            return &m_handlers[i];
        }
    }

    return nullptr;
}

bool MqttClient::connect(const char* brokerIP)
{
    constexpr int MQTT_PORT = 1883;
    esp_mqtt_client_config_t config{};
    char mqttHost[50]{};

    if (m_client != nullptr)
    {
        return false;
    }

    snprintf(mqttHost, sizeof(mqttHost), "mqtt://%s:%d", brokerIP, MQTT_PORT);
    ESPARRAG_LOG_INFO("trying to connect to %s", mqttHost);

    config.port = MQTT_PORT;
    config.uri = mqttHost;
    config.host = mqttHost;
    config.client_id = DEVICE_NAME;
    config.username = DEVICE_NAME;

    m_client = esp_mqtt_client_init(&config);
    if (!m_client)
    {
        ESPARRAG_LOG_ERROR("could not create mqtt client");
        return false;
    }

    esp_err_t err = esp_mqtt_client_register_event(m_client, MQTT_EVENT_ANY, mqttEventHandler, this);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("could not register event handler");
        return false;
    }

    err = esp_mqtt_client_start(m_client);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("could not start mqtt");
        return false;
    }

    ESPARRAG_LOG_INFO("mqtt initiated");
    return true;
}

const char *MqttClient::constructFullTopic(const char *topic)
{
    static char buffer[TOPIC_BUFFER_SIZE];

    ESPARRAG_ASSERT(topic && strlen(topic) > 0);
    ESPARRAG_ASSERT(topic[0] == '/');

    memset(buffer, 0, TOPIC_BUFFER_SIZE);
    snprintf(buffer, TOPIC_BUFFER_SIZE, "/%s%s", DEVICE_NAME, topic);

    return buffer;
}

bool MqttClient::subscribe(const char *topic)
{
    ESPARRAG_ASSERT(IsInState<STATE_CONNECTED>());
    esp_err_t err = esp_mqtt_client_subscribe(m_client, topic, 0);
    if (err == -1)
    {
        ESPARRAG_LOG_ERROR("subscribe failed, err %d", err);
        return false;
    }

    return true;
}

void MqttClient::reSubscribe()
{
    ESPARRAG_ASSERT(IsInState<STATE_CONNECTED>());

    for (size_t i = 0; i < m_handlers.size(); i++)
    {
        m_handlers[i].isSubscribed =
            subscribe(m_handlers[i].topic.data());
    }
}
