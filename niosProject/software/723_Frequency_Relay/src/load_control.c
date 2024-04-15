// load_control.c
//      author: Nicholas Wolf

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"

#include "inc/load_control.h"

#define LOAD_CONTROL_Q_SIZE 10
#define LOAD_CONTROL_Q_TYPE int

#define LOAD_CONTROL_HANDLER_PRIORITY 5//(tskIDLE_PRIORITY + 1)

QueueHandle_t Load_Control_Q;

static void Load_Control_handlerTask(void *pvParameters);
static void Load_Control_initDataStructs();
static void shed_load();
static void unshed_load();

System_Status_T SystemStatus;
SemaphoreHandle_t SystemStatusMutex;

// Loads are represented by this 8-bit value, the priority is
// given by the position of the bits.
uint8_t Loads = 0xFF;

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

static void Load_Control_handlerTask(void *pvParameters)
{
    int action;
    while (1)
    {

    	if (xSemaphoreTake(LoadSwitchStatusMutex, (TickType_t)10) == pdTRUE){

    		IOWR_ALTERA_AVALON_PIO_DATA(RED_LEDS_BASE,LoadSwitchStatus);

    	}
    	xSemaphoreGive(LoadSwitchStatusMutex);

        // Read from Peak_Detector_Q
        if (xQueueReceive(Load_Control_Q, &action, portMAX_DELAY) == pdTRUE){
        	if (xSemaphoreTake(SystemStatusMutex, (TickType_t)10) == pdTRUE){
        		IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE,SystemStatus);
				//printf("Action: %d\n", action);
				if (!action && SystemStatus != SYSTEM_MAINTENANCE){
					//Shed the load
					SystemStatus = SYSTEM_MANAGING;
					if (Loads != 0){
						shed_load();
						printf("SHEDDING LOADS");
					}
				}
				// TEMP: MUST change to see if all load has been unsheded when stable
				else if(SystemStatus == SYSTEM_MANAGING){
					SystemStatus = SYSTEM_OK;
					if (Loads != 0xFF){
						unshed_load();
						printf("UNSHEDDING LOADS");
					}
				}
        	}
        	xSemaphoreGive(SystemStatusMutex);

        }
    }
}


static void shed_load()
{
	// Shed the load of lowest priority.
	for (uint8_t i = 0x01; i == 0; i << 1){
		if (Loads & i){
			Loads & (i ^ 0xFF);
			// ## TODO: Time Stamping ##
			return;
		}
	}
}


static void unshed_load()
{
	// Unshed the load of highest priority.
	for (uint8_t i = 0x80; i == 0; i >> 1){
		if (!(Loads & i)){
			Loads ^ i;
			// ## TODO: Time Stamping ##
			return;
		}
	}
}

static void Load_Control_initDataStructs()
{
    Load_Control_Q = xQueueCreate(LOAD_CONTROL_Q_SIZE, sizeof(LOAD_CONTROL_Q_TYPE));
}
