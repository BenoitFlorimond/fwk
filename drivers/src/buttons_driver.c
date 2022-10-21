/**
 * ****************************************************************************
 * buttons_driver.c
 * ****************************************************************************
 */

/* Documentation *************************************************************/

/* Includes ******************************************************************/

#include "buttons_driver.h"
#include "os_utils.h"
#include <string.h>
#include "driver/gpio.h"
#include "esp_log.h"

/* Private Defines & MacrosUtils **************************************************/

#define TAG_BUTTON                              ("BUT")
#define BUTTON_PRESSED_SHORT_DURATION_TICKS     (pdMS_TO_TICKS(50U))
#define BUTTON_PRESSED_LONG_DURATION_TICKS      (pdMS_TO_TICKS(3000U))
#define BUTTON_PRESSED_VERY_LONG_DURATION_TICKS (pdMS_TO_TICKS(10000U))
#define MAX_BUTTON_NUMBER                       (10U)
#define CHECK_BUTTONS_DELAY_TICKS               (pdMS_TO_TICKS(100U))

/* Private types definition **************************************************/

typedef enum {
    BUTTON_ACTION_RELEASE = 0,
    BUTTON_ACTION_PRESS,
    /* Do not erase */
    BUTTON_ACTION_NUMBER
} buttonAction_enum_t;

typedef enum {
    BUTTON_STATE_RELEASED = 0,
    BUTTON_STATE_PRESSED_SHORT,
    BUTTON_STATE_PRESSED_LONG,
    BUTTON_STATE_PRESSED_VERY_LONG,
    /* Do not erase */
    BUTTON_STATE_NUMBER
} buttonState_enum_t;

typedef enum {
    BUTTON_EVENT_ISR = 0,
    BUTTON_EVENT_CONFIG,
    BUTTON_EVENT_CHECK
} buttonEvent_enum_t;

/* Struct */

typedef struct {
    uint32_t gpio;
    buttonAction_enum_t action;
} buttonIsrEvent_struct_t;

typedef struct {
    uint32_t gpio;
    uint32_t triggerRegister;
    QueueHandle_t eventQueue;
    queueCtxt_struct_t responseQueue;
} buttonConfig_struct_t;

typedef struct {
    buttonConfig_struct_t config;
    buttonAction_enum_t lastAction;
    buttonState_enum_t currentState;
    TickType_t lastActionTimestamp;
    uint32_t eventsTriggered;
} buttonContext_struct_t;

typedef struct {
    buttonEvent_enum_t type;
    union {
        buttonIsrEvent_struct_t isr;
        buttonConfig_struct_t config;
    };
} buttonQueueEvent_struct_t;

/* Private variables *********************************************************/

static QueueHandle_t _queueForButtons                         = NULL;
static buttonContext_struct_t _buttonsList[MAX_BUTTON_NUMBER] = { 0 };
static uint8_t _buttonNumber                                  = 0;

/* Private prototypes ********************************************************/

static void _buttonsISR(void* gpioNum);
static bool _getButtonFromGpio(uint32_t gpio, buttonContext_struct_t** pButton);
static bool _notifyToUpperLayer(uint32_t triggerBitmap, buttonConfig_struct_t* pButtonConfig);

/* Private Functions *********************************************************/

static void _buttonsISR(void* gpioNum)
{
    BaseType_t higherTaskWoken = pdFALSE;
    buttonQueueEvent_struct_t event;

    event.type     = BUTTON_EVENT_ISR;
    event.isr.gpio = (uint32_t)gpioNum;
    if (gpio_get_level((uint32_t)gpioNum) == 0) {
        event.isr.action = BUTTON_ACTION_RELEASE;
    } else {
        event.isr.action = BUTTON_ACTION_PRESS;
    }

    if (_queueForButtons != NULL) {
        xQueueSendFromISR(_queueForButtons, &event, &higherTaskWoken);
    }

    if (higherTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static bool _getButtonFromGpio(uint32_t gpio, buttonContext_struct_t** pButton)
{
    bool result = false;

    for (uint8_t buttonCounter = 0U; buttonCounter < _buttonNumber; buttonCounter++) {
        if (_buttonsList[buttonCounter].config.gpio == gpio) {
            *pButton = &_buttonsList[buttonCounter];
            result   = true;
            break;
        }
    }
    return result;
}

static bool _notifyToUpperLayer(uint32_t triggerBitmap, buttonConfig_struct_t* pButtonConfig)
{
    buttonEvent_struct_t upperLayerEvent;
    bool result                   = false;

    upperLayerEvent.triggerBitmap = triggerBitmap;
    if (upperLayerEvent.triggerBitmap != BUTTON_TRIGGER_NONE) {
        upperLayerEvent.gpio = pButtonConfig->gpio;
        if (pButtonConfig->eventQueue != NULL) {
            xQueueSend(pButtonConfig->eventQueue, &upperLayerEvent, 10);
            result = true;
        }
    }

    return result;
}

/* Public Functions **********************************************************/

void BUTDRV_process(void* pvParameters)
{
    buttonQueueEvent_struct_t buttonEvent  = { 0 };
    buttonContext_struct_t* pCurrentButton = NULL;
    gpio_config_t gpioConfig               = { 0 };
    TickType_t taskBlockTime               = portMAX_DELAY;
    uint8_t buttonCounter                  = 0;
    uint32_t currentEventsTriggered        = BUTTON_TRIGGER_NONE;
    bool result                            = false;
    static bool _isrServiceInstalled       = false;

    _queueForButtons                       = xQueueCreate(10, sizeof(buttonEvent));

    OSUTILS_waitSystemStartup();

    for (;;) {
        if (xQueueReceive(_queueForButtons, &buttonEvent, taskBlockTime) == pdTRUE) {
            switch (buttonEvent.type) {
            case BUTTON_EVENT_CONFIG:
                if (_buttonNumber < MAX_BUTTON_NUMBER) {
                    /* add button in list */
                    memcpy(&_buttonsList[_buttonNumber].config, &buttonEvent.config, sizeof(buttonEvent.config));
                    _buttonsList[_buttonNumber].currentState        = BUTTON_STATE_RELEASED;
                    _buttonsList[_buttonNumber].lastAction          = BUTTON_ACTION_RELEASE;
                    _buttonsList[_buttonNumber].lastActionTimestamp = 0U;
                    _buttonsList[_buttonNumber].eventsTriggered     = 0U;
                    _buttonNumber++;
                    /* configure new gpio and isr */
                    gpioConfig.pin_bit_mask = BIT64(buttonEvent.config.gpio);
                    gpioConfig.mode         = GPIO_MODE_INPUT;
                    gpioConfig.pull_down_en = GPIO_PULLDOWN_ENABLE;
                    gpioConfig.pull_up_en   = GPIO_PULLUP_DISABLE;
                    gpioConfig.intr_type    = GPIO_INTR_ANYEDGE;
                    gpio_config(&gpioConfig);
                    if (!_isrServiceInstalled) {
                        _isrServiceInstalled = (gpio_install_isr_service(0) == ESP_OK);
                    }
                    gpio_isr_handler_add(buttonEvent.config.gpio, _buttonsISR, (void*)buttonEvent.config.gpio);
                    result = true;
                } else {
                    result = false;
                }
                OSUTILS_queueSendSafe(&buttonEvent.config.responseQueue, &result);
                break;

            case BUTTON_EVENT_ISR:
                if (_getButtonFromGpio(buttonEvent.isr.gpio, &pCurrentButton)) {
                    switch (buttonEvent.isr.action) {
                    case BUTTON_ACTION_PRESS:
                        /* Reset current events triggered */
                        pCurrentButton->eventsTriggered = BUTTON_TRIGGER_NONE;
                        currentEventsTriggered          = BUTTON_TRIGGER_EDGE_PRESSED;
                        break;
                    case BUTTON_ACTION_RELEASE:
                        currentEventsTriggered = BUTTON_TRIGGER_EDGE_RELEASED;
                        if ((xTaskGetTickCount() - pCurrentButton->lastActionTimestamp) > BUTTON_PRESSED_VERY_LONG_DURATION_TICKS) {
                            currentEventsTriggered |= BUTTON_TRIGGER_VERY_LONG_PRESS;
                        } else if ((xTaskGetTickCount() - pCurrentButton->lastActionTimestamp) > BUTTON_PRESSED_LONG_DURATION_TICKS) {
                            currentEventsTriggered |= BUTTON_TRIGGER_LONG_PRESS;
                        } else if ((xTaskGetTickCount() - pCurrentButton->lastActionTimestamp) > BUTTON_PRESSED_SHORT_DURATION_TICKS) {
                            currentEventsTriggered |= BUTTON_TRIGGER_SHORT_PRESS;
                        }
                        break;
                    default:
                        break;
                    }
                    /* Mask events triggered with register in conf and with events not already triggered in the past */
                    currentEventsTriggered &= pCurrentButton->config.triggerRegister;
                    currentEventsTriggered &= ~pCurrentButton->eventsTriggered;
                    /* Save new events triggered */
                    pCurrentButton->eventsTriggered |= currentEventsTriggered;
                    /* Save action context */
                    pCurrentButton->lastActionTimestamp = xTaskGetTickCount();
                    pCurrentButton->lastAction          = buttonEvent.isr.action;
                    /* Notify upper layer */
                    _notifyToUpperLayer(currentEventsTriggered, &pCurrentButton->config);
                }

                break;

            default:
                break;
            }
        }
        /* In any case, check other buttons state */
        taskBlockTime = portMAX_DELAY;
        for (buttonCounter = 0U; buttonCounter < _buttonNumber; buttonCounter++) {
            if (_buttonsList[buttonCounter].lastAction == BUTTON_ACTION_PRESS) {
                taskBlockTime          = CHECK_BUTTONS_DELAY_TICKS;
                currentEventsTriggered = BUTTON_TRIGGER_NONE;
                if ((xTaskGetTickCount() - _buttonsList[buttonCounter].lastActionTimestamp) > BUTTON_PRESSED_VERY_LONG_DURATION_TICKS) {
                    currentEventsTriggered |= BUTTON_TRIGGER_VERY_LONG_PRESS;
                } else if ((xTaskGetTickCount() - _buttonsList[buttonCounter].lastActionTimestamp) > BUTTON_PRESSED_LONG_DURATION_TICKS) {
                    currentEventsTriggered |= BUTTON_TRIGGER_LONG_PRESS;
                } else if ((xTaskGetTickCount() - _buttonsList[buttonCounter].lastActionTimestamp) > BUTTON_PRESSED_SHORT_DURATION_TICKS) {
                    currentEventsTriggered |= BUTTON_TRIGGER_SHORT_PRESS;
                }
                /* Mask events triggered with register in conf and with events not already triggered in the past */
                currentEventsTriggered &= _buttonsList[buttonCounter].config.triggerRegister;
                currentEventsTriggered &= ~_buttonsList[buttonCounter].eventsTriggered;
                /* Save new events triggered */
                _buttonsList[buttonCounter].eventsTriggered |= currentEventsTriggered;
                /* Notify upper layer */
                _notifyToUpperLayer(currentEventsTriggered, &_buttonsList[buttonCounter].config);
            }
        }
    }
}

bool BUTDRV_registerButton(uint32_t gpio, uint32_t triggerBitmap, QueueHandle_t eventQueue)
{
    buttonQueueEvent_struct_t newButton;
    bool result                      = false;

    newButton.type                   = BUTTON_EVENT_CONFIG;
    newButton.config.gpio            = gpio;
    newButton.config.triggerRegister = triggerBitmap;
    newButton.config.eventQueue      = eventQueue;
    if (!OSUTILS_sendAndWaitResponse(_queueForButtons, &newButton, &newButton.config.responseQueue, &result, sizeof(result), TASK_DEFAULT_REPONSE_TIME_TICKS)) {
        ESP_LOGE(TAG_BUTTON, "Cannot get response from task");
    }
    return result;
}
