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

/*
 * Create the demo tasks then start the scheduler.
 */
int main(void)
{

	// Context is not required, but this is an example of how to do it
	int ctx;
	//Enable the frequency analyser IRQ
	if (Frequency_Analyser_initIRQ(&ctx))
	{
		printf("Could not register Frequency Analyser ISR\n");
	}

	if (Peak_Detector_init())
	{
		printf("Could not start Peak Detector Task");
	}

	if (Load_Control_Init())
	{
		printf("Could not start Load Control Task");
	}
	if (Switch_Polling_init()){
		printf("Could not start Load Control Task");
	}


	/* Finally start the scheduler. */
	vTaskStartScheduler();
	/* Will only reach here if there is insufficient heap available to start
	 the scheduler. */
	for (;;)
		;
}

