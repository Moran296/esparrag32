#ifndef HEIHEI_WIFI__
#define HEIHEI_WIFI__

#include "esp_netif.h"
#include "esp_event.h"
#include "fsm_taskless.h"
#include "etl/string.h"
#include <variant>


using SSID_T = etl::string<32>;
using PASSWORD_T = etl::string<32>;

struct STATE_Offline{};
struct STATE_AP{};
struct STATE_Connecting{};
struct STATE_Connected{};
using WifiStates = std::variant<STATE_Offline, STATE_AP, STATE_Connected, STATE_Connecting>;

struct EVENT_APStart{};
struct EVENT_APStop{};
struct EVENT_Disconnect{};
struct EVENT_StaConnect{};
struct EVENT_StaStart{};
struct EVENT_StaConnected{};
struct EVENT_LoseConnection{};
struct EVENT_UserConnected{};
struct EVENT_GotIP{};
using WifiEvents = std::variant<EVENT_APStart, EVENT_APStop, EVENT_Disconnect, EVENT_StaConnect, EVENT_StaStart, EVENT_StaConnected, EVENT_LoseConnection, EVENT_UserConnected, EVENT_GotIP>;

class Wifi : public FsmTaskless<Wifi, WifiStates, WifiEvents>
{

public:
    Wifi();
    void Init();
    bool Connect(const SSID_T& ssid, const PASSWORD_T& password);
    bool SwitchToAP();
    bool Disconnect();

    void on_entry(STATE_Offline&);
    void on_entry(STATE_AP&);
    void on_entry(STATE_Connecting&);
    void on_entry(STATE_Connected&);

    //Offline
    auto on_event(STATE_Offline &, EVENT_APStart &);
    auto on_event(STATE_Offline &, EVENT_StaConnect &);
    //AP
    auto on_event(STATE_AP &, EVENT_APStart &);
    auto on_event(STATE_AP &, EVENT_APStop &);
    auto on_event(STATE_AP &, EVENT_StaConnect &);
    auto on_event(STATE_AP &, EVENT_UserConnected &);
    //COnnecting
    auto on_event(STATE_Connecting &, EVENT_LoseConnection &);
    auto on_event(STATE_Connecting &, EVENT_StaStart &);
    auto on_event(STATE_Connecting &, EVENT_StaConnected &);
    auto on_event(STATE_Connecting &, EVENT_GotIP &);
    // Connected
    auto on_event(STATE_Connected &, EVENT_LoseConnection &);
    auto on_event(STATE_Connected &, EVENT_GotIP &);
    auto on_event(STATE_Connected &, EVENT_Disconnect &);
    //Default
    template <typename State, typename Event> 
    auto on_event(State &, Event &) {
        printf("default event on wifi event handler\n");
        return std::nullopt;
    }

private:
    esp_netif_t *ap_netif;
    esp_netif_t *sta_netif;
    bool ap_start();
    void disconnect();
    bool sta_start();
    bool sta_connect();


    static constexpr char AP_SSID[] = DEVICE_NAME;
    static constexpr char AP_PASSWORD[] = "heihei22";

    SSID_T m_ssid{};
    PASSWORD_T m_password{};

    static void eventHandler(void *event_handler_arg,
                             esp_event_base_t event_base,
                             int32_t event_id,
                             void *event_data);
};


#endif
