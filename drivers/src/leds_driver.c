/**
 * ****************************************************************************
 * leds_driver.c
 * ****************************************************************************
 */

/* Documentation *************************************************************/

/* Includes ******************************************************************/

#include "leds_driver.h"
#include "driver/ledc.h"
#include "freertos_includes.h"
#include "esp_log.h"
#include "os_utils.h"
#include <string.h>

/* Private Defines & Macros **************************************************/

#define TAG_LEDS                              ("LEDS")
#define MAX_REGISTERED_LEDS                   (LEDC_CHANNEL_MAX) /* If only one color LEDs */

#define PIN_RED_INDEX                         (0)
#define PIN_GREEN_INDEX                       (1)
#define PIN_BLUE_INDEX                        (2)
#define MAX_PINS_PER_LED                      (3)

#define GET_SINGLE_COLOR_FROM_RGB(rgb, index) (((rgb) >> (16 - 8 * index)) & 0xFF)
#define LED_COLOR_BLACK                       (0)
#define RED_GAIN                              (1.0)
#define GREEN_GAIN                            (0.5)
#define BLUE_GAIN                             (1.0)

#define CHECK_BLINKING_PERIOD_TICKS           (pdMS_TO_TICKS(100))

/* Private types definition **************************************************/

typedef enum {
    EVENT_REGISTER,
    EVENT_SET_SOLID,
    EVENT_SET_BLINKING,
} ledEvent_enum_t;

typedef struct {
    uint32_t rGpio;
    uint32_t gGpio;
    uint32_t bGpio;
} ledConfig_struct_t;

typedef struct {
    uint32_t rgbColor;
    bool fade;
    uint32_t delayToFade;
    uint32_t blinkCount;
    uint32_t blinkPeriodMs;
} ledParam_struct_t;

typedef struct {
    ledEvent_enum_t type;
    queueCtxt_struct_t responseQueue;
    uint8_t ledHandle;
    union {
        ledConfig_struct_t config;
        ledParam_struct_t params;
    };
} ledEvent_struct_t;

typedef struct {
    ledParam_struct_t ledParams;
    uint32_t nextActionTimestamp;
    uint32_t nextColor;
} ledBlinkContext_struct_t;

/* Private variables *********************************************************/

static QueueHandle_t _queueForLeds                                        = NULL;
static ledc_channel_t _ledChannels[MAX_REGISTERED_LEDS][MAX_PINS_PER_LED] = { [0 ... MAX_REGISTERED_LEDS - 1][0 ... MAX_PINS_PER_LED - 1] = LEDC_CHANNEL_MAX };
static ledc_timer_config_t _ledTimer                                      = {
                                         .duty_resolution = LEDC_TIMER_13_BIT,    // resolution of PWM duty
                                         .freq_hz         = 5000,                 // frequency of PWM signal
                                         .speed_mode      = LEDC_HIGH_SPEED_MODE, // timer mode
                                         .timer_num       = LEDC_TIMER_0,         // timer index
                                         .clk_cfg         = LEDC_AUTO_CLK,        // Auto select the source clock
};

static ledc_channel_config_t _channelConfig = {
    .channel    = LEDC_CHANNEL_0,
    .duty       = 0,
    .gpio_num   = LED_NO_PIN,
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .hpoint     = 0,
    .timer_sel  = LEDC_TIMER_0,
};
static uint8_t _ledIndex                                          = 0;
static float _gainPerColor[MAX_PINS_PER_LED]                      = { RED_GAIN, GREEN_GAIN, BLUE_GAIN };
static ledBlinkContext_struct_t _ledsContext[MAX_REGISTERED_LEDS] = { 0 };

/* Private prototypes ********************************************************/

static void _setLed(uint8_t handle, uint32_t rgbColor, bool fade, uint32_t fadeDelayMs);

/* Private Functions *********************************************************/

static void _setLed(uint8_t handle, uint32_t rgbColor, bool fade, uint32_t fadeDelayMs)
{
    uint8_t colorIndex = 0;
    uint32_t dutyCycle = 0;

    for (colorIndex = 0; colorIndex < MAX_PINS_PER_LED; colorIndex++) {
        if (_ledChannels[handle][colorIndex] < LEDC_CHANNEL_MAX) {
            dutyCycle = _gainPerColor[colorIndex] * GET_SINGLE_COLOR_FROM_RGB(rgbColor, colorIndex) * BIT(LEDC_TIMER_13_BIT) / 0xFF;
            if (fade) {
                ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, _ledChannels[handle][colorIndex], dutyCycle, fadeDelayMs);
                ledc_fade_start(LEDC_HIGH_SPEED_MODE, _ledChannels[handle][colorIndex], LEDC_FADE_NO_WAIT);
            } else {
                ledc_set_duty(LEDC_HIGH_SPEED_MODE, _ledChannels[handle][colorIndex], dutyCycle);
                ledc_update_duty(LEDC_HIGH_SPEED_MODE, _ledChannels[handle][colorIndex]);
            }
        }
    }
}

/* Public Functions **********************************************************/

void LEDDRV_process(void* pvParameters)
{
    ledEvent_struct_t event    = { 0 };
    _queueForLeds              = xQueueCreate(10, sizeof(ledEvent_struct_t));
    uint8_t ledIndex           = 0;
    bool atLeastOneLedBlinking = false;
    TickType_t ticksToBlock    = portMAX_DELAY;

    ledc_timer_config(&_ledTimer);
    ledc_fade_func_install(0);

    for (;;) {
        if (xQueueReceive(_queueForLeds, &event, ticksToBlock) == pdTRUE) {
            switch (event.type) {
            case EVENT_REGISTER:
                if ((event.config.rGpio != LED_NO_PIN) && (_channelConfig.channel < LEDC_CHANNEL_MAX)) {
                    _channelConfig.gpio_num = event.config.rGpio;
                    ledc_channel_config(&_channelConfig);
                    _ledChannels[_ledIndex][PIN_RED_INDEX] = _channelConfig.channel++;
                }
                if ((event.config.gGpio != LED_NO_PIN) && (_channelConfig.channel < LEDC_CHANNEL_MAX)) {
                    _channelConfig.gpio_num = event.config.gGpio;
                    ledc_channel_config(&_channelConfig);
                    _ledChannels[_ledIndex][PIN_GREEN_INDEX] = _channelConfig.channel++;
                }
                if ((event.config.bGpio != LED_NO_PIN) && (_channelConfig.channel < LEDC_CHANNEL_MAX)) {
                    _channelConfig.gpio_num = event.config.bGpio;
                    ledc_channel_config(&_channelConfig);
                    _ledChannels[_ledIndex][PIN_BLUE_INDEX] = _channelConfig.channel++;
                }
                OSUTILS_queueSendSafe(&event.responseQueue, &_ledIndex);
                _ledIndex++;
                break;

            case EVENT_SET_BLINKING:
                memcpy(&_ledsContext[event.ledHandle].ledParams, &event.params, sizeof(event.params));
                _ledsContext[event.ledHandle].nextActionTimestamp = xTaskGetTickCount() + pdMS_TO_TICKS(_ledsContext[event.ledHandle].ledParams.blinkPeriodMs / 2);
                _ledsContext[event.ledHandle].nextColor           = LED_COLOR_BLACK;
                ticksToBlock                                      = CHECK_BLINKING_PERIOD_TICKS;
                _setLed(event.ledHandle, event.params.rgbColor, event.params.fade, event.params.delayToFade);
                break;

            case EVENT_SET_SOLID:
                _ledsContext[event.ledHandle].ledParams.blinkCount = 0;
                _setLed(event.ledHandle, event.params.rgbColor, event.params.fade, event.params.delayToFade);
                break;

            default:
                break;
            }
        }

        /* In any case, check blinking leds */
        atLeastOneLedBlinking = false;
        for (ledIndex = 0; ledIndex < MAX_REGISTERED_LEDS; ledIndex++) {
            if (_ledsContext[ledIndex].ledParams.blinkCount != 0) {
                atLeastOneLedBlinking = true;
                if (xTaskGetTickCount() > _ledsContext[ledIndex].nextActionTimestamp) {
                    _setLed(ledIndex, _ledsContext[ledIndex].nextColor, _ledsContext[ledIndex].ledParams.fade, _ledsContext[ledIndex].ledParams.delayToFade);
                    if ((_ledsContext[ledIndex].ledParams.blinkCount != BLINK_CONTINIOUSLY) && (_ledsContext[ledIndex].nextColor == LED_COLOR_BLACK)) {
                        _ledsContext[ledIndex].ledParams.blinkCount--;
                    }
                    _ledsContext[ledIndex].nextActionTimestamp = xTaskGetTickCount() + pdMS_TO_TICKS(_ledsContext[ledIndex].ledParams.blinkPeriodMs / 2);
                    _ledsContext[ledIndex].nextColor           = (_ledsContext[ledIndex].nextColor == LED_COLOR_BLACK) ? _ledsContext[ledIndex].ledParams.rgbColor : LED_COLOR_BLACK;
                }
            }
        }
        if (!atLeastOneLedBlinking) {
            ticksToBlock = portMAX_DELAY;
        }
    }
}

uint8_t LEDDRV_registerLed(uint32_t rGpio, uint32_t gGpio, uint32_t bGpio)
{
    uint8_t ledHandle       = LED_NO_HANDLE;
    ledEvent_struct_t event = { 0 };

    event.type              = EVENT_REGISTER;
    event.config.rGpio      = rGpio;
    event.config.gGpio      = gGpio;
    event.config.bGpio      = bGpio;

    if (!OSUTILS_sendAndWaitResponse(_queueForLeds, &event, &event.responseQueue, &ledHandle, sizeof(ledHandle), TASK_DEFAULT_REPONSE_TIME_TICKS)) {
        ESP_LOGE(TAG_LEDS, "Cannot get response from task");
    } else if (ledHandle == LED_NO_HANDLE) {
        ESP_LOGE(TAG_LEDS, "Cannot configure LED");
    }
    return ledHandle;
}

void LEDDRV_setLedSolid(uint8_t ledHandle, uint32_t color, bool fade, uint32_t delayToFadeMs)
{
    ledEvent_struct_t event  = { 0 };

    event.type               = EVENT_SET_SOLID;
    event.ledHandle          = ledHandle;
    event.params.rgbColor    = color;
    event.params.fade        = fade;
    event.params.delayToFade = delayToFadeMs;

    xQueueSend(_queueForLeds, &event, WRITE_IN_QUEUE_DEFAULT_TIMEOUT);
}

void LEDDRV_setLedBlinking(uint8_t ledHandle, uint32_t color, bool fade, uint32_t delayToFadeMs, uint32_t periodMs, uint32_t blinkCount)
{
    ledEvent_struct_t event    = { 0 };

    event.type                 = EVENT_SET_BLINKING;
    event.ledHandle            = ledHandle;
    event.params.rgbColor      = color;
    event.params.blinkPeriodMs = periodMs;
    event.params.blinkCount    = blinkCount;
    event.params.fade          = fade;
    event.params.delayToFade   = delayToFadeMs;

    xQueueSend(_queueForLeds, &event, WRITE_IN_QUEUE_DEFAULT_TIMEOUT);
}

void LEDDRV_setLedOff(uint8_t ledHandle, bool fade, uint32_t delayToFadeMs)
{
    LEDDRV_setLedSolid(ledHandle, LED_COLOR_BLACK, fade, delayToFadeMs);
}
