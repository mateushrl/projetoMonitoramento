#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "Adafruit_MPU6050.h"
#include <Servo.h>

// variaveis globais
Adafruit_MPU6050 acelerometro;
sensors_event_t event;
Servo servoMotor;
int posInicialServo = 0;

// Protótipo das tasks
void vTaskSensoresCasa(void *parameters);
void vTaskSensoresEstacao(void *parameters);

QueueHandle_t xQueueSensoresEstacao;
TaskHandle_t xTaskSensoresCasa, xTaskSensoresEstacao;

// Definição dos pinos
#define SensorUmidade 34
#define SensorChuva 36
#define buzzer 25

void setup()
{

  while (!Serial.begin(9600))
  {
    Serial.println("Serial não conectado!");
    delay(1000);
  }

  while (!acelerometro.begin())
  {
    Serial.println("Acelerometro não conectado!");
    delay(1000);
  }

  while (!servoMotor.attach(14))
  {
    Serial.println("Servo Motor não conectado!");
    delay(1000);
  }

  pinMode(buzzer, OUTPUT);

  xTaskCreate(vTaskSensoresCasa, "SensosresCasa", configMINIMAL_STACK_SIZE + 2048, NULL, 1, &xTaskSensoresCasa);
  xTaskCreate(vTaskSensoresEstacao, "SensosresEstacao", configMINIMAL_STACK_SIZE + 2048, NULL, 1, &xTaskSensoresEstacao);

  xQueueSensoresEstacao = xQueueCreate(1, sizeof(int));

  // COMUNICACAO COM O BANCO DE DADOS - firebase?
  // WiFiLocation - obter localização com base na rede
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(10));
}

void vTaskSensoresCasa(void *parameters)
{
  digitalWrite(buzzer, HIGH);

  posInicialServo = 0;
  while (1)
  {
    // Sensor Led

    // Buzzer
    digitalWrite(buzzer, HIGH);

    // MotorServo
    for (posInicialServo = 0; posInicialServo <= 180; posInicialServo++)
    {
      Serial.print("Posicao Servo: ");
      Serial.println(posInicialServo);

      servoMotor.write(posInicialServo);

      vTaskDelay(300);
    }
  }

  vTaskDelay(1000);
}

void vTaskSensoresEstacao(void *parameters)
{
  int sensorValueUmidade = 0;
  float percentual = 0.0;
  int sensorValueChuva = 0;

  while (1)
  {
    acelerometro.getAccelerometerSensor()->getEvent(&event); // event.acceleration.x
    sensorValueUmidade = analogRead(SensorUmidade);
    sensorValueChuva = analogRead(SensorUmidade);

    if (sensorValueUmidade != 0)
    {
      percentual = (100 - (sensorValueUmidade * 100 / 4095));
    }

    xQueueSend(xQueueSensoresEstacao, (void *)&ulVar, (TickType_t)10) != pdPASS;

    vTaskDelay(1000);
  }
}