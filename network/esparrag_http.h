#ifndef EPARRAG_HTTP_SERVER__
#define EPARRAG_HTTP_SERVER__

#include "esparrag_common.h"
#include "esparrag_database.h"
#include "esp_http_server.h"
#include "esparrag_request.h"
#include "app_data.h"
#include "esparrag_response.h"
#include "etl/delegate.h"
#include "etl/string.h"
#include "etl/vector.h"
#include "cJSON.h"

using http_handler_callback = etl::delegate<eResult(Request &, Response &)>;
using promise_callback = etl::delegate<void(void *)>;

struct http_event_handler_t
{
    static constexpr int URI_MAX_LEN = 15;
    http_handler_callback cb;
    etl::string<URI_MAX_LEN> uri;
    METHOD method;
};

class HttpServer
{
public:
    static constexpr uint8_t HANDLERS_MAX_NUM = 15;

    eResult Init();
    eResult On(const char *uri,
               METHOD method,
               http_handler_callback callback);

private:
    bool m_isRunning = false;
    etl::vector<http_event_handler_t, HANDLERS_MAX_NUM> m_handlers;

    httpd_handle_t m_handle = nullptr;
    httpd_config_t m_config{};

    eResult runServer();
    eResult stopServer();
    eResult registerHandlers();
    cJSON *parseHtmlBody(const char *body);
    http_event_handler_t *findHandler(httpd_req_t *esp_request);
    void sendResponse(httpd_req_t *esp_request, Response &response);
    void dbStatusChange(DB_PARAM_DIRTY_LIST(AppData::Status) list);

    static esp_err_t requestHandler(httpd_req_t *esp_request);
    static esp_err_t post_handler(httpd_req_t *req);
    static esp_err_t get_handler(httpd_req_t *req);
};

#endif