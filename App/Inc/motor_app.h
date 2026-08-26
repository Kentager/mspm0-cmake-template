#ifndef MOTOR_APP_H
#define MOTOR_APP_H

#include <stdint.h>
#include <stdbool.h>


typedef enum {
    SPEED_MODE = 0,
    ANGLE_MODE,
    SENSOR_MODE,
}Motor_Mode_e;
/*--------------------初始化--------------------*/

/**
 * @brief 初始化电机 App（内部调用 Motor_Init + Encoder_Init + Speed_PID_Init）
 */
void Motor_App_Init(void);

/**
 * @brief 周期性更新（10ms 调用）
 *
 * 内部执行：
 *   1. 编码器更新（速度 m/s + 里程累加）
 *   2. PID 计算
 *   3. 驱动电机
 *   4. 限位距离检测（到达后自动停车）
 */
void Motor_App_Update(void);

void Motor_App_YawUpdate(float yaw);

/*--------------------速度控制--------------------*/

/**
 * @brief 设置目标速度（连续行驶，不限距离）
 * @param left_m_s   左电机目标速度 m/s（正=前进，负=后退）
 * @param right_m_s  右电机目标速度 m/s
 */
void Motor_App_SetSpeed(float left_m_s, float right_m_s);

/*--------------------角度控制--------------------*/

/**
 * @brief 设置目标角度（连续行驶，不限距离）
 * @param yaw  左电机目标角度（-179 ~ +180, 逆时针为正）
 */
void Motor_App_SetTargetYaw(float yaw);

/*--------------------距离控制--------------------*/

/**
 * @brief 按指定速度行驶指定距离（到达后自动停车）
 * @param left_m     左电机行驶距离 米（正=前进，负=后退）
 * @param right_m    右电机行驶距离 米
 * @param speed_m_s  行驶速度 m/s（正值）
 */
void Motor_App_Drive(float left_m, float right_m, float speed_m_s);

/**
 * @brief 是否到达目标距离（仅限 Motor_App_Drive 模式）
 */
bool Motor_App_IsReached(void);

/*--------------------读取--------------------*/

/**
 * @brief 获取当前速度 m/s
 */
float Motor_App_GetVelocityLeft(void);
float Motor_App_GetVelocityRight(void);

/**
 * @brief 获取累计里程 m
 */
float Motor_App_GetDistanceLeft(void);
float Motor_App_GetDistanceRight(void);

/**
 * @brief 重置里程计
 */
void Motor_App_ResetDistance(void);

/**
 * @brief 急停：立即刹车，清除目标和 PID 状态
 */
void Motor_App_Brake(void);


void Motor_App_SetMode(Motor_Mode_e mode);
#endif /* MOTOR_APP_H */
