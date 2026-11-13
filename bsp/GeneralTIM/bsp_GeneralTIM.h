
#ifndef __GENERAL_TIM_H__
#define __GENERAL_TIM_H__

/* 包含头文件 ----------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* 类型定义 ------------------------------------------------------------------*/

/* 宏定义 --------------------------------------------------------------------*/
/********************通用定时器TIM参数定义，TIM2************/

#define GENERAL_TIMx                     TIM2
#define GENERAL_TIM_RCC_CLK_ENABLE()     __HAL_RCC_TIM2_CLK_ENABLE()
#define GENERAL_TIM_RCC_CLK_DISABLE()    __HAL_RCC_TIM2_CLK_DISABLE()
#define GENERAL_TIM_IRQ                  TIM2_IRQn
#define GENERAL_TIM_INT_FUN              TIM2_IRQHandler


/* 扩展变量 ------------------------------------------------------------------*/
extern TIM_HandleTypeDef htimx;

/* 函数声明 ------------------------------------------------------------------*/
void TIM_Configuration(uint8_t period);	


#endif	/* __GENERAL_TIM_H__ */

