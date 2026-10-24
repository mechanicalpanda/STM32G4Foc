#include "adc_injected.h"
#include "main.h"
#include <stdbool.h>
#include "FOC.h"
/* Defined by CubeMX in main.c (or adc.c/tim.c in split-file projects). */
extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim1;

volatile uint32_t count_test = 0U;
volatile uint16_t adc_raw[3] = {0U};

/*
 * 调试阶段：
 * 0 = 尚未开始
 * 1 = 正在配置
 * 2 = 校准完成
 * 3 = ADC启动成功
 * 4 = 定时器启动成功/软件触发启动完成
 */
volatile uint32_t adc_test_stage = 0U;

/* 0=HAL_OK，1=HAL_ERROR，2=HAL_BUSY，3=HAL_TIMEOUT */
volatile HAL_StatusTypeDef adc_test_status = HAL_OK;
volatile uint32_t adc_test_error = 0U;

extern uint8_t sector_sample;
extern bool isReSampleAdc;

uint16_t curr_offset_a = CURRENT_ZERO_OFFSET_ADC;
uint16_t curr_offset_b = CURRENT_ZERO_OFFSET_ADC;
uint16_t curr_offset_c = CURRENT_ZERO_OFFSET_ADC;

ThreePhase_t current = {0.0};
float testang = 0.0;

#define DYN_ADC_SAMPL
Clarke_t Clarke_out;
Park_t   Park_out;
float Angle = 0.0;
extern TIM_HandleTypeDef htim3;

const int speed_Div = 16; //speed loop 1K
int speed_Div_count = 0;
int enc = 0;
int enc_last = 0;
int speed_count = 0;

PI_Controller PI_Cur_d  = {
    .Kp = KP_cur,
    .Ki = KI_cur,
    .Ts = Ts_Cur,
    .integral = 0.0,
    .integral_limit = 2.0,
    .output_limit = 1.0,
    .output = 0.0
};

PI_Controller PI_Cur_q  = {
    .Kp = KP_cur,
    .Ki = KI_cur,
    .Ts = Ts_Cur,
    .integral = 0.0,
    .integral_limit = 2.0,
    .output_limit = 1.0,
    .output = 0.0
};

PI_Controller current_speed_pid = {
    .Kp = 0.0088,
    .Ki = 16.54,
    .Ts = 0.001,
    .integral = 0.0,
    .integral_limit = 1.0,
    .output_limit = 1.0,
    .output = 0.0
};

float PI_Update(PI_Controller *pi, float ref, float feedback) 
{
    float error = ref - feedback;
	
    pi->integral += pi->Ki * error * pi->Ts;
	
    if (pi->integral > pi->integral_limit)
		{
        pi->integral = pi->integral_limit;
		}
    else if (pi->integral < -pi->integral_limit)
		{
        pi->integral = -pi->integral_limit;
		}
		
    pi->output = pi->Kp * error + pi->integral;
		
    if (pi->output > pi->output_limit)
        pi->output = pi->output_limit;
    else if (pi->output < -pi->output_limit)
        pi->output = -pi->output_limit;
    return pi->output;
}

float PI_Update_AntiWindup(PI_Controller *pi, float ref, float feedback) 
{
    float error = ref - feedback;
    float output_unclamped = pi->Kp * error + pi->integral;
    
    if (output_unclamped > -pi->output_limit && output_unclamped < pi->output_limit) 
		{
        pi->integral += pi->Ki * error * pi->Ts;
    }
    
    pi->output = pi->Kp * error + pi->integral;
    
    if (pi->output > pi->output_limit)
        pi->output = pi->output_limit;
    else if (pi->output < -pi->output_limit)
        pi->output = -pi->output_limit;
    
    return pi->output;
}

float vd = 0.0;
float vq = 0.0;

//#define OPENLOOP

void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
			
			 HAL_GPIO_TogglePin(LED0_GPIO_Port,LED0_Pin);
        adc_raw[0] = (uint16_t)HAL_ADCEx_InjectedGetValue(
            hadc, ADC_INJECTED_RANK_1);

        adc_raw[1] = (uint16_t)HAL_ADCEx_InjectedGetValue(
            hadc, ADC_INJECTED_RANK_2);

        adc_raw[2] = (uint16_t)HAL_ADCEx_InjectedGetValue(
            hadc, ADC_INJECTED_RANK_3);
			
			count_test++;
			
			Angle =  _normalizeAngle(__HAL_TIM_GET_COUNTER(&htim3) * POLEPAIR * _2PI / ELEC_COUNT);
			 
#ifdef 		DYN_ADC_SAMPL	
			if(AdcInjectSel == ADCINVALDE)
			{
				; //无效的ADC采样值，使用上一次的采样值
			}
			else if(AdcInjectSel == THREESAMPLE)
		  {
      current.a = (adc_raw[0] - curr_offset_a) * ADCbitCurrent;
		  current.b = (adc_raw[1] - curr_offset_b) * ADCbitCurrent;
	    current.c = (adc_raw[2] - curr_offset_c) * ADCbitCurrent;
			}
      else if(AdcInjectSel == TWOSAMPLE)		
			{		
				
			current.a = (adc_raw[0] - curr_offset_a) * ADCbitCurrent;
		  current.b = (adc_raw[1] - curr_offset_b) * ADCbitCurrent;
	    current.c = (adc_raw[2] - curr_offset_c) * ADCbitCurrent;
        
				switch (sector_sample)
				{
					
				case SECTOR_1: // 扇区1 重构A
			  case SECTOR_6: // 扇区6
				current.a = -(current.b+current.c);	
					break;
				case SECTOR_2: // 扇区2 重构B
				case SECTOR_3: // 扇区3
			  current.b = -(current.a+current.c);	
					
					break;
				case SECTOR_4: // 扇区4 重构C
				case SECTOR_5: // 扇区5
				current.c = -(current.a+current.b);		
					break;
				
				default:
				
					break;

			 }
		 }
#else
       current.a = (adc_raw[0] - curr_offset_a) * ADCbitCurrent;
		   current.b = (adc_raw[1] - curr_offset_b) * ADCbitCurrent;
	     current.c = (adc_raw[2] - curr_offset_c) * ADCbitCurrent;
#endif			
		
		Clarke_out = Clarke_Transform(current);

#ifdef OPENLOOP		 
    Park_out = Park_Transform(Clarke_out, _normalizeAngle(testang)); 		
    testang = testang + 0.004;		 
#else
    Park_out = Park_Transform(Clarke_out, Angle); 		 
#endif		 
	
		float Id_trg = 0.0;
		float Iq_trg = 0.2;
		 
		if( speed_Div_count++ >= 15)
		{
			 speed_Div_count = 0;
			 
			 enc = (uint32_t)(__HAL_TIM_GET_COUNTER(&htim3));
			 
			 speed_count = enc - enc_last;
			  
			if((speed_count > 0)&&(__HAL_TIM_IS_TIM_COUNTING_DOWN(&htim3) == 1))
			{
				 speed_count = speed_count - ELEC_COUNT;
			}
			if((speed_count < 0)&&(__HAL_TIM_IS_TIM_COUNTING_DOWN(&htim3) == 0))
			{
				 speed_count = speed_count + ELEC_COUNT;
			}
			 
			 enc_last = enc;
			
			Iq_trg = PI_Update(&current_speed_pid, 50, speed_count);
			
		}
		//testangle = 0.0;
		
		
		
		vd = PI_Update(&PI_Cur_d, Id_trg, Park_out.d);
		vq = PI_Update(&PI_Cur_q, Iq_trg, Park_out.q);
		
		
#ifdef OPENLOOP		 		
		setPhaseVoltage(0.0, 1.8, testang);
#else
		setPhaseVoltage(vq, vd, Angle);
#endif
		 
		 //
    }
}

HAL_StatusTypeDef ADC_TestStart(void)
{
    ADC_InjectionConfTypeDef injected = {0};
    TIM_OC_InitTypeDef oc = {0};
    TIM_MasterConfigTypeDef master = {0};
    HAL_StatusTypeDef status;
    uint32_t i;

    const uint32_t channels[3] =
    {
        ADC_CHANNEL_7,
        ADC_CHANNEL_8,
        ADC_CHANNEL_9
    };

    const uint32_t ranks[3] =
    {
        ADC_INJECTED_RANK_1,
        ADC_INJECTED_RANK_2,
        ADC_INJECTED_RANK_3
    };

    adc_test_stage = 1U;

    /*
     * 前提：在 MX_ADC1_Init()、MX_TIM1_Init() 之后调用，
     * 此前不要启动 ADC 或 TIM1。
     */
    __HAL_TIM_DISABLE(&htim1);

    /* 一个完整中心对齐周期为62.5us，即16kHz。 */
    htim1.Init.Prescaler         = 0U;
    htim1.Init.CounterMode       = TIM_COUNTERMODE_CENTERALIGNED1;
    htim1.Init.Period            = 5000U;
    htim1.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0U;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    status = HAL_TIM_PWM_Init(&htim1);
    if (status != HAL_OK)
    {
        return status;
    }

    /*
     * PWM2：
     * 上数经过CCR4时，OC4REF产生上升沿；
     * 下数经过CCR4时，OC4REF产生下降沿。
     * ADC只选择上升沿，每完整PWM周期触发一次。
     */
    oc.OCMode       = TIM_OCMODE_PWM2;
    oc.Pulse        = 4800U;
    oc.OCPolarity   = TIM_OCPOLARITY_HIGH;
    oc.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
    oc.OCFastMode   = TIM_OCFAST_DISABLE;
    oc.OCIdleState  = TIM_OCIDLESTATE_RESET;
    oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;

    status = HAL_TIM_PWM_ConfigChannel(
        &htim1, &oc, TIM_CHANNEL_4);

    if (status != HAL_OK)
    {
        return status;
    }

    /* TRGO跟随OC4REF，不使用更新事件触发ADC。 */
    master.MasterOutputTrigger  = TIM_TRGO_OC4REF;
    master.MasterOutputTrigger2 = TIM_TRGO2_RESET;
    master.MasterSlaveMode      = TIM_MASTERSLAVEMODE_DISABLE;

    status = HAL_TIMEx_MasterConfigSynchronization(
        &htim1, &master);

    if (status != HAL_OK)
    {
        return status;
    }

    /* Configure phases while TIM1 is stopped; keep all six outputs LOW. */
    status = TIM1_ThreePhase_Init();
    if (status != HAL_OK) return status;

    /* 三路注入序列全部完成后产生一次中断。 */
    hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;

    status = HAL_ADC_Init(&hadc1);
    if (status != HAL_OK)
    {
        return status;
    }

    injected.InjectedNbrOfConversion = 3U;
    injected.InjectedSamplingTime   = ADC_SAMPLETIME_47CYCLES_5;
    injected.InjectedSingleDiff     = ADC_SINGLE_ENDED;
    injected.InjectedOffsetNumber   = ADC_OFFSET_NONE;
    injected.InjectedOffset         = 0U;

    injected.InjectedDiscontinuousConvMode = DISABLE;
    injected.AutoInjectedConv             = DISABLE;
    injected.QueueInjectedContext         = DISABLE;
    injected.InjecOversamplingMode        = DISABLE;

#if ADC_TEST_SOFTWARE_TRIGGER
    injected.ExternalTrigInjecConv =
        ADC_INJECTED_SOFTWARE_START;

    injected.ExternalTrigInjecConvEdge =
        ADC_EXTERNALTRIGINJECCONV_EDGE_NONE;
#else
    injected.ExternalTrigInjecConv =
        ADC_EXTERNALTRIGINJEC_T1_TRGO;

    injected.ExternalTrigInjecConvEdge =
        ADC_EXTERNALTRIGINJECCONV_EDGE_RISING;
#endif

    for (i = 0U; i < 3U; i++)
    {
        injected.InjectedChannel = channels[i];
        injected.InjectedRank    = ranks[i];

        status = HAL_ADCEx_InjectedConfigChannel(
            &hadc1, &injected);

        if (status != HAL_OK)
        {
            return status;
        }
    }

    status = HAL_ADCEx_Calibration_Start(
        &hadc1, ADC_SINGLE_ENDED);

    if (status != HAL_OK)
    {
        return status;
    }

    adc_test_stage = 2U;

    count_test = 0U;
    adc_test_error = 0U;

    __HAL_TIM_SET_COUNTER(&htim1, 0U);
    __HAL_TIM_CLEAR_FLAG(
        &htim1, TIM_FLAG_UPDATE | TIM_FLAG_CC4);

    HAL_NVIC_SetPriority(ADC1_2_IRQn, 1U, 0U);
    HAL_NVIC_ClearPendingIRQ(ADC1_2_IRQn);
    HAL_NVIC_EnableIRQ(ADC1_2_IRQn);

    /* 先启动ADC，让它等待定时器触发。 */
    status = HAL_ADCEx_InjectedStart_IT(&hadc1);

    if (status != HAL_OK)
    {
        return status;
    }

    adc_test_stage = 3U;

#if !ADC_TEST_SOFTWARE_TRIGGER
    status = HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);

    if (status != HAL_OK)
    {
        (void)HAL_ADCEx_InjectedStop_IT(&hadc1);
        return status;
    }
#endif

    adc_test_stage = 4U;
    return HAL_OK;
}

/* TIM1 three-phase complementary PWM, sharing the ADC trigger timer. */
#define PHASE_ENABLE_MASK (TIM_CCER_CC1E | TIM_CCER_CC1NE | \
                           TIM_CCER_CC2E | TIM_CCER_CC2NE | \
                           TIM_CCER_CC3E | TIM_CCER_CC3NE)
static uint8_t s_initialized = 0U;
volatile uint32_t tim1_pwm_running = 0U;

static void PhasePinsLow(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15, GPIO_PIN_RESET);
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_PULLDOWN;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10;
    HAL_GPIO_Init(GPIOA, &gpio);
    gpio.Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOB, &gpio);
}

static void PhasePinsAF(void)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLDOWN;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = GPIO_AF6_TIM1;
    gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10;
    HAL_GPIO_Init(GPIOA, &gpio);
    gpio.Pin = GPIO_PIN_13 | GPIO_PIN_14;
    HAL_GPIO_Init(GPIOB, &gpio);
    /* STM32G474 PB15 uses AF4, unlike PB13/PB14. */
    gpio.Alternate = GPIO_AF4_TIM1;
    gpio.Pin = GPIO_PIN_15;
    HAL_GPIO_Init(GPIOB, &gpio);
}

HAL_StatusTypeDef TIM1_ThreePhase_Init(void)
{
    TIM_OC_InitTypeDef oc = {0};
    TIM_BreakDeadTimeConfigTypeDef bd = {0};
    const uint32_t channels[3] = {TIM_CHANNEL_1, TIM_CHANNEL_2, TIM_CHANNEL_3};
    uint32_t i;
    HAL_StatusTypeDef status;

    if ((htim1.Instance != TIM1) ||
        (HAL_RCC_GetPCLK2Freq() != 160000000UL) ||
        (HAL_RCC_GetHCLKFreq() != 160000000UL) ||
        (TIM1->PSC != 0U) || (TIM1->ARR != TIM1_PWM_PERIOD_TICKS) ||
        ((TIM1->CR1 & TIM_CR1_CKD) != 0U) ||
        ((TIM1->CR1 & TIM_CR1_CMS) != TIM_COUNTERMODE_CENTERALIGNED1) ||
        ((TIM1->CR1 & TIM_CR1_CEN) != 0U) ||
        ((TIM1->BDTR & TIM_BDTR_LOCK) != 0U))
    {
        return HAL_ERROR;
    }

    PhasePinsLow();
    CLEAR_BIT(TIM1->CCER, PHASE_ENABLE_MASK);
    /* Use direct CCER enables, not commutation-event preloading. */
    CLEAR_BIT(TIM1->CR2, TIM_CR2_CCPC);
    s_initialized = 0U;
    tim1_pwm_running = 0U;

    oc.OCMode = TIM_OCMODE_PWM1;
    oc.Pulse = TIM1_PWM_PERIOD_TICKS / 2U;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    oc.OCIdleState = TIM_OCIDLESTATE_RESET;
    oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    for (i = 0U; i < 3U; i++)
    {
        status = HAL_TIM_PWM_ConfigChannel(&htim1, &oc, channels[i]);
        if (status != HAL_OK) return status;
        __HAL_TIM_ENABLE_OCxPRELOAD(&htim1, channels[i]);
    }

    bd.OffStateRunMode = TIM_OSSR_ENABLE;
    bd.OffStateIDLEMode = TIM_OSSI_ENABLE;
    bd.LockLevel = TIM_LOCKLEVEL_OFF;
    bd.DeadTime = TIM1_PWM_DEADTIME_DTG;
    bd.BreakState = TIM_BREAK_DISABLE;
    bd.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
    bd.BreakFilter = 0U;
    bd.BreakAFMode = TIM_BREAK_AFMODE_INPUT;
    bd.Break2State = TIM_BREAK2_DISABLE;
    bd.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
    bd.Break2Filter = 0U;
    bd.Break2AFMode = TIM_BREAK_AFMODE_INPUT;
    bd.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
    status = HAL_TIMEx_ConfigBreakDeadTime(&htim1, &bd);
    if (status != HAL_OK) return status;

    /* Load initial compares before ADC is armed. Never issue UG at runtime. */
    TIM1->EGR = TIM_EGR_UG;
    __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_UPDATE | TIM_FLAG_CC1 |
                       TIM_FLAG_CC2 | TIM_FLAG_CC3 | TIM_FLAG_CC4);
    s_initialized = 1U;
    return HAL_OK;
}

HAL_StatusTypeDef TIM1_ThreePhase_Start(void)
{
    if ((s_initialized == 0U) || ((TIM1->CR1 & TIM_CR1_CEN) == 0U) ||
        ((TIM1->CCER & TIM_CCER_CC4E) == 0U)) return HAL_ERROR;
    if (tim1_pwm_running != 0U) return HAL_OK;

    /* All three complementary pairs enabled by one register write.
     * Keep CH4, the timer counter and the ADC trigger running.
     */
    SET_BIT(TIM1->CCER, PHASE_ENABLE_MASK);
    __HAL_TIM_MOE_ENABLE(&htim1);
    PhasePinsAF();
    tim1_pwm_running = 1U;
    return HAL_OK;
}

void TIM1_ThreePhase_Stop(void)
{
    if (htim1.Instance == TIM1)
    {
        CLEAR_BIT(TIM1->CCER, PHASE_ENABLE_MASK);
        PhasePinsLow();
    }
    tim1_pwm_running = 0U;
}

HAL_StatusTypeDef TIM1_ThreePhase_SetCompare(uint16_t u, uint16_t v, uint16_t w)
{
    uint32_t saved_udis;
    if ((s_initialized == 0U) || (u > TIM1_PWM_PERIOD_TICKS) ||
        (v > TIM1_PWM_PERIOD_TICKS) || (w > TIM1_PWM_PERIOD_TICKS)) return HAL_ERROR;
    saved_udis = TIM1->CR1 & TIM_CR1_UDIS;
    /* Prevent an update from loading only part of this three-phase command. */
    SET_BIT(TIM1->CR1, TIM_CR1_UDIS);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, u);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, v);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, w);
    if (saved_udis == 0U) CLEAR_BIT(TIM1->CR1, TIM_CR1_UDIS);
    return HAL_OK;
}

volatile TIM1_PWM_Debug_t tim1_pwm_debug;
volatile HAL_StatusTypeDef pwm_compare_status = (HAL_StatusTypeDef)0xFF;
volatile HAL_StatusTypeDef pwm_start_status = (HAL_StatusTypeDef)0xFF;

void TIM1_ThreePhase_DebugRead(void)
{
    tim1_pwm_debug.cr1 = TIM1->CR1;
    tim1_pwm_debug.cr2 = TIM1->CR2;
    tim1_pwm_debug.ccer = TIM1->CCER;
    tim1_pwm_debug.ccmr1 = TIM1->CCMR1;
    tim1_pwm_debug.ccmr2 = TIM1->CCMR2;
    tim1_pwm_debug.bdtr = TIM1->BDTR;
    tim1_pwm_debug.cnt = TIM1->CNT;
    tim1_pwm_debug.arr = TIM1->ARR;
    tim1_pwm_debug.ccr1 = TIM1->CCR1;
    tim1_pwm_debug.ccr2 = TIM1->CCR2;
    tim1_pwm_debug.ccr3 = TIM1->CCR3;
    tim1_pwm_debug.ccr4 = TIM1->CCR4;
    tim1_pwm_debug.gpioa_moder = GPIOA->MODER;
    tim1_pwm_debug.gpioa_afrh = GPIOA->AFR[1];
    tim1_pwm_debug.gpiob_moder = GPIOB->MODER;
    tim1_pwm_debug.gpiob_afrh = GPIOB->AFR[1];
}
