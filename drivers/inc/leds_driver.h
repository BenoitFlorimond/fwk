/**
 * ****************************************************************************
 * leds_driver.h
 * ****************************************************************************
 */

#ifndef __LEDS_DRIVER_H__
#define __LEDS_DRIVER_H__

/* Documentation *************************************************************/

/* Includes ******************************************************************/

#include <stdbool.h>
#include <stdint.h>

/* Public Defines & Macros ***************************************************/

#define LED_NO_PIN         (0xFF)
#define LED_NO_HANDLE      (0xFF)
#define BLINK_CONTINIOUSLY (0xFF)

/* Public types definition ***************************************************/

/* Public prototypes *********************************************************/

void LEDDRV_process(void* pvParameters);

uint8_t LEDDRV_registerLed(uint32_t rGpio, uint32_t gGpio, uint32_t bGpio);

void LEDDRV_setLedSolid(uint8_t ledHandle, uint32_t color, bool fade, uint32_t delayToFadeMs);

void LEDDRV_setLedBlinking(uint8_t ledHandle, uint32_t color, bool fade, uint32_t delayToFadeMs, uint32_t periodMs, uint32_t blinkCount);

void LEDDRV_setLedOff(uint8_t ledHandle, bool fade, uint32_t delayToFadeMs);

#endif /* __LEDS_DRIVER_H__ */
