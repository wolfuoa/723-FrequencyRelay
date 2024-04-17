/*

VGA.h*
Created on: 12/04/2024
Author: skend
*/

#ifndef INCVGA_H
#define INCVGA_H

#include <stdio.h>
#include <stdlib.h>
#include "sys/alt_irq.h"
#include "system.h"
#include "io.h"
#include "altera_up_avalon_video_character_buffer_with_dma.h"
#include "altera_up_avalon_video_pixel_buffer_dma.h"

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/queue.h"

#include "inc/peak_detector.h"



extern QueueHandle_t Q_VGA_Stats;
extern QueueHandle_t Q_Threshhold;
extern QueueHandle_t Q_SystemStatus;
extern QueueHandle_t Q_PerformanceMeasure;


typedef struct VGA_Stats
{
    double currentFrequency;
    double currentROC;
} VGA_Stats;

typedef struct VGA_Thresholds
{
	double peakDetectorLowerFrequencyThreshold;
	double peakDetectorHigherFrequencyThreshold;
	double peakDetectorLowerROCThreshold;
	double peakDetectorHigherROCThreshold;
} VGA_Thresholds;

int VGA_Init();

#endif /* INC_VGA_H */
