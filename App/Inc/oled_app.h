#ifndef _OLED_APP_H_
#define _OLED_APP_H_

#include <stdint.h>
#include "key.h"
#include "FreeRTOS.h"
#include "queue.h"

#define JOB_QUEUE_LEN  8U

typedef enum {
  Job_0 = 0,
  Job_1,
  Job_2,
  Job_3,
} Job_e;

extern QueueHandle_t xJobQueue;

void OLED_AppInit(void);
void OLED_AppHandleKey(key_e key);
void OLED_AppSetAttitude(float pitch, float roll, float yaw);
void OLED_AppSetSensorValues(uint16_t* sensor_values);
void OLED_AppRefresh(void);
uint8_t OLED_AppIsAutoRunEnabled(void);
#endif