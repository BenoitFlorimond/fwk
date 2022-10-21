/**
 * ****************************************************************************
 * os_utils.h
 * ****************************************************************************
 */

#ifndef __OS_UTILS_H__
#define __OS_UTILS_H__

/* Documentation *************************************************************/

/* Includes ******************************************************************/
#include <stdint.h>
#include "freertos_includes.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* Public Defines & Macros ***************************************************/

#define WRITE_IN_QUEUE_DEFAULT_TIMEOUT  (pdMS_TO_TICKS(10U))
#define TIMER_API_DEFAULT_TIMEOUT       (pdMS_TO_TICKS(10U))
#define TASK_DEFAULT_REPONSE_TIME_TICKS (pdMS_TO_TICKS(100U))

/* Public types definition ***************************************************/

typedef struct {
    QueueHandle_t* queueHandle;
    TickType_t creationTime;
    TickType_t expirationTime;
} queueCtxt_struct_t;

/* Public prototypes *********************************************************/

void OSUTILS_deleteQueue(QueueHandle_t* QueueToDelete);
void OSUTILS_deleteSemaphore(SemaphoreHandle_t* semaphoreToDelete);
void OSUTILS_createResponseQueue(queueCtxt_struct_t* queueContext, QueueHandle_t* queue, TickType_t queueTimeout);
void OSUTILS_queueSendSafe(queueCtxt_struct_t* queueContext, void* item);
bool OSUTILS_sendAndWaitResponse(QueueHandle_t queueToSend, void* event, queueCtxt_struct_t* reponseQueue, void* response, uint8_t reponseSize, portTickType timeout);

#endif /* __OS_UTILS_H__ */