/**
 * ****************************************************************************
 * lis2dw12_hil.c
 * ****************************************************************************
 */

/* Documentation *************************************************************/

/* Includes ******************************************************************/

#include "lis2dw12_hil.h"
#include "driver/i2c.h"
#include "freertos_includes.h"

/* Private Defines & Macros **************************************************/

#define ACC_I2C_ADDR          (0x18)
#define I2C_MASTER_TIMEOUT_MS (100)
/* Registers */
#define REG_WHO_AM_I_ADDR     (0x0F)
#define REG_WHO_AM_I_VALUE    (0x44)
#define REG_CTRL1_ADDR        (0x20)
#define REG_CTRL1_VALUE       (0x53)
#define REG_CTRL2_ADDR        (0x21)
#define REG_CTRL2_VALUE       (0x04)
#define REG_CTRL3_ADDR        (0x22)
#define REG_CRTL3456_VALUE    (0x00)
#define REG_AXIS_X_ADDR       (0x28)
#define REG_AXIS_Y_ADDR       (0x2A)
#define REG_AXIS_Z_ADDR       (0x2C)
/* Device specific */
#define RAW_TO_MG(axis)       ((int16_t)( axis ) / 16)
/* Com related defines */
#define MAX_BUFFER_SIZE       (11)

/* Private types definition **************************************************/

/* Private variables *********************************************************/

static i2c_port_t _i2cPort = 0xFF;

/* Private prototypes ********************************************************/

static esp_err_t _write(uint8_t reg, uint8_t* buffer, size_t length);
static esp_err_t _read(uint8_t reg, uint8_t* buffer, size_t length);

/* Private Functions *********************************************************/

static esp_err_t _write(uint8_t reg, uint8_t* buffer, size_t length)
{
    uint8_t buf[MAX_BUFFER_SIZE] = { 0 };

    if (_i2cPort == 0xFF) {
        return ESP_ERR_INVALID_ARG;
    }

    if (length > (MAX_BUFFER_SIZE - 1)) {
        return ESP_ERR_INVALID_ARG;
    }

    buf[0] = reg;
    memcpy(&buf[1], buffer, length);

    return i2c_master_write_to_device(_i2cPort, ACC_I2C_ADDR, buf, length + 1, I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
}

static esp_err_t _read(uint8_t reg, uint8_t* buffer, size_t length)
{
    if (_i2cPort == 0xFF) {
        return ESP_ERR_INVALID_ARG;
    }

    return i2c_master_write_read_device(_i2cPort, ACC_I2C_ADDR, &reg, 1, buffer, length, I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
}

/* Public Functions **********************************************************/

esp_err_t lis2dw12_initCom(int sdaIoNum, int sclIoNum, i2c_port_t i2cPort, int i2cFreqHz)
{
    esp_err_t ret     = ESP_OK;
    uint8_t data      = 0;

    i2c_config_t conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = sdaIoNum,
        .scl_io_num       = sclIoNum,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = i2cFreqHz,
    };

    if ((ret = i2c_param_config(i2cPort, &conf)) != ESP_OK) {
        return ret;
    }

    if ((ret = i2c_driver_install(i2cPort, conf.mode, 0, 0, 0)) != ESP_OK) {
        return ret;
    }

    _i2cPort = i2cPort;

    if ((ret = _read(REG_WHO_AM_I_ADDR, &data, 1)) != ESP_OK) {
        return ret;
    }

    if (data != REG_WHO_AM_I_VALUE) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_OK;
}

esp_err_t lis2dw12_initDevice(void)
{
    uint8_t regData[4] = { 0 };
    esp_err_t ret      = ESP_OK;

    regData[0]         = REG_CTRL1_VALUE;

    if ((ret = _write(REG_CTRL1_ADDR, regData, 1)) != ESP_OK) {
        return ret;
    }

    regData[0] = REG_CTRL2_VALUE;

    if ((ret = _write(REG_CTRL2_ADDR, regData, 1)) != ESP_OK) {
        return ret;
    }

    memset(regData, REG_CRTL3456_VALUE, 4);

    if (((ret = _write(REG_CTRL3_ADDR, regData, 4)) != ESP_OK)) {
        return ret;
    }

    return ESP_OK;
}

esp_err_t lis2dw12_readAxis(int16_t* xAxis, int16_t* yAxis, int16_t* zAxis)
{
    uint8_t regData[2] = { 0 };
    esp_err_t ret      = ESP_OK;

    if (xAxis != NULL) {
        if ((ret = _read(REG_AXIS_X_ADDR, regData, 2)) != ESP_OK) {
            return ret;
        }
        *xAxis = RAW_TO_MG((regData[1] << 8) | regData[0]);
    }

    if (yAxis != NULL) {
        if ((ret = _read(REG_AXIS_Y_ADDR, regData, 2)) != ESP_OK) {
            return ret;
        }
        *yAxis = RAW_TO_MG((regData[1] << 8) | regData[0]);
    }

    if (zAxis != NULL) {
        if ((ret = _read(REG_AXIS_Z_ADDR, regData, 2)) != ESP_OK) {
            return ret;
        }
        *zAxis = RAW_TO_MG((regData[1] << 8) | regData[0]);
    }

    return ESP_OK;
}
