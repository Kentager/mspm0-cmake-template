#ifndef MADGWICK_H
#define MADGWICK_H

#include <math.h>
#include <stdint.h>

typedef struct {
    float q0, q1, q2, q3;   // 四元数
    float invSampleFreq;    // 采样周期
    float beta;             // 滤波增益
    uint8_t initialized;    // 初始化标志
} Madgwick_t;

// 初始化滤波器
void Madgwick_Init(Madgwick_t *f, float sampleFreq, float beta);

// 9轴融合（带磁力计）
void Madgwick_Update(Madgwick_t *f, float gx, float gy, float gz, 
                     float ax, float ay, float az, 
                     float mx, float my, float mz);

// 6轴融合（不带磁力计，更省算力）
void Madgwick_Update6Axis(Madgwick_t *f, float gx, float gy, float gz, 
                          float ax, float ay, float az);

// 获取欧拉角（角度制）
void Madgwick_GetEuler(Madgwick_t *f, float *pitch, float *roll, float *yaw);
void Madgwick_GetEulerAero(Madgwick_t *f, float *pitch, float *roll, float *yaw);

// 重置姿态（水平朝北）
void Madgwick_Reset(Madgwick_t *f);

#endif