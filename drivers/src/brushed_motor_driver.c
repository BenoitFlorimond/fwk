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

brushedMotorHandle_t BMDRV_createMotor(brushedMotorInitCbk_t initCbk, brushedMotorSetDutyCycleCbk_t setDutyCycleCbk)
{
    brushedMotorConf_struct_t* conf = NULL;

    if (initCbk == NULL || setDutyCycleCbk == NULL) {
        return NULL;
    }

    conf = malloc(sizeof(brushedMotorConf_struct_t));
    if (conf != NULL) {
        conf->init         = initCbk;
        conf->setDutyCycle = setDutyCycleCbk;
        conf->hilHandle    = NULL;
    }
    return (brushedMotorHandle_t)conf;
}

esp_err_t BMDRV_initMotor(brushedMotorHandle_t motorHandle)
{
    brushedMotorConf_struct_t* conf = (brushedMotorConf_struct_t*)motorHandle;
    esp_err_t ret = ESP_OK;

    if ((conf->hilHandle = conf->init(motorHandle)) == NULL) {
        ret = ESP_FAIL;
    }

    return ret;
}

esp_err_t BMDRV_setSpeed(brushedMotorHandle_t motorHandle, float dutyCycle)
{
    brushedMotorConf_struct_t* conf = (brushedMotorConf_struct_t*)motorHandle;

    if (conf == NULL || conf->setDutyCycle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (dutyCycle < MIN_DUTY_CYCLE){
        dutyCycle = MIN_DUTY_CYCLE;
    }else if(dutyCycle > MAX_DUTY_CYCLE) {
        dutyCycle = MAX_DUTY_CYCLE;
    }

    return conf->setDutyCycle(conf->hilHandle, dutyCycle);
}