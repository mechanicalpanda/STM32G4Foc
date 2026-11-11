#include "Lan9252_my.h"
#include "main.h"

extern SPI_HandleTypeDef hspi1;

static void LAN9252_CS_Enable(void)
{
    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);
	
}
 
/**
 * @brief  LAN9252 片选失能
 */
static void LAN9252_CS_Disable(void)
{
   HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_SET);
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
        HAL_SPI_TransmitReceive(&hspi1,
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
