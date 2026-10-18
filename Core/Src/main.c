/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
ADC_HandleTypeDef hadc1;

TIM_HandleTypeDef htim1;
uint8_t receive_buff[256] = {0};
UART_HandleTypeDef huart1;

#define DATA_COUNT_MAX 1
typedef struct
{
    float fdata[DATA_COUNT_MAX];
    uint8_t tail[4];
}Vofa_Packet;

Vofa_Packet Vofa_packet;	

void Vofa_Init(Vofa_Packet *frame)
{
    frame->tail[0] = 0x00;
    frame->tail[1] = 0x00;
    frame->tail[2] = 0x80;
    frame->tail[3] = 0x7f;
}

void Vofa_SendFloat(Vofa_Packet *packet)
 {
    uint8_t byte[4];
    uint8_t i;
    uint32_t temp;
	  uint8_t data[DATA_COUNT_MAX * 4 + 4] = {0};
		data[DATA_COUNT_MAX * 4 + 4 - 1] = 0x7f;
	  data[DATA_COUNT_MAX * 4 + 4 - 2] = 0x80;
		data[DATA_COUNT_MAX * 4 + 4 - 3] = 0x00;
		data[DATA_COUNT_MAX * 4 + 4 - 4] = 0x00;
		
    for(i=0;i<DATA_COUNT_MAX;i++)
    {
        temp = *((uint32_t*)&packet->fdata[i]);  // Reinterpret float as Uint32
        data[0+i*4] = (uint8_t)(temp);
        data[1+i*4] = (uint8_t)(temp >> 8);
        data[2+i*4] = (uint8_t)(temp >> 16);
        data[3+i*4] = (uint8_t)(temp >> 24);
			 //HAL_UART_Transmit(&huart1, (const uint8_t *)byte, 4, 0xff);
    }     
    HAL_UART_Transmit_DMA(&huart1,data,DATA_COUNT_MAX * 4 + 4);
 }
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/*
 * 0：TIM1 硬件触发，连续采样
 * 1：软件触发，仅采样一次，用于隔离定时器触发问题
 */
#define ADC_TEST_SOFTWARE_TRIGGER  0U

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
        adc_raw[0] = (uint16_t)HAL_ADCEx_InjectedGetValue(
            hadc, ADC_INJECTED_RANK_1);

        adc_raw[1] = (uint16_t)HAL_ADCEx_InjectedGetValue(
            hadc, ADC_INJECTED_RANK_2);

        adc_raw[2] = (uint16_t)HAL_ADCEx_InjectedGetValue(
            hadc, ADC_INJECTED_RANK_3);

        count_test++;
    }
}

static HAL_StatusTypeDef ADC_TestStart(void)
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
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/


UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_tx;
DMA_HandleTypeDef hdma_usart1_rx;

/* USER CODE BEGIN PV */

int fputc(int ch, FILE * f)
{
	HAL_UART_Transmit_DMA(&huart1, (uint8_t*)&ch, 1);
	return ch;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	switch(GPIO_Pin)
	{
		case GPIO_PIN_12:
			HAL_GPIO_TogglePin(LED0_GPIO_Port,LED0_Pin);
			break;
		case GPIO_PIN_13:
			HAL_GPIO_TogglePin(LED0_GPIO_Port,LED1_Pin);
			break;
		case GPIO_PIN_14:
			break;
	}
		
}
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void Debug_print(const char* buf, ...)
{
  const char *p = buf;
  char str[255] = {0};
  va_list v;
  va_start(v, buf);
  vsprintf(str, buf, v);
  HAL_UART_Transmit(&huart1, (const uint8_t *)str, strlen(str), 0xff);
  va_end(v);
}
uint8_t _dbg_Buff[150];
void printf_DMA(const char *format,...)
  {
    uint32_t length;
    va_list args;
    uint8_t  temp=0;

    va_start(args, format);
    length = vsnprintf((char*)_dbg_Buff, sizeof(_dbg_Buff)+1, (char*)format, args);
    va_end(args);

    HAL_UART_Transmit_DMA(&huart1,_dbg_Buff,length);
    while(!__HAL_UART_GET_FLAG(&huart1,UART_FLAG_TC));
  }

float test = 0;

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
	HAL_UART_Receive_DMA(&huart1, (uint8_t*)receive_buff, 255); 
	Vofa_Init(&Vofa_packet);
	
	uint8_t _dbg_Buff[150];
	
	adc_test_status = ADC_TestStart();

	if (adc_test_status != HAL_OK)
	{
			adc_test_error = HAL_ADC_GetError(&hadc1);
			Error_Handler();
  }
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		HAL_Delay(1);
		test = test + 0.01;
		if(test > 10.0)
			test = 0;
		Vofa_packet.fdata[0] = test;
		Vofa_SendFloat(&Vofa_packet);
		
		//HAL_Delay(1000);
		//printf_DMA("TEST \r\n");
		//HAL_UART_Transmit_DMA(&huart1, (const uint8_t *)"TEST", 5);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_InjectionConfTypeDef sConfigInjected = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.GainCompensation = 0;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Injected Channel
  */
  sConfigInjected.InjectedChannel = ADC_CHANNEL_7;
  sConfigInjected.InjectedRank = ADC_INJECTED_RANK_1;
  sConfigInjected.InjectedSamplingTime = ADC_SAMPLETIME_12CYCLES_5;
  sConfigInjected.InjectedSingleDiff = ADC_SINGLE_ENDED;
  sConfigInjected.InjectedOffsetNumber = ADC_OFFSET_NONE;
  sConfigInjected.InjectedOffset = 0;
  sConfigInjected.InjectedNbrOfConversion = 3;
  sConfigInjected.InjectedDiscontinuousConvMode = DISABLE;
  sConfigInjected.AutoInjectedConv = DISABLE;
  sConfigInjected.QueueInjectedContext = DISABLE;
  sConfigInjected.ExternalTrigInjecConv = ADC_EXTERNALTRIGINJEC_T1_CC4;
  sConfigInjected.ExternalTrigInjecConvEdge = ADC_EXTERNALTRIGINJECCONV_EDGE_RISING;
  sConfigInjected.InjecOversamplingMode = DISABLE;
  if (HAL_ADCEx_InjectedConfigChannel(&hadc1, &sConfigInjected) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Injected Channel
  */
  sConfigInjected.InjectedChannel = ADC_CHANNEL_8;
  sConfigInjected.InjectedRank = ADC_INJECTED_RANK_2;
  if (HAL_ADCEx_InjectedConfigChannel(&hadc1, &sConfigInjected) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Injected Channel
  */
  sConfigInjected.InjectedChannel = ADC_CHANNEL_9;
  sConfigInjected.InjectedRank = ADC_INJECTED_RANK_3;
  if (HAL_ADCEx_InjectedConfigChannel(&hadc1, &sConfigInjected) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_CENTERALIGNED1;
  htim1.Init.Period = 5000;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_OC_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_TIMING;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_OC_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.BreakAFMode = TIM_BREAK_AFMODE_INPUT;
  sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
  sBreakDeadTimeConfig.Break2Filter = 0;
  sBreakDeadTimeConfig.Break2AFMode = TIM_BREAK_AFMODE_INPUT;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, LED0_Pin|LED1_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : KEY0_Pin KEY1_Pin KEY2_Pin */
  GPIO_InitStruct.Pin = KEY0_Pin|KEY1_Pin|KEY2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : LED0_Pin LED1_Pin */
  GPIO_InitStruct.Pin = LED0_Pin|LED1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
