// load_control.c
//      author: Team 3 - nwol, rsef, jhet

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"

#include <system.h>

#include "inc/load_control.h"
#include "inc/vga.h"
#include "sys/alt_timestamp.h"

#define LOAD_CONTROL_Q_SIZE 10
#define LOAD_CONTROL_Q_TYPE int

#define LOAD_CONTROL_HANDLER_PRIORITY (tskIDLE_PRIORITY + 1)

QueueHandle_t Load_Control_Q;

static void Load_Control_handlerTask(void *pvParameters);
static void Load_Control_initDataStructs();
static void shed_load();
static void unshed_load();

System_Status_T SystemStatus = SYSTEM_OK;
SemaphoreHandle_t SystemStatusMutex;

uint8_t Load_Control_loads = 0xFF;

int Load_Control_Init()
{
	Load_Control_initDataStructs();

	SystemStatusMutex = xSemaphoreCreateMutex();
	xSemaphoreGive(SystemStatusMutex);

	if (xTaskCreate(Load_Control_handlerTask, "Load_Control_T", configMINIMAL_STACK_SIZE, NULL, LOAD_CONTROL_HANDLER_PRIORITY, NULL) != pdPASS)
	{
		return 1;
	}
	return 0;
}

uint8_t spammingTimestampFlag = 0;
static void Load_Control_handlerTask(void *pvParameters)
{
	static System_Frequency_State_T previousState;
	uint8_t localSwitchStatus;
	uint8_t tempSwitchStatus;

	int action;
	while (1)
	{
		// xQueueReceive(Switch_Polling_Q, localSwitchStatus, (TickType_t)10);
		// Read from Peak_Detector_Q
		if (xQueueReceive(Load_Control_Q, &action, portMAX_DELAY) == pdTRUE)
		{
			if (xSemaphoreTake(SystemStatusMutex, (TickType_t)10) == pdTRUE)
			{
				tempSwitchStatus = IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE);
				// printf("Loads: %d\r\n", Load_Control_loads);
				// IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE, ~Load_Control_loads);
				//				printf("Action: %d\n", action);
				if (!action && SystemStatus != SYSTEM_MAINTENANCE)
				{

					if (Load_Control_loads == 0xFF){
						spammingTimestampFlag = 1;
					}

					// Shed the load
					SystemStatus = SYSTEM_MANAGING;
					if (Load_Control_loads != 0)
					{
						shed_load();
						//						printf("SHEDDING LOADS\r\n");
					}
				}
				// TEMP: MUST change to see if all load has been unsheded when stable
				else if (SystemStatus == SYSTEM_MANAGING)
				{
					// SystemStatus = SYSTEM_OK;
					if (Load_Control_loads != 0xFF)
					{
						unshed_load();
						//						printf("UNSHEDDING LOADS\r\n");
					}
					else if (Load_Control_loads == 0xFF)
					{
						if (xSemaphoreTake(Peak_Detector_debounceMutex_X, (TickType_t)10) == pdTRUE)
                		{
							g_peakDetectorDebounceFlag = 0;
							xSemaphoreGive(Peak_Detector_debounceMutex_X);
						}
						SystemStatus = SYSTEM_OK;
						// spammingTimestampFlag = 1;
					}
				}

				if ((SystemStatus == SYSTEM_OK) || (SystemStatus == SYSTEM_MAINTENANCE))
				{
					Load_Control_loads = 0xFF;
					localSwitchStatus = tempSwitchStatus;
				}
				else if (SystemStatus == SYSTEM_MANAGING)
				{
					localSwitchStatus = Load_Control_loads & tempSwitchStatus;
				}

				// printf("LD_SW_ST: %d, LD_CTRL_LDS: %d, ANDED: %d\r\n", localSwitchStatus, Load_Control_loads, (localSwitchStatus & Load_Control_loads));

				IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE, ~(Load_Control_loads));

				IOWR_ALTERA_AVALON_PIO_DATA(RED_LEDS_BASE, localSwitchStatus);
				if(spammingTimestampFlag)
				{
					if(xSemaphoreTake(Peak_Detector_performanceTimerMutex_X, (TickType_t)10) == pdTRUE)
                        {
                            if (g_peakDetectorPerformanceTimestamp != 0){
								int entry = (alt_timestamp() - g_peakDetectorPerformanceTimestamp) / 1000;
								// printf("Time taken: %dms\n", entry);
								xQueueSendToBack(Q_PerformanceMeasure, &entry, pdFALSE);
								g_peakDetectorPerformanceTimestamp = 0;
							}
                            xSemaphoreGive(Peak_Detector_performanceTimerMutex_X);
							spammingTimestampFlag = 0;
							alt_timestamp_start();
                        }
				}
				xSemaphoreGive(SystemStatusMutex);
			}
			vTaskDelay((TickType_t)10);
		}
	}
}

static void shed_load()
{
	// Shed the load of lowest priority.

	for (uint8_t i = 0x01; i != 0; i <<= 1)
	{
		if (Load_Control_loads & i)
		{
			Load_Control_loads &= (i ^ 0xFF);
			// printf("SHEDDING OP: %d/r/n", (Load_Control_loads & (i ^ 0xFF)));
			return;
		}
	}
}

static void unshed_load()
{
	// Unshed the load of highest priority.
	for (uint8_t i = 0x80; i != 0; i >>= 1)
	{
		if (!(Load_Control_loads & i))
		{
			Load_Control_loads ^= i;
			return;
		}
	}
}

static void Load_Control_initDataStructs()
{
	Load_Control_Q = xQueueCreate(LOAD_CONTROL_Q_SIZE, sizeof(LOAD_CONTROL_Q_TYPE));
}
