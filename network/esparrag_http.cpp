#include "esparrag_http.h"
#include "etl/mutex.h"
#include "etl/string_view.h"
#include "etl/string.h"
#include "esparrag_wifi.h"

#define HTTP_REQUEST_CONTENT_MAX_SIZE 512
#define DATA_FIELD "\"body\""

cJSON *HttpServer::parseHtmlBody(const char *body)
{
    static char key_buffer[10] = {0};
    static char val_buffer[24] = {0};
    etl::string_view view(body);
    if (view.find("=") == etl::string_view::npos)
        return nullptr;
    cJSON *content = cJSON_CreateObject();
    ESPARRAG_ASSERT(content);

    size_t end = view.find("=");
    while (end != etl::string_view::npos)
    {
        strncpy(key_buffer, view.begin(), end);
        ESPARRAG_LOG_INFO("key - %s", key_buffer);

        view.remove_prefix(end + 1);
        end = view.find("&");

        strncpy(val_buffer, view.begin(), etl::min(end, view.size()));
        ESPARRAG_LOG_INFO("val - %s", val_buffer);

        view.remove_prefix(end + 1);
        end = view.find("=");

        if (strlen(key_buffer) > 0 && strlen(val_buffer) > 0)
            cJSON_AddStringToObject(content, key_buffer, val_buffer);

        memset(key_buffer, 0, sizeof(key_buffer));
        memset(val_buffer, 0, sizeof(val_buffer));
    }

    return content;
}

esp_err_t HttpServer::requestHandler(httpd_req_t *esp_request)
{
    HttpServer *server = reinterpret_cast<HttpServer *>(esp_request->user_ctx);
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

    ESPARRAG_LOG_INFO("handling %s", esp_request->uri);

    cJSON *json_body = cJSON_Parse(content);
    if (json_body == nullptr)
        json_body = server->parseHtmlBody(content);

    Request request(json_body, esp_request->uri, eMethod(esp_request->method));
    Response response;
    handler->cb(request, response);

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
                           m_handlers[i].method == eMethod::GENERAL;

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
    httpd_resp_set_type(esp_request, response.m_format.c_str());
    httpd_resp_set_status(esp_request, response.m_code.c_str());

    bool sendJson = response.m_format == Response::FORMAT::JSON;
    const char *responseString = sendJson == true ? cJSON_Print(response.m_json) : response.m_string;
    int bytes = httpd_resp_send(esp_request, responseString, HTTPD_RESP_USE_STRLEN);
    if (bytes < 0)
    {
        ESPARRAG_LOG_ERROR("error sending response, err %d", bytes);
    }

    if (sendJson)
        cJSON_free((void *)responseString);
}

eResult HttpServer::Init()
{
    auto changeCB = DB_PARAM_CALLBACK(Settings::Status)::create<HttpServer, &HttpServer::dbStatusChange>(*this);
    Settings::Status.Subscribe<eStatus::WIFI_STATE>(changeCB);

    m_config = HTTPD_DEFAULT_CONFIG();
    m_config.uri_match_fn = httpd_uri_match_wildcard;

    ESPARRAG_LOG_INFO("http server initialized");

    uint8_t mode = 0;
    Settings::Status.Get<eStatus::WIFI_STATE>(mode);
    if (mode != WIFI_OFFLINE && !m_isRunning)
    {
        runServer();
    }

    return eResult::SUCCESS;
}

eResult HttpServer::On(const char *uri,
                       eMethod method,
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
        bool uriMatch = httpd_uri_match_wildcard(m_handlers[i].uri.data(), uri, strlen(uri));
        bool methodMatch = m_handlers[i].method == method ||
                           m_handlers[i].method == eMethod::GENERAL;
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

void HttpServer::dbStatusChange(DB_PARAM_DIRTY_LIST(Settings::Status) list)
{
    uint8_t mode = 0;
    Settings::Status.Get<eStatus::WIFI_STATE>(mode);
    if (mode != WIFI_OFFLINE && !m_isRunning)
    {
        runServer();
    }
}