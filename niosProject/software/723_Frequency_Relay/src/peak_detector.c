// peak_detector.h
//      author: Nicholas Wolf

#include <stdbool.h>

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/timers.h"

#include "inc/peak_detector.h"
#include "inc/load_control.h"
#include "inc/vga.h"

#define PEAK_DETECTOR_Q_SIZE 100
#define PEAK_DETECTOR_Q_TYPE double

#define PEAK_DETECTOR_HANDLER_PRIORITY (tskIDLE_PRIORITY + 1)


// Global Variables
double g_peakDetectorLowerFrequencyThreshold = 49;  // Hz
double g_peakDetectorHigherFrequencyThreshold = 52; // Hz
double g_peakDetectorLowerROCThreshold = -10;        // Hz/s
double g_peakDetectorHigherROCThreshold = 10;        // Hz/s

int g_peakDetectorPerformanceTimestamp;
int g_peakDetectorDebounceFlag = 0;

// Global Data Structs
QueueHandle_t Peak_Detector_Q;
SemaphoreHandle_t Peak_Detector_thresholdMutex_X = NULL;
SemaphoreHandle_t Peak_Detector_performanceTimerMutex_X = NULL;
SemaphoreHandle_t Peak_Detector_debounceMutex_X = NULL;

// Protected Variables
TimerHandle_t repeatActionTimer;
TimerHandle_t debounceTimer;
SemaphoreHandle_t repeatActionMutex_X = NULL;
bool repeatActionTimeout = 0;
uint8_t janith = 0;

static void Peak_Detector_handlerTask(void *pvParameters);
static void Peak_Detector_initDataStructs();
static void repeatActionTimerCallback(TimerHandle_t t_timer);
static void debounceTimerCallback(TimerHandle_t t_timer);

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
    static double previousFrequency;
    static System_Frequency_State_T systemStability;
    static int previousTimestamp;

    double frequencyDeltaBuf[10];
    uint8_t frequencyBufIndex = 0;

    int temp;
    int timestampForROC;
    double frequencyReading;
    double rateOfChangeReading;
    System_Frequency_State_T thresholdEval;
    while (1)
    {
        // Read from Peak_Detector_Q
        if (xQueueReceive(Peak_Detector_Q, &temp, portMAX_DELAY) == pdTRUE)
        {
            // Calculate the instantaneous frequency
            frequencyReading = 16000 / (double)temp;
            // printf("Reading: %f Hz\n", frequencyReading);
            
            frequencyDeltaBuf[frequencyBufIndex] = frequencyReading - previousFrequency;
            frequencyBufIndex = (frequencyBufIndex < 10) ? frequencyBufIndex + 1 : 0;

            // Read the timestamp
            timestampForROC = xTaskGetTickCount();

            // Calculate the rate of change of frequency - Hz/s
            rateOfChangeReading = 0;
            for (int j = 0; j < 10; ++j){
                rateOfChangeReading += frequencyDeltaBuf[j];
            }
            rateOfChangeReading = 1000 * (rateOfChangeReading / 10) / (double)(timestampForROC - previousTimestamp);
            // rateOfChangeReading = 1000 * ((frequencyReading - previousFrequency) / (double)((timestampForROC - previousTimestamp)));
            
            // printf("Difference - %d, ROC: %f, index: %d\n", timestampForROC - previousTimestamp, rateOfChangeReading, frequencyBufIndex);
            previousTimestamp = timestampForROC;

            // Replace previousFrequency
            previousFrequency = frequencyReading;

            VGA_Stats currentVGAStats = {
                frequencyReading,
                rateOfChangeReading};

            // VGA Queue
            xQueueSendToBack(Q_VGA_Stats, &currentVGAStats, pdFALSE);

            // Dont change the thresholds while we are checking the thresholds
            if (xSemaphoreTake(Peak_Detector_thresholdMutex_X, (TickType_t)10) == pdTRUE)
            {
                //                printf("FreqL %f, FreqH %f, RocL %f, RocH %f\n", g_peakDetectorLowerFrequencyThreshold, g_peakDetectorHigherFrequencyThreshold, g_peakDetectorLowerROCThreshold, g_peakDetectorHigherROCThreshold);
                // Determine stability of system
                thresholdEval = ((frequencyReading > g_peakDetectorLowerFrequencyThreshold) && (frequencyReading < g_peakDetectorHigherFrequencyThreshold) && (rateOfChangeReading >= g_peakDetectorLowerROCThreshold) && (rateOfChangeReading < g_peakDetectorHigherROCThreshold));
                // printf("stable? %d\n", thresholdEval);

                //Testing: sending thresholds to VGA
                VGA_Thresholds currentThresholds = {
                	g_peakDetectorLowerFrequencyThreshold,  // Hz
                	g_peakDetectorHigherFrequencyThreshold, // Hz
                	g_peakDetectorLowerROCThreshold,        // Hz
                	g_peakDetectorHigherROCThreshold,       // Hz
                };
                xQueueSendToBack(Q_Threshhold, &currentThresholds, pdFALSE);

                xSemaphoreGive(Peak_Detector_thresholdMutex_X);
            }

            if (xSemaphoreTake(repeatActionMutex_X, (TickType_t)10) == pdTRUE)
            {

                if (xSemaphoreTake(Peak_Detector_debounceMutex_X, (TickType_t)10) == pdTRUE)
                {
                        // If system status is stable and goes outside threshold
                        if ((systemStability == SYSTEM_FREQUENCY_STATE_STABLE) && (thresholdEval == SYSTEM_FREQUENCY_STATE_UNSTABLE) && g_peakDetectorDebounceFlag == 0)
                        {
                            g_peakDetectorDebounceFlag = 1;

                            if(xSemaphoreTake(Peak_Detector_performanceTimerMutex_X, (TickType_t)10) == pdTRUE)
                            {
                                g_peakDetectorPerformanceTimestamp = xTaskGetTickCount();
                                xSemaphoreGive(Peak_Detector_performanceTimerMutex_X);
                            }
                            xQueueSendToBack(Load_Control_Q, &thresholdEval, pdFALSE);

                            // Reset the timer cus no need to repeat action
                            repeatActionTimeout = false;
                            if (xTimerStart(repeatActionTimer, 0) != pdPASS)
                            {
                                printf("Cannot reset timer\n\r");
                            }
                        }
                        // If system status is unstable and goes inside threshold
                        else if ((repeatActionTimeout) && (thresholdEval == SYSTEM_FREQUENCY_STATE_UNSTABLE))
                        {
                            xQueueSendToBack(Load_Control_Q, &thresholdEval, pdFALSE);

                            // Reset the timer cus no need to repeat action
                            repeatActionTimeout = false;
                            if (xTimerStart(repeatActionTimer, 0) != pdPASS)
                            {
                                printf("Cannot reset timer\n\r");
                            }
                        }
                    else if ((repeatActionTimeout) && (thresholdEval == SYSTEM_FREQUENCY_STATE_STABLE))
                    {
                    xQueueSendToBack(Load_Control_Q, &thresholdEval, pdFALSE);

                    // Reset the timer cus no need to repeat action
                    repeatActionTimeout = false;
                    if (xTimerStart(repeatActionTimer, 0) != pdPASS)
                    {
                        printf("Cannot reset timer\n\r");
                    }
                }
                    xSemaphoreGive(Peak_Detector_debounceMutex_X);
                }
                // Changing the systems status
                systemStability = thresholdEval;
                xQueueSendToBack(Q_SystemStatus, &systemStability, pdFALSE);

                xSemaphoreGive(repeatActionMutex_X);
            }
        }
    }
}

static void Peak_Detector_initDataStructs()
{
    repeatActionMutex_X = xSemaphoreCreateMutex();
    Peak_Detector_thresholdMutex_X = xSemaphoreCreateMutex();
    Peak_Detector_performanceTimerMutex_X = xSemaphoreCreateMutex();
    Peak_Detector_debounceMutex_X = xSemaphoreCreateMutex();
    // Important: 500 is the amount of ticks. To convert to ms, divide by portTICK_PERIOD_MS
    repeatActionTimer = xTimerCreate("Repeat_Action_Timer", 500 / portTICK_PERIOD_MS, pdTRUE, NULL, repeatActionTimerCallback);
    Peak_Detector_Q = xQueueCreate(PEAK_DETECTOR_Q_SIZE, sizeof(PEAK_DETECTOR_Q_TYPE));
}

static void repeatActionTimerCallback(TimerHandle_t t_timer)
{
    if (xSemaphoreTake(repeatActionMutex_X, (TickType_t)10) == pdTRUE)
    {
        repeatActionTimeout = true;
        xSemaphoreGive(repeatActionMutex_X);
    }
}