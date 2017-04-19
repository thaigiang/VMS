/* Includes ------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "HardwareConfig.h"
#include "typedef.h"
#include "common.h"
#include "ff.h"
#include "diskio.h"
#include "dataStruct.h"
#include "stm32_ds1307.h"
#include "DS18B20.h"
#include "pageMng.h"
#include "c_func.h"

//===================================
#define FW_VERSION						207	
#define FLASH_PAGE_SIZE				2048
#define HWCONFIG_ADDR					0x0807F800
#define IMG_HEADER_LENGTH 		5
#define MAX_ROM_PREDEFINED		5
#define POPUP_INVALID_ID			0
#define MAX_BRIGHTNESS_LEVEL	8
#define MIN_BRIGHTNESS_LEVEL	1

#define LOG_DIR		"Log"

//===================================
char imgDir[6] = "image";
char pageDir[5] = "page";
char fontDir[5] = "font";

static bool fGetVersion = false;
static bool fSwapPage = false;
static bool fReloadPage = false;
static bool fClearLED = false;
static bool fScanAll = false;
static bool fStopScan = false;
static bool fScanning = false;
static bool fOutOfControl = false;
static bool fCapture = false;
static bool fCheckLED = false;
static bool fSendFont = false;
static bool fSetPredefined = false;
static bool fWaitRes = false;
static bool fSetBright = false;


bool fSendRes = false;
bool fCaptureData = false;
bool fWarning = false;
bool fTestMode = false;
bool fSdcErr = false;
WARNING_CODE warning;
RESPONSE res_value = _SUCCESS;
static RESPONSE server_res = _SUCCESS;
static u16 preImgID = 0;
static u8 preOder = 0;
static u16 fontSendID = 0;
static u16 fontSendLength = 0;
static u16 popupChangedID = 0;
static u16 packetID = 0;
static u8 table[256];
u8 rcvData[TCP_PACKAGE_LENGTH];
u8 rcvTestCmd[TCP_PACKAGE_LENGTH];
char logData[2048];
u16 rcvCountUSART1 = 0;
const u8 poly = 0x88;			//10001000 -> x7+x3
tREALTIME rtS  = {0x00,0x52,0x15,0x06,0x23,0x08,0x13};
tREALTIME logTime;
FATFS Fatfs;					/* File system object */
FILINFO fno;
DIR Dir;
FIL Fil;
static FIL Fil_1;
static DIR Dir_1;
NETWORK_STATUS networkStatus = NETWORK_LOST;
EVENT event = EVENT_NONE;
PAGE* page;
u8* buffer;
u8 imgBuffer[SCREEN_BUFF_LENGTH];
u16 activePageID = 0;
u32 fwLength = 0;
u16 fwVersion = 0;
bool fUpdateFW = false;
STATUS sysStatus = SYSTEM_OK;
SYSTEM_STATUS status;
HW_CONFIG hwConfig;
WN_CONFIG wnConfig;
COMMAND_TAG cmd = CMD_INVALID;
u16 captureCount = 0;
u16 ledDataLength = 0;
u16 brightness = MIN_BRIGHTNESS_LEVEL;
u32 TimingDelay;
u16 rcvCount;
u16 rcvCountUSART2;
extern bool pageChange;
extern u8 sysTemp;
extern bool fRcvUSART1;
extern bool fRcvUSART2;
extern u16 uart_timeout;
extern bool fSlaveRes;
extern u8 slaveResValue;
extern u32 connTimeout;
extern bool fEthernetHardReseted;


//===================================
void initBoard(void);
void sendTCPData(u8* buf,u16 length);
void Crc8(void);
void send2Display(u8 c);
u8 rcvFromDisplay(void);
void sendPopup(POPUP popup,u8 cmd);
F_RETURN swapPage(u16 pageID);
u16 getCodeFromName(TCHAR* imgName);
void checkLed(void);
void captureScreen(void);
void sendFont(u8 fontID,u16 length);
u8 sendPredefined(u16 imgID,u8 order);
u16 findPage(PAGE_TYPE type);
void timeToString(tREALTIME t,char* str);
u8 waitResponse(void);
char *convert(unsigned int num, int base);
bool checkImgInUsed(PAGE* page,u16 imgID);
void ConfigDevice(void);
void saveBrigthness(void);
void loadBrightness(void);
void updateDisplayFW(char* fileName);
u16 getDisplayVersion(void);
void loadHWConfig(HW_CONFIG* config);
void saveHWConfig(HW_CONFIG config);
void sendIMEI(void);
void sendDisplayCmd(DISPLAY_COMMAND cmd,u8* cmdParam,u8 paramLength);
void getLog(tREALTIME tStart,tREALTIME tEnd);
void saveLogIndex(void);
void convertLogName(tREALTIME rts,char *name,LOGFILE_TYPE type);
void increaseTime(tREALTIME* rts);
void decreaseTime(tREALTIME* rts);
/* Private functions ---------------------------------------------------------*/
/**
  * @brief   Main program
  * @param  None
  * @retval None
  */
int main(void)
{	
	u16 i = 0,j = 0;
	u16 pageID = 0;	
	initBoard();		
	//updateDisplayFW("dfw.bin");
	//updateDisplayFW("fw.bin");
	sendResponse(CMD_REQUEST_RECONNECT,CMD_REQUEST_RECONNECT);
	// Initial receive buffer
	memset(rcvData,0x00,TCP_PACKAGE_LENGTH);
	rcvCountUSART1 = 0;	
	// Calculate led's data buffer length
	ledDataLength = (u16)(hwConfig.ledHeight/4)*hwConfig.ledWidth;
	// Get current temperature
	sysTemp = Read_Temperature();
	// Get current date time
	rtS = DS1307_GetTime(); 

	logTime = rtS;
	// Load brightness value 
	loadBrightness();
	// Set brightness level of display module
	sendDisplayCmd(DPL_CMD_CHANGE_BRIGHTNESS,(u8*)&brightness,2);	
	waitResponse();				
	// Find default page to display
	pageID = findPage(PAGE_DEFAULT);
	if(!pageID)
	{
		// if not found, find predefine page
		pageID = findPage(PAGE_PREDEFINED);
	}
	
	// If found page 
	if(pageID)
		swapPage(pageID);
	else
	{		
		// Else play ROM predefined
		sendDisplayCmd(DPL_CMD_PLAY_PREDEINED,NULL,0);
		waitResponse();
	}	

	
	while(1)
	{
		// Reload IWDG counter 
   	IWDG_ReloadCounter();
		
		// Check if need to update firmware
		if(fUpdateFW)
		{					
			fUpdateFW = false;			
			createLog(MAIN_MODULE,EVENT_LOG,"Start update firmware");
			saveLog2SDC();
			if(fwLength < 0x011000)
				f_rename("fw.bin","dfw.bin");
			// Reset to update firmware
			sendResponse(CMD_REQUEST_REBOOT,SEND_REQUEST_UPDATE);
			NVIC_SystemReset();			
		}

		// Check TTL of popup's item to update data of popups
		for(i = 0;i<page->header.itemCount;i++)
		{
			if((page->popup[i].Info.header.dataType == POPUP_PLAYLIST))
			{
				PLAYLIST_ITEM* playlist = (PLAYLIST_ITEM*)page->popup[i].Info.data;
				for(j = 0;j<page->popup[i].Info.header.itemCount;j++)
				{					
					if((playlist[j].isLive)&&(!playlist[j].TTL_count))
					{
						playlist[j].isLive = false;
						page->popup[i].Info.header.isUpdate = false;
					}
				}
			}
			if(!page->popup[i].Info.header.isUpdate)
			{
				createLog(MAIN_MODULE,INFO_LOG,"Update display's popup \t| Popup ID: %d",page->popup[i].Info.header.ID);
				switch(page->popup[i].Info.header.dataType)
				{
					case POPUP_PLAYLIST:
						{
							if(page->popup[i].Info.header.itemCount)
							{
								PLAYLIST_ITEM* playlist = (PLAYLIST_ITEM*)page->popup[i].Info.data;
								bool fCheck = false;
								
								for(j = 0;j<page->popup[i].Info.header.itemCount;j++)
								{
									page->popup[i].Info.header.activeItem ++;							
									if(page->popup[i].Info.header.activeItem >= page->popup[i].Info.header.itemCount)
										page->popup[i].Info.header.activeItem = 0;	
									if(playlist[page->popup[i].Info.header.activeItem].isLive)
									{
										fCheck = true;
										break;
									}
								}
								if(fCheck)
								{
									if(page->popup[i].Info.header.itemCount == 1)
									{
										page->popup[i].Info.header.activeItem = 0;	
										playlist[page->popup[i].Info.header.activeItem].header.TTS = 0;
									}
									page->popup[i].Info.header.countDown = playlist[page->popup[i].Info.header.activeItem].header.TTS;									
									sendPopup(page->popup[i],DPL_CMD_UPDATE_POPUP);
									waitResponse();															
								}
								else
								{
									createLog(MAIN_MODULE,INFO_LOG,"All popup's items not live.Clear popup!");
									//deletePopupCmd(page->popup[i].Info.header.ID);
									sendDisplayCmd(DPL_CMD_CLEAR_POPUP,(u8*)&page->popup[i].Info.header.ID,2);
									waitResponse();
								}							
							}
							else
							{
								createLog(MAIN_MODULE,INFO_LOG,"Popup don't contain any item.Clear popup!");
								//deletePopupCmd(page->popup[i].Info.header.ID);
								sendDisplayCmd(DPL_CMD_CLEAR_POPUP,(u8*)&page->popup[i].Info.header.ID,2);
								waitResponse();
							}
						}
						break;
					case POPUP_CLOCK:
					case POPUP_TEMP:						
						sendPopup(page->popup[i],DPL_CMD_UPDATE_POPUP);
						waitResponse();						
						page->popup[i].Info.header.countDown = 0;
						break;
					default:
						break;
				}
				page->popup[i].Info.header.isUpdate = true;
			}
		}
		
		if(fReloadPage)
		{
			PAGE* pageTemp;
			u16 pageID = 0;

			createLog(MAIN_MODULE,INFO_LOG,"Reload page \t\t| Page ID: %d",page->header.ID);
			pageID = page->header.ID;
			pageTemp = loadPage(pageID);	
			if(pageTemp != NULL)
			{
				for(i =0;i<pageTemp->header.itemCount;i++)
				{
					for(j = 0;j<page->header.itemCount;j++)
					{
						if(pageTemp->popup[i].Info.header.ID == page->popup[j].Info.header.ID)
						{
							if((cmd != CMD_UPDATE_POPUP)||(pageTemp->popup[i].Info.header.ID != popupChangedID))
								memcpy(&pageTemp->popup[i],&page->popup[j],sizeof(POPUP_HEADER));
							break;
						}
					}
				}
				freePageResource(page);
				page = pageTemp;		
				if(cmd == CMD_INSERT_POPUP)
				{
					for(i = 0;i<page->header.itemCount;i++)
					{
						if(page->popup[i].Info.header.ID == popupChangedID)
						{
							sendPopup(page->popup[i],DPL_CMD_CREATE_POPUP);
							page->popup[i].Info.header.isUpdate = true;
							waitResponse();							
							break;
						}
					}				
				}
				else if(cmd == CMD_DELETE_POPUP)
				{				
					sendDisplayCmd(DPL_CMD_DELETE_POPUP,(u8*)&popupChangedID,2);
					waitResponse();
				}
				sendResponse(cmd,_SUCCESS);
			}
			else
				sendResponse(cmd,_ERROR);
			cmd = CMD_INVALID;
			fReloadPage = false;
		}
		
		if(fOutOfControl)
		{
			u8 res = 0;				

			createLog(MAIN_MODULE,EVENT_LOG,"Device out of control!");
			// Check if lost connection while scanning LEDs
			if((sysStatus & SYSTEM_ERR_NETWORK)&&fScanning)
			{		
				// Stop scan LEDs
				sendDisplayCmd(DPL_CMD_STOP_SCAN_LED,NULL,0);
				res = waitResponse();		
				delayms(5000);
				if(!res)
					fScanning = false;
			}
			
			// Clear current page
			sendDisplayCmd(DPL_CMD_CLEAR_SCREEN,NULL,0);
			waitResponse();
			
			// Play ROM predefined
			sendDisplayCmd(DPL_CMD_PLAY_PREDEINED,NULL,0);
			waitResponse();
			fOutOfControl = false;
		}
		
		if(fSwapPage)
		{
			u8 res = 0;				
			F_RETURN err = F_SUCCESS;
			
			if((sysStatus & SYSTEM_ERR_NETWORK)&&fScanning)
			{								
				// Stop scan LEDs
				sendDisplayCmd(DPL_CMD_STOP_SCAN_LED,NULL,0);
				res = waitResponse();		
				delayms(5000);
				if(!res)
					fScanning = false;
			}
			err = swapPage(activePageID);
			fSwapPage = false;	
			if(cmd != CMD_INVALID)
			{	
				if(err != F_SUCCESS)
				{
					activePageID = 0;
					sendResponse(cmd,_ERROR);
				}
				else
					sendResponse(cmd,_SUCCESS);
				cmd = CMD_INVALID;
			}
		}
		
		if(fClearLED)
		{		
			u8 res = 0;

			createLog(MAIN_MODULE,EVENT_LOG,"Clear LED screen!");
			activePageID = 0; 
			if(cmd == CMD_DELETE_PAGE)
			{				
				freePageResource(page);
				page = NULL;
			}
			// Clear screen
			sendDisplayCmd(DPL_CMD_CLEAR_SCREEN,NULL,0);
			res = waitResponse();		
			if(cmd == CMD_DELETE_PAGE)
				sendResponse(cmd,_SUCCESS);
			else
			{
				if(res)
					sendResponse(cmd,_ERROR);
				else
					sendResponse(cmd,_SUCCESS);
			}
			cmd = CMD_INVALID;
			fClearLED = false;
		}
		
		if(fScanAll)
		{
			u8 res = 0;
			
			createLog(MAIN_MODULE,EVENT_LOG,"Scan all LEDs!");
			if(activePageID != 0)
			{				
				freePageResource(page);
				page = (PAGE*)(NULL);				
			}
			// Clear screen
			sendDisplayCmd(DPL_CMD_CLEAR_SCREEN,NULL,0);
			waitResponse();		

			// Scan all LEDs
			sendDisplayCmd(DPL_CMD_START_SCAN_LED,NULL,0);
			res = waitResponse();		
			delayms(5000);
			if(res)
				sendResponse(CMD_START_SCAN_LED,_ERROR);
			else
			{
				sendResponse(CMD_START_SCAN_LED,_SUCCESS);
				fScanning = true;
			}
			fScanning = true;
			fScanAll = false;
		}
		
		if(fStopScan)
		{	
			u8 res = 0;

			createLog(MAIN_MODULE,EVENT_LOG,"Stop scan LED!");
			sendDisplayCmd(DPL_CMD_STOP_SCAN_LED,NULL,0);
			res = waitResponse();		
			delayms(5000);
			if(res)
				sendResponse(CMD_STOP_SCAN_LED,_ERROR);
			else
			{
				sendResponse(CMD_STOP_SCAN_LED,_SUCCESS);
				fScanning = false;
			}
			fScanning = false;
			fStopScan = false;
			// Re-play current page
			fSwapPage = true;
		}
		
		if(fCapture)
		{
			captureScreen();
			fCapture = false;
		}
		
		if(fCheckLED)
		{
			fCheckLED = false;
		}
		
		if(fSendFont)
		{
			u8 res = 0;
			
			sendFont(fontSendID,fontSendLength);
			res = waitResponse();
			if(res)
				sendResponse(cmd,_ERROR);
			else
				sendResponse(cmd,_SUCCESS);
			fSendFont = false;
			fontSendID = 0;
			fontSendLength = 0;
		}
		
		if(fSetPredefined)
		{
			u8 res = 0;
			res = sendPredefined(preImgID,preOder);			
			preImgID = 0;
			preOder = 0;
			if(res)
				sendResponse(cmd,_ERROR);
			else
				sendResponse(cmd,_SUCCESS);
			cmd = CMD_INVALID;
			fSetPredefined = false;
		}
		
		if(fWarning)
		{
			sendWarning(warning);
			fWarning = false;
		}
		
		if(fSetBright)
		{				
			u8 res = 0;
			
			sendDisplayCmd(DPL_CMD_CHANGE_BRIGHTNESS,(u8*)&brightness,2);			
			res = waitResponse();		
			delayms(100);
			if(res)
				sendResponse(cmd,_ERROR);
			else
			{
				saveBrigthness();
				sendResponse(cmd,_SUCCESS);
			}
			cmd = CMD_INVALID;
			fSetBright = false;
		}
		if(fGetVersion)
		{			
			u8 data[5];
			u16 dplVersion = 0;
			u16 mainVersion = FW_VERSION;
			//createLog(SERVER,INFO_LOG,"Request get firmware version");
			dplVersion = getDisplayVersion();
			data[0] = ETHERNET_GET_VERSION;
			data[1] = (u8)(dplVersion);
			data[2] = (u8)(dplVersion>>8);
			data[3] = (u8)(mainVersion);
			data[4] = (u8)(mainVersion>>8);
			sendTCPData(data,5);
			fGetVersion = false;
		}
	}
}


//===================================
void initBoard(void)
{	
	u16 adc_value = 0;
	u16 battery_volt = 0;
		
	RCC_Configuration();					// System Clocks Configuration 	
	GPIO_Configuration();					// Configure the GPIO ports 
	USART1_Configuration();
	//USART2_Configuration();
	USART4_Configuration();
	ADC_Configuration();
	SPI_Configuration();
	I2C_config();
	TIMER_Configuration();	
	NVIC_Configuration();
	EXTI_Configuration();
	DS1307_Int();
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
	Crc8();
	// Init SD Card
	delayms(2000);
	initSDC();
	if(sysStatus&SYSTEM_ERR_SDC)
		fSdcErr = true;
		
	createLog(MAIN_MODULE,EVENT_LOG,"DEVICE Start");
	saveLog2SDC();
	
	sysStatus |= SYSTEM_ERR_NETWORK;
	displayStatus(sysStatus);
	loadHWConfig(&hwConfig);	
	wnConfig.maxTemp = DEFAULT_MAX_TEMP;
	
	adc_value = ADC_GetConversionValue(ADC1);
	//battery_volt = ((adc_value*33*62/4096)/15);
	battery_volt = (adc_value*33*2/4096);
	if(battery_volt < BATTERY_VOLT_EMPTY)
	{
		GPIO_WriteBit(CHARGE_PORT,CHARGE_PIN,Bit_RESET);
	}
	else if(battery_volt >= BATTERY_VOLT_FULL)
	{
		GPIO_WriteBit(CHARGE_PORT,CHARGE_PIN,Bit_SET);
	}	
	// Reload IWDG counter 
   	IWDG_ReloadCounter();
}

//===================================
void initSDC(void)
{
	DRESULT dr;
	DSTATUS dstatus;
	long p;
	
	dstatus = disk_initialize(0);

	if(!dstatus)
	{
		f_mount(0, &Fatfs);	  /* Register volume work area (never fails) */

		dr = disk_ioctl(0, GET_SECTOR_COUNT, &p); 
		if (dr == RES_OK)
		{ 
			f_mkdir(imgDir);

			f_mkdir(pageDir);
			
			f_mkdir(fontDir);

			f_mkdir(LOG_DIR);
			
			sysStatus &= ~SYSTEM_ERR_SDC;
		}
		else
			sysStatus |= SYSTEM_ERR_SDC;
	}
	else
	{
		sysStatus |= SYSTEM_ERR_SDC;
	}
}
//===================================
void displayStatus(STATUS status)
{
	GPIO_WriteBit(LED_1_PORT,LED_1_PIN,Bit_SET);
	GPIO_WriteBit(LED_2_PORT,LED_2_PIN,Bit_RESET);
	GPIO_WriteBit(LED_3_PORT,LED_3_PIN,Bit_RESET);

	if(status&SYSTEM_ERR_NETWORK)
	{
		GPIO_WriteBit(LED_3_PORT,LED_3_PIN,Bit_RESET);	
	}
	else
		GPIO_WriteBit(LED_3_PORT,LED_3_PIN,Bit_SET);	
	
	if(status&SYSTEM_ERR_SDC)
	{
		GPIO_WriteBit(LED_2_PORT,LED_2_PIN,Bit_RESET);	
	}
	else
		GPIO_WriteBit(LED_2_PORT,LED_2_PIN,Bit_SET);	
}


////===================================
//void TestStatus(STATUS status)
//{
//	if(status == SYSTEM_OK)
//	{
//		USART_SendData(USART1,0x30);	
//		return;
//	}
//	if(status&SYSTEM_ERR_NETWORK)
//	{
//		USART_SendData(USART1,0x31);	
//	}	
//	
//	if(status&SYSTEM_ERR_SDC)
//	{
//		USART_SendData(USART1,0x32);	
//	}
//	
//	if(status&SYSTEM_ERR_FLASH)
//	{
//		USART_SendData(USART1,0x33);	
//	}
//	
//	if(status&SYSTEM_ERR_SIM)
//	{
//		USART_SendData(USART1,0x34);	
//	}
//}

//===================================
void saveBrigthness(void)
{
	FRESULT rc;
	UINT bw;
	
	rc = f_open(&Fil_1,"bri.bin",FA_WRITE);
	if(rc == FR_NO_FILE)
	{
		rc = f_open(&Fil_1,"bri.bin",FA_CREATE_NEW);
	}
	if(rc == FR_OK)
	{		
		rc = f_write(&Fil_1,&brightness,2,&bw);
	}
	f_close(&Fil_1);
}

//===================================
void loadBrightness(void)
{
	FRESULT rc;
	UINT br;
	
	rc = f_open(&Fil_1,"bri.bin",FA_READ);
	if(rc == FR_OK)
	{		
		rc = f_read(&Fil_1,&brightness,2,&br);
	}
	f_close(&Fil_1);
	createLog(MAIN_MODULE,INFO_LOG,"Load current brightness \t| Value: %d",brightness);
	saveLog2SDC();
}

//===================================
void saveHWConfig(HW_CONFIG config)
{
	u32 data_temp = 0;
	u16 i = 0;
	u16 length = 0;
	u32 addr = 0;
	u8* ptr;
	FLASH_Status status = FLASH_COMPLETE;

	createLog(MAIN_MODULE,INFO_LOG,"Set new config \t| ledWidth: %d, ledHeight: %d, ledType: %d",config.ledWidth,config.ledHeight,config.ledType);
	length = sizeof(HW_CONFIG);
	ptr = (u8*)&config;
	// Unlock flash to rewrite to flash
	FLASH_UnlockBank1();
	status = FLASH_ErasePage(HWCONFIG_ADDR);	
	addr = HWCONFIG_ADDR;
	for(i = 0;i<length;i+=4)
	{
		memcpy(&data_temp,&ptr[i],((length-i)>4?4:(length-i)));
		status = FLASH_ProgramWord(addr,data_temp);
		addr += sizeof(u32);
	}
	FLASH_LockBank1();
}

//===================================
void loadHWConfig(HW_CONFIG* config)
{
	u32 addr = 0;
	u8* ptr;

	addr = HWCONFIG_ADDR;
	ptr = (u8*)addr;
	memcpy(config,ptr,sizeof(HW_CONFIG));

	config->ledWidth = DEFAULT_LED_WIDTH;
	config->ledHeight = DEFAULT_LED_HEIGHT;	
	createLog(MAIN_MODULE,INFO_LOG,"Current Device's Config \t| ledWidth: %d, ledHeight: %d, ledType: %d",config->ledWidth,config->ledHeight,config->ledType);
	saveLog2SDC();
}

//===================================
void ConfigDevice(void)
{
	u8 buff[5];

	buff[0] = (u8)(hwConfig.ledWidth&0xFF);
	buff[1] = (u8)(hwConfig.ledWidth>>8);
	buff[2] = (u8)(hwConfig.ledHeight&0xFF);
	buff[3] = (u8)(hwConfig.ledHeight>>8);
	buff[4] = hwConfig.ledType;
	
	// Send hwConfig
	sendDisplayCmd(DPL_CMD_SEND_HWCONFIG,buff,5);
	waitResponse();
}

//===================================
void createLog(SOURCE_LOG srcLog,LOG_TYPE logType,char * frmt,...)
{
	/*
#ifdef SAVE_LOG
	char time[10];
	va_list argp;
	char* p;
	char *s;
	u16 u = 0;
	u16 index = 0;

	va_start(argp, frmt);

	index = strlen((char*)logData);
	if(index>=1920)
	{
		saveLog2SDC();		
		index = 0;
	}
	memset(time,0x00,10);
	rtS = DS1307_GetTime();
	if((rtS.data.Minute > logTime.data.Minute)||(rtS.data.Hour > logTime.data.Hour))
	{
		saveLogIndex();
		logTime = rtS;
	}
	switch(logType)
	{
		case EVENT_LOG:
			//Save log time		
			timeToString(rtS,time);
			strcat(logData,time);
			strcat(logData," | EVENT | ");
			break;
		case INFO_LOG: 
			//Save log time		
			timeToString(rtS,time);
			strcat(logData,time);
			strcat(logData," | INFO  | ");
			break;
		case RESPONSE_LOG:
			//Save log time		
			timeToString(rtS,time);	
			strcat(logData,time);
			strcat(logData," | RES   | ");
			break;		
		case ERROR_LOG:
			//Save log time		
			timeToString(rtS,time);	
			strcat(logData,time);
			strcat(logData," | ERROR | ");
			break;
		default:
			break;
	}
	switch(srcLog)
	{		
		case MAIN_MODULE:	
			strcat(logData,"Device | ");
			break;
		case NETWORK_MODULE:
			strcat(logData,"Network| ");
			break;
		case SERVER:
			strcat(logData,"Server | ");
			break;
	}
	p=frmt;
	for(p=frmt; *p!='\0';p++)
	{
		if(*p!='%')
		{			
			logData[strlen(logData)] = *p;
			continue;
		}
		p++;			
		switch(*p)
		{			
			case 'd':
				u=va_arg(argp, unsigned int); 
				s  = convert(u,10);
				strcat(logData,s);
				break;
			default:
				break;
		}
	}
	strcat(logData,"\r\n");
	va_end(argp);	
#endif
*/
}

//===================================
void saveLog2SDC(void)
{
	/*
#ifdef SAVE_LOG	
	UINT bw = 0;
	u16 length = 0;
	char fileName[17];
	FRESULT rc;
	
	memset(fileName,0x00,15);
	// Convert date to file name
	convertLogName(rtS,fileName,LOG_DATA);
	rc = f_opendir(&Dir,LOG_DIR);		
	if(rc == FR_OK)
		rc = f_open(&Fil,fileName,FA_WRITE);
	if(rc == FR_NO_FILE)
	{
		// Create new log file
		rc = f_open(&Fil,fileName,FA_CREATE_NEW);
		if(rc == FR_OK)
		{
			switch(fileName[8])
			{
				case 0x30:
					fileName[9] -= 1;
					if(fileName[9] == 0x30)
					{
						fileName[8] = 0x31;
						fileName[9] = 0x32;
						if(fileName[7]>0x30)
							fileName[7] -=1;
						else
						{
							fileName[7] = 0x39;
							if(fileName[6]>0x30)
								fileName[6] -= 1;
							else
								fileName[6] = 0x39;
						}
					}
					break;
				case 0x31:
					if(fileName[9] == 0x30)
					{
						fileName[8] = 0x30;
						fileName[9] = 0x39;
					}
					else
						fileName[9] -= 1;
					break;
			}
			f_unlink(fileName);	
		}
	}
	if(rc == FR_OK)
	{						
		length = strlen(logData);
		f_lseek(&Fil,Fil.fsize);						
		rc = f_write(&Fil,logData,length,&bw);	
		f_close(&Fil);
		memset(logData,0x00,2048);
	}
#endif
*/
}

//===================================
void saveLogIndex(void)
{
	UINT bw = 0;
	char fileName[17];
	char time[10];
	u32 fileLength = 0;
	u8* ptr;
	FRESULT rc;
	
	memset(fileName,0x00,15);
	// Convert date to file name
	convertLogName(rtS,fileName,LOG_INDEX);
	rc = f_opendir(&Dir,LOG_DIR);		
	if(rc == FR_OK)
		rc = f_open(&Fil,fileName,FA_WRITE);
	if(rc == FR_NO_FILE)
	{
		// Create new log file
		rc = f_open(&Fil,fileName,FA_CREATE_NEW);
		if(rc == FR_OK)
		{
			switch(fileName[8])
			{
				case 0x30:
					fileName[9] -= 1;
					if(fileName[9] == 0x30)
					{
						fileName[8] = 0x31;
						fileName[9] = 0x32;
						if(fileName[7]>0x30)
							fileName[7] -=1;
						else
						{
							fileName[7] = 0x39;
							if(fileName[6]>0x30)
								fileName[6] -= 1;
							else
								fileName[6] = 0x39;
						}
					}
					break;
				case 0x31:
					if(fileName[9] == 0x30)
					{
						fileName[8] = 0x30;
						fileName[9] = 0x39;
					}
					else
						fileName[9] -= 1;
					break;
			}
			f_unlink(fileName);	
		}
	}
	if(rc == FR_OK)
	{						
		timeToString(rtS,time);
		time[6] = 0;
		time[7] = 0;
		ptr = (u8*)strstr(logData,time);
		if(ptr != NULL)
		{
			fileLength = (ptr - &logData[0]);
		}
		fileName[13] = 't';
		fileName[14] = 'x';
		fileName[15] = 't';
		rc = f_open(&Fil_1,fileName,FA_READ);
		fileLength += Fil_1.fsize;
		f_close(&Fil_1);
		f_lseek(&Fil,Fil.fsize);
		rc = f_write(&Fil,(u8*)&rtS.data.Hour,1,&bw);
		rc = f_write(&Fil,(u8*)&rtS.data.Minute,1,&bw);
		rc = f_write(&Fil,(u8*)&fileLength,4,&bw);
		f_close(&Fil);
	}
}

//===================================
void timeToString(tREALTIME t,char* str)
{
	char temp[4];

	temp[3] = 0;
	
	temp[0] = (rtS.data.Hour>>4) + 0x30;
	temp[1] = (rtS.data.Hour&0x0F) + 0x30;
	temp[2] = ':';
	strcpy(str,temp);	
	temp[0] = (rtS.data.Minute>>4) + 0x30;
	temp[1] = (rtS.data.Minute&0x0F) + 0x30;
	temp[2] = ':';
	strcat(str,temp);	
	temp[0] = (rtS.data.Second>>4) + 0x30;
	temp[1] = (rtS.data.Second&0x0F) + 0x30;
	temp[2] = 0;
	strcat(str,temp);	
}

void convertLogName(tREALTIME rts,char *name,LOGFILE_TYPE type)
{
	strcpy(name,LOG_DIR);
	name[3] = '/';
	name[4] = '2';
	name[5] = '0';
	name[6] = (rts.data.Year>>4) + 0x30;
	name[7] = (rts.data.Year&0x0F) + 0x30;
	name[8] = (rts.data.Month>>4) + 0x30;
	name[9] = (rts.data.Month&0x0F) + 0x30;
	name[10] = (rts.data.Date>>4) + 0x30;
	name[11] = (rts.data.Date&0x0F) + 0x30;
	name[12] = '.';
	switch(type)
	{		
		case LOG_INDEX:
			name[13] = 'i';
			name[14] = 'd';
			name[15] = 'x';
			break;
		case LOG_DATA:
		default:			
			name[13] = 't';
			name[14] = 'x';
			name[15] = 't';
			break;
	}
	name[16] = 0x00;
}

void increaseTime(tREALTIME* rts)
{
	rts->data.Minute ++;
	if((rts->data.Minute&0x0F)==0x0A)
	{
		rts->data.Minute = (rts->data.Minute&0xF0) + 0x10;
	}
	if(rts->data.Minute == 0x60)
	{
		rts->data.Minute = 0x00;
		rts->data.Hour ++;
		if((rts->data.Hour&0x0F)==0x0A)
		{
			rts->data.Hour = (rts->data.Hour&0xF0) + 0x10;
		}
	}
}

void decreaseTime(tREALTIME* rts)
{
	if(rts->data.Minute == 0x00)
	{
		rts->data.Minute = 0x59;				
		if((rts->data.Hour&0x0F)==0x00)
		{
			rts->data.Hour = ((rts->data.Hour&0xF0) - 0x10)|0x09;
		}
		else
			rts->data.Hour --;
	}	
	else if((rts->data.Minute&0x0F)==0x00)
	{
		rts->data.Minute = ((rts->data.Minute&0xF0) - 0x10)|0x09;						
	}
	else
		rts->data.Minute --;
}

//===================================
void EthMsgControl(u8* msg)
{
	u16 ID = 0;
	
	switch(msg[1])
	{
		case ETHERNET_GET_VERSION:
			fGetVersion = true;
			break;
		case NETWORK_LOST:
			createLog(NETWORK_MODULE,EVENT_LOG,"DEVICE Lost Connection");
			networkStatus = NETWORK_LOST;
			event = EVENT_NETWORK_LOST;
			sysStatus |= SYSTEM_ERR_NETWORK;
			displayStatus(sysStatus);
			// Find and play predefine page
			ID = findPage(PAGE_PREDEFINED);
			if(ID != activePageID)
			{
				activePageID = ID;
				fSwapPage = true;						
			}
			else
			{
				// Set flag to play predefined page in ROM
				fOutOfControl = true;
			}
			ID = 0;			
			break;
			
		case NETWORK_ETHERNET:
		case NETWORK_GPRS:
			createLog(NETWORK_MODULE,EVENT_LOG,"DEVICE connected to server");
			connTimeout = 0;			
			networkStatus = msg[1];

			sysStatus &= ~SYSTEM_ERR_NETWORK;
			displayStatus(sysStatus);
			
			sendIMEI();
			if(sysStatus & SYSTEM_ERR_SIM)
			{
				fWarning = true;
				warning = WARNING_SIM_ERR;
			}
			if(event == EVENT_NETWORK_LOST)
				event = EVENT_NONE;
			//fSwapPage = true;		
			ID = findPage(PAGE_DEFAULT);
			if(ID != activePageID)
			{
				activePageID = ID;
				fSwapPage = true;						
			}	
			ID = 0;			
			break;
		case ETHERNET_GET_TIME:			
			{
				BYTE buff[10];

				createLog(NETWORK_MODULE,EVENT_LOG,"Request get current time");
				rtS = DS1307_GetTime();
				buff[0] = ETHERNET_GET_TIME;
				memcpy(&buff[1],&rtS,sizeof(rtS));
				sendTCPData(buff,sizeof(rtS)+1);
			}
			break;
		case ETHERNET_SET_TIME:
			createLog(NETWORK_MODULE,EVENT_LOG,"Request set time");
			memcpy(&rtS,&msg[2],sizeof(rtS));
			DS1307_Int();
			DS1307_SetTime(rtS);
			DS1307_Start();
			break;
		case ETHERNET_GET_IMEI:
			{
				u8 i;
				BYTE buff[25];
				uint64_t temp = 0;
				struct u_id imei;
				uid_read(&imei);
				temp = (temp | imei.off4) << 32 | (imei.off8);
				buff[0] = ETHERNET_GET_IMEI;
				
				for(i=1;i<=15;i++)
				{
					buff[i] = temp %10 + '0';
					temp = (temp-buff[i])/10;
				}
				
				sendTCPData(buff,16);
			}
			break;
		case ETHERNET_GET_CONFIG:			
			{
				BYTE buff[25];
				
				createLog(NETWORK_MODULE,EVENT_LOG,"Request get device's config");
				buff[0] = ETHERNET_GET_CONFIG;
				memcpy(&buff[1],&hwConfig,sizeof(hwConfig));
				sendTCPData(buff,sizeof(hwConfig)+1);						
			}
			break;
		case ETHERNET_SET_CONFIG:
			createLog(NETWORK_MODULE,EVENT_LOG,"Request config device");
			memcpy(&hwConfig,&msg[2],sizeof(hwConfig));
			saveHWConfig(hwConfig);
			ConfigDevice();
			NVIC_SystemReset();
			break;
		case ETHERNET_REPORT:
			createLog(NETWORK_MODULE,EVENT_LOG,"Report error from network module");
			if((msg[2] == SIM_ERROR)||(msg[2] == SIM_NOT_READY))
				sysStatus |= SYSTEM_ERR_SIM;
			else if(msg[2] == SIM_READY)
				sysStatus &= ~SYSTEM_ERR_SIM;
			break;
		case ETHERNET_SCAN_LED:
			if(!fScanning)
				fScanAll = true;
			else
				fStopScan = true;
			break;
		default:
			createLog(NETWORK_MODULE,EVENT_LOG,"Network send wrong command");
			break;
	}
	saveLog2SDC();
}

//===================================
void msgControl(u8 * msg,u16 length)
{
	u32 i = 0;
	static u16 ID = 0;
	static u16 fontID = 0;
	u8* data = NULL;
	static u16 imgLength = 0;
	static u16 index = 0;
	static u32 fontLength = 0;
	FRESULT rc;
	UINT bw = 0;
	
	// Check data 
	switch(msg[0])
	{
		case PING:
			connTimeout = 0;
			fEthernetHardReseted = false;
			break;
		case SERVER_TAG:
			connTimeout = 0;
			if(fScanning&&(msg[1]!=CMD_STOP_SCAN_LED))
			{
				sendResponse(msg[1],ERROR_DEVICE_BUSY);
				break;
			}
			switch(msg[1])
			{	
				case CMD_SET_TIME:
					{
						createLog(SERVER,INFO_LOG,"Request set time");
						memcpy(&rtS,&msg[2],sizeof(rtS));
						
						rtS.data.Second = covertDecToHex(rtS.data.Second);
						rtS.data.Minute = covertDecToHex(rtS.data.Minute);
						rtS.data.Hour = covertDecToHex(rtS.data.Hour);
						rtS.data.Date = covertDecToHex(rtS.data.Date);
						rtS.data.Month = covertDecToHex(rtS.data.Month);
						rtS.data.Year = covertDecToHex(rtS.data.Year);
						
						DS1307_Int();
						DS1307_SetTime(rtS);
						DS1307_Start();
						
						cmd = CMD_SET_TIME;
						sendResponse(cmd,_SUCCESS);
						cmd = CMD_INVALID;
					}
					break;
				case CMD_GET_TIME:
					{
						u8 data[sizeof(rtS)+2];
						//u32 time;
						createLog(SERVER,INFO_LOG,"Request get current time");
						rtS = DS1307_GetTime();
						
						rtS.data.Second = covertHexToDec(rtS.data.Second);
						rtS.data.Minute = covertHexToDec(rtS.data.Minute);
						rtS.data.Hour = covertHexToDec(rtS.data.Hour);
						rtS.data.Date = covertHexToDec(rtS.data.Date);
						rtS.data.Month = covertHexToDec(rtS.data.Month);
						rtS.data.Year = covertHexToDec(rtS.data.Year);
												
						data[0] = DEVICE_TAG;
						data[1] = CMD_GET_TIME;
						memcpy(&data[2],&rtS,sizeof(rtS));
						sendTCPData(data,sizeof(rtS)+2);
					}
					break;
				case SET_BRIGHTNESS:					
					memcpy(&brightness,&msg[2],2);
					createLog(SERVER,INFO_LOG,"Request change brightness value \t| New value:%d",brightness);
					if((brightness>MAX_BRIGHTNESS_LEVEL)||(brightness<MIN_BRIGHTNESS_LEVEL))
						sendResponse(SET_BRIGHTNESS,_ERROR);
					else
					{
						fSetBright = true;
						cmd = SET_BRIGHTNESS;
						fSendRes = false;						
					}
					break;
				case GET_BRIGHTNESS:					
					{
						u8 data[4];

						createLog(SERVER,INFO_LOG,"Request get brightness value");
						data[0] = DEVICE_TAG;
						data[1] = GET_BRIGHTNESS;
						memcpy(&data[2],&brightness,2);
						sendTCPData(data,4);
					}
					break;
				case LED_SCREEN_DATA:
					fWaitRes = true;
					server_res = msg[2];	
					break;
				case LED_STATUS_DATA:
					fWaitRes = true;
					server_res = msg[2];	
					break;
				case CMD_CLEAR_LED:
					createLog(SERVER,INFO_LOG,"Request clear screen");
					fClearLED = true;
					cmd = CMD_CLEAR_LED;					
					break;
				case CMD_STOP_SCAN_LED:
					createLog(SERVER,INFO_LOG,"Request stop scan led");
					if(fScanning)
						fStopScan = true;
					else
						sendResponse(CMD_STOP_SCAN_LED,_SUCCESS);
					break;
				case CMD_START_SCAN_LED:
					createLog(SERVER,INFO_LOG,"Request scan all LEDs");
					if(!fScanning)
						fScanAll = true;
					else
						sendResponse(CMD_START_SCAN_LED,_SUCCESS);
					break;
				case CMD_REBOOT:
					createLog(SERVER,INFO_LOG,"Request reboot");
					sendResponse(CMD_REQUEST_REBOOT,SEND_REQUEST_UPDATE);
					saveLog2SDC();
					NVIC_SystemReset();
					break;
				case CMD_CAPTURE_SCREEN:
					createLog(SERVER,INFO_LOG,"Request capture screen");
					fCapture = true;					
					break;
				case CMD_CHECK_LED:	
					//createLog(SERVER,INFO_LOG,"Request check LED");
					//fCheckLED = true;					
					//sendResponse(CMD_CHECK_LED,_ERROR);	
					
					sendResponse(CMD_REQUEST_REBOOT,SEND_REQUEST_UPDATE);
		
					break;
				case CMD_SET_PREDEFINED:					
					memcpy(&preImgID,&msg[2],2);
					preOder = msg[4];
					createLog(SERVER,INFO_LOG,"Request update predefined \t| Image ID: %d, Predef Order: %d",preImgID,preOder);					
					// Check is parameters valid
					if((preOder>=MAX_ROM_PREDEFINED) || (!checkImgExist(preImgID)))
					{
						sendResponse(CMD_SET_PREDEFINED,_ERROR);	
					}
					else
					{
						fSetPredefined = true;											
						cmd = CMD_SET_PREDEFINED;	
						fSendRes = false;
					}
					break;
				case CMD_GET_STATUS:
					{
						FATFS* fs;
						u32 clusterCount = 0;
						
						createLog(SERVER,INFO_LOG,"Request get device status");												
						data = (u8*)m_calloc(sizeof(SYSTEM_STATUS) + 2,sizeof(u8));
						data[0] = DEVICE_TAG;
						data[1] = CMD_GET_STATUS;
						status.temp = sysTemp;
						if(sysStatus & SYSTEM_ERR_SDC)
							status.SD_Status = 0;
						else
							status.SD_Status = 1;
						// Get number of free cluster
						f_getfree(NULL,&clusterCount,&fs);
						// Calculate SDCard free memory size (KB)
						status.SD_Free_Mem = (clusterCount*fs->csize)/2;
						// Send system status to server
						memcpy(&data[2],&status,sizeof(SYSTEM_STATUS));
						sendTCPData(data,sizeof(SYSTEM_STATUS) + 2);
						m_free(data);
					}
					break;
				case GET_VERSION:
					{			
						u8 data[4];
						u16 version = 0;

						createLog(SERVER,INFO_LOG,"Request get firmware version");
						version = FW_VERSION;
						data[0] = DEVICE_TAG;
						data[1] = GET_VERSION;
						data[2] = (u8)(version);
						data[3] = (u8)(version>>8);
						sendTCPData(data,4);
					}
					break;
				case CMD_SERVER_FIRMWARE_UPDATE_REQ:
					createLog(SERVER,INFO_LOG,"Request update firmware");					
					fwLength = 0;					
					fSendRes = true;
					cmd = CMD_SERVER_FIRMWARE_UPDATE_REQ;
					res_value = _SUCCESS;
					break;
				case FIRMWARE_INFO_HEADER:						
					memcpy(&fwVersion,&msg[2],2);
					memcpy(&fwLength,&msg[4],4);
					createLog(SERVER,INFO_LOG,"Info of new firmware \t| Version: %d,FW length: %d bytes",fwVersion,fwLength);
					rc = f_unlink("fw.bin");
					if((rc == FR_OK)||(rc == FR_NO_FILE))
						rc = f_open(&Fil,"fw.bin",FA_CREATE_ALWAYS | FA_WRITE);
					if(rc == FR_OK)
						rc = f_write(&Fil,&msg[4],4,&bw);						
					if(rc == FR_OK)
					{
						fSendRes = true;
						cmd = FIRMWARE_INFO_HEADER;
						res_value = _SUCCESS;
						packetID = 0;
					}
					else
					{
						fSendRes = true;
						cmd = FIRMWARE_INFO_HEADER;
						res_value = ERROR_STORAGE_NOT_AVAILABLE;
						sysStatus |= SYSTEM_ERR_SDC;
					}
					f_close(&Fil);											
					break;
				case FIRMWARE_PACKAGE_HEADER:
					{
						u16 dataLength = 0;
						u16 pkIndex = 0;
						u16 pkLength = 0;

						memcpy(&pkIndex,&msg[2],2);
						memcpy(&dataLength,&msg[4],2);
						pkLength = length;

						createLog(SERVER,INFO_LOG,"Firmware data package \t| Package index: %d, length: %d bytes",pkIndex,dataLength);
						// Check packet ID
						if(pkIndex != packetID)
						{
							fSendRes = true;
							cmd = FIRMWARE_PACKAGE_HEADER;
							res_value = ERROR_PACKAGE_ID;
						}
						// Check data length
						else if((dataLength != (pkLength - 7)) && (dataLength != ((pkLength - 7)-(16-(dataLength+7)%16))))
						{
							fSendRes = true;
							cmd = FIRMWARE_PACKAGE_HEADER;
							res_value = _ERROR;
						}
						else
						{
							rc = f_open(&Fil,"fw.bin",FA_WRITE);							
							if(rc == FR_OK)
								rc = f_lseek(&Fil,f_size(&Fil));
							if(rc == FR_OK)	
								rc = f_write(&Fil,&msg[6],dataLength,&bw);
							if(rc == FR_OK)
							{
								if(f_size(&Fil) == (fwLength+4))
								{
									fUpdateFW = true;
									fSendRes = true;
									cmd = FIRMWARE_PACKAGE_HEADER;
									res_value = _SUCCESS;
								}
								else
								{
									fSendRes = true;
									cmd = FIRMWARE_PACKAGE_HEADER;
									res_value = _SUCCESS;
									packetID ++;
								}
							}
							else
							{
								fSendRes = true;
								cmd = FIRMWARE_PACKAGE_HEADER;
								res_value = _ERROR;
							}
							f_close(&Fil);							
						}
					}
					break;
				case CMD_UPDATE_ETHERNET_REQ:
					createLog(SERVER,INFO_LOG,"Request reboot");
					sendResponse(CMD_REQUEST_REBOOT,SEND_REQUEST_UPDATE);
					saveLog2SDC();
					break;
				case CMD_INSERT_IMAGE:
				case CMD_SERVER_DATA_UPDATE_REQ:
					createLog(SERVER,INFO_LOG,"Request insert new image");			
					// Response to server
					fSendRes = true;
					cmd = CMD_SERVER_DATA_UPDATE_REQ;
					res_value = _SUCCESS;
					break;
				case DATA_INFO_HEADER:
					{
						u16 width = 0;
						u16 height = 0;
						u16 bpp = 0;
						u16 fileCount = 0;
						u16* fileList;
						bool fCheck = false;
					
						if(length >= 10)
						{
							memcpy(&ID,&msg[2],2);
							memcpy(&width,&msg[4],2);
							memcpy(&height,&msg[6],2);
							memcpy(&bpp,&msg[8],2);
							createLog(SERVER,INFO_LOG,"New image info \t\t\t| ID: %d, Width: %d, Height: %d, BPP: %d",ID,width,height,bpp);												
							// Check size of image (need to fit with LED's size)
							if(width>hwConfig.ledWidth || height > hwConfig.ledHeight)
							{
								fSendRes = true;
								cmd = DATA_INFO_HEADER;
								res_value = ERROR_DATA_IMAGE_FORMAT;
							}
							else
							{
								imgLength = (width*height+3)/4;
								index = 0;
								// Check is this image in used
								fileCount = countFile(imgDir);
								if(fileCount)
								{
									fileList = (u16*)m_calloc(fileCount,sizeof(u16));
									if(fileList != NULL)
									{
										fileCount = getListFile(imgDir,fileList);
										for(i = 0;i<fileCount;i++)
										{
											if(fileList[i] == ID)
											{
												fCheck = true;
												break;
											}
										}
									}
									m_free(fileList);
								}

								if(!fCheck)
								{		
									//imgBuffer = (u8*)m_calloc(imgLength+IMG_HEADER_LENGTH,sizeof(u8));
									memset(imgBuffer,0x00,SCREEN_BUFF_LENGTH);
									memcpy(imgBuffer,&msg[4],IMG_HEADER_LENGTH);
									// Response to server
									fSendRes = true;
									cmd = DATA_INFO_HEADER;
									res_value = _SUCCESS;
								}
								else
								{
									imgLength = 0;
									index = 0;
									ID = 0;
									fSendRes = true;
									cmd = DATA_INFO_HEADER;
									res_value = ERROR_ID_DUPLICATE;
								}
							}
						}
						else
						{
							createLog(SERVER,INFO_LOG,"New image info \t\t| Info error!");
							fSendRes = true;
							cmd = DATA_INFO_HEADER;
							res_value = _ERROR;
						}
					}
					
					break;
				case DATA_PACKAGE_HEADER:
					{
						u16 dataLength = 0;
						char name[20] = "";		
						u16 pkIndex = 0;

						memcpy(&pkIndex,&msg[2],2);
						memcpy(&dataLength,&msg[4],2);

						createLog(SERVER,INFO_LOG,"New image data package \t| package index: %d, length: %d bytes",pkIndex,dataLength);					
						// Check image length
						if(!imgLength)
						{							
							//m_free(imgBuffer);
							memset(imgBuffer,0x00,SCREEN_BUFF_LENGTH);
							fSendRes = true;
							cmd = DATA_PACKAGE_HEADER;
							res_value = _ERROR;
							createLog(SERVER,ERROR_LOG,"Error Image length!");					
						}
						// Check data length
						else if((dataLength!=(length - 7))&&(dataLength != ((length - 7)-(16-(dataLength+7)%16))))
						{							
							//m_free(imgBuffer);
							memset(imgBuffer,0x00,SCREEN_BUFF_LENGTH);
							fSendRes = true;
							cmd = DATA_PACKAGE_HEADER;
							res_value = _ERROR;
							createLog(SERVER,ERROR_LOG,"Error package length:dataLength=%d, length=%d",dataLength,length);	
						}
						else
						{		
							memcpy(&imgBuffer[index+IMG_HEADER_LENGTH],&msg[6],dataLength);
							index += dataLength;
							if(index >= imgLength)
							{																																	
								convert_name(ID,name,FILE_IMAGE);	
								rc = f_opendir(&Dir,imgDir);
								if(rc == FR_OK)
									rc = f_open(&Fil,&name[0],FA_CREATE_ALWAYS | FA_WRITE);
								if(rc == FR_OK)
									rc = f_write(&Fil,imgBuffer,imgLength + IMG_HEADER_LENGTH,&bw);
								f_close(&Fil);
								if(rc == FR_OK)
								{
									fSendRes = true;
									cmd = DATA_PACKAGE_HEADER;
									res_value = _SUCCESS;
								}
								else
								{
									fSendRes = true;
									cmd = DATA_PACKAGE_HEADER;
									res_value = ERROR_STORAGE_NOT_AVAILABLE;
									sysStatus |= SYSTEM_ERR_SDC;
								}
								//m_free(imgBuffer);
								memset(imgBuffer,0x00,SCREEN_BUFF_LENGTH);
								imgLength = 0;
								ID = 0;
								index = 0;
							}
							else
							{
								fSendRes = true;
								cmd = DATA_PACKAGE_HEADER;
								res_value = _SUCCESS;
							}							
						}
					}
					break;
				case CMD_SERVER_FONT_UPDATE_REQ:
					createLog(SERVER,INFO_LOG,"Request update font");
					fSendRes = true;
					cmd = CMD_SERVER_FONT_UPDATE_REQ;
					res_value = _SUCCESS;
					break;
				case FONT_INFO_HEADER:
					{
						char name[20] = "";

						//fontID = msg[3];
						//fontID = (fontID<<8 | msg[2]);	
						memcpy(&fontID,&msg[2],2);
						memcpy(&fontLength,&msg[4],4);
						createLog(SERVER,INFO_LOG,"Send info of font \t| Font ID: %d, size: %d bytes",fontID,fontLength);					
						
						convert_name(fontID,name,FILE_FONT);
						rc = f_opendir(&Dir,fontDir);
						if(rc == FR_OK)
							rc = f_unlink(name);
						if((rc == FR_OK)||(rc == FR_NO_FILE))
							rc = f_open(&Fil,name,FA_CREATE_NEW);
						if(rc == FR_OK)
						{
							fSendRes = true;
							cmd = FONT_INFO_HEADER;
							res_value = _SUCCESS;
						}
						else
						{
							fontLength = 0;
							fSendRes = true;
							cmd = FONT_INFO_HEADER;
							res_value = ERROR_STORAGE_NOT_AVAILABLE;
							sysStatus |= SYSTEM_ERR_SDC;
						}
						f_close(&Fil);
						
					}
					break;
				case FONT_PACKAGE_HEADER:
					{
						u16 dataLength = 0;
						char name[20] = "";	
						u16 pklength = 0;
						u16 pkIndex = 0;
									
						memcpy(&pkIndex,&msg[2],2);
						memcpy(&dataLength,&msg[4],2);
						pklength = length;

						createLog(SERVER,INFO_LOG,"Send font data package \t| Package index: %d, length: %d bytes",pkIndex,dataLength);
						// Check image length
						if(!fontLength)
						{
							fSendRes = true;
							cmd = FONT_PACKAGE_HEADER;
							res_value = _ERROR;
						}
						// Check data length
						else if(dataLength != (pklength - 7))
						{
							fSendRes = true;
							cmd = FONT_PACKAGE_HEADER;
							res_value = _ERROR;
						}
						else
						{
							convert_name(fontID,name,FILE_FONT);
							rc = f_opendir(&Dir,fontDir);
							if(rc == FR_OK)
								rc = f_open(&Fil,&name[0],FA_WRITE);							
							if(rc == FR_OK)
								rc = f_lseek(&Fil,f_size(&Fil));
							if(rc == FR_OK)
								rc = f_write(&Fil,&msg[6],dataLength,&bw);
							f_close(&Fil);
							if(rc == FR_OK)
							{
								index += dataLength;
								if(index >= fontLength)
								{
									rc = f_open(&Fil,&name[0],FA_READ);
									if(rc == FR_OK)
									{
										if(fontLength != f_size(&Fil))
										{
											// To do ...
											fSendRes = true;
											cmd = FONT_PACKAGE_HEADER;
											res_value = _ERROR;
											rc = f_unlink(name);
										}
										else
										{										
											fSendFont = true;
											fontSendID = fontID;
											fontSendLength = fontLength;										
//											fSendRes = false;
//											cmd = FONT_PACKAGE_HEADER;
											fSendRes = true;
											cmd = FONT_PACKAGE_HEADER;
											res_value = _SUCCESS;
										}
									}																	
									f_close(&Fil);
									fontLength = 0;
									fontID= 0;
									index = 0;
								}
								else
								{
									fSendRes = true;
									cmd = FONT_PACKAGE_HEADER;
									res_value = _SUCCESS;
								}
							}
							if(rc != FR_OK)
							{
								fSendRes = true;
								cmd = FONT_PACKAGE_HEADER;
								res_value = ERROR_STORAGE_NOT_AVAILABLE;
								sysStatus |= SYSTEM_ERR_SDC;
							}
						}
					}
					break;
				case CMD_DELETE_IMAGE:
					{
						u16 imgID = 0;
						char name[20] = "";
						RESPONSE res = _SUCCESS;
						
						//imgID = msg[3];
						//imgID = (imgID<<8 | msg[2]);
						memcpy(&imgID,&msg[2],2);

						createLog(SERVER,INFO_LOG,"Request delete image \t| Image ID: %d",imgID);	
						if(checkImgInUsed(page,imgID))
						{
							// Response error image is used
							res = _ERROR;
						}
						else
						{
							convert_name(imgID,&name[0],FILE_IMAGE);
							rc = f_unlink(&name[0]);

							if(rc == FR_OK)
							{
								res = _SUCCESS;
							}
							else if(rc == FR_NO_FILE)
							{
								res = ERROR_ID_NOT_FOUND;
							}
							else 
							{
								res = ERROR_STORAGE_NOT_AVAILABLE;
								sysStatus |= SYSTEM_ERR_SDC;
							}
						}
						sendResponse(CMD_DELETE_IMAGE,res);
					}
					break;
				case CMD_DELETE_ALL_IMAGE:
					{
						u16 fileCount = 0;
						u16* fileList;
						char name[20] = "";

						createLog(SERVER,INFO_LOG,"Request delete all image");					
						fileCount = countFile(imgDir);
						if(fileCount)
						{
							fileList = (u16*)m_calloc(fileCount,sizeof(u16));
							if(fileList != NULL)
							{
								fileCount = getListFile(imgDir,fileList);
								for(i = 0;i<fileCount;i++)
								{
									convert_name(fileList[i],name,FILE_IMAGE);
									rc = f_unlink(name);
								}
							}
							m_free(fileList);
							if(rc != FR_OK)
							{
								sendResponse(CMD_DELETE_ALL_IMAGE,_ERROR);
							}
							else
								// Response error to server
								sendResponse(CMD_DELETE_ALL_IMAGE,_SUCCESS);
						}		
						else
							// Response error to server
							sendResponse(CMD_DELETE_ALL_IMAGE,_SUCCESS);
					}
					break;
				case CMD_GET_LIST_IMAGE_INDEX:
					{
						u16 fileCount = 0;
						u16* fileList;

						createLog(SERVER,INFO_LOG,"Request get list image");											
						fileCount = countFile(imgDir);
						if(fileCount)
						{
							fileList = (u16*)m_calloc(fileCount,sizeof(u16));
							if(fileList != NULL)
							{
								fileCount = getListFile(imgDir,fileList);
								if(fileCount)
								{
									data = (u8*)m_calloc(fileCount*2 + 4,sizeof(u8));
								}
								if(data != NULL)
								{
									data[0] = DEVICE_TAG;
									data[1] = CMD_GET_LIST_IMAGE_INDEX;
									memcpy(&data[2],&fileCount,2);
									memcpy(&data[4],&fileList[0],fileCount*2);
									sendTCPData(data,fileCount*2 + 4);
									m_free(data);
								}
							}
							m_free(fileList);
						}
						else
						{
							data = (u8*)m_calloc(fileCount*2 + 4,sizeof(u8));
							data[0] = DEVICE_TAG;
							data[1] = CMD_GET_LIST_IMAGE_INDEX;
							memcpy(&data[2],&fileCount,2);
							sendTCPData(data,fileCount*2 + 4);
							m_free(data);
						}
					}
					break;
				case CMD_CREATE_PAGE:
					{
						u16 pageID = 0;
						u16 fileCount = 0;
						u16* fileList;
						bool fCheck = false;
						u16 length = 0;
						F_RETURN result = F_SUCCESS;

						memcpy(&length,&msg[2],2);												
						memcpy(&pageID,&msg[4],2);
						createLog(SERVER,INFO_LOG,"Request create page \t| Page ID: %d",pageID);
						// Check is pageID in used
						fileCount = countFile(pageDir);
						if(fileCount)
						{
							fileList = (u16*)m_calloc(fileCount,sizeof(u16));
							if(fileList != NULL)
							{
								fileCount = getListFile(pageDir,fileList);
								for(i = 0;i<fileCount;i++)
								{
									if(fileList[i] == pageID)
									{
										fCheck = true;
										break;
									}
								}
							}
							m_free(fileList);
						}
						if(!fCheck)
						{
							index = 4;
							
							result = createPage(&msg[index]);

							if(result == F_SUCCESS)
							{
								// Response error to server
								sendResponse(CMD_CREATE_PAGE,_SUCCESS);
							}
							else if(result == F_SDC_ERR)
							{
								sendResponse(CMD_CREATE_PAGE,ERROR_STORAGE_NOT_AVAILABLE);
								sysStatus |= SYSTEM_ERR_SDC;
							}
							else
							{
								sendResponse(CMD_CREATE_PAGE,_ERROR);
							}
						}
						else
						{
							sendResponse(CMD_CREATE_PAGE,ERROR_ID_DUPLICATE);
						}
					}
					break;
				case CMD_UPDATE_PAGE:
					{
						u16 pageID = 0;
						u16 fileCount = 0;
						u16* fileList;
						bool fCheck = false;						
						F_RETURN err_code = F_SUCCESS;
						
						//pageID = msg[5];
						//pageID = (pageID<<8 | msg[4]);						
						memcpy(&pageID,&msg[4],2);
						index = 6;
						
						createLog(SERVER,INFO_LOG,"Request update page \t| Page ID: %d",pageID);
						// Check is page exist
						fileCount = countFile(pageDir);
						if(fileCount)
						{
							fileList = (u16*)m_calloc(fileCount,sizeof(u16));
							if(fileList != NULL)
							{
								fileCount = getListFile(pageDir,fileList);
								for(i = 0;i<fileCount;i++)
								{
									if(fileList[i] == pageID)
									{
										fCheck = true;
										break;
									}
								}
							}
							m_free(fileList);
						}
						if(fCheck)
						{
							err_code = updatePage(pageID,&msg[index]);
							if(err_code == F_SUCCESS)
							{
								if(pageID == activePageID)
								{
									fSwapPage = true;
									cmd = CMD_UPDATE_PAGE;									
								}
								else
									// Response error to server
									sendResponse(CMD_UPDATE_PAGE,_SUCCESS);
							}
							else
								sendResponse(CMD_UPDATE_PAGE,_ERROR);
						}						
						else
							sendResponse(CMD_UPDATE_PAGE,ERROR_ID_NOT_FOUND);
					}					
					break;
				case CMD_GET_PAGE_INFO:
					{						
						u16 pageID = 0;
						char name[20] = "";
						UINT br = 0;
						
						//pageID = msg[5];
						//pageID = (pageID<<8 | msg[4]);
						memcpy(&pageID,&msg[4],2);
						createLog(SERVER,INFO_LOG,"Request get page info \t| Page ID: %d",pageID);
						convert_name(pageID,name,FILE_PAGE);
						rc = f_opendir(&Dir,pageDir);
						if(rc == FR_OK)
						{
							rc = f_open(&Fil,name,FA_READ);
						}
						if(rc == FR_OK)
						{
							data = (u8*)m_calloc(f_size(&Fil) + 4,sizeof(u8));
							// Message header
							data[0] = DEVICE_TAG;
							data[1] = CMD_GET_PAGE_INFO;
							data[2] = (f_size(&Fil)&0xFF);
							data[3] = (f_size(&Fil)>>8);
							// Page data
							f_read(&Fil,&data[4],f_size(&Fil),&br);							
							sendTCPData(data,f_size(&Fil) + 4);
							m_free(data);
						}				
						else
							sendResponse(CMD_GET_PAGE_INFO,_ERROR);
					}
					break;
				case CMD_DELETE_PAGE:
					{
						u16 pageID = 0;
						char name[20] = "";
						
						//pageID = msg[3];
						//pageID = (pageID<<8 | msg[2]);
						memcpy(&pageID,&msg[2],2);
						createLog(SERVER,INFO_LOG,"Request delete page \t| Page ID: %d",pageID);
						convert_name(pageID,&name[0],FILE_PAGE);
						rc = f_unlink(&name[0]);						
						if(rc == FR_OK)
						{
							if(pageID == activePageID)
							{
								fClearLED = true;
								//activePageID = 0;								
								cmd = CMD_DELETE_PAGE;
							}
							else
								// Response error to server
								sendResponse(CMD_DELETE_PAGE,_SUCCESS);
						}
						else if(rc == FR_NO_FILE)
						{
							// Response error to server
							sendResponse(CMD_DELETE_PAGE,ERROR_ID_NOT_FOUND);
						}
						else
						{
							// Response error to server
							sendResponse(CMD_DELETE_PAGE,ERROR_STORAGE_NOT_AVAILABLE);
							sysStatus |= SYSTEM_ERR_SDC;
						}
					}
					break;
				case CMD_GET_LIST_PAGE:
					{
						u16 fileCount = 0;
						u16* fileList;

						createLog(SERVER,INFO_LOG,"Request get list page");													
						fileCount = countFile(pageDir);
						if(fileCount)
						{
							fileList = (u16*)m_calloc(fileCount,sizeof(u16));
							if(fileList != NULL)
							{
								fileCount = getListFile(pageDir,fileList);
								if(fileCount)
								{
									data = (u8*)m_calloc(fileCount*2 + 4,sizeof(u8));
								}
								if(data != NULL)
								{
									data[0] = DEVICE_TAG;
									data[1] = CMD_GET_LIST_PAGE;
									memcpy(&data[2],&fileCount,2);
									memcpy(&data[4],&fileList[0],fileCount*2);
									sendTCPData(data,fileCount*2 + 4);
									m_free(data);
								}
							}
							m_free(fileList);
						}
						else
						{
							data = (u8*)m_calloc(fileCount*2 + 4,sizeof(u8));
							data[0] = DEVICE_TAG;
							data[1] = CMD_GET_LIST_PAGE;
							memcpy(&data[2],&fileCount,2);
							sendTCPData(data,fileCount*2 + 4);
							m_free(data);
						}	
					}
					break;
				case CMD_SET_ACTIVE_PAGE:
					{
						u16 pageID = 0;
						u16 fileCount = 0;
						u16* fileList;
						bool fCheck = false;
						
						//pageID = msg[3];
						//pageID = (pageID<<8 | msg[2]);
						memcpy(&pageID,&msg[2],2);
						createLog(SERVER,INFO_LOG,"Request active page \t| Page ID: %d",pageID);
						// Check is page exist in SD Card
						fileCount = countFile(pageDir);
						if(fileCount)
						{
							fileList = (u16*)m_calloc(fileCount,sizeof(u16));
							if(fileList != NULL)
							{
								fileCount = getListFile(pageDir,fileList);
								for(i = 0;i<fileCount;i++)
								{
									if(fileList[i] == pageID)
									{
										fCheck = true;
										break;
									}
								}
							}
							m_free(fileList);
						}
						if(fCheck)
						{
							activePageID = pageID;
							fSwapPage = true;
							cmd = CMD_SET_ACTIVE_PAGE;		
						}
						else							
							// Response error to server
							sendResponse(CMD_SET_ACTIVE_PAGE,ERROR_ID_NOT_FOUND);
					}
					break;
				case CMD_GET_ACTIVE_PAGE:
					{
						createLog(SERVER,INFO_LOG,"Request get active page");						
						data = (u8*)m_calloc(4,sizeof(u8));
						data[0] = DEVICE_TAG;
						data[1] = CMD_GET_ACTIVE_PAGE;
						data[2] = activePageID & 0xFF;
						data[3] = activePageID >> 8;

						sendTCPData(data,4);
						m_free(data);
					}
					break;			
				case CMD_INSERT_POPUP:
					{
						u16 pageID = 0;
						u16 popupID = 0;
						POPUP popupTemp;
						F_RETURN r = F_SUCCESS;
						
						//pageID = msg[5];
						//pageID = (pageID<<8 | msg[4]);
						memcpy(&pageID,&msg[4],2);
						//popupID = msg[7];
						//popupID = (popupID<<8 | msg[6]);
						memcpy(&popupID,&msg[6],2);
						createLog(SERVER,INFO_LOG,"Request insert popup \t| Page ID: %d, Popup ID: %d",pageID,popupID);				
						index = 8;
						memcpy(&popupTemp,&msg[index],POPUP_HEADER_LENGTH);
						if(popupTemp.Info.header.ID != popupID)
							sendResponse(CMD_INSERT_POPUP,_ERROR);
						else
						{
							r = createPopup(pageID,popupID,&msg[index]);

							if(r == F_SUCCESS)
							{
								if(pageID == activePageID)
								{
									//fSwapPage = true;
									fReloadPage = true;
									popupChangedID = popupID;
									cmd = CMD_INSERT_POPUP;
									fSendRes = false;
								}
								else
									sendResponse(CMD_INSERT_POPUP,_SUCCESS);
							}
							else
								sendResponse(CMD_INSERT_POPUP,_ERROR);
						}
					}
					break;
				case CMD_UPDATE_POPUP:
					{
						u16 pageID = 0;
						u16 popupID = 0;	
						F_RETURN r = F_SUCCESS;
						
						//pageID = msg[5];
						//pageID = (pageID<<8 | msg[4]);	
						memcpy(&pageID,&msg[4],2);
						//popupID = msg[7];
						//popupID = (popupID<<8 | msg[6]);
						memcpy(&popupID,&msg[6],2);
						createLog(SERVER,INFO_LOG,"Request update popup \t| Page ID: %d, Popup ID: %d",pageID,popupID);
						
						index = 8;
						r = updatePopup(pageID,popupID,&msg[index]);
						if(r == F_SUCCESS)
						{
							if(pageID == activePageID)
							{
								fReloadPage = true;
								popupChangedID = popupID;
								cmd = CMD_UPDATE_POPUP;
								fSendRes = false;
							}
							else
								sendResponse(CMD_UPDATE_POPUP,_SUCCESS);
						}
						else
						{
							switch(r)
							{
								case F_INVALID_ID:
									sendResponse(CMD_UPDATE_POPUP,ERROR_ID_NOT_FOUND);
									break;
								case F_DUPLICATE:
									sendResponse(CMD_UPDATE_POPUP,ERROR_ID_DUPLICATE);
									break;
								case F_SDC_ERR:
									sendResponse(CMD_UPDATE_POPUP,ERROR_STORAGE_NOT_AVAILABLE);
									sysStatus |= SYSTEM_ERR_SDC;
									break;
								case F_SIZE_ERR:
									sendResponse(CMD_UPDATE_POPUP,_ERROR);
									break;
								default:
									sendResponse(CMD_UPDATE_POPUP,_ERROR);
									break;
							}							
						}
					}
					break;
				case CMD_DELETE_POPUP:
					{
						u16 pageID = 0;
						u16 popupID = 0;
						F_RETURN r = F_SUCCESS;
						
						//pageID = msg[5];
						//pageID = (pageID<<8 | msg[4]);
						memcpy(&pageID,&msg[4],2);
						//popupID = msg[7];
						//popupID = (popupID<<8 | msg[6]);
						memcpy(&popupID,&msg[6],2);
						createLog(SERVER,INFO_LOG,"Request delete popup \t| Page ID: %d, Popup ID: %d",pageID,popupID);						
						r = deletePopup(pageID,popupID);
						if(r == F_SUCCESS)
						{
							if(pageID == activePageID)
							{
								fReloadPage = true;
								popupChangedID = popupID;
								cmd = CMD_DELETE_POPUP;
								fSendRes = false;
							}
							else
								sendResponse(CMD_DELETE_POPUP,_SUCCESS);
						}
						else
						{
							sendResponse(CMD_DELETE_POPUP,_ERROR);
						}
					}
					break;
				default:
					break;
			}
			break;
		default:
			createLog(SERVER,INFO_LOG,"Server send wrong command");
			sendResponse(msg[1],_ERROR);
			break;
	}	
	saveLog2SDC();
}

//===================================
void sendTCPData(u8* buf,u16 length)
{
	u16 i;

	while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) {};
	for(i = 0;i<length;i++)
	{
		USART_SendData(USART1,buf[i]);
		while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {};
	}
	USART_SendData(USART1,'\r');
	while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {};
	USART_SendData(USART1,'\n');
	while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {};
}

//===================================
u8 ComputeChecksum(u8* data, u16 length)
{
    u8 crc = 0;
	unsigned int i = 0;
	
    for(i = 0;i<length; i++)
    {
        crc = table[crc ^ data[i]];
    }

    return crc;
}

//===================================
void Crc8(void)
{
	int i = 0, j = 0;
	int temp = 0;
	
    for(i = 0; i < 256; ++i)
    {
        temp = i;
        for (j = 0; j < 8; ++j)
        {
            if (temp & 0x80)
            {
                temp = (temp << 1) ^ poly;
            }
            else
            {
                temp <<= 1;
            }
        }
        table[i] = (u8)temp;
    }
}

//===================================
void sendPopup(POPUP popup,u8 cmd)
{
	u16 i = 0;	
	UINT br;
	u16 length = 0;
	FRESULT rc;
	PLAYLIST_ITEM* playlist = (PLAYLIST_ITEM*)popup.Info.data;
	char name[20] = "";
	u8* img_data = NULL;
	u8* img_header = NULL;

	switch(cmd)
	{
		case DPL_CMD_CREATE_POPUP:
		case DPL_CMD_UPDATE_POPUP:
			switch(playlist[popup.Info.header.activeItem].header.dataType)
			{
				case PLAYLIST_IMAGE:
					{
						// Open image
						rc = f_opendir(&Dir_1,imgDir);
						if(rc == FR_OK)
						{
							u16 ID = 0;
							memcpy(&ID,playlist[popup.Info.header.activeItem].data,2);
							convert_name(ID,name,FILE_IMAGE);
							rc = f_open(&Fil_1,name,FA_READ);
						}
						if(rc == FR_OK)
						{
							// Read image header
							img_header = (u8*)m_malloc(5);
							// f_lseek(&Fil,5);
							rc = f_read(&Fil_1,&img_header[0],5,&br);
							if(br == 5)
							{
								length = (img_header[0] + img_header[1]*256)*(img_header[2] + img_header[3]*256);								
								length = (length + 3)/4;
								img_data = (u8*)m_malloc(length);
								rc = f_read(&Fil_1,&img_data[0],length,&br);
								f_close(&Fil_1);
								if(br == length)
								{
									send2Display(MAIN_TAG);
									send2Display(cmd);									
									for(i = 0;i<12;i++)
									{
										send2Display(popup.byte.b[i]);
									}
									for(i = 0;i<5;i++)
										send2Display(img_header[i]);
									
									send2Display(0x00);
									for(i = 0;i<length;i++)
									{
										send2Display(img_data[i]);
									}
									send2Display('\r');
									send2Display('\n');
								}
								m_free(img_data);
								m_free(img_header);
							}
							else
							{
								f_close(&Fil_1);
								m_free(img_header);
							}							
						}
					}
					break;
				case PLAYLIST_TEXT:
					{			
						u16 length = 0;
						
						send2Display(MAIN_TAG);
						send2Display(cmd);
						// Send popup info
						popup.Info.header.dataType = POPUP_TEXT;
						for(i = 0;i<12;i++)
						{
							send2Display(popup.byte.b[i]);
						}
						popup.Info.header.dataType = POPUP_PLAYLIST;
						length = playlist[popup.Info.header.activeItem].header.length;
						// Send text 
						send2Display(length);
						send2Display(length>>8);
						for(i = 0;i<length;i++)
							send2Display(playlist[popup.Info.header.activeItem].data[i]);
						send2Display('\r');
						send2Display('\n');
					}
					break;
				case PLAYLIST_CLOCK:
					{			
						u16 length = 0;
						
						send2Display(MAIN_TAG);
						send2Display(cmd);
						// Send popup info
						popup.Info.header.dataType = POPUP_CLOCK;
						for(i = 0;i<12;i++)
						{
							send2Display(popup.byte.b[i]);
						}
						popup.Info.header.dataType = POPUP_PLAYLIST;
						length = playlist[popup.Info.header.activeItem].header.length;
						// Send text 
						send2Display(length);
						send2Display(length>>8);
						for(i = 0;i<length;i++)
							send2Display(playlist[popup.Info.header.activeItem].data[i]);
						for(i = 0;i<7;i++)
						{
							send2Display(rtS.byte.b[i]);
						}
						send2Display('\r');
						send2Display('\n');
					}
					break;
				case PLAYLIST_TEMP:
					{			
						u16 length = 0;
						
						send2Display(MAIN_TAG);
						send2Display(cmd);
						// Send popup info
						popup.Info.header.dataType = POPUP_TEMP;
						for(i = 0;i<12;i++)
						{
							send2Display(popup.byte.b[i]);
						}
						popup.Info.header.dataType = POPUP_PLAYLIST;
						length = playlist[popup.Info.header.activeItem].header.length;
						// Send text 
						send2Display(length);
						send2Display(length>>8);
						for(i = 0;i<length;i++)
							send2Display(playlist[popup.Info.header.activeItem].data[i]);
						send2Display(sysTemp);
						send2Display('\r');
						send2Display('\n');
					}
					break;
				default:
					break;
			}
			break;
		case DPL_CMD_DELETE_POPUP:
			send2Display(MAIN_TAG);
			send2Display(cmd);
			// Send popup info
			for(i = 0;i<12;i++)
			{
				send2Display(popup.byte.b[i]);
			}
			break;
		default:
			break;
	}
}

//===================================
F_RETURN swapPage(u16 pageID)
{
	u16 i = 0;
	u8 res = 0;
	F_RETURN result = F_SUCCESS;

	createLog(MAIN_MODULE,INFO_LOG,"Swap page \t\t\t\t| Page ID: %d",pageID);
	if(page != NULL)
		freePageResource(page);
	
	page = loadPage(pageID);

	if(page != NULL)
	{
		activePageID = page->header.ID;	

		// Clear current page
		sendDisplayCmd(DPL_CMD_CLEAR_SCREEN,NULL,0);
		res = waitResponse();
		
		// Send page info
		sendDisplayCmd(DPL_CMD_SEND_INFO,&page->header.itemCount,1);
		res = waitResponse();
		
		// Send popup data	
		for(i = 0;i<page->header.itemCount;i++)
		{
			sendPopup(page->popup[i],DPL_CMD_CREATE_POPUP);
			page->popup[i].Info.header.isUpdate = true;
			res = waitResponse();
			if(res)
			{
				result = F_ERROR;				
				break;
			}
		}	
	}
	else
	{
		createLog(MAIN_MODULE,ERROR_LOG,"Load page error!");
		result = F_ERROR;
	}

	if(result == F_ERROR)
		createLog(MAIN_MODULE,ERROR_LOG,"Swap page error!");
	return result;
}

//===================================
void send2Display(u8 c)
{
	while(USART_GetFlagStatus(UART4, USART_FLAG_TXE) == RESET){};    
	USART_SendData(UART4,c);
	while(USART_GetFlagStatus(UART4, USART_FLAG_TC) == RESET){};    
}

//===================================
u8 rcvFromDisplay(void)
{
	u8 c;
	u32 i = 0;
	
	while((USART_GetFlagStatus(UART4, USART_FLAG_RXNE) == RESET)&&(i<1000))
	{
		i++;
	}

	if(i == 1000)
		c = 0xFF;
	else
		c = USART_ReceiveData(UART4);

	return c;
}

//===================================
void sendBuffer2Display(u8 *buff,u16 length)
{
	u16 i = 0;

	for(i = 0;i<length;i++)
	{
		send2Display(buff[i]);
	}
	
	send2Display('\r');
	send2Display('\n');
}

//===================================
u16 getCodeFromName(TCHAR* imgName)
{
	u16 imgCode = 0;

	while(*imgName != '.')
	{
		imgCode = imgCode*10 + (*imgName-0x30);
		imgName++;
	}

	return imgCode;
}

//===================================
u16 getListFile(char *path,u16* codeArray)
{
	u16 count = 0;
	FRESULT rc;

	rc = f_opendir(&Dir,path);                       		/* Open the directory */
	if (rc == FR_OK) 
	{
	    for (;;) 
		{
	        rc = f_readdir(&Dir, &fno);                   	/* Read a directory item */
	        if (rc != FR_OK || fno.fname[0] == 0) break;  	/* Break on error or end of dir */
	        if (fno.fname[0] == '.') continue;             	/* Ignore dot entry */
	        if(!(fno.fattrib & AM_DIR)) 					/* It is a file */
			{                    
				*codeArray = getCodeFromName(fno.fname);
				codeArray ++;
				count ++;
	        } 
		}
	}

	return count;
}	

//===================================
u16 countFile(char *path)
{
	u16 count = 0;
	FRESULT rc;
	
	rc = f_opendir(&Dir,path);                       		/* Open the directory */
	if (rc == FR_OK) 
	{
	    for (;;) 
		{
	        rc = f_readdir(&Dir, &fno);                   	/* Read a directory item */
	        if (rc != FR_OK || fno.fname[0] == 0) break;  	/* Break on error or end of dir */
	        if (fno.fname[0] == '.') continue;             	/* Ignore dot entry */
	        if(!(fno.fattrib & AM_DIR)) 					/* It is a file */
			{                    
				count ++;
	        } 
		}
	}

	return count;
}

//===================================
void sendResponse(u8 cmdID,u8 res)
{
	u8 msg[5];
	
	msg[0] = DEVICE_TAG;
	msg[1] = cmdID;
	msg[2] = res;
	
	sendTCPData(msg,3);	
	if(res == _SUCCESS)
	{
		createLog(MAIN_MODULE,RESPONSE_LOG,"Device response \t\t| SUCCESS");
	}
	else if((cmdID != CMD_REQUEST_REBOOT)&&(cmdID != CMD_REQUEST_RECONNECT))
	{
		switch(res)
		{
			case  _ERROR:
				createLog(MAIN_MODULE,ERROR_LOG,"Device response \t\t| Error");
				break;
			case ERROR_ID_DUPLICATE:
				createLog(MAIN_MODULE,ERROR_LOG,"Device response \t\t| ERROR_ID_DUPLICATE");
				break;
			case ERROR_ID_NOT_FOUND:
				createLog(MAIN_MODULE,ERROR_LOG,"Device response \t\t| ERROR_ID_NOT_FOUND");
				break;
			case ERROR_STORAGE_NOT_AVAILABLE:
				createLog(MAIN_MODULE,ERROR_LOG,"Device response \t\t| ERROR_STORAGE_NOT_AVAILABLE");
				break;
			case ERROR_DATA_IMAGE_FORMAT:
				createLog(MAIN_MODULE,ERROR_LOG,"Device response \t\t| ERROR_DATA_IMAGE_FORMAT");
				break;
			case ERROR_PACKAGE_ID:
				createLog(MAIN_MODULE,ERROR_LOG,"Device response \t\t| ERROR_PACKAGE_ID");
				break;
			case ERROR_PACKAGE_CHECKSUM:
				createLog(MAIN_MODULE,ERROR_LOG,"Device response \t\t| ERROR_PACKAGE_CHECKSUM");
				break;
			case ERROR_DEVICE_BUSY:
				createLog(MAIN_MODULE,ERROR_LOG,"Device response \t\t| ERROR_DEVICE_BUSY");
				break;
			default:
				break;
		}		
	}
}

//===================================
void checkLed(void)
{
	
}

//===================================
void captureScreen(void)
{
	u16 i = 0;
	u8 res = 0;
	u8 buff[10];
	u16 count = 0;
	u16 index = 0;
	u8 pkCount = 0;
	u32 timeout = 0;
	u16 dataLength = 0;

	createLog(MAIN_MODULE,INFO_LOG,"Start capture screen");
	captureCount = 0;
	memset(imgBuffer,0x00,SCREEN_BUFF_LENGTH);
	// Send Request command
	sendDisplayCmd(DPL_CMD_CAPTURE_SCREEN,NULL,0);
	res = waitResponse();
	if(res)
	{
		sendResponse(CMD_CAPTURE_SCREEN,_ERROR);
	}
	else
	{
		fCaptureData = true;
		count = 0;
		while(fCaptureData && count<100)
		{
			delayms(50);
			count++;
		}
		if(count >= 100)
		{
			createLog(MAIN_MODULE,ERROR_LOG,"Capture screen timeout!");
			sendResponse(CMD_CAPTURE_SCREEN,_ERROR);
			captureCount = 0;
			memset(imgBuffer,0x00,SCREEN_BUFF_LENGTH);
			fCaptureData = false;
		}
		else
		{
			createLog(MAIN_MODULE,INFO_LOG,"Send capture data \t\t| LedWidth: %d, LedHeight: %d, Total package: %d",
						hwConfig.ledWidth, hwConfig.ledHeight,((ledDataLength+999)/1000));
			// Send data to server
			buff[0] = DEVICE_TAG;
			buff[1] = LED_INFO_HEADER;
			buff[2] = hwConfig.ledWidth;
			buff[3] = hwConfig.ledWidth>>8;
			buff[4] = hwConfig.ledHeight;
			buff[5] = hwConfig.ledHeight>>8;
			buff[6] = (ledDataLength+999)/1000;
			buff[7] =(u8)(1000&0xFF);
			buff[8] =(u8)(1000>>8);
			sendTCPData(buff,9);
			
			delayms(1000);
		
			count = ledDataLength;
			index = 0;
			while(count != 0)
			{			
				// Reload IWDG counter 
   				IWDG_ReloadCounter();
				
				fWaitRes = false;
				buff[0] = DEVICE_TAG;
				buff[1] = LED_SCREEN_DATA;
				buff[2] = pkCount;
				buff[3] = 0;
				for(i = 0;i<4;i++)
				{
					USART_SendData(USART1,buff[i]);
					while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) 
					{ }
				}			
				
				if(count>1000)
					dataLength = 1000;					
				else
					dataLength = count;
				
				createLog(MAIN_MODULE,INFO_LOG,"Capture data package \t| Package ID: %d, Length: %d",pkCount,dataLength);
												
				sendTCPData(&imgBuffer[index],dataLength);				
								
				timeout = 0;
				while((!fWaitRes)&&(timeout<500))
				{
					delayms(10);
					timeout++;
				}
				if((server_res != _SUCCESS)||(timeout>=500))
				{
					if(timeout>500)
						createLog(SERVER,ERROR_LOG,"Wait server response timeout!");
					else
						createLog(SERVER,ERROR_LOG,"Server response error!");
					//break;			
				}
				else
				{
					pkCount ++;
					index += dataLength;
					count -=dataLength;	
					createLog(SERVER,INFO_LOG,"Server response OK!");
				}
			}		
		}
	}	
	createLog(MAIN_MODULE,INFO_LOG,"End capture screen!");
	saveLog2SDC();
}

//===================================
void sendFont(u8 fontID,u16 length)
{
	u16 i = 0;
	char name[20] = "";
	u8 fontData[100];
	UINT br;
	FRESULT rc;

	createLog(MAIN_MODULE,INFO_LOG,"Update font for display module \t| FontID: %d, length: %d bytes",fontID,length);
	convert_name(fontID,name,FILE_FONT);
	rc = f_opendir(&Dir_1,fontDir);
	rc = f_open(&Fil_1,(const TCHAR *)name,FA_READ);
	if(rc == FR_OK)
	{
		// Send Request command
		send2Display(MAIN_TAG);
		send2Display(DPL_CMD_SEND_FONT);
		send2Display(fontID);
		send2Display(length);
		send2Display(length>>8);
		do
		{
			f_read(&Fil_1,fontData,100,&br);
			for(i = 0;i<br;i++)
			{
				send2Display(fontData[i]);
			}
		}while(br != 0);
		send2Display('\r');
		send2Display('\n');
	}
	f_close(&Fil_1);
	createLog(MAIN_MODULE,INFO_LOG,"Update font success!");
}

//===================================
u8 sendPredefined(u16 imgID,u8 order)
{
	u16 i = 0;
	char name[20] = "";
	u16 length = 0;
	u8 img_header[5];
	u16 imgWidth = 0,imgHeight = 0; 
	UINT br;
	FRESULT rc;

	createLog(MAIN_MODULE,INFO_LOG,"Update Display's ROM Predefined \t| Image ID: %d, predefined ID: %d",imgID,order);
	convert_name(imgID,name,FILE_IMAGE);
	rc = f_opendir(&Dir_1,imgDir);
	rc = f_open(&Fil_1,(const TCHAR *)name,FA_READ);	
	
	if(rc == FR_OK)
	{
		// Check image info
		rc = f_read(&Fil_1,&img_header[0],IMG_HEADER_LENGTH,&br);
		if(br == IMG_HEADER_LENGTH)
		{
			memcpy(&imgWidth,&img_header[0],2);
			memcpy(&imgHeight,&img_header[2],2);			
			if((imgWidth != hwConfig.ledWidth)||(imgHeight != hwConfig.ledHeight))
			{
				createLog(MAIN_MODULE,ERROR_LOG,"Image's size not fit with predefined image's size!");
				return 1;
			}
			// Send Request command
			send2Display(MAIN_TAG);
			send2Display(DPL_CMD_SEND_PREDEFINE);		
			send2Display(order);
			length = Fil_1.fsize - IMG_HEADER_LENGTH;
			send2Display(length);
			send2Display(length>>8);
			buffer = (u8*)m_malloc(sizeof(u8)*length);
			memset(buffer,0x00,length); 
			f_lseek(&Fil_1,IMG_HEADER_LENGTH);
			f_read(&Fil_1,buffer,length,&br);
			for(i = 0;i<br;i++)
			{
				send2Display(buffer[i]);
			}
			send2Display('\r');
			send2Display('\n');
			m_free(buffer);
		}				
	}
	f_close(&Fil_1);
	if(rc == FR_OK)
	{
		createLog(MAIN_MODULE,INFO_LOG,"Update ROM's predefined success!");
		return 0;
	}
	else
	{
		createLog(MAIN_MODULE,ERROR_LOG,"Read image file error!");
		return 1;
	}	
}

//===================================
u16 findPage(PAGE_TYPE type)
{
	u16 pageID = 0;
	u16 fileCount = 0;
	u16* fileList;
	u16 i = 0;
	char name[20] = "";
	u8 pageHeader[4];
	DIR Dir;
	FIL Fil;
	UINT br;

	fileCount = countFile(pageDir);
	if(fileCount > 0)
	{
		fileList = (u16*)m_calloc(fileCount,sizeof(u16));
		if(fileList != NULL)
		{
			fileCount = getListFile(pageDir,fileList);
			for(i = 0;i<fileCount;i++)
			{
				memset(name,0x00,20);
				convert_name(fileList[i],name,FILE_PAGE);
				f_opendir(&Dir,pageDir);
				f_open(&Fil,(const TCHAR *)name,FA_READ);
				f_read(&Fil,pageHeader,4,&br);
				if(br == 4)
				{
					if(pageHeader[3] == type)
					{
						pageID = fileList[i];
						break;
					}
				}
			}
		}
		m_free(fileList);
	}
		
	return pageID;
}

//===================================
u8 waitResponse(void)
{
	u32 count = 0;
	u8 result = 0;

	fSlaveRes = false;
	while((!fSlaveRes) && count < 0xFFFFF)
	{		
		count ++;
	}

	if(count >= 0xFFFFF) 
	{
		createLog(MAIN_MODULE,ERROR_LOG,"Display not response!");
		result = 1;
	}
	else
	{		
		fSlaveRes = false;
		switch(slaveResValue)
		{
			case DPL_RES_OK:
				createLog(MAIN_MODULE,INFO_LOG,"Display response OK!");
				result = 0;
				break;
			case DPL_RES_ERROR:
				createLog(MAIN_MODULE,ERROR_LOG,"Display response error!");
				result = 1;
				break;
			default:
				break;
		}
		slaveResValue = 0;		
	}
	
	return result;
}

//===================================
char *convert(unsigned int num, int base)
{
	static char buff[10];
	char *ptr;

	memset(buff,0x00,10);
	ptr=&buff[sizeof(buff)-1];
	*ptr='\0';
	do
	{
		*--ptr="0123456789ABCDEF"[num%base];
		num/=base;
	}while(num!=0);
	return(ptr);
}

//===================================
u8 covertHexToDec(u8 hex)
{
	return (hex % 16) + (int)(hex / 16) * 10;
}


u8 covertDecToHex(u8 dec)
{
	return dec % 10 + (dec / 10) * 16;
}

//===================================
/* Read U_ID register */
void uid_read(struct u_id *id)
{
    id->off0 = MMIO16(U_ID + 0x0);
    id->off2 = MMIO16(U_ID + 0x2);
    id->off4 = MMIO32(U_ID + 0x4);
    id->off8 = MMIO32(U_ID + 0x8);
}

//===================================
void sendWarning(WARNING_CODE wn)
{	
	while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) {};
	USART_SendData(USART1,SEND_WARNING_HEADER);
	while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {};
	USART_SendData(USART1,DEVICE_TAG);
	while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {};
	USART_SendData(USART1,wn);
	while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {};
	USART_SendData(USART1,'\r');
	while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {};
	USART_SendData(USART1,'\n');
	while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {};
	switch(wn)
	{
		case WARNING_TOO_HOT:
			createLog(MAIN_MODULE,INFO_LOG,"Device warning \t\t| WARNING_TOO_HOT");
			break;
		case WARNING_DAMAGED:
			createLog(MAIN_MODULE,INFO_LOG,"Device warning \t\t| WARNING_DAMAGED");
			break;
		case WARNING_OUT_OF_POWER:
			createLog(MAIN_MODULE,INFO_LOG,"Device warning \t\t| WARNING_OUT_OF_POWER");
			break;
		case WARNING_OPENNED:
			createLog(MAIN_MODULE,INFO_LOG,"Device warning \t\t| WARNING_OPENNED");
			break;
		case WARNING_SIM_ERR:
			createLog(MAIN_MODULE,INFO_LOG,"Device warning \t\t| WARNING_SIM_ERR");
			break;
		default:
			break;
	}
}

//===================================
bool checkImgInUsed(PAGE* page,u16 imgID)
{
	u16 i=0,j=0;
	u16 ID = 0;
	bool result = false;

	if((page == NULL)||(activePageID == 0x00))
		result = false;
	else
	{
		for(i = 0;i<page->header.itemCount;i++)
		{
			switch(page->popup[i].Info.header.dataType)
			{
				case POPUP_PLAYLIST:
					{
						PLAYLIST_ITEM* playlist;				
						playlist = (PLAYLIST_ITEM*)page->popup[i].Info.data;
						for(j = 0;j<page->popup[i].Info.header.itemCount;j++)
						{												
							switch(playlist[j].header.dataType)
							{
								case PLAYLIST_IMAGE:											
									memcpy(&ID,playlist[j].data,2);
									if(ID == imgID)
										result = true;
									break;
								case PLAYLIST_TEXT:
								case PLAYLIST_CLOCK:
								case PLAYLIST_TEMP:								
								default:
									break;
							}
							if(result)
								break;
						}							
					}
					break;						
				default:
					break;
			}
			if(result)
				break;
		}
	}

	return result;
}

//===================================
void delayms(u32 nTime)
{
	TimingDelay = nTime;

	while(TimingDelay != 0);
}

//===================================
void sendIMEI(void)
{
	u8 msg[18];
	u16 i = 0,j = 0;
	
	// Send IMEI number through operation port
	msg[0] = DEVICE_TAG;
	msg[1] = CMD_SEND_IMEI;
	memcpy(&msg[2],hwConfig.imei,15);
	
	sendTCPData(msg,17);	
	// Delay some ms
	for(i = 0;i<50000;i++)
		for(j = 0;j<100;j++) {};
		
	// Send IMEI number through alert port
	msg[0] = SEND_WARNING_HEADER;
	msg[1] = DEVICE_TAG;
	msg[2] = CMD_SEND_IMEI;
	memcpy(&msg[3],hwConfig.imei,15);
	sendTCPData(msg,18);	
}

//===================================
void sendDisplayCmd(DISPLAY_COMMAND cmd,u8* cmdParam,u8 paramLength)
{
	u8 i = 0;

	switch(cmd)
	{
		case DPL_CMD_CLEAR_SCREEN:
			createLog(MAIN_MODULE,INFO_LOG,"Send display command \t| CMD_CLEAR_SCREEN");
			break;
		case DPL_CMD_UPDATE:
			createLog(MAIN_MODULE,INFO_LOG,"Send display command \t| CMD_UPDATE");
			break;
		case DPL_CMD_CHANGE_BRIGHTNESS:
			createLog(MAIN_MODULE,INFO_LOG,"Send display command \t| CMD_CHANGE_BRIGHTNESS");
			break;
		case DPL_CMD_SEND_DATA:
			createLog(MAIN_MODULE,INFO_LOG,"Send display command \t| CMD_SEND_DATA");
			break;
		case DPL_CMD_SEND_INFO:
			createLog(MAIN_MODULE,INFO_LOG,"Send display command \t| CMD_SEND_INFO");
			break;
		case DPL_CMD_CREATE_POPUP:
			createLog(MAIN_MODULE,INFO_LOG,"Send display command \t| CMD_CREATE_POPUP");
			break;
		case DPL_CMD_UPDATE_POPUP:
			createLog(MAIN_MODULE,INFO_LOG,"Send display command \t| CMD_UPDATE_POPUP");
			break;
		case DPL_CMD_DELETE_POPUP:
			createLog(MAIN_MODULE,INFO_LOG,"Send display command \t| CMD_DELETE_POPUP");
			break;
		case DPL_CMD_CHECK_LED:
			createLog(MAIN_MODULE,INFO_LOG,"Send display command \t| CMD_CHECK_LED");
			break;
		case DPL_CMD_CAPTURE_SCREEN:
			createLog(MAIN_MODULE,INFO_LOG,"Send display command \t| CMD_CAPTURE_SCREEN");
			break;
		case DPL_CMD_START_SCAN_LED:
			createLog(MAIN_MODULE,INFO_LOG,"Send display command \t| CMD_START_SCAN_LED");
			break;
		case DPL_CMD_STOP_SCAN_LED:
			createLog(MAIN_MODULE,INFO_LOG,"Send display command \t| CMD_STOP_SCAN_LED");
			break;
		case DPL_CMD_SEND_FONT:
			createLog(MAIN_MODULE,INFO_LOG,"Send display command \t| CMD_SEND_FONT");
			break;
		case DPL_CMD_REQUEST_REBOOT:
			createLog(MAIN_MODULE,INFO_LOG,"Send display command \t| CMD_REQUEST_REBOOT");
			break;
		case DPL_CMD_SEND_PREDEFINE:
			createLog(MAIN_MODULE,INFO_LOG,"Send display command \t| CMD_SEND_PREDEFINE");
			break;
		case DPL_CMD_PLAY_PREDEINED:
			createLog(MAIN_MODULE,INFO_LOG,"Send display command \t| CMD_PLAY_PREDEINED");
			break;
		case DPL_CMD_SEND_HWCONFIG:
			createLog(MAIN_MODULE,INFO_LOG,"Send display command \t| CMD_SEND_HWCONFIG");
			break;
		case DPL_CMD_UPDATE_FW:
			createLog(MAIN_MODULE,INFO_LOG,"Send display command \t| CMD_UPDATE_FW");
			break;
		case DPL_CMD_FW_INFO:
			createLog(MAIN_MODULE,INFO_LOG,"Send display command \t| CMD_FW_INFO");
			break;
		default:
			break;				
	}
	send2Display(MAIN_TAG);
	send2Display(cmd);
	for(i = 0;i<paramLength;i++)
		send2Display(cmdParam[i]);
	send2Display('\r');
	send2Display('\n');
}

//===================================
void updateDisplayFW(char* fileName)
{
	u8 result = 0;
	u8 ack = 0;
	u32 addr = 0;
	u32 fwLength = 0;
	u8 temp[4];
	u8* fwBuff;
	u8 crc = 0;
	u16 i = 0,j = 0;
	UINT br = 0;
	FRESULT rc = FR_OK;

	// Open firmware file in SD Card
	rc = f_open(&Fil,fileName,FA_READ);
	if(rc == FR_OK)
	{
		// Read firmware length
		f_read(&Fil,&temp,4,&br);
		memcpy((u8*)&fwLength,temp,4);
		if(fwLength == (f_size(&Fil) - 4))
		{				
			createLog(MAIN_MODULE,EVENT_LOG,"Start update firmware for display module");			
			fwBuff = m_calloc(1,FLASH_PAGE_SIZE);
			addr = 0x08004000;	

			// Reboot display module
			sendDisplayCmd(DPL_CMD_REQUEST_REBOOT,NULL,0);
			delayms(200);
			NVIC_DeInit();	
			for(i = 0;i<50000;i++)
				for(j = 0;j<10;j++) {};		
			while(fwLength > 0)
			{			
				// Reload IWDG counter 
		   		IWDG_ReloadCounter();
				
				// Read fw data
				rc = f_read(&Fil,fwBuff,FLASH_PAGE_SIZE,&br);		
				send2Display(CMD_DPL_WRITE);
				send2Display(CMD_DPL_WRITE^0xFF);
				while(USART_GetFlagStatus(UART4, USART_FLAG_RXNE) == RESET) {};
				ack = USART_ReceiveData(UART4);
				if(ack == ACK)
				{
					// Send address
					memcpy(temp,&addr,4);
					crc = 0;
					for(i = 0;i<4;i++)
					{
						send2Display(temp[i]);
						crc ^= temp[i];
					}
					send2Display(crc);
					// Wait ack
					while(USART_GetFlagStatus(UART4, USART_FLAG_RXNE) == RESET) {};
					ack = USART_ReceiveData(UART4);
				}
				if(ack == ACK)
				{
					// Send data
					crc = 0;						
					send2Display(br>>8);
					crc ^= (br>>8);
					send2Display(br);
					crc ^= (br&0xFF);
					for(i =0;i<br;i++)
					{
						send2Display(fwBuff[i]);
						crc ^= fwBuff[i];
					}
					send2Display(crc);
					// Wait ack
					while(USART_GetFlagStatus(UART4, USART_FLAG_RXNE) == RESET) {};
					ack = USART_ReceiveData(UART4);
				}
				if(ack == ACK)
				{
					fwLength -= br;
					addr += FLASH_PAGE_SIZE;
				}
				else if(ack == NACK)
				{
					break;
				}
			}
			// Check if firmware update success
			if(ack == ACK)
			{
				f_close(&Fil);
				send2Display(CMD_DPL_UPDATED);
				send2Display(CMD_DPL_UPDATED^0xFF);
				while(USART_GetFlagStatus(UART4, USART_FLAG_RXNE) == RESET) {};
				ack = USART_ReceiveData(UART4);
			}
			if(ack == ACK)
			{
				f_unlink(fileName);
				send2Display(CMD_DPL_RUNAPP);
				send2Display(CMD_DPL_RUNAPP^0xFF);
				while(USART_GetFlagStatus(UART4, USART_FLAG_RXNE) == RESET) {};
				ack = USART_ReceiveData(UART4);			
			}
			if(ack != ACK)
			{
				result = 1;				
				createLog(MAIN_MODULE,EVENT_LOG,"Update display's firmware error!");
			}
			else
			{
				createLog(MAIN_MODULE,EVENT_LOG,"Display's firmware update successful!");
			}
			saveLog2SDC();
			NVIC_Configuration();	
			for(i = 0;i<50000;i++)
				for(j = 0;j<10;j++) {};		
		}
		else
			result = 2;
	}
	else
		result = 3;

	if(result != 0)
	{
		send2Display(CMD_DPL_RUNAPP);
		send2Display(CMD_DPL_RUNAPP^0xFF);	
		delayms(1000);
	}
}

//===================================
u16 getDisplayVersion(void)
{
	u16 i = 0;
	u16 version = 0;
	
	// Disable UART4 interrupt
	USART_ITConfig(UART4, USART_IT_RXNE, DISABLE);
	for(i = 0;i<1000;i++) {};
	// Send request to get display's fw version
	sendDisplayCmd(DPL_CMD_FW_INFO,NULL,0);
	// Wait to get  2 bytes version
	while(USART_GetFlagStatus(UART4, USART_FLAG_RXNE) == RESET) {};
	version = USART_ReceiveData(UART4);
	while(USART_GetFlagStatus(UART4, USART_FLAG_RXNE) == RESET) {};
	version |= (u16)(USART_ReceiveData(UART4))<<8;
	// Re-enable UART4 interrupt
	USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);
	for(i = 0;i<1000;i++) {};
	// Return version value
	return version;
}

//===================================
void getLog(tREALTIME tStart,tREALTIME tEnd)
{
	char fileName[17];
	u32 startIndex = 0,endIndex = 0;
	UINT bw;
	UINT br;
	bool fFound = false;
	s16 first = 0,last = 0,middle = 0;
	FRESULT rc;

	saveLog2SDC();
	memset(fileName,0x00,15);
	// Convert date to file name
	convertLogName(tStart,fileName,LOG_INDEX);
	rc = f_opendir(&Dir,LOG_DIR);		
	if(rc == FR_OK)
		rc = f_open(&Fil,fileName,FA_READ);
	if(rc == FR_OK)
	{
		increaseTime(&tEnd);
		// Search start index
		memset(logData,0x00,2048);
		first = 0;
		last = (Fil.fsize/6);
		middle = (first + last)/2;
		// Search start log point
		while( first < last )
		{
			rc = f_lseek(&Fil,middle*6);	
			rc = f_read(&Fil,logData,6,&br);
			if(br) 
			{
				if((tStart.data.Hour>logData[0])|| \
				   ((tStart.data.Hour == logData[0])&&(tStart.data.Minute > logData[1])))
					first = middle + 1;
				else if((tStart.data.Hour == logData[0])&&(tStart.data.Minute == logData[1]))
				{
					memcpy((u8*)&startIndex,&logData[2],4);	
					memset(logData,0x00,6);
					fFound = true;
					break;
				}
				else
					last = middle - 1;
					
			}			
			middle = (first + last)/2;
			memset(logData,0x00,6);
		}
		if(first>=last)
		{
			rc = f_lseek(&Fil,first*6);	
			rc = f_read(&Fil,logData,6,&br);
			if(br) 
			{
				memcpy((u8*)&startIndex,&logData[2],4);				
				memset(logData,0x00,6);
				fFound = true;	
			}	
		}
		if(fFound)
		{			
			last = (Fil.fsize/6);
			middle = (first+last)/2;
			// Search end log point
			while( first < last )
			{
				rc = f_lseek(&Fil,middle*6);	
				rc = f_read(&Fil,logData,6,&br);
				if(br) 
				{
					if((tEnd.data.Hour>logData[0])|| \
				  	   ((tEnd.data.Hour == logData[0])&&(tEnd.data.Minute > logData[1])))
						first = middle + 1;
					else if((tEnd.data.Hour == logData[0])&&(tEnd.data.Minute == logData[1]))
					{
						memcpy((u8*)&endIndex,&logData[2],4);
						memset(logData,0x00,6);
						break;
					}
					else
						last = middle - 1;
						
				}			
				middle = (first + last)/2;
				memset(logData,0x00,6);
			}
		}
		if(first>=last)
		{
			if(Fil.fsize == (last*6))
				endIndex = 0xFFFFFFFF;
			else
			{
				rc = f_lseek(&Fil,last*6);	
				rc = f_read(&Fil,logData,6,&br);
				if(br) 
				{
					memcpy((u8*)&endIndex,&logData[2],4);				
					memset(logData,0x00,6);
				}	
			}
		}
		f_close(&Fil);
		if(endIndex != 0)
		{			
			fileName[13] = 't';
			fileName[14] = 'x';
			fileName[15] = 't';
			rc = f_open(&Fil,fileName,FA_READ);
			rc = f_unlink("log.txt");
			rc = f_open(&Fil_1,"log.txt",FA_CREATE_ALWAYS | FA_WRITE);
			f_lseek(&Fil,startIndex);
			while(Fil.fptr<endIndex)
			{
				if((endIndex - Fil.fptr)>=2048)
					f_read(&Fil,logData,2048,&br);
				else
					f_read(&Fil,logData,(endIndex - Fil.fptr),&br);
				if(br)
				{
					rc = f_write(&Fil_1,logData,br,&bw);
					memset(logData,0,2048);
				}
				else
					break;
			}
			f_close(&Fil_1);
			f_close(&Fil);
		}
	}
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
