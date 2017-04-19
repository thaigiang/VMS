#include <string.h>
#include "stm32f10x_gpio.h"
#include "ledMatrix.h"
#include "HardwareConfig.h"
#include "common.h"

u16 dplData[4608];
u16 dplDataTemp[4608];
u8 line_count = 0;

extern u32 ledDelay;
extern HW_CONFIG hwConfig;
extern bool fReload;


//=================================
static __inline void getData(u16* dst,u16* data)
{
	if((hwConfig.ledType == SCAN_1_4_TYPE1)||(hwConfig.ledType == SCAN_1_4_TYPE2))
	{
		*dst = (data[0]&0x03)|((data[1]&0x03)<<2)|((data[2]&0x03)<<4)
			   |((data[3]&0x03)<<6)|((data[4]&0x03)<<8)|((data[5]&0x03)<<10);
		data[0] = data[0]>>2;
		data[1] = data[1]>>2;
		data[2] = data[2]>>2;
		data[3] = data[3]>>2;
		data[4] = data[4]>>2;
		data[5] = data[5]>>2;
	}
	else if(hwConfig.ledType == SCAN_1_8)
	{
		*dst = (data[0]&0x03)|((data[1]&0x03)<<2)|((data[2]&0x03)<<4)
			   |((data[3]&0x03)<<6)|((data[4]&0x03)<<8)|((data[5]&0x03)<<10);
		data[0] = data[0]>>2;
		data[1] = data[1]>>2;
		data[2] = data[2]>>2;
		data[3] = data[3]>>2;
		data[4] = data[4]>>2;
		data[5] = data[5]>>2;
	}
	else if(hwConfig.ledType == SCAN_STATIC)
	{
		*dst = (data[0]&0x03)|((data[1]&0x03)<<2)|((data[2]&0x03)<<4)|((data[3]&0x03)<<6)
					|((data[4]&0x03)<<8)|((data[5]&0x03)<<10)|((data[6]&0x03)<<12)|((data[7]&0x03)<<14)
					|((data[8]&0x03)<<16)|((data[9]&0x03)<<18)|((data[10]&0x03)<<20)|((data[11]&0x03)<<22);
		data[0] = data[0]>>2;
		data[1] = data[1]>>2;
		data[2] = data[2]>>2;
		data[3] = data[3]>>2;
		data[4] = data[4]>>2;
		data[5] = data[5]>>2;
		data[6] = data[6]>>2;
		data[7] = data[7]>>2;
		data[8] = data[8]>>2;
		data[9] = data[9]>>2;
		data[10] = data[10]>>2;
		data[11] = data[11]>>2;
	}
}

//=================================
void convertData(u8* ledData)
{
	u16 index = 0;
	u8 i = 0,n = 0,k = 0;
	s8 j = 0;
	u32 indexStep = 0;
	u16 count_temp = 2;
	u16 lineData[18];
	u16 led_line_num = 0;
	u8 temp = 0;
	u16 idx = 0;
	u8 line = 0;
	u16 index_count = 0;
	
	if(hwConfig.ledType == SCAN_1_8)
	{
		led_line_num = 6;
		indexStep = hwConfig.ledWidth/4;	
		index = (hwConfig.ledWidth)/4;
		temp = (hwConfig.ledWidth/4);
		
		for(line = 0; line < 8; line ++)
		{					
			for(i = 0; i < temp; i += 2)
			{				
				for(n = 0; n < led_line_num; n++)
				{				
					idx = (n%2+(n/2)*16)*indexStep+index*line*2+i; 
					memcpy(&lineData[n],&ledData[idx],2);
					idx += 0x0900;
					memcpy(&lineData[n+6],&ledData[idx],2);
					idx += 0x0900;
					memcpy(&lineData[n+12],&ledData[idx],2);
				}	
				for(k = 0; k < 8; k++)
				{		
					getData(&dplDataTemp[index_count++],&lineData[0]);
					getData(&dplDataTemp[index_count++],&lineData[6]);
					getData(&dplDataTemp[index_count++],&lineData[12]);
				}
			}
		}
	}
	else
	{		
		led_line_num = 6;
		indexStep = 8*(hwConfig.ledWidth/4);	
		index = (hwConfig.ledWidth)/4;
		temp = (hwConfig.ledWidth/4);			
		for(line = 0;line<4;line ++)
		{
			// Send data to LED
			if(hwConfig.ledType == SCAN_1_4_TYPE2)
			{
				for(i = 0;i<temp;i+=2)
				{
					for(j = 0;j<count_temp;j++)
					{					
						for(n = 0;n<led_line_num;n++)
						{				
							idx = n*indexStep+index*(j*4+line)+i; 
							memcpy(&lineData[n],&ledData[idx],2);
							idx += 0x0900;				
							memcpy(&lineData[n+6],&ledData[idx],2);
							idx += 0x0900;				
							memcpy(&lineData[n+12],&ledData[idx],2);
						}	
						for(k = 0;k<8;k++)
						{		
							getData(&dplDataTemp[index_count++],&lineData[0]);
							getData(&dplDataTemp[index_count++],&lineData[6]);
							getData(&dplDataTemp[index_count++],&lineData[12]);
						}
					}
				}
			}
			else if(hwConfig.ledType == SCAN_1_4_TYPE1)
			{
				for(i = 0;i<temp;i+=2)
				{
					for(j = 1;j>=0;j--)
					{					
						for(n = 0;n<led_line_num;n++)
						{				
							idx = n*indexStep+index*(j*4+line)+i; 
							memcpy(&lineData[n],&ledData[idx],2);
							idx += 0x0900;				
							memcpy(&lineData[n+6],&ledData[idx],2);
							idx += 0x0900;				
							memcpy(&lineData[n+12],&ledData[idx],2);
						}	
						for(k = 0;k<8;k++)
						{		
							getData(&dplDataTemp[index_count++],&lineData[0]);
							getData(&dplDataTemp[index_count++],&lineData[6]);
							getData(&dplDataTemp[index_count++],&lineData[12]);
						}
					}
				}
			}	
		}
	}
}

//=================================
void OE_Control(bool oeState)
{
	if((hwConfig.ledType == SCAN_1_4_TYPE1)||(hwConfig.ledType == SCAN_1_4_TYPE2))
	{
		if(!oeState)
		{
			LED_OE_PORT1->BSRR = LED_OE_PIN11;
			LED_OE_PORT1->BSRR = LED_OE_PIN12;
			LED_OE_PORT2->BSRR = LED_OE_PIN21;
			LED_OE_PORT2->BSRR = LED_OE_PIN22;
			LED_OE_PORT3->BSRR = LED_OE_PIN31;
			LED_OE_PORT3->BSRR = LED_OE_PIN32;
		}
		else
		{
			LED_OE_PORT1->BRR = LED_OE_PIN11;
			LED_OE_PORT1->BRR = LED_OE_PIN12;
			LED_OE_PORT2->BRR = LED_OE_PIN21;
			LED_OE_PORT2->BRR = LED_OE_PIN22;
			LED_OE_PORT3->BRR = LED_OE_PIN31;
			LED_OE_PORT3->BRR = LED_OE_PIN32;
		}
	}
	else if(hwConfig.ledType == SCAN_1_8)
	{	
		if(!oeState)
		{
			LED_OE_PORT1->BSRR = LED_OE_PIN12;
			LED_OE_PORT2->BSRR = LED_OE_PIN22;
			LED_OE_PORT3->BSRR = LED_OE_PIN32;
		}
		else
		{
			LED_OE_PORT1->BRR = LED_OE_PIN12;
			LED_OE_PORT2->BRR = LED_OE_PIN22;
			LED_OE_PORT3->BRR = LED_OE_PIN32;
		}	
	}
}

//=================================
static __inline void selectRow(u8 rowNum)
{
	if(hwConfig.ledType == SCAN_1_8)
	{
		if(rowNum & 0x04)
		{
			LED_OE_PORT1->BSRR = LED_SC_PIN1;		
			LED_OE_PORT2->BSRR = LED_SC_PIN2;	
			LED_OE_PORT3->BSRR = LED_SC_PIN3;	
		}
		else
		{
			LED_OE_PORT1->BRR = LED_SC_PIN1;
			LED_OE_PORT2->BRR = LED_SC_PIN2;
			LED_OE_PORT3->BRR = LED_SC_PIN3;
		}
		if(rowNum & 0x02)
		{
			LED_SELECT_PORT1->BSRR = LED_SB_PIN1;
			LED_SELECT_PORT2->BSRR = LED_SB_PIN2;
			LED_SELECT_PORT3->BSRR = LED_SB_PIN3;		
		}
		else
		{
			LED_SELECT_PORT1->BRR = LED_SB_PIN1;
			LED_SELECT_PORT2->BRR = LED_SB_PIN2;
			LED_SELECT_PORT3->BRR = LED_SB_PIN3;
		}
		if(rowNum & 0x01)
		{
			LED_SELECT_PORT1->BSRR = LED_SA_PIN1;
			LED_SELECT_PORT2->BSRR = LED_SA_PIN2;
			LED_SELECT_PORT3->BSRR = LED_SA_PIN3;		
		}
		else
		{
			LED_SELECT_PORT1->BRR = LED_SA_PIN1;
			LED_SELECT_PORT2->BRR = LED_SA_PIN2;
			LED_SELECT_PORT3->BRR = LED_SA_PIN3;
		}
	}
	else
	{
		if(rowNum & 0x02)
		{
			LED_SELECT_PORT1->BSRR = LED_SB_PIN1;
			LED_SELECT_PORT2->BSRR = LED_SB_PIN2;
			LED_SELECT_PORT3->BSRR = LED_SB_PIN3;		
		}
		else
		{
			LED_SELECT_PORT1->BRR = LED_SB_PIN1;
			LED_SELECT_PORT2->BRR = LED_SB_PIN2;
			LED_SELECT_PORT3->BRR = LED_SB_PIN3;
		}
		if(rowNum & 0x01)
		{
			LED_SELECT_PORT1->BSRR = LED_SA_PIN1;
			LED_SELECT_PORT2->BSRR = LED_SA_PIN2;
			LED_SELECT_PORT3->BSRR = LED_SA_PIN3;		
		}
		else
		{
			LED_SELECT_PORT1->BRR = LED_SA_PIN1;
			LED_SELECT_PORT2->BRR = LED_SA_PIN2;
			LED_SELECT_PORT3->BRR = LED_SA_PIN3;
		}
	}
}

//=================================
static __inline void sendData(u16* data)
{
	u8 i = 0;
	volatile u16 portA,portB,portC,portD,portE;
	
	if((hwConfig.ledType == SCAN_1_4_TYPE1)||(hwConfig.ledType == SCAN_1_4_TYPE2)||(hwConfig.ledType == SCAN_1_8))
	{
		portA = (u16)(GPIOA->ODR & 0xFF00);
		portB = (u16)(GPIOB->ODR & 0xFFF0);
		portC = (u16)(GPIOC->ODR & 0xF000);
		portD = (u16)(GPIOD->ODR & 0x00FF);
		portE = (u16)(GPIOE->ODR & 0xFFF0);
		
		GPIOC->ODR = (portC|data[0]); 	
		GPIOD->ODR = (portD|((data[1]<<8)&0xFF00));
		GPIOE->ODR = (portE|(data[1]>>8));	
		GPIOA->ODR = (portA|(data[2]&0x00FF));
		GPIOB->ODR = (portB|(data[2]>>8));	
		
		// Latch clock
		LED_CLK_PORT->BSRR = LED_CLK_PIN; 
		LED_CLK_PORT->BRR = LED_CLK_PIN; 
		
		GPIOC->ODR = (portC|data[3]); 	
		GPIOD->ODR = (portD|((data[4]<<8)&0xFF00));
		GPIOE->ODR = (portE|(data[4]>>8));	
		GPIOA->ODR = (portA|(data[5]&0x00FF));
		GPIOB->ODR = (portB|(data[5]>>8));
		
		// Latch clock
		LED_CLK_PORT->BSRR = LED_CLK_PIN; 
		LED_CLK_PORT->BRR = LED_CLK_PIN; 

		GPIOC->ODR = (portC|data[6]); 	
		GPIOD->ODR = (portD|((data[7]<<8)&0xFF00));
		GPIOE->ODR = (portE|(data[7]>>8));	
		GPIOA->ODR = (portA|(data[8]&0x00FF));
		GPIOB->ODR = (portB|(data[8]>>8));	
		
		// Latch clock
		LED_CLK_PORT->BSRR = LED_CLK_PIN; 
		LED_CLK_PORT->BRR = LED_CLK_PIN; 

		GPIOC->ODR = (portC|data[9]); 	
		GPIOD->ODR = (portD|((data[10]<<8)&0xFF00));
		GPIOE->ODR = (portE|(data[10]>>8));	
		GPIOA->ODR = (portA|(data[11]&0x00FF));
		GPIOB->ODR = (portB|(data[11]>>8));	
		
		// Latch clock
		LED_CLK_PORT->BSRR = LED_CLK_PIN; 
		LED_CLK_PORT->BRR = LED_CLK_PIN; 

		GPIOC->ODR = (portC|data[12]); 	
		GPIOD->ODR = (portD|((data[13]<<8)&0xFF00));
		GPIOE->ODR = (portE|(data[13]>>8));	
		GPIOA->ODR = (portA|(data[14]&0x00FF));
		GPIOB->ODR = (portB|(data[14]>>8));
		
		// Latch clock
		LED_CLK_PORT->BSRR = LED_CLK_PIN; 
		LED_CLK_PORT->BRR = LED_CLK_PIN; 

		GPIOC->ODR = (portC|data[15]); 	
		GPIOD->ODR = (portD|((data[16]<<8)&0xFF00));
		GPIOE->ODR = (portE|(data[16]>>8));	
		GPIOA->ODR = (portA|(data[17]&0x00FF));
		GPIOB->ODR = (portB|(data[17]>>8));	
		
		// Latch clock
		LED_CLK_PORT->BSRR = LED_CLK_PIN; 
		LED_CLK_PORT->BRR = LED_CLK_PIN; 

		GPIOC->ODR = (portC|data[18]); 	
		GPIOD->ODR = (portD|((data[19]<<8)&0xFF00));
		GPIOE->ODR = (portE|(data[19]>>8));	
		GPIOA->ODR = (portA|(data[20]&0x00FF));
		GPIOB->ODR = (portB|(data[20]>>8));	
		
		// Latch clock
		LED_CLK_PORT->BSRR = LED_CLK_PIN; 
		LED_CLK_PORT->BRR = LED_CLK_PIN; 
		
		GPIOC->ODR = (portC|data[21]);	
		GPIOD->ODR = (portD|((data[22]<<8)&0xFF00));
		GPIOE->ODR = (portE|(data[22]>>8));	
		GPIOA->ODR = (portA|(data[23]&0x00FF));
		GPIOB->ODR = (portB|(data[23]>>8));		
		// Latch clock
		LED_CLK_PORT->BSRR = LED_CLK_PIN; 
		LED_CLK_PORT->BRR = LED_CLK_PIN; 
	}
	else if(hwConfig.ledType == SCAN_STATIC)
	{
		for(i = 0;i<8;i++)
		{		
			// Send data to Red Line
			GPIOC->ODR &= ~(0xFFFF);
			GPIOC->ODR |= (data[0]^0xFFFF);		
			GPIOD->ODR &= ~(0xFFFF);
			GPIOD->ODR |= (((data[0]>>16)|(data[1]<<8))^0xFFFF);
			GPIOE->ODR &= ~(0xFFFF);
			GPIOE->ODR |= ((data[1]>>8)^0xFFFF);	
			GPIOA->ODR &= 0xFF00;
			GPIOA->ODR |= ((data[2]&0x00FF)^0x00FF);
			GPIOB->ODR &= ~(0xFFFF);
			GPIOB->ODR |= ((data[2]>>8)^0xFFFF);	
			
			// Latch clock
			LED_CLK_PORT->BSRR = LED_CLK_PIN; 
			LED_CLK_PORT->BRR = LED_CLK_PIN; 
		}
	}
}

//=================================
void display(u8* data)
{
	u16 index = 0;
	u8 i = 0,n = 0;
	s8 j = 0;
	u16 lineData[36];
	u32 indexStep = 0;
	u16 count_temp = 2;
	u16 led_line_num = 0;
	u16 index_count = 0;
	u8 temp = 0;
	u16 idx = 0;
	u32 delay = 0;

	if(fReload)
	{
		memcpy(dplData,dplDataTemp,4608*2);
		OE_Control(false);		
		fReload = false;
	}
	if(hwConfig.ledType == SCAN_1_8)
	{
		temp = hwConfig.ledWidth;		
		index_count = line_count*576;
		// Shift data to LED
		for(i = 0;i<temp;i+=8)
		{					
			if(delay == ledDelay)
				OE_Control(false);
			else
				delay +=2;	
			
			sendData(&dplData[index_count]);	
			index_count += 24;										
		}
		
		// Turn-off LEDs
		OE_Control(false);
		// Latch data
		LED_LAT_PORT->BSRR = LED_LAT_PIN; 
		LED_LAT_PORT->BRR = LED_LAT_PIN; 	
		// Select LED's row to turn-on
		selectRow(line_count);	
		line_count ++;
		if(line_count == 8)
		{
			line_count = 0;
		}		
		// Turn-on LEDs
		OE_Control(true);		
	}
	else if((hwConfig.ledType == SCAN_1_4_TYPE1)||(hwConfig.ledType == SCAN_1_4_TYPE2))
	{	
		temp = (hwConfig.ledWidth/4);		
		index_count = line_count*1152;
		// Shift data to LED
		if(hwConfig.ledType == SCAN_1_4_TYPE2)
		{
			for(i = 0;i<temp;i+=2)
			{					
				if(delay == ledDelay)
					OE_Control(false);
				else
					delay +=2;										
				sendData(&dplData[index_count]);	
				index_count += 24;										
				sendData(&dplData[index_count]);	
				index_count += 24;
			}
		}
		else if(hwConfig.ledType == SCAN_1_4_TYPE1)
		{
			for(i = 0;i<temp;i+=2)
			{				
				if(delay == ledDelay)
					OE_Control(false);
				else
					delay +=2;
				sendData(&dplData[index_count]);	
				index_count += 24;	
				sendData(&dplData[index_count]);	
				index_count += 24;	
			}
		}		
		// Turn-off LEDs
		OE_Control(false);
		// Latch data
		LED_LAT_PORT->BSRR = LED_LAT_PIN; 
		LED_LAT_PORT->BRR = LED_LAT_PIN; 	
		// Select LED's row to turn-on
		selectRow(line_count);	
		line_count ++;
		if(line_count == 4)
		{
			line_count = 0;
		}		
		// Turn-on LEDs
		OE_Control(true);		
	}
	else if(hwConfig.ledType == SCAN_STATIC)
	{
		led_line_num = 12;
		count_temp = 2;
		indexStep = 4*(hwConfig.ledWidth/4);	
		index = (hwConfig.ledWidth/4);
		temp = (hwConfig.ledWidth/4);
		
		// Send data to LED
		for(i = 0;i<temp;i++)
		{
			for(j = 0;j<count_temp;j++)
			{
				for(n = 0;n<led_line_num;n++)
				{
					idx = n*indexStep+index*j*2+i;				
					lineData[n] = data[idx+index];
					lineData[n] = (lineData[n]<<8|data[idx]);
					idx += 0x0900;				
					lineData[n+12] = data[idx+index];
					lineData[n+12] = (lineData[n+12]<<8|data[idx]);
					idx += 0x0900;				
					lineData[n+24] = data[idx+index];
					lineData[n+24] = (lineData[n+24]<<8|data[idx]);
				}
				sendData(lineData);
			}
		}
		// Latch data
		LED_LAT_PORT->BSRR = LED_LAT_PIN; 
		LED_LAT_PORT->BRR = LED_LAT_PIN; 
	}
}

