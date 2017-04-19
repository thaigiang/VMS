
#ifndef _DS18B20_H_
#define _DS18B20_H_

//********************************************


#define GPIO_PORT       				GPIOB
#define GPIO_PIN_DS    				GPIO_Pin_0
#define RCC_APB2Periph_GPIO_PORT    RCC_APB2Periph_GPIOB


void delayus(u16 nCount);
void DS18B20_GPIO_In(void);
void DS18B20_GPIO_Out(void);
int Init_DS18B20(void);
void WriteOneChar(u8 dat);
int ReadOneChar(void);
unsigned char Read_Temperature(void) ;

//********************************************

#endif

