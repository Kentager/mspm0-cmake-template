#include "channel_grayscale_sensor.h"

irSensorData_t irSensorData;

void irSensor_HwInit(void)
{
    Grayscale_Sensor_Init();
}

void irSensor_DataInit(irSensorData_t *sensor)
{
    if (sensor == 0) {
        return;
    }

    sensor->sensorFlag = 0;
    sensor->diffSpeedMax = CHANNEL_GRAYSCALE_DIFF_SPEED_MAX;
    sensor->diffSpeed = 0.0f;

    for (uint8_t i = 0; i < CHANNEL_GRAYSCALE_SENSOR_CHANNELS; i++) {
        sensor->sensorData[i] = 0.0f;
        sensor->sensorState[i] = 0;
        sensor->sensorDiff[i] = CHANNEL_GRAYSCALE_SENSOR_DIFF[i];
    }
}

void irSensor_Update(irSensorData_t *sensor)
{
    uint16_t sensor_values[CHANNEL_GRAYSCALE_SENSOR_CHANNELS] = {0};
    uint8_t online_count = 0;
    float diff_speed = 0.0f;

    if (sensor == 0) {
        return;
    }

    Grayscale_Sensor_Read_All(sensor_values);

    for (uint8_t i = 0; i < CHANNEL_GRAYSCALE_SENSOR_CHANNELS; i++) {
        sensor->sensorData[i] = (float)sensor_values[i];

        /* 当前灰度驱动返回 0/1；按参考实现约定：0 为黑线，1 为白区 */
        if (sensor_values[i] != 0U) {
            sensor->sensorState[i] = 1U;
            online_count++;
            diff_speed += sensor->sensorDiff[i] * 0.01f;
        } else {
            sensor->sensorState[i] = 0U;
        }
    }

    sensor->sensorFlag = (online_count > 0U) ? 1U : 0U;
    sensor->diffSpeed = diff_speed * sensor->diffSpeedMax;
}

void irSensor_Updata(irSensorData_t *sensor)
{
    irSensor_Update(sensor);
}

float irSensor_GetDiffSpeed(irSensorData_t *sensor)
{
    if (sensor == 0) {
        return 0.0f;
    }

    return sensor->diffSpeed;
}

int irSensor_GetSensorFlag(irSensorData_t *sensor)
{
    if (sensor == 0) {
        return 0;
    }

    return sensor->sensorFlag;
}
