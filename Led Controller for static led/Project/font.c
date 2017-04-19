#include <string.h>
#include <stdlib.h>
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "common.h"
#include "ledMatrix.h"
#include "font.h"
#include "c_func.h"

#define HASHTAG		137

//extern u8 ledData[LED_DATA_LENGTH];
extern u8* ledData;
extern HW_CONFIG hwConfig;
//=====================================
void getChar(FONT_ID fontID,u8 charID,IMAGE* charImg)
{
	u16 index = 0;
	u16 offset = 0;
	u8* data;
	u32 addr = 0;
/*
	if(charImg->data != NULL)
		m_free(charImg->data);
*/
	//charImg = (IMAGE*)m_malloc(sizeof(IMAGE));

	switch(fontID)
	{
		case FONT_ID1:
			addr = FONT_ID1_ADDR;
			break;
		case FONT_ID2:
			addr = FONT_ID2_ADDR;
			break;
		case FONT_ID3:
			addr = FONT_ID3_ADDR;
			break;
		case FONT_ID4:
			addr = FONT_ID4_ADDR;
			break;
		case FONT_ID5:
			addr = FONT_ID5_ADDR;
			break;
		case FONT_ID6:
			addr = FONT_ID6_ADDR;
			break;
		default:
			addr = 0;
			break;
	}
	if(addr != 0)
	{
		data = (u8*)(addr + 1);
		charImg->imgHeight = *data;
		charImg->effectType = EFFECT_NONE;

		index = 3 + charID*3;
		data = (u8*)(addr+index+2);
		offset = *data;
		data = (u8*)(addr+index+1);
		offset = (offset << 8) | *data;

		data = (u8*)(addr+offset);
		charImg->imgWidth = *data;
		if((charImg->imgWidth == 0xFF) || (charImg->imgHeight == 0xFF))
		{
			charImg->imgWidth = 0;
			charImg->imgHeight = 0;	
			charImg->data = NULL;
		}
		else
		{
			charImg->imgLength = (charImg->imgWidth*charImg->imgHeight + 7)/8;
			charImg->data = (u8*)m_malloc(sizeof(u8)*charImg->imgLength);
			if(charImg->data != NULL)
			{
				memset(charImg->data,0x00,charImg->imgLength);
				data = (u8*)(addr+offset+1);
				if(charImg->data != NULL)
					memcpy(charImg->data,data,charImg->imgLength);
			}
			else 
				return;
		}
	}
}

//=====================================
void displayChar(u16 x_pos,u16 y_pos,IMAGE* charImg,LEDColor color)
{
	u16 x = 0,y = 0;
	u16 x_temp = 0;
	u8 temp = 0;
	u16 index = 0;
	u8 count = 0;
	u16 des_index = 0;
	u8 mod = 0;
	
	temp = charImg->data[index];
	for(y = 0;y<charImg->imgHeight;y++)
	{
		x_temp = x_pos;
		des_index= (x_temp + y_pos*hwConfig.ledWidth)/4;
		mod = (x_temp%4)*2;
		for(x = 0;x<charImg->imgWidth;x++)
		{
			ledData[des_index] &= ~(0x03<<mod);
			if(temp&0x01)
			{
				if(color != LED_COLOR_NONE)
					ledData[des_index] |= (color)<<mod;
				else
					ledData[des_index] |= (temp)<<mod;
			}
			temp = temp>>1;
			count++;
			mod +=2;
			if(count == 8)
			{
				index++;
				temp = charImg->data[index];
				count = 0;
			}
			if(mod == 8)
			{
				mod = 0;
				des_index++;
			}
			x_temp++;
		}
		y_pos ++;
	}
}

//=====================================
void printChar(u16 x_pos,u16 y_pos,IMAGE* strImg,IMAGE* charImg,LEDColor color)
{
	u16 x = 0,y = 0;
	u16 x_temp = 0;
	u8 temp = 0;
	u16 index = 0;
	u8 count = 0;
	u16 des_index = 0;
	u8 mod = 0;
	
	temp = charImg->data[index];
	x_temp = x_pos;
	for(y = 0;y<charImg->imgHeight;y++)
	{
		x_temp = x_pos;
		mod = ((x_temp+y_pos*strImg->imgWidth)%4)*2;
		des_index= (x_temp + y_pos*strImg->imgWidth)/4;
		for(x = 0;x<charImg->imgWidth;x++)
		{
			strImg->data[des_index] &= ~(0x03<<mod);
			if(temp&0x01)
			{
				if(color != LED_COLOR_NONE)
					strImg->data[des_index] |= (color)<<mod;
			}
			temp = temp>>1;
			count++;
			mod +=2;
			if(count == 8)
			{
				index++;
				temp = charImg->data[index];
				count = 0;
			}
			if(mod == 8)
			{
				mod = 0;
				des_index++;
			}
			x_temp++;
		}
		y_pos ++;
	}
}

//=====================================
void generateImage(u8* str,u8 length,u16 imgWidth,u16 imgHeight,IMAGE* strImg)
{
	u8 i = 0;
	u16 x = 0,y = 0;
	EFFECT effect = EFFECT_NONE;
	LEDColor ledColor = LED_COLOR_RED;
	FONT_ID fontID = FONT_ID2;
	TEXT_ALIGN align = ALIGN_LEFT;
	u8 index = 0;
	u16 width = 0,width_temp = 0;
	u16 w[10];
	u16 height = 0;
	IMAGE charImg;

	charImg.data = NULL;
	charImg.imgHeight = 0;
	charImg.imgWidth = 0;
	
	// Check font,color,effect
	strImg->imgWidth = 0;

	// Count length of image which used to display string
	i = 0;
	height = 0;
	index = 0;
	while(i<length)
	{
		if(str[i] == HASHTAG)
		{
			i++;
			if(str[i] == 'n')
			{
				if(width_temp < width)
				{
					width_temp = width;					
				}
				w[index] = width;
				index ++;
				width = 0;
				if(charImg.imgHeight)
					height += charImg.imgHeight;
				else
				{
					getChar(fontID,0,&charImg);
					if(charImg.data == NULL)
						return;
					height += charImg.imgHeight;
					m_free(charImg.data);
				}
			}
			else if(str[i] == 'f')
			{
				i++;
				switch(str[i])
				{
					case '0':
						fontID = FONT_ID1;
						break;
					case '1':
						fontID = FONT_ID2;
						break;
					case '2':
						fontID = FONT_ID3;
						break;
					case '3':
						fontID = FONT_ID4;
						break;
					case '4':
						fontID = FONT_ID5;
						break;
					case '5':
						fontID = FONT_ID6;
						break;
					default:
						break;
				}
			}
			else
				i++;
		}
		else
		{
			getChar(fontID,str[i],&charImg);
			width += charImg.imgWidth;
			m_free(charImg.data);
		}
		i++;
	}
	
	height += charImg.imgHeight;

	w[index] = width;
		
	if(width<width_temp)
		width = width_temp;

	// Generate image from string
	strImg->imgHeight = height;
	/*
	if(width%4 != 0)
		strImg->imgWidth = width + (4 - width%4);
	else
		strImg->imgWidth = width;
	if(imgWidth%4 != 0)
		strImg->imgWidth = imgWidth + (4 - imgWidth%4);
	else
		strImg->imgWidth = imgWidth;
	*/
	strImg->imgWidth = imgWidth;
	strImg->imgLength = (strImg->imgHeight*strImg->imgWidth + 3)/4;
	strImg->data = (u8*)m_malloc(sizeof(u8)*strImg->imgLength);
	if(strImg->data == NULL)
		return;
	memset(strImg->data,0x00,strImg->imgLength);
	x = 0;
	y = 0;
	i = 0;
	index = 0;
	charImg.data = NULL;
	charImg.imgHeight = 0;
	charImg.imgWidth = 0;
	while(i<length)
	{
		if(str[i] == HASHTAG)
		{
			i++;
			switch(str[i])
			{
				case 'a':
					i++;
					if(str[i] == 'r')
					{
						x = strImg->imgWidth - w[index];
						align = ALIGN_RIGHT;
					}
					else if(str[i] == 'c')	
					{
						align = ALIGN_CENTER;
						x = (strImg->imgWidth - w[index])/2;
					}
					else if(str[i] == 'l')
					{
						x = 0;
						align = ALIGN_LEFT;
					}
					break;
				case 'f':
					i++;
					switch(str[i])
					{
						case '0':
							fontID = FONT_ID1;
							break;
						case '1':
							fontID = FONT_ID2;
							break;
						case '2':
							fontID = FONT_ID3;
							break;
						case '3':
							fontID = FONT_ID4;
							break;
						case '4':
							fontID = FONT_ID5;
							break;
						case '5':
							fontID = FONT_ID6;
							break;
						default:
							break;
					}
					break;
				case 'c':
					i++;
					if(str[i] == 'r')
						ledColor = LED_COLOR_RED;
					else if(str[i] == 'g')
						ledColor = LED_COLOR_GREEN;
					else if(str[i] == 'y')
						ledColor = LED_COLOR_YELLOW;
					break;
				case 'e':
					i++;
					if(str[i] == '0')
						effect = EFFECT_NONE;
					if(str[i] == '1')
						effect = EFFECT_TOP_DOWN;
					if(str[i] == '2')
						effect = EFFECT_BOTTOM_UP;
					if(str[i] == '3')
						effect = EFFECT_LEFT_RIGHT;
					if(str[i] == '4')
						effect = EFFECT_RIGHT_LEFT;
					break;
				case 'n':
					if(charImg.imgHeight)
						y += charImg.imgHeight;
					else
					{
						getChar(fontID,0,&charImg);
						y += charImg.imgHeight;
						m_free(charImg.data);
					}					
					index ++;
					switch(align)
					{
						case ALIGN_LEFT:
							x = 0;
							break;
						case ALIGN_CENTER:
							if(w[index]<=strImg->imgWidth)
								x = (strImg->imgWidth - w[index])/2;
							else
								x = 0;
							break;
						case ALIGN_RIGHT:
							if(w[index]<=strImg->imgWidth)
								x = strImg->imgWidth - w[index];
							else
								x = 0;
							break;
						default:
							break;
					}
					break;
				default:
					i++;
					break;
			}
		}
		else
		{
			getChar(fontID,str[i],&charImg);
			if((x<=(strImg->imgWidth - charImg.imgWidth)) && (y<=(strImg->imgHeight - charImg.imgHeight)))
			{
				printChar(x,y,strImg,&charImg,ledColor);
				x += charImg.imgWidth;				
			}
			m_free(charImg.data);
		}
		i++;
	}
	strImg->effectType = effect;
	//m_free(charImg.data);
}

//=====================================
void saveFont(u32 addr,u8* fontData,u16 length)
{
	u32 i = 0;
	u32 addr_temp;
	u32 data_temp = 0;
	FLASH_Status status = FLASH_COMPLETE;

	addr_temp = addr;
	// Unlock flash to rewrite to flash
	FLASH_UnlockBank1();
	// Erase pages used to store font
	for(i = 0;i<3;i++)
	{
		status = FLASH_ErasePage(addr_temp);
		addr_temp += FLASH_PAGE_SIZE;
	}
	
	// Write font data to flash
	for(i = 0;i<length;i+=4)
	{
		memcpy(&data_temp,&fontData[i],((length-i)>4?4:(length-i)));
		status = FLASH_ProgramWord(addr,data_temp);
		addr += sizeof(u32);
	}
	FLASH_LockBank1();
}

