#ifndef __TYPEDEF__H__
#define __TYPEDEF__H__

#define ETHERNET_ACK		1
#define MAIN_ACK			2

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

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned int u16;
typedef signed int s16;
typedef unsigned long u32;
typedef signed long s32;

typedef enum {true = 1, false = 0} bool;

// integer number define for FASFS
/* These types must be 16-bit, 32-bit or larger integer */
typedef int				INT;
typedef unsigned int	UINT;

/* These types must be 8-bit integer */
typedef char			CHAR;
typedef unsigned char	UCHAR;
typedef unsigned char	BYTE;

/* These types must be 16-bit integer */
typedef short			SHORT;
typedef unsigned short	USHORT;
typedef unsigned short	WORD;
typedef unsigned short	WCHAR;

/* These types must be 32-bit integer */
typedef long			LONG;
typedef unsigned long	ULONG;
typedef unsigned long	DWORD;


#endif
