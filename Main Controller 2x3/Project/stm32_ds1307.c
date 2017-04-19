#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "stm32_ds1307.h"

//**********************************************************
void I2C_config(void) 
{
	GPIO_InitTypeDef GPIO_InitStructure;
	I2C_InitTypeDef I2C_InitStructure;  

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;  
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	// I2C CONFIGURATION--/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
	// Configure I2C
	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;  //only effects fast mode
	I2C_InitStructure.I2C_OwnAddress1 = 0xA0;
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_InitStructure.I2C_ClockSpeed = 100000; 
	// Initialise the I2C1 peripheral after enabling it 
	I2C_Init(I2C2, &I2C_InitStructure);
	// Enable I2C1 peripheral
	I2C_Cmd(I2C2, ENABLE);  
}

//**********************************************************
u8 DS1307_Read(u8 addr, u8 *poi_data, u8 num_byte)
{
	u32 time_out = TIME_OUT ;

	time_out = TIME_OUT ;
	while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY)) 
	{
		if((time_out--) == 0)return BUS_ERROR ;
	}

	time_out = TIME_OUT ;
	I2C_GenerateSTART(I2C2, ENABLE);
	while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_SB)) // start bit flag
	{
		if((time_out--) == 0)return BUS_ERROR ;
	}

	time_out = TIME_OUT ;		
	I2C_Send7bitAddress(I2C2, DS1307_ID, I2C_Direction_Transmitter);
	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
	{
		if((time_out--) == 0)return BUS_ERROR ;
	}

	time_out = TIME_OUT ;		
	I2C_SendData(I2C2, addr); //continious conversion mode
	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
	{
		if((time_out--) == 0)return BUS_ERROR ;
	}

	time_out = TIME_OUT ;	
	I2C_GenerateSTART(I2C2, ENABLE);
	while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_SB)) // start bit flag
	{
		if((time_out--) == 0)return BUS_ERROR ;
	}

	time_out = TIME_OUT ;
	I2C_Send7bitAddress(I2C2, DS1307_ID, I2C_Direction_Receiver); 
	while(I2C_GetFlagStatus(I2C2, I2C_FLAG_ADDR) == RESET) 
	{
		if((time_out--) == 0)return BUS_ERROR ;
	}

	I2C_AcknowledgeConfig(I2C2, ENABLE);

	(void)I2C2->SR2;

	while (num_byte)
	{
		if(num_byte==1)
		{
			I2C_AcknowledgeConfig(I2C2, DISABLE);	
			(void)I2C2->SR2;
			time_out = TIME_OUT ;
			I2C_GenerateSTOP(I2C2, ENABLE);	
			while(I2C_GetFlagStatus(I2C2, I2C_FLAG_RXNE) == RESET) 
			{
				if((time_out--) == 0)return BUS_ERROR ;
			}
		}
		time_out = TIME_OUT;
		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED)) /* EV7 */
		{
			if((time_out--) == 0)return BUS_ERROR ;
		}

		*poi_data = I2C_ReceiveData(I2C2);
		poi_data++;
		/* Decrement the read bytes counter */
		num_byte--;
	}

	return BUS_OK ;		
}

//*********************************************************
u8 DS1307_Write(unsigned char address,unsigned char data_wr)
{	
	u32 time_out = TIME_OUT ;

	while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY)) 
	{
		if((time_out--) == 0)return BUS_ERROR ;
	}

	time_out = TIME_OUT ;
	I2C_GenerateSTART(I2C2, ENABLE);
	while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_SB)) // start bit flag
	{
		if((time_out--) == 0)return BUS_ERROR ;
	}

	time_out = TIME_OUT ;		
	I2C_Send7bitAddress(I2C2, DS1307_ID, I2C_Direction_Transmitter);
	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
	{
		if((time_out--) == 0)return BUS_ERROR ;
	}

	time_out = TIME_OUT ;
	I2C_SendData(I2C2, address);
	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
	{
		if((time_out--) == 0)return BUS_ERROR ;
	}

	time_out = TIME_OUT ;
	I2C_SendData(I2C2, data_wr); 
	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
	{
		if((time_out--) == 0)return BUS_ERROR ;
	}

	time_out = TIME_OUT ;
	I2C_GenerateSTOP(I2C2, ENABLE);	
	while(I2C_GetFlagStatus(I2C2, I2C_FLAG_STOPF)) // stop bit flag
	{
		if((time_out--) == 0)return BUS_ERROR ;
	}

	return BUS_OK ;	
}

//*********************************************************
void DS1307_Int(void)
{
	DS1307_Write(DS1307_CONTROL,DS1307_SWE) ;
}

//*********************************************************
void DS1307_SetTime(tREALTIME rtX)
{
	DS1307_SetYear(rtX.data.Year);
	DS1307_SetMonth(rtX.data.Month);
	DS1307_SetDate(rtX.data.Date);
	DS1307_SetHour(rtX.data.Hour);
	DS1307_SetMinute(rtX.data.Minute);
	DS1307_SetSecond(rtX.data.Second);
}

//*********************************************************
void DS1307_SetYear(char cYear)
{
	DS1307_Write(DS1307_YEAR,cYear);
}

//*********************************************************
void DS1307_SetMonth(char cMonth)
{
	DS1307_Write(DS1307_MONTH,cMonth);
}

//*********************************************************
void DS1307_SetDate(char cDate)
{
	DS1307_Write(DS1307_DATE,cDate);
}
//*********************************************************
void DS1307_SetHour(char cHour)
{
	DS1307_Write(DS1307_HOURS,cHour);
}
//*********************************************************
void DS1307_SetMinute(char cMinute)
{
	DS1307_Write(DS1307_MINUTES,cMinute);
}
//*********************************************************
void DS1307_SetSecond(char cSecond)
{
	DS1307_Write(DS1307_SECONDS,cSecond);
}
//*********************************************************
tREALTIME DS1307_GetTime(void)
{
	tREALTIME* t;
	u8 data[7];
	DS1307_Read(DS1307_SECONDS,data,7);
	t = (tREALTIME*)data;
	return *t;
}

void DS1307_Stop(void)
{
	DS1307_Write(DS1307_SECONDS, 0x80);	
}

void DS1307_Start(void)
{
	unsigned char ucResults;
	char	cSecond;
	DS1307_Read(DS1307_SECONDS,&ucResults,1);
	cSecond = ucResults&0x7F;
	DS1307_Write(DS1307_SECONDS, cSecond);
}

//*********************************************************
unsigned char bcd_cvr(unsigned char code_digi)
{
	unsigned char temp ;
	temp = (code_digi & 0xF0) >> 4 ;
	temp = temp * 10 +  (code_digi & 0x0F) ;
	return temp ;
}
//*********************************************************
unsigned char dec_cvr(unsigned char dec_digi)
{
	unsigned char temp ; 
	temp = (dec_digi / 10)<< 4 ;
	temp = temp | (dec_digi % 10)  ;
	return temp ;
}

