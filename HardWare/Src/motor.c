/*--------------------头文件--------------------*/

#include "motor.h"

/*--------------------变量定义--------------------*/

motor_t motor_left;
motor_t motor_right;

/*--------------------内部函数--------------------*/

/**
 * @brief 设置电机方向引脚
 * @param motor  电机指针
 * @param dir    目标方向
 *
 * TB6612 真值表:
 *   IN1=H, IN2=L -> 正转
 *   IN1=L, IN2=H -> 反转
 *   IN1=L, IN2=L -> 制动
 */
static void Motor_SetDir(motor_t *motor, Motor_Dir_e dir)
{
    if (motor == &motor_left) {
        switch (dir) {
            case MOTOR_DIR_FORWARD:
                DL_GPIO_setPins(MOTOR_L_IN1_PORT, MOTOR_L_IN1_PIN);
                DL_GPIO_clearPins(MOTOR_L_IN2_PORT, MOTOR_L_IN2_PIN);
                break;
            case MOTOR_DIR_BACKWARD:
                DL_GPIO_clearPins(MOTOR_L_IN1_PORT, MOTOR_L_IN1_PIN);
                DL_GPIO_setPins(MOTOR_L_IN2_PORT, MOTOR_L_IN2_PIN);
                break;
            case MOTOR_DIR_STOP:
            default:
                DL_GPIO_clearPins(MOTOR_L_IN1_PORT, MOTOR_L_IN1_PIN);
                DL_GPIO_clearPins(MOTOR_L_IN2_PORT, MOTOR_L_IN2_PIN);
                break;
        }
    } else {
        switch (dir) {
            case MOTOR_DIR_FORWARD:
                DL_GPIO_setPins(MOTOR_R_IN1_PORT, MOTOR_R_IN1_PIN);
                DL_GPIO_clearPins(MOTOR_R_IN2_PORT, MOTOR_R_IN2_PIN);
                break;
            case MOTOR_DIR_BACKWARD:
                DL_GPIO_clearPins(MOTOR_R_IN1_PORT, MOTOR_R_IN1_PIN);
                DL_GPIO_setPins(MOTOR_R_IN2_PORT, MOTOR_R_IN2_PIN);
                break;
            case MOTOR_DIR_STOP:
            default:
                DL_GPIO_clearPins(MOTOR_R_IN1_PORT, MOTOR_R_IN1_PIN);
                DL_GPIO_clearPins(MOTOR_R_IN2_PORT, MOTOR_R_IN2_PIN);
                break;
        }
    }
    motor->dir = dir;
}

/*--------------------函数定义--------------------*/

/**
 * @brief 电机初始化，启动 PWM 定时器
 */
void Motor_Init(void) {
    motor_left.id = MOTOR_LEFT;
    motor_left.dir  = MOTOR_DIR_STOP;
    motor_left.speed = 0;
    motor_right.id = MOTOR_RIGHT;
    motor_right.dir  = MOTOR_DIR_STOP;
    motor_right.speed = 0;

    /* 方向引脚已在 SYSCFG_DL_GPIO_init() 中配置好，这里确保初始为制动 */
    Motor_SetDir(&motor_left, MOTOR_DIR_STOP);
    Motor_SetDir(&motor_right, MOTOR_DIR_STOP);

    /* 启动定时器，PWM 开始输出（此时占空比为 0，电机不转） */
    DL_TimerA_startCounter(PWM_0_INST);
}

/**
 * @brief 设置电机速度和方向
 * @param motor  电机指针 (motor_left / motor_right)
 * @param speed  -MOTOR_PWM_PERIOD ~ +MOTOR_PWM_PERIOD
 *                正值正转，负值反转，0 停止
 */
void Motor_SetSpeed(motor_t *motor, int16_t speed)
{
    /* 限幅 */
    if (speed > (int16_t)MOTOR_PWM_PERIOD)
        speed = (int16_t)MOTOR_PWM_PERIOD;
    if (speed < -(int16_t)MOTOR_PWM_PERIOD)
        speed = -(int16_t)MOTOR_PWM_PERIOD;

    /* 获取通道索引 */
    uint32_t cc_idx = (motor == &motor_left) ? MOTOR_L_PWM_IDX : MOTOR_R_PWM_IDX;

    /* PWM 硬件极性反转：compare = (PERIOD - speed) 才能得到正比占空比 */
    uint32_t compare = (uint32_t)(MOTOR_PWM_PERIOD - (speed > 0 ? speed : -speed));

    if (speed > 0) {
        Motor_SetDir(motor, MOTOR_DIR_FORWARD);
        DL_TimerA_setCaptureCompareValue(PWM_0_INST, compare, cc_idx);
    } else if (speed < 0) {
        Motor_SetDir(motor, MOTOR_DIR_BACKWARD);
        DL_TimerA_setCaptureCompareValue(PWM_0_INST, compare, cc_idx);
    } else {
        /* speed == 0：占空比归零，但不改方向，让 Update 继续跟踪目标 */
        DL_TimerA_setCaptureCompareValue(PWM_0_INST, (uint32_t)MOTOR_PWM_PERIOD, cc_idx);
        Motor_SetDir(motor, MOTOR_DIR_STOP);
    }

    motor->speed = speed;
}

/**
 * @brief 设置目标速度（不立即生效，由 Update 斜坡跟踪）
 * @param motor   电机指针
 * @param target  目标速度 -MOTOR_PWM_PERIOD ~ +MOTOR_PWM_PERIOD
 */
void Motor_SetTarget(motor_t *motor, int16_t target)
{
    // if (target > (int16_t)MOTOR_PWM_PERIOD)
    //     target = (int16_t)MOTOR_PWM_PERIOD;
    // if (target < -(int16_t)MOTOR_PWM_PERIOD)
    //     target = -(int16_t)MOTOR_PWM_PERIOD;

    motor->target_speed = target;
}

/**
 * @brief 斜坡更新，周期调用使实际速度平滑跟踪目标
 *
 * 在 RTOS 任务中以固定周期调用，例如 10ms：
 *   Motor_SetTarget(&motor_left, 2000);  // 随时可以改目标
 *   ...
 *   Motor_Update();  // 每 10ms 调一次
 */
void Motor_Update(void)
{
    motor_t *motors[2] = { &motor_left, &motor_right };

    for (int i = 0; i < 2; i++) {
        motor_t *m = motors[i];

        // /* 斜坡跟踪：逐步调整 speed 到 target_speed */
        // int16_t diff = m->target_speed - m->speed;
        // if (diff > 0) {
        //     m->speed += (diff > 100) ? 100 : diff;  /* 每次最多增加 10 */
        // } else if (diff < 0) {
        //     m->speed -= (diff < -100) ? 100 : -diff; /* 每次最多减少 10 */
        // }
        m->speed = m->target_speed;

        /* 应用到硬件 */
        Motor_SetSpeed(m, m->speed);
    }
}

/**
 * @brief 电机制动（同时清零目标）
 */
void Motor_Brake(motor_t *motor)
{
    uint32_t cc_idx = (motor == &motor_left) ? MOTOR_L_PWM_IDX : MOTOR_R_PWM_IDX;

    /* 占空比归零 */
    DL_TimerA_setCaptureCompareValue(PWM_0_INST, 0, cc_idx);

    /* 方向引脚拉低 */
    Motor_SetDir(motor, MOTOR_DIR_STOP);

    motor->speed = 0;
    motor->target_speed = 0;
}
