/*
 * COMPSYS 723 ASSIGNMENT 1
 * Household Frequency Relay
 * 		Author: Nicholas Wolf, Robert Sefaj, Janith Hettiarachchi
 *
 * This project sees through the development of a household frequency relay
 * capable of disconnecting loads when power singal conditions
 */

#include "../software/FreeRTOS/FreeRTOS.h"
#include "../software/FreeRTOS/task.h"
#include "altera_avalon_pio_regs.h"
#include "system.h"
#include <stdio.h>

#include "timer.h"

TimerHandle_t timer;
TaskHandle_t Timer_Reset;

int main() {

  timer = xTimerCreate("Timer Name", 1000, pdTRUE, NULL, vTimerCallback);

  if (xTimerStart(timer, 0) != pdPASS) {
    printf("ERROR: Failed to start timer");
    return 1;
  }

  vTaskStartScheduler();

  // Program will never reach this point
  while (1) {
  }

  return 0;
}
