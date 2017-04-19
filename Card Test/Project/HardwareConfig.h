#ifndef __HARDWARE_CONFIG_H_
#define __HARDWARE_CONFIG_H_

// IO Pins definition
#define LAT_IO_PORT		GPIOA
#define LAT_IO_PIN		GPIO_Pin_15
#define CLK_IO_PORT		GPIOA
#define CLK_IO_PIN		GPIO_Pin_12
#define OE_IO_PORT		GPIOA
#define OE_IO_PIN		GPIO_Pin_8
#define OE_IO_PORT		GPIOA
#define OE_IO_PIN		GPIO_Pin_8
#define OE_IO_PORT		GPIOA
#define OE_IO_PIN		GPIO_Pin_8

#define LED_SELECT_PORT1	GPIOC
#define LED_SA_PIN1			GPIO_Pin_14
#define LED_SB_PIN1			GPIO_Pin_15

#define LED_SELECT_PORT2	GPIOE
#define LED_SA_PIN2			GPIO_Pin_6
#define LED_SB_PIN2			GPIO_Pin_7

#define LED_SELECT_PORT3	GPIOB
#define LED_SA_PIN3			GPIO_Pin_6
#define LED_SB_PIN3			GPIO_Pin_7

// Function prototypes
void LED_IO_Config(void);
void EXTI_Configuration(void);
void NVIC_Configuration(void);
void EXTI_Disable(void);
void EXTI_Enable(void);
void VGA_INT_Enable(void);
void VGA_INT_Disable(void);
void TIMER6_Configuration(void);

#endif
