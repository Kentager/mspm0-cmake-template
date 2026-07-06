#ifndef _KEY_H
#define _KEY_H

#include "FreeRTOS.h"
#include "queue.h"

typedef enum {
  KEY_NONE = -1,
  KEY_0 = 0,
  KEY_1 = 1,
  KEY_2 = 2,
  KEY_3 = 3,
}key_e;

#define KEY_QUEUE_LEN  8U

extern key_e key;
extern QueueHandle_t xKeyQueue;

#endif