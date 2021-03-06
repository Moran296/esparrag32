#ifndef ESPARRAG_RESPONSE_H__
#define ESPARRAG_RESPONSE_H__

#include "cJSON.h"
#include "esp_http_server.h"
#include "esparrag_common.h"
#include "etl/enum_type.h"

struct Response
{
    struct FORMAT
    {
        enum enum_type
        {
            JSON,
            HTML
        };

        ETL_DECLARE_ENUM_TYPE(FORMAT, uint8_t)
        ETL_ENUM_TYPE(JSON, "application/json")
        ETL_ENUM_TYPE(HTML, "text/html")
        ETL_END_ENUM_TYPE
    };

    struct CODE
    {
        enum enum_type
        {
            HTTP_CODE_PROCESSING = 102,
            HTTP_CODE_OK = 200,
            HTTP_CODE_CREATED = 201,
            HTTP_CODE_ACCEPTED = 202,
            HTTP_CODE_NO_CONTENT = 204,
            HTTP_CODE_BAD_REQUEST = 400,
            HTTP_CODE_UNAUTHORIZED = 401,
            HTTP_CODE_NOT_FOUND = 404,
            HTTP_CODE_METHOD_NOT_ALLOWED = 405,
            HTTP_CODE_REQUEST_TIMEOUT = 408,
            HTTP_CODE_GONE = 410,
            HTTP_CODE_URI_TOO_LONG = 414,
            HTTP_CODE_INTERNAL_SERVER_ERROR = 500,
            HTTP_CODE_SERVICE_UNAVAILABLE = 503,
            HTTP_CODE_GATEWAY_TIMEOUT = 504,
        };

        ETL_DECLARE_ENUM_TYPE(CODE, uint16_t)
        ETL_ENUM_TYPE(HTTP_CODE_PROCESSING, "102 Processing")
        ETL_ENUM_TYPE(HTTP_CODE_OK, HTTPD_200)
        ETL_ENUM_TYPE(HTTP_CODE_CREATED, "201 Created")
        ETL_ENUM_TYPE(HTTP_CODE_ACCEPTED, "202 Accepted")
        ETL_ENUM_TYPE(HTTP_CODE_NO_CONTENT, HTTPD_204)
        ETL_ENUM_TYPE(HTTP_CODE_BAD_REQUEST, HTTPD_400)
        ETL_ENUM_TYPE(HTTP_CODE_UNAUTHORIZED, "401 Unauthorized")
        ETL_ENUM_TYPE(HTTP_CODE_NOT_FOUND, HTTPD_404)
        ETL_ENUM_TYPE(HTTP_CODE_METHOD_NOT_ALLOWED, "Method Not Allowed")
        ETL_ENUM_TYPE(HTTP_CODE_REQUEST_TIMEOUT, HTTPD_408)
        ETL_ENUM_TYPE(HTTP_CODE_GONE, "410 Gone")
        ETL_ENUM_TYPE(HTTP_CODE_URI_TOO_LONG, "414 Request-URI Too Large")
        ETL_ENUM_TYPE(HTTP_CODE_INTERNAL_SERVER_ERROR, HTTPD_500)
        ETL_ENUM_TYPE(HTTP_CODE_SERVICE_UNAVAILABLE, "503 Service Unavailable")
        ETL_ENUM_TYPE(HTTP_CODE_GATEWAY_TIMEOUT, "504 Gateway Time-out")
        ETL_END_ENUM_TYPE
    };
    Response() : m_json(cJSON_CreateObject()), m_string(nullptr), m_code(200), m_format(0)
    {
        ESPARRAG_ASSERT(m_json != nullptr);
    }

    ~Response()
    {
        cJSON_Delete(m_json);
    }

    cJSON *m_json;
    const char *m_string;
    CODE m_code;
    FORMAT m_format;
};

#endif