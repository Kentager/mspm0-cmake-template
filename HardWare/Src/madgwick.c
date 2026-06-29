#include "madgwick.h"
#include <math.h>
#include <string.h>

// 快速平方根倒数（避免使用 sqrt，速度更快）
static float invSqrt(float x) {
    volatile float y = x;
    volatile long i = *(long*)&y;
    i = 0x5f3759df - (i >> 1);
    y = *(float*)&i;
    y = y * (1.5f - 0.5f * x * y * y);
    return y;
}

void Madgwick_Init(Madgwick_t *f, float sampleFreq, float beta) {
    f->q0 = 1.0f;
    f->q1 = 0.0f;
    f->q2 = 0.0f;
    f->q3 = 0.0f;
    f->invSampleFreq = 1.0f / sampleFreq;
    f->beta = beta;
    f->initialized = 1;
}

void Madgwick_Reset(Madgwick_t *f) {
    f->q0 = 1.0f;
    f->q1 = 0.0f;
    f->q2 = 0.0f;
    f->q3 = 0.0f;
}

// 9轴融合（完整版）
void Madgwick_Update(Madgwick_t *f, float gx, float gy, float gz, 
                     float ax, float ay, float az, 
                     float mx, float my, float mz) {
    float q0 = f->q0, q1 = f->q1, q2 = f->q2, q3 = f->q3;
    float beta = f->beta;
    float invSampleFreq = f->invSampleFreq;
    float norm;
    
    // 检查输入是否有效（防止除零）
    float accel_mag = ax*ax + ay*ay + az*az;
    if (accel_mag < 0.001f) return;
    
    // 归一化加速度计
    norm = invSqrt(accel_mag);
    ax *= norm; ay *= norm; az *= norm;
    
    // 归一化磁力计
    float mag_mag = mx*mx + my*my + mz*mz;
    if (mag_mag > 0.001f) {
        norm = invSqrt(mag_mag);
        mx *= norm; my *= norm; mz *= norm;
    } else {
        // 磁力计无效，降级为6轴
        Madgwick_Update6Axis(f, gx, gy, gz, ax, ay, az);
        return;
    }
    
    // 计算参考磁场方向
    float q0q0 = q0*q0, q0q1 = q0*q1, q0q2 = q0*q2, q0q3 = q0*q3;
    float q1q1 = q1*q1, q1q2 = q1*q2, q1q3 = q1*q3;
    float q2q2 = q2*q2, q2q3 = q2*q3, q3q3 = q3*q3;
    
    float hx = 2.0f * (mx * (0.5f - q2q2 - q3q3) + my * (q1q2 - q0q3) + mz * (q1q3 + q0q2));
    float hy = 2.0f * (mx * (q1q2 + q0q3) + my * (0.5f - q1q1 - q3q3) + mz * (q2q3 - q0q1));
    float hz = 2.0f * (mx * (q1q3 - q0q2) + my * (q2q3 + q0q1) + mz * (0.5f - q1q1 - q2q2));
    
    float bx = invSqrt(hx*hx + hy*hy);
    if (bx > 0) {
        bx = 1.0f / bx;  // 归一化
    }
    float bz = hz;
    
    // 计算梯度方向
    float vx = 2.0f * (q1q3 - q0q2);
    float vy = 2.0f * (q0q1 + q2q3);
    float vz = q0q0 - q1q1 - q2q2 + q3q3;
    
    float wx = 2.0f * bx * (0.5f - q2q2 - q3q3) + 2.0f * bz * (q1q3 - q0q2);
    float wy = 2.0f * bx * (q1q2 - q0q3) + 2.0f * bz * (q0q1 + q2q3);
    float wz = 2.0f * bx * (q0q2 + q1q3) + 2.0f * bz * (0.5f - q1q1 - q2q2);
    
    // 误差
    float ex = (ay*vz - az*vy) + (my*wz - mz*wy);
    float ey = (az*vx - ax*vz) + (mz*wx - mx*wz);
    float ez = (ax*vy - ay*vx) + (mx*wy - my*wx);
    
    // 四元数导数
    float q0_dot = -0.5f * (q1*gx + q2*gy + q3*gz) - beta * (2.0f * q1*ex + 2.0f * q2*ey + 2.0f * q3*ez);
    float q1_dot =  0.5f * (q0*gx - q3*gy + q2*gz) - beta * (-2.0f * q0*ex - 2.0f * q3*ey + 2.0f * q2*ez);
    float q2_dot =  0.5f * (q3*gx + q0*gy - q1*gz) - beta * (2.0f * q3*ex - 2.0f * q0*ey - 2.0f * q1*ez);
    float q3_dot = -0.5f * (q2*gx - q1*gy - q0*gz) - beta * (-2.0f * q2*ex + 2.0f * q1*ey - 2.0f * q0*ez);
    
    // 更新四元数
    q0 += q0_dot * invSampleFreq;
    q1 += q1_dot * invSampleFreq;
    q2 += q2_dot * invSampleFreq;
    q3 += q3_dot * invSampleFreq;
    
    // 归一化
    norm = invSqrt(q0*q0 + q1*q1 + q2*q2 + q3*q3);
    f->q0 = q0 * norm;
    f->q1 = q1 * norm;
    f->q2 = q2 * norm;
    f->q3 = q3 * norm;
}

// 6轴融合（省算力版本，不用磁力计）
void Madgwick_Update6Axis(Madgwick_t *f, float gx, float gy, float gz, 
                          float ax, float ay, float az) {
    float q0 = f->q0, q1 = f->q1, q2 = f->q2, q3 = f->q3;
    float beta = f->beta;
    float invSampleFreq = f->invSampleFreq;
    float norm;
    
    // 检查加速度计有效
    float accel_mag = ax*ax + ay*ay + az*az;
    if (accel_mag < 0.001f) return;
    
    // 归一化加速度计
    norm = invSqrt(accel_mag);
    ax *= norm; ay *= norm; az *= norm;
    
    // 计算重力方向
    float vx = 2.0f * (q1*q3 - q0*q2);
    float vy = 2.0f * (q0*q1 + q2*q3);
    float vz = q0*q0 - q1*q1 - q2*q2 + q3*q3;
    
    // 加速度计误差
    float ex = ay*vz - az*vy;
    float ey = az*vx - ax*vz;
    float ez = ax*vy - ay*vx;
    
    // 四元数导数
    float q0_dot = -0.5f * (q1*gx + q2*gy + q3*gz) - beta * (2.0f * q1*ex + 2.0f * q2*ey + 2.0f * q3*ez);
    float q1_dot =  0.5f * (q0*gx - q3*gy + q2*gz) - beta * (-2.0f * q0*ex - 2.0f * q3*ey + 2.0f * q2*ez);
    float q2_dot =  0.5f * (q3*gx + q0*gy - q1*gz) - beta * (2.0f * q3*ex - 2.0f * q0*ey - 2.0f * q1*ez);
    float q3_dot = -0.5f * (q2*gx - q1*gy - q0*gz) - beta * (-2.0f * q2*ex + 2.0f * q1*ey - 2.0f * q0*ez);
    
    // 更新四元数
    q0 += q0_dot * invSampleFreq;
    q1 += q1_dot * invSampleFreq;
    q2 += q2_dot * invSampleFreq;
    q3 += q3_dot * invSampleFreq;
    
    // 归一化
    norm = invSqrt(q0*q0 + q1*q1 + q2*q2 + q3*q3);
    f->q0 = q0 * norm;
    f->q1 = q1 * norm;
    f->q2 = q2 * norm;
    f->q3 = q3 * norm;
}

void Madgwick_GetEuler(Madgwick_t *f, float *pitch, float *roll, float *yaw) {
    float q0 = f->q0, q1 = f->q1, q2 = f->q2, q3 = f->q3;
    
    #define RAD_TO_DEG 57.29578f
    
    *pitch = asin(-2.0f * q1 * q3 + 2.0f * q0 * q2) * RAD_TO_DEG;
    *roll  = atan2(2.0f * q2 * q3 + 2.0f * q0 * q1, 
                   -2.0f * q1 * q1 - 2.0f * q2 * q2 + 1.0f) * RAD_TO_DEG;
    *yaw   = atan2(2.0f * (q1 * q2 + q0 * q3), 
                   q0 * q0 + q1 * q1 - q2 * q2 - q3 * q3) * RAD_TO_DEG;
}

void Madgwick_GetEulerAero(Madgwick_t *f, float *pitch, float *roll, float *yaw) {
    float q0 = f->q0, q1 = f->q1, q2 = f->q2, q3 = f->q3;
    
    #define RAD_TO_DEG 57.29578f
    
    // 航空顺序：先 Yaw，再 Pitch，再 Roll (Z-Y-X)
    *roll  = atan2(2.0f * (q0*q1 + q2*q3), 1.0f - 2.0f * (q1*q1 + q2*q2)) * RAD_TO_DEG;
    *pitch = asin(2.0f * (q0*q2 - q3*q1)) * RAD_TO_DEG;
    *yaw   = atan2(2.0f * (q0*q3 + q1*q2), 1.0f - 2.0f * (q2*q2 + q3*q3)) * RAD_TO_DEG;
}