# Framework
## Overview
This a generic framework in C.

## Requirements
You need to have in your projects an include file including all freertos headers needed.
This file HAS to be named `freertos_includes.h`.

Example:
```c
/**
* ****************************************************************************
* freertos_includes.h
* ****************************************************************************
*/

#ifndef __FREERTOS_INCLUDES_H__
#define __FREERTOS_INCLUDES_H__

/* Includes ******************************************************************/
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#endif /* __FREERTOS_INCLUDES_H__ */
```
