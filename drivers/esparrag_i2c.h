#ifndef ESPARRAG_I2C_H__
#define ESPARRAG_I2C_H__

#include "esparrag_common.h"
#include "driver/i2c.h"

class I2C
{
    static constexpr int I2C_MASTER_SCL_IO = 22;
    static constexpr int I2C_MASTER_SDA_IO = 21;
    static constexpr int I2C_MASTER_PORT = I2C_NUM_0;
    static constexpr int I2C_MASTER_FREQ_HZ = 110000;
    static constexpr int I2C_MASTER_TX_BUF_DISABLE = 0;
    static constexpr int I2C_MASTER_RX_BUF_DISABLE = 0;

public:
    I2C(uint8_t deviceAddress);

    eResult Write(uint8_t regAddr, uint8_t *data, uint16_t len);
    eResult Write(uint8_t *data, uint16_t len);
    eResult Read(uint8_t regAddr, uint8_t *data, uint16_t len);
    eResult Read(uint8_t *data, uint16_t len);
    uint8_t printAddress() { return m_deviceAddress; }

private:
    static bool s_isInitialized;
    static constexpr bool ACK_CHECK_EN = true;
    static constexpr bool ACK_CHECK_DIS = false;
    static SemaphoreHandle_t m_mutex;
    static StaticSemaphore_t m_mutexBuffer;

    I2C(const I2C &) = delete;
    I2C &operator=(const I2C &) = delete;

    static eResult init();
    eResult switchRegister(uint8_t regAddr);
    eResult read(uint8_t *data, uint16_t len);

    uint8_t m_deviceAddress;
};

#endif
