#include "adc_injected.h"
#include "main.h"
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

void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
			 //HAL_GPIO_TogglePin(LED0_GPIO_Port,LED0_Pin);
        adc_raw[0] = (uint16_t)HAL_ADCEx_InjectedGetValue(
            hadc, ADC_INJECTED_RANK_1);

        adc_raw[1] = (uint16_t)HAL_ADCEx_InjectedGetValue(
            hadc, ADC_INJECTED_RANK_2);

        adc_raw[2] = (uint16_t)HAL_ADCEx_InjectedGetValue(
            hadc, ADC_INJECTED_RANK_3);

        count_test++;
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
