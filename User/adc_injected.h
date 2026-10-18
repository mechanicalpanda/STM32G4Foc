#ifndef ADC_INJECTED_H
#define ADC_INJECTED_H

#include "stm32g4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0: TIM1 TRGO hardware trigger; 1: one software-triggered sequence. */
#ifndef ADC_TEST_SOFTWARE_TRIGGER
#define ADC_TEST_SOFTWARE_TRIGGER 0U
#endif

extern volatile uint32_t count_test;
extern volatile uint16_t adc_raw[3];
extern volatile uint32_t adc_test_stage;
extern volatile HAL_StatusTypeDef adc_test_status;
extern volatile uint32_t adc_test_error;

/* Call once after MX_ADC1_Init() and MX_TIM1_Init(), before starting either.
 * Requires TIM1 input clock = 160 MHz. Configures TIM1 base, CH4 and TRGO.
 * Uses CubeMX handles hadc1/htim1 and the existing ADC1_2_IRQHandler.
 * adc_raw[0..2] correspond to ADC1 channels 7/8/9.
 */
HAL_StatusTypeDef ADC_TestStart(void);

#ifdef __cplusplus
}
#endif

#endif
