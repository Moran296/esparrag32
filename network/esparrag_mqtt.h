#ifndef ESPARRAG_MQTT_H__
#define ESPARRAG_MQTT_H__

#include "etl/string.h"
#include "dns_resolver.h"
#include "mqtt_client.h"
#include "etl/delegate.h"
#include "etl/string.h"
#include "etl/vector.h"
#include "esparrag_request.h"
#include "esparrag_settings.h"
#include "esparrag_response.h"
#include "esparrag_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cJSON.h"

using mqtt_handler_callback = etl::delegate<void(Request &, Response &)>;

class MqttClient
{
public:
    static constexpr int TOPIC_BUFFER_SIZE = 100;
    static constexpr int PAYLOAD_BUFFER_SIZE = 4096;
    static constexpr char IDENTIFICATION_TOPIC[] = "/hiya";
    static constexpr char IDENTIFICATION_REQUEST[] = "/identify";
    struct mqtt_event_handler_t
    {
        mqtt_handler_callback cb;
        etl::string<TOPIC_BUFFER_SIZE> topic;
        bool isSubscribed{};
    };
    using handlers_t = etl::vector<mqtt_event_handler_t, 30>;

    void Init();
    void On(const char *topic, mqtt_handler_callback callback);
    eResult Publish(const char *topic, cJSON *msg);
    eResult Publish(const char *topic, const char *msg);

private:
    esp_mqtt_client_handle_t m_client{};
    bool m_connected{};
    handlers_t m_handlers;

    const char *constructFullTopic(const char *topic);
    void handleData(esp_mqtt_event_t *event);
    void updateCloudState(eMqttState state);
    void init(DB_PARAM_DIRTY_LIST(Settings::Status) list);
    void identify();
    bool subscribe(const char *topic);
    eResult publish(const char *topic, const char *payload);
    void reSubscribe();
    mqtt_event_handler_t *findHandler(const char *topic);

    static void mqttEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

    static char m_payload[PAYLOAD_BUFFER_SIZE];
};

#endif