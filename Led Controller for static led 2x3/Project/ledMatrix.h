#ifndef __LED_H__
#define __LED_H__

#include "stm32f10x.h"
#include "common.h"

//=========================================
#define USING_LOAD_CONFIG		1

#define DEFAULT_LED_WIDTH			144 //192//256
#define DEFAULT_LED_HEIGHT		96 //96//128
#define LED_DATA_LENGTH	(DEFAULT_LED_WIDTH*DEFAULT_LED_HEIGHT/4)			//2048

typedef enum
{
	SCAN_STATIC = 1,
	SCAN_1_4_TYPE1,
	SCAN_1_4_TYPE2,
	SCAN_1_8,
} LED_SCAN_TYPE;

#define MIN_SCAN_TYPE	SCAN_STATIC
#define MAX_SCAN_TYPE	SCAN_1_8

//#define stopLed()		{LED_OE_PORT->BSRR = LED_OE_PIN;}

#define LED_DR_MASK		0x5555
#define LED_DG_MASK		0xAAAA


#ifndef DISPLAY_LED_STATIC
#define DISPLAY_LED_STATIC	1
#endif
#ifndef DISPLAY_LED_1x4
#define DISPLAY_LED_1x4		0
#endif



#if DISPLAY_LED_1x4
#define LED_CLK_PORT	GPIOA
#define LED_CLK_PIN		GPIO_Pin_12
#define LED_OE_PORT		GPIOA
#define LED_OE_PIN		GPIO_Pin_11
#define LED_LAT_PORT	GPIOA

#elif DISPLAY_LED_STATIC
#define LED_CLK_PORT	GPIOD
#define LED_CLK_PIN		GPIO_Pin_1
#define LED_OE_PORT		GPIOD
#define LED_OE_PIN		GPIO_Pin_13
#define LED_LAT_PORT	GPIOD

#endif


#define LED_LAT_PIN		GPIO_Pin_15
#define LED_LAT_PIN1	GPIO_Pin_10
#define LED_LAT_PIN2	GPIO_Pin_14
#define LED_LAT_PIN3	GPIO_Pin_11
#define LED_CLK_PIN1	GPIO_Pin_8
#define LED_CLK_PIN2	GPIO_Pin_12
#define LED_CLK_PIN3	GPIO_Pin_9


#define LED_SELECT_PORT1	GPIOC
#define LED_SA_PIN1			GPIO_Pin_14
#define LED_SB_PIN1			GPIO_Pin_15

#define LED_SELECT_PORT2	GPIOE
#define LED_SA_PIN2			GPIO_Pin_6
#define LED_SB_PIN2			GPIO_Pin_7

#define LED_SELECT_PORT3	GPIOB
#define LED_SA_PIN3			GPIO_Pin_6
#define LED_SB_PIN3			GPIO_Pin_7

#define LED_OE_PORT1		GPIOC
#define LED_OE_PIN11		GPIO_Pin_12
#define LED_OE_PIN12		GPIO_Pin_13

#define LED_OE_PORT2		GPIOE
#define LED_OE_PIN21		GPIO_Pin_4
#define LED_OE_PIN22		GPIO_Pin_5

#define LED_OE_PORT3		GPIOB
#define LED_OE_PIN31		GPIO_Pin_4
#define LED_OE_PIN32		GPIO_Pin_5

#define LED_SC_PIN1			GPIO_Pin_12
#define LED_SC_PIN2			GPIO_Pin_4
#define LED_SC_PIN3			GPIO_Pin_4



//=========================================
// Functions prototype
static void selectRow(u8 rowNum);
static void sendData(u16* data);
void display(u8* data);
void convertData(u8* ledData);
void OE_Control(bool oeState);
#endif
 
