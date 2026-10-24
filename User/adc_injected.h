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

/* Three-phase PWM API (same TIM1 as ADC triggering). */
#define TIM1_PWM_PERIOD_TICKS  5000U
#define TIM1_PWM_DEADTIME_DTG  96U /* 600 ns at TIM1=160MHz, CKD=DIV1 */
extern volatile uint32_t tim1_pwm_running;

/* Invoked inside ADC_TestStart(), while TIM1 is stopped.
 * Does not reinitialize the timer base, CH4 or TRGO.
 * CH1/2/3: PA8/PA9/PA10 AF6; CH1N/2N: PB13/PB14 AF6;
 * CH3N: PB15 AF4. Both driver inputs are active HIGH.
 * Initialization leaves all six pins driven LOW, phase outputs disabled.
 */
HAL_StatusTypeDef TIM1_ThreePhase_Init(void);
/* Call from main after ADC_TestStart succeeds. No automatic power enable. */
HAL_StatusTypeDef TIM1_ThreePhase_Start(void);
/* Disables phase outputs and drives pins LOW; CH4/ADC keep running.
 * This is a software stop, not a hardware overcurrent protection function.
 */
void TIM1_ThreePhase_Stop(void);
/* Call SetCompare only from the ADC ISR; no global interrupt masking is used.
 * Raw compare counts 0..5000, transferred together on a natural update.
 * Do not call HAL_TIM_PWM_Start/Stop on CH1-3 separately from this module.
 */
HAL_StatusTypeDef TIM1_ThreePhase_SetCompare(uint16_t u, uint16_t v, uint16_t w);

/* Live register snapshots for Watch. Capture while the program is running. */
typedef struct
{
    uint32_t cr1, cr2, ccer, ccmr1, ccmr2, bdtr;
    uint32_t cnt, arr, ccr1, ccr2, ccr3, ccr4;
    uint32_t gpioa_moder, gpioa_afrh, gpiob_moder, gpiob_afrh;
} TIM1_PWM_Debug_t;
extern volatile TIM1_PWM_Debug_t tim1_pwm_debug;
extern volatile HAL_StatusTypeDef pwm_compare_status;
extern volatile HAL_StatusTypeDef pwm_start_status;
void TIM1_ThreePhase_DebugRead(void);

#define ENCODER_COUNT 2500
#define POLEPAIR      5
#define ELEC_COUNT (ENCODER_COUNT * 4 - 1)
#define MOTOR_MAX_SPEED_RPM 3800
#define CURRLOOPFREQ 1000
#define SPEED_LOOP_COUNT (3800 * ELEC_COUNT / 60 / CURRLOOPFREQ)

#define MOTOR_R 0.235
#define MOTOR_L 0.00027
#define CUR_LOOP_BW_Hz 1500
#define KP_cur (MOTOR_L * CUR_LOOP_BW_Hz * 2 * 3.1415926)
#define KI_cur (MOTOR_R * CUR_LOOP_BW_Hz * 2 * 3.1415926)
#define Ts_Cur     0.0000625
typedef struct {
    float Kp;
    float Ki;
    float Ts;
    float integral;
    float integral_limit;
    float output_limit;
    float output;
} PI_Controller;

#ifdef __cplusplus
}
#endif

#endif
