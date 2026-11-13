
/* 包含头文件 ----------------------------------------------------------------*/
#include "bsp_GeneralTIM.h"
#include "ecat_def.h"

/* 私有类型定义 --------------------------------------------------------------*/

/* 私有宏定义 ----------------------------------------------------------------*/

/* 私有变量 ------------------------------------------------------------------*/
TIM_HandleTypeDef htimx;

/* 扩展变量 ------------------------------------------------------------------*/

/* 私有函数原形 --------------------------------------------------------------*/

/* 函数体 --------------------------------------------------------------------*/
/**
  * 函数功能: 定时器初始化
  * 输入参数: 无
  * 返 回 值: 无
  * 说    明: 无
  */
void TIM_Configuration(uint8_t period)		//100us
{
  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htimx.Instance = GENERAL_TIMx;
  htimx.Init.Prescaler = 41;
  htimx.Init.CounterMode = TIM_COUNTERMODE_UP;
  htimx.Init.Period = period*200;
  htimx.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htimx.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;	
  HAL_TIM_Base_Init(&htimx);

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;//选择使用内部时钟
  HAL_TIM_ConfigClockSource(&htimx, &sClockSourceConfig);

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  HAL_TIMEx_MasterConfigSynchronization(&htimx, &sMasterConfig);  
	#if ECAT_TIMER_INT
  HAL_TIM_Base_Start_IT(&htimx);
	#endif	
	
}

/**
  * 函数功能: 定时器硬件初始化配置
  * 输入参数: htim_base：基本定时器句柄类型指针
  * 返 回 值: 无
  * 说    明: 该函数被HAL库内部调用
  */
//void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim_base)
//{

//  if(htim_base->Instance==GENERAL_TIMx)
//  {
//    /* 定时器外设时钟使能 */
//    GENERAL_TIM_RCC_CLK_ENABLE();

//    /* 外设中断配置 */
//    HAL_NVIC_SetPriority(GENERAL_TIM_IRQ, 0, 1);
//    //HAL_NVIC_EnableIRQ(GENERAL_TIM_IRQ);
//  }
//}

/**
  * 函数功能: 定时器硬件反初始化配置
  * 输入参数: htim_base：基本定时器句柄类型指针
  * 返 回 值: 无
  * 说    明: 该函数被HAL库内部调用
  */
//void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* htim_base)
//{

//  if(htim_base->Instance==GENERAL_TIMx)
//  {
//    /* 定时器外设时钟禁用 */
//    GENERAL_TIM_RCC_CLK_DISABLE();

//    /* 关闭外设中断 */
//    HAL_NVIC_DisableIRQ(GENERAL_TIM_IRQ);
//  }
//} 


