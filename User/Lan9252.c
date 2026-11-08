#include "Lan9252.h"

#ifndef HAL_SPI_MODULE_ENABLED
#error "Enable HAL_SPI_MODULE_ENABLED in stm32g4xx_hal_conf.h and add stm32g4xx_hal_spi.c to the project"
#endif

SPI_HandleTypeDef hspi1;
volatile LAN9252_Debug_t lan9252_debug;
#define CS_LOW()  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_RESET)
#define CS_HIGH() HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_SET)
#define LAN_SPI_TIMEOUT_MS 10U

static void LAN9252_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    /* Establish inactive outputs before enabling output drivers. */
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9 | GPIO_PIN_10, GPIO_PIN_SET);
    gpio.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOD, &gpio);
    gpio.Pin = GPIO_PIN_8;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL; /* IRQ electrical mode/pull-up must match the board. */
    HAL_GPIO_Init(GPIOD, &gpio);
    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOC, &gpio); /* SYNC0/1: no EXTI enabled at this stage. */
    gpio.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOB, &gpio);
    gpio.Pin = GPIO_PIN_4;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &gpio);
}

/* Reference SPIReadDWord_test flow: FAST_READ, address MSB/LSB,
 * one dummy byte (0x01), then four little-endian data bytes.
 * CS stays asserted across every byte. Full-duplex byte transfers drain RX. */
HAL_StatusTypeDef LAN9252_Read32(uint16_t address, uint32_t *value)
{
    uint8_t tx[8] = {0x0B, (uint8_t)(address >> 8), (uint8_t)address,
                     0x01, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t rx[8] = {0};
    HAL_StatusTypeDef status = HAL_OK;
    if (value == NULL) return HAL_ERROR;
    CS_LOW();
    for (uint32_t i = 0U; i < 8U; ++i) {
        status = HAL_SPI_TransmitReceive(&hspi1, &tx[i], &rx[i], 1U,
                                         LAN_SPI_TIMEOUT_MS);
        if (status != HAL_OK) break;
    }
    CS_HIGH();
    if (status == HAL_OK) {
        *value = (uint32_t)rx[4] | ((uint32_t)rx[5] << 8)
               | ((uint32_t)rx[6] << 16) | ((uint32_t)rx[7] << 24);
    }
    return status;
}

HAL_StatusTypeDef LAN9252_Write32(uint16_t address, uint32_t value)
{
    uint8_t tx[7] = {0x02, (uint8_t)(address >> 8), (uint8_t)address,
        (uint8_t)value, (uint8_t)(value >> 8), (uint8_t)(value >> 16), (uint8_t)(value >> 24)};
    HAL_StatusTypeDef status;
    CS_LOW();
    status = HAL_SPI_Transmit(&hspi1, tx, 7U, LAN_SPI_TIMEOUT_MS);
    CS_HIGH();
    return status;
}

HAL_StatusTypeDef LAN9252_WaitReady(uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick(), value = 0U;
    lan9252_debug.stage = 3U;
    HAL_StatusTypeDef status;
    for (;;) {
        status = LAN9252_Read32(LAN9252_BYTE_TEST_ADDR, &value);
        lan9252_debug.status = status;
        lan9252_debug.spi_error = HAL_SPI_GetError(&hspi1);
        if (status != HAL_OK) return status;
        lan9252_debug.byte_test = value;
        if (value == LAN9252_BYTE_TEST_VALUE) break;
        if ((HAL_GetTick() - start) >= timeout_ms) return HAL_TIMEOUT;
        HAL_Delay(1U);
    }
    start = HAL_GetTick();
    lan9252_debug.stage = 4U;
    for (;;) {
        status = LAN9252_Read32(LAN9252_HW_CFG_ADDR, &value);
        lan9252_debug.status = status;
        lan9252_debug.spi_error = HAL_SPI_GetError(&hspi1);
        if (status != HAL_OK) return status;
        lan9252_debug.hw_cfg = value;
        if ((value & LAN9252_READY_MASK) != 0U) {
            lan9252_debug.stage = 5U;
            return HAL_OK;
        }
        if ((HAL_GetTick() - start) >= timeout_ms) return HAL_TIMEOUT;
        HAL_Delay(1U);
    }
}

HAL_StatusTypeDef LAN9252_Init(void)
{
    HAL_StatusTypeDef status;
    lan9252_debug.stage = 1U;
    lan9252_debug.byte_test = 0U;
    lan9252_debug.hw_cfg = 0U;
    LAN9252_GPIO_Init();
    __HAL_RCC_SPI1_CLK_ENABLE();
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 7U;
    hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    status = HAL_SPI_Init(&hspi1);
    lan9252_debug.status = status;
    lan9252_debug.spi_error = HAL_SPI_GetError(&hspi1);
    if (status != HAL_OK) return status;
    lan9252_debug.stage = 2U;
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, GPIO_PIN_RESET);
    HAL_Delay(100U);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, GPIO_PIN_SET);
    status = LAN9252_WaitReady(1000U);
    lan9252_debug.status = status;
    return status;
}

HAL_StatusTypeDef LAN9252_SelfTest(uint32_t repetitions, uint32_t *errors)
{
    uint32_t value, count = 0U;
    if (errors == NULL) return HAL_ERROR;
    for (uint32_t i = 0U; i < repetitions; ++i) {
        if (LAN9252_Read32(LAN9252_BYTE_TEST_ADDR, &value) != HAL_OK) ++count;
        else if (value != LAN9252_BYTE_TEST_VALUE) ++count;
    }
    *errors = count;
    return count == 0U ? HAL_OK : HAL_ERROR;
}
