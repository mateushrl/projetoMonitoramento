#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "Adafruit_MPU6050.h"

Adafruit_MPU6050 acelerometro;
sensors_event_t event;
TaskHandle_t xTaskHandleAcelerometro, xTaskUmidade, xTaskChuva, xTaskSensoresCasa;

// Protótipo das Tasks
void vTaskAcelerometro(void *parameters);
void vTaskUmidade(void *parameters);
void vTaskChuva(void *parameters);
void vTaskSensoresCasa(void *parameters);



void setup()
{

  while (!Serial)
  {
  }

  acelerometro.begin();

  xTaskCreate(vTaskAcelerometro, "Acelerometro", configMINIMAL_STACK_SIZE + 2048, NULL, 1, &xTaskHandleAcelerometro);
  xTaskCreate(vTaskUmidade, "Umidade", configMINIMAL_STACK_SIZE + 2048, NULL, 1, &xTaskUmidade);
  xTaskCreate(vTaskChuva, "Chuva", configMINIMAL_STACK_SIZE + 2048, NULL, 1, &xTaskChuva);
  xTaskCreate(vTaskSensoresCasa, "SensosresCasa", configMINIMAL_STACK_SIZE + 2048, NULL, 1, &xTaskSensoresCasa);


  //COMUNICACAO COM O BANCO DE DADOS - firebase?
  //WiFiLocation - obter localização com base na rede


}

void loop() {}


void vTaskSensoresCasa(void *parameters)
{
  //Acionado por meio de uma fila

  //Sensor Led
  //Buzzer
  //MotorServo

}



void vTaskUmidade(void *parameters)
{
}

void vTaskChuva(void *parameters)
{
}


void vTaskAcelerometro(void *parameters)
{

  while (1)
  {
    acelerometro.getAccelerometerSensor()->getEvent(&event);

    Serial.print("[");
    Serial.print(millis());
    Serial.print("] X: ");
    Serial.print(event.acceleration.x);

    Serial.print(", Y: ");
    Serial.print(event.acceleration.y);
    Serial.print(", Z: ");
    Serial.print(event.acceleration.z);
    Serial.println(" m/s^2");
  }
}