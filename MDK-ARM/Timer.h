#ifndef ADC_INJECTED_H
#define ADC_INJECTED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32g4xx_hal.h"

typedef struct
{
    uint16_t ch7;
    uint16_t ch8;
    uint16_t ch9;
    uint32_t sequence;
} ADC_InjectedSample_t;

/* 初始化后，再调用 Start 开始采样。 */
HAL_StatusTypeDef ADC_Injected_Init(void);
HAL_StatusTypeDef ADC_Injected_Start(void);
HAL_StatusTypeDef ADC_Injected_Stop(void);

/* 获取同一注入序列的三路结果。
 * 返回 0 表示尚未完成第一次采样，1 表示数据有效。
 * 在主循环中调用。
 */
uint8_t ADC_Injected_GetLatest(ADC_InjectedSample_t *sample);

#ifdef __cplusplus
}
#endif

#endif