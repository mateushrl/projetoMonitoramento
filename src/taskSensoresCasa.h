
// #include <Arduino.h>
// #include <Servo.h>

// #define buzzer 26
// #define led 13
// #define servo 27

// Servo servoMotor;
// void controlaAlerta(uint8_t val, int initServo, int finalServo);

// void vTaskSensoresCasa(void *parameters)
// {
//   bool alertaAtivado = false;
//   QueueHandle_t *pxQueue = (QueueHandle_t *)parameters; // Converte o parâmetro para um ponteiro para a fila

//   while (1)
//   {
//     if (xQueueReceive(pxQueue, &alertaAtivado, portMAX_DELAY) == pdPASS) // Usa a fila recebida como parâmetro
//     {
//       controlaAlerta(1, 0, 180);
//       if (alertaAtivado == true)
//         controlaAlerta(1, 0, 180);
//       else
//         controlaAlerta(1, 180, 0);
//     }
//     vTaskDelay(500);
//   }
// }

// void controlaAlerta(uint8_t val, int initServo, int finalServo)
// {
//   digitalWrite(buzzer, val);
//   digitalWrite(led, val);

//   for (int i = initServo; i <= finalServo; i++)
//   {
//     servoMotor.write(i);
//     vTaskDelay(50);
//   }
// }