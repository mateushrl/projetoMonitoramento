#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "Adafruit_MPU6050.h"
#include <Servo.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include <NTPClient.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>

Adafruit_MPU6050 acelerometro; // acelerometro
sensors_event_t event;         // evento acelerometro
Servo servoMotor;              // motor servo

QueueHandle_t xQueueFirebase, xQueueSensoresCasa;                                                        // filas
TaskHandle_t xTaskHandleSensoresCasa, xTaskHandleSensoresEstacao, xTaskHandleFirebase, xTaskHandleClima; // handles das tasks

WiFiUDP udp;                                          // obter data hora
NTPClient ntp(udp, "a.st1.ntp.br", -3 * 3600, 60000); // obter data hora
unsigned long epochTime = 0;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool singupOK = false;

bool ativaSensoresCasa = false;

// protótipo das funcoes
void vTaskSensoresCasa(void *parameters);
void vTaskSensoresEstacao(void *parameters);
void vTaskFirebase(void *parameters);
void vTaskClima(void *parameters);
void configuraWifi();
void configuraPortaSerial();
void configuraFirebase();
void configuraPinosSensores();

// definição dos pinos
#define SensorUmidade 33
#define SensorChuva 34
#define buzzer 26
#define led 13
#define servo 27

// Firebase e Wifi
#define Wifi_SSID "MALU"                                                                // nome wifi

// API Clima
float temperatura = 0.0;
float umidade = 0.0;
String clima = "";
String descricaoClima = "";
WiFiClient wifiClient;
const int cityId = 3470127;                              // codigo para cidade de BH

// struct para salvar e enviar dados da estação para o firebase
typedef struct
{
  float sensorUmidade;
  float sensorChuva;
  float inclinacao_x;
  float inclinacao_y;
  float inclinacao_z;
} DadosSensoresEstacao;

void setup()
{
  configuraPortaSerial();
  configuraWifi();
  configuraFirebase();
  configuraPinosSensores();

  xTaskCreate(vTaskClima, "Clima", configMINIMAL_STACK_SIZE + 4098, NULL, 1, &xTaskHandleClima);
  xTaskCreate(vTaskSensoresCasa, "SensosresCasa", configMINIMAL_STACK_SIZE + 4098, NULL, 1, &xTaskHandleSensoresCasa);
  xTaskCreate(vTaskSensoresEstacao, "SensosresEstacao", configMINIMAL_STACK_SIZE + 8192, NULL, 1, &xTaskHandleSensoresEstacao);
  xTaskCreate(vTaskFirebase, "Firebase", configMINIMAL_STACK_SIZE + 8192, NULL, 1, &xTaskHandleFirebase);

  xQueueFirebase = xQueueCreate(3, sizeof(DadosSensoresEstacao));
}

void loop()
{
  // vTaskDelay(pdMS_TO_TICKS(60000));
}

void vTaskSensoresCasa(void *parameters)
{
  vTaskSuspend(xTaskHandleSensoresCasa);

  while (1)
  {
    // digitalWrite(buzzer, 1);
    digitalWrite(led, 1);

    for (int i = servoMotor.read(); i <= 180; i++)
    {
      servoMotor.write(i);
      vTaskDelay(50);
    }

    vTaskDelay(500);
  }
}

void vTaskFirebase(void *parameters)
{
  DadosSensoresEstacao receivedData;
  while (1)
  {
    if (xQueueReceive(xQueueFirebase, &receivedData, portMAX_DELAY) == pdPASS)
    {
      // epochTime = ntp.getEpochTime();
      if (Firebase.ready() && singupOK)
      {
        Firebase.RTDB.setFloat(&fbdo, "SensoresRealTime/UmidadeSolo/", receivedData.sensorUmidade);
        Firebase.RTDB.setFloat(&fbdo, "SensoresRealTime/Chuva/", receivedData.sensorChuva);
        Firebase.RTDB.setFloat(&fbdo, "SensoresRealTime/Acelerometro/Inclinacao_X", receivedData.inclinacao_x);
        Firebase.RTDB.setFloat(&fbdo, "SensoresRealTime/Acelerometro/Inclinacao_Y", receivedData.inclinacao_y);
        Firebase.RTDB.setFloat(&fbdo, "SensoresRealTime/Acelerometro/Inclinacao_Z", receivedData.inclinacao_z);
        Firebase.RTDB.setFloat(&fbdo, "SensoresRealTime/Clima/Temperatura", temperatura);
        Firebase.RTDB.setString(&fbdo, "SensoresRealTime/Clima/Clima", clima);
        Firebase.RTDB.setFloat(&fbdo, "SensoresRealTime/Clima/UmidadeAr", umidade);
        Firebase.RTDB.setString(&fbdo, "SensoresRealTime/Clima/Descricao", descricaoClima);

        // Historico
        // Firebase.RTDB.setInt(&fbdo, "Historico/Acelerometro/" + std::to_string(epochTime)+"", receivedData.Acelerometro);
        // Firebase.RTDB.setInt(&fbdo, "Historico/Chuva/" + std::to_string(epochTime)+"", receivedData.sensorChuva);
        // Firebase.RTDB.setInt(&fbdo, "Historico/Umidade/" + std::to_string(epochTime)+"", receivedData.sensorUmidade);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void vTaskSensoresEstacao(void *parameters)
{
  float sensorValueChuva = 0;
  float sensorValueUmidade = 0;
  DadosSensoresEstacao sendDadosEstacaoFila;
  float resultante = 0.0;
  float inclinacao_x = 0.0;
  float inclinacao_y = 0.0;
  float inclinacao_z = 0.0;

  while (1)
  {
    sensorValueUmidade = analogRead(SensorUmidade);
    sensorValueChuva = analogRead(SensorChuva);

    if (sensorValueUmidade != 0)
      sensorValueUmidade = (100 - (sensorValueUmidade * 100 / 4095));

    if (sensorValueChuva != 0)
      sensorValueChuva = (100 - (sensorValueChuva * 100 / 4095));

    for (int i = 0; i < 10; i++)
    {
      acelerometro.getAccelerometerSensor()->getEvent(&event);
      resultante += sqrt(pow(event.acceleration.x, 2) + pow(event.acceleration.y, 2) + pow(event.acceleration.z, 2));
      resultante /= 10;
    }

    inclinacao_x = atan(event.acceleration.x / resultante) * (180 / PI);
    inclinacao_y = atan(event.acceleration.y / resultante) * (180 / PI);
    inclinacao_z = atan(event.acceleration.z / resultante) * (180 / PI);

    if (inclinacao_x > 20 && clima == "Rain" && sensorValueUmidade > 20)
    {
      vTaskResume(xTaskHandleSensoresCasa);
      vTaskSuspend(xTaskHandleSensoresEstacao);
    }

    sendDadosEstacaoFila = {sensorValueUmidade, sensorValueChuva, inclinacao_x, inclinacao_y, inclinacao_z};
    xQueueSend(xQueueFirebase, &sendDadosEstacaoFila, portMAX_DELAY);

    vTaskDelay(3000);
  }
}

void vTaskClima(void *parameters)
{
  HttpClient http(wifiClient, "api.openweathermap.org", 80);
  String url = "/data/2.5/weather?id=" + String(cityId) + "&appid=" + apiKey + "&units=metric" + "&lang=pt_br";
  while (1)
  {
    http.beginRequest();
    http.get(url);
    http.endRequest();
    int httpCode = http.responseStatusCode();

    if (httpCode == 200)
    {
      String payload = http.responseBody();
      Serial.println(payload);

      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);

      temperatura = doc["main"]["temp"];
      umidade = doc["main"]["humidity"];
      clima = doc["weather"][0]["main"].as<String>();
      descricaoClima = doc["weather"][0]["description"].as<String>();

      Serial.println();

      Serial.print("Temperatura: ");
      Serial.println(temperatura);

      Serial.print("Umidade: ");
      Serial.println(umidade);

      Serial.print("Clima: ");
      Serial.println(clima);

      Serial.print("Descrição: ");
      Serial.println(descricaoClima);
    }
    else
    {
      Serial.println("Erro ao fazer requisição HTTP");
    }
    vTaskDelay(10000000);
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

  ntp.begin(); // Obter a data e hora
  ntp.forceUpdate();
}

void configuraPortaSerial()
{
  Serial.begin(9600);

  if (!Serial)
  {
    Serial.println("Porta Serial não conectada.");
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
