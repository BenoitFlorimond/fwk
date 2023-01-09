/**
* ****************************************************************************
* hbridge_dir_hil.h
* ****************************************************************************
*/

#ifndef __HBRIDGE_DIR_HIL_H__
#define __HBRIDGE_DIR_HIL_H__

/* Documentation *************************************************************/

/* Includes ******************************************************************/

#include <esp_err.h>
#include <stdint.h>

/* Public Defines & Macros ***************************************************/

/* Public types definition ***************************************************/

/* Public prototypes *********************************************************/

void * HBDHIL_initIos(uint32_t pwmIo, uint32_t dirIo, uint32_t freq);

esp_err_t HBDHIL_setDutyCycle(void * handle, float dutyCycle);

#endif /* __HBRIDGE_DIR_HIL_H__ */