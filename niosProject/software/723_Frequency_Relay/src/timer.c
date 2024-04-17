/*
 * timer.c
 * 		Author: Nicholas Wolf
 *
 * Includes the required resources for timers and alarms
 */

#include "inc/timer.h"

// The function to call when timer times out. Implement as many as needed
// void vTimerCallback(xTimerHandle t_timer){
//	;
//}

// alt_u32 TIMER_interrupt(void *ctx)
//{
//   int *counter = (int *)ctx;
//   (*counter)++;
//   return TIMER_TIMEOUT_PERIOD;
// }
//
// alt_u32 TIMER_SR(void *ctx)
//{
//   IOWR_ALTERA_AVALON_PIO_DATA(LEDS_RED_BASE, 0);
// }
//
// void TIMER_alarm_start(alt_alarm *timer, int *timerCtx, int timeout)
//{
//   void *ctx = (void *)timerCtx;
//   TIMER_TIMEOUT_PERIOD = timeout;
//   alt_alarm_start(timer, timeout, TIMER_interrupt, ctx);
// }
