#ifndef __PLL_H
#define __PLL_H
#include <stdint.h>
/* 可修改参数 */
#define ENC_PLL_CPR          10000.0f
#define ENC_PLL_SAMPLE_HZ    2000.0f
#define ENC_PLL_FREQ_HZ       30.0f
#define ENC_PLL_DAMPING       1.0f

#define ENC_PLL_TS           (1.0f / ENC_PLL_SAMPLE_HZ)
#define ENC_PLL_WN           (6.28318530718f * ENC_PLL_FREQ_HZ)
#define ENC_PLL_KP           (2.0f * ENC_PLL_DAMPING * ENC_PLL_WN)
#define ENC_PLL_KI           (ENC_PLL_WN * ENC_PLL_WN)

typedef struct
{
    float position;     /* 估计位置：[0, CPR)，单位 count */
    float speed;        /* 估计速度：count/s */
} EncoderPLL_t;

extern EncoderPLL_t encoder_pll;
void EncoderPLL_Init(EncoderPLL_t *pll, uint32_t encoder_count);
void EncoderPLL_Update(EncoderPLL_t *pll, uint32_t encoder_count);
float EncoderPLL_GetCountPerTick(const EncoderPLL_t *pll);
#endif
