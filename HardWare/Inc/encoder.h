#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

/* 编码器参数 */
#define ENCODER_PPR  500       /* 编码器线数（每转脉冲数） */
#define GEAR_RATIO   28     /* 减速比 */
#define WHEEL_CIRCUMFERENCE_MM  3.14159f * 65.0f  /* 轮子周长（毫米），根据实际轮子调整 */

/* 编码器 ID */
typedef enum {
    ENCODER_LEFT = 0,
    ENCODER_RIGHT = 1,
    ENCODER_MAX = 2
} Encoder_Id_e;

/* 编码器数据结构 */
typedef struct {
    Encoder_Id_e id;
    volatile int32_t count;      /* 累计脉冲数 */
    int32_t last_count;          /* 上次读取时的脉冲数 */
    int16_t speed;               /* 当前速度（脉冲/采样周期） */
} encoder_t;

/* 全局变量声明 */
extern encoder_t encoder_left;
extern encoder_t encoder_right;

/* 函数声明 */
void Encoder_Init(void);
int32_t Encoder_GetCount(encoder_t *enc);
int16_t Encoder_GetSpeed(encoder_t *enc);
void Encoder_Reset(encoder_t *enc);
float Encoder_GetDistance_mm(encoder_t *enc);

/* 中断处理函数（在 main.c 或 stm32fxxx_it.c 中调用） */
void Encoder_Update(encoder_t *enc, uint8_t a_state, uint8_t b_state);

#endif /* ENCODER_H */
