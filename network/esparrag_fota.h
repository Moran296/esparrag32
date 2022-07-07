#ifndef __ESPARRAG_FOTA__
#define __ESPARRAG_FOTA__

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/task.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"


class DirectOta
{
    static constexpr int UDP_PORT = 3232;
    static constexpr int UDP_OTA_PACKET_MIN_SIZE = 44;
    static constexpr int OTA_ADDRESS_BUFFER_SIZE = 128;
    static constexpr int DIRECT_OTA_TASK_SIZE = 4096;
    static constexpr int UDP_BUFFER_SIZE = 1024;
    static constexpr int TASK_PRIORITY = 5;
    static constexpr int TCP_OTA_BUFFER_SIZE = 1024;
    static constexpr char OTA_RESPONSE[] = "OK";
    static constexpr int TCP_RECEIVE_TIMEOUT = 30;

    public:
    DirectOta();
    void Init();
    void HandleEvents();

    private:
    enum eUdpBufferStructure
    {
        INDEX_COMMAND = 0,
        INDEX_PORT,
        INDEX_OTA_SIZE,
        INDEX_MD5,
        INDEX_NUM
    };

    int m_udpServerSocket;
    int m_tcpClientSocket;
    int m_tcpPort;
    bool m_otaFailed = false;
    uint32_t m_otaSize;
    TimerHandle_t m_otaWdt;
    StaticTimer_t m_otaWdtBuffer;

    char m_udpBuffer[UDP_BUFFER_SIZE]{};
    char m_otaAddressBuffer[OTA_ADDRESS_BUFFER_SIZE]{};
    uint8_t m_otaBuffer[TCP_OTA_BUFFER_SIZE]{};
    TaskHandle_t m_task;

    void handleOTA();
    bool initUdpServer();
    bool isOtaAddressValid() const;
    bool parseUdpMessage();
    bool initTcpSocket();
    void setUdpSocketTimeout();

    esp_ota_handle_t m_directOtaHandle{};
    int m_directOtaAccumulatedSize{};
    int m_directOtaSize{};
    const esp_partition_t *m_updatePartition{};

    bool fotaBegin(int otaSize);
    bool fotaWrite(const void *data, size_t size);
    bool fotaFinish();
    void fotaAbort();

    static void entryFunction(void* arg);

    static void otaWdtHandler(TimerHandle_t timer);
};


#endif