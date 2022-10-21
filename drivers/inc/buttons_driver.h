/**
* ****************************************************************************
* buttons_driver.h
* ****************************************************************************
*/

#ifndef __BUTTONS_DRIVER_H__
#define __BUTTONS_DRIVER_H__

/* Documentation *************************************************************/

/* Includes ******************************************************************/
#include "freertos_includes.h"
#include <stdbool.h>

/* Public Defines & Macros ***************************************************/

#define BUTTON_TRIGGER_NONE            (0)
#define BUTTON_TRIGGER_EDGE_PRESSED    (BIT(0))
#define BUTTON_TRIGGER_EDGE_RELEASED   (BIT(1))
#define BUTTON_TRIGGER_SHORT_PRESS     (BIT(2))
#define BUTTON_TRIGGER_LONG_PRESS      (BIT(3))
#define BUTTON_TRIGGER_VERY_LONG_PRESS (BIT(4))

/* Public types definition ***************************************************/

typedef struct {
    uint32_t gpio;
    uint32_t triggerBitmap;
} buttonEvent_struct_t;

/* Public prototypes *********************************************************/

void BUTDRV_process(void* pvParameters);

bool BUTDRV_registerButton(uint32_t gpio, uint32_t triggerBitmap, QueueHandle_t eventQueue);

#endif /* __BUTTONS_DRIVER_H__ */
