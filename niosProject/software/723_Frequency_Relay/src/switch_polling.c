/* Standard includes. */
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <sys/alt_irq.h>
#include <io.h>
#include <altera_avalon_pio_regs.h>

#include "inc/Switch_Polling.h"
#include "inc/frequency_analyser.h"
#include "inc/peak_detector.h"
#include "inc/load_control.h"

#define SWITCH_POLLING_TASK_PRIORITY (tskIDLE_PRIORITY + 1)

#define SWITCH_POLLING_Q_TYPE int
#define SWITCH_POLLING_Q_SIZE 10

// Mutexed LoadSwitchStatus
int LoadSwitchStatus;
SemaphoreHandle_t LoadSwitchStatusMutex;

QueueHandle_t Switch_Polling_Q;

static void Switch_Polling_initDataStructs();

int Switch_Polling_init()
{
	Switch_Polling_initDataStructs();

	if (xTaskCreate(Switch_Polling_handlerTask, "Switch_Polling_handlerTask", configMINIMAL_STACK_SIZE, NULL, SWITCH_POLLING_TASK_PRIORITY, NULL) != pdPASS)
	{
		return 1;
	}
	return 0;
}

static void Switch_Polling_handlerTask(void *pvParameters)
{
	static int previousLoads;

	int entry;
	while (1)
	{
		entry = IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE);
		if (entry != previousLoads)
		{
			if (xSemaphoreTake(SystemStatusMutex, (TickType_t)10) == pdTRUE)
			{
				previousLoads = entry;

				// Cant turn on load when in managing
				if (SystemStatus == SYSTEM_MANAGING)
				{
					LoadSwitchStatus &= entry;
				}
				else
				{
					LoadSwitchStatus = entry;
				}
				xSemaphoreGive(SystemStatusMutex);
			}
			xQueueSendToBack(Switch_Polling_Q, LoadSwitchStatus, pdFALSE);
		}
	}
	// vTaskDelay(pdMS_TO_TICKS(500));
}

static void Switch_Polling_initDataStructs()
{
	LoadSwitchStatusMutex = xSemaphoreCreateMutex();
	Switch_Polling_Q = xQueueCreate(SWITCH_POLLING_Q_SIZE, sizeof(SWITCH_POLLING_Q_TYPE));
}
