#ifndef LAN9252MY_H
#define LAN9252MY_H

#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_spi.h"
#ifdef __cplusplus
extern "C" {
#endif

//extern SPI_HandleTypeDef hspi1;
HAL_StatusTypeDef LAN9252_ReadReg(uint16_t reg_addr,uint32_t *p_data);
#ifdef __cplusplus
}
#endif

#endif
