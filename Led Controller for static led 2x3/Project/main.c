/* Includes ------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "ledMatrix.h"
#include "HardwareConfig.h"
#include "common.h"
#include "font.h"
#include "c_func.h"


#define FW_VERSION		117		// Version 1.1.0

typedef enum
{
	POPUP_IMAGE=1,
	POPUP_CLOCK,
	POPUP_TEMP,
	POPUP_TABLE,
	POPUP_TEXT,
} POPUP_TYPE;

#define MAX_BRIGHTNESS_LEVEL	8
#define MIN_BRIGHTNESS_LEVEL	1

#define IMG1_ADDR		0x08049000
#define IMG2_ADDR		0x0804B000
#define IMG3_ADDR		0x0804D000
#define IMG4_ADDR		0x0804F000
#define IMG5_ADDR		0x08051000
#define IMG6_ADDR		0x08053000
#define IMG7_ADDR		0x08055000
#define IMG8_ADDR		0x08057000 

#define HWCONFIG_ADDR	0x0805A000

u8* ledData;
u8 rcvData[MAX_PACKAGE_LENGTH];
u16 rcvCount = 0;
POPUP_LED *popup;
u8 popup_count = 0;
u8 popup_index = 0;
u8 t = 0;
u32 ledDelay = 0;
WATCH watch;
bool fScanLED = false;
bool fStopScan = false;
bool fCheckLED = false;
bool fPlayPredef = false;
bool fUpdateFW = false;
bool fChangeData = false;
extern bool fReceived;
u16 brightness = MIN_BRIGHTNESS_LEVEL;
u8 predefIndex = 0;
HW_CONFIG hwConfig;
tREALTIME rtS  = {0x00,0x58,0x10,0x02,0x18,0x05,0x12};
u8 fontClock[12] = "";
FONT_ID fClk =FONT_ID1;
u8 fontTemp[13] = "";
FONT_ID fTemp =FONT_ID1;
u32 TimingDelay = 0;
u32 speed = 30;
extern u8 delay_count;
extern u16 uart_timeout;

/* Private function prototypes -----------------------------------------------*/
void send2Main(u8* data,u16 length);
u8 updatePage(POPUP_LED* popup);
void mixMatrix(u8* source,u16 w_src,u16 h_src,u8* des,u16 w_des,u16 h_des,u16 x_pos,u16 y_pos,LEDColor color);
u16 getMatrix(u8* source,u16 w_src,u8* des,u16 w_des,u16 h_des,u16 x,u16 y);
void clearPopupArea(POPUP_LED* popup);
void deinitPopupHeader(POPUP_LED* popup);
u8 createPopup(u8* data);
u8 updatePopup(u16 popupID,u8* data);
u8 deletePopup(u16 popupID);
void clearPopup(u16 popupID);
u8 getMatrixPixel(u8* buff,u16 w_buff,u16 x,u16 y);
void setMatrixPixel(u8* buff,u16 w_buff,u16 x,u16 y,u8 value);
u8 displayClock(POPUP_LED popup,LEDColor color,u8 dot);
u8 displayTemp(POPUP_LED popup,LEDColor color);
void savePredefinedImg(u8 ID,u8* data,u16 length);
void saveHWConfig(HW_CONFIG config);
void loadHWConfig(HW_CONFIG* config);
void scanLED(void);
void checkLed(void);
bool CheckRetangles(POPUP_LED a, POPUP_LED b); 
/* Private functions ---------------------------------------------------------*/

/**
  * @brief   Main program
  * @param  None
  * @retval None
  */
	
int main(void)
{
	u16 i = 0;
	
	RCC_Configuration();
	loadHWConfig(&hwConfig);
	GPIO_Configuration();
	USART_Configuration();
	TIMER_Configuration();
	SysTick_Config(SystemCoreClock/1000);
	NVIC_Configuration();
	
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
	
	ledData = (u8*)m_malloc(sizeof(u8)*LED_DATA_LENGTH);
	memset(ledData,0x00,LED_DATA_LENGTH);

	fChangeData = true;
	brightness = MIN_BRIGHTNESS_LEVEL;
	ledDelay = (brightness*(hwConfig.ledWidth/4))/MAX_BRIGHTNESS_LEVEL;
	watch.active = false;
	watch.dot = false; 
	fPlayPredef = true;
	predefIndex = 0;		
	loadPredefinedImg(predefIndex);
	//fScanLED = true;
		
	while(1)
	{
		// Reload IWDG counter 
   	IWDG_ReloadCounter();		
		if(uart_timeout >= 2)
		{
			// Reset global variables
			memset(rcvData,0x00,rcvCount);
			rcvCount = 0;
			uart_timeout = 0;
			fReceived = false;
		}
		
//		if(fChangeData)
//		{
//			convertData(ledData);
//			display(ledData);
//			fChangeData = false;
//		}
		if(fScanLED)
		{
			bool temp = watch.active;
			for(i = 0;i<popup_index;i++)
		  	{
		  		popup[i].info.isUpdated = false;	
			}
			watch.active = false;
			while(!fStopScan)
			{
				IWDG_ReloadCounter();
				scanLED();
			}			
			memset(ledData,0x00,LED_DATA_LENGTH);
			fChangeData = true;
			watch.active = temp;
			fStopScan = false;
			fScanLED = false;		
		}
		if(fCheckLED)
		{
			bool temp = watch.active;
			for(i = 0;i<popup_index;i++)
		  	{
		  		popup[i].info.isUpdated = false;	
			}
			memset(ledData,0x00,LED_DATA_LENGTH);
			watch.active = false;
			checkLed();
			watch.active = temp;
			
			fCheckLED = false;		
		}
	}
}
//========================================
void CmdExecute(void)
{
	u32 i = 0;
	u16 id = 0;
	u32 popup_size = 0;
	u8 response = DPL_NOT_RESPONSE;
	u8 err_code = 0;
	
	if(rcvData[0] == MAIN_TAG)
	{				
		switch(rcvData[1])
		{						
			case DPL_CMD_FW_INFO:
				{
					u16 fwVersion = 0;

					fwVersion = (u16)FW_VERSION;
					send2Main((u8*)&fwVersion,2);
				}
				break;
			case DPL_CMD_SEND_PREDEFINED:
				{						
					u8 order = 0;
					u16 length = 0;
											
					order = rcvData[MSG_HEADER_LENGTH];
					memcpy(&length,&rcvData[MSG_HEADER_LENGTH + 1],2);					
					savePredefinedImg(order,&rcvData[MSG_HEADER_LENGTH + 3],length);
					response = DPL_RES_OK;
				}
				break;
			case DPL_CMD_PLAY_PREDEINED:
				watch.active = false;
				fPlayPredef = true;
				predefIndex = 0;
				loadPredefinedImg(predefIndex);
				delay_count = 0;
				response = DPL_RES_OK;
				break;
			case DPL_CMD_CHECK_LED:
				fPlayPredef = false;
				fCheckLED = true;
				break;
			case DPL_CMD_REQUEST_REBOOT:
				// Clear LED
				OE_Control(false);
				
				NVIC_SystemReset();
				break;
			case DPL_CMD_SEND_HWCONFIG:
				{
					HW_CONFIG configTemp;
					
					memcpy(&configTemp.ledWidth,&rcvData[MSG_HEADER_LENGTH],2);
					memcpy(&configTemp.ledHeight,&rcvData[MSG_HEADER_LENGTH + 2],2);
					memcpy(&configTemp.ledType,&rcvData[MSG_HEADER_LENGTH + 4],1);
					if(configTemp.ledType != hwConfig.ledType)
					{
						if((configTemp.ledType >= MIN_SCAN_TYPE)&&(configTemp.ledType <= MAX_SCAN_TYPE))
						{
							saveHWConfig(configTemp);
							NVIC_SystemReset();
						}
					}
					else
						response = DPL_RES_ERROR;
				}
				break;
			case DPL_CMD_SEND_FONT:
				{
					u8 fontID = 0;
					u16 length = 0;
					u8* fontData = NULL;
					u32 font_addr = 0;						
					
					fontID = rcvData[MSG_HEADER_LENGTH];
					memcpy(&length,&rcvData[MSG_HEADER_LENGTH + 1],2);
					fontData = (u8*)m_malloc(sizeof(u8)*length);
					if(fontData != NULL)
					{
						memcpy(fontData,&rcvData[MSG_HEADER_LENGTH + 3],length);
						switch(fontID)
						{
							case FONT_ID1:
								font_addr = FONT_ID1_ADDR;
								break;
							case FONT_ID2:
								font_addr = FONT_ID2_ADDR;
								break;
							case FONT_ID3:
								font_addr = FONT_ID3_ADDR;
								break;
							case FONT_ID4:
								font_addr = FONT_ID4_ADDR;
								break;
							case FONT_ID5:
								font_addr = FONT_ID5_ADDR;
								break;
							case FONT_ID6:
								font_addr = FONT_ID6_ADDR;
								break;
							default:
								font_addr = 0;
								break;
						}
						if(font_addr != 0)
						{
							saveFont(font_addr,fontData,length);
							response = DPL_RES_OK;
						}
						else
						{
							response = DPL_RES_ERROR;
						}
						m_free(fontData);
					}
					else
					{
						response = DPL_RES_ERROR;
					}
				}
				break;			
			case DPL_CMD_START_SCAN_LED:
				fScanLED = true;
				response = DPL_RES_OK;
				break;
			case DPL_CMD_STOP_SCAN_LED:
				fStopScan = true;
				response = DPL_RES_OK;
				break;
			case DPL_CMD_CAPTURE_SCREEN:
				{
					// Send led status to server
					response = DPL_RES_OK;
					send2Main(&response,1);
					response = DPL_NOT_RESPONSE;
					// Delay some us
					for(i = 0;i<50000;i++) {};
					send2Main(ledData,LED_DATA_LENGTH);
				}
				break;
			case DPL_CMD_CLEAR_SCREEN:
				{
					POPUP_LED popup_temp;
					
					// Free resource used by Popups
					for(i = 0;i<popup_index;i++)
					{
						if(popup[i].info.dataType == POPUP_IMAGE)
						{
							IMAGE* img = (IMAGE*)popup[i].info.data;								
							m_free(img->data);
							m_free(popup[i].info.data);
						}							
						else if(popup[i].info.dataType == POPUP_TEXT)
						{
							LED_STRING* ledStr = NULL;																
							ledStr = (LED_STRING*)popup[i].info.data;								
							m_free(ledStr->str);																	
							m_free(ledStr->strImg->data);
							m_free(ledStr->strImg);																
							m_free(popup[i].info.data);
						}
					}													
					// Clear global variables						
					if(popup_index>0)
						m_free(popup);						
					popup_index = 0;
					popup_count = 0;
					watch.active = false;
					fPlayPredef = false;
					predefIndex = 0;
					delay_count = 0;
					// Clear screen
					popup_temp.info.height = DEFAULT_LED_HEIGHT;
					popup_temp.info.width = DEFAULT_LED_WIDTH;
					popup_temp.info.x = 0;
					popup_temp.info.y = 0;
					clearPopupArea(&popup_temp);
					response = DPL_RES_OK;
				}
				break;
			case DPL_CMD_UPDATE:						
				break;
				
			case DPL_CMD_CHANGE_BRIGHTNESS:
				{
					u16 b = 0;
					
					memcpy(&b,&rcvData[MSG_HEADER_LENGTH],2);
					if(b>MAX_BRIGHTNESS_LEVEL || b<MIN_BRIGHTNESS_LEVEL)
						response = DPL_RES_ERROR;
					else
					{
						brightness = b;
						ledDelay = (brightness*(hwConfig.ledWidth/4))/MAX_BRIGHTNESS_LEVEL;
						response = DPL_RES_OK;
					}
				}
				break;
				
			case DPL_CMD_SEND_INFO:
				{
					POPUP_LED popup_temp;
					// Clear screen
					popup_temp.info.height = DEFAULT_LED_HEIGHT;
					popup_temp.info.width = DEFAULT_LED_WIDTH;
					popup_temp.info.x = 0;
					popup_temp.info.y = 0;
					clearPopupArea(&popup_temp);
					
					popup_count = rcvData[MSG_HEADER_LENGTH]; 
					//popup_size = popup_count*sizeof(POPUP_LED);
					popup = (POPUP_LED*)m_calloc(popup_count,sizeof(POPUP_LED));
					response = DPL_RES_OK;
				}
				break;

			case DPL_CMD_CLEAR_POPUP:				
				memcpy(&id,&rcvData[MSG_HEADER_LENGTH],2);
				clearPopup(id);
				response = DPL_RES_OK;
				break;
			case DPL_CMD_DELETE_POPUP:
				memcpy(&id,&rcvData[MSG_HEADER_LENGTH],2);
				deletePopup(id);
				response = DPL_RES_OK;
				break;
				
			case DPL_CMD_UPDATE_POPUP:
				memcpy(&id,&rcvData[MSG_HEADER_LENGTH],2);
				updatePopup(id,&rcvData[MSG_HEADER_LENGTH]);
				response = DPL_RES_OK;
				break;
				
			case DPL_CMD_CREATE_POPUP:
				memcpy(&id,&rcvData[MSG_HEADER_LENGTH],2);
				// Check popup ID
				for(i = 0;i<popup_index;i++)
				{
					if(popup[i].info.ID == id)
					{
						break;
					}
				}
				if(i < popup_index)
				{
					// This ID is in used
					response = DPL_RES_ERROR;
				}
				else
				{
					if(popup_index >= popup_count)
					{
						void* ptr = NULL;
						popup_count ++;
						popup_size = popup_count*sizeof(POPUP_LED);
						ptr = (POPUP_LED*)m_realloc(popup,popup_size);
						if(ptr != NULL)
						{
							popup = ptr;
							err_code = createPopup(&rcvData[MSG_HEADER_LENGTH]);
							if(!err_code)
							{
								response = DPL_RES_OK;
								popup_index++;
							}
							else
								response = DPL_RES_ERROR;
						}
						else
							response = DPL_RES_ERROR;
					}
					else
					{
						err_code = createPopup(&rcvData[MSG_HEADER_LENGTH]);
						if(!err_code)
						{
							response = DPL_RES_OK;
							popup_index++;
						}
						else
							response = DPL_RES_ERROR;
					}
				}				
				break;					
			default:
				response = DPL_RES_ERROR;
				break;
		}
	}		
	else
		response = DPL_RES_ERROR;
	// Reset global variables
	memset(rcvData,0x00,rcvCount);
	rcvCount = 0;
	if(response != DPL_NOT_RESPONSE)
		send2Main(&response,1);
}

//========================================
void send2Main(u8* data,u16 length)
{
	u16 i;

	for(i = 0;i<length;i++)
	{
		while(((USART1->SR & USART_FLAG_TXE)==RESET));
		// Send data
		USART1->DR = data[i];
		while(((USART1->SR & USART_FLAG_TC)==RESET));
	} 
}

//========================================
u8 updatePage(POPUP_LED* popup)
{
	clearPopupArea(popup);	
	switch(popup->info.dataType)
	{
		case POPUP_IMAGE:
			{
				IMAGE* img = (IMAGE*)popup->info.data;
				u16 width = 0,height = 0;
				// Check size of image to display fit with size of popup 
				width = (img->imgWidth<popup->info.width)?img->imgWidth:popup->info.width;
				height = (img->imgHeight<popup->info.height)?img->imgHeight:popup->info.height; 
					
				if(img->effectType == EFFECT_NONE)
				{
					if((width<img->imgWidth)||(height<img->imgHeight))
					{
						u8* data_temp = NULL;
						u16 length = 0;
						length = (width*height + 3)/4;
						data_temp = (u8*)m_calloc(length,sizeof(u8));
						getMatrix(img->data,img->imgWidth,data_temp,width,height,0,0);
						mixMatrix(data_temp,width,height,ledData,DEFAULT_LED_WIDTH,DEFAULT_LED_HEIGHT,popup->info.x,popup->info.y,LED_COLOR_NONE);
						m_free(data_temp);
					}
					else
						mixMatrix(img->data,img->imgWidth,img->imgHeight,ledData,DEFAULT_LED_WIDTH,DEFAULT_LED_HEIGHT,popup->info.x,popup->info.y,LED_COLOR_NONE);
				}
			}
			break;
		case POPUP_CLOCK:
			watch.x = popup->info.x;
			watch.y = popup->info.y;
			watch.color = LED_COLOR_RED;
			watch.active = true;
			displayClock(popup[0],watch.color,watch.dot);
			break;
		case POPUP_TEMP:
			//x = popup->info.x;
			//y = popup->info.y;
			displayTemp(popup[0],LED_COLOR_RED);
			break;
		case POPUP_TEXT:
			{
				LED_STRING* ledStr = (LED_STRING*)popup->info.data;
				u16 width = 0,height = 0;
				u16 x = 0;						
				
				// Check size of image to display fit with size of popup 
				width = (ledStr->strImg->imgWidth<popup->info.width)?ledStr->strImg->imgWidth:popup->info.width;
				height = (ledStr->strImg->imgHeight<popup->info.height)?ledStr->strImg->imgHeight:popup->info.height; 
					
				if(ledStr->strImg->effectType == EFFECT_NONE)
				{				
					if(ledStr->strImg->imgWidth<popup->info.width)
					{
						x = popup->info.x + (popup->info.width - ledStr->strImg->imgWidth)/2;
					}
					else
						x = popup->info.x;
					
					if((width<ledStr->strImg->imgWidth)||(height<ledStr->strImg->imgHeight))
					{
						u8* data_temp = NULL;
						u16 length = 0;
						length = (width*height + 3)/4;
						data_temp = (u8*)m_calloc(length,sizeof(u8));
						if(data_temp != NULL)
						{
							getMatrix(ledStr->strImg->data,ledStr->strImg->imgWidth,data_temp,width,height,0,0);
							mixMatrix(data_temp,width,height,ledData,DEFAULT_LED_WIDTH,DEFAULT_LED_HEIGHT,x,popup->info.y,LED_COLOR_NONE);
							m_free(data_temp);
						}
						else
							return 1;
					}
					else
					{
						mixMatrix(ledStr->strImg->data,ledStr->strImg->imgWidth,ledStr->strImg->imgHeight,ledData,DEFAULT_LED_WIDTH,DEFAULT_LED_HEIGHT,x,popup->info.y,LED_COLOR_NONE);
					}
				}				
			}
			break;
		default:
			break;
	}

	fChangeData = true;
	return 0;
}

//========================================
void clearPopupArea(POPUP_LED *popup)
{
	u16 i =0,j = 0;

	for(i = 0;i<popup->info.height;i++)
	{
		for(j = 0;j<popup->info.width;j++)
		{
			setMatrixPixel(ledData,DEFAULT_LED_WIDTH,popup->info.x+j,popup->info.y+i,0x00);	
		}
	}
	//if(hwConfig.ledType != SCAN_STATIC)
	fChangeData = true;
}

//========================================
u8 deletePopup(u16 popupID)
{
	u16 i = 0,j = 0,k = 0;
	POPUP_LED* popup_temp = NULL;
	
	for(i = 0;i<popup_index;i++)
	{
		if(popup[i].info.ID == popupID)
		{
			popup[i].info.isUpdated = true;		
			clearPopupArea(&popup[i]);	
			// Check to re-draw popups
			if(popup[i].info.z == 0)
			{
				for(j =0;j<popup_index;j++)
				{
					if((popup[j].info.z == 0)&&(popup[j].info.ID != popupID))
					{					
						if(CheckRetangles(popup[i],popup[j]))
							updatePage(&popup[j]);
					}					
				}
				for(j = 0;j<popup_index;j++)
				{
					if(popup[j].info.z != 0)
					{
						//if(CheckRetangles(popup[i],popup[j]))
						{
							updatePage(&popup[j]);
						}
					}
				}
			}
			else if(popup[i].info.z == 1)
			{
				for(j =0;j<popup_index;j++)
				{
					if(popup[j].info.z == 0)
					{					
						if(CheckRetangles(popup[i],popup[j]))
							updatePage(&popup[j]);
					}					
				}
				for(j = 0;j<popup_index;j++)
				{
					if((popup[j].info.z != 0)&&(popup[j].info.ID != popupID))
					{
						//if(CheckRetangles(popup[i],popup[j]))
						{
							updatePage(&popup[j]);
						}
					}
				}
			}			
			// Copy popups data to temp buffer
			popup_temp = (POPUP_LED*)m_calloc(popup_count,sizeof(POPUP_LED));			
			for(j = 0;j<popup_index;j++)
			{
				memcpy(&popup_temp[j],&popup[j],sizeof(POPUP_LED));
			}
			// Release popup memory
			m_free(popup);
			// Realocate new memory size for popups and copy data from temp buffer
			popup_count --;			
			popup = (POPUP_LED*)m_calloc(popup_count,sizeof(POPUP_LED));			
			k = 0;
			for(j = 0;j<popup_index;j++)
			{
				if(popup_temp[j].info.ID != popupID)
				{
					memcpy(&popup[k],&popup_temp[j],sizeof(POPUP_LED));	
					k++;
				}
				else
				{
					switch(popup_temp[j].info.dataType)
					{
						case POPUP_IMAGE:
							{
								IMAGE* img = popup_temp[j].info.data;
								m_free(img->data);
								m_free(popup_temp[j].info.data);
							}
							
							break;
						case POPUP_CLOCK:
							watch.active = false;
							break;
						case POPUP_TEXT:
							{
								LED_STRING* ledStr = (LED_STRING*)popup_temp[j].info.data;
								m_free(ledStr->strImg->data);
								m_free(ledStr->strImg);
								m_free(ledStr->str);								
								m_free(popup_temp[j].info.data);
							}
							break;
						default:
							break;
					}
				}
			}
			popup_index --;
			// Release temp buffer
			m_free(popup_temp);			
			break;
		}
	}
	return 0;
}

//========================================
void clearPopup(u16 popupID)
{
	u16 i = 0,j = 0;
	//POPUP_LED* popup_temp = NULL;
	
	for(i = 0;i<popup_index;i++)
	{
		if(popup[i].info.ID == popupID)
		{
			popup[i].info.isUpdated = true;		
			clearPopupArea(&popup[i]);	
			// Check to re-draw popups
			if(popup[i].info.z == 0)
			{
				for(j =0;j<popup_index;j++)
				{
					if((popup[j].info.z == 0)&&(popup[j].info.ID != popupID))
					{					
						if(CheckRetangles(popup[i],popup[j]))
							updatePage(&popup[j]);
					}					
				}
				for(j = 0;j<popup_index;j++)
				{
					if(popup[j].info.z != 0)
					{						
						updatePage(&popup[j]);
					}
				}
			}
			else if(popup[i].info.z == 1)
			{
				for(j =0;j<popup_index;j++)
				{
					if(popup[j].info.z == 0)
					{					
						if(CheckRetangles(popup[i],popup[j]))
							updatePage(&popup[j]);
					}					
				}
				for(j = 0;j<popup_index;j++)
				{
					if((popup[j].info.z != 0)&&(popup[j].info.ID != popupID))
					{
						updatePage(&popup[j]);
					}
				}
			}						
			break;
		}
	}
}

//========================================
void deinitPopupHeader(POPUP_LED* popup)
{
	memset(popup->byte.b,0x00,sizeof(popup->byte));
}
//========================================
u8 updatePopup(u16 popupID,u8* data)
{
	u16 i = 0,j = 0;
	u16 index = 0;
	u16 count = 0;
	u16 length = 0;
	POPUP_LED popup_temp;
	
	for(i = 0;i<popup_index;i++)
	{
		if(popup[i].info.ID == popupID)
		{
			// Free current popup data
			switch(popup[i].info.dataType)
			{
				case POPUP_IMAGE:
					{
						IMAGE* img = popup[i].info.data;
						m_free(img->data);	
						m_free(popup[i].info.data);
					}
					
					break;
				case POPUP_TEXT:
					{
						LED_STRING* ledStr = (LED_STRING*)popup[i].info.data;
						m_free(ledStr->strImg->data);
						m_free(ledStr->strImg);
						m_free(ledStr->str);						
						m_free(popup[i].info.data);
					}
					break;
				default:
					break;
			}
			// Update new data to popup
			deinitPopupHeader(&popup_temp);
			index = 0;
			count = sizeof(popup[i].byte);
			memcpy(popup_temp.byte.b,&data[index],count);
			// Check is popup area changed
			if((popup[i].info.x!=popup_temp.info.x)||(popup[i].info.y!=popup_temp.info.y)
				|| (popup[i].info.width!=popup_temp.info.width)||(popup[i].info.height!=popup_temp.info.height))
			{
				// clear current popup area
				clearPopupArea(&popup[i]);
			}
			memcpy(popup[i].byte.b,&data[index],count);
			popup[i].info.isUpdated = false;
			index += count;
			switch(popup[i].info.dataType)
			{
				case POPUP_IMAGE:
					{
						IMAGE* img = NULL;
						popup[i].info.data = (IMAGE*)m_malloc(sizeof(IMAGE));
						if(popup[i].info.data == NULL)
							return 1;
						img = (IMAGE*)popup[i].info.data;
						count = sizeof(IMAGE) - sizeof(u8*) - 2;
						memcpy(img,&data[index],count);
						index += count;
						img->imgLength = (img->imgWidth*img->imgHeight + 3)/4;
						img->data = (u8*)m_malloc(sizeof(u8)*img->imgLength);
						if(img->data == NULL)
							return 1;
						memcpy(img->data,&data[index],img->imgLength);
						popup[i].info.rev = 0;
					}					
					break;
				case POPUP_CLOCK:
					memcpy(&length,&data[index],2);
					index+=2;
					memcpy(fontClock,&data[index],length);
					index += length;
					memcpy(rtS.byte.b,&data[index],sizeof(tREALTIME));
					break;			
				case POPUP_TEMP:
					memcpy(&length,&data[index],2);
					index+=2;
					memcpy(fontTemp,&data[index],length);
					index += length;
					t = data[index];
					break;			
				case POPUP_TEXT:
					{
						LED_STRING* ledStr = NULL;
						popup[i].info.data = (LED_STRING*)m_malloc(sizeof(LED_STRING));
						if(popup[i].info.data == NULL)
							return 1;
						ledStr = (LED_STRING*)popup[i].info.data;
						ledStr->strLen = data[index+1];
						ledStr->strLen = (ledStr->strLen<<8) | data[index];
						index +=2;
						ledStr->str = (u8*)m_malloc(sizeof(u8)*ledStr->strLen);
						if(ledStr->str == NULL)
							return 1;
						memcpy(ledStr->str,&data[index],ledStr->strLen);
						ledStr->strImg = (IMAGE*)m_malloc(sizeof(IMAGE));
						if(ledStr->strImg == NULL)
							return 1;
						generateImage(ledStr->str,ledStr->strLen,popup[i].info.width,popup[i].info.height,ledStr->strImg);
					}
					break;
				default:
					break;
			}
			// Reload popup to screen
			if(popup[i].info.z == 0)
			{
				updatePage(&popup[i]);
				for(j =0;j<popup_index;j++)
				{
					if(popup[j].info.z != 0)
					{
						if(CheckRetangles(popup[i],popup[j]))					
							updatePage(&popup[j]);
					}
				}
			}
			else
				updatePage(&popup[i]);
			popup[i].info.isUpdated = true;
			break;
		}
	}
	return 0;
}

//========================================
u8 createPopup(u8* data)
{
	u16 index = 0;
	u16 count = 0;
	u16 length = 0;
	u8 i = 0;

	index = 0;
	memcpy(popup[popup_index].byte.b,&data[index],sizeof(popup[popup_index].byte));
	index += sizeof(popup[popup_index].byte);

	popup[popup_index].info.rev = 0;
	popup[popup_index].info.isUpdated = false;
	// Clear popup area if it is not background
	if(popup[popup_index].info.z > 0)
		clearPopupArea(&popup[popup_index]);
	
	switch(popup[popup_index].info.dataType)
	{
		case POPUP_IMAGE:
			{
				IMAGE* img = NULL;
				popup[popup_index].info.data = (IMAGE*)m_malloc(sizeof(IMAGE));
				if(popup[popup_index].info.data == NULL)
					return 1;
				img = (IMAGE*)popup[popup_index].info.data;
				count = sizeof(IMAGE) - sizeof(u8*) - 2;
				memcpy(img,&data[index],count);
				index += count;
				img->imgLength = (img->imgWidth*img->imgHeight + 3)/4;
				img->data = (u8*)m_malloc(sizeof(u8)*img->imgLength);
				if(img->data == NULL)
					return 1;
				memcpy(img->data,&data[index],img->imgLength);
			}	
			break;
		case POPUP_CLOCK:
			memcpy(&length,&data[index],2);
			index+=2;
			memcpy(fontClock,&data[index],length);
			index += length;
			memcpy(rtS.byte.b,&data[index],sizeof(tREALTIME));
			break;			
		case POPUP_TEMP:
			memcpy(&length,&data[index],2);
			index+=2;
			memcpy(fontTemp,&data[index],length);
			index += length;
			t = data[index];
			break;			
		case POPUP_TEXT:
			{
				LED_STRING* ledStr = NULL;
				popup[popup_index].info.data = (LED_STRING*)m_malloc(sizeof(LED_STRING));
				if(popup[popup_index].info.data == NULL)
					return 1;
				ledStr = (LED_STRING*)popup[popup_index].info.data;
				ledStr->strLen = data[index+1];
				ledStr->strLen = (ledStr->strLen<<8) | data[index];
				index +=2;
				ledStr->str = (u8*)m_malloc(sizeof(u8)*ledStr->strLen);
				if(ledStr->str == NULL)
					return 1;
				memcpy(ledStr->str,&data[index],ledStr->strLen);
				ledStr->strImg = (IMAGE*)m_malloc(sizeof(IMAGE));
				if(ledStr->strImg == NULL)
					return 1;
				generateImage(ledStr->str,ledStr->strLen,popup[popup_index].info.width,popup[popup_index].info.height,ledStr->strImg);
			}
			break;
		default:
			break;
	}
	// Reload popup to screen
	if(popup[popup_index].info.z == 0)
	{
		updatePage(&popup[popup_index]);
		for(i =0;i<popup_index;i++)
		{
			if(popup[i].info.z != 0)
			{
				if(CheckRetangles(popup[i],popup[i]))					
					updatePage(&popup[i]);
			}
		}
	}
	else
		updatePage(&popup[popup_index]);
	popup[popup_index].info.isUpdated = true;

	return 0;
}

//========================================
u8 displayClock(POPUP_LED popup,LEDColor color,u8 dot)
{	
	LED_STRING ledStr;
	
	switch(fontClock[0])
	{
		case 0:
			fClk = FONT_ID1;
			break;
		case 1:
			fClk = FONT_ID2;
			break;
		case 2:
			fClk = FONT_ID3;
			break;
		case 3:
			fClk = FONT_ID4;
			break;
		case 4:
			fClk = FONT_ID5;
			break;
		case 5:
			fClk = FONT_ID6;
			break;
		default:
			break;
	}
	
	ledStr.strLen = 5;
	ledStr.str = (u8*)m_malloc(sizeof(u8)*ledStr.strLen);
	if(ledStr.str == NULL)
		return 1;
	ledStr.str[0] = fontClock[(rtS.data.Hour>>4)+1];
	ledStr.str[1] = fontClock[(rtS.data.Hour&0x0F)+1];
	ledStr.str[2] = fontClock[11];
	ledStr.str[3] = fontClock[(rtS.data.Minute>>4)+1];
	ledStr.str[4] = fontClock[(rtS.data.Minute&0x0F)+1];
	
	ledStr.strImg = (IMAGE*)m_malloc(sizeof(IMAGE));
	if(ledStr.strImg == NULL)
		return 1;
	generateImage(ledStr.str,ledStr.strLen,popup.info.width,popup.info.height,ledStr.strImg);
	mixMatrix(ledStr.strImg->data,ledStr.strImg->imgWidth,ledStr.strImg->imgHeight,ledData,DEFAULT_LED_WIDTH,DEFAULT_LED_HEIGHT,popup.info.x,popup.info.y,color);
	m_free(ledStr.strImg->data);
	m_free(ledStr.strImg);
	m_free(ledStr.str);

	return 0;
}

//========================================
u8 displayTemp(POPUP_LED popup,LEDColor color)
{
	LED_STRING ledStr;
	
	switch(fontTemp[0])
	{
		case 0:
			fTemp= FONT_ID1;
			break;
		case 1:
			fTemp = FONT_ID2;
			break;
		case 2:
			fTemp = FONT_ID3;
			break;
		case 3:
			fTemp = FONT_ID4;
			break;
		case 4:
			fTemp = FONT_ID5;
			break;
		case 5:
			fTemp = FONT_ID6;
			break;
		default:
			break;
	}
	
	ledStr.strLen = 4;
	ledStr.str = (u8*)m_malloc(sizeof(u8)*ledStr.strLen);
	if(ledStr.str == NULL)
		return 1;
	ledStr.str[0] = fontTemp[(t/10)+1];
	ledStr.str[1] = fontTemp[(t%10)+1];
	ledStr.str[2] = fontTemp[11];
	ledStr.str[3] = fontTemp[12];
	
	ledStr.strImg = (IMAGE*)m_malloc(sizeof(IMAGE));
	if(ledStr.strImg == NULL)
		return 1;
	generateImage(ledStr.str,ledStr.strLen,popup.info.width,popup.info.height,ledStr.strImg);
	mixMatrix(ledStr.strImg->data,ledStr.strImg->imgWidth,ledStr.strImg->imgHeight,ledData,DEFAULT_LED_WIDTH,DEFAULT_LED_HEIGHT,popup.info.x,popup.info.y,color);
	m_free(ledStr.strImg->data);
	m_free(ledStr.strImg);
	m_free(ledStr.str);

	return 0;
}

//========================================
u8 getMatrixPixel(u8* buff,u16 w_buff,u16 x,u16 y)
{
	u16 index = 0;
	u8 mod = 0;
	u8 value = 0;
	u16 temp = 0;

	temp = (x + y*w_buff);
	index = temp/4;

	mod = (temp%4)*2;
	value = (buff[index]>>mod)&0x03;

	return value;
}

//========================================
void setMatrixPixel(u8 * buff,u16 w_buff,u16 x,u16 y,u8 value)
{
	u16 index = 0;
	
	index = (x + y*w_buff)/4;

	value = (value & 0x03)<<((x%4)*2);

	buff[index] &= ~(0x03<<((x%4)*2));
	buff[index] |= value;	
}

//========================================
void mixMatrix(u8 * source,u16 w_src,u16 h_src,u8 * des,u16 w_des,u16 h_des,u16 x_pos,u16 y_pos,LEDColor color)
{
	u16 x,y;
	u8 temp = 0;
	u16 index = 0;
	u8 count = 0;
	u16 des_index = 0;
	u8 mod = 0;

	temp = source[index];
	for(y = 0;y<h_src;y++)
	{
		des_index= (x_pos + y_pos*w_des)/4;
		mod = ((x_pos + y_pos*w_des)%4)*2;
		for(x = 0;x<w_src;x++)
		{
			des[des_index] &= ~(0x03<<mod);
			if(temp&0x03)
			{
				if(color != LED_COLOR_NONE)
					des[des_index] |= (color)<<mod;
				else
					des[des_index] |= (temp&0x03)<<mod;
			}
			temp = temp>>2;
			count++;
			mod +=2;
			if(count == 4)
			{
				index++;
				temp = source[index];
				count = 0;
			}
			if(mod == 8)
			{
				mod = 0;
				des_index++;
			}
		}
		y_pos ++;
	}
}

//========================================
u16 getMatrix(u8* source,u16 w_src,u8* des,u16 w_des,u16 h_des,u16 x,u16 y)
{
	u16 i,j;
	u8 temp = 0;
	u8 p_value = 0;
	u8 count = 0;
	u16 length = 0;

	temp = 0x00;
	for(i = 0;i<h_des;i++)
	{
		for(j = 0;j<w_des;j++)
		{
			p_value = getMatrixPixel(source,w_src,x+j,y+i);
			temp = temp | (p_value<<count);
			count +=2;
			if(count == 8)
			{
				count = 0;
				*des = temp;
				des++;
				temp = 0x00;
				length++;
			}
		}
	}
	return length;
}

//================================
void savePredefinedImg(u8 ID,u8 * data,u16 length)
{
	u32 i = 0;
	u32 data_temp = 0;
	u32 addr = 0;
	u32 addr_temp = 0;
	FLASH_Status status = FLASH_COMPLETE;

	switch(ID)
	{
		case 0:
			addr = IMG1_ADDR;
			break;
		case 1:
			addr = IMG2_ADDR;
			break;
		case 2:
			addr = IMG3_ADDR;
			break;
		case 3:
			addr = IMG4_ADDR;
			break;
		case 4:
			addr = IMG5_ADDR;
			break;
		default:
			addr = 0;
			break;
	}
	if(addr != 0)
	{
		// Unlock flash to rewrite to flash
		FLASH_UnlockBank1();
		addr_temp = addr;
		for(i = 0;i<4;i++)
		{
			// Erase pages used to store font
			status = FLASH_ErasePage(addr_temp);
			addr_temp += FLASH_PAGE_SIZE;
		}
		status = FLASH_ProgramWord(addr,length);
		addr += sizeof(u32);
		// Write data to flash
		for(i = 0;i<length;i+=4)
		{
			memcpy(&data_temp,&data[i],((length-i)>4?4:(length-i)));
			status = FLASH_ProgramWord(addr,data_temp);
			addr += sizeof(u32);
		}
		
		FLASH_LockBank1();
	}
}

//================================
void loadPredefinedImg(u8 ID)
{
	u16 length = 0;
	u32* data_temp;
	u32 addr = 0;
	u16 i = 0;

	switch(ID)
	{
		case 0:
			addr = IMG1_ADDR;
			break;
		case 1:
			addr = IMG2_ADDR;
			break;
		case 2:
			addr = IMG3_ADDR;
			break;
		case 3:
			addr = IMG4_ADDR;
			break;
		case 4:
			addr = IMG5_ADDR;
			break;
		default:
			addr = 0;
			break;
	}
	if(addr != 0)
	{
		data_temp = (u32*)(addr);
		addr += sizeof(u32);
		length = (u16)(*data_temp);
		if(length == LED_DATA_LENGTH)
		{
			for(i = 0;i<length;i+=4)
			{
				data_temp = (u32*)(addr);
				addr += sizeof(u32);
				memcpy(&ledData[i],data_temp,((length-i)>4?4:(length-i)));
			}
			fChangeData = true;
		}
	}
}

//========================================
void scanLED(void)
{
	u16 i = 0;

	for(i = 0;i<LED_DATA_LENGTH;i+=4)
	{
		ledData[i] = 0x55;
		ledData[i+1] = 0x55;
		ledData[i+2] = 0xAA;
		ledData[i+3] = 0xAA;
	}
	convertData(ledData);
	display(ledData);
	delayms(1000);
	for(i = 0;i<LED_DATA_LENGTH;i+=4)
	{
		ledData[i] = 0xAA;
		ledData[i+1] = 0xAA;
		ledData[i+2] = 0x55;
		ledData[i+3] = 0x55;
	}
	convertData(ledData);
	display(ledData);
	delayms(1000);
}

//========================================
void checkLed(void)
{
	
}

//========================================
void saveHWConfig(HW_CONFIG config)
{
	u32 data_temp = 0;
	FLASH_Status status = FLASH_COMPLETE;

	// Unlock flash to rewrite to flash
	FLASH_UnlockBank1();
	status = FLASH_ErasePage(HWCONFIG_ADDR);
	data_temp = config.ledWidth<<16 | config.ledHeight;
	status = FLASH_ProgramWord(HWCONFIG_ADDR,data_temp);	
	data_temp = (u32)(config.ledType);
	status = FLASH_ProgramWord(HWCONFIG_ADDR+4,data_temp);	
	FLASH_LockBank1();
}

//========================================
void loadHWConfig(HW_CONFIG* config)
{
#ifdef USING_LOAD_CONFIG
	u32* data_temp;

	data_temp = (u32*)(HWCONFIG_ADDR);
	config->ledWidth = (u16)((*data_temp)>>16);
	config->ledHeight = (u16)(*data_temp);
	data_temp = (u32*)(HWCONFIG_ADDR+4);
	config->ledType = (u8)((*data_temp)&0xFF);
	if((config->ledType < MIN_SCAN_TYPE)||(config->ledType > MAX_SCAN_TYPE))
		config->ledType = SCAN_STATIC;
	config->ledWidth = DEFAULT_LED_WIDTH;
	config->ledHeight = DEFAULT_LED_HEIGHT;
#else
	config->ledWidth = DEFAULT_LED_WIDTH;
	config->ledHeight = DEFAULT_LED_HEIGHT;
#endif
}

//========================================
bool CheckRetangles(POPUP_LED a, POPUP_LED b) 
{ 
	if(a.info.x>b.info.x) 
		return CheckRetangles(b,a); 
	//a và b ngoài nhau 
	if((a.info.x+a.info.width)<b.info.x)
		return false; 
	else if(((a.info.y<=b.info.y)&&(a.info.y+a.info.height)<=b.info.y)
			 || ((b.info.y<=a.info.y)&&(b.info.y+b.info.height)<=a.info.y)) 
		return false;
	
	return true; 
}

//========================================
void delayms(u32 nTime)
{
	TimingDelay = nTime;

	while(TimingDelay != 0);
}

void delays( u32 duration)
{
    while ( ( duration -- )> 0);
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
