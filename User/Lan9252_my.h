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
/* PRAM address and byte length must be 4-byte aligned; main/task only.
 * SelfTest uses 0x1000..0x10FF; call before the stack uses process RAM.
 */
typedef struct
{
    uint32_t stage;
    uint32_t failed_stage;
    uint32_t last_command;
    uint32_t offset;
    uint32_t expected;
    uint32_t actual;
    uint32_t errors;
    uint32_t restore_verified;
    HAL_StatusTypeDef status;
    HAL_StatusTypeDef restore_status;
} LAN9252_PRAM_Debug_t;

extern LAN9252_PRAM_Debug_t lan9252_pram_debug;
HAL_StatusTypeDef LAN9252_PRAM_Read(uint16_t address, uint8_t *buffer, uint16_t length);
HAL_StatusTypeDef LAN9252_PRAM_Write(uint16_t address, const uint8_t *buffer, uint16_t length);
HAL_StatusTypeDef LAN9252_PRAM_SelfTest(void);

#ifdef __cplusplus

}

#endif



#endif

