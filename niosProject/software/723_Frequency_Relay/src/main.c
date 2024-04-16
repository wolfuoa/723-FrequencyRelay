/*
 * 723 ASSIGNMENT 1 - FREQUENCY RELAY
 * 		authors: Nicholas Wolf, Janith Hetteriachchi, Robert Sefaj
 */

/* Scheduler includes. */
#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/queue.h"
#include "FreeRTOS/semphr.h"

#include "inc/frequency_analyser.h"
#include "inc/peak_detector.h"
#include "inc/load_control.h"

#include "inc/button.h"
#include "inc/switch_polling.h"
#include "inc/vga.h"
#include "inc/keyboard.h"

#include "inc/vga.h"

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
		printf("Could not start Peak Detector Task\n");
	}

	if (Load_Control_Init())
	{
		printf("Could not start Load Control Task");
	}
	if (Switch_Polling_init())
	{
		printf("Could not start Load Control Task");
	}
	if (VGA_Init())
	{
		printf("Could not VGA Task");
	}

	// if (switch_polling_init())
	// {
	// 	printf("Could not start Load Control Task\n");
	// }

	if (Button_init())
	{
		printf("Could not start Button Task\n");
	}

	if (VGA_Init())
	{
		printf("Could not start VGA Task\n");
	}

	if (Keyboard_init())
	{
		printf("Could not start Keyboard Task\n");
	}

	/* Finally start the scheduler. */
	vTaskStartScheduler();
	/* Will only reach here if there is insufficient heap available to start
	 the scheduler. */
	for (;;)
		;
}
