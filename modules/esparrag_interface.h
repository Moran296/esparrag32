#ifndef ESPARRAG_INTERFACE_H__
#define ESPARRAG_INTERFACE_H__

#include "esparrag_common.h"
#include "esparrag_mqtt.h"
#include "esparrag_http.h"

#define DECLARE_HANDLER(NAME) \
    static void NAME(Request &req, Response &res);

class EsparragInterface
{
public:
    EsparragInterface(MqttClient &mqtt, HttpServer &http) : m_mqtt(mqtt), m_http(http) {}
    void RegisterHandlers();

private:
    MqttClient &m_mqtt;
    HttpServer &m_http;

    DECLARE_HANDLER(credentials_post)
    DECLARE_HANDLER(mqtt_hello)
};

#endif