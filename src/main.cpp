#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "Adafruit_MPU6050.h"
#include <Servo.h>

Adafruit_MPU6050 acelerometro;
sensors_event_t event;
TaskHandle_t xTaskHandleAcelerometro, xTaskUmidade, xTaskChuva, xTaskSensoresCasa;
Servo servoMotor;
int posInicialServo = 0;


// Protótipo das Tasks
void vTaskAcelerometro(void *parameters);
void vTaskUmidade(void *parameters);
void vTaskChuva(void *parameters);
void vTaskSensoresCasa(void *parameters);

// Definição dos Pinos
#define SensorUmidade 34
#define SensorChuva 36
#define buzzer 25

void setup()
{
  Serial.begin(9600);

  // if (!Serial)
  // {
  //   Serial.println("Serial não conectado!");
  //   delay(1000);
  // }

  while (!acelerometro.begin())
  {
    Serial.println("MPU6050 não conectado!");
    delay(1000);
  }

  servoMotor.attach(14);

  pinMode(buzzer, OUTPUT);

  xTaskCreate(vTaskAcelerometro, "Acelerometro", configMINIMAL_STACK_SIZE + 2048, NULL, 1, &xTaskHandleAcelerometro);
  xTaskCreate(vTaskUmidade, "Umidade", configMINIMAL_STACK_SIZE + 2048, NULL, 1, &xTaskUmidade);
  xTaskCreate(vTaskChuva, "Chuva", configMINIMAL_STACK_SIZE + 2048, NULL, 1, &xTaskChuva);
  xTaskCreate(vTaskSensoresCasa, "SensosresCasa", configMINIMAL_STACK_SIZE + 2048, NULL, 1, &xTaskSensoresCasa);

  // COMUNICACAO COM O BANCO DE DADOS - firebase?
  // WiFiLocation - obter localização com base na rede
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(10));
}

void vTaskSensoresCasa(void *parameters)
{

  while (1)
  {
    for (posInicialServo = 0; posInicialServo <= 180; posInicialServo++)
    {
      Serial.print("Posicao Servo: ");
      Serial.println(posInicialServo);

      servoMotor.write(posInicialServo);

      vTaskDelay(300);
    }
    for (posInicialServo = 180; posInicialServo >= 0; posInicialServo--)
    {
      Serial.print("Posicao Servo: ");
      Serial.println(posInicialServo);
      servoMotor.write(posInicialServo);

      vTaskDelay(300);
    }
  }

  // Acionado por meio de uma fila

  // Sensor Led
  // Buzzer
  // MotorServo
}

void vTaskUmidade(void *parameters)
{
  int sensorValueUmidade = 0;
  float percentual = 0.0;
  sensorValueUmidade = analogRead(SensorUmidade);

  while (1)
  {
    sensorValueUmidade = analogRead(SensorUmidade);

    if (sensorValueUmidade != 0)
    {
      percentual = (100 - (sensorValueUmidade * 100 / 4095));
    }

    Serial.print("Sensor Umidade: ");
    Serial.println(sensorValueUmidade);

    vTaskDelay(5000);
  }
}

void vTaskChuva(void *parameters)
{

  int sensorValueChuva = 0;

  while (1)
  {
    sensorValueChuva = analogRead(SensorUmidade);

    Serial.print("Sensor Chuva: ");
    Serial.println(sensorValueChuva);
    digitalWrite(buzzer, HIGH);
    vTaskDelay(5000);
  }
}

void vTaskAcelerometro(void *parameters)
{

  while (1)
  {

    acelerometro.getAccelerometerSensor()->getEvent(&event);

    Serial.print("Acelerometro: ");
    Serial.print("X: ");
    Serial.print(event.acceleration.x);

    Serial.print(", Y: ");
    Serial.print(event.acceleration.y);
    Serial.print(", Z: ");
    Serial.print(event.acceleration.z);
    Serial.println(" m/s^2");

    vTaskDelay(5000);
  }
}
