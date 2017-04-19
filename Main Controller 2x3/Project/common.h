#ifndef __COMMON_H__
#define __COMMON_H__

#define ETHERNET_TAG			2
#define CMD_REQUEST_RECONNECT	0x1A
#define SEND_REQUEST_UPDATE		0xEF
#define CMD_REQUEST_REBOOT		0xAB

#define SCREEN_BUFF_LENGTH		3461			// Buffer data 144*96/4 bytes + image header length 5 bytes

typedef enum
{
	NETWORK_ETHERNET = 0,	
	NETWORK_GPRS,		
	NETWORK_LOST
} NETWORK_STATUS;

typedef enum 
{		
	ETHERNET_GET_TIME = 3,
	ETHERNET_SET_TIME,
	ETHERNET_GET_CONFIG,
	ETHERNET_SET_CONFIG,
	ETHERNET_REPORT,
	ETHERNET_SCAN_LED,
	ETHERNET_GET_VERSION,
	ETHERNET_GET_IMEI = 0x0B
} ETHERNET_CMD;


typedef enum
{
	SIM_READY = 0,
	SIM_NOT_READY,
	SIM_OFF,
	SIM_ERROR
} SIM_STATUS;

typedef enum
{
	NETWORK_MODULE = 0,
	MAIN_MODULE,
	SERVER
} SOURCE_LOG;
typedef enum
{
	EVENT_LOG = 0,
	INFO_LOG,
	RESPONSE_LOG,
	ERROR_LOG,
	DATA_LOG
} LOG_TYPE;
typedef enum
{
	LOG_DATA = 0,
	LOG_INDEX
} LOGFILE_TYPE;

#define SEND_WARNING_HEADER			0xAB

#define BATTERY_VOLT_FULL				50		//(8V)
#define BATTERY_VOLT_EMPTY			40		//(7V)

//#define HN_VERSION						1
#define VMS_LUGIA							1
//#define VMS_LONG_MON						1

#define DEFAULT_MAX_TEMP 				50

//define loai bang
#ifdef  VMS_LUGIA
	#define DEFAULT_LED_HEIGHT		96
	#define DEFAULT_LED_WIDTH			144
#elif VMS_LONG_MON
	#define DEFAULT_LED_HEIGHT		48
	#define DEFAULT_LED_WIDTH			672
#else
	#define DEFAULT_LED_HEIGHT		96 	//96 //128
	#define DEFAULT_LED_WIDTH			192 //192 //256
#endif


#define DEFAULT_LED_TYPE				1

#define CONFIG_INFO_PAGE				0

#define TCP_PACKAGE_LENGTH			2048

#define SAVE_LOG								1
//#define DATA_LENGTH 					8192

#define ACK											0x7F
#define NACK										0x19
#define CMD_DPL_WRITE						0x31
#define CMD_DPL_REBOOT					0x41
#define CMD_DPL_RUNAPP					0x51
#define CMD_DPL_UPDATED					0x61

typedef enum
{
 	POPUP_PLAYLIST = 1,
	POPUP_CLOCK,
 	POPUP_TEMP,
 	POPUP_TABLE,
 	POPUP_TEXT,
} POPUP_DATA;

typedef enum
{
	UNKNOW = 0,
	PLAYLIST_TEXT,
	PLAYLIST_IMAGE,	
	PLAYLIST_CLOCK,
	PLAYLIST_TEMP,
} PLAYLIST_DATA;

#define ETHERNET_ACK 	1
#define MAIN_ACK		2

typedef enum 
{
   		SERVER_TAG = 0x00,
        DEVICE_TAG,
        PING = 0x03,

		// Font data update commands
        CMD_SERVER_FONT_UPDATE_REQ = 0x20,
        FONT_INFO_HEADER,
        FONT_PACKAGE_HEADER,

        // Bitmap data update commands
        CMD_SERVER_DATA_UPDATE_REQ = 0x30,
        DATA_INFO_HEADER,
        DATA_PACKAGE_HEADER,
        
        // FIRMWARE update commands
        CMD_SERVER_FIRMWARE_UPDATE_REQ = 0x40,
        FIRMWARE_INFO_HEADER,
        FIRMWARE_PACKAGE_HEADER,
				CMD_UPDATE_ETHERNET_REQ,

        // Image binary commands
				CMD_INSERT_IMAGE = 0x50,
        CMD_UPDATE_IMAGE,
        CMD_DELETE_IMAGE,
        CMD_DELETE_ALL_IMAGE,
        CMD_GET_LIST_IMAGE_INDEX,        
        
        // Page control commands
        CMD_CREATE_PAGE = 0x60,
        CMD_UPDATE_PAGE,
        CMD_DELETE_PAGE,
        CMD_GET_PAGE_INFO,
        CMD_SET_ACTIVE_PAGE,
        CMD_GET_ACTIVE_PAGE,
        CMD_GET_LIST_PAGE,
        
        // Popup control commands
        CMD_INSERT_POPUP = 0x70,
        CMD_UPDATE_POPUP,
        CMD_DELETE_POPUP,
        
        // Control commands server sent to device
        GET_VERSION = 0xA0,
        GET_PASSWORD,
        SET_PASSWORD,
        GET_BRIGHTNESS,
        SET_BRIGHTNESS,
        CMD_CHECK_LED,
        CMD_CLEAR_LED,
        CMD_START_SCAN_LED,
        CMD_STOP_SCAN_LED,
        CMD_CAPTURE_SCREEN,
        CMD_GET_STATUS,
        CMD_REBOOT,
        CMD_SET_PREDEFINED,       
        CMD_SHUTDOWN,
				CMD_SET_TIME,
				CMD_GET_TIME,

        // Device commands sent back to server
        CMD_SEND_IMEI = 0x5D,
        // Screen data sent from device
        LED_INFO_HEADER = 0xD1,
        LED_SCREEN_DATA,
        LED_STATUS_DATA,        

		CMD_INVALID = 0xF0
} COMMAND_TAG;		

typedef enum
{
	// Warning data
    WARNING_TOO_HOT = 0x00,
    WARNING_DAMAGED,
    WARNING_OUT_OF_POWER,
    WARNING_OPENNED,
    WARNING_SIM_ERR
} WARNING_CODE;

typedef enum 
{ 
    _SUCCESS = 0,
    _ERROR,
    ERROR_ID_DUPLICATE,
    ERROR_ID_NOT_FOUND,
    ERROR_STORAGE_NOT_AVAILABLE,
    ERROR_DATA_IMAGE_FORMAT,
    ERROR_PACKAGE_ID,
    ERROR_PACKAGE_CHECKSUM,
    ERROR_DEVICE_BUSY,
    ERROR_IMAGE_NOT_FOUND,
    ERROR_IMAGE_IN_USE,
    ERROR_PACKAGE_LENGTH,
} RESPONSE;		

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
	DPL_CMD_SEND_PREDEFINE,
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
} DISPLAY_RESPONSE;

typedef enum
{
	SYSTEM_OK = 0,
	SYSTEM_ERR_NETWORK = 1,
	SYSTEM_ERR_SDC = 2,
	SYSTEM_ERR_FLASH = 4,
	SYSTEM_ERR_SIM = 8,
} STATUS;

typedef enum
{
	EVENT_NONE = 0,
	EVENT_NETWORK_LOST,
	EVENT_OVER_TEMP
} EVENT;

typedef enum
{
	F_SUCCESS = 0,
	F_ERROR,
	F_INVALID_ID,
	F_DUPLICATE,
	F_SDC_ERR,
	F_SIZE_ERR,
	F_IMG_NOT_EXIST,
} F_RETURN;

typedef enum
{
	FILE_IMAGE = 0,
	FILE_PAGE,
	FILE_FONT
} FILE_TYPE;

typedef enum
{
	PAGE_NORMAL = 0,
	PAGE_DEFAULT,
	PAGE_PREDEFINED,
	PAGE_EMERGENCY,
} PAGE_TYPE;

typedef enum
{
	EFFECT_NONE	= 0,
	EFFECT_TOP_DOWN,
	EFFECT_BOTTOM_UP,
	EFFECT_LEFT_RIGHT,	
	EFFECT_RIGHT_LEFT,
} EFFECT;

#define RS485_ID_MIN		1
#define RS485_ID_MAX		1
#define RS485_BROADCAST_ID	(RS485_ID_MAX + 1)

struct u_id {
    uint16_t off0;
    uint16_t off2;
    uint32_t off4;
    uint32_t off8;
};
	
#define MMIO16(addr)  (*(volatile uint16_t *)(addr))
#define MMIO32(addr)  (*(volatile uint32_t *)(addr))
#define U_ID          0x1ffff7e8

void createLog(SOURCE_LOG srcLog,LOG_TYPE logType,char * frmt,...);
void saveLog2SDC(void);
void initSDC(void);
void sendWarning(WARNING_CODE wn);
void displayStatus(STATUS status);
void msgControl(u8 * msg,u16 length);
void sendResponse(u8 cmdID,u8 res);
void EthMsgControl(u8* msg);
void delayms(u32 nTime);
u16 getListFile(char *path,u16* codeArray);
u16 countFile(char *path);
u8 ComputeChecksum(u8* data, u16 length);
u8 covertHexToDec(u8);
u8 covertDecToHex(u8);
void uid_read(struct u_id *id);
#endif
