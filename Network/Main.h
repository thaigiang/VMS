/*********************************************************************
 *
 *                  Headers for TCPIP Demo App
 *
 *********************************************************************
 * FileName:        MainDemo.h
 * Dependencies:    Compiler.h
 * Processor:       PIC18, PIC24F, PIC24H, dsPIC30F, dsPIC33F, PIC32
 * Compiler:        Microchip C32 v1.05 or higher
 *					Microchip C30 v3.12 or higher
 *					Microchip C18 v3.30 or higher
 *					HI-TECH PICC-18 PRO 9.63PL2 or higher
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * Copyright (C) 2002-2009 Microchip Technology Inc.  All rights
 * reserved.
 *
 * Microchip licenses to you the right to use, modify, copy, and
 * distribute:
 * (i)  the Software when embedded on a Microchip microcontroller or
 *      digital signal controller product ("Device") which is
 *      integrated into Licensee's product; or
 * (ii) ONLY the Software driver source files ENC28J60.c, ENC28J60.h,
 *		ENCX24J600.c and ENCX24J600.h ported to a non-Microchip device
 *		used in conjunction with a Microchip ethernet controller for
 *		the sole purpose of interfacing with the ethernet controller.
 *
 * You should refer to the license agreement accompanying this
 * Software for additional information regarding your rights and
 * obligations.
 *
 * THE SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION, ANY WARRANTY OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * MICROCHIP BE LIABLE FOR ANY INCIDENTAL, SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF
 * PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY OR SERVICES, ANY CLAIMS
 * BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE
 * THEREOF), ANY CLAIMS FOR INDEMNITY OR CONTRIBUTION, OR OTHER
 * SIMILAR COSTS, WHETHER ASSERTED ON THE BASIS OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE), BREACH OF WARRANTY, OR OTHERWISE.
 *
 *
 * Author               Date    Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * E. Wood				4/26/08 Copied from MainDemo.c
 ********************************************************************/
#ifndef _MAINDEMO_H
#define _MAINDEMO_H

#define FW_VERSION	103

#define CONFIG_INFO_LENGTH	(3+sizeof(AppConfig)+sizeof(wnConfig)+sizeof(hwConfig))
#define USER_PROFILE_LENGTH	0x84
#define CONFIG_INFO_PAGE	0

#define DEFAULT_SIM_TYPE	SIM_MOBIFONE

// Application-dependent structure used to contain hardware information
typedef struct __attribute__((__packed__)) 
{
	BYTE Dummy;
	WORD ledWidth;
	WORD ledHeight;
	BYTE ledType;
	BYTE imei[15];
} HW_CONFIG;

typedef struct __attribute__((__packed__)) 
{
	BYTE maxTemp;	
//	BYTE Dummy1;
//	BYTE Dummy2;
//	BYTE Dummy3;
} WN_CONFIG;

typedef struct tagREALTIME {
	char Second;
	char Minute;
	char Hour;
	char Day;
	char Date;
	char Month;
	char Year;
} tREALTIME;

#define MAX_DATA_PACKAGE		1024

#define DEFAULT_LED_HEIGHT		144
#define DEFAULT_LED_WIDTH		192
#define DEFAULT_LED_TYPE		1
#define DEFAULT_IMEI_SIZE		15

#define DEFAULT_MAX_TEMP		50

/*
// STM32F's Boot pins
#define STM32F_BOOT0_TRIS		(TRISHbits.TRISH0)
#define STM32F_BOOT0_IO			(LATHbits.LATH0)

#define STM32F_BOOT1_TRIS		(TRISHbits.TRISH1)
#define STM32F_BOOT1_IO			(LATHbits.LATH1)
*/

#if !defined(THIS_IS_STACK_APPLICATION)
	extern BYTE AN0String[8];
#endif

#define CPU_Reset()		{_asm RESET _endasm}

void InitAppConfig(void);
void DoUARTConfig(void);
void SaveAppConfig(void);
void BerkeleyTCPClient(void);
void InitHWConfig(void);
void SaveHWConfig(void);
void SaveUserProfile(BYTE* cUser, int userLeng, BYTE* cPass, int passLeng);
void InitUserProfile(void);
void GetDateTime(void);
void SetDateTime(void);
void GetIMEIFromMain(unsigned char *imei);
void GetHWConfig(void);
void SetHWConfig(void);
void GetVersion(void);




#endif // _MAINDEMO_H
