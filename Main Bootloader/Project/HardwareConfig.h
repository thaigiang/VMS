#ifndef __HARDWARE_CONFIG_H_
#define __HARDWARE_CONFIG_H_

// IO Pins definition
#define RTS_IO_PORT		GPIOC
#define RTS_IO_PIN		GPIO_Pin_9

#define SDC_CS_PORT		GPIOA
#define SDC_CS_PIN		GPIO_Pin_0

// LED Status
#define LED_1_PORT		GPIOD
#define LED_1_PIN		GPIO_Pin_9
#define LED_2_PORT		GPIOD
#define LED_2_PIN		GPIO_Pin_10
#define LED_3_PORT		GPIOD
#define LED_3_PIN		GPIO_Pin_11

// Function prototypes
void RCC_Configuration(void);
void GPIO_Configuration(void);
void SPI_Configuration(void);

#endif
