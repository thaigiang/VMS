#ifndef __LED_DATA_STRUCT_H__
#define __LED_DATA_STRUCT_H__
#include "common.h"

#pragma pack(1)
typedef struct
{
	u8 playOrder;	
	u16 TTL;
	u16 TTS;
	PLAYLIST_DATA dataType;
	u16 length;	
} PLAYLIST_HEADER;
typedef struct
{	
	PLAYLIST_HEADER header;
	u8* data;
	u16 TTL_count;	
	bool isLive;
} PLAYLIST_ITEM;
typedef struct
{
	u8 year;
	u8 month;
	u8 day;
	u8 hour;
	u8 minute;
	u8 sec;
} DATETIME;
typedef struct
{
	u16 ID;
	u16 x;
	u16 y;
	u8 z;
	u16 width;
	u16 height;
	u8 dataType;
	u8 itemCount;
	u8 activeItem;
	u16 countDown;
	DATETIME startDate;
	DATETIME endDate;
	bool isUpdate;
} POPUP_HEADER;
typedef union
{
	struct
	{
		POPUP_HEADER header;
		void* data;
	} Info;
	struct
	{
		u8 b[12];
	} byte;
} POPUP;
typedef struct
{
	u16 ID;
	u8 itemCount;
	u8 eventType;
} PAGE_HEADER;
typedef struct
{
	PAGE_HEADER header;
	POPUP *popup;
} PAGE;
typedef struct
{
	BYTE Dummy;
	WORD ledWidth;
	WORD ledHeight;
	BYTE ledType;
	BYTE imei[15];
} HW_CONFIG;
typedef struct
{
	BYTE maxTemp;	
} WN_CONFIG;

typedef struct
{
	u8 temp;
	u8 SD_Status;
	u32 SD_Free_Mem;
} SYSTEM_STATUS;

#endif


