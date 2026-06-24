/*--------------------头文件--------------------*/

#include "motor_app.h"
#include "motor.h"
#include "encoder.h"
#include "speed_pid.h"

/*--------------------内部常量--------------------*/

/*
 * m/s → 脉冲/采样周期 换算
 *   counts_per_sec   = velocity / DIST_PER_PULSE
 *   counts_per_sample = counts_per_sec × (period_ms / 1000)
 *   合并：velocity / DIST_PER_PULSE × (period_ms / 1000)
 */
#define MS_TO_PULSES_PER_SAMPLE(m_s, period_ms) \
    ((m_s) / ENCODER_DIST_PER_PULSE * ((period_ms) / 1000.0f))

#define APP_SAMPLE_PERIOD_MS  10

/*--------------------内部状态--------------------*/

static float target_left_ms;        /* 左电机目标速度 m/s */
static float target_right_ms;       /* 右电机目标速度 m/s */

static bool  distance_mode;         /* 是否处于限距模式 */
static float dist_target_left;      /* 左电机目标距离 m */
static float dist_target_right;     /* 右电机目标距离 m */
static float dist_start_left;       /* 左电机起始里程 m */
static float dist_start_right;      /* 右电机起始里程 m */

/*--------------------内部函数--------------------*/

/**
 * @brief 行驶到目标距离后自动停车
 */
static void Motor_App_Stop(void)
{
    Motor_SetSpeed(&motor_left,  0);
    Motor_SetSpeed(&motor_right, 0);
    target_left_ms  = 0.0f;
    target_right_ms = 0.0f;
    Speed_PID_Reset(&pid_left);
    Speed_PID_Reset(&pid_right);
}

/*--------------------函数定义--------------------*/

void Motor_App_Init(void)
{
    Motor_Init();
    Encoder_Init();
    Speed_PID_Init();

    target_left_ms    = 0.0f;
    target_right_ms   = 0.0f;
    distance_mode     = false;
    dist_target_left  = 0.0f;
    dist_target_right = 0.0f;
    dist_start_left   = 0.0f;
    dist_start_right  = 0.0f;
}

void Motor_App_Update(void)
{
    /* 1. 编码器更新：脉冲速度 → m/s，累加里程 */
    Encoder_UpdateMetrics(&encoder_left,  APP_SAMPLE_PERIOD_MS);
    Encoder_UpdateMetrics(&encoder_right, APP_SAMPLE_PERIOD_MS);

    /* 实际速度（脉冲/采样周期），用 float 保留精度 */
    float actual_l = (float)Encoder_GetSpeed(&encoder_left);
    float actual_r = (float)Encoder_GetSpeed(&encoder_right);

    /* 2. 限距模式：检查是否到达目标 */
    if (distance_mode) {
        float traveled_l = Encoder_GetDistance(&encoder_left)  - dist_start_left;
        float traveled_r = Encoder_GetDistance(&encoder_right) - dist_start_right;

        bool left_reached  = (dist_target_left  >= 0) ? (traveled_l >= dist_target_left)
                                                       : (traveled_l <= dist_target_left);
        bool right_reached = (dist_target_right >= 0) ? (traveled_r >= dist_target_right)
                                                       : (traveled_r <= dist_target_right);

        if (left_reached && right_reached) {
            Motor_App_Stop();
            distance_mode = false;
            return;
        }

        /* 单侧到达后停止该侧 */
        if (left_reached)  target_left_ms  = 0.0f;
        if (right_reached) target_right_ms = 0.0f;
    }

    /* 3. m/s → 脉冲/采样周期（float 保留精度） */
    float target_l = MS_TO_PULSES_PER_SAMPLE(target_left_ms,  APP_SAMPLE_PERIOD_MS);
    float target_r = MS_TO_PULSES_PER_SAMPLE(target_right_ms, APP_SAMPLE_PERIOD_MS);

    /* 4. PID 计算 */
    float out_l = Speed_PID_Compute(&pid_left,  target_l, actual_l);
    float out_r = Speed_PID_Compute(&pid_right, target_r, actual_r);

    /* 5. 驱动电机（转为 int16 给 PWM） */
    Motor_SetSpeed(&motor_left,  (int16_t)out_l);
    Motor_SetSpeed(&motor_right, (int16_t)out_r);
}

/*--------------------速度控制--------------------*/

void Motor_App_SetSpeed(float left_m_s, float right_m_s)
{
    distance_mode    = false;
    target_left_ms   = left_m_s;
    target_right_ms  = right_m_s;
}

/*--------------------距离控制--------------------*/

void Motor_App_Drive(float left_m, float right_m, float speed_m_s)
{
    /* 记录起始里程 */
    dist_start_left  = Encoder_GetDistance(&encoder_left);
    dist_start_right = Encoder_GetDistance(&encoder_right);

    /* 设置目标距离 */
    dist_target_left  = left_m;
    dist_target_right = right_m;

    /* 设置速度方向与距离方向一致 */
    target_left_ms  = (left_m  >= 0) ? speed_m_s : -speed_m_s;
    target_right_ms = (right_m >= 0) ? speed_m_s : -speed_m_s;

    distance_mode = true;
}

bool Motor_App_IsReached(void)
{
    return !distance_mode;
}

/*--------------------读取--------------------*/

float Motor_App_GetVelocityLeft(void)
{
    return Encoder_GetVelocity(&encoder_left);
}

float Motor_App_GetVelocityRight(void)
{
    return Encoder_GetVelocity(&encoder_right);
}

float Motor_App_GetDistanceLeft(void)
{
    return Encoder_GetDistance(&encoder_left);
}

float Motor_App_GetDistanceRight(void)
{
    return Encoder_GetDistance(&encoder_right);
}

void Motor_App_ResetDistance(void)
{
    Encoder_Reset(&encoder_left);
    Encoder_Reset(&encoder_right);
}

/*--------------------急停--------------------*/

void Motor_App_Brake(void)
{
    Motor_Brake(&motor_left);
    Motor_Brake(&motor_right);
    Speed_PID_Reset(&pid_left);
    Speed_PID_Reset(&pid_right);
    target_left_ms   = 0.0f;
    target_right_ms  = 0.0f;
    distance_mode    = false;
}
