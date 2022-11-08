/**
* ****************************************************************************
* lis2dw12_hil.h
* ****************************************************************************
*/

#ifndef __LIS2DW12_HIL_H__
#define __LIS2DW12_HIL_H__

/* Documentation *************************************************************/

/* Includes ******************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <esp_err.h>
#include <hal/i2c_types.h>

/* Public Defines & Macros ***************************************************/

/* Public types definition ***************************************************/

/* Public prototypes *********************************************************/

esp_err_t lis2dw12_initCom(int sdaIoNum, int sclIoNum, i2c_port_t i2cPort, int i2cFreqHz);

esp_err_t lis2dw12_initDevice(void);

esp_err_t lis2dw12_readAxis(int16_t * xAxis, int16_t * yAxis, int16_t * zAxis);


#endif /* __LIS2DW12_HIL_H__ */