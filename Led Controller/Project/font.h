#ifndef __FONT_H__
#define __FONT_H__

#define STORAGE_START_ADDR	0x08040000
#define FLASH_PAGE_SIZE		2048

#define FONT_ID1_ADDR		0x08040000
#define FONT_ID2_ADDR		0x08041800
#define FONT_ID3_ADDR		0x08043000
#define FONT_ID4_ADDR		0x08044800
#define FONT_ID5_ADDR		0x08046000
#define FONT_ID6_ADDR		0x08047800

typedef enum
{
	FONT_ID1 = 0,
	FONT_ID2,
	FONT_ID3,
	FONT_ID4,
	FONT_ID5,
	FONT_ID6
} FONT_ID;

typedef enum
{
	ALIGN_LEFT = 0,
	ALIGN_CENTER,
	ALIGN_RIGHT,
} TEXT_ALIGN;
//=====================================
void getChar(FONT_ID fontID,u8 charID,IMAGE* charImg);
void displayChar(u16 x,u16 y,IMAGE* charImg,LEDColor color);
void generateImage(u8* str,u8 length,u16 imgWidth,u16 imgHeight,IMAGE* strImg);
void printChar(u16 x_pos,u16 y_pos,IMAGE* strImg,IMAGE* charImg,LEDColor color);
void saveFont(u32 addr,u8* fontData,u16 length);
#endif
