#include <Arduino.h>
#include "freertos/FreeRTOS.h"

TaskHandle_t xTaskHandleAcelerometro;

// Protótipo das Tasks
void vTaskAcelerometro(void *parameters);


void setup() {

  while (!Serial)
  {
    ; /* Somente vai em frente quando a serial estiver pronta para funcionar */
  }

  xTaskCreate(vTaskAcelerometro, "Acelerometro", configMINIMAL_STACK_SIZE + 2048, NULL, 1, &xTaskHandleAcelerometro);




}

void loop() {
  // put your main code here, to run repeatedly:
}


void vTaskAcelerometro(void *parameters)
{

while (1)
{
  
}

}