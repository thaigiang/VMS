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
#include <string.h>
#include "stm32f10x.h"
#include "stm32f10x_it.h"
#include "stm32f10x_conf.h"
#include "typedef.h"
#include "dataStruct.h"
#include "Stm32_ds1307.h"
#include "common.h"
#include "DS18B20.h"
#include "HardwareConfig.h"
#include "aes128.h"

#define CONNECTION_TIMEOUT		30			// seconds
/** @addtogroup STM32F10x_StdPeriph_Examples
  * @{
  */

/** @addtogroup USART_Interrupt
  * @{
  */ 
u32 key[AES_KEY_SIZE]={0x30313233,0x34353637,0x38396162,0x63646566};
u32 IV[AES_BLOCK_SIZE] = {0,0,0,0};
/* Expanded key */
u32 exp_key[AES_EXPKEY_SIZE];

u16 page_count = 0;
bool pageChange = false;
bool fRcvUSART1 = false;
bool fRcvUSART2 = false;
bool fEthernetHardReseted = true;
u16 uart_timeout = 0;
u8 sysTemp = 0;
bool fSlaveRes = false;
u8 slaveResValue = 0;
u32 connTimeout = 0;

extern WN_CONFIG wnConfig;
extern STATUS sysStatus;
extern u8 rcvData[TCP_PACKAGE_LENGTH];
extern u16 rcvCount;
extern u8 rcvTestCmd[TCP_PACKAGE_LENGTH];
extern u16 rcvCountUSART2;
extern bool fTestMode;
extern PAGE* page;
extern bool fUpdateLedData;
extern tREALTIME rtS;
extern bool fSendRes;
extern RESPONSE res_value;
extern COMMAND_TAG cmd;
extern bool fCaptureData;
extern u8 imgBuffer[SCREEN_BUFF_LENGTH];
extern u16 captureCount;
extern u16 ledDataLength;
extern bool fWarning;
extern WARNING_CODE warning;
extern NETWORK_STATUS networkStatus;
extern u32 TimingDelay;
extern EVENT event;
extern bool fSdcErr;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/
static void TimingDelay_Decrement(void)
{
  if (TimingDelay != 0x00)
  { 
    TimingDelay--;
  }
}

u32 littleToBig(u32 little)
{
	u8* ptr = (u8*)&little;
	u32 big = 0;

	big = (*ptr++<<24)|(*ptr++<<16)|(*ptr++<<8)|(*ptr++);
	return big;
}

static u32 bigToLittle(u32 big)
{
	u8* ptr = (u8*)&big;
	u32 little = 0;

	little = (*ptr++<<24)|(*ptr++<<16)|(*ptr++<<8)|(*ptr++);
	return little;
}

u8 aes_128_cbc_decrypt(u32* key, u32* iv, u8 *data, u16 data_len)
{
	u32 cbc[AES_BLOCK_SIZE], tmp[AES_BLOCK_SIZE];
	u8 *pos = data;
	u32 data_tmp[AES_BLOCK_SIZE];
	u16 data_size = 0;
	int i, j, blocks;

	// Decryption key scheduling, to be done once 
	AES_keyschedule_dec((u32*)key,(u32*)exp_key);
	data_size = AES_BLOCK_SIZE*4;
	memcpy(cbc, iv, data_size);
	
	blocks = (data_len/data_size);
	
	for (i = 0; i < blocks; i++) 
	{
		memcpy(data_tmp, pos, data_size);
		for(j = 0;j<AES_BLOCK_SIZE;j++)
			tmp[j] = littleToBig(data_tmp[j]);
		AES_decrypt(tmp, data_tmp, exp_key);		
		for (j = 0; j < AES_BLOCK_SIZE; j++)
		        data_tmp[j] ^= cbc[j];		
		memcpy(cbc, tmp, data_size);
		for(j = 0;j<AES_BLOCK_SIZE;j++)
			tmp[j] = bigToLittle(data_tmp[j]);		
		memcpy(pos, tmp, data_size);
		pos += data_size;
	}
	return 0;
}

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
	createLog(MAIN_MODULE,EVENT_LOG,"Hard Fault exception, force DEVICE reset");
	saveLog2SDC();
	NVIC_SystemReset();
	/* Go to infinite loop when Hard Fault exception occurs */
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
	createLog(MAIN_MODULE,EVENT_LOG,"Memory Manage exception, force DEVICE reset");
	saveLog2SDC();
	// Reset MU
	NVIC_SystemReset();
	/* Go to infinite loop when Memory Manage exception occurs */
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
	createLog(MAIN_MODULE,EVENT_LOG,"Bus Fault exception, force DEVICE reset");
	saveLog2SDC();
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
	createLog(MAIN_MODULE,EVENT_LOG,"Usage Fault exception, force DEVICE reset");
	saveLog2SDC();
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
	TimingDelay_Decrement();
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
	u8 c = 0;

	if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET)
	{
		c = USART_ReceiveData(USART1);
		uart_timeout = 0;		
		fRcvUSART1 = true;
		rcvData[rcvCount++] = c;		
			
	}
	else
	{
		u32 temp = 0;

		temp = USART1->SR;
		temp = USART1->DR;
	}
}


void USART2_IRQHandler(void)
{
	u8 c = 0;

	if(USART_GetFlagStatus(USART2, USART_FLAG_RXNE) != RESET)
	{
		c = USART_ReceiveData(USART2);
		uart_timeout = 0;		
		fRcvUSART2 = true;
		fTestMode = true;
		rcvTestCmd[rcvCountUSART2++] = c;	
		USART_SendData(USART2,'H');
	}
	else
	{
		u32 temp = 0;

		temp = USART1->SR;
		temp = USART1->DR;
	}
}

//===========================================
void UART4_IRQHandler(void)
{
	u8 c = 0;

	if(USART_GetFlagStatus(UART4, USART_FLAG_RXNE) != RESET)
	{
		c = USART_ReceiveData(UART4);	
		if(fCaptureData)
		{
			imgBuffer[captureCount] = c;
			captureCount++;
			if(captureCount == ledDataLength)
				fCaptureData = false;
		}
		else
		{
			if((c == DPL_RES_OK) || (c == DPL_RES_ERROR))
			{
				slaveResValue = c;
				fSlaveRes = true;
			}
		}
	}
	else
	{
		u32 temp = 0;

		temp = UART4->SR;
		temp = UART4->DR;
	}
}

//===========================================
void EXTI1_IRQHandler(void)
{
	if(EXTI_GetFlagStatus(EXTI_Line1) == SET)
	{
		u8 i = 0,j = 0;
		static u8 sec_count = 0;
		static u16 bat_count = 0;
		static u8 current_sec = 0;
		
		sec_count ++;
		if(networkStatus != NETWORK_LOST)
		{
			connTimeout ++;
			if(connTimeout == CONNECTION_TIMEOUT)
			{
				networkStatus = NETWORK_LOST;
				event = EVENT_NETWORK_LOST;
				sysStatus |= SYSTEM_ERR_NETWORK;
				displayStatus(sysStatus);
				sendResponse(CMD_REQUEST_RECONNECT,CMD_REQUEST_RECONNECT);
				connTimeout = 0;
			}
		}
		else //network lost
		{
			if(!fEthernetHardReseted)
				{
					u32 i = 0;
					
					GPIO_InitTypeDef GPIO_InitStructure;
					GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
					GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
					GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
					GPIO_Init(GPIOB, &GPIO_InitStructure);
					GPIO_WriteBit(ETHERNET_RESET_PORT, ETHERNET_RESET_PIN,Bit_SET); 
					
					for(i=0;i<10000;i++) __NOP;
					
					GPIO_WriteBit(ETHERNET_RESET_PORT, ETHERNET_RESET_PIN,Bit_RESET);
					fEthernetHardReseted = true;
				}
		}
		

		// Check if SDC error in running time
		if(sysStatus & SYSTEM_ERR_SDC)
		{
			// Try to reinit SDC
			initSDC();
			// If SDC still not avaiable then reset system
			if(sysStatus & SYSTEM_ERR_SDC)
			{
				if(!fSdcErr)
					NVIC_SystemReset();
			}
			else
			{
				fSdcErr = false;
				displayStatus(sysStatus);
			}	
		}
		
		
		if(sec_count == 60)
		{
			sec_count = 0;
			bat_count ++;
			if(bat_count == 5)
			{
				u16 adc_value = 0;
				u32 battery_volt = 0;
				u32 power_volt = 0;
				
				adc_value = ADC_GetConversionValue(ADC1);
				battery_volt = ((adc_value*33*62/4096)/15);
				if(battery_volt < BATTERY_VOLT_EMPTY)
				{
					GPIO_WriteBit(CHARGE_PORT,CHARGE_PIN,Bit_RESET);
				}
				else if(battery_volt >= BATTERY_VOLT_FULL)
				{
					GPIO_WriteBit(CHARGE_PORT,CHARGE_PIN,Bit_SET);
				}
				adc_value = ADC_GetConversionValue(ADC2);
				power_volt = ((adc_value*33*62/4096)/15);
				if(power_volt < BATTERY_VOLT_FULL)
				{
					//sendWarning(WARNING_OUT_OF_POWER);
//					fWarning = true;
//					warning = WARNING_OUT_OF_POWER;
				}
				bat_count = 0;
			}
			rtS = DS1307_GetTime();
			for(i = 0;i<page->header.itemCount;i++)
			{
				PLAYLIST_ITEM* playlist = (PLAYLIST_ITEM*)page->popup[i].Info.data;
				
				if((playlist[0].header.dataType == PLAYLIST_CLOCK)||(playlist[0].header.dataType == PLAYLIST_TEMP))
					page->popup[i].Info.header.isUpdate = false;
			}
			
			sysTemp = Read_Temperature();
			if(sysTemp > wnConfig.maxTemp)
			{
				//sendWarning(WARNING_TOO_HOT);
				//fWarning = true;
				//warning = WARNING_TOO_HOT;
			}			
		}
		for(i = 0;i<page->header.itemCount;i++)
		{
			if(page->popup[i].Info.header.dataType == POPUP_PLAYLIST)
			{
				PLAYLIST_ITEM* playlist = (PLAYLIST_ITEM*)page->popup[i].Info.data;
				
				if((page->popup[i].Info.header.isUpdate)&&(page->popup[i].Info.header.countDown))
				{
					page->popup[i].Info.header.countDown --;
					if(!page->popup[i].Info.header.countDown)
						page->popup[i].Info.header.isUpdate = false;
				}
				for(j = 0;j<page->popup[i].Info.header.itemCount;j++)
				{
					if((playlist[j].TTL_count < 0xFFFF)&&(playlist[j].TTL_count>0))
						playlist[j].TTL_count --;					
				}
			}
		}
		EXTI_ClearFlag(EXTI_Line1);
	}
}


void TIM5_IRQHandler(void)
{
	
	TIM_ClearFlag(TIM5,TIM_FLAG_Update);
}

//===========================================
void TIM6_IRQHandler(void)
{	
	if(fRcvUSART1 && (uart_timeout<0xFFFF))
	{
		uart_timeout ++;
		// Check data received		
		if(uart_timeout>=10)
		{
			u32 i = 0;
			u16 index = 0;
			u16 count_temp = 0;
			u8* ptr;
			u8 crc = 0;
			
			count_temp = rcvCount;
			fRcvUSART1 = false;
			uart_timeout = 0;	
//			TIM_Cmd(TIM6,DISABLE);
			// Decrypt data
			if(rcvData[0] != ETHERNET_TAG)
			{				
				if(networkStatus == NETWORK_GPRS)
				{					
					ptr = (u8*)strstr(rcvData,"+RECEIVE");
					if(ptr!=NULL)
					{
						while(*ptr != 0x0A)
							ptr++;
						ptr++;
						index = ptr - rcvData;	
						for(i = index;i<rcvCount;i++)
						{
							rcvData[i] ^= 0xAB;
						}		
						/*
						crc = ComputeChecksum(&rcvData[index],rcvCount-index-1);
						if(rcvData[rcvCount - 1] != crc)
						{
							sendResponse(CMD_INVALID,_ERROR);
						}
						else
							msgControl(&rcvData[index],rcvCount-index);	
						*/
						msgControl(&rcvData[index],rcvCount-index);	
					}			
				}
				else if(networkStatus == NETWORK_ETHERNET)
				{			
					for(i = 0;i<rcvCount;i++)
					{
						rcvData[i] ^= 0xAB;
					}	
					/*
					crc = ComputeChecksum(rcvData,rcvCount-1);
					if(rcvData[rcvCount - 1] != crc)
					{
						sendResponse(CMD_INVALID,_ERROR);
					}
					else
						msgControl(rcvData,rcvCount);	
					*/
					msgControl(rcvData,rcvCount);	
				}				
			}
			else
				EthMsgControl(rcvData);
			
			if(fSendRes)
			{				
				fSendRes = false;
				sendResponse(cmd,res_value);
				cmd = CMD_INVALID;
				res_value = _SUCCESS;				
			}					
			// Clear global variables							
			for(i = 0;i<count_temp;i++)
				rcvData[i] = 0;
			if(rcvCount > count_temp)
			{
				index = rcvCount-count_temp;
				for(i = 0;i<index;i++)
				{
					rcvData[i] = rcvData[count_temp+i];				
					rcvData[count_temp+i] = 0;
				}
			}
			rcvCount -=count_temp;	
		}		
	}
	TIM_ClearFlag(TIM6,TIM_FLAG_Update);
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
