/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "HardwareConfig.h"
#include "typedef.h"
#include "common.h"
#include "ff.h"
#include "diskio.h"
#include "updateFW.h"

typedef  void (*pFunction)(void);

pFunction Jump_To_Application;
u32 JumpAddress;

FATFS Fatfs;					/* File system object */
FILINFO fno;
DIR Dir;
FIL Fil;

extern bool fUpdateErr;

//===================================
void initBoard(void);
void runMainApp(void);
/* Private functions ---------------------------------------------------------*/
/**
  * @brief   Main program
  * @param  None
  * @retval None
  */

int main(void)
{		
	initBoard();		

	do
	{
		updateFW();
	}while(fUpdateErr);

	runMainApp();
}

//===================================
void initBoard(void)
{			
	RCC_Configuration();					// System Clocks Configuration 	
	GPIO_Configuration();					// Configure the GPIO ports 
	SPI_Configuration();
	
	// Init SD Card
	initSDC();
}

//===================================
void initSDC(void)
{
	FRESULT rc ;
	DRESULT dr;
	DSTATUS dstatus;
	long p;
	
	dstatus = disk_initialize(0);

	if(!dstatus)
	{
		f_mount(0, &Fatfs);	  /* Register volume work area (never fails) */

		dr = disk_ioctl(0, GET_SECTOR_COUNT, &p); 
	}	
}

//===================================
void runMainApp(void)
{
	// Jump to user application 
	JumpAddress = *(__IO u32*) (MAIN_APP_ADDR+ 4);
	Jump_To_Application = (pFunction) JumpAddress;
	// Initialize user application's Stack Pointer 
	__set_MSP(*(__IO u32*) MAIN_APP_ADDR);
	Jump_To_Application();
}

//===================================
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
