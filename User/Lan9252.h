#ifndef LAN9252_H
#define LAN9252_H
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_spi.h"
#ifdef __cplusplus
extern "C" {
#endif
#define LAN9252_BYTE_TEST_ADDR 0x0064U
#define LAN9252_HW_CFG_ADDR    0x0074U
#define LAN9252_BYTE_TEST_VALUE 0x87654321UL
#define LAN9252_READY_MASK     (1UL << 27)
extern SPI_HandleTypeDef hspi1;
/* stage: 1=SPI init, 2=reset, 3=BYTE_TEST, 4=HW_CFG, 5=ready.
 * On error stage remains at the failing operation. */
typedef struct {
    uint32_t stage;
    uint32_t byte_test;
    uint32_t hw_cfg;
    uint32_t spi_error;
    HAL_StatusTypeDef status;
} LAN9252_Debug_t;
extern volatile LAN9252_Debug_t lan9252_debug;
/* Call after HAL_Init/SystemClock_Config. Configures PB3/4/5 and PD8/9/10, PC6/7.
 * PD9 must connect to an independent, active-low reset input (no output conflict).
 * Blocking, non-reentrant API: use from main task, not the FOC ISR.
 */
HAL_StatusTypeDef LAN9252_Init(void);
HAL_StatusTypeDef LAN9252_Read32(uint16_t address, uint32_t *value);
HAL_StatusTypeDef LAN9252_Write32(uint16_t address, uint32_t value);
HAL_StatusTypeDef LAN9252_WaitReady(uint32_t timeout_ms);
/* Repeated BYTE_TEST check; returns HAL_ERROR on a mismatch. */
HAL_StatusTypeDef LAN9252_SelfTest(uint32_t repetitions, uint32_t *errors);
#ifdef __cplusplus
}
#endif
#endif
