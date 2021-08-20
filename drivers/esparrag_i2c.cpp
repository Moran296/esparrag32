
#include "esparrag_i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>
#include "esparrag_log.h"
#include "lock.h"

bool I2C::s_isInitialized = false;

I2C::I2C(uint8_t deviceAddress) : m_deviceAddress(deviceAddress)
{

    if (!s_isInitialized && init() != eResult::SUCCESS)
    {
        ESPARRAG_LOG_ERROR("failed to initiate i2c");
    }
}

eResult I2C::init()
{
    if (s_isInitialized)
    {
        return eResult::SUCCESS;
    }

    m_mutex = xSemaphoreCreateMutexStatic(&m_mutexBuffer);
    if (m_mutex == nullptr)
    {
        return eResult::ERROR_MEMORY;
    }

    i2c_port_t i2c_master_port = I2C_MASTER_PORT;
    i2c_config_t conf;
    memset(&conf, 0, sizeof(conf));
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = (gpio_num_t)I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    conf.scl_io_num = (gpio_num_t)I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;

    int res = i2c_param_config(i2c_master_port, &conf);
    if (res != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("i2c_param_config res: %d", res);
        return eResult::ERROR_INVALID_PARAMETER;
    }

    res = i2c_driver_install(i2c_master_port,
                             conf.mode,
                             I2C_MASTER_RX_BUF_DISABLE,
                             I2C_MASTER_TX_BUF_DISABLE,
                             0);
    if (res != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("i2c_driver_install res: %d", res);
        return eResult::ERROR_GENERAL;
    }

    s_isInitialized = true;
    return eResult::SUCCESS;
}

eResult I2C::switchRegister(uint8_t regAddr)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL)
    {
        ESPARRAG_LOG_ERROR("cmd create failed");
        return eResult::ERROR_MEMORY;
    }

    esp_err_t ret = i2c_master_start(cmd);
    if (ret != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("i2c_master_start failed, res: %d", ret);
        i2c_cmd_link_delete(cmd);
        return eResult::ERROR_CONNECTION_FAILURE;
    }

    uint8_t addressWritePermission = (m_deviceAddress << 1) | I2C_MASTER_WRITE;
    ret = i2c_master_write_byte(cmd, addressWritePermission, I2C::ACK_CHECK_EN);
    if (ret != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("i2c_master_write_byte failed, writeData: %u res: %d", addressWritePermission, ret);
        i2c_cmd_link_delete(cmd);
        return eResult::ERROR_GENERAL;
    }

    ret = i2c_master_write_byte(cmd, regAddr, I2C::ACK_CHECK_EN);
    if (ret != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("i2c_master_write_byte failed, regAddr %u res: %d", regAddr, ret);
        i2c_cmd_link_delete(cmd);
        return eResult::ERROR_GENERAL;
    }

    ret = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, pdMS_TO_TICKS(100));
    if (ret != ESP_OK)
    {

        ESPARRAG_LOG_ERROR("i2c_master_cmd_begin failed, write res: %d", ret);
        i2c_cmd_link_delete(cmd);
        return eResult::ERROR_GENERAL;
    }

    i2c_cmd_link_delete(cmd);

    return eResult::SUCCESS;
}

eResult I2C::read(uint8_t *data, uint16_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL)
    {
        ESPARRAG_LOG_ERROR("cmd create failed");
        return eResult::ERROR_MEMORY;
    }

    esp_err_t ret = i2c_master_start(cmd);
    if (ret != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("i2c_master_start failed, res: %d", ret);
        i2c_cmd_link_delete(cmd);
        return eResult::ERROR_GENERAL;
    }

    uint8_t addressReadPermission = (m_deviceAddress << 1) | I2C_MASTER_READ;
    ret = i2c_master_write_byte(cmd, addressReadPermission, I2C::ACK_CHECK_EN);
    if (ret != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("i2c_master_write_byte failed, addressReadPermission: %u res: %d", addressReadPermission, ret);
        i2c_cmd_link_delete(cmd);
        return eResult::ERROR_GENERAL;
    }

    if (len > 1)
    {
        ret = i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
        if (ret != ESP_OK)
        {
            ESPARRAG_LOG_ERROR("i2c_master_read failed, res: %d", ret);
            i2c_cmd_link_delete(cmd);
            return eResult::ERROR_GENERAL;
        }
    }

    ret = i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    if (ret != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("i2c_master_read_byte failed, res: %d", ret);
        i2c_cmd_link_delete(cmd);
        return eResult::ERROR_GENERAL;
    }

    ret = i2c_master_stop(cmd);
    if (ret != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("i2c_master_stop failed, res: %d", ret);
        i2c_cmd_link_delete(cmd);
        return eResult::ERROR_GENERAL;
    }

    ret = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, pdMS_TO_TICKS(100));
    if (ret != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("i2c_master_cmd_begin failed, res: %d", ret);
        i2c_cmd_link_delete(cmd);
        return eResult::ERROR_GENERAL;
    }

    i2c_cmd_link_delete(cmd);

    return eResult::SUCCESS;
}

eResult I2C::Write(uint8_t *data, uint16_t len)
{
    esp_err_t ret = 0;
    Lock lock(m_mutex);

    if (!s_isInitialized)
    {
        ESPARRAG_LOG_ERROR("I2C is not initialized");
        return eResult::ERROR_INVALID_STATE;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL)
    {
        ESPARRAG_LOG_ERROR("cmd create failed");
        return eResult::ERROR_MEMORY;
    }

    ret = i2c_master_start(cmd);
    if (ret != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("i2c_master_start failed, res: %d", ret);
        i2c_cmd_link_delete(cmd);
        return eResult::ERROR_CONNECTION_FAILURE;
    }

    ret = i2c_master_write(cmd, data, len, ACK_CHECK_EN);
    if (ret != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("i2c_master_write failed, res: %d", ret);
        i2c_cmd_link_delete(cmd);
        return eResult::ERROR_CONNECTION_FAILURE;
    }

    ret = i2c_master_stop(cmd);
    if (ret != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("i2c_master_stop failed, res: %d", ret);
        i2c_cmd_link_delete(cmd);
        return eResult::ERROR_GENERAL;
    }

    ret = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, pdMS_TO_TICKS(100));
    if (ret != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("i2c_master_cmd_begin failed, res: %d", ret);
        i2c_cmd_link_delete(cmd);
        return eResult::ERROR_GENERAL;
    }

    i2c_cmd_link_delete(cmd);

    return eResult::SUCCESS;
}

eResult I2C::Write(uint8_t regAddr, uint8_t *data, uint16_t len)
{
    Lock lock(m_mutex);
    esp_err_t ret = 0;

    if (!s_isInitialized)
    {
        ESPARRAG_LOG_ERROR("I2C is not initialized");
        return eResult::ERROR_INVALID_STATE;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL)
    {
        ESPARRAG_LOG_ERROR("cmd create failed");
        return eResult::ERROR_MEMORY;
    }

    ret = i2c_master_start(cmd);
    if (ret != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("i2c_master_start failed, res: %d", ret);
        i2c_cmd_link_delete(cmd);
        return eResult::ERROR_CONNECTION_FAILURE;
    }

    uint8_t addressWritePermission = (m_deviceAddress << 1) | I2C_MASTER_WRITE;
    ret = i2c_master_write_byte(cmd, addressWritePermission, ACK_CHECK_EN);
    if (ret != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("i2c_master_write_byte failed, res: %d", ret);
        i2c_cmd_link_delete(cmd);
        return eResult::ERROR_CONNECTION_FAILURE;
    }

    ret = i2c_master_write_byte(cmd, regAddr, ACK_CHECK_EN);
    if (ret != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("i2c_master_write_byte failed, res: %d", ret);
        i2c_cmd_link_delete(cmd);
        return eResult::ERROR_CONNECTION_FAILURE;
    }

    ret = i2c_master_write(cmd, data, len, ACK_CHECK_EN);
    if (ret != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("i2c_master_write failed, res: %d", ret);
        i2c_cmd_link_delete(cmd);
        return eResult::ERROR_CONNECTION_FAILURE;
    }

    ret = i2c_master_stop(cmd);
    if (ret != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("i2c_master_stop failed, res: %d", ret);
        i2c_cmd_link_delete(cmd);
        return eResult::ERROR_GENERAL;
    }

    ret = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, pdMS_TO_TICKS(100));
    if (ret != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("i2c_master_cmd_begin failed, res: %d", ret);
        i2c_cmd_link_delete(cmd);
        return eResult::ERROR_GENERAL;
    }

    i2c_cmd_link_delete(cmd);

    return eResult::SUCCESS;
}

eResult I2C::Read(uint8_t *data, uint16_t len)
{
    Lock lock(m_mutex);

    if (!s_isInitialized)
    {
        ESPARRAG_LOG_ERROR("I2C is not initialized");
        return eResult::ERROR_INVALID_STATE;
    }

    return read(data, len);
}

eResult I2C::Read(uint8_t regAddr, uint8_t *data, uint16_t len)
{
    Lock lock(m_mutex);

    if (!s_isInitialized)
    {
        ESPARRAG_LOG_ERROR("I2C is not initialized");
        return eResult::ERROR_INVALID_STATE;
    }

    eResult res = switchRegister(regAddr);
    if (res != eResult::SUCCESS)
        return res;

    return read(data, len);
}
