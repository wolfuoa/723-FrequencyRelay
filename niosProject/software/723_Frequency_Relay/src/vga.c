/*
 * VGA.c
 *
 *  Created on: 12/04/2024
 *      Author: skend
 */
/* Standard includes. */
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "inc/VGA.h"

void PRVGADraw_Task(void *pvParameters);
void freq_relay();

// For frequency plot
#define FREQPLT_ORI_X 101     // x axis pixel position at the plot origin
#define FREQPLT_GRID_SIZE_X 80 // pixel separation in the x axis between two data points
#define FREQPLT_ORI_Y 199.0   // y axis pixel position at the plot origin
#define FREQPLT_FREQ_RES 20.0 // number of pixels per Hz (y axis scale)

#define ROCPLT_ORI_X 101
#define ROCPLT_GRID_SIZE_X 80
#define ROCPLT_ORI_Y 259.0
#define ROCPLT_ROC_RES 0.5 // number of pixels per Hz/s (y axis scale)

#define MIN_FREQ 45.0 // minimum frequency to draw

#define PRVGADraw_Task_P (tskIDLE_PRIORITY + 1)
TaskHandle_t PRVGADraw;

QueueHandle_t Q_VGA_Stats;
QueueHandle_t Q_DFREQ;

typedef struct
{
    unsigned int x1;
    unsigned int y1;
    unsigned int x2;
    unsigned int y2;
} Line;

int VGA_Init()
{
    Q_VGA_Stats = xQueueCreate(100, sizeof(VGA_Stats));

    if (xTaskCreate(PRVGADraw_Task, "DrawTsk", configMINIMAL_STACK_SIZE, NULL, PRVGADraw_Task_P, &PRVGADraw) != pdPASS)
    {
        return 1;
    };
    return 0;
}

/****** VGA display ******/

void PRVGADraw_Task(void *pvParameters)
{

    // initialize VGA controllers
    alt_up_pixel_buffer_dma_dev *pixel_buf;
    pixel_buf = alt_up_pixel_buffer_dma_open_dev(VIDEO_PIXEL_BUFFER_DMA_NAME);
    if (pixel_buf == NULL)
    {
        printf("can't find pixel buffer device\n");
    }
    alt_up_pixel_buffer_dma_clear_screen(pixel_buf, 0);

    alt_up_char_buffer_dev *char_buf;
    char_buf = alt_up_char_buffer_open_dev("/dev/video_character_buffer_with_dma");
    if (char_buf == NULL)
    {
        printf("can't find char buffer device\n");
    }
    alt_up_char_buffer_clear(char_buf);

    // Set up plot axes
    alt_up_pixel_buffer_dma_draw_hline(pixel_buf, 100, 500, 200, ((0x3ff << 20) + (0x3ff << 10) + (0x3ff)), 0);
    alt_up_pixel_buffer_dma_draw_hline(pixel_buf, 100, 500, 300, ((0x3ff << 20) + (0x3ff << 10) + (0x3ff)), 0);
    alt_up_pixel_buffer_dma_draw_vline(pixel_buf, 100, 50, 200, ((0x3ff << 20) + (0x3ff << 10) + (0x3ff)), 0);
    alt_up_pixel_buffer_dma_draw_vline(pixel_buf, 100, 220, 300, ((0x3ff << 20) + (0x3ff << 10) + (0x3ff)), 0);

    alt_up_char_buffer_string(char_buf, "Frequency(Hz)", 4, 4);
    alt_up_char_buffer_string(char_buf, "52", 10, 7);
    alt_up_char_buffer_string(char_buf, "50", 10, 12);
    alt_up_char_buffer_string(char_buf, "48", 10, 17);
    alt_up_char_buffer_string(char_buf, "46", 10, 22);

    alt_up_char_buffer_string(char_buf, "df/dt(Hz/s)", 4, 26);
    alt_up_char_buffer_string(char_buf, "60", 10, 28);
    alt_up_char_buffer_string(char_buf, "30", 10, 30);
    alt_up_char_buffer_string(char_buf, "0", 10, 32);
    alt_up_char_buffer_string(char_buf, "-30", 9, 34);
    alt_up_char_buffer_string(char_buf, "-60", 9, 36);

    //Thresholds default displays
    alt_up_char_buffer_string(char_buf, "Lower Frequency Threshold:", 4, 41);

    double freq[100], dfreq[100];
    int i = 0, j = 0;
    Line line_freq, line_roc;
    VGA_Stats stats;

    while (1)
    {

        // receive frequency data from queue
        if (xQueueReceive(Q_VGA_Stats, &stats, portMAX_DELAY) == pdTRUE)
        {
            freq[i] = stats.currentFrequency;
            if (stats.currentROC > 10)
            {
                dfreq[i] = 0;
            }
            else
            {
                dfreq[i] = stats.currentROC * 100;
            }

            i = ++i % 100; // point to the next data (oldest) to be overwritten
        }

        // clear old graph to draw new graph
        alt_up_pixel_buffer_dma_draw_box(pixel_buf, 101, 0, 639, 199, 0, 0);
        alt_up_pixel_buffer_dma_draw_box(pixel_buf, 101, 201, 639, 299, 0, 0);

        for (j = 0; j <5; ++j)
        { // i here points to the oldest data, j loops through all the data to be drawn on VGA
            if (((int)(freq[(i + j) % 100]) > MIN_FREQ) && ((int)(freq[(i + j + 1) % 100]) > MIN_FREQ))
            {
                // Calculate coordinates of the two data points to draw a line in between
                // Frequency plot
                line_freq.x1 = FREQPLT_ORI_X + FREQPLT_GRID_SIZE_X * j;
                line_freq.y1 = (int)(FREQPLT_ORI_Y - FREQPLT_FREQ_RES * (freq[(i + j) % 100] - MIN_FREQ));

                line_freq.x2 = FREQPLT_ORI_X + FREQPLT_GRID_SIZE_X * (j + 1);
                line_freq.y2 = (int)(FREQPLT_ORI_Y - FREQPLT_FREQ_RES * (freq[(i + j + 1) % 100] - MIN_FREQ));

                // Frequency RoC plot
                line_roc.x1 = ROCPLT_ORI_X + ROCPLT_GRID_SIZE_X * j;
                line_roc.y1 = (int)(ROCPLT_ORI_Y - ROCPLT_ROC_RES * dfreq[(i + j) % 100]);

                line_roc.x2 = ROCPLT_ORI_X + ROCPLT_GRID_SIZE_X * (j + 1);
                line_roc.y2 = (int)(ROCPLT_ORI_Y - ROCPLT_ROC_RES * dfreq[(i + j + 1) % 100]);

                // Draw
                alt_up_pixel_buffer_dma_draw_line(pixel_buf, line_freq.x1, line_freq.y1, line_freq.x2, line_freq.y2, 0x3ff << 0, 0);
                alt_up_pixel_buffer_dma_draw_line(pixel_buf, line_roc.x1, line_roc.y1, line_roc.x2, line_roc.y2, 0x3ff << 0, 0);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
