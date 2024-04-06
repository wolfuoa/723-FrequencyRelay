// peak_detector.h

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"

#include "inc/peak_detector.h"

#define PEAK_DETECTOR_Q_SIZE 100
#define PEAK_DETECTOR_Q_TYPE float

#define PEAK_DETECTOR_HANDLER_PRIORITY (tskIDLE_PRIORITY + 1)

QueueHandle_t Peak_Detector_Q;

static void Peak_Detector_handlerTask(void *pvParameters);
static void Peak_Detector_initDataStructs();

int Peak_Detector_init()
{
    Peak_Detector_initDataStructs();
    if (xTaskCreate(Peak_Detector_handlerTask, "Peak_Detector_T", configMINIMAL_STACK_SIZE, NULL, PEAK_DETECTOR_HANDLER_PRIORITY, NULL) != pdPASS)
    {
        return 1;
    }
    return 0;
}

static void Peak_Detector_handlerTask(void *pvParameters)
{
    int frequencyReading;
    while (1)
    {
        if (xQueueReceive(Peak_Detector_Q, &frequencyReading, portMAX_DELAY) == pdTRUE)
        {
            printf("Reading: %f Hz\n", 16000 / (double)frequencyReading);
        }
    }
}

static void Peak_Detector_initDataStructs()
{
    Peak_Detector_Q = xQueueCreate(PEAK_DETECTOR_Q_SIZE, sizeof(PEAK_DETECTOR_Q_TYPE));
}