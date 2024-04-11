// freqency_analyser.c
//      author: Nicholas Wolf

// MUST GO BEFORE OTHERS
#include "FreeRTOS/FreeRTOS.h"

#include "inc/frequency_analyser.h"
#include <altera_avalon_pio_regs.h>
#include <sys/alt_irq.h>

#include "inc/peak_detector.h"

// Define the external variable ONCE in one src file. Preferably the one it
// relates to the most
SemaphoreHandle_t freq_semaphore;

// Dont declare this in the header file
// Name the ISR something with ISR in the name
// Here I am using the header/src domain at the front so we know exactly where
// the function is from
void Frequency_Analyser_ISR(void *ctx, alt_u32 id) {
  // Please use extended macros like IORD_ALTERA_AVALON_PIO_DATA for clarity
  unsigned int temp = IORD_ALTERA_AVALON_PIO_DATA(FREQUENCY_ANALYSER_BASE);
  xQueueSendToBackFromISR(Peak_Detector_Q, &temp, pdFALSE);
  // BaseType_t handle = pdFALSE;
  // xSemaphoreGiveFromISR(freq_semaphore, &handle);
  return;
}

// Most of the following code doesn't do anything, but it is just to be safe and
// explicit with what were doing
int Frequency_Analyser_initIRQ(int *receiver) {
  printf("Creating Frequency ISR\n");
  void *ctx = (void *)receiver;
  IOWR_ALTERA_AVALON_PIO_IRQ_MASK(FREQUENCY_ANALYSER_BASE, 0xF);
  // Make sure to use FREQUENCY_ANALYSER_IRQ, not FREQUENCY_ANALYSER_BASE
  // Pass the ctx in if needed, if not, use 0. If NULL, the ISR wont work
  if (alt_irq_register(FREQUENCY_ANALYSER_IRQ, ctx, Frequency_Analyser_ISR) !=
      0) {
    // Fail
    return 1;
  }
  alt_irq_enable(FREQUENCY_ANALYSER_IRQ);
  return 0;
}
