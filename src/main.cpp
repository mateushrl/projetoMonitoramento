#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "Adafruit_MPU6050.h"
#include <Servo.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <taskSensoresCasa.h>

Adafruit_MPU6050 acelerometro; // acelerometro
sensors_event_t event;         // evento acelerometro
Servo servoMotor;              // motor servo

QueueHandle_t xQueueFirebase, xQueueControlaSensoresCasa;                                                // filas
TaskHandle_t xTaskHandleSensoresCasa, xTaskHandleSensoresEstacao, xTaskHandleFirebase, xTaskHandleClima; // handles das tasks

unsigned long epochTime = 0;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool singupOK = false;

bool statusSesoresCasa = false;

// protótipo das funcoes
void vTaskSensoresCasa(void *parameters);
void vTaskSensoresEstacao(void *parameters);
void vTaskFirebase(void *parameters);
void vTaskClima(void *parameters);
void configuraWifi();
void configuraPortaSerial();
void configuraFirebase();
void configuraPinosSensores();
void getLatLong();

//  definição dos pinos
#define SensorUmidade 33
#define buzzer 26
#define led 13
#define servo 27



// API Clima
float temperatura = 0.0;
float umidade = 0.0;
String clima = "";
String descricaoClima = "";
WiFiClient wifiClient;
const char *apiKey = "85af602f320d79e3c7e1bd814798105d"; // Chave api clima
const int cityId = 3470127;                              // codigo para cidade de BH
String lat = "-19.92328334446637";
String lng = "-43.995484040856454";

float x_default = 1.0;
float z_default = 1.0;
float y_default = 1.0;
typedef struct
{
  float sensorUmidade;
  float inclinacao_x;
  float inclinacao_y;
  float inclinacao_z;
  String situacao;
} DadosSensoresEstacao;

void setup()
{
  configuraPortaSerial();
  configuraWifi();
  configuraFirebase();
  configuraPinosSensores();
  getLatLong();

  xQueueFirebase = xQueueCreate(3, sizeof(DadosSensoresEstacao));
  xQueueControlaSensoresCasa = xQueueCreate(1, sizeof(bool));

  xTaskCreate(vTaskClima, "Clima", configMINIMAL_STACK_SIZE + 4098, NULL, 1, &xTaskHandleClima);
  xTaskCreate(vTaskSensoresCasa, "SensosresCasa", configMINIMAL_STACK_SIZE + 4098, NULL, 1, &xTaskHandleSensoresCasa);
  xTaskCreate(vTaskFirebase, "Firebase", configMINIMAL_STACK_SIZE + 8192, NULL, 1, &xTaskHandleFirebase);
  xTaskCreate(vTaskSensoresEstacao, "SensosresEstacao", configMINIMAL_STACK_SIZE + 8192, NULL, 1, &xTaskHandleSensoresEstacao);
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(1000));
}

void vTaskSensoresCasa(void *parameters)
{
  bool alertaAtivado = false;
  while (1)
  {
    if (xQueueReceive(xQueueControlaSensoresCasa, &alertaAtivado, portMAX_DELAY) == pdPASS)
    {
      if (alertaAtivado == true)
      {
        digitalWrite(buzzer, 1);
        digitalWrite(led, 1);

        for (int i = 0; i <= 180; i++)
        {
          servoMotor.write(i);
          vTaskDelay(50);
        }
      }
      else
      {
        digitalWrite(buzzer, 0);
        digitalWrite(led, 0);

        for (int i = 180; i <= 0; i--)
        {
          servoMotor.write(i);
          vTaskDelay(50);
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void vTaskFirebase(void *parameters)
{
  DadosSensoresEstacao receivedData;
  bool alertaAtivado = false;
  while (1)
  {
    if (xQueueReceive(xQueueFirebase, &receivedData, portMAX_DELAY) == pdPASS)
    {
      if (Firebase.ready() && singupOK)
      {
        Firebase.RTDB.setFloat(&fbdo, "SensoresRealTime/UmidadeSolo/", receivedData.sensorUmidade);
        Firebase.RTDB.setFloat(&fbdo, "SensoresRealTime/Clima/Temperatura", temperatura);
        Firebase.RTDB.setString(&fbdo, "SensoresRealTime/Clima/Clima", clima);
        Firebase.RTDB.setString(&fbdo, "SensoresRealTime/Clima/Descricao", descricaoClima);
        Firebase.RTDB.setString(&fbdo, "SensoresRealTime/situacao", receivedData.situacao);
      }
    }
    if (Firebase.RTDB.getBool(&fbdo, "SensoresRealTime/AtivaAlerta"))
    {
      if (fbdo.boolData() == true)
        alertaAtivado = true;
      else
        alertaAtivado = false;
      xQueueSend(xQueueControlaSensoresCasa, &alertaAtivado, portMAX_DELAY);
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void vTaskSensoresEstacao(void *parameters)
{
  float valorUmidade = 0;
  float inclinacao_x = 0.0;
  float inclinacao_y = 0.0;
  float inclinacao_z = 0.0;
  DadosSensoresEstacao dadosEstacao;

  while (1)
  {
    for (int i = 1; i <= 5; i++)
    {
      valorUmidade += analogRead(SensorUmidade);
      acelerometro.getAccelerometerSensor()->getEvent(&event);
      inclinacao_x = atan(event.acceleration.x / sqrt(pow(event.acceleration.y, 2) + pow(event.acceleration.z, 2))) * (180 / PI);
      inclinacao_y = atan(event.acceleration.y / sqrt(pow(event.acceleration.x, 2) + pow(event.acceleration.z, 2))) * (180 / PI);
      inclinacao_z = atan(event.acceleration.z / sqrt(pow(event.acceleration.y, 2) + pow(event.acceleration.x, 2))) * (180 / PI);
    }
    inclinacao_x /= 5;
    inclinacao_y /= 5;
    inclinacao_z /= 5;
    valorUmidade /= 5;

    if (valorUmidade != 0)
      valorUmidade = (100 - (valorUmidade * 100 / 4098));

    dadosEstacao = {valorUmidade, inclinacao_x, inclinacao_y, inclinacao_z};

    if (inclinacao_x != x_default &&
        inclinacao_y != y_default &&
        inclinacao_z != z_default &&
        valorUmidade > 20 && clima == "Rain")
    {
      bool ativaAlerta = true;
      dadosEstacao.situacao = "Urgente";
      xQueueSend(xQueueControlaSensoresCasa, &ativaAlerta, portMAX_DELAY);
      xQueueSend(xQueueFirebase, &dadosEstacao, portMAX_DELAY);
      vTaskSuspend(xTaskHandleClima);
      vTaskSuspend(xTaskHandleSensoresEstacao);
    }
    else if (dadosEstacao.sensorUmidade > 10 && clima == "Rain")
      dadosEstacao.situacao = "Atenção";
    else
      dadosEstacao.situacao = "Atenção";

    inclinacao_x = 0.0;
    inclinacao_y = 0.0;
    inclinacao_z = 0.0;
    valorUmidade = 0.0;

    xQueueSend(xQueueFirebase, &dadosEstacao, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void vTaskClima(void *parameters)
{
  HttpClient http(wifiClient, "api.openweathermap.org", 80);
  String url = "/data/2.5/weather?lat=" + lat + "&lon=" + lng + "&appid=" + apiKey + "&units=metric";

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

      singupOK = true;
    }
    else
    {
      Serial.println("Erro ao fazer requisição HTTP");
    }
    vTaskDelay(pdMS_TO_TICKS(600000));
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

  // ntp.begin(); // Obter a data e hora
  // ntp.forceUpdate();
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
  auth.user.email = EMAIL;
  auth.user.password = SENHA;

  Firebase.begin(&config, &auth);
  // singupOK = true;
  config.token_status_callback = tokenStatusCallback;
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

void getLatLong()
{
  if (Firebase.RTDB.getString(&fbdo, "SensoresRealTime/lat"))
  {
    lat = fbdo.stringData();
  }
  if (Firebase.RTDB.getString(&fbdo, "SensoresRealTime/long"))
  {
    lng = fbdo.stringData();
  }
}
