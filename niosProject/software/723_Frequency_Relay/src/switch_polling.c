/* Standard includes. */
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "inc/Switch_Polling.h"

#define SWITCH_POLLING_TASK_PRIORITY 4

// Mutexed LoadSwitchStatus
int LoadSwitchStatus;
SemaphoreHandle_t LoadSwitchStatusMutex;

int switch_polling_init(){
	LoadSwitchStatusMutex = xSemaphoreCreateMutex();
	xSemaphoreGive(LoadSwitchStatusMutex);

	if(xTaskCreate(Switch_Polling_handlerTask, "Switch_Polling_handlerTask", configMINIMAL_STACK_SIZE, NULL, SWITCH_POLLING_TASK_PRIORITY, NULL) !=pdPASS){
		return 1;
	}
	return 0;
}

static void Switch_Polling_handlerTask(void *pvParameters)
{
	while(1){

		if (xSemaphoreTake(SystemStatusMutex, (TickType_t)10) == pdTRUE){

			if (xSemaphoreTake(LoadSwitchStatusMutex, (TickType_t)10) == pdTRUE) {
				//printf("LoadSwithSemaphor\n\r");
				if (SystemStatus == SYSTEM_MANAGING){
					LoadSwitchStatus &= IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE);
				} else{
					LoadSwitchStatus = IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE);
				}

				xSemaphoreGive(LoadSwitchStatusMutex);
			}
			xSemaphoreGive(SystemStatusMutex);
			//must delay when giving the semaphore so that the task wont take it again before the other
			vTaskDelay(pdMS_TO_TICKS(500));
		}
	}


}
