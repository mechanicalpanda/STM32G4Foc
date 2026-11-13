#include "Lan9252_my.h"

#include "main.h"

//extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi3;
static void LAN9252_CS_Enable(void)

{
    //HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);
	    HAL_GPIO_WritePin(SPI3_CS_GPIO_Port, SPI3_CS_Pin, GPIO_PIN_RESET);
}

/**

 * @brief  LAN9252 片选失能

 */

static void LAN9252_CS_Disable(void)

{
   //HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_SET);
	 HAL_GPIO_WritePin(SPI3_CS_GPIO_Port, SPI3_CS_Pin, GPIO_PIN_SET);
}


HAL_StatusTypeDef LAN9252_ReadReg(uint16_t reg_addr, uint32_t *p_data)
{
    if (p_data == NULL)
        return HAL_ERROR;

    uint8_t tx_buf[7] = {
        0x03,                          // 普通读，无dummy
        (uint8_t)(reg_addr >> 8),       // 地址高字节
        (uint8_t)reg_addr,              // 地址低字节
        0xFF, 0xFF, 0xFF, 0xFF
    };

    uint8_t rx_buf[7] = {0};
    LAN9252_CS_Enable();                // 必须拉低CS
    HAL_StatusTypeDef ret =
        HAL_SPI_TransmitReceive(&hspi3,
                                tx_buf,
                                rx_buf,
                                sizeof(tx_buf),
                                100);

    LAN9252_CS_Disable();               // 拉高CS

    if (ret == HAL_OK)
    {
        *p_data =  (uint32_t)rx_buf[3]
                 | ((uint32_t)rx_buf[4] << 8)
                 | ((uint32_t)rx_buf[5] << 16)
                 | ((uint32_t)rx_buf[6] << 24);
    }
    return ret;
}

HAL_StatusTypeDef LAN9252_WriteReg(uint16_t reg_addr, uint32_t data)
{
    uint8_t tx[7] = {0x02, (uint8_t)(reg_addr >> 8), (uint8_t)reg_addr,(uint8_t)data, (uint8_t)(data >> 8), (uint8_t)(data >> 16), (uint8_t)(data >> 24)};
    HAL_StatusTypeDef ret;
			
    LAN9252_CS_Enable();
    ret = HAL_SPI_Transmit(&hspi3, tx, sizeof(tx), 100U);
    LAN9252_CS_Disable();
    return ret;
}

#define CSR_CMD_REG     0x0304U
#define CSR_BUSY        (1UL << 31)
#define CSR_TIMEOUT_MS 20U

static HAL_StatusTypeDef CSR_WaitIdle(void)
{
    uint32_t cmd;
    uint32_t start = HAL_GetTick();

    do
    {
        HAL_StatusTypeDef ret = LAN9252_ReadReg(CSR_CMD_REG, &cmd);

        if (ret != HAL_OK)
            return ret;

        /* bit31为0，等待结束 */
        if ((cmd & CSR_BUSY) == 0U)
            return HAL_OK;

    } while ((HAL_GetTick() - start) < CSR_TIMEOUT_MS);

    return HAL_TIMEOUT;
}

//等待BUSY清零
//→ 写CSR_CMD：0xC0000000 | length<<16 | address
//→ 等待BUSY清零
//→ 读取CSR_DATA

HAL_StatusTypeDef LAN9252_CSR_Read(uint16_t address, uint8_t length, uint32_t *value)
{
	HAL_StatusTypeDef ret;
	  //等待BUSY清零
	  ret = CSR_WaitIdle();
	  if (ret != HAL_OK)
    return ret;
		
//		写CSR_CMD：0xC0000000 | length<<16 | address
//     bit31	BUSY，写 1 启动操作
//     bit30	READ，1 表示读取
//     bit23～16	读取字节数
//     bit15～0	ESC 内部寄存器地址
		
		uint32_t command = 0xC0000000UL | ((uint32_t)length << 16) | (uint32_t)address;
		
		ret = LAN9252_WriteReg(0x0304, command);
		if (ret != HAL_OK)
      return ret;
		
		//等待BUSY清零
		 ret = CSR_WaitIdle();
	  if (ret != HAL_OK)
    return ret;
		 
		//→ 读取CSR_DATA
		 uint32_t data = 0;

     ret = LAN9252_ReadReg(0x0300U, &data);
			if (ret != HAL_OK)
			return ret;
			
		 *value = 	data;
			
			return HAL_OK;
}

HAL_StatusTypeDef LAN9252_CSR_Write(uint16_t address, uint8_t length, uint32_t value)
{
	HAL_StatusTypeDef ret;
	  //等待BUSY清零
	  ret = CSR_WaitIdle();
	  if (ret != HAL_OK)
    return ret;
		
//		写CSR_CMD：0xC0000000 | length<<16 | address
//     bit31	BUSY，写 1 启动操作
//     bit30	READ，1 表示读取
//     bit23～16	读取字节数
//     bit15～0	ESC 内部寄存器地址
		
	
		
		ret = LAN9252_WriteReg(0x0300, value);
		if (ret != HAL_OK)
      return ret;
		
		uint32_t command = 0x80000000UL | ((uint32_t)length << 16) | (uint32_t)address;
		ret = LAN9252_WriteReg(0x0304, command);
		if (ret != HAL_OK)
      return ret;
		
		//等待BUSY清零
		 ret = CSR_WaitIdle();
	  if (ret != HAL_OK)
    return ret;
		 
		
			return HAL_OK;
}

HAL_StatusTypeDef LAN9252_WaitReady(uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    uint32_t hw_cfg;

    do
    {
        HAL_StatusTypeDef ret =
            LAN9252_ReadReg(0x0074U, &hw_cfg);

        if (ret != HAL_OK)
            return ret;

        /* 排除全FF无响应，再判断READY */
        if (hw_cfg != 0xFFFFFFFFUL &&
            (hw_cfg & 0x08000000UL) != 0U)
        {
            return HAL_OK;
        }

        HAL_Delay(1U);

    } while ((HAL_GetTick() - start) < timeout_ms);

    return HAL_TIMEOUT;
}
