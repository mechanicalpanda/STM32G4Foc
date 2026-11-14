#ifndef __BSP_GPIO_H__
#define __BSP_GPIO_H__

/* 包含头文件 ----------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* 类型定义 --------------------------------------------------------------*/

/* 宏定义 --------------------------------------------------------------------*/
#define RST_ESC     HAL_GPIO_WritePin(GPIOE, GPIO_PIN_10, GPIO_PIN_RESET);
#define RST_ESCEND  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_10, GPIO_PIN_SET);


/* 扩展变量 ------------------------------------------------------------------*/
/* 函数声明 ------------------------------------------------------------------*/
void EXTI0_Configuration(void);
void EXTI1_Configuration(void);
void EXTI3_Configuration(void);
void RST_Configuration(void);
void EXTI6_Configuration(void);
void EXTI7_Configuration(void);
void EXTI8_Configuration(void);

#endif  // __BSP_GPIO_H__


