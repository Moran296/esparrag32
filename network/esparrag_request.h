#ifndef ESPARRAG_REQUEST_H__
#define ESPARRAG_REQUEST_H__

#include "cJSON.h"
#include "esp_http_server.h"
#include "mqtt_client.h"
#include "esparrag_common.h"

struct eMethod
{
    enum enum_type
    {
        GET = HTTP_GET,
        POST = HTTP_POST,
        DELETE = HTTP_DELETE,
        PUT = HTTP_PUT,
        GENERAL
    };

    ETL_DECLARE_ENUM_TYPE(eMethod, uint8_t)
    ETL_ENUM_DEFAULT(GET)
    ETL_ENUM_DEFAULT(POST)
    ETL_ENUM_DEFAULT(DELETE)
    ETL_ENUM_DEFAULT(PUT)
    ETL_ENUM_DEFAULT(GENERAL)
    ETL_END_ENUM_TYPE
};

struct Request
{
    Request(cJSON *content, const char *uri, eMethod method) : m_content(content),
                                                               m_uri(uri),
                                                               m_method(method)
    {
    }
    Request(cJSON *content, const char *uri) : m_content(content),
                                               m_uri(uri),
                                               m_method(eMethod::GENERAL)
    {
    }

    ~Request()
    {
        if (m_content)
            cJSON_Delete(m_content);
    }

    cJSON *m_content;
    const char *m_uri;
    eMethod m_method;
};

#endif