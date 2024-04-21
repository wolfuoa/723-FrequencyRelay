// peak_detector.h
//      author: Nicholas Wolf

#include <stdbool.h>

#include <sys/alt_timestamp.h>

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
            // Put delta frequency readings into an array, this will be used to calculate an averaged ROC
            frequencyDeltaBuf[frequencyBufIndex] = frequencyReading - previousFrequency;
            frequencyBufIndex = (frequencyBufIndex < 10) ? frequencyBufIndex + 1 : 0;
            // Read the timestamp for ROC calculation
            timestampForROC = xTaskGetTickCount();
            // Calculate the rate of change of frequency - Hz/s
            rateOfChangeReading = 0;
            for (int j = 0; j < 10; ++j){
                rateOfChangeReading += frequencyDeltaBuf[j];
            }
            rateOfChangeReading = 1000 * (rateOfChangeReading / 10) / (double)(timestampForROC - previousTimestamp);
            previousTimestamp = timestampForROC;
            // Replace previousFrequency
            previousFrequency = frequencyReading;

            VGA_Stats currentVGAStats = {frequencyReading,
                                rateOfChangeReading};

            // Send the frequency and ROC to the VGATask via the VGAStats Queue
            xQueueSendToBack(Q_VGA_Stats, &currentVGAStats, pdFALSE);

            VGA_Thresholds currentThresholds = {g_peakDetectorLowerFrequencyThreshold,
                	                    g_peakDetectorHigherFrequencyThreshold,
                	                    g_peakDetectorLowerROCThreshold,
                	                    g_peakDetectorHigherROCThreshold};

            // Dont change the thresholds while we are checking the thresholds
            if (xSemaphoreTake(Peak_Detector_thresholdMutex_X, (TickType_t)10) == pdTRUE)
            {
                // Determine stability of system and send to VGATask via Thresholds Queue
                thresholdEval = ((frequencyReading > g_peakDetectorLowerFrequencyThreshold) && (frequencyReading < g_peakDetectorHigherFrequencyThreshold) && (rateOfChangeReading >= g_peakDetectorLowerROCThreshold) && (rateOfChangeReading < g_peakDetectorHigherROCThreshold));
                xQueueSendToBack(Q_Threshhold, &currentThresholds, pdFALSE);
                xSemaphoreGive(Peak_Detector_thresholdMutex_X);
            }

            // Dont check the system status while the timer callback is running or if the load control is
            // changing the peak detector debounce flag.
            if (xSemaphoreTake(repeatActionMutex_X, (TickType_t)10) == pdTRUE)
            {
                if (xSemaphoreTake(Peak_Detector_debounceMutex_X, (TickType_t)10) == pdTRUE)
                {
                    // If system status is stable and goes outside threshold for the first time (first shed)
                    if ((systemStability == SYSTEM_FREQUENCY_STATE_STABLE) && (thresholdEval == SYSTEM_FREQUENCY_STATE_UNSTABLE) && g_peakDetectorDebounceFlag == 0)
                    {
                        // Set the flag indicating a first shed
                        g_peakDetectorDebounceFlag = 1;
                        // Write the time measurement takenn from the frequency analyser ISR into a global variable.
                        if(xSemaphoreTake(Peak_Detector_performanceTimerMutex_X, (TickType_t)10) == pdTRUE)
                        {
                            g_peakDetectorPerformanceTimestamp = g_freqAnalyserExampleGlobalTimestamp;
                            xSemaphoreGive(Peak_Detector_performanceTimerMutex_X);
                        }
                        // Send the system stability to the load control queue
                        xQueueSendToBack(Load_Control_Q, &thresholdEval, pdFALSE);
                        // Reset the timer as there is no need to repeat the action
                        repeatActionTimeout = false;
                        if (xTimerStart(repeatActionTimer, 0) != pdPASS)
                        {
                            printf("Cannot reset timer\n\r");
                        }
                    }
                    // If the system status switches from unstable to stable in less than 500ms, reset the timer.
                    else if ((systemStability == SYSTEM_FREQUENCY_STATE_UNSTABLE) && (thresholdEval == SYSTEM_FREQUENCY_STATE_STABLE))
                    {
                        repeatActionTimeout = false;
                        if (xTimerStart(repeatActionTimer, 0) != pdPASS)
                        {
                            printf("Cannot reset timer\n\r");
                        }
                    }
                    // If the timer has expired and the system is unstable
                    else if ((repeatActionTimeout) && (thresholdEval == SYSTEM_FREQUENCY_STATE_UNSTABLE))
                    {
                        // Send the system stability to the load control queue
                        xQueueSendToBack(Load_Control_Q, &thresholdEval, pdFALSE);
                        // Reset the timer as there is no need to repeat the action
                        repeatActionTimeout = false;
                        if (xTimerStart(repeatActionTimer, 0) != pdPASS)
                        {
                            printf("Cannot reset timer\n\r");
                        }
                    }
                    // If the timer has expired and the system is stable
                    else if ((repeatActionTimeout) && (thresholdEval == SYSTEM_FREQUENCY_STATE_STABLE))
                    {
                        // Send the system stability to the load control queue
                        xQueueSendToBack(Load_Control_Q, &thresholdEval, pdFALSE);
                        // Reset the timer as there is no need to repeat the action
                        repeatActionTimeout = false;
                        if (xTimerStart(repeatActionTimer, 0) != pdPASS)
                        {
                            printf("Cannot reset timer\n\r");
                        }
                    }
                    xSemaphoreGive(Peak_Detector_debounceMutex_X);
                }
                // Set the ssystem stability to the evaluated value.
                systemStability = thresholdEval;
                // Send the system stability to the VGATask through the SystemStatus queue.
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