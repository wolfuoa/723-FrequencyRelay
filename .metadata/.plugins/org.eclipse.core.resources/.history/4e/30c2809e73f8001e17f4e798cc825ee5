#ifndef SWITCH_POLLING_H
#define SWITCH_POLLING_H

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/queue.h"
#include "FreeRTOS/semphr.h"

static void Switch_Polling_handlerTask(void *pvParameters);

extern int LoadSwitchStatus;
extern QueueHandle_t Switch_Polling_Q;
extern SemaphoreHandle_t LoadSwitchStatusMutex;

#endif // SWITCH_POLLING_H