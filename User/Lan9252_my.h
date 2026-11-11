#ifndef LAN9252MY_H

#define LAN9252MY_H



#include "stm32g4xx_hal.h"

#include "stm32g4xx_hal_spi.h"

#ifdef __cplusplus

extern "C" {

#endif



//extern SPI_HandleTypeDef hspi1;

HAL_StatusTypeDef LAN9252_WriteReg(uint16_t reg_addr, uint32_t data);
/* CSR access requires initialized/ready SPI; main/task context only. */
HAL_StatusTypeDef LAN9252_ReadReg(uint16_t reg_addr,uint32_t *p_data);

HAL_StatusTypeDef LAN9252_CSR_Read(uint16_t address, uint8_t length, uint32_t *value);

HAL_StatusTypeDef LAN9252_CSR_Write(uint16_t address, uint8_t length, uint32_t value);

HAL_StatusTypeDef LAN9252_WaitReady(uint32_t timeout_ms);
#ifdef __cplusplus

}

#endif



#endif

