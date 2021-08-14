#include "esparrag_mqtt.h"
#include "esparrag_log.h"

char MqttClient::m_payload[PAYLOAD_BUFFER_SIZE]{};

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
        client->m_connected = true;
        client->reSubscribe();
        client->identify();
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESPARRAG_LOG_INFO("MQTT_EVENT_DISCONNECTED");
        client->m_connected = false;
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

void MqttClient::identify()
{
    ESPARRAG_ASSERT(m_connected);
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "name", DEVICE_NAME);
    cJSON_AddStringToObject(json, "description", DEVICE_DESC);
    ESPARRAG_ASSERT(Publish(IDENTIFICATION_TOPIC, json) == eResult::SUCCESS);
    cJSON_Delete(json);
}

void MqttClient::Init()
{
    auto changeCB = DB_PARAM_CALLBACK(Settings::Status)::create<MqttClient, &MqttClient::init>(*this);
    Settings::Status.Subscribe<eStatus::BROKER_IP>(changeCB);
}

void MqttClient::On(const char *topic, mqtt_handler_callback callback)
{
    ESPARRAG_ASSERT(m_handlers.size() != m_handlers.capacity());
    ESPARRAG_ASSERT(callback.is_valid());
    mqtt_event_handler_t handler{.cb = callback, .topic = constructFullTopic(topic), .isSubscribed = false};

    if (m_connected)
    {
        if (subscribe(constructFullTopic(topic)))
        {
            handler.isSubscribed = true;
        }
    }

    m_handlers.push_back(handler);
}

void MqttClient::handleData(esp_mqtt_event_t *event)
{
    static char topic[TOPIC_BUFFER_SIZE];
    memset(topic, 0, sizeof(topic));
    memset(m_payload, 0, sizeof(m_payload));
    strlcpy(topic, event->topic, event->topic_len + 1);
    strlcpy(m_payload, event->data, event->data_len + 1);

    if (strncmp(topic, IDENTIFICATION_REQUEST, strlen(topic)) == 0)
    {
        identify();
        return;
    }

    mqtt_event_handler_t *handler = findHandler(topic);
    if (!handler)
    {
        ESPARRAG_LOG_ERROR("no handler for topic %s", topic);
        return;
    }

    // --handle request
    cJSON *jsonPayload = cJSON_Parse(m_payload);
    Request request(jsonPayload, topic);
    Response response;
    handler->cb(request, response);

    // --prepare response
    ESPARRAG_ASSERT(response.m_format == Response::FORMAT::JSON)

    cJSON *uuid = nullptr;
    if (request.m_content)
    {
        uuid = cJSON_GetObjectItem(request.m_content, "uuid");
    }

    if (uuid && cJSON_IsString(uuid))
    {
        ESPARRAG_LOG_INFO("there is uuid here");
        cJSON_AddItemReferenceToObject(response.m_json, "uuid", uuid);
    }
    else
        ESPARRAG_LOG_ERROR("missing uuid");

    strlcat(topic, "/response", TOPIC_BUFFER_SIZE);
    memset(m_payload, 0, sizeof(m_payload));
    bool printed = cJSON_PrintPreallocated(response.m_json, m_payload, PAYLOAD_BUFFER_SIZE, false);
    if (!printed)
    {
        ESPARRAG_LOG_ERROR("mqtt publish buffer too small");
        return;
    }

    publish(topic, m_payload);
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

eResult MqttClient::Publish(const char *topic, const char *msg)
{
    return publish(constructFullTopic(topic), msg);
}

eResult MqttClient::publish(const char *topic, const char *payload)
{
    if (!m_connected)
        return eResult::ERROR_INVALID_STATE;

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
    if (Settings::Status.Set<eStatus::MQTT_STATE, uint8_t>(state))
        Settings::Status.Commit();
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
    ESPARRAG_ASSERT(m_connected);
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
    ESPARRAG_ASSERT(m_connected);

    subscribe(IDENTIFICATION_REQUEST);
    for (size_t i = 0; i < m_handlers.size(); i++)
    {
        m_handlers[i].isSubscribed =
            subscribe(m_handlers[i].topic.data());
    }
}
