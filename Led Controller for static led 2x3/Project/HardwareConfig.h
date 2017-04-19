#ifndef __HARDWARE_CONFIG_H_
#define __HARDWARE_CONFIG_H_

//====================================
// Functions prototype
void RCC_Configuration(void);
void GPIO_Configuration(void);
void ADC_Configuration(void);
void NVIC_Configuration(void);
void USART_Configuration(void);
void TIMER_Configuration(void);
void EXTI_Configuration(void);
void NVIC_DeInit(void);

#endif
