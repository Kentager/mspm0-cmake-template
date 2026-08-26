#include "super_sensor.h"
#include "ti_msp_dl_config.h"

/* PIN_0 到 PIN_7 从左向右；正值使小车向左修正，负值向右修正 */
static const float SUPER_SENSOR_WEIGHTS[SUPER_SENSOR_COUNT] = {
    0.48f,
    0.4f,
    0.2f,
    0.1f,
    -0.1f,
    -0.2f,
    -0.4f,
    -0.48f,
};

Super_Sensor_t superSensor;

static uint8_t Super_Sensor_ReadChannel(uint8_t channel)
{
    uint8_t level;

    switch (channel) {
        case 0U:
            level = (DL_GPIO_readPins(GrayS_PIN_0_PORT, GrayS_PIN_0_PIN) != 0U) ? 1U : 0U;
            break;
        case 1U:
            level = (DL_GPIO_readPins(GrayS_PIN_1_PORT, GrayS_PIN_1_PIN) != 0U) ? 1U : 0U;
            break;
        case 2U:
            level = (DL_GPIO_readPins(GrayS_PIN_2_PORT, GrayS_PIN_2_PIN) != 0U) ? 1U : 0U;
            break;
        case 3U:
            level = (DL_GPIO_readPins(GrayS_PIN_3_PORT, GrayS_PIN_3_PIN) != 0U) ? 1U : 0U;
            break;
        case 4U:
            level = (DL_GPIO_readPins(GrayS_PIN_4_PORT, GrayS_PIN_4_PIN) != 0U) ? 1U : 0U;
            break;
        case 5U:
            level = (DL_GPIO_readPins(GrayS_PIN_5_PORT, GrayS_PIN_5_PIN) != 0U) ? 1U : 0U;
            break;
        case 6U:
            level = (DL_GPIO_readPins(GrayS_PIN_6_PORT, GrayS_PIN_6_PIN) != 0U) ? 1U : 0U;
            break;
        case 7U:
            level = (DL_GPIO_readPins(GrayS_PIN_7_PORT, GrayS_PIN_7_PIN) != 0U) ? 1U : 0U;
            break;
        default:
            return 0U;
    }

    return (level == SUPER_SENSOR_ACTIVE_LEVEL) ? 1U : 0U;
}

void Super_Sensor_Init(Super_Sensor_t *sensor)
{
    uint8_t i;

    if (sensor == 0) {
        return;
    }

    sensor->sensorFlag = 0U;
    sensor->diffSpeedMax = SUPER_SENSOR_DIFF_SPEED_MAX;
    sensor->diffSpeed = 0.0f;

    for (i = 0U; i < SUPER_SENSOR_COUNT; i++) {
        sensor->sensorState[i] = 0U;
    }
}

void Super_Sensor_Update(Super_Sensor_t *sensor)
{
    float weighted_sum = 0.0f;
    uint8_t online_count = 0U;
    uint8_t i;

    if (sensor == 0) {
        return;
    }

    for (i = 0U; i < SUPER_SENSOR_COUNT; i++) {
        sensor->sensorState[i] = Super_Sensor_ReadChannel(i);
        if (sensor->sensorState[i] != 0U) {
            weighted_sum += SUPER_SENSOR_WEIGHTS[i];
            online_count++;
        }
    }

    sensor->sensorFlag = (online_count > 0U) ? 1U : 0U;
    if (online_count == 0U) {
        sensor->diffSpeed = 0.0f;
    } else {
        sensor->diffSpeed =
            (weighted_sum / (float)online_count) * sensor->diffSpeedMax;
    }
}

float Super_Sensor_GetDiffSpeed(const Super_Sensor_t *sensor)
{
    if (sensor == 0) {
        return 0.0f;
    }

    return sensor->diffSpeed;
}

uint8_t Super_Sensor_GetFlag(const Super_Sensor_t *sensor)
{
    if (sensor == 0) {
        return 0U;
    }

    return sensor->sensorFlag;
}
