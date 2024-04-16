// peak_detector.h
//      author: Nicholas Wolf

#ifndef PEAK_DETECTOR_H
#define PEAK_DETECTOR_H

#include "FreeRTOS/queue.h"
#include "FreeRTOS/semphr.h"

extern QueueHandle_t Peak_Detector_Q;
extern SemaphoreHandle_t Peak_Detector_thresholdMutex_X;

extern double g_peakDetectorLowerFrequencyThreshold;  // Hz
extern double g_peakDetectorHigherFrequencyThreshold; // Hz
extern double g_peakDetectorLowerROCThreshold;        // Hz
extern double g_peakDetectorHigherROCThreshold;       // Hz

int Peak_Detector_init();

#endif // PEAK_DETECTOR_H