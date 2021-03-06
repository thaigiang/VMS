/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "HardwareConfig.h"
#include "updateFW.h"


u32 timingCount = 0;
/* Private functions ---------------------------------------------------------*/

/**
  * @brief   Main program
  * @param  None
  * @retval None
  */
int main(void)
{	
	RCC_Configuration();
	GPIO_Configuration();
	USART_Configuration();	
	SysTick_Config(SystemCoreClock/10);
	// Enable the LSI OSC 
 	RCC_LSICmd(ENABLE);
	// Wait till LSI is ready 
	while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET) {};
	
 	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
	// IWDG counter clock: LSI/256 
	IWDG_SetPrescaler(IWDG_Prescaler_256);
	IWDG_SetReload(0x0FFF);
	// Reload IWDG counter 
	IWDG_ReloadCounter();
	// Enable IWDG (the LSI oscillator will be enabled by hardware) 
	IWDG_Enable(); 
	
	// Write memmory
	FLASH_UnlockBank1();
	FLASH_ErasePage(FLAG_ADDR);
	FLASH_ProgramWord(FLAG_ADDR,(u32)FLAG_UPDATED);
	FLASH_LockBank1();
	
	updateFW_control();
}
//========================================
#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}

#endif

/**
  * @}
  */ 

/**
  * @}
  */ 

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
