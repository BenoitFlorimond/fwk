/**
 * ****************************************************************************
 * hbridge_dir_hil.c
 * ****************************************************************************
 */

/* Documentation *************************************************************/

/* Includes ******************************************************************/

#include "hbridge_dir_hil.h"
#include "mcpwm_api.h"
#include <driver/gpio.h>
#include <driver/mcpwm.h>
#include <esp_log.h>
#include <math.h>
#include <string.h>

/* Private Defines & Macros **************************************************/

#define LOG_TAG             ("H_BRIDGE")
#define LOG_ERROR(fmt, ...) ESP_LOGE(LOG_TAG, fmt, ##__VA_ARGS__)

/* Private types definition **************************************************/

typedef struct {
    mcpwmConf_struct_t pwmIoConf;
    uint32_t dirIo;
} hbridge_dir_conf_struct_t;

/* Private variables *********************************************************/

/* Private prototypes ********************************************************/

/* Private Functions *********************************************************/

/* Public Functions **********************************************************/

void* HBDHIL_initIos(uint32_t pwmIo, uint32_t dirIo, uint32_t freq)
{
    esp_err_t ret                   = ESP_OK;
    mcpwmConf_struct_t pwmIoConf    = { 0 };
    mcpwm_config_t pwmConfig        = { 0 };
    gpio_config_t gpioConfig        = { 0 };
    hbridge_dir_conf_struct_t* conf = NULL;

    if ((ret = MCPWMAPI_getConfig(pwmIo, freq, &pwmIoConf)) != ESP_OK) {
        LOG_ERROR("Unable to get config");
        return NULL;
    }
    if (mcpwm_gpio_init(pwmIoConf.unit, pwmIoConf.signal, pwmIo) != ESP_OK) {
        LOG_ERROR("mcpwm_gpio_init failed");
    }
    pwmConfig.frequency    = freq;
    pwmConfig.cmpr_a       = 0;
    pwmConfig.cmpr_b       = 0;
    pwmConfig.counter_mode = MCPWM_UP_COUNTER;
    pwmConfig.duty_mode    = MCPWM_DUTY_MODE_0;
    if (mcpwm_init(pwmIoConf.unit, pwmIoConf.timer, &pwmConfig) != ESP_OK) {
        LOG_ERROR("mcpwm_init failed for pwm signal init");
    }

    gpioConfig.pin_bit_mask = BIT64(dirIo);
    gpioConfig.mode         = GPIO_MODE_OUTPUT;
    gpioConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpioConfig.pull_up_en   = GPIO_PULLUP_DISABLE;
    gpioConfig.intr_type    = GPIO_INTR_DISABLE;
    if (gpio_config(&gpioConfig) != ESP_OK) {
        LOG_ERROR("gpio_config failed for dir pin init");
    }

    conf = malloc(sizeof(hbridge_dir_conf_struct_t));
    if (conf != NULL) {
        memcpy(&conf->pwmIoConf, &pwmConfig, sizeof(mcpwmConf_struct_t));
        conf->dirIo = dirIo;
    } else {
        LOG_ERROR("Malloc failed when trying to initialize h bridge settings");
    }

    return (void*)conf;
}

esp_err_t HBDHIL_setDutyCycle(void* handle, float dutyCycle)
{
    hbridge_dir_conf_struct_t* conf = (hbridge_dir_conf_struct_t*)handle;
    esp_err_t ret                   = ESP_OK;

    if (handle == NULL) {
        LOG_ERROR("Unknown handle (NULL)");
        return ESP_ERR_INVALID_ARG;
    }

    if ((ret = mcpwm_set_duty(conf->pwmIoConf.unit, conf->pwmIoConf.timer, conf->pwmIoConf.gen, fabs(dutyCycle))) != ESP_OK) {
        LOG_ERROR("Fail in HBDHIL_setDutyCycle() -> mcpwm_set_duty(%d, %d, %d, %f) returned %d", conf->pwmIoConf.unit, conf->pwmIoConf.timer, conf->pwmIoConf.gen, fabs(dutyCycle), ret);
        return ret;
    }

    return gpio_set_level(conf->dirIo, (dutyCycle > 0.0) ? 0 : 1);
}