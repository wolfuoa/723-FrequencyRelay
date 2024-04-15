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

#include <sys/alt_irq.h>
#include <io.h>
#include <altera_avalon_pio_regs.h>

#include "inc/frequency_analyser.h"
#include "inc/peak_detector.h"
#include "inc/load_control.h"
#include "inc/button.h"
#include "inc/switch_polling.h"

/*
 * Create the demo tasks then start the scheduler.
 */
int main(void)
{

	// Context is not required, but this is an example of how to do it int freqCtx;
	int freqCtx;
	if (Frequency_Analyser_initIRQ(&freqCtx))
	{
		printf("Could not register Frequency Analyser ISR\n");
	}

	int buttonCtx;
	if (Button_initIRQ(&buttonCtx))
	{
		printf("Could not register Button ISR\n");
	}

	if (Peak_Detector_init())
	{
		printf("Could not start Peak Detector Task");
	}

	if (Load_Control_Init())
	{
		printf("Could not start Load Control Task");
	}

	if (switch_polling_init())
	{
		printf("Could not start Load Control Task");
	}

	if (Button_init())
	{
		printf("Could not start Button Task");
	}

	/* Finally start the scheduler. */
	vTaskStartScheduler();
	/* Will only reach here if there is insufficient heap available to start
	 the scheduler. */
	for (;;)
		;
}
