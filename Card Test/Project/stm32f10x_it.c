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

u16 dplData[4608];
u16 tempBuff[3];
u16 dplCnt = 0;
u8 fCatchData = 0; 
u8 fReload = 0;
u8 fCheck = 0;
u8 cCnt = 0;
/** @addtogroup STM32F10x_StdPeriph_Examples
  * @{
  */

/** @addtogroup USART_Interrupt
  * @{
  */ 
extern void	sysTickCount(void);

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
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
	sysTickCount();
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
	
}

void TIM6_IRQHandler(void)
{
	u8 portA = 0;
	
	tempBuff[0] = (GPIOC->IDR&0x0FFF);
	tempBuff[1] = (((GPIOD->IDR&0xFF00)>>8)|((GPIOE->IDR&0x000F)<<8));
	portA = (GPIOA->IDR&0x003D)|((GPIOA->IDR&0x0800)<<1)|(GPIOD->IDR&0x00C0);
	tempBuff[2] = (portA|((GPIOB->IDR&0x000F)<<8));
	
	TIM6->SR = ~TIM_FLAG_Update;
}

/**
  * @brief  This function handles USART2 global interrupt request.
  * @param  None
  * @retval None
  */

void EXTI15_10_IRQHandler(void)
{	
	u8 portA = 0;
	
	if((EXTI->PR & EXTI_Line12) != RESET)
	{			
		if(fCatchData)
		{
			dplData[dplCnt++] = (GPIOC->IDR&0x0FFF);
			dplData[dplCnt++] = (((GPIOD->IDR&0xFF00)>>8)|((GPIOE->IDR&0x000F)<<8));
			portA = (GPIOA->IDR&0x003D)|((GPIOA->IDR&0x0800)<<1)|(GPIOD->IDR&0x00C0);
			dplData[dplCnt++] = (portA|((GPIOB->IDR&0x000F)<<8));					
			if(dplCnt == 4608)
			{
				if(cCnt == 10)
				{
					cCnt = 0;
					fReload = 1;
					fCatchData = 0;
				}
				else
				{
					cCnt ++;
					dplCnt = 0;
				}
				//VGA_INT_Enable();
			}
		}
		EXTI->PR = EXTI_Line12;
	}
	if((EXTI->PR & EXTI_Line15) != RESET)
	{
		if(fCheck)
		{			
			if((LED_SELECT_PORT1->IDR&LED_SA_PIN1) && (LED_SELECT_PORT1->IDR&LED_SB_PIN1))
			{
				fCheck = 0;
				dplCnt = 0;
				fCatchData = 1;				
			}
		}
		EXTI->PR = EXTI_Line15;
	}	
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
