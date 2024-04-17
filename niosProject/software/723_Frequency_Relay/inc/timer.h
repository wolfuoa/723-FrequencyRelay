/*
 * timer.h
 * 		Author: Nicholas Wolf
 *
 * Includes the required resources for timers and alarms
 */

#ifndef TIMER_H
#define TIMER_H

// #include "../723_Frequency_Relay/FreeRTOS/timers.h"
// #include "sys/alt_alarm.h"
// #include "system.h"
// #include <alt_types.h> // alt_u32 is a kind of alt_types
// #include <altera_avalon_pio_regs.h>
// #include <stdio.h>
// #include <sys/alt_irq.h> // to register interrupts

#define Timer_Reset_Task_P (tskIDLE_PRIORITY + 1)

// TIMER TIMEOUT HANDLERS
// void vTimerCallback(xTimerHandle t_timer);
// TIMER TIMEOUT HANDLERS

// void TIMER_alarm_start(alt_alarm *timer, int *timerCtx, int timeout);

#endif // TIMER_H
