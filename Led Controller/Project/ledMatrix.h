#ifndef __LED_H__
#define __LED_H__

#include "stm32f10x.h"

//=========================================
#define USING_LOAD_CONFIG		1

#define DEFAULT_LED_WIDTH		192 //192//256
#define DEFAULT_LED_HEIGHT		144 //96//128

typedef enum
{
	SCAN_STATIC = 1,
	SCAN_1_4_TYPE1,
	SCAN_1_4_TYPE2,
	SCAN_1_8,
} LED_SCAN_TYPE;

#define MIN_SCAN_TYPE	SCAN_STATIC
#define MAX_SCAN_TYPE	SCAN_1_8

// Define LED information
//#define LED_WIDTH		192 //192	//256		// Width of LED Matrix
//#define LED_HEIGHT		8								// Height of 1 LED
//#define LED_LINE_NUM	18	//8 //6	
#define LED_DATA_LENGTH	(DEFAULT_LED_WIDTH*DEFAULT_LED_HEIGHT/4)			//2048

//#define stopLed()		{LED_OE_PORT->BSRR = LED_OE_PIN;}

#define LED_DR_MASK		0x5555
#define LED_DG_MASK		0xAAAA


#ifndef DISPLAY_LED_STATIC
#define DISPLAY_LED_STATIC	0
#endif
#ifndef DISPLAY_LED_1x4
#define DISPLAY_LED_1x4		1
#endif

// LED IO pins
// Define I/Os used to control LED matrix
#define LED_CLK_PORT	GPIOA
#define LED_CLK_PIN		GPIO_Pin_12

#define LED_LAT_PORT	GPIOA
#define LED_LAT_PIN		GPIO_Pin_15

#define LED_OE_PORT		GPIOA
#define LED_OE_PIN		GPIO_Pin_11

#if DISPLAY_LED_1x4
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

#endif

//=========================================
// Functions prototype
void selectRow(u8 rowNum);
void sendData(u16* data);
void display(u8* data);
void convertData(u8* ledData);
void shiftLeft(void);

#endif
 
