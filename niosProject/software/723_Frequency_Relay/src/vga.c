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
#define ROCPLT_ROC_RES 3 // number of pixels per Hz/s (y axis scale)

#define MIN_FREQ 45.0 // minimum frequency to draw

#define PRVGADraw_Task_P (tskIDLE_PRIORITY + 1)
TaskHandle_t PRVGADraw;

QueueHandle_t Q_VGA_Stats;
QueueHandle_t Q_Threshhold;
QueueHandle_t Q_SystemStatus;
QueueHandle_t Q_PerformanceMeasure;

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
    Q_Threshhold =  xQueueCreate(100, sizeof(VGA_Thresholds));
    Q_SystemStatus =  xQueueCreate(100, sizeof(int));
    Q_PerformanceMeasure =  xQueueCreate(100, sizeof(int));


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
    alt_up_char_buffer_string(char_buf, "10", 10, 28);
    alt_up_char_buffer_string(char_buf, "5", 10, 30);
    alt_up_char_buffer_string(char_buf, "0", 10, 32);
    alt_up_char_buffer_string(char_buf, "-5", 9, 34);
    alt_up_char_buffer_string(char_buf, "-10", 9, 36);

    //Thresholds default displays
    alt_up_char_buffer_string(char_buf, "- Freq Threshold:", 4, 41);
    alt_up_char_buffer_string(char_buf, "+ Freq Threshold:", 4, 44);

    alt_up_char_buffer_string(char_buf, "- ROC Threshold:", 4, 48);
    alt_up_char_buffer_string(char_buf, "+ ROC Threshold:", 4, 51);

    // System State
    alt_up_char_buffer_string(char_buf, "> System Status:", 34, 41);

    //current Freq and ROC value
    alt_up_char_buffer_string(char_buf, "> Current ROC:", 34, 44);
    alt_up_char_buffer_string(char_buf, "> Current Freq:", 34, 48);    

    // Performance measures
    alt_up_char_buffer_string(char_buf, "> Reaction time: ", 34, 51);
    alt_up_char_buffer_string(char_buf, "Avg Reaction time: ", 2, 55);
    alt_up_char_buffer_string(char_buf, "Max Reaction time: ", 28, 55);
    alt_up_char_buffer_string(char_buf, "Min Reaction time: ", 54, 55);

    double freq[100], dfreq[100];
    int i = 0, j = 0;
    Line line_freq, line_roc;
    VGA_Stats stats;

    VGA_Thresholds thresholdsToPrint;
    char ThreshStr[5];

    int performance_mesaure_to_print;
    int max_performance_time = 0;
    int min_performance_time = 0;
    int running_performance_total = 0;
    int numb_running_index = 0;


    System_Frequency_State_T currentVgaSystemStatus;

    while (1)
    {
    	//reciving the threshold queue
    	if (xQueueReceive(Q_Threshhold, &thresholdsToPrint, portMAX_DELAY) == pdTRUE){
    		sprintf(ThreshStr, "%.1f Hz", thresholdsToPrint.peakDetectorLowerFrequencyThreshold);
    		alt_up_char_buffer_string(char_buf, ThreshStr, 23, 41);

    		sprintf(ThreshStr, "%.1f Hz", thresholdsToPrint.peakDetectorHigherFrequencyThreshold);
    		alt_up_char_buffer_string(char_buf, ThreshStr, 23, 44);

    		sprintf(ThreshStr, "%.1f Hz", thresholdsToPrint.peakDetectorLowerROCThreshold);
    		alt_up_char_buffer_string(char_buf, ThreshStr, 23, 48);

    		sprintf(ThreshStr, "%.1f Hz", thresholdsToPrint.peakDetectorHigherROCThreshold);
    		alt_up_char_buffer_string(char_buf, ThreshStr, 23, 51);

    	}

        //reciving the performance value
    	if (xQueueReceive(Q_PerformanceMeasure, &performance_mesaure_to_print, (TickType_t)10) == pdTRUE){
            //update running average
            running_performance_total += performance_mesaure_to_print;
            numb_running_index++;

            if (performance_mesaure_to_print > max_performance_time){
                max_performance_time = performance_mesaure_to_print;
            }
            if (performance_mesaure_to_print < min_performance_time){
                min_performance_time = performance_mesaure_to_print;
            }

            // reaction time
            sprintf(ThreshStr, "%d ms", performance_mesaure_to_print);
            alt_up_char_buffer_string(char_buf, ThreshStr, 52, 51);

            // AVG reaction code
            alt_up_char_buffer_string(char_buf, "     ", 21, 55);
            sprintf(ThreshStr, "%d ms", (running_performance_total/numb_running_index));
            alt_up_char_buffer_string(char_buf, ThreshStr, 21, 55);

            //Max reaction code
            alt_up_char_buffer_string(char_buf, "     ", 47, 55);
            sprintf(ThreshStr, "%d ms", max_performance_time);
            alt_up_char_buffer_string(char_buf, ThreshStr, 47, 55);

            //Min reaction code
            alt_up_char_buffer_string(char_buf, "    ", 73, 55);
            sprintf(ThreshStr, "%d ms", min_performance_time);
            alt_up_char_buffer_string(char_buf, ThreshStr, 73, 55);
        }
        


    	//recieving the system status queue
    	if (xQueueReceive(Q_SystemStatus, &currentVgaSystemStatus, portMAX_DELAY) == pdTRUE){
    		switch(currentVgaSystemStatus){
    			case(SYSTEM_FREQUENCY_STATE_UNSTABLE):
						alt_up_char_buffer_string(char_buf, "Unstable", 50, 41);
    			break;
    			case(SYSTEM_FREQUENCY_STATE_STABLE):
                    //clear the previous value before placing the new one
					alt_up_char_buffer_string(char_buf, "         ", 50, 41);
					alt_up_char_buffer_string(char_buf, "Stable", 50, 41);
    		}
    	}


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
                dfreq[i] = stats.currentROC;
            }

            i = ++i % 100; // point to the next data (oldest) to be overwritten

            alt_up_char_buffer_string(char_buf, "            ", 50, 44);
            sprintf(ThreshStr, "%2.2f Hz/s", dfreq[i]);
    	    alt_up_char_buffer_string(char_buf, ThreshStr, 50, 44);

            alt_up_char_buffer_string(char_buf, "           ", 50, 48);
            sprintf(ThreshStr, "%2.2f Hz/s", freq[i]);
    	    alt_up_char_buffer_string(char_buf, ThreshStr, 50, 48);
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
