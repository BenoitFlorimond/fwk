/**
 * ****************************************************************************
 * pid_service.c
 * ****************************************************************************
 */

/* Documentation *************************************************************/

/* Includes ******************************************************************/

#include "pid_service.h"
#include "freertos_includes.h"
#include "os_utils.h"
#include <esp_log.h>
#include <string.h>

/* Private Defines & Macros **************************************************/

#define TASK_TAG              ("PID")
#define TASK_STACK_SIZE       (2048)
#define TASK_PRIO             (4)
#define PIDS_MAX_NUMBER       (10)
#define PID_INFO(fmt, ...)    ESP_LOGI(TASK_TAG, fmt, ##__VA_ARGS__)
#define PID_ERROR(fmt, ...)   ESP_LOGE(TASK_TAG, fmt, ##__VA_ARGS__)
#define PID_WARNING(fmt, ...) ESP_LOGW(TASK_TAG, fmt, ##__VA_ARGS__)
#define PID_DEBUG(fmt, ...)   ESP_LOGD(TASK_TAG, fmt, ##__VA_ARGS__)

/* Private types definition **************************************************/

typedef struct {
    uint8_t id;
    uint32_t decimation;
    uint32_t decimationCounter;
    double kp;
    double ki;
    double kd;
    double sp;
    double err;
    double errSum;
    pidReadOutputCbk measureOutputCbk;
    pidSetOutputCbk setOutputCbk;
} pidConfig_struct_t;

/* Private variables *********************************************************/

static TaskHandle_t _taskHandle                       = NULL;
static pidConfig_struct_t* _pidsConf[PIDS_MAX_NUMBER] = { 0 };
static uint8_t _pidNumber                             = 0;
static uint32_t _samplingPeriodMs                     = 0;

/* Private prototypes ********************************************************/

static void _process(void* priv);
static void _runPid(pidConfig_struct_t* pid);

/* Private Functions *********************************************************/

static void _process(void* priv)
{
    TickType_t lastWakeTime;

    OSUTILS_waitSystemStartup();

    lastWakeTime = xTaskGetTickCount();

    for (;;) {
        for (uint8_t pidIndex = 0; pidIndex < _pidNumber; pidIndex++) {
            if (_pidsConf[pidIndex] == NULL) {
                break;
            }
            _runPid(_pidsConf[pidIndex]);
        }

        xTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(_samplingPeriodMs));
    }
}

static void _runPid(pidConfig_struct_t* pid)
{
    double measure = 0.0;
    double output  = 0.0;
    double lastErr = 0.0;

    if (pid == NULL || pid->measureOutputCbk == NULL || pid->setOutputCbk == NULL) {
        return;
    }

    if (pid->measureOutputCbk(&measure) != ESP_OK) {
        return;
    }

    lastErr  = pid->err;
    pid->err = measure - pid->sp;
    pid->errSum += pid->err;
    output = pid->kp * pid->err + pid->ki * pid->errSum * _samplingPeriodMs * pid->decimation + pid->kd * (pid->err - lastErr);

    PID_DEBUG("ID%d: sp=%0.2f, measure=%0.2f, err=%0.2f, err_sum=%0.2f, out=%0.2f", pid->id, pid->sp, measure, pid->err, pid->errSum, output);

    pid->setOutputCbk(output);
}

/* Public Functions **********************************************************/

esp_err_t PIDSVC_init(uint32_t samplingPeriodMs)
{
    if (samplingPeriodMs == 0) {
        PID_ERROR("PID init failed because sampling period is 0");
        return ESP_ERR_INVALID_ARG;
    }

    if (_taskHandle != NULL) {
        PID_WARNING("PID init returns because process is already running with sampling period %dms", _samplingPeriodMs);
        return ESP_OK;
    }

    _samplingPeriodMs = samplingPeriodMs;

    if (xTaskCreate(_process, TASK_TAG, TASK_STACK_SIZE, NULL, TASK_PRIO, &_taskHandle) != pdPASS) {
        PID_ERROR("PID init failed because we are out of memory (task not created)");
        return ESP_ERR_NO_MEM;
    }

    PID_INFO("PID service correctly initialized with sampling period %dms", _samplingPeriodMs);

    return ESP_OK;
}

esp_err_t PIDSVC_addPid(uint32_t decimation, double kp, double ki, double kd, double sp, pidSetOutputCbk setOutput, pidReadOutputCbk readOutput)
{
    pidConfig_struct_t* conf = NULL;

    if (setOutput == NULL || readOutput == NULL) {
        PID_ERROR("Could not add PID because one callback is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    conf = malloc(sizeof(pidConfig_struct_t));
    if (conf == NULL) {
        PID_ERROR("Could not add PID because we are out of memory");
        return ESP_ERR_NO_MEM;
    }

    memset(conf, 0, sizeof(pidConfig_struct_t));
    conf->id               = _pidNumber++;
    conf->decimation       = decimation;
    conf->kp               = kp;
    conf->ki               = ki;
    conf->kd               = kd;
    conf->sp               = sp;
    conf->setOutputCbk     = setOutput;
    conf->measureOutputCbk = readOutput;

    PID_INFO("PID correctly added: id %d (kp=%0.2f, ki=%0.2f, kd=%0.2f, dec=%d, sp=%0.2f)",
        conf->id,
        conf->kp,
        conf->ki,
        conf->kd,
        conf->decimation,
        conf->sp);

    _pidsConf[conf->id] = conf;

    return ESP_OK;
}