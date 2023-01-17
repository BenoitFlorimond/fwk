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

typedef void (*pidSetOutputCbk)(double output);
typedef esp_err_t (*pidReadOutputCbk)(double * output);

/* Public prototypes *********************************************************/

esp_err_t PIDSVC_init(uint32_t samplingPeriodMs);

esp_err_t PIDSVC_addPid(uint32_t decimation, double kp, double ki, double kd, double sp, pidSetOutputCbk setOutput, pidReadOutputCbk readOutput);

#endif /* __PID_SERVICE_H__ */