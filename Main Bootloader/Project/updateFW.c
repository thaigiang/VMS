#include "stm32f10x_flash.h"
#include "ff.h"
#include "diskio.h"
#include "updateFW.h"

#define FLASH_PAGE_SIZE		2048

static u8 fwBuff[FLASH_PAGE_SIZE];
bool fUpdateErr = false;

//===================================
/* Copy memory to memory */
static void mem_cpy (void* dst, const void* src, UINT cnt) 
{
	BYTE *d = (BYTE*)dst;
	const BYTE *s = (const BYTE*)src;
 
	while (cnt--)
		*d++ = *s++;
}

//===================================
void updateFW(void)
{
	u32 i = 0;	
	FLASH_Status status = FLASH_COMPLETE;
	u32 addr_temp = 0;
	u32 data = 0;
	u32 fwLength = 0;
	u8 page_count = 0; 
	FIL Fil;
	UINT br = 0;
	u8 buff[4];
	FRESULT rc = FR_OK;

	// Open firmware file in SD Card
	rc = f_open(&Fil,"fw.bin",FA_READ);
	if(rc == FR_OK)
	{
		// Read firmware length
		f_read(&Fil,&buff,4,&br);
		mem_cpy((u8*)&fwLength,buff,4);
		if(fwLength == (f_size(&Fil) - 4))
		{
			page_count = ((fwLength+FLASH_PAGE_SIZE-1)/FLASH_PAGE_SIZE); 
			addr_temp = MAIN_APP_ADDR;
			// Unlock flash to rewrite to flash
			FLASH_UnlockBank1();
			// Erase pages used to store font
			for(i = 0;i<page_count;i++)
			{
				status = FLASH_ErasePage(addr_temp);
				if(status != FLASH_COMPLETE)
					break;
				addr_temp += FLASH_PAGE_SIZE;
			}
			if(status == FLASH_COMPLETE)
			{
				addr_temp = MAIN_APP_ADDR;
				while(fwLength > 0)
				{
					// Read fw data
					rc = f_read(&Fil,fwBuff,FLASH_PAGE_SIZE,&br);
					// Write font data to flash
					for(i = 0;i<br;i+=4)
					{
						mem_cpy(&data,&fwBuff[i],((br-i)>4?4:(br-i)));
						status = FLASH_ProgramWord(addr_temp,data);
						addr_temp += 4;
						if(status != FLASH_COMPLETE)
							break;
					}
					fwLength -= br;
					if(status != FLASH_COMPLETE)
							break;
				}
			}
			FLASH_LockBank1();
			if(status == FLASH_COMPLETE)
			{
				// Remove fw file
				rc = f_unlink("fw.bin");		
				fUpdateErr = false;
			}
			else
				fUpdateErr = true;
		}
	}
}
