#include "encoder.h"
#include "ti_msp_dl_config.h"

/*--------------------全局变量定义--------------------*/

encoder_t encoder_left;
encoder_t encoder_right;

/*--------------------函数定义--------------------*/

void Encoder_Init(void)
{
    encoder_left.id         = ENCODER_LEFT;
    encoder_left.count      = 0;
    encoder_left.last_count = 0;
    encoder_left.speed      = 0;
    encoder_left.velocity   = 0.0f;
    encoder_left.distance_m = 0.0f;

    encoder_right.id         = ENCODER_RIGHT;
    encoder_right.count      = 0;
    encoder_right.last_count = 0;
    encoder_right.speed      = 0;
    encoder_right.velocity   = 0.0f;
    encoder_right.distance_m = 0.0f;

    /* 使能 GPIO 中断（A 相上升沿和下降沿都触发） */
    NVIC_EnableIRQ(ENC_GPIOA_INT_IRQN);  /* 左电机编码器 A2 */
    NVIC_EnableIRQ(ENC_GPIOB_INT_IRQN);  /* 右电机编码器 B6 */
}

/* 正交解码（中断中调用） */
void Encoder_Update(encoder_t *enc, uint8_t a_state, uint8_t b_state)
{
    if (a_state == b_state) {
        enc->count++;
    } else {
        enc->count--;
    }
}

int32_t Encoder_GetCount(encoder_t *enc)
{
    return enc->count;
}

/* 获取脉冲速度（需先调用 Encoder_UpdateMetrics） */
int16_t Encoder_GetSpeed(encoder_t *enc)
{
    return enc->speed;
}

/* 获取线速度 m/s（需先调用 Encoder_UpdateMetrics） */
float Encoder_GetVelocity(encoder_t *enc)
{
    return enc->velocity;
}

/* 获取累计里程 米（需先调用 Encoder_UpdateMetrics） */
float Encoder_GetDistance(encoder_t *enc)
{
    return enc->distance_m;
}

void Encoder_Reset(encoder_t *enc)
{
    enc->count      = 0;
    enc->last_count = 0;
    enc->speed      = 0;
    enc->velocity   = 0.0f;
    enc->distance_m = 0.0f;
}

/**
 * @brief 周期性更新：脉冲速度 → m/s，累加里程
 *
 * 速度换算：
 *   speed    = count - last_count                         （脉冲/采样周期）
 *   velocity = speed × DIST_PER_PULSE / (period_ms/1000)  （m/s）
 *
 * 里程换算：
 *   distance += speed × DIST_PER_PULSE                    （米）
 */
void Encoder_UpdateMetrics(encoder_t *enc, uint16_t period_ms)
{
    int32_t current = enc->count;

    /* 脉冲速度 */
    enc->speed = (int16_t)(current - enc->last_count);
    enc->last_count = current;

    /* m/s = 脉冲/周期 × 米/脉冲 / (秒/周期) = 脉冲/周期 × 米/脉冲 × 周期/秒 */
    float period_s = (float)period_ms / 1000.0f;
    enc->velocity  = (float)enc->speed * ENCODER_DIST_PER_PULSE / period_s;

    /* 累加里程 */
    enc->distance_m += (float)enc->speed * ENCODER_DIST_PER_PULSE;
}
