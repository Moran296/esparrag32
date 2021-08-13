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
#include "cJSON.h"

class MqttClient
{
public:
    using mqtt_handler_callback = etl::delegate<eResult(Request &, Response &)>;
    struct mqtt_event_handler_t
    {
        static constexpr int TOPIC_MAX_LEN = 15;
        mqtt_handler_callback cb;
        etl::string<TOPIC_MAX_LEN> topic;
    };

    void Init();
    // eResult Publish(const char *topic, cJSON *msg) {}
    // eResult On(const char *topic, mqtt_handler_callback callback) {}

private:
    esp_mqtt_client_handle_t m_client{};

    const char *constructFullTopic(const char *topic);
    void handleData(esp_mqtt_event_t *event);
    void updateCloudState(eMqttState state);
    void init(DB_PARAM_DIRTY_LIST(Settings::Status) list);

    static void mqttEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
};

#endif