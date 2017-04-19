/*********************************************************************
 *
 *	Hardware specific definitions
 *
 *********************************************************************
 * FileName:        HardwareProfile.h
 * Dependencies:    None
 * Processor:       PIC18, PIC24F, PIC24H, dsPIC30F, dsPIC33F, PIC32
 * Compiler:        Microchip C32 v1.10 or higher
 *					Microchip C30 v3.12 or higher
 *					Microchip C18 v3.34 or higher
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
 * Author               Date		Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Howard Schlunder		10/03/06	Original, copied from Compiler.h
 * Ken Hesky            07/01/08    Added ZG2100-specific features
 ********************************************************************/
#ifndef __HARDWARE_PROFILE_H
#define __HARDWARE_PROFILE_H

#include "GenericTypeDefs.h"
#include "Compiler.h"


#define GetSystemClock()		(25000000ul)		//(41666667ul)      // Hz
#define GetInstructionClock()	(GetSystemClock()/4)
#define GetPeripheralClock()	GetInstructionClock()

// PICDEM.net 2 (PIC18F97J60 + ENC28J60 or ENC624J600)

// ENC28J60 I/O pins
#define ENC_RST_TRIS		(TRISDbits.TRISD2)	// Not connected by default
#define ENC_RST_IO			(LATDbits.LATD2)
//#define ENC_CS_TRIS			(TRISDbits.TRISD3)	// Uncomment this line if you wish to use the ENC28J60 on the PICDEM.net 2 board instead of the internal PIC18F97J60 Ethernet module
#define ENC_CS_IO			(LATDbits.LATD3)
#define ENC_SCK_TRIS		(TRISCbits.TRISC3)
#define ENC_SDI_TRIS		(TRISCbits.TRISC4)
#define ENC_SDO_TRIS		(TRISCbits.TRISC5)
#define ENC_SPI_IF			(PIR1bits.SSP1IF)
#define ENC_SSPBUF			(SSP1BUF)
#define ENC_SPISTAT			(SSP1STAT)
#define ENC_SPISTATbits		(SSP1STATbits)
#define ENC_SPICON1			(SSP1CON1)
#define ENC_SPICON1bits		(SSP1CON1bits)
#define ENC_SPICON2			(SSP1CON2)

// 25LC256 I/O pins
//#define EEPROM_CS_TRIS		(TRISJbits.TRISJ4)
#define EEPROM_CS_IO		(LATJbits.LATJ4)
#define EEPROM_SCK_TRIS		(TRISCbits.TRISC3)
#define EEPROM_SDI_TRIS		(TRISCbits.TRISC4)
#define EEPROM_SDO_TRIS		(TRISCbits.TRISC5)
#define EEPROM_SPI_IF		(PIR1bits.SSP1IF)
#define EEPROM_SSPBUF		(SSP1BUF)
#define EEPROM_SPICON1		(SSP1CON1)
#define EEPROM_SPICON1bits	(SSP1CON1bits)
#define EEPROM_SPICON2		(SSP1CON2)
#define EEPROM_SPISTAT		(SSP1STAT)
#define EEPROM_SPISTATbits	(SSP1STATbits)

/*
// LCD I/O pins
#define LCD_DATA_TRIS		(TRISE)
#define LCD_DATA_IO			(LATE)
#define LCD_RD_WR_TRIS		(TRISHbits.TRISH1)
#define LCD_RD_WR_IO		(LATHbits.LATH1)
#define LCD_RS_TRIS			(TRISHbits.TRISH2)
#define LCD_RS_IO			(LATHbits.LATH2)
#define LCD_E_TRIS			(TRISHbits.TRISH0)
#define LCD_E_IO			(LATHbits.LATH0)

#define BusyUART()				BusyUSART()
#define CloseUART()				CloseUSART()
#define ConfigIntUART(a)		ConfigIntUSART(a)
#define DataRdyUART()			DataRdyUSART()
#define OpenUART(a,b,c)			OpenUSART(a,b,c)
#define ReadUART()				ReadUSART()
#define WriteUART(a)			WriteUSART(a)
#define getsUART(a,b,c)			getsUSART(b,a)
#define putsUART(a)				putsUSART(a)
#define getcUART()				ReadUSART()
#define putcUART(a)				WriteUSART(a)
#define putrsUART(a)			putrsUSART((far rom char*)a)
*/

#define STM32_RST_TRIS			(TRISHbits.TRISH2)
#define STM32_RST_IO			(LATHbits.LATH2)
#define STM32_RST_IN			(PORTHbits.RH2)

#endif
