// load_control.h
//      author: Nicholas Wolf

#ifndef LOAD_CONTROL_H
#define LOAD_CONTROL_H

/* Scheduler includes. */
#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/queue.h"
#include "FreeRTOS/semphr.h"
#include "FreeRTOS/task.h"

#include <altera_avalon_pio_regs.h>
#include <sys/alt_irq.h>

#include "inc/frequency_analyser.h"
#include "inc/load_control.h"
#include "inc/peak_detector.h"
#include "inc/switch_polling.h"

typedef enum System_Status_T {
  SYSTEM_OK = 0,
  SYSTEM_MANAGING = 1,
  SYSTEM_MAINTENANCE = 2
} System_Status_T;

extern QueueHandle_t Load_Control_Q;

extern System_Status_T SystemStatus;
extern SemaphoreHandle_t SystemStatusMutex;

int Load_Control_Init();

#endif // LOAD_CONTROL_H
