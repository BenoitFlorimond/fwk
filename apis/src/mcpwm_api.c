/**
 * ****************************************************************************
 * mcpwm_api.c
 * ****************************************************************************
 */

/* Documentation *************************************************************/

/* Includes ******************************************************************/

#include "mcpwm_api.h"

/* Private Defines & Macros **************************************************/

/* Private types definition **************************************************/

typedef struct {
    mcpwm_unit_t unit;
    mcpwm_io_signals_t signal;
    mcpwm_generator_t gen;
    bool used;
} outSignalCtxt_struct_t;

/* Private variables *********************************************************/

static uint32_t _timerFreq[MCPWM_TIMER_MAX]    = { 0 };
static outSignalCtxt_struct_t _outSignalCtxt[] = {
    { MCPWM_UNIT_0, MCPWM0A, MCPWM_GEN_A, false },
    { MCPWM_UNIT_0, MCPWM0B, MCPWM_GEN_B, false },
    { MCPWM_UNIT_0, MCPWM1A, MCPWM_GEN_A, false },
    { MCPWM_UNIT_0, MCPWM1B, MCPWM_GEN_B, false },
    { MCPWM_UNIT_0, MCPWM2A, MCPWM_GEN_A, false },
    { MCPWM_UNIT_0, MCPWM2B, MCPWM_GEN_B, false },
    { MCPWM_UNIT_1, MCPWM0A, MCPWM_GEN_A, false },
    { MCPWM_UNIT_1, MCPWM0B, MCPWM_GEN_B, false },
    { MCPWM_UNIT_1, MCPWM1A, MCPWM_GEN_A, false },
    { MCPWM_UNIT_1, MCPWM1B, MCPWM_GEN_B, false },
    { MCPWM_UNIT_1, MCPWM2A, MCPWM_GEN_A, false },
    { MCPWM_UNIT_1, MCPWM2B, MCPWM_GEN_B, false },
};

/* Private prototypes ********************************************************/

/* Private Functions *********************************************************/

/* Public Functions **********************************************************/

esp_err_t MCPWMAPI_getConfig(uint32_t gpio, uint32_t freq, mcpwmConf_struct_t* config)
{
    esp_err_t ret = ESP_OK;

    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    config->io = gpio;

    ret = ESP_FAIL;
    for (uint8_t signalIndex = 0; signalIndex < sizeof(_outSignalCtxt); signalIndex++) {
        if (!_outSignalCtxt[signalIndex].used) {
            config->signal = _outSignalCtxt[signalIndex].signal;
            config->unit   = _outSignalCtxt[signalIndex].unit;
            config->gen    = _outSignalCtxt[signalIndex].gen;
            ret            = ESP_OK;
            break;
        }
    }

    if (ret == ESP_FAIL) {
        return ret;
    }

    ret = ESP_FAIL;
    for (uint8_t timerIndex = 0; timerIndex < MCPWM_TIMER_MAX; timerIndex++) {
        if (_timerFreq[timerIndex] == 0 || _timerFreq[timerIndex] == freq) {
            _timerFreq[timerIndex] = freq;
            config->timer          = (mcpwm_timer_t)timerIndex;
            ret                    = ESP_OK;
            break;
        }
    }

    return ret;
}
