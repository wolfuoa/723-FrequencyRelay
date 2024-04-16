/* Scheduler includes. */
#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/semphr.h"


#include <altera_avalon_pio_regs.h>

#include "inc/load_control.h"


static void Switch_Polling_handlerTask(void *pvParameters);
int Switch_Polling_init();


extern int LoadSwitchStatus;
extern SemaphoreHandle_t LoadSwitchStatusMutex;
