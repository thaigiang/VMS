/**
  ******************************************************************************
  * @file    USART/Interrupt/stm32f10x_it.c 
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and peripherals
  *          interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32f10x_it.h"
#include "stm32f10x_conf.h"
#include "HardwareConfig.h"
#include "LedMatrix.h"
#include "common.h"

/** @addtogroup STM32F10x_StdPeriph_Examples
  * @{
  */

/** @addtogroup USART_Interrupt
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
bool fReceived = false;
bool fCheckCmd = false;
bool fPredef = false;
u16 time_count = 0;
u8 delay_count = 0;
u16 uart_timeout = 0;
bool fReload = false;
extern HW_CONFIG hwConfig;

extern u8 rcvData[MAX_PACKAGE_LENGTH];
extern u8* ledData;
extern u16 rcvCount;
extern u8 predefIndex;
extern bool fPlayPredef;
extern WATCH watch;
extern bool fChangeData;
extern u32 TimingDelay;
bool fDisplay = true;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  // Reset MU
  NVIC_SystemReset();
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  // Reset MU
  NVIC_SystemReset();
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
	 // Reset MU
  NVIC_SystemReset();
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
	  // Reset MU
  NVIC_SystemReset();
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSV_Handler exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
	if (TimingDelay != 0x00)
	{ 
		TimingDelay--;
	}

/*
	if(TimingDelay < 0xFFFFFFFF)
		TimingDelay ++;
*/
}

/******************************************************************************/
/*            STM32F10x Peripherals Interrupt Handlers                        */
/******************************************************************************/

/**
  * @brief  This function handles USART1 global interrupt request.
  * @param  None
  * @retval None
  */
void USART1_IRQHandler(void)
{
	if((USART1->SR & USART_FLAG_RXNE) != RESET)
  {
  	u8 c;

		uart_timeout = 0;
		fReceived = true;
		
		c = (USART1->DR & (u16)0x01FF);

		if(rcvCount<MAX_PACKAGE_LENGTH)
		{
			rcvData[rcvCount] = c;
			rcvCount ++;
			if((rcvCount==1) && (c != MAIN_TAG))
			{
				rcvCount = 0;
			}
			if(rcvData[rcvCount - 1] == '\n')
			{
				if(rcvData[rcvCount - 2] == '\r')
				{
					fCheckCmd = true;					
				}
			}
		}
		else
		{
			fCheckCmd = true;
		}
	}
	else
	{
		u32 temp = 0;

		temp = USART1->SR;
		temp = USART1->DR;
	}
}


void EXTI9_5_IRQHandler(void)
{
	
}

void TIM6_IRQHandler(void)
{	
	time_count ++;
	if(time_count >= 10)
	{		
		if(fReceived)
			uart_timeout ++;
		
		if(watch.active)
		{
			if(watch.dot == true)
			{				
				watch.dot = false;
			}
			else
			{				
				watch.dot = true;
			}
		}
		if(fPlayPredef)
		{
			watch.active = false;
			delay_count ++;
			if(delay_count >= PREDEFINED_DELAY_TIME)
			{
				predefIndex ++;
				if(predefIndex == MAX_PREDEFINED_IMG)
					predefIndex = 0;
				fPredef = true;
				delay_count = 0;
			}
		}		
		time_count = 0;
	}
	TIM_ClearFlag(TIM6,TIM_FLAG_Update);
}

void TIM2_IRQHandler(void)
{	
	static int count = 0;
	if(count<2)
		count++;
	else
	{
		display(ledData);	
		count = 0;
	}
	
//	if(hwConfig.ledType==SCAN_STATIC && !fDisplay)
//	{
//		display(ledData);
//		fDisplay = true;
//	}
//	else if(hwConfig.ledType!=SCAN_STATIC)
//		display(ledData);	

	
//	display(ledData);
	
	TIM_ClearFlag(TIM2,TIM_FLAG_Update);
}

void TIM3_IRQHandler(void)
{
	if(fCheckCmd)
	{
		CmdExecute();
		fCheckCmd = false;
	}
	if(fPredef)
	{
		loadPredefinedImg(predefIndex);
		fPredef = false;
	}
	

	if(fChangeData)
	{
		convertData(ledData);
		fReload = true;
		fChangeData = false;
		fDisplay = false;
	}
	
	TIM_ClearFlag(TIM3,TIM_FLAG_Update);
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */ 

/**
  * @}
  */ 

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
