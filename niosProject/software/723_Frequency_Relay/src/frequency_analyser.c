// freqency_analyser.c

// MUST GO BEFORE OTHERS
#include "FreeRTOS/FreeRTOS.h"

#include "inc/frequency_analyser.h"
#include <sys/alt_irq.h>
#include <altera_avalon_pio_regs.h>

SemaphoreHandle_t freq_semaphore;

// Dont declare this in the header file
void freq_relay(void *ctx, alt_u32 id)
{
    unsigned int temp = IORD_ALTERA_AVALON_PIO_DATA(FREQUENCY_ANALYSER_BASE);
    printf("%f Hz\n", 16000 / (double)temp);
    BaseType_t handle = pdFALSE;
    xSemaphoreGiveFromISR(freq_semaphore, &handle);
    return;
}

int Frequency_Analyser_initIRQ(int *receiver)
{
    printf("Creating ISR\n");
    void *ctx = (void *)receiver;
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(FREQUENCY_ANALYSER_BASE, 0xF);
    if (alt_irq_register(FREQUENCY_ANALYSER_IRQ, ctx, freq_relay) != 0)
    {
        return 1;
    }
    alt_irq_enable(FREQUENCY_ANALYSER_IRQ);
    return 0;
}
