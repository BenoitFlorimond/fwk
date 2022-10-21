/**
 * ****************************************************************************
 * os_utils.c
 * ****************************************************************************
 */

/* Documentation *************************************************************/

/* Includes ******************************************************************/

#include "os_utils.h"

/* Private Defines & Macros **************************************************/

#define WAIT_INIT_DELAY_MS (10)

/* Private types definition **************************************************/

/* Private variables *********************************************************/

static bool _initDone = false;

/* Private prototypes ********************************************************/

static bool _isQueueReady(QueueHandle_t queueToSend, uint32_t startTime, uint32_t timeoutValue);

/* Private Functions *********************************************************/

static bool _isQueueReady(QueueHandle_t queueToSend, uint32_t startTime, uint32_t timeoutValue)
{
    bool result = false;

    if (timeoutValue == (uint32_t)portMAX_DELAY) {
        /* Should not happen but check it anyway */
        if (queueToSend != NULL) {
            result = true;
        }
    } else {
        if (xTaskGetTickCount() - startTime < timeoutValue) {
            if (queueToSend != NULL) {
                result = true;
            }
        }
    }

    return result;
}

/* Public Functions **********************************************************/

void OSUTILS_waitSystemStartup(void)
{
    while (_initDone == false) {
        vTaskDelay(pdMS_TO_TICKS(WAIT_INIT_DELAY_MS));
    }
}

void OSUTILS_setSystemReady(void)
{
    _initDone = true;
}

void OSUTILS_deleteQueue(QueueHandle_t* queueToDelete)
{
    QueueHandle_t tempHandle;

    tempHandle     = *queueToDelete;
    *queueToDelete = NULL;
    vQueueDelete(tempHandle);
}

void OSUTILS_deleteSemaphore(SemaphoreHandle_t* semaphoreToDelete)
{
    SemaphoreHandle_t tempHandle;

    tempHandle         = *semaphoreToDelete;
    *semaphoreToDelete = NULL;
    vSemaphoreDelete(tempHandle);
}

void OSUTILS_createResponseQueue(queueCtxt_struct_t* queueContext, QueueHandle_t* queue, TickType_t queueTimeout)
{
    queueContext->queueHandle    = queue;
    queueContext->expirationTime = queueTimeout;
    queueContext->creationTime   = xTaskGetTickCount();
}

void OSUTILS_queueSendSafe(queueCtxt_struct_t* queueContext, void* item)
{
    if (_isQueueReady(*queueContext->queueHandle, queueContext->creationTime, queueContext->expirationTime)) {
        xQueueSend(*queueContext->queueHandle, item, 0U);
    }
}

bool OSUTILS_sendAndWaitResponse(QueueHandle_t queueToSend, void* event, queueCtxt_struct_t* reponseQueue, void* response, uint8_t reponseSize, portTickType timeout)
{
    bool result                  = false;
    QueueHandle_t responseQueue  = xQueueCreate(1U, reponseSize);

    reponseQueue->queueHandle    = &responseQueue;
    reponseQueue->expirationTime = timeout;
    reponseQueue->creationTime   = xTaskGetTickCount();
    if (queueToSend != NULL) {
        if (xQueueSend(queueToSend, event, 0U) == pdTRUE) {
            if (xQueueReceive(responseQueue, response, timeout) == pdTRUE) {
                result = true;
            }
        }
    }
    OSUTILS_deleteQueue(&responseQueue);

    return result;
}
