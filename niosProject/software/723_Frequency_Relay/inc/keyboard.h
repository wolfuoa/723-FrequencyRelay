// keyboard.h
//      author: Nicholas Wolf

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "FreeRTOS/queue.h"
#include "FreeRTOS/semphr.h"

int Keyboard_initIRQ(void *receiver);
int Keyboard_init();

#endif // KEYBOARD_H