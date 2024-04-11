// keyboard.c
//      author: Nicholas Wolf

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"

#include "inc/keyboard.h"

#define KEYBOARD_Q_SIZE 10
#define KEYBOARD_Q_TYPE unsigned char

#define KEYBOARD_HANDLER_PRIORITY (tskIDLE_PRIORITY + 1)

QueueHandle_t Keyboard_Q;

static void Keyboard_handlerTask(void *pvParameters);
static void Keyboard_initDataStructs();

void Keyboard_ISR(void *ctx, alt_u32 id) {
  unsigned int temp = IORD_ALTERA_AVALON_PIO_DATA(PS2_BASE);
  xQueueSendToBackFromISR(Keyboard_Q, &temp, pdFALSE);
}

int Keyboard_init() {
  Keyboard_initDataStructs();
  if (xTaskCreate(Keyboard_handlerTask, "Keyboard_T", configMINIMAL_STACK_SIZE,
                  NULL, KEYBOARD_HANDLER_PRIORITY, NULL) != pdPASS) {
    return 1;
  }
  return 0;
}

static void Keyboard_handlerTask(void *pvParameters) {
  unsigned char asciiReading;
  while (1) {
    if (xQueueReceive(Keyboard_Q, &asciiReading, portMAX_DELAY) == pdTRUE) {
      printf("Key Press: %c\n", asciiReading);
    }
  }
}

static void Keyboard_initDataStructs() {
  Keyboard_Q = xQueueCreate(KEYBOARD_Q_SIZE, sizeof(KEYBOARD_Q_TYPE));
}

int Keyboard_initIRQ(int *receiver) {
  printf("Creating Keyboard ISR\n");
  void *ctx = (void *)receiver;
  IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PS2_BASE, 0);
  // enable interrupts for last 3 buttons {0111}
  IOWR_ALTERA_AVALON_PIO_IRQ_MASK(PS2_BASE, 0x7);
  // Register ISR into LUT
  if (alt_irq_register(PS2_IRQ, ctx, Keyboard_ISR) != 0) {
    // Fail
    return 1;
  }
  alt_irq_enable(PS2_IRQ);
  return 0;
}