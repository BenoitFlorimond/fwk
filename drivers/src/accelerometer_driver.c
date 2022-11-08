/**
 * ****************************************************************************
 * accelerometer_driver.c
 * ****************************************************************************
 */

/* Documentation *************************************************************/

/* Includes ******************************************************************/

#include "accelerometer_driver.h"
#include "freertos_includes.h"
#include "os_utils.h"
#include <esp_log.h>
#include <string.h>

/* Private Defines & Macros **************************************************/

#define TAG_ACC               ("ACC")
#define TASK_STACK_SIZE       (2048)
#define TASK_PRIO             (4)
#define ACC_INFO(fmt, ...)    ESP_LOGI(TAG_ACC, fmt, ##__VA_ARGS__)
#define ACC_ERROR(fmt, ...)   ESP_LOGE(TAG_ACC, fmt, ##__VA_ARGS__)
#define ACC_WARNING(fmt, ...) ESP_LOGW(TAG_ACC, fmt, ##__VA_ARGS__)

/* Private types definition **************************************************/

typedef struct {
    acceleroApi_struct_t api;
    bool poll;
    uint32_t pollingPeriodMs;
    void (*cbk)(int16_t xAxis, int16_t yAxis, int16_t zAxis);
} driverConfig_struct_t;

typedef struct {
    int16_t xAxis;
    int16_t yAxis;
    int16_t zAxis;
} axisValue_struct_t;

/* Private variables *********************************************************/

static driverConfig_struct_t _config        = { 0 };
static QueueHandle_t _driverQueue           = NULL;
static SemaphoreHandle_t _driverMutex       = NULL;
static axisValue_struct_t _currentAxisValue = { 0 };

/* Private prototypes ********************************************************/

static void _process(void* priv);

/* Private Functions *********************************************************/

static void _process(void* priv)
{
    bool userRequest                 = false;
    queueCtxt_struct_t responseQueue = { 0 };
    axisValue_struct_t axisValue     = { 0 };
    uint32_t pollingPeriodTicks      = 0;

    OSUTILS_waitSystemStartup();

    pollingPeriodTicks = (_config.poll ? pdMS_TO_TICKS(_config.pollingPeriodMs) : portMAX_DELAY);

    for (;;) {

        userRequest = (xQueueReceive(_driverQueue, &responseQueue, pollingPeriodTicks) == pdTRUE);

        if (_config.api.readAxis != NULL) {
            _config.api.readAxis(&axisValue.xAxis, &axisValue.yAxis, &axisValue.zAxis);

            xSemaphoreTake(_driverMutex, portMAX_DELAY);
            memcpy(&_currentAxisValue, &axisValue, sizeof(axisValue_struct_t));
            xSemaphoreGive(_driverMutex);

            if (userRequest) {
                OSUTILS_queueSendSafe(&responseQueue, &axisValue);
            } else if (_config.cbk != NULL) {
                _config.cbk(axisValue.xAxis, axisValue.yAxis, axisValue.zAxis);
            }
        }
    }
}

/* Public Functions **********************************************************/

esp_err_t ACCDRV_init(acceleroApi_struct_t* conf, bool poll, uint32_t pollingPeriodMs, void (*cbk)(int16_t xAxis, int16_t yAxis, int16_t zAxis))
{
    esp_err_t ret = ESP_OK;

    if (xTaskCreate(_process, TAG_ACC, TASK_STACK_SIZE, NULL, TASK_PRIO, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    _driverQueue = xQueueCreate(1, sizeof(queueCtxt_struct_t));

    if (_driverQueue == NULL) {
        return ESP_ERR_NO_MEM;
    }

    _driverMutex = xSemaphoreCreateMutex();

    if (_driverMutex == NULL) {
        return ESP_ERR_NO_MEM;
    }

    memcpy(&_config.api, conf, sizeof(acceleroApi_struct_t));
    _config.poll            = poll;
    _config.pollingPeriodMs = pollingPeriodMs;
    _config.cbk             = cbk;

    if ((ret = _config.api.initCom()) != ESP_OK) {
        ACC_ERROR("Com init failed: %02X", ret);
        return ret;
    }

    if ((ret = _config.api.initDevice()) != ESP_OK) {
        ACC_ERROR("Device init failed: %02X", ret);
        return ret;
    }

    return ESP_OK;
}

void ACCDRV_getAxis(int16_t* xAxis, int16_t* yAxis, int16_t* zAxis)
{
    xSemaphoreTake(_driverMutex, portMAX_DELAY);
    *xAxis = _currentAxisValue.xAxis;
    *yAxis = _currentAxisValue.yAxis;
    *zAxis = _currentAxisValue.zAxis;
    xSemaphoreGive(_driverMutex);
}

esp_err_t ACCDRV_readAxisBl(int16_t* xAxis, int16_t* yAxis, int16_t* zAxis)
{
    queueCtxt_struct_t event     = { 0 };
    axisValue_struct_t axisValue = { 0 };

    if (!OSUTILS_sendAndWaitResponse(_driverQueue, &event, &event, &axisValue, sizeof(axisValue), TASK_DEFAULT_REPONSE_TIME_TICKS)) {
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}