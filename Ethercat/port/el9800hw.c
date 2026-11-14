/**
\file    el9800hw.c
\author EthercatSSC@beckhoff.com
\brief Implementation
Hardware access implementation for EL9800 onboard PIC18/PIC24 connected via SPI to ESC

\version 5.11

<br>Changes to version V5.10:<br>
V5.11 ECAT10: change PROTO handling to prevent compiler errors<br>
V5.11 EL9800 2: change PDI access test to 32Bit ESC access and reset AL Event mask after test even if AL Event is not enabled<br>
<br>Changes to version V5.01:<br>
V5.10 ESC5: Add missing swapping<br>
V5.10 HW3: Sync1 Isr added<br>
V5.10 HW4: Add volatile directive for direct ESC DWORD/WORD/BYTE access<br>
           Add missing swapping in mcihw.c<br>
           Add "volatile" directive vor dummy variables in enable and disable SyncManger functions<br>
           Add missing swapping in EL9800hw files<br>
<br>Changes to version V5.0:<br>
V5.01 HW1: Invalid ESC access function was used<br>
<br>Changes to version V4.40:<br>
V5.0 ESC4: Save SM disable/Enable. Operation may be pending due to frame handling.<br>
<br>Changes to version V4.30:<br>
V4.40 : File renamed from spihw.c to el9800hw.c<br>
<br>Changes to version V4.20:<br>
V4.30 ESM: if mailbox Syncmanger is disabled and bMbxRunning is true the SyncManger settings need to be revalidate<br>
V4.30 EL9800: EL9800_x hardware initialization is moved to el9800.c<br>
V4.30 SYNC: change synchronisation control function. Add usage of 0x1C32:12 [SM missed counter].<br>
Calculate bus cycle time (0x1C32:02 ; 0x1C33:02) CalcSMCycleTime()<br>
V4.30 PDO: rename PDO specific functions (COE_xxMapping -> PDO_xxMapping and COE_Application -> ECAT_Application)<br>
V4.30 ESC: change requested address in GetInterruptRegister() to prevent acknowledge events.<br>
(e.g. reading an SM config register acknowledge SM change event)<br>
GENERIC: renamed several variables to identify used SPI if multiple interfaces are available<br>
V4.20 MBX 1: Add Mailbox queue support<br>
V4.20 SPI 1: include SPI RxBuffer dummy read<br>
V4.20 DC 1: Add Sync0 Handling<br>
V4.20 PIC24: Add EL9800_4 (PIC24) required source code<br>
V4.08 ECAT 3: The AlStatusCode is changed as parameter of the function AL_ControlInd<br>
<br>Changes to version V4.02:<br>
V4.03 SPI 1: In ISR_GetInterruptRegister the NOP-command should be used.<br>
<br>Changes to version V4.01:<br>
V4.02 SPI 1: In HW_OutputMapping the variable u16OldTimer shall not be set,<br>
             otherwise the watchdog might exceed too early.<br>
<br>Changes to version V4.00:<br>
V4.01 SPI 1: DI and DO were changed (DI is now an input for the uC, DO is now an output for the uC)<br>
V4.01 SPI 2: The SPI has to operate with Late-Sample = FALSE on the Eva-Board<br>
<br>Changes to version V3.20:<br>
V4.00 ECAT 1: The handling of the Sync Manager Parameter was included according to<br>
              the EtherCAT Guidelines and Protocol Enhancements Specification<br>
V4.00 APPL 1: The watchdog checking should be done by a microcontroller<br>
                 timer because the watchdog trigger of the ESC will be reset too<br>
                 if only a part of the sync manager data is written<br>
V4.00 APPL 4: The EEPROM access through the ESC is added

*/


/*--------------------------------------------------------------------------------------
------
------    Includes
------
--------------------------------------------------------------------------------------*/
#include "ecat_def.h"

#if EL9800_HW
#include "bsp_spiflash.h"
#include "bsp_gpio.h"
#include "bsp_GeneralTIM.h"
#include "bsp_debug_usart.h"
#include "main.h"
#include "ecatslv.h"

#define   _EL9800HW_ 1
#include "el9800hw.h"
#undef    _EL9800HW_
/* ECATCHANGE_START(V5.11) ECAT10*/
/*remove definition of _EL9800HW_ (#ifdef is used in el9800hw.h)*/
/* ECATCHANGE_END(V5.11) ECAT10*/

#include "ecatappl.h"

void mem_test(void);
/*--------------------------------------------------------------------------------------
------
------    internal Types and Defines
------
--------------------------------------------------------------------------------------*/

typedef union
{
    unsigned short    Word;
    unsigned char    Byte[2];
} UBYTETOWORD;

typedef union 
{
    UINT8           Byte[2];
    UINT16          Word;
}
UALEVENT;

/*-----------------------------------------------------------------------------------------
------
------    SPI defines/macros
------
-----------------------------------------------------------------------------------------*/
#define    SPI_DEACTIVE                      1
#define    SPI_ACTIVE                        0

#if        INTERRUPTS_SUPPORTED
/*-----------------------------------------------------------------------------------------
------
------    Global Interrupt setting
------
-----------------------------------------------------------------------------------------*/

#define    DISABLE_GLOBAL_INT             __disable_irq()			
#define    ENABLE_GLOBAL_INT           		__enable_irq()				
#define    DISABLE_AL_EVENT_INT           DISABLE_GLOBAL_INT
#define    ENABLE_AL_EVENT_INT            ENABLE_GLOBAL_INT

/*-----------------------------------------------------------------------------------------
------
------    ESC Interrupt
------
-----------------------------------------------------------------------------------------*/
#if        AL_EVENT_ENABLED
#define    INIT_ESC_INT           				EXTI8_Configuration();	//PE8
#define    EcatIsr                				EXTI8_IRQHandler
#define    ACK_ESC_INT         						__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_8);

#endif //#if AL_EVENT_ENABLED

/*-----------------------------------------------------------------------------------------
------
------    SYNC0 Interrupt
------
-----------------------------------------------------------------------------------------*/
#if        DC_SUPPORTED && _STM32_IO8

#define    INIT_SYNC0_INT                 EXTI6_Configuration();	            //PC6
#define    DISABLE_SYNC0_INT              NVIC_DisableIRQ(EXTI9_5_IRQn);      // {(_INT3IE)=0;}//disable interrupt source INT3
#define    ENABLE_SYNC0_INT               NVIC_EnableIRQ(EXTI9_5_IRQn);	      // {(_INT3IE) = 1;} //enable interrupt source INT3
#define    ACK_SYNC0_INT                  __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_6);

/*ECATCHANGE_START(V5.10) HW3*/
#define    INIT_SYNC1_INT                 EXTI7_Configuration();               //PC7
#define    DISABLE_SYNC1_INT              NVIC_DisableIRQ(EXTI9_5_IRQn);       // {(_INT4IE)=0;}//disable interrupt source INT4
#define    ENABLE_SYNC1_INT               NVIC_EnableIRQ(EXTI9_5_IRQn);        //{(_INT4IE) = 1;} //enable interrupt source INT4
#define    ACK_SYNC1_INT                  __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_7);

/*ECATCHANGE_END(V5.10) HW3*/

#endif //#if DC_SUPPORTED && _STM32_IO8

#endif	//#if INTERRUPTS_SUPPORTED
/*-----------------------------------------------------------------------------------------
------
------    Hardware timer
------
-----------------------------------------------------------------------------------------*/
#if        _STM32_IO8

#if        ECAT_TIMER_INT
#define    ECAT_TIMER_INT_STATE       
#define    ECAT_TIMER_ACK_INT        		__HAL_TIM_CLEAR_IT(&htimx , TIM_IT_UPDATE);	
#define    TimerIsr                    	TIM2_IRQHandler               //SysTick_Handler//						
#define    ENABLE_ECAT_TIMER_INT        NVIC_EnableIRQ(TIM2_IRQn);	  //SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;//NVIC_EnableIRQ(TIM2_IRQn) ;	
#define    DISABLE_ECAT_TIMER_INT       NVIC_DisableIRQ(TIM2_IRQn);   //SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;//NVIC_DisableIRQ(SysTick_IRQn/*TIM2_IRQn*/) ;

#define    INIT_ECAT_TIMER              TIM_Configuration(10) ;       //SysTick_Config(SystemCoreClock/1000);  
#define    STOP_ECAT_TIMER              DISABLE_ECAT_TIMER_INT;       /*disable timer interrupt*/
#define    START_ECAT_TIMER             ENABLE_ECAT_TIMER_INT
#else    //#if ECAT_TIMER_INT

#define INIT_ECAT_TIMER      					TIM_Configuration(10) //SysTick_Config(SystemCoreClock/1000);//          {(PR7) = 2000;/*set period*/

#define STOP_ECAT_TIMER               TIM_Cmd(TIM2, DISABLE);	// SysTick->CTRL  &=  ~SysTick_CTRL_ENABLE_Msk;      //			 

#define START_ECAT_TIMER              TIM_Cmd(TIM2, ENABLE);// SysTick->CTRL  |=  SysTick_CTRL_ENABLE_Msk;      			

#endif //#else #if ECAT_TIMER_INT

#elif _STM32_IO4

#if !ECAT_TIMER_INT
#define    ENABLE_ECAT_TIMER_INT       NVIC_EnableIRQ(TIM2_IRQn) ;	// SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;//
#define    DISABLE_ECAT_TIMER_INT      NVIC_DisableIRQ(TIM2_IRQn) ;//SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;//
#define INIT_ECAT_TIMER               TIM_Configuration(10) ;//	SysTick_Config(SystemCoreClock/1000);//			
#define STOP_ECAT_TIMER              	TIM_Cmd(TIM2, DISABLE);//	SysTick->CTRL  &=  ~SysTick_CTRL_ENABLE_Msk; //		
#define START_ECAT_TIMER           		TIM_Cmd(TIM2, ENABLE);	//	SysTick->CTRL  |=  SysTick_CTRL_ENABLE_Msk; //		

#else    //#if !ECAT_TIMER_INT

#warning "define Timer Interrupt Macros"

#endif //#else #if !ECAT_TIMER_INT
#endif //#elif _STM32_IO4

/*-----------------------------------------------------------------------------------------
------
------    Configuration Bits
------
-----------------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------------------
------
------    LED defines
------
-----------------------------------------------------------------------------------------*/
#if _STM32_IO8
#define LED_ECATGREEN               
#define LED_ECATRED                   
#endif //_STM32_IO8


/*--------------------------------------------------------------------------------------
------
------    internal Variables
------
--------------------------------------------------------------------------------------*/
UALEVENT         EscALEvent;            //contains the content of the ALEvent register (0x220), this variable is updated on each Access to the Esc

/*--------------------------------------------------------------------------------------
------
------    internal functions
------
--------------------------------------------------------------------------------------*/
#define BYTE_TEST_ADDR   0x0064U
#define BYTE_TEST_VALUE  0x87654321UL
#define BYTE_TEST_COUNT  1000U

volatile uint32_t byte_test_value = 0;
volatile uint32_t byte_test_first_error = 0;
volatile uint32_t byte_test_completed = 0;
volatile uint32_t byte_test_errors = 0;
volatile HAL_StatusTypeDef byte_test_status = HAL_BUSY;

HAL_StatusTypeDef LAN9252_ByteTest(void)
{
    uint32_t value;

   
    byte_test_first_error = 0U;
    byte_test_completed = 0U;
    byte_test_errors = 0U;
    byte_test_status = HAL_BUSY;

    for (uint32_t i = 0U; i < BYTE_TEST_COUNT; i++)
    {
        value = SPIReadDWord(BYTE_TEST_ADDR);
        byte_test_value = value;

        if (value != BYTE_TEST_VALUE)
        {
            if (byte_test_errors == 0U)
            {
                byte_test_first_error = value;
            }

            byte_test_errors++;
        }

        byte_test_completed++;
    }

    byte_test_status =
        (byte_test_errors == 0U) ? HAL_OK : HAL_ERROR;

    return byte_test_status;
}

/////////////////////////////////////////////////////////////////////////////////////////
/**
 \brief  The function operates a SPI access without addressing.

        The first two bytes of an access to the EtherCAT ASIC always deliver the AL_Event register (0x220).
        It will be saved in the global "EscALEvent"
*////////////////////////////////////////////////////////////////////////////////////////
static void GetInterruptRegister(void)
{

    #if AL_EVENT_ENABLED
    DISABLE_AL_EVENT_INT;
#endif
	
    /* select the SPI */
    SELECT_SPI;
	
	 HW_EscReadIsr((MEM_ADDR *)&EscALEvent.Word, 0x220, 2);
	/* if the SPI transmission rate is higher than 15 MBaud, the Busy detection shall be
       done here */
	
  DESELECT_SPI;
 #if AL_EVENT_ENABLED
    ENABLE_AL_EVENT_INT;
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////
/**
 \brief  The function operates a SPI access without addressing.
        Shall be implemented if interrupts are supported else this function is equal to "GetInterruptRegsiter()"

        The first two bytes of an access to the EtherCAT ASIC always deliver the AL_Event register (0x220).
        It will be saved in the global "EscALEvent"
*////////////////////////////////////////////////////////////////////////////////////////
#if !INTERRUPTS_SUPPORTED
#define ISR_GetInterruptRegister GetInterruptRegister
#else
static void ISR_GetInterruptRegister(void)
{
    /* SPI should be deactivated to interrupt a possible transmission */
    DESELECT_SPI;

    /* select the SPI */
    SELECT_SPI;

		HW_EscReadIsr((MEM_ADDR *)&EscALEvent.Word, 0x220, 2);

  /* if the SPI transmission rate is higher than 15 MBaud, the Busy detection shall be
       done here */

    DESELECT_SPI;
}
#endif //#else #if !INTERRUPTS_SUPPORTED



void GPIO_Config(void) 
{ 
   /* 定义IO硬件初始化结构体变量 */
  GPIO_InitTypeDef GPIO_InitStruct;
	
  __HAL_RCC_GPIOA_CLK_ENABLE();	
  __HAL_RCC_GPIOE_CLK_ENABLE();	
  __HAL_RCC_GPIOB_CLK_ENABLE();	
	
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;	
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);   
	
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;	
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;	
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);   
	
  GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;	
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);   	
} 

UINT8 HW_Init(void)
{
	
		UINT16 intMask;
		UINT32 data;
		/* 复位所有外设，初始化Flash接口和系统滴答定时器 */
		HAL_Init();

		/* 配置系统时钟 */
		//SystemClock_Config();

		GPIO_Config();
			/* initialize the SSP registers for the ESC SPI */
		MX_SPIFlash_Init();	

		MX_DEBUG_USART_Init();

		RST_Configuration();

		printf("EtherCAT测试\n");

		RST_ESC;

		HAL_Delay(100);

		RST_ESCEND;

		HAL_Delay(100);

		mem_test();// 测试PDI接口
		
		if (LAN9252_ByteTest() != HAL_OK)
		{
				Error_Handler();
		}

		do
		{
				intMask = 0x0093;
				HW_EscWriteDWord(intMask, ESC_AL_EVENTMASK_OFFSET);
				intMask = 0;
				HW_EscReadDWord(intMask, ESC_AL_EVENTMASK_OFFSET);
		} while (intMask!= 0x0093);
		
		//IRQ enable,IRQ polarity, IRQ buffer type in Interrupt Configuration register.
    //Wrte 0x54 - 0x00000101
    data = 0x00000101;
 
    SPIWriteDWord (0x54,data);
    
    //Write in Interrupt Enable register -->
    //Write 0x5c - 0x00000001
    data = 0x00000001;
    SPIWriteDWord (0x5C, data);
    
    SPIReadDWord(0x58);	

		intMask = 0x00;
	  
    HW_EscWriteDWord(intMask, ESC_AL_EVENTMASK_OFFSET);

#if AL_EVENT_ENABLED
    INIT_ESC_INT;
    ENABLE_ESC_INT();
#endif

#if DC_SUPPORTED && _STM32_IO8
    INIT_SYNC0_INT
    INIT_SYNC1_INT

    ENABLE_SYNC0_INT;
    ENABLE_SYNC1_INT;
#endif

    INIT_ECAT_TIMER;
    START_ECAT_TIMER;
  
	
#if INTERRUPTS_SUPPORTED
    /* enable all interrupts */
    ENABLE_GLOBAL_INT;
#endif

    printf("ENT测试\n");

    return 0;
}



/////////////////////////////////////////////////////////////////////////////////////////
/**
 \brief    This function shall be implemented if hardware resources need to be release
        when the sample application stops
*////////////////////////////////////////////////////////////////////////////////////////
void HW_Release(void)
{

}

/////////////////////////////////////////////////////////////////////////////////////////
/**
 \return    first two Bytes of ALEvent register (0x220)

 \brief  This function gets the current content of ALEvent register
*////////////////////////////////////////////////////////////////////////////////////////
UINT16 HW_GetALEventRegister(void)
{
    GetInterruptRegister();
    return EscALEvent.Word;
}
#if INTERRUPTS_SUPPORTED
/////////////////////////////////////////////////////////////////////////////////////////
/**
 \return    first two Bytes of ALEvent register (0x220)

 \brief  The SPI PDI requires an extra ESC read access functions from interrupts service routines.
        The behaviour is equal to "HW_GetALEventRegister()"
*////////////////////////////////////////////////////////////////////////////////////////
#if _STM32_IO4  && AL_EVENT_ENABLED
/* the pragma interrupt_level is used to tell the compiler that these functions will not
   be called at the same time from the main function and the interrupt routine */
//#pragma interrupt_level 1
#endif
UINT16 HW_GetALEventRegister_Isr(void)
{
     ISR_GetInterruptRegister();
     return EscALEvent.Word;
}
#endif


#if UC_SET_ECAT_LED
/////////////////////////////////////////////////////////////////////////////////////////
/**
 \param RunLed            desired EtherCAT Run led state
 \param ErrLed            desired EtherCAT Error led state

  \brief    This function updates the EtherCAT run and error led
*////////////////////////////////////////////////////////////////////////////////////////
void HW_SetLed(UINT8 RunLed,UINT8 ErrLed)
{
#if _STM32_IO8
 //     LED_ECATGREEN = RunLed;
//      LED_ECATRED   = ErrLed;
#endif
}
#endif //#if UC_SET_ECAT_LED
/////////////////////////////////////////////////////////////////////////////////////////
/**
 \param pData          Pointer to a byte array which holds data to write or saves read data.
 \param Address        EtherCAT ASIC address ( upper limit is 0x1FFF )    for access.
 \param Len            Access size in Bytes.

 \brief  This function operates the SPI read access to the EtherCAT ASIC.
*////////////////////////////////////////////////////////////////////////////////////////
void HW_EscRead( MEM_ADDR *pData, UINT16 Address, UINT16 Len )
{
    /* HBu 24.01.06: if the SPI will be read by an interrupt routine too the
			 mailbox reading may be interrupted but an interrupted
			 reading will remain in a SPI transmission fault that will
			 reset the internal Sync Manager status. Therefore the reading
			 will be divided in 1-byte reads with disabled interrupt
  	*/
	
    UINT16 i;
    UINT8 *pTmpData = (UINT8 *)pData;

    /* loop for all bytes to be read */
    while ( Len > 0 )
    {
        if (Address >= 0x1000)
        {
            i = Len;
        }
        else
        {
            i= (Len > 4) ? 4 : Len;

            if(Address & 01)
            {
               i=1;
            }
            else if (Address & 02)
            {
               i= (i&1) ? 1:2;
            }
            else if (i == 03)
            {
                i=1;
            }
        }

        DISABLE_AL_EVENT_INT;
        SPIReadDRegister(pTmpData,Address,i);				
        ENABLE_AL_EVENT_INT;
        Len -= i;
        pTmpData += i;
        Address += i;
    }
}

#if INTERRUPTS_SUPPORTED
/////////////////////////////////////////////////////////////////////////////////////////
/**
 \param pData        Pointer to a byte array which holds data to write or saves read data.
 \param Address     EtherCAT ASIC address ( upper limit is 0x1FFF )    for access.
 \param Len            Access size in Bytes.

\brief  The SPI PDI requires an extra ESC read access functions from interrupts service routines.
        The behaviour is equal to "HW_EscRead()"
*////////////////////////////////////////////////////////////////////////////////////////
#if _STM32_IO4  && AL_EVENT_ENABLED
/* the pragma interrupt_level is used to tell the compiler that these functions will not
   be called at the same time from the main function and the interrupt routine */
//#pragma interrupt_level 1
#endif
void HW_EscReadIsr( MEM_ADDR *pData, UINT16 Address, UINT16 Len )
{

   UINT16 i;
   UINT8 *pTmpData = (UINT8 *)pData;
   /* send the address and command to the ESC */
   /* loop for all bytes to be read */
   while ( Len > 0 )
   {
        if (Address >= 0x1000)
        {
            i = Len;
        }
        else
        {
            i= (Len > 4) ? 4 : Len;

            if(Address & 01)
            {
               i=1;
            }
            else if (Address & 02)
            {
               i= (i&1) ? 1:2;
            }
            else if (i == 03)
            {
                i=1;
            }
        }

        SPIReadDRegister(pTmpData, Address,i);
        Len -= i;
        pTmpData += i;
        Address += i;
    }
   
}

#endif //#if INTERRUPTS_SUPPORTED
/////////////////////////////////////////////////////////////////////////////////////////
/**
 \param pData        Pointer to a byte array which holds data to write or saves write data.
 \param Address     EtherCAT ASIC address ( upper limit is 0x1FFF )    for access.
 \param Len            Access size in Bytes.

  \brief  This function operates the SPI write access to the EtherCAT ASIC.
*////////////////////////////////////////////////////////////////////////////////////////
void HW_EscWrite( MEM_ADDR *pData, UINT16 Address, UINT16 Len )
{

    UINT16 i;
    UINT8 *pTmpData = (UINT8 *)pData;

    /* loop for all bytes to be written */
    while ( Len )
    {

        if (Address >= 0x1000)
        {
            i = Len;
        }
        else
        {
            i= (Len > 4) ? 4 : Len;

            if(Address & 01)
            {
               i=1;
            }
            else if (Address & 02)
            {
               i= (i&1) ? 1:2;
            }
            else if (i == 03)
            {
                i=1;
            }
        }

        DISABLE_AL_EVENT_INT;
       
        /* start transmission */
        SPIWriteRegister(pTmpData, Address, i);
        ENABLE_AL_EVENT_INT;  
        /* next address */
        Len -= i;
        pTmpData += i;
        Address += i;
    }
}

#if INTERRUPTS_SUPPORTED
/////////////////////////////////////////////////////////////////////////////////////////
/**
 \param pData        Pointer to a byte array which holds data to write or saves write data.
 \param Address     EtherCAT ASIC address ( upper limit is 0x1FFF )    for access.
 \param Len            Access size in Bytes.

 \brief  The SPI PDI requires an extra ESC write access functions from interrupts service routines.
        The behaviour is equal to "HW_EscWrite()"
*////////////////////////////////////////////////////////////////////////////////////////
#if _STM32_IO4  && AL_EVENT_ENABLED
/* the pragma interrupt_level is used to tell the compiler that these functions will not
   be called at the same time from the main function and the interrupt routine */
//#pragma interrupt_level 1
#endif
void HW_EscWriteIsr( MEM_ADDR *pData, UINT16 Address, UINT16 Len )
{

    UINT16 i ;
    UINT8 *pTmpData = (UINT8 *)pData;
    /* loop for all bytes to be written */
    while ( Len )
    {

        if (Address >= 0x1000)
        {
            i = Len;
        }
        else
        {
            i= (Len > 4) ? 4 : Len;

            if(Address & 01)
            {
               i=1;
            }
            else if (Address & 02)
            {
               i= (i&1) ? 1:2;
            }
            else if (i == 03)
            {
                i=1;
            }
        }
        
       /* start transmission */
       SPIWriteRegister(pTmpData, Address, i);       
       /* next address */
        Len -= i;
        pTmpData += i;
        Address += i;
    }

}

#endif

#if AL_EVENT_ENABLED
/////////////////////////////////////////////////////////////////////////////////////////
/**
 \brief    Interrupt service routine for the PDI interrupt from the EtherCAT Slave Controller
*////////////////////////////////////////////////////////////////////////////////////////

#if _STM32_IO4
/* the pragma interrupt_level is used to tell the compiler that these functions will not
   be called at the same time from the main function and the interrupt routine */
//#pragma interrupt_level 1

//interrupt
//void HWISR_EcatIsr(void)
void EcatIsr(void)
#else
//void EcatIsr(void)		//void __attribute__ ((__interrupt__, no_auto_psv)) EscIsr(void)
#endif
//{
//	if(HAL_GPIO_ReadPin(GPIOD,GPIO_PIN_4)==0)
//	{		
//		 HAL_NVIC_DisableIRQ(EXTI4_IRQn);	
//     PDI_Isr();
//     HAL_NVIC_EnableIRQ(EXTI4_IRQn);
//    /* reset the interrupt flag */
//		ACK_ESC_INT; 
//	}
//}
#endif     // AL_EVENT_ENABLED







//#if DC_SUPPORTED&& _STM32_IO8
///////////////////////////////////////////////////////////////////////////////////////////
///**
// \brief    Interrupt service routine for the interrupts from SYNC0
//*////////////////////////////////////////////////////////////////////////////////////////

//void EXTI9_5_IRQHandler(void)
//{
//    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6);
//    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
//    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_8);
//}

//void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
//{
//  if(GPIO_Pin==GPIO_PIN_8)
//  {		
//     PDI_Isr();
//     /* reset the interrupt flag */
//		 //ACK_ESC_INT; 		
//		     return;
//	}
//  else if(GPIO_Pin==GPIO_PIN_7)
//  {	
//    Sync1_Isr();
//    /* reset the interrupt flag */

//    //ACK_SYNC1_INT;		
//		     return;
//	}
//  else if(GPIO_Pin==GPIO_PIN_6)
//  {	
//    Sync0_Isr();
//    /* reset the interrupt flag */

//    //ACK_SYNC0_INT;		
//		     return;
//	}	
//}

///*ECATCHANGE_END(V5.10) HW3*/
//#endif

#if INTERRUPTS_SUPPORTED && _STM32_IO8

void EXTI9_5_IRQHandler(void)
{
#if AL_EVENT_ENABLED
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_8);  // PE8：INT
#endif

#if DC_SUPPORTED
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6);  // PC6：SYNC0
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);  // PC7：SYNC1
#endif
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
#if AL_EVENT_ENABLED
    if (GPIO_Pin == GPIO_PIN_8)
    {
        PDI_Isr();
        return;
    }
#endif

#if DC_SUPPORTED
    if (GPIO_Pin == GPIO_PIN_6)
    {
        Sync0_Isr();
        return;
    }

    if (GPIO_Pin == GPIO_PIN_7)
    {
        Sync1_Isr();
        return;
    }
#endif

    (void)GPIO_Pin;
}

#endif

#if _STM32_IO8 && ECAT_TIMER_INT
// Timer 2 ISR (0.1ms)
void TimerIsr(void)
{
			DISABLE_ESC_INT();
		
		  ECAT_CheckTimer();

			ECAT_TIMER_ACK_INT;
		
			ENABLE_ESC_INT();
}

#endif

#endif //#if EL9800_HW
/** @} */




