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


/* PRAM: call from main/task context, before the EtherCAT stack uses this RAM.
 * Address and byte length must both be multiples of four. No concurrent access.
 */
#define PRAM_READ_FIFO       0x0000U
#define PRAM_WRITE_FIFO      0x0020U
#define PRAM_READ_ADDR_LEN   0x0308U
#define PRAM_READ_CMD        0x030CU
#define PRAM_WRITE_ADDR_LEN  0x0310U
#define PRAM_WRITE_CMD       0x0314U
#define PRAM_BUSY            0x80000000UL
#define PRAM_ABORT           0x40000000UL
#define PRAM_TIMEOUT_MS      100U
#define PRAM_TEST_ADDRESS    0x1000U
#define PRAM_TEST_BYTES      256U

 LAN9252_PRAM_Debug_t lan9252_pram_debug;

static HAL_StatusTypeDef PRAM_WaitIdle(uint16_t command_reg)
{
    uint32_t command;
    uint32_t start = HAL_GetTick();
    HAL_StatusTypeDef ret;

    do
    {
        ret = LAN9252_ReadReg(command_reg, &command);
        if (ret != HAL_OK)
            return ret;

        lan9252_pram_debug.last_command = command;
        if ((command & PRAM_BUSY) == 0U)
            return HAL_OK;
    } while ((HAL_GetTick() - start) < PRAM_TIMEOUT_MS);

    return HAL_TIMEOUT;
}

static HAL_StatusTypeDef PRAM_WaitFIFO(uint16_t command_reg,
                                       uint32_t *word_count)
{
    uint32_t command;
    uint32_t start = HAL_GetTick();
    HAL_StatusTypeDef ret;

    do
    {
        ret = LAN9252_ReadReg(command_reg, &command);
        if (ret != HAL_OK)
            return ret;

        lan9252_pram_debug.last_command = command;
        *word_count = (command >> 8) & 0x1FU;
        if ((command & 1U) != 0U && *word_count != 0U)
            return HAL_OK;
    } while ((HAL_GetTick() - start) < PRAM_TIMEOUT_MS);

    return HAL_TIMEOUT;
}

HAL_StatusTypeDef LAN9252_PRAM_Read(uint16_t address, uint8_t *buffer,
                                    uint16_t length)
{
    HAL_StatusTypeDef ret;
    uint32_t count, data;
    uint32_t offset = 0U;

    if (buffer == NULL || length == 0U ||
        (address & 3U) != 0U || (length & 3U) != 0U ||
        address < 0x1000U || (uint32_t)address + length > 0x2000U)
        return HAL_ERROR;

    ret = LAN9252_WriteReg(PRAM_READ_CMD, PRAM_ABORT);
    if (ret != HAL_OK)
        return ret;
    ret = PRAM_WaitIdle(PRAM_READ_CMD);
    if (ret != HAL_OK)
        return ret;

    ret = LAN9252_WriteReg(PRAM_READ_ADDR_LEN,
                           ((uint32_t)length << 16) | address);
    if (ret != HAL_OK)
        return ret;
    ret = LAN9252_WriteReg(PRAM_READ_CMD, PRAM_BUSY);
    if (ret != HAL_OK)
        goto failed;

    while (offset < length)
    {
        ret = PRAM_WaitFIFO(PRAM_READ_CMD, &count);
        if (ret != HAL_OK)
            goto failed;

        while (count != 0U && offset < length)
        {
            ret = LAN9252_ReadReg(PRAM_READ_FIFO, &data);
            if (ret != HAL_OK)
                goto failed;

            buffer[offset]      = (uint8_t)data;
            buffer[offset + 1U] = (uint8_t)(data >> 8);
            buffer[offset + 2U] = (uint8_t)(data >> 16);
            buffer[offset + 3U] = (uint8_t)(data >> 24);
            offset += 4U;
            count--;
        }
    }

    ret = PRAM_WaitIdle(PRAM_READ_CMD);
    if (ret == HAL_OK)
        return HAL_OK;
failed:
    /* Best effort cleanup; retain the original error. */
    (void)LAN9252_WriteReg(PRAM_READ_CMD, PRAM_ABORT);
    return ret;
}

HAL_StatusTypeDef LAN9252_PRAM_Write(uint16_t address, const uint8_t *buffer,
                                     uint16_t length)
{
    HAL_StatusTypeDef ret;
    uint32_t count, data;
    uint32_t offset = 0U;

    if (buffer == NULL || length == 0U ||
        (address & 3U) != 0U || (length & 3U) != 0U ||
        address < 0x1000U || (uint32_t)address + length > 0x2000U)
        return HAL_ERROR;

    ret = LAN9252_WriteReg(PRAM_WRITE_CMD, PRAM_ABORT);
    if (ret != HAL_OK)
        return ret;
    ret = PRAM_WaitIdle(PRAM_WRITE_CMD);
    if (ret != HAL_OK)
        return ret;

    ret = LAN9252_WriteReg(PRAM_WRITE_ADDR_LEN,
                           ((uint32_t)length << 16) | address);
    if (ret != HAL_OK)
        return ret;
    ret = LAN9252_WriteReg(PRAM_WRITE_CMD, PRAM_BUSY);
    if (ret != HAL_OK)
        goto failed;

    while (offset < length)
    {
        ret = PRAM_WaitFIFO(PRAM_WRITE_CMD, &count);
        if (ret != HAL_OK)
            goto failed;

        while (count != 0U && offset < length)
        {
            data =  (uint32_t)buffer[offset]
                 | ((uint32_t)buffer[offset + 1U] << 8)
                 | ((uint32_t)buffer[offset + 2U] << 16)
                 | ((uint32_t)buffer[offset + 3U] << 24);
            ret = LAN9252_WriteReg(PRAM_WRITE_FIFO, data);
            if (ret != HAL_OK)
                goto failed;

            offset += 4U;
            count--;
        }
    }

    ret = PRAM_WaitIdle(PRAM_WRITE_CMD);
    if (ret == HAL_OK)
        return HAL_OK;
failed:
    (void)LAN9252_WriteReg(PRAM_WRITE_CMD, PRAM_ABORT);
    return ret;
}

HAL_StatusTypeDef LAN9252_PRAM_SelfTest(void)
{
    static uint8_t saved[PRAM_TEST_BYTES];
    static uint8_t expected[PRAM_TEST_BYTES];
    static uint8_t actual[PRAM_TEST_BYTES];
    const uint16_t sizes[2] = {16U, PRAM_TEST_BYTES};
    uint16_t length;
    HAL_StatusTypeDef ret, restore;

    lan9252_pram_debug.stage = 1U;
    lan9252_pram_debug.failed_stage = 0U;
    lan9252_pram_debug.last_command = 0U;
    lan9252_pram_debug.offset = 0U;
    lan9252_pram_debug.expected = 0U;
    lan9252_pram_debug.actual = 0U;
    lan9252_pram_debug.errors = 0U;
    lan9252_pram_debug.restore_verified = 0U;
    lan9252_pram_debug.status = HAL_BUSY;
    lan9252_pram_debug.restore_status = HAL_BUSY; /* Not attempted. */

    ret = LAN9252_PRAM_Read(PRAM_TEST_ADDRESS, saved, PRAM_TEST_BYTES);
    if (ret != HAL_OK)
    {
        lan9252_pram_debug.failed_stage = 1U;
        lan9252_pram_debug.status = ret;
        return ret; /* No test write if backup failed. */
    }

    for (uint32_t pass = 0U; pass < 2U; pass++)
    {
        length = sizes[pass];
        for (uint32_t i = 0U; i < length; i++)
            expected[i] = (uint8_t)((i * 37U + 0x12U) ^ (pass * 0xFFU));

        lan9252_pram_debug.stage = 2U + pass * 2U;
        ret = LAN9252_PRAM_Write(PRAM_TEST_ADDRESS, expected, length);
        if (ret != HAL_OK)
            break;

        lan9252_pram_debug.stage = 3U + pass * 2U;
        ret = LAN9252_PRAM_Read(PRAM_TEST_ADDRESS, actual, length);
        if (ret != HAL_OK)
            break;

        for (uint32_t i = 0U; i < length; i++)
        {
            if (actual[i] != expected[i])
            {
                if (lan9252_pram_debug.errors == 0U)
                {
                    lan9252_pram_debug.offset = i;
                    lan9252_pram_debug.expected = expected[i];
                    lan9252_pram_debug.actual = actual[i];
                }
                lan9252_pram_debug.errors++;
                ret = HAL_ERROR;
            }
        }
        if (ret != HAL_OK)
            break;
    }

    lan9252_pram_debug.status = ret;
    if (ret != HAL_OK)
        lan9252_pram_debug.failed_stage = lan9252_pram_debug.stage;

    /* A timed-out write may still have modified RAM: always restore backup. */
    lan9252_pram_debug.stage = 6U;
    restore = LAN9252_PRAM_Write(PRAM_TEST_ADDRESS, saved, PRAM_TEST_BYTES);
    if (restore == HAL_OK)
        restore = LAN9252_PRAM_Read(PRAM_TEST_ADDRESS, actual, PRAM_TEST_BYTES);
    if (restore == HAL_OK)
    {
        for (uint32_t i = 0U; i < PRAM_TEST_BYTES; i++)
        {
            if (actual[i] != saved[i])
            {
                restore = HAL_ERROR;
                break;
            }
        }
    }
    lan9252_pram_debug.restore_status = restore;
    if (restore != HAL_OK)
    {
        if (lan9252_pram_debug.failed_stage == 0U)
            lan9252_pram_debug.failed_stage = 6U;
        return restore;
    }

    lan9252_pram_debug.restore_verified = 1U;
    if (ret == HAL_OK)
        lan9252_pram_debug.stage = 7U;
    return ret;
}
