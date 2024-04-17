// peak_detector.h
//      author: Nicholas Wolf

#ifndef PEAK_DETECTOR_H
#define PEAK_DETECTOR_H

#include "FreeRTOS/queue.h"
#include "FreeRTOS/semphr.h"
#include "sys/alt_timestamp.h"

extern QueueHandle_t Peak_Detector_Q;
extern SemaphoreHandle_t Peak_Detector_thresholdMutex_X;
extern SemaphoreHandle_t Peak_Detector_performanceTimerMutex_X;
extern SemaphoreHandle_t Peak_Detector_debounceMutex_X;

extern double g_peakDetectorLowerFrequencyThreshold;  // Hz
extern double g_peakDetectorHigherFrequencyThreshold; // Hz
extern double g_peakDetectorLowerROCThreshold;        // Hz/s
extern double g_peakDetectorHigherROCThreshold;       // Hz/s

extern int g_peakDetectorPerformanceTimestamp;
extern int g_peakDetectorDebounceFlag;

typedef enum System_Frequency_State_T {
  SYSTEM_FREQUENCY_STATE_UNSTABLE = 0,
  SYSTEM_FREQUENCY_STATE_STABLE = 1
} System_Frequency_State_T;

int Peak_Detector_init();

#endif // PEAK_DETECTOR_H
