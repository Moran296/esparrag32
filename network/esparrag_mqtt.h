#ifndef ESPARRAG_MQTT_H__
#define ESPARRAG_MQTT_H__

#include "etl/string.h"
#include "mqtt_client.h"
#include <functional>
#include "etl/vector.h"
#include "esparrag_request.h"
#include "cJSON.h"
#include "fsm_task.h"

namespace MqttFSM {

struct STATE_DISABLED{
    static constexpr const char* NAME = "STATE_DISABLED";
};
struct STATE_CONNECTING{
    static constexpr const char * NAME = "STATE_CONNECTING";
};
struct STATE_CONNECTED{
    static constexpr const char* NAME = "STATE_CONNECTED";
};

using States = std::variant<STATE_DISABLED,
                                STATE_CONNECTING,
                                STATE_CONNECTED>;


struct EVENT_CONNECT{
    static constexpr int MQTT_BROKER_IP_SIZE = 20;
    static constexpr const char* NAME = "EVENT_CONNECT";

    EVENT_CONNECT(const char* brokerIP) {
        strlcpy(m_brokerIp, brokerIP, MQTT_BROKER_IP_SIZE);
    }

    static char m_brokerIp[MQTT_BROKER_IP_SIZE];
};

struct EVENT_BEFORE_CONNECT{
    static constexpr const char* NAME = "EVENT_BEFORE_CONNECT";
};
struct EVENT_CONNECTED{
    static constexpr const char* NAME = "EVENT_CONNECTED";
};
struct EVENT_DISCONNECTED{
    static constexpr const char* NAME = "EVENT_DISCONNECTED";
};
struct EVENT_SUBSCRIBE{
    static constexpr const char* NAME = "EVENT_SUBSCRIBE";
};
struct EVENT_SUBSCRIBED{
    static constexpr const char* NAME = "EVENT_SUBSCRIBED";
};
struct EVENT_PUBLISH{
    static constexpr const char* NAME = "EVENT_PUBLISH";
    const char* topic;
    cJSON* payload;
};
struct EVENT_PUBLISHED{
    static constexpr const char* NAME = "EVENT_PUBLISHED";
};
struct EVENT_ERROR{
    static constexpr const char* NAME = "EVENT_ERROR";
};
struct EVENT_INCOMING_DATA{
    static constexpr const char* NAME = "EVENT_INCOMING_DATA";
};

using Events = std::variant<EVENT_BEFORE_CONNECT,
                               EVENT_CONNECTED,
                               EVENT_CONNECT,
                               EVENT_DISCONNECTED,
                               EVENT_SUBSCRIBE,
                               EVENT_SUBSCRIBED,
                               EVENT_PUBLISH,
                               EVENT_PUBLISHED,
                               EVENT_ERROR,
                               EVENT_INCOMING_DATA>;


} // namespace MqttFSM


class MqttClient : public FsmTask<MqttClient, MqttFSM::States, MqttFSM::Events>
{
public:
    using mqtt_handler_callback = std::function<void(const char* topic, cJSON* payload)>;

    static constexpr int TOPIC_BUFFER_SIZE = 100;
    static constexpr int PAYLOAD_BUFFER_SIZE = 4096;
    struct mqtt_event_handler_t
    {
        mqtt_handler_callback cb;
        etl::string<TOPIC_BUFFER_SIZE> topic;
        bool isSubscribed{};
    };
    using handlers_t = etl::vector<mqtt_event_handler_t, 30>;

    MqttClient();
    void Init();
    void On(const char *topic, mqtt_handler_callback callback);
    eResult Publish(const char *topic, cJSON *msg);
    eResult TryConnect(const char* brokerIp);


    void on_entry(MqttFSM::STATE_DISABLED&);
    void on_entry(MqttFSM::STATE_CONNECTING&);
    void on_entry(MqttFSM::STATE_CONNECTED&);

    using return_state_t = std::optional<MqttFSM::States>;

    return_state_t on_event(MqttFSM::STATE_DISABLED &, MqttFSM::EVENT_CONNECT &);
    return_state_t on_event(MqttFSM::STATE_DISABLED &, MqttFSM::EVENT_BEFORE_CONNECT &);

    return_state_t on_event(MqttFSM::STATE_CONNECTING &, MqttFSM::EVENT_CONNECTED &);
    return_state_t on_event(MqttFSM::STATE_CONNECTING &, MqttFSM::EVENT_DISCONNECTED &);
    return_state_t on_event(MqttFSM::STATE_CONNECTING &, MqttFSM::EVENT_ERROR &);

    return_state_t on_event(MqttFSM::STATE_CONNECTED &, MqttFSM::EVENT_SUBSCRIBE &);
    return_state_t on_event(MqttFSM::STATE_CONNECTED &, MqttFSM::EVENT_SUBSCRIBED &);
    return_state_t on_event(MqttFSM::STATE_CONNECTED &, MqttFSM::EVENT_PUBLISH &);
    return_state_t on_event(MqttFSM::STATE_CONNECTED &, MqttFSM::EVENT_PUBLISHED &);
    return_state_t on_event(MqttFSM::STATE_CONNECTED &, MqttFSM::EVENT_ERROR &);
    return_state_t on_event(MqttFSM::STATE_CONNECTED &, MqttFSM::EVENT_DISCONNECTED &);
    return_state_t on_event(MqttFSM::STATE_CONNECTED &, MqttFSM::EVENT_INCOMING_DATA &);

    template <typename State, typename Event> 
    auto on_event(State &state, Event &event) {
        printf("unhandled event!: %s got %s\n", state.NAME, event.NAME);
        return std::nullopt;
    }


private:
    esp_mqtt_client_handle_t m_client{};
    handlers_t m_handlers;
    SemaphoreHandle_t m_dataMutex;

    const char *constructFullTopic(const char *topic);
    void handleData();
    bool connect(const char* brokerIP);
    bool subscribe(const char *topic);
    eResult publish(const char *topic, const char *payload);
    void reSubscribe();
    mqtt_event_handler_t *findHandler(const char *topic);

    static void mqttEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

    static char m_topic[TOPIC_BUFFER_SIZE];
    static char m_payload[PAYLOAD_BUFFER_SIZE];
};

#endif