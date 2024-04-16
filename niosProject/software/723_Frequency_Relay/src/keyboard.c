// keyboard.c
//      author: Nicholas Wolf

#include <stdio.h>
#include <unistd.h>
#include "system.h"
#include "altera_up_avalon_ps2.h"
#include "altera_up_ps2_keyboard.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/queue.h"
#include "FreeRTOS/semphr.h"

#include "inc/keyboard.h"
#include "inc/keyboard_common.h"
#include "inc/peak_detector.h"

#define KEYBOARD_HANDLER_PRIORITY (tskIDLE_PRIORITY + 1)

#define KEYBOARD_Q_TYPE unsigned char
#define KEYBOARD_Q_SIZE 10

alt_up_ps2_dev *Keyboard_ps2Dev;

QueueHandle_t Keyboard_Q;

typedef enum KEYBOARD_ENTRY_STATUS_T
{
    KEYBOARD_STATE_TYPING,
    KEYBOARD_STATE_WAITING,
    KEYBOARD_STATE_WAITING2,
    KEYBOARD_STATE_IDLE

} KEYBOARD_ENTRY_STATUS_T;

static void
Keyboard_initDataStructs();
static void Keyboard_handlerTask(void *pvParameters);
static void Keyboard_initIRQ(unsigned char *receiver);
static int isEntryOk(unsigned char *buf, int len);

void Keyboard_ISR(void *ctx, alt_u32 id)
{
    unsigned char entry;
    char refToMem;
    KB_CODE_TYPE mode;

    int status = decode_scancode(ctx, &mode, &entry, &refToMem);
    if (status == 0)
    {
        switch (mode)
        {
        case KB_ASCII_MAKE_CODE:
            //            printf("ENTRY: %d\n", entry);
            xQueueSendToBackFromISR(Keyboard_Q, &entry, pdFALSE);
            break;
        case KB_LONG_BINARY_MAKE_CODE:
            break;
        case KB_BINARY_MAKE_CODE:
            //            printf("ENTRY: %d\n", entry);
            xQueueSendToBackFromISR(Keyboard_Q, &entry, pdFALSE);
            break;
        case KB_BREAK_CODE:
            break;
        default:
            break;
        }
    }
}

static void Keyboard_initIRQ(unsigned char *receiver)
{
    alt_up_ps2_enable_read_interrupt(Keyboard_ps2Dev);
    alt_irq_register(PS2_IRQ, Keyboard_ps2Dev, Keyboard_ISR);
    IOWR_8DIRECT(PS2_BASE, 4, 1);
}

int Keyboard_init()
{
    Keyboard_initDataStructs();

    if (Keyboard_ps2Dev == NULL)
    {
        return 1;
    }

    alt_up_ps2_clear_fifo(Keyboard_ps2Dev);

    unsigned char ctx;
    Keyboard_initIRQ(&ctx);

    if (xTaskCreate(Keyboard_handlerTask, "Keyboard_T", configMINIMAL_STACK_SIZE,
                    NULL, KEYBOARD_HANDLER_PRIORITY, NULL) != pdPASS)
    {
        return 1;
    }

    return 0;
}

static void Keyboard_handlerTask(void *pvParameters)
{
    KEYBOARD_ENTRY_STATUS_T keyboardStatus;
    unsigned char buf[10];
    unsigned char resultStr[10];
    int changeEntry;
    int len = 0;
    int idx = 0;

    unsigned char scanCode;
    while (1)
    {
        if (xQueueReceive(Keyboard_Q, &scanCode, portMAX_DELAY) == pdTRUE)
        {
            if (keyboardStatus == KEYBOARD_STATE_WAITING2)
            {
                if (scanCode == key_U)
                {
                    printf("Updated Upper Frequency Threshold...\n");
                    // Set upper threshold
                    if (xSemaphoreTake(Peak_Detector_thresholdMutex_X, (TickType_t)portMAX_DELAY) == pdTRUE)
                    {
                        g_peakDetectorHigherFrequencyThreshold = (double)changeEntry;
                        xSemaphoreGive(Peak_Detector_thresholdMutex_X);
                    }
                    keyboardStatus = KEYBOARD_STATE_IDLE;
                }
                else if (scanCode == key_L)
                {
                    printf("Updated Lower Frequency Threshold...\n");
                    // Set lower threshold
                    if (xSemaphoreTake(Peak_Detector_thresholdMutex_X, (TickType_t)portMAX_DELAY) == pdTRUE)
                    {
                        g_peakDetectorLowerFrequencyThreshold = (double)changeEntry;
                        xSemaphoreGive(Peak_Detector_thresholdMutex_X);
                    }
                    keyboardStatus = KEYBOARD_STATE_IDLE;
                }
                else if (scanCode == key_Esc)
                {
                    printf("Cancelled Request...\n");
                    keyboardStatus = KEYBOARD_STATE_IDLE;
                }
            }
            else if (keyboardStatus == KEYBOARD_STATE_WAITING)
            {
                if (scanCode == key_F)
                {
                    printf("Set Upper Threshold - (U), Set Lower Threshold - (L), Cancel Request - (Esc)\n");
                    keyboardStatus = KEYBOARD_STATE_WAITING2;
                }
                else if (scanCode == key_R)
                {
                    printf("Updated ABS ROC...\n");
                    // Set ROC
                    if (xSemaphoreTake(Peak_Detector_thresholdMutex_X, (TickType_t)portMAX_DELAY) == pdTRUE)
                    {
                        g_peakDetectorHigherROCThreshold = (double)changeEntry;
                        g_peakDetectorLowerROCThreshold = -(double)changeEntry;
                        xSemaphoreGive(Peak_Detector_thresholdMutex_X);
                    }
                    keyboardStatus = KEYBOARD_STATE_IDLE;
                }
                else if (scanCode == key_Esc)
                {
                    printf("Cancelled Request...\n");
                    keyboardStatus = KEYBOARD_STATE_IDLE;
                }
            }
            else if (scanCode == returnKey)
            {
                for (int i = 0; i < len; ++i)
                {
                    idx += sprintf(&resultStr[idx], "%d", buf[i]);
                }

                changeEntry = atoi(resultStr);
                printf("%d\n", changeEntry);

                printf("ENTERED: %s\n", resultStr);

                if (isEntryOk(buf, len))
                {
                    printf("Error: Number out of bounds\n");
                }
                else
                {
                    printf("Set Frequency Threshold - (F), Set ABS ROC Threshold - (R), Cancel Request - (Esc)\n");
                    keyboardStatus = KEYBOARD_STATE_WAITING;
                }
                len = 0;
                idx = 0;
                resultStr[0] = '\0';
            }
            else
            {
                for (int i = 0; i < 10; ++i)
                {
                    if (scanCode == keyCodes[i])
                    {
                        buf[len] = i;
                        len++;

                        break;
                    }
                }
            }
        }
    }
}

static void Keyboard_initDataStructs()
{
    Keyboard_ps2Dev = alt_up_ps2_open_dev(PS2_NAME);
    Keyboard_Q = xQueueCreate(KEYBOARD_Q_SIZE, sizeof(KEYBOARD_Q_TYPE));
}

static int isEntryOk(unsigned char *buf, int len)
{
    if (len >= 3)
    {
        return 1;
    }
    if (buf[0] > 5 && len != 1)
    {
        return 1;
    }
    return 0;
}
