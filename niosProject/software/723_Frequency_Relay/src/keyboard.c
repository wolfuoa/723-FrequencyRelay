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

#include "inc/keyboard.h"
#include "inc/keyboard_common.h"

#define KEYBOARD_HANDLER_PRIORITY (tskIDLE_PRIORITY + 1)

#define KEYBOARD_Q_TYPE unsigned char
#define KEYBOARD_Q_SIZE 10

alt_up_ps2_dev *Keyboard_ps2Dev;

QueueHandle_t Keyboard_Q;

static void Keyboard_initDataStructs();
static void Keyboard_handlerTask(void *pvParameters);
static void Keyboard_initIRQ(unsigned char *receiver);

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
            xQueueSendToBackFromISR(Keyboard_Q, &entry, pdFALSE);
            break;
        case KB_LONG_BINARY_MAKE_CODE:
            // do nothing
        case KB_BINARY_MAKE_CODE:
            xQueueSendToBackFromISR(Keyboard_Q, &entry, pdFALSE);
            break;
        case KB_BREAK_CODE:
            // do nothing
        default:
            // printf("DEFAULT   : %d\n", entry);
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
    unsigned char scanCode;
    while (1)
    {
        if (xQueueReceive(Keyboard_Q, &scanCode, portMAX_DELAY) == pdTRUE)
        {
            if (scanCode == returnKey)
            {
                printf("RETURN\n");
            }
            for (int i = 0; i < 10; ++i)
            {
                if (scanCode == keyCodes[i])
                {
                    printf("Number: %d\n", i);
                    break;
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
