/**
 * ****************************************************************************
 * brushed_motor_driver.c
 * ****************************************************************************
 */

/* Documentation *************************************************************/

/* Includes ******************************************************************/

#include "brushed_motor_driver.h"
#include "freertos_includes.h"
#include "mcpwm_api.h"
#include <esp_log.h>
#include <string.h>

/* Private Defines & Macros **************************************************/

#define TASK_TAG            ("BM")
#define LOG_ERROR(fmt, ...) ESP_LOGE(TASK_TAG, fmt, ##__VA_ARGS__)
#define MIN_DUTY_CYCLE      (-100.0)
#define MAX_DUTY_CYCLE      (100.0)

/* Private types definition **************************************************/

typedef struct {
    brushedMotorInitCbk_t init;
    void* initArgs;
    brushedMotorSetDutyCycleCbk_t setDutyCycle;
    void* hilHandle;
} brushedMotorConf_struct_t;

/* Private variables *********************************************************/

/* Private prototypes ********************************************************/

/* Private Functions *********************************************************/

/* Public Functions **********************************************************/

brushedMotorHandle_t BMDRV_addMotor(uint32_t freq, brushedMotorInitCbk_t initCbk, void* initArgs, brushedMotorSetDutyCycleCbk_t setDutyCycleCbk)
{
    brushedMotorConf_struct_t* conf = NULL;
    void* hilHandle                 = NULL;

    if (initCbk == NULL || setDutyCycleCbk == NULL) {
        return NULL;
    }

    if ((hilHandle = initCbk(freq, initArgs)) == NULL) {
        return NULL;
    }

    conf = malloc(sizeof(brushedMotorConf_struct_t));
    if (conf != NULL) {
        conf->init         = initCbk;
        conf->setDutyCycle = setDutyCycleCbk;
        conf->hilHandle    = hilHandle;
    }
    return (brushedMotorHandle_t)conf;
}

esp_err_t BMDRV_setSpeed(brushedMotorHandle_t motorHandle, float dutyCycle)
{
    brushedMotorConf_struct_t* conf = (brushedMotorConf_struct_t*)motorHandle;

    if (conf == NULL || conf->setDutyCycle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (dutyCycle < MIN_DUTY_CYCLE || dutyCycle > MAX_DUTY_CYCLE) {
        LOG_ERROR("Duty cycle %0.2f is out of range [%0.2f; %0.2f]", dutyCycle, MIN_DUTY_CYCLE, MAX_DUTY_CYCLE);
        return ESP_ERR_INVALID_ARG;
    }

    return conf->setDutyCycle(conf->hilHandle, dutyCycle);
}