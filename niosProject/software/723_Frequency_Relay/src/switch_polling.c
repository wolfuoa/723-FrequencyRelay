/* Standard includes. */
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Scheduler includes. */
#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/queue.h"
#include "FreeRTOS/semphr.h"

#include <sys/alt_irq.h>
#include <io.h>
#include <altera_avalon_pio_regs.h>

#include "inc/frequency_analyser.h"
#include "inc/peak_detector.h"
#include "inc/load_control.h"

#include "inc/frequency_analyser.h"
#include "inc/peak_detector.h"
#include "inc/load_control.h"
#include "inc/Switch_Polling.h"

#define SWITCH_POLLING_TASK_PRIORITY 4

// Mutexed LoadSwitchStatus
int LoadSwitchStatus;
SemaphoreHandle_t LoadSwitchStatusMutex;

int switch_polling_init(){
	LoadSwitchStatusMutex = xSemaphoreCreateMutex();

	if(xTaskCreate(Switch_Polling_handlerTask, "Switch_Polling_handlerTask", configMINIMAL_STACK_SIZE, NULL, SWITCH_POLLING_TASK_PRIORITY, NULL) !=pdPASS){
		return 1;
	}
	return 0;
}

static void Switch_Polling_handlerTask(void *pvParameters)
{
	while(1){
		vTaskDelay(pdMS_TO_TICKS(1000));
		printf("The switches are: %d\n\r",IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE));
	}

}
