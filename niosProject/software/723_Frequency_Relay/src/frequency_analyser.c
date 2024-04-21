// freqency_analyser.c
//      author: Nicholas Wolf

// MUST GO BEFORE OTHERS
#include "FreeRTOS/FreeRTOS.h"

#include "inc/frequency_analyser.h"
#include <sys/alt_irq.h>
#include <altera_avalon_pio_regs.h>

#include "inc/peak_detector.h"

SemaphoreHandle_t freq_semaphore;
int g_freqAnalyserExampleGlobalTimestamp;

void Frequency_Analyser_ISR(void *ctx, alt_u32 id)
{
    g_freqAnalyserExampleGlobalTimestamp = alt_timestamp();
    // Read the value at the frequency analyser address and sent to peak detector queue
    unsigned int temp = IORD_ALTERA_AVALON_PIO_DATA(FREQUENCY_ANALYSER_BASE);
    xQueueSendToBackFromISR(Peak_Detector_Q, &temp, pdFALSE);
    return;
}

int Frequency_Analyser_initIRQ(int *receiver)
{
    alt_timestamp_start();
    printf("Creating ISR\n");
    void *ctx = (void *)receiver;
    //Enable Interupts for the frequency analyser
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(FREQUENCY_ANALYSER_BASE, 0xF);
    // Pass the ctx in if needed, if not, use 0. If NULL, the ISR wont work
    if (alt_irq_register(FREQUENCY_ANALYSER_IRQ, ctx, Frequency_Analyser_ISR) != 0)
    {
        // Fail
        return 1;
    }
    alt_irq_enable(FREQUENCY_ANALYSER_IRQ);
    return 0;
}
