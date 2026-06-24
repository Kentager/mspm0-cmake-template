#ifndef SPEED_PID_H
#define SPEED_PID_H

#include <stdint.h>

/*--------------------类型定义--------------------*/

typedef struct {
    /* PID 参数 */
    float kp;          /* 比例系数 */
    float ki;          /* 积分系数 */
    float kd;          /* 微分系数 */

    /* 内部状态 */
    float integral;    /* 累计误差 */
    float prev_error;  /* 上次误差 */

    /* 输出限幅（绝对值） */
    float output_limit;

    /* 积分限幅（防止积分饱和，绝对值） */
    float integral_limit;

    /* 输出 */
    float output;
} Speed_PID_t;

/*--------------------全局变量--------------------*/

extern Speed_PID_t pid_left;
extern Speed_PID_t pid_right;

/*--------------------函数声明--------------------*/

/**
 * @brief 初始化速度 PID 控制器
 */
void Speed_PID_Init(void);

/**
 * @brief PID 计算
 * @param pid        PID 控制器指针
 * @param target     目标速度（脉冲数/采样周期，float 保留小数精度）
 * @param actual     实际速度（脉冲数/采样周期，float 保留小数精度）
 * @return           PID 输出（-output_limit ~ +output_limit，用作 PWM 占空比）
 */
float Speed_PID_Compute(Speed_PID_t *pid, float target, float actual);

/**
 * @brief 重置 PID 状态（积分、微分归零）
 */
void Speed_PID_Reset(Speed_PID_t *pid);

#endif /* SPEED_PID_H */
