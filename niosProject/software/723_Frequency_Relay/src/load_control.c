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

#define LOAD_CONTROL_HANDLER_PRIORITY (tskIDLE_PRIORITY + 2)

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
	int end_timestamp;

	int action;
	while (1)
	{
		// Read from Peak_Detector_Q
		if (xQueueReceive(Load_Control_Q, &action, portMAX_DELAY) == pdTRUE)
		{
			// Take the system status mutex as we may be changing it.
			if (xSemaphoreTake(SystemStatusMutex, (TickType_t)10) == pdTRUE)
			{
				// Read the switches at the IO address for them.
				tempSwitchStatus = IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE);
				// If the signal is unstable and the system status is not in maintenence
				if (!action && SystemStatus != SYSTEM_MAINTENANCE)
				{
					// If the load being shed is the first load, set a flag to take the performance measurement
					if (Load_Control_loads == 0xFF){
						spammingTimestampFlag = 1;
					}
					// Shed the load if there are loads to be shed
					SystemStatus = SYSTEM_MANAGING;
					if (Load_Control_loads != 0)
					{
						shed_load();
					}
				}
				// If the system status is managing (and the signal is stable),
				else if (SystemStatus == SYSTEM_MANAGING)
				{
					// Unshed the load if there are loads to be unshed
					if (Load_Control_loads != 0xFF)
					{
						unshed_load();
					}
					// If all the loads have been unshed, reset the initial load shed flag, so that the peak detector
					// knows that the next load to be shed will be an first-load (instant response).
					else if (Load_Control_loads == 0xFF)
					{
						if (xSemaphoreTake(Peak_Detector_debounceMutex_X, (TickType_t)10) == pdTRUE)
                		{
							g_peakDetectorDebounceFlag = 0;
							xSemaphoreGive(Peak_Detector_debounceMutex_X);
						}
						// Set the system status to OK
						SystemStatus = SYSTEM_OK;
					}
				}

				// Logic for RED switches
				// If the system status is OK or MAINTENANCE, set the loads to all ON, and set the localSwitchStatus to
				// the value of the read switches.
				if ((SystemStatus == SYSTEM_OK) || (SystemStatus == SYSTEM_MAINTENANCE))
				{
					Load_Control_loads = 0xFF;
					localSwitchStatus = tempSwitchStatus;
				}
				// Else if the system status is MANAGING, set the localSwitchStatus to the current loads ANDED with the
				// value of the read switches (i.e. loads can only be turned off)
				else if (SystemStatus == SYSTEM_MANAGING)
				{
					localSwitchStatus = Load_Control_loads & tempSwitchStatus;
				}

				// Write green LEDs to the inverse of the loads, write the red LEDs to localSwitchStatus.
				IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE, ~(Load_Control_loads));
				IOWR_ALTERA_AVALON_PIO_DATA(RED_LEDS_BASE, localSwitchStatus);

				// Grab a end timestamp for the performance measurement
				end_timestamp = alt_timestamp();
				// If the flag for taking a timestamp has been set, caculate the time taken and send to VGATask through
				// peformance timestamp queue and reset the timer.
				if(spammingTimestampFlag)
				{
					if(xSemaphoreTake(Peak_Detector_performanceTimerMutex_X, (TickType_t)10) == pdTRUE)
                        {
                            if (g_peakDetectorPerformanceTimestamp != 0){
								int entry = (end_timestamp - g_peakDetectorPerformanceTimestamp) / 1000;
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
