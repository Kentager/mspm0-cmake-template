/*--------------------头文件--------------------*/

#include "speed_pid.h"
#include "motor.h"

/*--------------------全局变量--------------------*/

Speed_PID_t pid_left;
Speed_PID_t pid_right;

/*--------------------函数定义--------------------*/

/**
 * @brief 初始化速度 PID 控制器
 *
 * 默认参数需根据实际电机/编码器调试：
 *   Kp - 响应速度，从小往大调
 *   Ki - 消除稳态误差，注意积分饱和
 *   Kd - 抑制超调/振荡
 */
void Speed_PID_Init(void)
{
    pid_left.kp            = 2.0f;
    pid_left.ki            = 0.5f;
    pid_left.kd            = 0.0f;
    pid_left.integral      = 0.0f;
    pid_left.prev_error    = 0.0f;
    pid_left.output_limit  = MOTOR_PWM_PERIOD;
    pid_left.integral_limit = MOTOR_PWM_PERIOD * 0.8f;
    pid_left.output        = 0;

    pid_right.kp            = 2.0f;
    pid_right.ki            = 0.5f;
    pid_right.kd            = 0.0f;
    pid_right.integral      = 0.0f;
    pid_right.prev_error    = 0.0f;
    pid_right.output_limit  = MOTOR_PWM_PERIOD;
    pid_right.integral_limit = MOTOR_PWM_PERIOD * 0.8f;
    pid_right.output        = 0;
}

/**
 * @brief PID 计算（位置式 PID）
 * @param pid        PID 控制器指针
 * @param target     目标速度（脉冲数/采样周期）
 * @param actual     实际速度（脉冲数/采样周期）
 * @return           PID 输出
 */
float Speed_PID_Compute(Speed_PID_t *pid, float target, float actual)
{
    float error = target - actual;

    /* 积分累加 */
    pid->integral += error;

    /* 积分限幅（Anti-windup） */
    if (pid->integral > pid->integral_limit)
        pid->integral = pid->integral_limit;
    if (pid->integral < -pid->integral_limit)
        pid->integral = -pid->integral_limit;

    /* 微分 */
    float derivative = error - pid->prev_error;

    /* PID 输出 */
    float out = pid->kp * error + pid->ki * pid->integral + pid->kd * derivative;

    /* 输出限幅 */
    if (out > pid->output_limit)
        out = pid->output_limit;
    if (out < -pid->output_limit)
        out = -pid->output_limit;

    pid->prev_error = error;
    pid->output = out;

    return pid->output;
}

/**
 * @brief 重置 PID 状态
 */
void Speed_PID_Reset(Speed_PID_t *pid)
{
    pid->integral   = 0.0f;
    pid->prev_error = 0.0f;
    pid->output     = 0;
}
