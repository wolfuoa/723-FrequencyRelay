
// VGA.h
// Created on: 12/04/2024
// Author: skend

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

int VGA_Init();

#endif /* INC_VGA_H */
