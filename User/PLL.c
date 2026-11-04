
#include "PLL.h"


EncoderPLL_t encoder_pll;

/* 将位置折算到 [0, CPR) */
static inline float EncoderPLL_WrapPosition(float position)
{
    while (position >= ENC_PLL_CPR)
        position -= ENC_PLL_CPR;

    while (position < 0.0f)
        position += ENC_PLL_CPR;

    return position;
}

/* 将位置误差折算到 [-CPR/2, CPR/2) */
static inline float EncoderPLL_WrapError(float error)
{
    const float half_cpr = 0.5f * ENC_PLL_CPR;

    while (error >= half_cpr)
        error -= ENC_PLL_CPR;

    while (error < -half_cpr)
        error += ENC_PLL_CPR;

    return error;
}

/* 在开启周期更新前调用；默认从静止开始估计 */
void EncoderPLL_Init(EncoderPLL_t *pll, uint32_t encoder_count)
{
    pll->position = EncoderPLL_WrapPosition((float)encoder_count);
    pll->speed = 0.0f;
}

/* 每 0.5 ms 调用一次 */
void EncoderPLL_Update(EncoderPLL_t *pll, uint32_t encoder_count)
{
    const float measured = (float)encoder_count;

    /* 1. 根据已有速度预测位置 */
    float predicted = pll->position + pll->speed * ENC_PLL_TS;
    predicted = EncoderPLL_WrapPosition(predicted);

    /* 2. 计算跨零点修正后的位置误差 */
    const float error =
        EncoderPLL_WrapError(measured - predicted);

    /* 3. 修正位置和速度 */
    pll->position = EncoderPLL_WrapPosition(
        predicted + ENC_PLL_KP * error * ENC_PLL_TS);

    pll->speed += ENC_PLL_KI * error * ENC_PLL_TS;
}

/* 换算为 count/采样周期，方便接入原来的速度环 */
float EncoderPLL_GetCountPerTick(const EncoderPLL_t *pll)
{
    return pll->speed * ENC_PLL_TS;
}

/* 换算为 rpm，用于显示 */
static inline float EncoderPLL_GetRPM(const EncoderPLL_t *pll)
{
    return pll->speed * (60.0f / ENC_PLL_CPR);
}