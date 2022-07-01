#ifndef ESPARRAG__RESULT_H__
#define ESPARRAG__RESULT_H__

#include <variant>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <experimental/source_location>
#include "esparrag_common.h"

template <class OK, class ERROR = eResult::enum_type>
class EsparragResult
{
    using location = std::experimental::source_location;

    public:
    EsparragResult(OK ok) : m_res(ok) {}
    EsparragResult(ERROR error, location loc = location::current()) : m_res(error), m_errorLoc(loc) {}

    bool IsOk() const { return std::holds_alternative<OK>(m_res); }
    bool IsError() const { return std::holds_alternative<ERROR>(m_res); }
    operator bool() { return IsOk(); }

    OK& ok_or_assert(std::experimental::source_location assert_loc = std::experimental::source_location::current()) {
        if (!IsOk()) {
            resultAssert(assert_loc);
            vTaskDelay(portMAX_DELAY);
        }

        return std::get<OK>(m_res);
    }

    OK& ok_or_default(const OK &def) {
        if (IsOk())
            return std::get<OK>(m_res);
        return def;
    }

    ERROR &Error() { return std::get<ERROR>(m_res); }
    const location &Location() const { return m_errorLoc; }

    private:
    void resultAssert(std::experimental::source_location &assert_loc) {
        char error_msg[100];
        snprintf(error_msg, sizeof(error_msg), "failed unwrapping result at %.30s:%d", strrchr(assert_loc.file_name(), '/'), assert_loc.line());
        ets_printf(error_msg);
        configASSERT(0);
    }

    std::variant<OK, ERROR> m_res;
    location m_errorLoc;
};

#define PRINT_ERROR_RES(err)                                     \
    do {                                                         \
        std::experimental::source_location loc = err.Location(); \
        ESPARRAG_LOG_ERROR("%s, at %.30s:%d %s",                 \
                       eResult(err.Error()).c_str(),             \
                       strrchr(loc.file_name(), '/'),            \
                       loc.line(), loc.function_name());         \
    } while (0)


#endif
