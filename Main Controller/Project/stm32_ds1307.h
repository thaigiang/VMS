
#ifndef __STM32_DS1307_H
#define __STM32_DS1307_H

#ifdef __cplusplus
 extern "C" {
#endif

// Define DS1307 Register addr
#define DS1307_SECONDS	0x00		// addr = 00h
#define DS1307_MINUTES	0x01
#define DS1307_HOURS	0x02
#define DS1307_DAY		0x03
#define DS1307_DATE		0x04
#define DS1307_MONTH	0x05
#define DS1307_YEAR		0x06
#define DS1307_CONTROL	0x07

// Define Control register
#define DS1307_OUT	0x80
#define DS1307_SWE	0x10
#define DS1307_RS1	0x02
#define DS1307_RS0	0x01

#define DS1307_OUTCLK_1Hz 		DS1307_SWE
#define DS1307_OUTCLK_None0		0x00&(~DS1307_SWE)
#define DS1307_OUTCLK_None1 	0x00&(~DS1307_SWE)|DS1307_OUT

#define DS1307_ID 		0xD0	
#define BUS_ERROR 		1
#define BUS_OK			0
#define TIME_OUT		100000

typedef union tagREALTIME {
	struct
	{
	  char Second;
	  char Minute;
	  char Hour;
	  char Day;
	  char Date;
	  char Month;
	  char Year;
	} data;
	struct
	{
		char b[7];
	} byte;
} tREALTIME;

void I2C_config(void) ;
u8 DS1307_Read(u8 addr, u8 *poi_data, u8 num_byte);
u8 DS1307_Write(unsigned char address,unsigned char data_wr);
void DS1307_Int(void);
void DS1307_SetTime(tREALTIME rtX);
void DS1307_SetYear(char cYear);
void DS1307_SetMonth(char cMonth);
void DS1307_SetDate(char cDate);
void DS1307_SetHour(char cHour);
void DS1307_SetMinute(char cMinute);
void DS1307_SetSecond(char cSecond);
tREALTIME DS1307_GetTime(void);
void DS1307_Stop(void);
void DS1307_Start(void);
unsigned char bcd_cvr(unsigned char code_digi);
unsigned char dec_cvr(unsigned char dec_digi);

/* Includes ------------------------------------------------------------------*/


#ifdef __cplusplus
}
#endif

#endif 
