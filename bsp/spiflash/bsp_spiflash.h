
#ifndef __BSP_SPIFLASH_H__
#define __BSP_SPIFLASH_H__

/* 包含头文件 ----------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* 类型定义 ------------------------------------------------------------------*/
/* 宏定义 --------------------------------------------------------------------*/
#define FLASH_SPIx                                 SPI1

#define FLASH_SPIx_RCC_CLK_ENABLE()                __HAL_RCC_SPI1_CLK_ENABLE()

#define GPIO_AFx_SPIx                              GPIO_AF5_SPI1

#define FLASH_SPI_SCK_ClK_ENABLE()                 __HAL_RCC_GPIOA_CLK_ENABLE()
#define FLASH_SPI_SCK_PORT                         GPIOA
#define FLASH_SPI_SCK_PIN                          GPIO_PIN_5

#define FLASH_SPI_MISO_ClK_ENABLE()                __HAL_RCC_GPIOA_CLK_ENABLE()
#define FLASH_SPI_MISO_PORT                        GPIOA
#define FLASH_SPI_MISO_PIN                         GPIO_PIN_6

#define FLASH_SPI_MOSI_ClK_ENABLE()                __HAL_RCC_GPIOA_CLK_ENABLE()
#define FLASH_SPI_MOSI_PORT                        GPIOA
#define FLASH_SPI_MOSI_PIN                         GPIO_PIN_7

#define FLASH_SPI_CS_ClK_ENABLE()                  __HAL_RCC_GPIOA_CLK_ENABLE()
#define FLASH_SPI_CS_PORT                          GPIOA
#define FLASH_SPI_CS_PIN                           GPIO_PIN_8

#define DESELECT_SPI                          HAL_GPIO_WritePin(FLASH_SPI_CS_PORT, FLASH_SPI_CS_PIN, GPIO_PIN_SET)
#define SELECT_SPI                            HAL_GPIO_WritePin(FLASH_SPI_CS_PORT, FLASH_SPI_CS_PIN, GPIO_PIN_RESET)

#define CSLOW()      SELECT_SPI
#define CSHIGH()     DESELECT_SPI

#define SPIWriteByte SPIWrite
#define SPIReadByte() SPIRead()

	// *****************************************************************************
	// *****************************************************************************
	// Section: File Scope or Global Data Types
	// *****************************************************************************
	// *****************************************************************************
	#define CMD_SERIAL_READ 0x03
	#define CMD_FAST_READ 0x0B
	#define CMD_DUAL_OP_READ 0x3B
	#define CMD_DUAL_IO_READ 0xBB
	#define CMD_QUAD_OP_READ 0x6B
	#define CMD_QUAD_IO_READ 0xEB
	#define CMD_SERIAL_WRITE 0x02
	#define CMD_DUAL_DATA_WRITE 0x32
	#define CMD_DUAL_ADDR_DATA_WRITE 0xB2
	#define CMD_QUAD_DATA_WRITE 0x62
	#define CMD_QUAD_ADDR_DARA_WRITE 0xE2

	#define CMD_SERIAL_READ_DUMMY 0
	#define CMD_FAST_READ_DUMMY 1
	#define CMD_DUAL_OP_READ_DUMMY 1
	#define CMD_DUAL_IO_READ_DUMMY 2
	#define CMD_QUAD_OP_READ_DUMMY 1
	#define CMD_QUAD_IO_READ_DUMMY 4
	#define CMD_SERIAL_WRITE_DUMMY 0
	#define CMD_DUAL_DATA_WRITE_DUMMY 0
	#define CMD_DUAL_ADDR_DATA_WRITE_DUMMY 0
	#define CMD_QUAD_DATA_WRITE_DUMMY 0
	#define CMD_QUAD_ADDR_DARA_WRITE_DUMMY 0

	#define ESC_CSR_CMD_REG		0x304
	#define ESC_CSR_DATA_REG	0x300
	#define ESC_WRITE_BYTE 		0x80
	#define ESC_READ_BYTE 		0xC0
	#define ESC_CSR_BUSY		0x80


	/////////////////////////////////////////////////////////////////////////////////
	
	
typedef union
{
	uint32_t Val;
	uint8_t v[4];
	uint16_t w[2];
	struct
	{
		uint8_t LB;
		uint8_t HB;	
		uint8_t UB;
		uint8_t MB;
	}byte;
}UINT32_VAL;


typedef union
{
	uint16_t Val;
	struct
	{
		uint8_t LB;
		uint8_t HB;	
	}byte;
}UINT16_VAL;

/* 扩展变量 ------------------------------------------------------------------*/
extern SPI_HandleTypeDef hspix;

/* 函数声明 ------------------------------------------------------------------*/
void MX_SPIFlash_Init(void);
uint8_t WR_CMD (uint8_t cmd)       ;
void ADC_GPIO_Configuration(void);
void ADC_Configuration(void);
void NVIC_Configuration(void);
void TIM_Configuration(uint8_t period)	;
void EXTI0_Configuration(void);
void EXTI1_Configuration(void);
void EXTI8_Configuration(void);
void SPIReadDRegister(uint8_t *ReadBuffer, uint16_t Address, uint16_t Count);
void SPIWriteRegister( uint8_t *WriteBuffer, uint16_t Address, uint16_t Count);
uint32_t SPIReadDWord (uint16_t Address);
void SPIWriteDWord (uint16_t Address, uint32_t Val);


#endif  /* __BSP_SPIFLASH_H__ */


