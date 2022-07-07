#include "esparrag_fota.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <arpa/inet.h>
#include <lwip/netdb.h>

#include "esparrag_log.h"
#include "esparrag_common.h"
#include "esparrag_time_units.h"


DirectOta::DirectOta() {}

void DirectOta::Init() {
    initUdpServer();

    m_otaWdt = xTimerCreateStatic("ota_wdt", Seconds(TCP_RECEIVE_TIMEOUT).toTicks(), pdFALSE, this, otaWdtHandler, &m_otaWdtBuffer);
    ESPARRAG_ASSERT(m_otaWdt != nullptr);

    xTaskCreatePinnedToCore(entryFunction, "DIRECT_OTA", DIRECT_OTA_TASK_SIZE, this, TASK_PRIORITY, &m_task, 0);
    ESPARRAG_ASSERT(m_task != nullptr);
}

void DirectOta::entryFunction(void *pvParameters) {
    DirectOta *directOta = (DirectOta *)pvParameters;
    ESPARRAG_LOG_INFO("Waiting for ota");
    for(;;) {
        directOta->HandleEvents();
    }
}

void DirectOta::HandleEvents() {

    struct sockaddr_storage source_addr;
    socklen_t socklen = sizeof(source_addr);

    int len = recvfrom(m_udpServerSocket, m_udpBuffer, UDP_BUFFER_SIZE - 1, 0, (struct sockaddr *)&source_addr, &socklen);
    if (len < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // socket timeout
            return;
        } else {
            ESPARRAG_LOG_ERROR("udp socket error: %s", strerror(errno));
            return;
        }
    } else {
        m_udpBuffer[len] = '\0';
        inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, m_otaAddressBuffer, OTA_ADDRESS_BUFFER_SIZE - 1);
        ESPARRAG_LOG_INFO("from: %s", m_otaAddressBuffer);
        ESPARRAG_LOG_INFO("received: %s", m_udpBuffer);
        if (strlen(m_udpBuffer) < UDP_OTA_PACKET_MIN_SIZE || !isOtaAddressValid()) {
            ESPARRAG_LOG_ERROR("invalid message. len = %d", strlen(m_udpBuffer));
            return;
        }

        if (!parseUdpMessage()) {
            ESPARRAG_LOG_ERROR("udp message parse failed");
            return;
        }

        int err = sendto(m_udpServerSocket, OTA_RESPONSE, sizeof(OTA_RESPONSE), 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
        if (err < 0) {
            ESPARRAG_LOG_ERROR("send udp response failed, errno %d", errno);
            return;
        }

        handleOTA();
    }
}

bool DirectOta::parseUdpMessage() {
    char *token = strtok(m_udpBuffer, " ");
    for (size_t i = 0; i < INDEX_NUM; i++) {

        switch (i) {
        case INDEX_COMMAND:
            if (strlen(token) != 1) {
                ESPARRAG_LOG_ERROR("command invalid");
                return false;
            }
            break;
        case INDEX_PORT:
            m_tcpPort = atoi(token);
            ESPARRAG_LOG_INFO("parsed port = %d", m_tcpPort);
            if (m_tcpPort < 80) {
                ESPARRAG_LOG_ERROR("invalid port");
                return false;
            }
            break;
        case INDEX_OTA_SIZE:
            m_otaSize = atoi(token);
            ESPARRAG_LOG_INFO("parsed ota size = %d", m_otaSize);
            if (m_tcpPort < 1000) {
                ESPARRAG_LOG_ERROR("invalid ota size");
                return false;
            }
            break;
        case INDEX_MD5:
            return true;
        default:
            ESPARRAG_ASSERT(false);
        }

        token = strtok(nullptr, " ");
    }

    return true;
}

bool DirectOta::initTcpSocket() {
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(m_otaAddressBuffer);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(m_tcpPort);

    m_tcpClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (m_tcpClientSocket < 0) {
        ESPARRAG_LOG_ERROR("Unable to create tcp socket: errno %d", errno);
        return false;
    }
    int err = connect(m_tcpClientSocket, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in));
    if (err != 0) {
        ESPARRAG_LOG_ERROR("Socket unable to connect: errno %d", errno);
        return false;
    }

    ESPARRAG_LOG_INFO("direct ota successfully connected to tcp server %s:%d", m_otaAddressBuffer, m_tcpPort);
    return true;
}

void DirectOta::handleOTA() {
    static constexpr int RESPONSE_BUFFER_SIZE = 20;
    static constexpr int PRINT_READ_INTERVAL = 10;
    static constexpr int NUM_OF_CONSECUTIVE_ITERATIONS = 40;
    static constexpr int WATCHDOG_FEED_TIME = 1;

    char responseBuffer[RESPONSE_BUFFER_SIZE]{};
    int readNum = 0;
    m_otaFailed = false;
    bool res{};
    uint16_t i{};

    res = initTcpSocket();
    if (!res) {
        ESPARRAG_LOG_ERROR("tcp socket init failed");
        return;
    }

    res = fotaBegin(m_otaSize);
    if (!res) {
        ESPARRAG_LOG_ERROR("Direct ota begin failed");
        return;
    }

    ESPARRAG_ASSERT(xTimerStart(m_otaWdt, Seconds(10).toTicks()));
    int bytesReceived = 0;
    while (bytesReceived < m_otaSize) {
        int sizeLeft = m_otaSize - bytesReceived;
        int len = recv(m_tcpClientSocket, m_otaBuffer, std::min(TCP_OTA_BUFFER_SIZE, sizeLeft), 0);
        if (len <= 0 || m_otaFailed) {
            m_otaFailed = true;
            ESPARRAG_LOG_ERROR("recv failed. errno = %d", errno);
            break;
        }

        snprintf(responseBuffer, RESPONSE_BUFFER_SIZE, "%u", len);
        int err = send(m_tcpClientSocket, responseBuffer, strlen(responseBuffer), 0);
        if (err < 0) {
            m_otaFailed = true;
            ESPARRAG_LOG_ERROR("Error during sending size: errno %d", errno);
            break;
        }

        res = fotaWrite(m_otaBuffer, len);
        if (!res) {
            m_otaFailed = true;
            ESPARRAG_LOG_ERROR("direct ota write failed");
            break;
        }

        readNum++;
        bytesReceived += len;
        if (readNum % PRINT_READ_INTERVAL == 0)
            ESPARRAG_LOG_INFO("ota bytes recieved: %u", bytesReceived);

        ESPARRAG_ASSERT(xTimerReset(m_otaWdt, Seconds(10).toTicks()));

        // Give idle task some time to avoid watchdog timer from throwing an assert
        if (i++ % NUM_OF_CONSECUTIVE_ITERATIONS == 0) {
            vTaskDelay(MilliSeconds(WATCHDOG_FEED_TIME).toTicks());
        }
    }

    ESPARRAG_ASSERT(xTimerStop(m_otaWdt, Seconds(10).toTicks()));
    if (m_otaFailed) {
        close(m_tcpClientSocket);
        fotaAbort();
        return;
    }

    res = fotaFinish();
    if (!res) {
        fotaAbort();
        close(m_tcpClientSocket);
        ESPARRAG_LOG_ERROR("direct ota finish failed");
        return;
    }

    int err = send(m_tcpClientSocket, OTA_RESPONSE, sizeof(OTA_RESPONSE), 0);
    if (err < 0) {
        ESPARRAG_LOG_ERROR("Error during sending size: errno %d", errno);
    }

    close(m_tcpClientSocket);
    close(m_udpServerSocket);
    ESPARRAG_LOG_INFO("OTA downloaded successfully, restarting...");

    vTaskDelay(Seconds(5).toTicks());
    esp_restart();

    vTaskSuspend(NULL);
    return;
}

void DirectOta::setUdpSocketTimeout() {
    Seconds timeout(30);
    struct timeval tv;
    tv.tv_sec = timeout.value();
    tv.tv_usec = 0;
    int err = setsockopt(m_udpServerSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if (err < 0) {
        ESPARRAG_LOG_ERROR("Error during setting socket timeout: errno %d", errno);
        ESPARRAG_ASSERT(err == 0);
    }
}

bool DirectOta::initUdpServer() {

    struct sockaddr_in dest_addr;

    struct sockaddr_in *dest_addr_ip4 = &dest_addr;
    dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr_ip4->sin_family = AF_INET;
    dest_addr_ip4->sin_port = htons(UDP_PORT);
    m_udpServerSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (m_udpServerSocket < 0) {
        ESPARRAG_LOG_ERROR("Unable to create udp socket: errno %d", errno);
        return false;
    }

    setUdpSocketTimeout();

    int err = bind(m_udpServerSocket, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0) {
        ESPARRAG_LOG_ERROR("Socket unable to bind: errno %d", errno);
        return false;
    }

    return true;
}

bool DirectOta::isOtaAddressValid() const {
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, m_otaAddressBuffer, &(sa.sin_addr));
    return result != 0;
}

void DirectOta::otaWdtHandler(TimerHandle_t timer) {
    DirectOta *directOta = (DirectOta *)pvTimerGetTimerID(timer);
    ESPARRAG_LOG_ERROR("direct ota watchdog timeout");
    directOta->m_otaFailed = true;
    close(directOta->m_tcpClientSocket);
}

// ++++++++++++++++++++++++++OTA HANDLERS +++++++++++++++++++++++++++++++

bool DirectOta::fotaBegin(int otaSize) {
    m_directOtaAccumulatedSize = 0;
    m_directOtaSize = 0;

    m_updatePartition = esp_ota_get_next_update_partition(nullptr);
    ESPARRAG_ASSERT(m_updatePartition != nullptr);
    esp_err_t err = esp_ota_begin(m_updatePartition, otaSize, &m_directOtaHandle);
    if (err != ESP_OK || m_directOtaHandle == 0) {
        ESPARRAG_LOG_ERROR("esp_https_ota_begin failed, code %d", err);
        return false;
    }

    m_directOtaSize = otaSize;
    return true;
}

bool DirectOta::fotaWrite(const void *data, size_t size) {
    esp_err_t err = ESP_OK;

    ESPARRAG_ASSERT(m_directOtaHandle != 0);
    ESPARRAG_ASSERT(m_updatePartition != nullptr);
    m_directOtaAccumulatedSize += size;
    if (m_directOtaAccumulatedSize > m_directOtaSize) {
        ESPARRAG_LOG_ERROR("direct fota size exceeded total size");
        return false;
    }

    err = esp_ota_write(m_directOtaHandle, data, size);
    if (err != ESP_OK) {
        ESPARRAG_LOG_ERROR("fota write failed, err %d", err);
        return false;
    }

    return true;
}

bool DirectOta::fotaFinish() {
    esp_err_t err = ESP_OK;

    ESPARRAG_ASSERT(m_directOtaHandle != 0);
    ESPARRAG_ASSERT(m_updatePartition != nullptr);
    if (m_directOtaAccumulatedSize != m_directOtaSize) {
        ESPARRAG_LOG_ERROR("direct fota size inconsistent");
        return false;
    }

    err = esp_ota_end(m_directOtaHandle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESPARRAG_LOG_ERROR("Image validation failed, image is corrupted");
            return false;
        }

        ESPARRAG_LOG_ERROR("esp_ota_end failed. error = %d", err);
        return false;
    }

    err = esp_ota_set_boot_partition(m_updatePartition);
    if (err != ESP_OK) {
        ESPARRAG_LOG_ERROR("esp_ota_set_boot_partition failed. error = %d", err);
        return false;
    }


    return true;
}

void DirectOta::fotaAbort() {
    ESPARRAG_ASSERT(m_directOtaHandle != 0);
    ESPARRAG_ASSERT(m_updatePartition != nullptr);

    esp_err_t err = esp_ota_abort(m_directOtaHandle);
    if (err != ESP_OK) {
        ESPARRAG_LOG_ERROR("failed to abort fota, err %d", err);
    }

    m_directOtaHandle = 0;
    m_updatePartition = nullptr;
}