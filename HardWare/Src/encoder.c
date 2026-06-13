#include "encoder.h"
#include "ti_msp_dl_config.h"

/* 全局变量定义 */
encoder_t encoder_left;
encoder_t encoder_right;

/* 初始化编码器 */
void Encoder_Init(void)
{
    encoder_left.id = ENCODER_LEFT;
    encoder_left.count = 0;
    encoder_left.last_count = 0;
    encoder_left.speed = 0;

    encoder_right.id = ENCODER_RIGHT;
    encoder_right.count = 0;
    encoder_right.last_count = 0;
    encoder_right.speed = 0;

    /* 使能 GPIO 中断（A 相上升沿和下降沿都触发） */
    NVIC_EnableIRQ(ENC_GPIOA_INT_IRQN);  /* 左电机编码器 A2 */
    NVIC_EnableIRQ(ENC_GPIOB_INT_IRQN);  /* 右电机编码器 B6 */
}

/* 更新编码器计数（在中断中调用） */
void Encoder_Update(encoder_t *enc, uint8_t a_state, uint8_t b_state)
{
    /* 正交解码：根据 A 相和 B 相状态判断方向 */
    if (a_state == b_state) {
        enc->count++;   /* A == B 时正转 */
    } else {
        enc->count--;   /* A != B 时反转 */
    }
}

/* 获取当前累计脉冲数 */
int32_t Encoder_GetCount(encoder_t *enc)
{
    return enc->count;
}

/* 获取当前速度（脉冲数/采样周期） */
int16_t Encoder_GetSpeed(encoder_t *enc)
{
    int32_t current = enc->count;
    enc->speed = (int16_t)(current - enc->last_count);
    enc->last_count = current;
    return enc->speed;
}

/* 重置编码器计数 */
void Encoder_Reset(encoder_t *enc)
{
    enc->count = 0;
    enc->last_count = 0;
    enc->speed = 0;
}

/* 获取行驶距离（毫米） */
float Encoder_GetDistance_mm(encoder_t *enc)
{
    /* 距离 = 脉冲数 / (每转脉冲数 × 减速比) × 轮子周长 */
    float rotations = (float)enc->count / (ENCODER_PPR * GEAR_RATIO);
    return rotations * WHEEL_CIRCUMFERENCE_MM;
}
