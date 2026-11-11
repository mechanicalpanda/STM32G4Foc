#include "Lan9252.h"



#ifndef HAL_SPI_MODULE_ENABLED

#error "Enable HAL_SPI_MODULE_ENABLED in stm32g4xx_hal_conf.h and add stm32g4xx_hal_spi.c to the project"

#endif



//extern SPI_HandleTypeDef hspi1;

//volatile LAN9252_Debug_t lan9252_debug;

//#define CS_LOW()  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_RESET)

//#define CS_HIGH() HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_SET)

//#define LAN_SPI_TIMEOUT_MS 10U



//static void LAN9252_GPIO_Init(void)

//{

//    GPIO_InitTypeDef gpio = {0};

//    __HAL_RCC_GPIOB_CLK_ENABLE();

//    __HAL_RCC_GPIOC_CLK_ENABLE();

//    __HAL_RCC_GPIOD_CLK_ENABLE();

//    /* Establish inactive outputs before enabling output drivers. */

//    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9 | GPIO_PIN_10, GPIO_PIN_SET);

//    gpio.Pin = GPIO_PIN_9 | GPIO_PIN_10;

//    gpio.Mode = GPIO_MODE_OUTPUT_PP;

//    gpio.Pull = GPIO_NOPULL;

//    gpio.Speed = GPIO_SPEED_FREQ_HIGH;

//    HAL_GPIO_Init(GPIOD, &gpio);

//    gpio.Pin = GPIO_PIN_8;

//    gpio.Mode = GPIO_MODE_INPUT;

//    gpio.Pull = GPIO_NOPULL; /* IRQ electrical mode/pull-up must match the board. */

//    HAL_GPIO_Init(GPIOD, &gpio);

//    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;

//    HAL_GPIO_Init(GPIOC, &gpio); /* SYNC0/1: no EXTI enabled at this stage. */

//    gpio.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;

//    gpio.Mode = GPIO_MODE_AF_PP;

//    gpio.Pull = GPIO_NOPULL;

//    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

//    gpio.Alternate = GPIO_AF5_SPI1;

//    HAL_GPIO_Init(GPIOB, &gpio);

//    gpio.Pin = GPIO_PIN_4;

//    gpio.Pull = GPIO_PULLUP;

//    HAL_GPIO_Init(GPIOB, &gpio);

//}



///* Reference SPIReadDWord_test flow: FAST_READ, address MSB/LSB,

// * one dummy byte (0x01), then four little-endian data bytes.

// * CS stays asserted across every byte. Full-duplex byte transfers drain RX. */

//HAL_StatusTypeDef LAN9252_Read32(uint16_t address, uint32_t *value)

//{

//    uint8_t tx[8] = {0x0B, (uint8_t)(address >> 8), (uint8_t)address,

//                     0x01, 0xFF, 0xFF, 0xFF, 0xFF};

//    uint8_t rx[8] = {0};

//    HAL_StatusTypeDef status = HAL_OK;

//    if (value == NULL) return HAL_ERROR;

//    CS_LOW();

//    for (uint32_t i = 0U; i < 8U; ++i) {

//        status = HAL_SPI_TransmitReceive(&hspi1, &tx[i], &rx[i], 1U,

//                                         LAN_SPI_TIMEOUT_MS);

//        if (status != HAL_OK) break;

//    }

//    CS_HIGH();

//    if (status == HAL_OK) {

//        *value = (uint32_t)rx[4] | ((uint32_t)rx[5] << 8)

//               | ((uint32_t)rx[6] << 16) | ((uint32_t)rx[7] << 24);

//    }

//    return status;

//}



//HAL_StatusTypeDef LAN9252_Write32(uint16_t address, uint32_t value)

//{

//    uint8_t tx[7] = {0x02, (uint8_t)(address >> 8), (uint8_t)address,

//        (uint8_t)value, (uint8_t)(value >> 8), (uint8_t)(value >> 16), (uint8_t)(value >> 24)};

//    HAL_StatusTypeDef status;

//    CS_LOW();

//    status = HAL_SPI_Transmit(&hspi1, tx, 7U, LAN_SPI_TIMEOUT_MS);

//    CS_HIGH();

//    return status;

//}



//HAL_StatusTypeDef LAN9252_WaitReady(uint32_t timeout_ms)

//{

//    uint32_t start = HAL_GetTick(), value = 0U;

//    lan9252_debug.stage = 3U;

//    HAL_StatusTypeDef status;

//    for (;;) {

//        status = LAN9252_Read32(LAN9252_BYTE_TEST_ADDR, &value);

//        lan9252_debug.status = status;

//        lan9252_debug.spi_error = HAL_SPI_GetError(&hspi1);

//        if (status != HAL_OK) return status;

//        lan9252_debug.byte_test = value;

//        if (value == LAN9252_BYTE_TEST_VALUE) break;

//        if ((HAL_GetTick() - start) >= timeout_ms) return HAL_TIMEOUT;

//        HAL_Delay(1U);

//    }

//    start = HAL_GetTick();

//    lan9252_debug.stage = 4U;

//    for (;;) {

//        status = LAN9252_Read32(LAN9252_HW_CFG_ADDR, &value);

//        lan9252_debug.status = status;

//        lan9252_debug.spi_error = HAL_SPI_GetError(&hspi1);

//        if (status != HAL_OK) return status;

//        lan9252_debug.hw_cfg = value;

//        if ((value & LAN9252_READY_MASK) != 0U) {

//            lan9252_debug.stage = 5U;

//            return HAL_OK;

//        }

//        if ((HAL_GetTick() - start) >= timeout_ms) return HAL_TIMEOUT;

//        HAL_Delay(1U);

//    }

//}



//HAL_StatusTypeDef LAN9252_Init(void)

//{

//    HAL_StatusTypeDef status;

//    lan9252_debug.stage = 1U;

//    lan9252_debug.byte_test = 0U;

//    lan9252_debug.hw_cfg = 0U;

//    LAN9252_GPIO_Init();

//    __HAL_RCC_SPI1_CLK_ENABLE();

//    hspi1.Instance = SPI1;

//    hspi1.Init.Mode = SPI_MODE_MASTER;

//    hspi1.Init.Direction = SPI_DIRECTION_2LINES;

//    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;

//    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;

//    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;

//    hspi1.Init.NSS = SPI_NSS_SOFT;

//    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;

//    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;

//    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;

//    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;

//    hspi1.Init.CRCPolynomial = 7U;

//    hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;

//    hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;

//    status = HAL_SPI_Init(&hspi1);

//    lan9252_debug.status = status;

//    lan9252_debug.spi_error = HAL_SPI_GetError(&hspi1);

//    if (status != HAL_OK) return status;

//    lan9252_debug.stage = 2U;

//    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, GPIO_PIN_RESET);

//    HAL_Delay(100U);

//    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, GPIO_PIN_SET);

//    status = LAN9252_WaitReady(1000U);

//    lan9252_debug.status = status;

//    return status;

//}



//HAL_StatusTypeDef LAN9252_SelfTest(uint32_t repetitions, uint32_t *errors)

//{

//    uint32_t value, count = 0U;

//    if (errors == NULL) return HAL_ERROR;

//    for (uint32_t i = 0U; i < repetitions; ++i) {

//        if (LAN9252_Read32(LAN9252_BYTE_TEST_ADDR, &value) != HAL_OK) ++count;

//        else if (value != LAN9252_BYTE_TEST_VALUE) ++count;

//    }

//    *errors = count;

//    return count == 0U ? HAL_OK : HAL_ERROR;

//}





//#define LAN_CSR_DATA 0x0300U

//#define LAN_CSR_CMD  0x0304U

//#define LAN_CSR_BUSY (1UL << 31)

//#define LAN_CSR_READ (1UL << 30)

//#define LAN_CSR_TIMEOUT_MS 20U

//volatile LAN9252_CSR_Debug_t lan9252_csr_debug;



//static HAL_StatusTypeDef LAN9252_CSR_WaitIdle(void)

//{

//    uint32_t command, start = HAL_GetTick();

//    HAL_StatusTypeDef status;

//    do {

//        status = LAN9252_Read32(LAN_CSR_CMD, &command);

//        if (status != HAL_OK) return status;

//        lan9252_csr_debug.last_command = command;

//        if ((command & LAN_CSR_BUSY) == 0U) return HAL_OK;

//    } while ((HAL_GetTick() - start) < LAN_CSR_TIMEOUT_MS);

//    return HAL_TIMEOUT;

//}



///* CSR read is implemented in Lan9252_my.c. */



//HAL_StatusTypeDef LAN9252_CSR_SelfTest(void)

//{

//    uint32_t value = 0;

//    HAL_StatusTypeDef status;

//    lan9252_csr_debug.stage = 1U;

//    lan9252_csr_debug.identity = 0U;

//    lan9252_csr_debug.chip_id = 0U;

//    lan9252_csr_debug.al_status = 0U;

//    lan9252_csr_debug.completed = 0U;

//    lan9252_csr_debug.read_errors = 0U;

//    lan9252_csr_debug.mismatch_count = 0U;

//    status = LAN9252_CSR_Read(0x0000U, 4U, &value);

//    if (status != HAL_OK) goto failed;

//    lan9252_csr_debug.identity = value;

//    lan9252_csr_debug.stage = 2U;

//    /* LAN9252 product ID is at ESC 0x0E02 (16 bits). */

//    status = LAN9252_CSR_Read(0x0E00U, 4U, &value);

//    if (status != HAL_OK) goto failed;

//    lan9252_csr_debug.chip_id = value;

//    if ((value >> 16) != 0x9252U) { status = HAL_ERROR; goto failed; }

//    lan9252_csr_debug.stage = 3U;

//    for (uint32_t i = 0U; i < 1000U; ++i) {

//        status = LAN9252_CSR_Read(0x0000U, 4U, &value);

//        if (status != HAL_OK) goto failed;

//        ++lan9252_csr_debug.completed;

//        if (value != lan9252_csr_debug.identity) ++lan9252_csr_debug.mismatch_count;

//    }

//    if (lan9252_csr_debug.mismatch_count != 0U) { status = HAL_ERROR; goto failed; }

//    lan9252_csr_debug.stage = 4U;

//    status = LAN9252_CSR_Read(0x0130U, 4U, &value);

//    if (status != HAL_OK) goto failed;

//    lan9252_csr_debug.al_status = value & 0xFFFFU;

//    lan9252_csr_debug.stage = 5U;

//    lan9252_csr_debug.status = HAL_OK;

//    return HAL_OK;

//failed:

//    if (status == HAL_TIMEOUT || status == HAL_BUSY ||

//        (status == HAL_ERROR && lan9252_csr_debug.stage != 2U &&

//         lan9252_csr_debug.mismatch_count == 0U)) ++lan9252_csr_debug.read_errors;

//    lan9252_csr_debug.status = status;

//    return status;

//}





//volatile LAN9252_CSR_WriteDebug_t lan9252_csr_write_debug;

///* CSR write is implemented in Lan9252_my.c. */



//HAL_StatusTypeDef LAN9252_CSR_WriteSelfTest(void)

//{

// const uint16_t address = 0x0204U; /* AL Event Mask: ordinary PDI R/W. */

// const uint32_t test_mask = 1UL; /* AL Control event mapping only. */

// uint32_t original = 0U, value = 0U;

// uint32_t irq_enabled = EXTI->IMR1 & GPIO_PIN_8;

// HAL_StatusTypeDef status, restore;

// lan9252_csr_write_debug.stage = 1U;

// lan9252_csr_write_debug.failed_stage = 0U;

// lan9252_csr_write_debug.address = address;

// lan9252_csr_write_debug.original = 0U;

// lan9252_csr_write_debug.expected = 0U;

// lan9252_csr_write_debug.readback = 0U;

// lan9252_csr_write_debug.restored_value = 0U;

// lan9252_csr_write_debug.mismatch_count = 0U;

// lan9252_csr_write_debug.restore_verified = 0U;

// lan9252_csr_write_debug.test_status = HAL_BUSY;

// lan9252_csr_write_debug.restore_status = HAL_BUSY; /* not attempted */

// CLEAR_BIT(EXTI->IMR1, GPIO_PIN_8);

// status = LAN9252_CSR_Read(address, 4U, &original);

// if (status != HAL_OK) goto no_write;

// lan9252_csr_write_debug.original = original;

// for (uint32_t i = 0U; i < 2U; ++i) {

//     uint32_t expected = (i == 0U) ? (original ^ test_mask) : original;

//     lan9252_csr_write_debug.stage = 2U + i;

//     lan9252_csr_write_debug.expected = expected;

//     status = LAN9252_CSR_Write(address, 4U, expected);

//     if (status != HAL_OK) goto restore_original;

//     status = LAN9252_CSR_Read(address, 4U, &value);

//     if (status != HAL_OK) goto restore_original;

//     lan9252_csr_write_debug.readback = value;

//     if (value != expected) {

//         ++lan9252_csr_write_debug.mismatch_count;

//         status = HAL_ERROR;

//         goto restore_original;

//     }

// }

//restore_original:

// lan9252_csr_write_debug.test_status = status;

// if (status != HAL_OK) lan9252_csr_write_debug.failed_stage = lan9252_csr_write_debug.stage;

// lan9252_csr_write_debug.stage = 4U;

// /* Even a timed-out write could have completed: always attempt restoration. */

// restore = LAN9252_CSR_Write(address, 4U, original);

// if (restore == HAL_OK) {

//     restore = LAN9252_CSR_Read(address, 4U, &value);

//     if (restore == HAL_OK) {

//         lan9252_csr_write_debug.restored_value = value;

//         if (value != original) restore = HAL_ERROR;

//         else lan9252_csr_write_debug.restore_verified = 1U;

//     }

// }

// lan9252_csr_write_debug.restore_status = restore;

// if (restore != HAL_OK) {

//     if (lan9252_csr_write_debug.failed_stage == 0U) lan9252_csr_write_debug.failed_stage = 4U;

//     /* Leave EXTI8 masked if the IRQ mask could not be restored. */

//     return restore;

// }

// if (status == HAL_OK) lan9252_csr_write_debug.stage = 5U;

// if (irq_enabled != 0U) SET_BIT(EXTI->IMR1, GPIO_PIN_8);

// return status;

//no_write:

// lan9252_csr_write_debug.failed_stage = 1U;

// lan9252_csr_write_debug.test_status = status;

// if (irq_enabled != 0U) SET_BIT(EXTI->IMR1, GPIO_PIN_8);

// return status;

//}





//#define PRAM_BUSY (1UL << 31)

//#define PRAM_ABORT (1UL << 30)

//#define PRAM_TIMEOUT_MS 100U

//volatile LAN9252_PRAM_Debug_t lan9252_pram_debug;



//static HAL_StatusTypeDef PRAM_Wait(uint16_t cmd, uint32_t start, uint8_t idle)

//{

// uint32_t v;

// HAL_StatusTypeDef st;

// do {

//  st = LAN9252_Read32(cmd, &v);

//  if (st != HAL_OK) return st;

//  lan9252_pram_debug.last_command = v;

//  if (idle) { if (!(v & PRAM_BUSY)) return HAL_OK; }

//  else if ((v & 1U) && ((v >> 8) & 0x1FU)) return HAL_OK;

// } while ((HAL_GetTick() - start) < PRAM_TIMEOUT_MS);

// return HAL_TIMEOUT;

//}



//static HAL_StatusTypeDef PRAM_Transfer(uint16_t address, uint8_t *rd,

// const uint8_t *wr, uint16_t length)

//{

// uint16_t cmd = wr ? 0x0314U : 0x030CU;

// uint16_t addr_len = wr ? 0x0310U : 0x0308U;

// uint16_t fifo = wr ? 0x0020U : 0x0000U;

// uint32_t start = HAL_GetTick(), word, offset = address & 3U, done = 0U;

// HAL_StatusTypeDef st;

// if (address < 0x1000U || (uint32_t)address + length > 0x2000U ||

//     (length && !rd && !wr)) return HAL_ERROR;

// if (length == 0U) return HAL_OK;

// st = LAN9252_Write32(cmd, PRAM_ABORT);

// if (st != HAL_OK) return st;

// st = PRAM_Wait(cmd, start, 1U);

// if (st != HAL_OK) goto fail;

// st = LAN9252_Write32(addr_len, ((uint32_t)length << 16) | address);

// if (st != HAL_OK) goto fail;

// st = LAN9252_Write32(cmd, PRAM_BUSY);

// if (st != HAL_OK) goto fail;

// while (done < length) {

//  uint32_t n = 4U - offset;

//  if (n > length - done) n = length - done;

//  /* Conservative implementation: recheck availability before every DWORD.

//     FIFO addresses are aliased; each access pops/pushes one DWORD. */

//  st = PRAM_Wait(cmd, start, 0U);

//  if (st != HAL_OK) goto fail;

//  if (wr) {

//   word = 0U;

//   for (uint32_t j = 0U; j < n; ++j)

//    word |= (uint32_t)wr[done + j] << (8U * (offset + j));

//   st = LAN9252_Write32(fifo, word);

//  } else {

//   st = LAN9252_Read32(fifo, &word);

//   if (st == HAL_OK)

//    for (uint32_t j = 0U; j < n; ++j)

//     rd[done + j] = (uint8_t)(word >> (8U * (offset + j)));

//  }

//  if (st != HAL_OK) goto fail;

//  done += n; offset = 0U;

// }

// st = PRAM_Wait(cmd, start, 1U);

// if (st == HAL_OK) return st;

//fail:

// /* Best effort abort; preserve original error. */

// (void)LAN9252_Write32(cmd, PRAM_ABORT);

// return st;

//}



//HAL_StatusTypeDef LAN9252_PRAM_Read(uint16_t address, uint8_t *buffer, uint16_t length)

//{

// if (length && buffer == NULL) return HAL_ERROR;

// return PRAM_Transfer(address, buffer, NULL, length);

//}

//HAL_StatusTypeDef LAN9252_PRAM_Write(uint16_t address, const uint8_t *buffer, uint16_t length)

//{

// if (length && buffer == NULL) return HAL_ERROR;

// return PRAM_Transfer(address, NULL, buffer, length);

//}



///* Reserve 0x1800..0x1907 for this startup test. Save and restore all 264 bytes,

// * including guards. Do not run once SyncManagers/master use process RAM. */

//#define PRAM_TEST_BASE 0x1800U

//#define PRAM_TEST_SIZE 264U

//static uint8_t pram_saved[PRAM_TEST_SIZE];

//static uint8_t pram_expected[PRAM_TEST_SIZE];

//static uint8_t pram_actual[PRAM_TEST_SIZE];



//HAL_StatusTypeDef LAN9252_PRAM_SelfTest(void)

//{

// static const uint16_t offsets[4] = {4U, 4U, 5U, 7U};

// static const uint16_t lengths[4] = {16U, 256U, 17U, 253U};

// HAL_StatusTypeDef st, restore;

// uint32_t irq = EXTI->IMR1 & GPIO_PIN_8;

// lan9252_pram_debug.stage = 1U;

// lan9252_pram_debug.failed_stage = 0U;

// lan9252_pram_debug.test_case = 0U;

// lan9252_pram_debug.address = PRAM_TEST_BASE;

// lan9252_pram_debug.length = PRAM_TEST_SIZE;

// lan9252_pram_debug.completed = 0U;

// lan9252_pram_debug.mismatch_count = 0U;

// lan9252_pram_debug.first_error_offset = 0xFFFFFFFFUL;

// lan9252_pram_debug.expected = 0U;

// lan9252_pram_debug.actual = 0U;

// lan9252_pram_debug.restore_verified = 0U;

// lan9252_pram_debug.test_status = HAL_BUSY;

// lan9252_pram_debug.restore_status = HAL_BUSY;

// CLEAR_BIT(EXTI->IMR1, GPIO_PIN_8);

// st = LAN9252_PRAM_Read(PRAM_TEST_BASE, pram_saved, PRAM_TEST_SIZE);

// if (st != HAL_OK) goto no_write;

// for (uint32_t c = 0U; c < 4U; ++c) {

//  uint16_t off = offsets[c], len = lengths[c];

//  lan9252_pram_debug.test_case = c + 1U;

//  lan9252_pram_debug.address = PRAM_TEST_BASE + off;

//  lan9252_pram_debug.length = len;

//  for (uint32_t pattern = 0U; pattern < 2U; ++pattern) {

//   lan9252_pram_debug.stage = 2U;

//   /* Restore known baseline before every case; compare entire guard region. */

//   st = LAN9252_PRAM_Write(PRAM_TEST_BASE, pram_saved, PRAM_TEST_SIZE);

//   if (st != HAL_OK) goto restore_data;

//   for (uint32_t i = 0U; i < PRAM_TEST_SIZE; ++i) pram_expected[i] = pram_saved[i];

//   for (uint32_t i = 0U; i < len; ++i)

//    pram_expected[off+i] = pattern ? ((i & 1U) ? 0xAAU : 0x55U) : (uint8_t)(i ^ 0xA5U);

//   st = LAN9252_PRAM_Write(PRAM_TEST_BASE + off, &pram_expected[off], len);

//   if (st != HAL_OK) goto restore_data;

//   lan9252_pram_debug.stage = 3U;

//   st = LAN9252_PRAM_Read(PRAM_TEST_BASE + off, pram_actual, len);

//   if (st != HAL_OK) goto restore_data;

//   for (uint32_t i = 0U; i < len; ++i) {

//    if (pram_actual[i] != pram_expected[off+i]) {

//     lan9252_pram_debug.first_error_offset = off+i;

//     lan9252_pram_debug.expected = pram_expected[off+i];

//     lan9252_pram_debug.actual = pram_actual[i];

//     ++lan9252_pram_debug.mismatch_count;

//     st = HAL_ERROR; goto restore_data;

//    }

//   }

//   st = LAN9252_PRAM_Read(PRAM_TEST_BASE, pram_actual, PRAM_TEST_SIZE);

//   if (st != HAL_OK) goto restore_data;

//   for (uint32_t i = 0U; i < PRAM_TEST_SIZE; ++i) {

//    if (pram_actual[i] != pram_expected[i]) {

//     if (lan9252_pram_debug.mismatch_count == 0U) {

//      lan9252_pram_debug.first_error_offset = i;

//      lan9252_pram_debug.expected = pram_expected[i];

//      lan9252_pram_debug.actual = pram_actual[i];

//     }

//     ++lan9252_pram_debug.mismatch_count;

//    }

//   }

//   if (lan9252_pram_debug.mismatch_count) { st = HAL_ERROR; goto restore_data; }

//   ++lan9252_pram_debug.completed;

//  }

// }

//restore_data:

// lan9252_pram_debug.test_status = st;

// if (st != HAL_OK) lan9252_pram_debug.failed_stage = lan9252_pram_debug.stage;

// lan9252_pram_debug.stage = 4U;

// restore = LAN9252_PRAM_Write(PRAM_TEST_BASE, pram_saved, PRAM_TEST_SIZE);

// if (restore == HAL_OK) restore = LAN9252_PRAM_Read(PRAM_TEST_BASE, pram_actual, PRAM_TEST_SIZE);

// if (restore == HAL_OK) {

//  for (uint32_t i = 0U; i < PRAM_TEST_SIZE; ++i)

//   if (pram_actual[i] != pram_saved[i]) { restore = HAL_ERROR; break; }

// }

// lan9252_pram_debug.restore_status = restore;

// if (restore != HAL_OK) {

//  if (!lan9252_pram_debug.failed_stage) lan9252_pram_debug.failed_stage = 4U;

//  return restore; /* Leave IRQ masked if restoration failed. */

// }

// lan9252_pram_debug.restore_verified = 1U;

// if (st == HAL_OK) lan9252_pram_debug.stage = 5U;

// if (irq) SET_BIT(EXTI->IMR1, GPIO_PIN_8);

// return st;

//no_write:

// lan9252_pram_debug.failed_stage = 1U;

// lan9252_pram_debug.test_status = st;

// if (irq) SET_BIT(EXTI->IMR1, GPIO_PIN_8);

// return st;

//}

