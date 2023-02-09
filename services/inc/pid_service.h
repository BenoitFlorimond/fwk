/**
* ****************************************************************************
* pid_service.h
* ****************************************************************************
*/

#ifndef __PID_SERVICE_H__
#define __PID_SERVICE_H__

/* Documentation *************************************************************/

/* Includes ******************************************************************/

#include <esp_err.h>

/* Public Defines & Macros ***************************************************/

/* Public types definition ***************************************************/

struct pidConfig_struct_t;
typedef struct pidConfig_struct_t* pidHandle_t;
typedef void (*pidSetOutputCbk_t)(pidHandle_t pid, double output);
typedef esp_err_t (*pidReadOutputCbk_t)(pidHandle_t pid, double * output);

/* Public prototypes *********************************************************/

esp_err_t PIDSVC_init(uint32_t samplingPeriodMs);

pidHandle_t PIDSVC_createPid(uint32_t decimation, double kp, double ki, double kd, double sp, pidSetOutputCbk_t setOutput, pidReadOutputCbk_t readOutput);

#endif /* __PID_SERVICE_H__ */