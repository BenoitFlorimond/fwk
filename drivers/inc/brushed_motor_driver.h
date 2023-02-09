/**
 * ****************************************************************************
 * brushed_motor_driver.h
 * ****************************************************************************
 */

#ifndef __BRUSHED_MOTOR_DRIVER_H__
#define __BRUSHED_MOTOR_DRIVER_H__

/* Documentation *************************************************************/

/* Includes ******************************************************************/

#include <esp_err.h>

/* Public Defines & Macros ***************************************************/

/* Public types definition ***************************************************/

struct brushedMotorConf_struct_t;
typedef struct brushedMotorConf_struct_t* brushedMotorHandle_t;
typedef void* (*brushedMotorInitCbk_t)(brushedMotorHandle_t handle);
typedef esp_err_t (*brushedMotorSetDutyCycleCbk_t)(void* hilHandle, float dutyCycle);

/* Public prototypes *********************************************************/

brushedMotorHandle_t BMDRV_createMotor(brushedMotorInitCbk_t initCbk, brushedMotorSetDutyCycleCbk_t setDutyCycleCbk);

esp_err_t BMDRV_initMotor(brushedMotorHandle_t motorHandle);

esp_err_t BMDRV_setSpeed(brushedMotorHandle_t motorHandle, float dutyCycle);

#endif /* __BRUSHED_MOTOR_DRIVER_H__ */