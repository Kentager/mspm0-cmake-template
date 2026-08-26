#ifndef SUPER_SENSOR_H
#define SUPER_SENSOR_H

#include <stdint.h>

#define SUPER_SENSOR_COUNT          8U
#define SUPER_SENSOR_DIFF_SPEED_MAX (0.9f * 1.025f)
// #define SUPER_SENSOR_DIFF_SPEED_MAX (0.8f * 1.025f)
#define SUPER_SENSOR_ACTIVE_LEVEL   1U

/* PIN_0 到 PIN_7 从小车左侧向右侧排列 */
typedef enum {
    SUPER_SENSOR_LEFT_OUTER = 0,
    SUPER_SENSOR_LEFT_3,
    SUPER_SENSOR_LEFT_2,
    SUPER_SENSOR_LEFT_INNER,
    SUPER_SENSOR_RIGHT_INNER,
    SUPER_SENSOR_RIGHT_2,
    SUPER_SENSOR_RIGHT_3,
    SUPER_SENSOR_RIGHT_OUTER,
} Super_Sensor_Channel_t;

typedef struct {
    uint8_t sensorFlag;                    /* 0：未检测到线，1：检测到线 */
    uint8_t sensorState[SUPER_SENSOR_COUNT]; /* 0：白区，1：黑线 */
    float diffSpeedMax;                    /* 最大差速，单位 m/s */
    float diffSpeed;                       /* 当前差速，单位 m/s */
} Super_Sensor_t;

extern Super_Sensor_t superSensor;

void Super_Sensor_Init(Super_Sensor_t *sensor);
void Super_Sensor_Update(Super_Sensor_t *sensor);
float Super_Sensor_GetDiffSpeed(const Super_Sensor_t *sensor);
uint8_t Super_Sensor_GetFlag(const Super_Sensor_t *sensor);

#endif /* SUPER_SENSOR_H */
