// button.h
//      author: Nicholas Wolf

#ifndef BUTTON_H
#define BUTTON_H

#include "FreeRTOS/queue.h"
#include "FreeRTOS/semphr.h"

int Button_initIRQ(int *receiver);
int Button_init();

#endif // BUTTON_H