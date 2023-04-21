#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "Adafruit_MPU6050.h"
#include <Servo.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"

// variaveis globais
Adafruit_MPU6050 acelerometro;
sensors_event_t event;
Servo servoMotor;
int posInicialServo = 0;

// Protótipo das tasks
void vTaskSensoresCasa(void *parameters);
void vTaskSensoresEstacao(void *parameters);
void configuraWifi();
void configuraPortaSerial();
void configuraFirebase();
void configuraPinosSensores();

QueueHandle_t xQueueSensoresEstacao;
TaskHandle_t xTaskSensoresCasa, xTaskSensoresEstacao;

// Definição dos pinos
#define SensorUmidade 33
#define SensorChuva 34
#define buzzer 26
#define led 13
#define servo 27

// Firebase e Wifi
#define Wifi_SSID "MALU"
#define Wifi_Senha "37159480"
#define API_Key "AIzaSyAfh8oyA1gxW6alJvT8KfieBXj_8FgsnLI"
#define DataBase_Url "https://projeto-monitoramento-39de2-default-rtdb.firebaseio.com/"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool singupOK = false;

unsigned long sendDataPrevMillis = 0;

void setup()
{

  ConfiguraPortaSerial();
  ConfiguraWifi();
  configuraFirebase();
  configuraPinosSensores();

  xTaskCreate(vTaskSensoresCasa, "SensosresCasa", configMINIMAL_STACK_SIZE + 2048, NULL, 1, &xTaskSensoresCasa);
  xTaskCreate(vTaskSensoresEstacao, "SensosresEstacao", configMINIMAL_STACK_SIZE + 2048, NULL, 1, &xTaskSensoresEstacao);

  // xQueueSensoresEstacao = xQueueCreate(1, sizeof(int));
  // WiFiLocation - obter localização com base na rede
}

void loop()
{

  // if (Firebase.ready() && singupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0))
  // {
  //   sendDataPrevMillis = millis();

  //   if (Firebase.RTDB.setInt(&fbdo, "Projeto/Sensores/Led/", 20));
  // }

  // vTaskDelay(pdMS_TO_TICKS(1000));
}

void vTaskSensoresCasa(void *parameters)
{
  posInicialServo = 0;

  while (1)
  {

    digitalWrite(buzzer, 1);
    digitalWrite(led, 1);

    // MotorServo
    for (int i = 0; i <= 180; i++)
    {

      servoMotor.write(i);

      vTaskDelay(25);
    }
    digitalWrite(buzzer, 0);
    digitalWrite(led, 0);

    vTaskDelay(3000);
  }
}

void vTaskSensoresEstacao(void *parameters)
{
  int sensorValueUmidade = 0;
  float percentual = 0.0;
  int sensorValueChuva = 0;
  float pitch = 0;

  while (1)
  {
    acelerometro.getAccelerometerSensor()->getEvent(&event); // event.acceleration.x
    sensorValueUmidade = analogRead(SensorUmidade);
    sensorValueChuva = analogRead(SensorUmidade);

    if (sensorValueUmidade != 0)
    {
      percentual = (100 - (sensorValueUmidade * 100 / 4095));
    }

    float pitch = atan2(-event.acceleration.x, sqrt(event.acceleration.y * event.acceleration.y + event.acceleration.z * event.acceleration.z)) * 180 / M_PI;

    Serial.print("AC ");
    Serial.println(pitch);
    Serial.print("UM ");
    Serial.println(sensorValueUmidade);
    Serial.print("CH ");
    Serial.println(sensorValueChuva);

    // xQueueSend(xQueueSensoresEstacao, (void *)&ulVar, (TickType_t)10) != pdPASS;

    vTaskDelay(1000);
  }
}

void configuraWifi()
{
  WiFi.begin(Wifi_SSID, Wifi_Senha);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Conectando ao Wifi...");
    delay(1000);
  }
  Serial.print("Wifi conectado: IP  ");
  Serial.println(WiFi.localIP());
}

void configuraPortaSerial()
{
  Serial.begin(9600);

  if (!Serial)
  {
    Serial.println("Porta Seria não conectada.");
  }
}

void configuraFirebase()
{
  config.api_key = API_Key;
  config.database_url = DataBase_Url;

  if (Firebase.signUp(&config, &auth, "", ""))
  {
    singupOK = true;
  }
  else
  {
    Serial.println("Erro ao conectar com o Firebase. ");
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void configuraPinosSensores()
{
  while (!acelerometro.begin())
  {
    Serial.println("Acelerometro não conectado.");
    delay(1000);
  }

  pinMode(servo, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(led, OUTPUT);

  servoMotor.attach(27);
}
