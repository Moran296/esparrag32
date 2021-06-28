#include "esparrag_http.h"
#include "etl/mutex.h"
#include "etl/string_view.h"
#include "esparrag_wifi.h"

#define HTTP_REQUEST_CONTENT_MAX_SIZE 512
#define JSON_CONTENT_TYPE "application/json"
#define DATA_FIELD "\"body\""

cJSON *HttpServer::parseBody(const char *body)
{
    char *index = strstr(body, DATA_FIELD);
    if (!index)
        return nullptr;

    return cJSON_Parse(index + sizeof(DATA_FIELD));
}

esp_err_t HttpServer::requestHandler(httpd_req_t *esp_request)
{
    HttpServer *server = reinterpret_cast<HttpServer *>(esp_request->user_ctx);
    eResult res = eResult::SUCCESS;
    static char content[HTTP_REQUEST_CONTENT_MAX_SIZE];
    memset(content, 0, sizeof(content));

    size_t recv_size = etl::min<size_t>(esp_request->content_len, HTTP_REQUEST_CONTENT_MAX_SIZE);

    //recieve the content
    int ret = httpd_req_recv(esp_request, content, recv_size);
    if (ret < 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(esp_request);
        }

        return ESP_FAIL;
    }

    //find the handler for the request
    http_event_handler_t *handler = server->findHandler(esp_request);
    if (!handler || !handler->cb.is_valid())
    {
        ESPARRAG_LOG_ERROR("no handler found for uri %s, method %d", esp_request->uri, esp_request->method);
        httpd_resp_send_404(esp_request);
        return ESP_OK;
    }

    ESPARRAG_LOG_INFO("found handler");

    //prepare request and response structs and activate the handler
    Request request(server->parseBody(content), esp_request->uri, METHOD(esp_request->method));
    Response response;
    res = handler->cb(request, response);
    if (res != eResult::SUCCESS)
    {
        //result should be error only if there was a problem handling the request (invalid request struct)
        ESPARRAG_LOG_ERROR("handle request failed!");
        httpd_resp_send_404(esp_request);
        return ESP_OK;
    }

    //send the response aquired from the handler
    server->sendResponse(esp_request, response);
    return ESP_OK;
}

http_event_handler_t *HttpServer::findHandler(httpd_req_t *esp_request)
{
    for (size_t i = 0; i < m_handlers.size(); i++)
    {
        bool uriMatch = m_handlers[i].uri == esp_request->uri;
        bool methodMatch = m_handlers[i].method == esp_request->method ||
                           m_handlers[i].method == METHOD::GENERAL;

        if (uriMatch && methodMatch)
        {
            return &m_handlers[i];
        }
    }

    return nullptr;
}

void HttpServer::sendResponse(httpd_req_t *esp_request, Response &response)
{
    httpd_resp_set_hdr(esp_request, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(esp_request, "Access-Control-Max-Age", "10000");
    httpd_resp_set_hdr(esp_request, "Access-Control-Allow-Methods", "POST,GET,DELETE,OPTIONS");
    httpd_resp_set_hdr(esp_request, "Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
    httpd_resp_set_type(esp_request, JSON_CONTENT_TYPE);
    httpd_resp_set_status(esp_request, response.m_code.c_str());

    char *responseString = cJSON_Print(response.m_response);
    int bytes = httpd_resp_send(esp_request, responseString, HTTPD_RESP_USE_STRLEN);
    if (bytes < 0)
    {
        ESPARRAG_LOG_ERROR("error sending response, err %d", bytes);
    }

    cJSON_free(responseString);
}

HttpServer::HttpServer(ConfigDB &db) : m_db(db),
                                       m_handle(nullptr) {}

eResult HttpServer::Init()
{
    auto changeCB = config_change_cb::create<HttpServer, &HttpServer::dbConfigChange>(*this);
    m_db.Subscribe<CONFIG_ID::WIFI_STATUS> (changeCB);

    m_config = HTTPD_DEFAULT_CONFIG();
    m_config.uri_match_fn = httpd_uri_match_wildcard;

    ESPARRAG_LOG_INFO("http server initialized");

    uint8_t enumGetter = 0;
    m_db.Get(CONFIG_ID::WIFI_STATUS, enumGetter);
    WIFI_STATUS mode(enumGetter);

    if (mode != WIFI_STATUS::OFFLINE && !m_isRunning)
    {
        runServer();
    }

    return eResult::SUCCESS;
}

eResult HttpServer::On(const char *uri,
                       METHOD method,
                       http_handler_callback callback)
{
    if (!uri || !callback)
    {
        ESPARRAG_LOG_ERROR("NULL parameters");
        return eResult::ERROR_INVALID_PARAMETER;
    }

    //TODO use esp wildcard function to check instead of manually checking
    if (uri[0] != '/')
    {
        ESPARRAG_LOG_ERROR("uri must start with \"/\"");
        return eResult::ERROR_INVALID_PARAMETER;
    }

    if (m_handlers.size() >= HANDLERS_MAX_NUM)
    {
        ESPARRAG_LOG_ERROR("maximum amount of handlers have been registered");
        return eResult::ERROR_INVALID_PARAMETER;
    }

    for (size_t i = 0; i < m_handlers.size(); i++)
    {
        bool uriMatch = m_handlers[i].uri == uri;
        bool methodMatch = m_handlers[i].method == method ||
                           m_handlers[i].method == METHOD::GENERAL;
        if (uriMatch && methodMatch)
        {
            ESPARRAG_LOG_ERROR("Handler for this request already exists! uri %s", uri);
            return eResult::ERROR_INVALID_PARAMETER;
        }
    }

    http_event_handler_t handler = {.cb = callback,
                                    .uri{uri},
                                    .method = method};
    m_handlers.push_back(handler);

    ESPARRAG_LOG_INFO("Request handler added!");
    return eResult::SUCCESS;
}

eResult HttpServer::registerHandlers()
{
    httpd_uri_t uriParams = {
        .uri = "/*",
        .method = HTTP_POST,
        .handler = requestHandler,
        .user_ctx = this};

    esp_err_t err = ESP_OK;
    err |= httpd_register_uri_handler(m_handle, &uriParams);
    uriParams.method = HTTP_GET;
    err |= httpd_register_uri_handler(m_handle, &uriParams);
    uriParams.method = HTTP_DELETE;
    err |= httpd_register_uri_handler(m_handle, &uriParams);
    uriParams.method = HTTP_PUT;
    err |= httpd_register_uri_handler(m_handle, &uriParams);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("couldnt register uri handlers. err %d", err);
        return eResult::ERROR_NOT_INITIALIZED;
    }

    ESPARRAG_LOG_INFO("request handlers registered");
    return eResult::SUCCESS;
}

eResult HttpServer::runServer()
{
    ESPARRAG_LOG_INFO("running http server");
    if (m_isRunning)
    {
        ESPARRAG_LOG_INFO("already running!");
        return eResult::ERROR_INVALID_STATE;
    }

    m_isRunning = true;
    esp_err_t err = httpd_start(&m_handle, &m_config);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("couldn't start server, err = %d", err);
        m_isRunning = false;
        return eResult::ERROR_NOT_INITIALIZED;
    }

    return registerHandlers();
}

eResult HttpServer::stopServer()
{
    ESPARRAG_LOG_INFO("stopping http server");

    if (!m_isRunning)
    {
        ESPARRAG_LOG_INFO("already stopped!");
        return eResult::ERROR_INVALID_STATE;
    }

    esp_err_t err = httpd_unregister_uri(m_handle, "/*");
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("couldn't unregister, err = %d", err);
        return eResult::ERROR_GENERAL;
    }

    err = httpd_stop(m_handle);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("couldn't stop server, err = %d", err);
        return eResult::ERROR_GENERAL;
    }

    m_isRunning = false;
    return eResult::SUCCESS;
}

void HttpServer::dbConfigChange(const dirty_list_t &dirty_list)
{
    uint8_t getter = 0;
    m_db.Get(CONFIG_ID::WIFI_STATUS, getter);
    WIFI_STATUS mode(getter);

    if (mode != WIFI_STATUS::OFFLINE && !m_isRunning)
    {
        runServer();
    }
}
