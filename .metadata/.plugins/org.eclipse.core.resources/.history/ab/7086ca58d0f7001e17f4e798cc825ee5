// load_control.h
//      author: Nicholas Wolf

#ifndef LOAD_CONTROL_H
#define LOAD_CONTROL_H

#include "FreeRTOS/queue.h"
#include "FreeRTOS/semphr.h"

typedef enum System_Status_T
{
    SYSTEM_OK = 0,
    SYSTEM_MANAGING = 1,
    SYSTEM_MAINTENANCE = 2
} System_Status_T;

extern QueueHandle_t Load_Control_Q;

int Load_Control_Init();

#endif // LOAD_CONTROL_H