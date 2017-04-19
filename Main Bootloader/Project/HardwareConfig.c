#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "HardwareConfig.h"

/**
  * @brief  Configures the different system clocks.
  * @param  None
  * @retval None
  */

//========================================
void RCC_Configuration(void)
{     
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
}

//========================================
void GPIO_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	// Configure SPI1 pins
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);	 

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);	

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);	 
	
	// Configure SD Card CS Pin
	GPIO_InitStructure.GPIO_Pin = SDC_CS_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(SDC_CS_PORT, &GPIO_InitStructure);	 

	// Config LED status pins
	GPIO_InitStructure.GPIO_Pin = LED_1_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = LED_1_PIN;
	GPIO_Init(LED_1_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(LED_1_PORT,LED_1_PIN,Bit_RESET);
	GPIO_InitStructure.GPIO_Pin = LED_2_PIN;
	GPIO_Init(LED_2_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(LED_2_PORT,LED_2_PIN,Bit_RESET);
	GPIO_InitStructure.GPIO_Pin = LED_3_PIN;
	GPIO_Init(LED_3_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(LED_3_PORT,LED_3_PIN,Bit_RESET);
}

//========================================
void SPI_Configuration(void)
{
	SPI_InitTypeDef SPI_InitStruct;

	SPI_StructInit(&SPI_InitStruct);
	SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
	SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
	SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;
	SPI_NSSInternalSoftwareConfig(SPI1,SPI_NSSInternalSoft_Set);

	SPI_Init(SPI1,&SPI_InitStruct);

	SPI_Cmd(SPI1, ENABLE);
}

