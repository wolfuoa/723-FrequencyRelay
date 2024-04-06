// peak_detector.h

#ifndef PEAK_DETECTOR_H
#define PEAK_DETECTOR_H

#include "FreeRTOS/queue.h"

extern QueueHandle_t Peak_Detector_Q;

int Peak_Detector_init();

#endif // PEAK_DETECTOR_H