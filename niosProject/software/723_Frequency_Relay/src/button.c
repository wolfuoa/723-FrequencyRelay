// button.c
//      author: Nicholas Wolf

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/queue.h"

#include <altera_avalon_pio_regs.h>
#include <system.h>
#include <sys/alt_irq.h>

#include "inc/button.h"

#define BUTTON_Q_SIZE 10
#define BUTTON_Q_TYPE int

#define BUTTON_HANDLER_PRIORITY (tskIDLE_PRIORITY + 2)

QueueHandle_t Button_Q;

System_Status_T SystemStatus;

static void Button_handlerTask(void *pvParameters);
static void Button_initDataStructs();

void Button_ISR(void *ctx, alt_u32 id)
{
    // return;
    int temp = IORD_ALTERA_AVALON_PIO_EDGE_CAP(PUSH_BUTTON_BASE);
    xQueueSendToBackFromISR(Button_Q, &temp, pdFALSE);

    // clears the edge capture register
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PUSH_BUTTON_BASE, 0x7);
}

int Button_init()
{
    Button_initDataStructs();
    if (xTaskCreate(Button_handlerTask, "Button_T", configMINIMAL_STACK_SIZE,
                    NULL, BUTTON_HANDLER_PRIORITY, NULL) != pdPASS)
    {
        return 1;
    }
    return 0;
}

static void Button_handlerTask(void *pvParameters)
{
    int buttonVal;
    while (1)
    {
        xQueueReceive(Button_Q, &buttonVal, portMAX_DELAY);

        if (buttonVal != 0)
        {
            if (xSemaphoreTake(SystemStatusMutex, (TickType_t)10) == pdTRUE)
            {
                if (buttonVal == 4)
                {
                    SystemStatus = SYSTEM_MAINTENANCE;
                } 
                else if (buttonVal == 2)
                {
                    SystemStatus = SYSTEM_MANAGING;
                } 

                buttonVal = 0;

                xSemaphoreGive(SystemStatusMutex); 
            }
        }
    }
}

static void Button_initDataStructs()
{
    Button_Q = xQueueCreate(BUTTON_Q_SIZE, sizeof(BUTTON_Q_TYPE));
}

int Button_initIRQ(int *receiver)
{
    printf("Creating Button ISR\n");
    void *ctx = (void *)receiver;
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PUSH_BUTTON_BASE, 0);
    // enable interrupts for KEY3 and KEY2 {0110}
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(PUSH_BUTTON_BASE, 0x6);
    // Register ISR into LUT
    if (alt_irq_register(PUSH_BUTTON_IRQ, ctx, Button_ISR) != 0)
    {
        // Fail
        return 1;
    }

    return 0;
}
