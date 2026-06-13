#ifndef _MOTOR_H
#define _MOTOR_H

/*--------------------头文件--------------------*/

#include "ti_msp_dl_config.h"

/*--------------------宏定义--------------------*/

#define MOTOR_PWM_PERIOD 3200 /* 64MHz / 3200 = 20kHz */


/* 左电机引脚 */
#define MOTOR_L_PWM_INST PWM_0_INST
#define MOTOR_L_PWM_IDX GPIO_PWM_0_C0_IDX 
#define MOTOR_L_IN1_PORT IN_PORT
#define MOTOR_L_IN1_PIN IN_LEFT_IN1_PIN 
#define MOTOR_L_IN2_PORT IN_PORT
#define MOTOR_L_IN2_PIN IN_LEFT_IN2_PIN 

/* 右电机引脚 */
#define MOTOR_R_PWM_INST PWM_0_INST
#define MOTOR_R_PWM_IDX GPIO_PWM_0_C1_IDX 
#define MOTOR_R_IN1_PORT IN_PORT
#define MOTOR_R_IN1_PIN IN_RIGHT_IN1_PIN 
#define MOTOR_R_IN2_PORT IN_PORT
#define MOTOR_R_IN2_PIN IN_RIGHT_IN2_PIN 

/*--------------------类型定义--------------------*/

typedef enum {
  MOTOR_DIR_FORWARD = 0,
  MOTOR_DIR_BACKWARD = 1,
  MOTOR_DIR_STOP = 2
} Motor_Dir_e;

typedef enum {
  MOTOR_LEFT = 0,
  MOTOR_RIGHT = 1,
  MOTOR_MAX = 2
}Motor_Id_e;

typedef struct {
  Motor_Dir_e dir;
  Motor_Id_e id;
  int16_t speed;        /* 当前实际速度 */
  int16_t target_speed; /* 目标速度 */
} motor_t;

/*--------------------变量声明--------------------*/

extern motor_t motor_left;
extern motor_t motor_right;

/*--------------------函数声明--------------------*/

void Motor_Init(void);
void Motor_SetSpeed(motor_t *motor, int16_t speed);
void Motor_SetTarget(motor_t *motor, int16_t target);
void Motor_Update(void);
void Motor_Brake(motor_t *motor);

#endif /* _MOTOR_H */