/**
 * ****************************************************************************
 * mcpwm_api.h
 * ****************************************************************************
 */

#ifndef __MCPWM_API_H__
#define __MCPWM_API_H__

/* Documentation *************************************************************/

/* Includes ******************************************************************/

#include "mcpwm.h"
#include <esp_err.h>

/* Public Defines & Macros ***************************************************/

/* Public types definition ***************************************************/

typedef struct {
    uint32_t io;
    mcpwm_unit_t unit;
    mcpwm_timer_t timer;
    mcpwm_generator_t gen;
    mcpwm_io_signals_t signal;
} mcpwmConf_struct_t;

/* Public prototypes *********************************************************/

esp_err_t MCPWMAPI_getConfig(uint32_t gpio, uint32_t freq, mcpwmConf_struct_t* config);

#endif /* __MCPWM_API_H__ */