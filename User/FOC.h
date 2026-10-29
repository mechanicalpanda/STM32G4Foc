#ifndef __FOC_H
#define __FOC_H

#include <stdint.h>
#include "arm_math.h"

#define SECTOR_1        (uint32_t)3

#define SECTOR_2        (uint32_t)1

#define SECTOR_3        (uint32_t)5

#define SECTOR_4        (uint32_t)4

#define SECTOR_5        (uint32_t)6

#define SECTOR_6        (uint32_t)2

#define CURRENT_ZERO_OFFSET_ADC 1488

#define ADCbitCurrent (0.00671384)

typedef enum{THREESAMPLE, TWOSAMPLE,ADCINVALDE} ADCINJ;
extern ADCINJ AdcInjectSel;
typedef struct {
    float32_t a;
    float32_t b;
    float32_t c;
} ThreePhase_t;

typedef struct {
    float32_t alpha;
    float32_t beta;
} Clarke_t;

typedef struct {
    float32_t d;
    float32_t q;
} Park_t;

typedef struct {
    float32_t Kp;
    float32_t Ki;
    float32_t Kd;
    float32_t integral;
    float32_t prev_error;
    float32_t output_min;
    float32_t output_max;
} PID_Controller;

typedef struct{
	float32_t Ta;
	float32_t Tb;
	float32_t Tc;
}svpwm_;

#define FOC_PWM_UPDATE_OK       0U
#define FOC_PWM_UPDATE_MISSED   1U

#define FOC_ADC_RECONSTRUCT_NONE  0U
#define FOC_ADC_RECONSTRUCT_A     1U
#define FOC_ADC_RECONSTRUCT_B     2U
#define FOC_ADC_RECONSTRUCT_C     3U

extern volatile uint8_t foc_pwm_update_missed;
extern volatile uint8_t foc_adc_reconstruction_mode;

float PID_Calculate(PID_Controller *pid, float error, float dt);
void PWM_Init(void);
uint16_t setPhaseVoltage(float Uq, float Ud, float angle_el);
void Set_PWM(float _CCR1, float _CCR2, float _CCR3);
Clarke_t Clarke_Transform(ThreePhase_t current) ;
Park_t Park_Transform(Clarke_t clarke, float theta);
Clarke_t Inverse_Park_Transform(Park_t park, float theta) ;
float _normalizeAngle(float angle);

#define  _2PI  (6.28318530717958f)
#define  _0_5_PI  (1.57079632679f)
#endif
