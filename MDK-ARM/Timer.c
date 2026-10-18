#include "Timer.h"

#define ADC_SAMPLE_HZ          16000UL
#define TIM1_INPUT_CLOCK_HZ    160000000UL
#define TIM1_PERIOD_TICKS      (TIM1_INPUT_CLOCK_HZ / ADC_SAMPLE_HZ)

static ADC_HandleTypeDef s_hadc1;
static TIM_HandleTypeDef s_htim1;

static volatile ADC_InjectedSample_t s_sample;
static uint8_t s_initialized;
static uint8_t s_running;

static HAL_StatusTypeDef ADC1_Injected_Config(void);
static HAL_StatusTypeDef TIM1_CH4_Config(void);

HAL_StatusTypeDef ADC_Injected_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    /* 本模块要求 HCLK = 160 MHz。
     * TIM1 时钟还必须由工程时钟树保证为 160 MHz。
     */
    if (HAL_RCC_GetHCLKFreq() != 160000000UL)
    {
        return HAL_ERROR;
    }

    if (s_initialized != 0U)
    {
        return HAL_OK;
    }

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_ADC12_CLK_ENABLE();
    __HAL_RCC_TIM1_CLK_ENABLE();

    /* PC1: ADC1_IN7
     * PC2: ADC1_IN8
     * PC3: ADC1_IN9
     */
    gpio.Pin  = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &gpio);

    if (TIM1_CH4_Config() != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (ADC1_Injected_Config() != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* 校准必须在启动 ADC 之前执行。 */
    if (HAL_ADCEx_Calibration_Start(&s_hadc1,
                                   ADC_SINGLE_ENDED) != HAL_OK)
    {
        return HAL_ERROR;
    }

    s_sample.ch7 = 0U;
    s_sample.ch8 = 0U;
    s_sample.ch9 = 0U;
    s_sample.sequence = 0U;

    HAL_NVIC_SetPriority(ADC1_2_IRQn, 2U, 0U);
    HAL_NVIC_ClearPendingIRQ(ADC1_2_IRQn);
    HAL_NVIC_EnableIRQ(ADC1_2_IRQn);

    s_initialized = 1U;
    return HAL_OK;
}

static HAL_StatusTypeDef TIM1_CH4_Config(void)
{
    TIM_OC_InitTypeDef oc = {0};
    TIM_ClockConfigTypeDef clock = {0};

    /*
     * f_trigger = f_TIM1 / ((PSC + 1) * (ARR + 1))
     *           = 160 MHz / (1 * 10000)
     *           = 16 kHz
     *
     * CCR4 = 5000：在每个周期中间产生一次 CC4 比较事件。
     * 向上计数，每个周期只匹配一次。
     */
    s_htim1.Instance = TIM1;
    s_htim1.Init.Prescaler         = 0U;
    s_htim1.Init.CounterMode       = TIM_COUNTERMODE_UP;
    s_htim1.Init.Period            = TIM1_PERIOD_TICKS - 1U;
    s_htim1.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    s_htim1.Init.RepetitionCounter = 0U;
    s_htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_OC_Init(&s_htim1) != HAL_OK)
    {
        return HAL_ERROR;
    }

    clock.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&s_htim1, &clock) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* 内部 CC4 比较事件直接触发 ADC，不使用 TRGO。 */
    oc.OCMode       = TIM_OCMODE_TIMING;
    oc.Pulse        = TIM1_PERIOD_TICKS / 2U;
    oc.OCPolarity   = TIM_OCPOLARITY_HIGH;
    oc.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
    oc.OCFastMode   = TIM_OCFAST_DISABLE;
    oc.OCIdleState  = TIM_OCIDLESTATE_RESET;
    oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;

    return HAL_TIM_OC_ConfigChannel(&s_htim1, &oc, TIM_CHANNEL_4);
}

static HAL_StatusTypeDef ADC1_Injected_Config(void)
{
    ADC_InjectionConfTypeDef injected = {0};
    ADC_MultiModeTypeDef multimode = {0};

    s_hadc1.Instance = ADC1;

    /* 同步时钟：HCLK / 4 = 40 MHz。 */
    s_hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    s_hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    s_hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    s_hadc1.Init.GainCompensation      = 0U;
    s_hadc1.Init.ScanConvMode          = ADC_SCAN_ENABLE;

    /* 注入组整个序列完成后才进入中断。 */
    s_hadc1.Init.EOCSelection          = ADC_EOC_SEQ_CONV;

    s_hadc1.Init.LowPowerAutoWait      = DISABLE;
    s_hadc1.Init.ContinuousConvMode    = DISABLE;

    /* 规则组不启动，以下为其基本配置。 */
    s_hadc1.Init.NbrOfConversion       = 1U;
    s_hadc1.Init.DiscontinuousConvMode = DISABLE;
    s_hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    s_hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    s_hadc1.Init.DMAContinuousRequests = DISABLE;
    s_hadc1.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;
    s_hadc1.Init.OversamplingMode      = DISABLE;

    if (HAL_ADC_Init(&s_hadc1) != HAL_OK)
    {
        return HAL_ERROR;
    }

    multimode.Mode = ADC_MODE_INDEPENDENT;
    if (HAL_ADCEx_MultiModeConfigChannel(&s_hadc1,
                                         &multimode) != HAL_OK)
    {
        return HAL_ERROR;
    }

    injected.InjectedSamplingTime = ADC_SAMPLETIME_47CYCLES_5;
    injected.InjectedSingleDiff   = ADC_SINGLE_ENDED;
    injected.InjectedOffsetNumber = ADC_OFFSET_NONE;
    injected.InjectedOffset       = 0U;
    injected.InjectedNbrOfConversion = 3U;

    /* 一个外部触发转换完整的三通道序列。 */
    injected.InjectedDiscontinuousConvMode = DISABLE;
    injected.AutoInjectedConv             = DISABLE;

    /* 不使用注入上下文队列，重复执行同一序列。 */
    injected.QueueInjectedContext = DISABLE;

    injected.ExternalTrigInjecConv =
        ADC_EXTERNALTRIGINJEC_T1_CC4;
    injected.ExternalTrigInjecConvEdge =
        ADC_EXTERNALTRIGINJECCONV_EDGE_RISING;

    injected.InjecOversamplingMode = DISABLE;

    /* Rank 1：通道7 */
    injected.InjectedChannel = ADC_CHANNEL_7;
    injected.InjectedRank    = ADC_INJECTED_RANK_1;
    if (HAL_ADCEx_InjectedConfigChannel(&s_hadc1,
                                        &injected) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Rank 2：通道8 */
    injected.InjectedChannel = ADC_CHANNEL_8;
    injected.InjectedRank    = ADC_INJECTED_RANK_2;
    if (HAL_ADCEx_InjectedConfigChannel(&s_hadc1,
                                        &injected) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Rank 3：通道9 */
    injected.InjectedChannel = ADC_CHANNEL_9;
    injected.InjectedRank    = ADC_INJECTED_RANK_3;
    return HAL_ADCEx_InjectedConfigChannel(&s_hadc1, &injected);
}

HAL_StatusTypeDef ADC_Injected_Start(void)
{
    if (s_initialized == 0U)
    {
        return HAL_ERROR;
    }

    if (s_running != 0U)
    {
        return HAL_OK;
    }

    __HAL_TIM_SET_COUNTER(&s_htim1, 0U);
    __HAL_TIM_CLEAR_FLAG(&s_htim1, TIM_FLAG_UPDATE | TIM_FLAG_CC4);

    /* 先让 ADC 等待外部触发，再启动定时器。 */
    if (HAL_ADCEx_InjectedStart_IT(&s_hadc1) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* 无需启动 TIM1 中断。 */
    if (HAL_TIM_OC_Start(&s_htim1, TIM_CHANNEL_4) != HAL_OK)
    {
        (void)HAL_ADCEx_InjectedStop_IT(&s_hadc1);
        return HAL_ERROR;
    }

    s_running = 1U;
    return HAL_OK;
}

HAL_StatusTypeDef ADC_Injected_Stop(void)
{
    HAL_StatusTypeDef tim_status;
    HAL_StatusTypeDef adc_status;

    if (s_running == 0U)
    {
        return HAL_OK;
    }

    tim_status = HAL_TIM_OC_Stop(&s_htim1, TIM_CHANNEL_4);
    adc_status = HAL_ADCEx_InjectedStop_IT(&s_hadc1);

    if ((tim_status != HAL_OK) || (adc_status != HAL_OK))
    {
        return HAL_ERROR;
    }

    s_running = 0U;
    return HAL_OK;
}

/*
 * ADC1 和 ADC2 共用中断入口。
 * 工程中只能保留一份 ADC1_2_IRQHandler。
 */
void ADC1_2_IRQHandler(void)
{
    HAL_ADC_IRQHandler(&s_hadc1);

    /* 若同时使用 ADC2，在此补充 ADC2 的 IRQ 处理。 */
}

/* 在 ADC 注入组序列完成中断上下文中执行。 */
void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        s_sample.ch7 = (uint16_t)HAL_ADCEx_InjectedGetValue(
            hadc, ADC_INJECTED_RANK_1);

        s_sample.ch8 = (uint16_t)HAL_ADCEx_InjectedGetValue(
            hadc, ADC_INJECTED_RANK_2);

        s_sample.ch9 = (uint16_t)HAL_ADCEx_InjectedGetValue(
            hadc, ADC_INJECTED_RANK_3);

        s_sample.sequence++;

        /* 可在此添加耗时很短的控制算法。
         * 不要在这里使用阻塞串口输出或 HAL_Delay。
         */
    }
}

uint8_t ADC_Injected_GetLatest(ADC_InjectedSample_t *sample)
{
    uint32_t primask;

    if (sample == NULL)
    {
        return 0U;
    }

    /* 短暂关中断，保证三个结果来自同一序列。 */
    primask = __get_PRIMASK();
    __disable_irq();

    sample->ch7      = s_sample.ch7;
    sample->ch8      = s_sample.ch8;
    sample->ch9      = s_sample.ch9;
    sample->sequence = s_sample.sequence;

    __set_PRIMASK(primask);

    return (sample->sequence != 0U) ? 1U : 0U;
}