// freqency_analyser.c

#ifndef FREQUENCY_ANALYSER_H
#define FREQUENCY_ANALYSER_H

// In the other file, I include FreeRTOS.h before this file, including FreeRtos.h before semphr.h. Very important
#include "FreeRTOS/semphr.h"

extern SemaphoreHandle_t freq_semaphore;

int Frequency_Analyser_initIRQ(int *receiver);

#endif // FREQUENCY_ANALYSER_H