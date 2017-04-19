#ifndef __HARDWARE_CONFIG_H_
#define __HARDWARE_CONFIG_H_

// IO Pins definition
#define RTS_IO_PORT		GPIOC
#define RTS_IO_PIN		GPIO_Pin_9

#define SDC_CS_PORT		GPIOA
#define SDC_CS_PIN		GPIO_Pin_0

// Ethernet reset pin
#define ETHERNET_RESET_PORT		GPIOB
#define ETHERNET_RESET_PIN		GPIO_Pin_9

// SIM900 Power Pin
#define SIM_PWRKEY_PORT		GPIOA
#define SIM_PWRKEY_PIN		GPIO_Pin_4

#define DS1307_OUT_PORT		GPIOC
#define DS1307_OUT_PIN		GPIO_Pin_1

// AT45DB control pins
#define AT45DB_CS_PORT		GPIOB
#define AT45DB_CS_PIN			GPIO_Pin_8

#define AT45DB_RST_PORT		GPIOB
#define AT45DB_RST_PIN		GPIO_Pin_9

#define CHARGE_PORT			GPIOC
#define CHARGE_PIN			GPIO_Pin_2

#define ACC_PORT			GPIOE
#define ACC_PIN				GPIO_Pin_9

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
void USART1_Configuration(void);
void USART2_Configuration(void);
void USART4_Configuration(void);
void NVIC_Configuration(void);
void NVIC_DeInit(void);
void ADC_Configuration(void);
void I2C_Configuration(void);
void SPI_Configuration(void);
void EXTI_Configuration(void);
void TIMER_Configuration(void);

#endif
