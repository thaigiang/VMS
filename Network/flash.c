#include <p18cxxx.h>
#include "typedef.h"
#include "flash.h"

BYTE flashReadByte(u32 addr)
{
	BYTE data = 0;
	
	TBLPTR = addr;
	
	_asm TBLRDPOSTINC _endasm
	data = TABLAT;		
	
	return data;
}

void flashReadBlock(u32 addr,u8* buff)
{
	u16 i = 0;
	
	TBLPTR = addr;

	for(i = 0;i<FLASH_WRITE_BLOCK;i+=2)
	{
		_asm TBLRDPOSTINC _endasm
		buff[i] = TABLAT;		
		_asm TBLRDPOSTINC _endasm
		buff[i+1] = TABLAT;
	}
}

void flashEraseBlock(u32 addr)
{
	TBLPTR = addr;
	EECON1bits.WREN = 1;
	EECON1bits.FREE = 1;
	INTCONbits.GIE = 0;				// disable interrupts
	_asm
		movlw	0x55
		movwf	EECON2, ACCESS
		movlw	0xAA
		movwf	EECON2, ACCESS
		bsf		EECON1, 1, ACCESS	//WR
	_endasm
	EECON1bits.WREN = 0;
	INTCONbits.GIE = 1;				// re-enable interrupts
}

void flashWriteBlock(u32 addr,u8* buff)
{
	u8 i = 0;
	u8 temp = 0;
	
	TBLPTR = addr;
	
	// Push data to write buffer
	for(i = 0;i<FLASH_WRITE_BLOCK;i++)
	{
		temp = buff[i];
		TABLAT = temp;
		_asm TBLWTPOSTINC _endasm
	}

	_asm TBLWTPOSTDEC _endasm
		
	// Copy the holding registers into FLASH.  This takes approximately 2.8ms.
	EECON1bits.FREE = 0;
	EECON1bits.WREN = 1;
	INTCONbits.GIE = 0;				// disable interrupts
	_asm
		movlw	0x55
		movwf	EECON2, ACCESS
		movlw	0xAA
		movwf	EECON2, ACCESS
		bsf		EECON1, 1, ACCESS	//WR
	_endasm
	while(EECON1bits.WR);
	EECON1bits.WREN = 0;
	INTCONbits.GIE = 1;				// re-enable interrupts
}

