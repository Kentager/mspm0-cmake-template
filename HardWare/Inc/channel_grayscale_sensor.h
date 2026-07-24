#ifndef __CHANNEL_GRAYSCALE_SENSOR_H
#define __CHANNEL_GRAYSCALE_SENSOR_H

#include "grayscale_sensor.h"
#include <stdint.h>

#define CHANNEL_GRAYSCALE_SENSOR_CHANNELS GRAYSCALE_SENSOR_CHANNELS
#define CHANNEL_GRAYSCALE_DIFF_SPEED_MAX 0.1f

/* 差分权重，单位为百分比 */
static const float
    CHANNEL_GRAYSCALE_SENSOR_DIFF[CHANNEL_GRAYSCALE_SENSOR_CHANNELS] = {
        75.0f, 50.0f, 25.0f, 5.0f, -5.0f, -25.0f, -50.0f, -75.0f};

typedef struct {
  uint8_t sensorFlag; // 循迹标志位（0：不在线上 1：在线上）
  float sensorData[CHANNEL_GRAYSCALE_SENSOR_CHANNELS]; // 传感器数据
  uint8_t sensorState[CHANNEL_GRAYSCALE_SENSOR_CHANNELS]; // 传感器状态（0：白区
                                                          // 1：黑区）
  float diffSpeedMax; // 输出最大差速值 (单位m/s)
  float diffSpeed;    // 输出差速值 (单位m/s)
  float sensorDiff[CHANNEL_GRAYSCALE_SENSOR_CHANNELS]; // 差分值(单位%)
} irSensorData_t;

extern irSensorData_t irSensorData;

void irSensor_HwInit(void);
void irSensor_DataInit(irSensorData_t *sensor);
void irSensor_Update(irSensorData_t *sensor);
void irSensor_Updata(irSensorData_t *sensor);
float irSensor_GetDiffSpeed(irSensorData_t *sensor);
int irSensor_GetSensorFlag(irSensorData_t *sensor);

#endif // __CHANNEL_GRAYSCALE_SENSOR_H
