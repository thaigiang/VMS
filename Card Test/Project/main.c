/***************************************************************************
 * STM32 VGA demo
 * Copyright (C) 2012 Artekit Italy
 * http://www.artekit.eu
 * Written by Ruben H. Meleca
 
### main.c
 
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

***************************************************************************/
#include <math.h>
#include "stm32f10x.h"
#include "sys.h"
#include "video.h"
#include "gdi.h"
#include "stm32f10x_conf.h"
#include "HardwareConfig.h"


#define X_START			3
#define Y_START			2


extern u8 fReload;
extern u8 fCheck;
extern u16 dplData[4608];
extern void demoInit(void);
extern u16 tempBuff[3];

u8 getMatrixPixel(u8* buff,u16 w_buff,u16 x,u16 y);
void display(void);
void DPL(void);

void RCC_Configuration(void)
{
	/* TIM1, GPIOA, GPIOB, GPIOE and AFIO clocks enable */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1 | RCC_AHBPeriph_DMA2, ENABLE);	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA  | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD |
							RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOE |RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM3 | RCC_APB1Periph_SPI2 | RCC_APB1Periph_SPI3, ENABLE);
}

void GPIO_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_WriteBit(GPIOA,GPIO_Pin_7,Bit_RESET);
}

int main(void)
{
	u8 temp = 0;
	u16 i = 0,j = 0;
	
 	RCC_Configuration();
	GPIO_Configuration();
	LED_IO_Config();
//	TIMER6_Configuration();
	memset(dplData,0x00,4608);
//	EXTI_Disable();		
	for(i = 0;i<10000;i++)
		for(j = 0;j<100;j++) {};
	fCheck = 1;
	fReload = 0;		
	EXTI_Enable();
	while(!fReload) 
	{		
	};
	EXTI_Disable();
	DPL();			
	for(i = 0;i<50000;i++)
		for(j = 0;j<1000;j++) {};
	vidInit();
	VGA_INT_Enable();
	while(1) 
	{	
/*	
		fCheck = 1;
		fReload = 0;		
		EXTI_Enable();
		while(!fReload) {};
		EXTI_Disable();
		display();			
		for(i = 0;i<50000;i++)
			for(j = 0;j<1000;j++) {};
*/			
	}
}

void DPL(void)
{
	u16 index_count = 0;
	u8 temp = 0;
	u8 i = 0,line = 0;
	u16 j = 0,k = 0;
	u16 x = 0;
	u16 n = 0;
	u16 step = 0;
	u16 xStep = 0,yStep = 0;

	yStep = 8;
	for(line = 0;line<4;line++)
	{
		temp = (192/4);		
		index_count = line*1152;		
		for(i = 0;i<temp;i+=2)
		{			
			xStep = i*8;
			//sendData(&dplData[index_count]);	
			for(n = 0;n<8;n++)
			{				
				for(x = 0;x<3;x++)
				{
					for(k = 0;k<6;k++)
					{
						if(dplData[index_count+x]&(0x0001<<(k*2)))
						{
							gdiPoint(8 + xStep,Y_START + yStep*k + x*48 + line,0,RED_COLOR);
							gdiPoint(8 + xStep+1,Y_START + yStep*k + x*48+ line,0,RED_COLOR);
						}						
						if(dplData[index_count+x] & (0x0002<<(k*2)))
						{
							gdiPoint(10 + xStep,Y_START + yStep*k + x*48+ line,0,GREEN_COLOR);
							gdiPoint(10 + xStep+1,Y_START + yStep*k + x*48+ line,0,GREEN_COLOR);
						}												
					}
				}
				index_count +=3;
				xStep += 2;
			}	
			xStep = i*8;
			for(n = 0;n<8;n++)
			{				
				for(x = 0;x<3;x++)
				{
					for(k = 0;k<6;k++)
					{
						if(dplData[index_count+x]&(0x0001<<(k*2)))
						{
							gdiPoint(8 + xStep,Y_START + yStep*k+4 + x*48+ line,0,RED_COLOR);
							gdiPoint(8 + xStep+1,Y_START + yStep*k+4 + x*48+ line,0,RED_COLOR);
						}
						if(dplData[index_count+x] & (0x0002<<(k*2)))
						{
							gdiPoint(10 + xStep,Y_START + yStep*k+4 + x*48+ line,0,GREEN_COLOR);
							gdiPoint(10 + xStep+1,Y_START + yStep*k+4 + x*48+ line,0,GREEN_COLOR);
						}		
					}
				}
				index_count +=3;
				xStep += 2;
			}
		}
	}
}

void display(void)
{
	u16 i = 0,j = 0,k = 0;
	u16 x = 0;
	u16 n = 0;
	u16 step = 0;
	u16 xStep = 0,yStep = 0;
	u16 count = 0;

	yStep = 8;	
	for(i = 0;i<4;i++)
	{		
		xStep = 0;
		for(j = 0;j<1152;j+=48)
		{			
			for(n = 0;n<8;n++)
			{				
				for(x = 0;x<3;x++)
				{
					for(k = 0;k<6;k++)
					{
						if(dplData[count+x]&(0x0001<<(k*2)))
						{
							gdiPoint(8 + xStep,Y_START + yStep*k + x*48 + i,0,RED_COLOR);
							gdiPoint(8 + xStep+1,Y_START + yStep*k + x*48+ i,0,RED_COLOR);
						}
						/*
						if(dplData[count+x] & (0x0002<<(k*2)))
						{
							gdiPoint(10 + xStep,Y_START + yStep*k + x*48+ i,0,GREEN_COLOR);
							gdiPoint(10 + xStep+1,Y_START + yStep*k + x*48+ i,0,GREEN_COLOR);
						}
						*/
						if(dplData[count+x+24]&(0x0001<<(k*2)))
						{
							gdiPoint(8 + xStep,Y_START + yStep*k+4 + x*48+ i,0,RED_COLOR);
							gdiPoint(8 + xStep+1,Y_START + yStep*k+4 + x*48+ i,0,RED_COLOR);
						}
						/*
						if(dplData[count+x+24] & (0x0002<<(k*2)))
						{
							gdiPoint(10 + xStep,Y_START + yStep*k+4 + x*48+ i,0,GREEN_COLOR);
							gdiPoint(10 + xStep+1,Y_START + yStep*k+4 + x*48+ i,0,GREEN_COLOR);
						}		
						*/
					}
				}
				count +=3;
				xStep += 2;
			}
			count +=24;							
		}	
	}
}

//========================================
u8 getMatrixPixel(u8* buff,u16 w_buff,u16 x,u16 y)
{
	u16 index = 0;
	u8 mod = 0;
	u8 value = 0;
	u16 temp = 0;

	temp = (x + y*w_buff);
	index = temp/4;

	mod = (temp%4)*2;
	value = (buff[index]>>mod)&0x03;

	return value;
}

