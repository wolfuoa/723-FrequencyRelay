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
#include "FreeRTOS/queue.h"
#include "FreeRTOS/semphr.h"
#include "FreeRTOS/task.h"

#include <altera_avalon_pio_regs.h>
#include <io.h>
#include <sys/alt_irq.h>

#include "inc/frequency_analyser.h"
#include "inc/load_control.h"
#include "inc/peak_detector.h"

/* The parameters passed to the reg test tasks.  This is just done to check
 the parameter passing mechanism is working correctly. */
// #define mainREG_TEST_1_PARAMETER ((void *)0x12345678)
// #define mainREG_TEST_2_PARAMETER ((void *)0x87654321)
// #define mainREG_TEST_PRIORITY (tskIDLE_PRIORITY + 1)
// static void prvFirstRegTestTask(void *pvParameters);
// static void prvSecondRegTestTask(void *pvParameters);

/*
 * Create the demo tasks then start the scheduler.
 */
int main(void) {

  // Context is not required, but this is an example of how to do it
  int freqCtx;
  if (Frequency_Analyser_initIRQ(&freqCtx)) {
    printf("Could not register Frequency Analyser ISR\n");
  }

  int buttonCtx;
  if (Button_initIRQ(&buttonCtx)) {
    printf("Could not register Button ISR\n");
  }

  int keyboardCtx;
  if (Keyboard_initIRQ(&keyboardCtx)) {
    printf("Could not register Keyboard ISR\n");
  }

  if (Peak_Detector_init()) {
    printf("Could not start Peak Detector Task");
  }

  if (Load_Control_init()) {
    printf("Could not start Load Control Task");
  }

  if (Button_init()) {
    printf("Could not start Button Task");
  }

  if (Keyboard_init()) {
    printf("Could not start Key Listener Task");
  }
  // freq_semaphore = xSemaphoreCreateBinary();

  /* The RegTest tasks as described at the top of this file. */
  // xTaskCreate(prvFirstRegTestTask, "Rreg1", configMINIMAL_STACK_SIZE,
  // mainREG_TEST_1_PARAMETER, mainREG_TEST_PRIORITY, NULL);
  // xTaskCreate(prvSecondRegTestTask, "Rreg2", configMINIMAL_STACK_SIZE,
  // mainREG_TEST_2_PARAMETER, mainREG_TEST_PRIORITY, NULL);

  /* Finally start the scheduler. */
  vTaskStartScheduler();
  /* Will only reach here if there is insufficient heap available to start
   the scheduler. */
  for (;;)
    ;
}
// static void prvFirstRegTestTask(void *pvParameters)
// {
// 	while (1)
// 	{
// 		printf("Task 1\n");
// 		vTaskDelay(1000);
// 		if (xSemaphoreTake(freq_semaphore, portMAX_DELAY) == pdTRUE)
// 		{
// 			printf("found\n");
// 		}
// 	}
// }
// static void prvSecondRegTestTask(void *pvParameters)
// {
// 	while (1)
// 	{
// 		printf("Task 2\n");
// 		vTaskDelay(1000);
// 	}
// }
