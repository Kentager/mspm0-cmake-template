#ifndef _OLED_APP_H_
#define _OLED_APP_H_

#include <stdint.h>
#include "key.h"

void OLED_AppInit(void);
void OLED_AppHandleKey(key_e key);
void OLED_AppSetAttitude(float pitch, float roll, float yaw);
uint8_t OLED_AppIsAutoRunEnabled(void);

#endif