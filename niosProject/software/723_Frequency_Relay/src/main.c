/*
 * 723 ASSIGNMENT 1 - FREQUENCY RELAY
 * 		authors: Nicholas Wolf, Janith Hetteriachchi, Robert Sefaj
 */

/* Standard includes. */
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Scheduler includes. */
#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/queue.h"
#include "FreeRTOS/semphr.h"

#include "sys/alt_irq.h"
#include "io.h"
#include "altera_avalon_pio_regs.h"

/* The parameters passed to the reg test tasks.  This is just done to check
 the parameter passing mechanism is working correctly. */
#define mainREG_TEST_1_PARAMETER    ( ( void * ) 0x12345678 )
#define mainREG_TEST_2_PARAMETER    ( ( void * ) 0x87654321 )
#define mainREG_TEST_PRIORITY       ( tskIDLE_PRIORITY + 1)
static void prvFirstRegTestTask(void *pvParameters);
static void prvSecondRegTestTask(void *pvParameters);
SemaphoreHandle_t freq_semaphore;
void freq_relay(){
	//unsigned int temp = IORD(FREQUENCY_ANALYSER_BASE, 0);
	//printf("%f Hz\n", 16000/(double)temp);
	BaseType_t handle = pdFALSE;
	xSemaphoreGiveFromISR(freq_semaphore,&handle);

	return;
}

/*
 * Create the demo tasks then start the scheduler.
 */
int main(void)
{
	alt_irq_register(0x43100, 0, freq_relay);

	freq_semaphore = xSemaphoreCreateBinary();


	/* The RegTest tasks as described at the top of this file. */
	xTaskCreate( prvFirstRegTestTask, "Rreg1", configMINIMAL_STACK_SIZE, mainREG_TEST_1_PARAMETER, mainREG_TEST_PRIORITY, NULL);
	xTaskCreate( prvSecondRegTestTask, "Rreg2", configMINIMAL_STACK_SIZE, mainREG_TEST_2_PARAMETER, mainREG_TEST_PRIORITY, NULL);

	/* Finally start the scheduler. */
	vTaskStartScheduler();

	/* Will only reach here if there is insufficient heap available to start
	 the scheduler. */
	for (;;);
}
static void prvFirstRegTestTask(void *pvParameters)
{
	while (1)
	{
		printf("Task 1\n");
		vTaskDelay(1000);
		if (xSemaphoreTake(freq_semaphore,portMAX_DELAY) == pdTRUE) {
			printf("found\n");
		}
	}

}
static void prvSecondRegTestTask(void *pvParameters)
{
	while (1)
	{
		printf("Task 2\n");
		vTaskDelay(1000);
	}
}
