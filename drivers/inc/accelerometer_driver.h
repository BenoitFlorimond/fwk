/**
* ****************************************************************************
* accelerometer_driver.h
* ****************************************************************************
*/

#ifndef __ACCELEROMETER_DRIVER_H__
#define __ACCELEROMETER_DRIVER_H__

/* Documentation *************************************************************/

/* Includes ******************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

/* Public Defines & Macros ***************************************************/

/* Public types definition ***************************************************/

typedef struct{
    esp_err_t (*initCom)(void);
    esp_err_t (*initDevice)(void);
    esp_err_t (*readAxis)(int16_t *xAxis, int16_t *yAxis, int16_t *zAxis);
}acceleroApi_struct_t;

/* Public prototypes *********************************************************/

esp_err_t ACCDRV_init(acceleroApi_struct_t * conf, bool poll, uint32_t pollingPeriodMs, void (*cbk)(int16_t xAxis, int16_t yAxis, int16_t zAxis));

void ACCDRV_getAxis(int16_t *xAxis, int16_t *yAxis, int16_t *zAxis);

esp_err_t ACCDRV_readAxisBl(int16_t *xAxis, int16_t *yAxis, int16_t *zAxis);


#endif /* __ACCELEROMETER_DRIVER_H__ */