// peak_detector.h
//      author: Nicholas Wolf

#ifndef PEAK_DETECTOR_H
#define PEAK_DETECTOR_H

#include "FreeRTOS/queue.h"
#include "FreeRTOS/semphr.h"

extern QueueHandle_t Peak_Detector_Q;
extern SemaphoreHandle_t Peak_Detector_thresholdMutex_X;

extern float g_peakDetectorLowerFrequencyThreshold;  // Hz
extern float g_peakDetectorHigherFrequencyThreshold; // Hz
extern float g_peakDetectorLowerROCThreshold;        // Hz
extern float g_peakDetectorHigherROCThreshold;       // Hz

int Peak_Detector_init();

#endif // PEAK_DETECTOR_H