#ifndef __COMMON_H__
#define __COMMON_H__


#define MAX_PACKAGE_LENGTH 		(8256)
#define PREDEFINED_DELAY_TIME	10
#define MAX_PREDEFINED_IMG		5

typedef  void (*pFunction)(void);

typedef enum
{
	EFFECT_NONE	= 0,
	EFFECT_TOP_DOWN,
	EFFECT_BOTTOM_UP,
	EFFECT_LEFT_RIGHT,	
	EFFECT_RIGHT_LEFT,
} EFFECT;

#define MSG_HEADER_LENGTH	2
#define FONT_9_12_WIDTH		9
#define FONT_9_12_HEIGHT	12
#define FONT_7_10_WIDTH		7
#define FONT_7_10_HEIGHT	10
#define FONT_SYMBOL_WIDTH	3


typedef enum
{
	MAIN_TAG = 1,
	DPL_CMD_CLEAR_SCREEN,
	DPL_CMD_UPDATE,
	DPL_CMD_CHANGE_BRIGHTNESS,
	DPL_CMD_SEND_DATA,
	DPL_CMD_SEND_INFO,
	DPL_CMD_CREATE_POPUP,
	DPL_CMD_UPDATE_POPUP,
	DPL_CMD_DELETE_POPUP,
	DPL_CMD_CHECK_LED,
	DPL_CMD_CAPTURE_SCREEN,
	DPL_CMD_START_SCAN_LED,
	DPL_CMD_STOP_SCAN_LED,
	DPL_CMD_SEND_FONT,
	DPL_CMD_REQUEST_REBOOT,
	DPL_CMD_SEND_PREDEFINED,
	DPL_CMD_PLAY_PREDEINED,
	DPL_CMD_SEND_HWCONFIG,
	DPL_CMD_UPDATE_FW,
	DPL_CMD_FW_INFO,
	DPL_CMD_CLEAR_POPUP,
} DISPLAY_COMMAND;

typedef enum
{
	DPL_RES_OK = 0,
	DPL_RES_ERROR = 1,
	DPL_NOT_RESPONSE
} DISPLAY_RESPONSE;


// Define led Color
typedef enum
{
	LED_COLOR_NONE = 0x00,
	LED_COLOR_RED=0x01,
	LED_COLOR_GREEN,
	LED_COLOR_YELLOW
} LEDColor;

typedef enum
{
	UNKNOW = 0,
	PLAYLIST_TEXT,
	PLAYLIST_IMAGE,	
	PLAYLIST_CLOCK,
	PLAYLIST_TEMP,
} PLAYLIST_DATA;

typedef enum {false = 0, true} bool;

typedef union tagREALTIME 
{
	struct
	{
	  char Second;
	  char Minute;
	  char Hour;
	  char Day;
	  char Date;
	  char Month;
	  char Year;
	} data;
	struct
	{
		char b[7];
	} byte;
} tREALTIME;
typedef struct
{
	u16 x;
	u16 y;
	u16 width;
	u16 height;
	u8 row;
	u8 column;
} LEDTable;
typedef struct
{
	u16 ledWidth;
	u16 ledHeight;
	u8 ledType;
} HW_CONFIG;

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
} PLAYLIST_ITEM;

typedef struct
{
	u16 imgWidth;
	u16 imgHeight;
	u8 imgBpp;
	u8 effectType;
	u16 imgLength;
	u8* data;
} IMAGE;
typedef struct 
{
	u8 row;
	u8 column;
	u8 symbol;
	u16 dataLength;
	u8* data;
} TABLE;
typedef struct
{
	u16 strLen;
	u8* str;
	IMAGE* strImg;
} LED_STRING;
typedef union
{
	struct
	{
		u16 ID;
		u16 x;
		u16 y;
		u8 z;
		u16 width;
		u16 height;
		u8 dataType;
		bool isUpdated;
		void* data;
		u16 rev;
	} info;
	struct
	{
		u8 b[12];
	} byte;
} POPUP_LED;

typedef struct
{
	u16 x;
	u16 y;
	LEDColor color;
	bool dot;
	bool active;
} WATCH;

void delayms(u32 nTime);
void delays( u32 duration);
void CmdExecute(void);
void loadPredefinedImg(u8 ID);

#endif 
