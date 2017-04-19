#include <p18cxxx.h>
#include "uart.h"

#define FOSC 			25000000
#define BAUDRATE 		115200
#define BAUDRATE_UART2 	115200
#define BRGVAL 			((FOSC/BAUDRATE)/16-1)
#define BRGVAL_UART2	((FOSC/BAUDRATE_UART2)/16-1)

//=======================================================
rom unsigned char CharacterArray[]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

//=======================================================
void init_uart1(void)
{
	
	unsigned int i = 0;
	// Config baudrate
	TXSTA1bits.BRGH = 1;
	BAUDCON1bits.BRG16 = 0;
	TXSTA1bits.SYNC = 0;
	SPBRGH1 = BRGVAL>>8;
	SPBRG1 = BRGVAL;

	// Set direction for I/O port
	TRISCbits.TRISC6 = 0;		// PIN RG1 is output (TX pin)
	TRISCbits.TRISC7 = 1;		// PIN RG2 is input (RX pin)

	//Enable asynchronous serial port
	RCSTA1bits.SPEN = 1;
	// Choose 8-bits transmit mode
	TXSTA1bits.TX9 = 0;
	RCSTA1bits.ADDEN = 0;
	
	//Disable transmit interrupt
	PIE1bits.TX1IE = 0;
	//Enable receive interrupt
	PIE1bits.RC1IE = 1;
	INTCONbits.GIE = 1;
	INTCONbits.PEIE = 1;
	IPR1bits.TX1IP = 0;
	//Set high priority for receive interrupt
	IPR1bits.RC1IP = 1;
	
	// Clear interrupt flag
	PIR1bits.RC1IF = 0;

	//Enable the transmition
	TXSTA1bits.TXEN = 1;
	
	//Enable reception
	RCSTA1bits.CREN = 1;
	/* wait at least 104 usec (1/9600) before sending first char */ 
    for(i = 0; i < 1664; i++) 
    { 
     	Nop(); 
    }                    
}

void deinit_uart1(void)
{
	//Disable transmit interrupt
	PIE1bits.TX1IE = 0;
	//Disable receive interrupt
	PIE1bits.RC1IE = 0;	
	//Disable asynchronous serial port
	RCSTA1bits.SPEN = 0;
	//Disable reception
	RCSTA1bits.CREN = 0;
	//Disable the transmition
	TXSTA1bits.TXEN = 0;
}


//=======================================================
void uart1_putc(char c)
{
	while(!TXSTA1bits.TRMT) {}; // wait while Tx buffer full
	TXREG1 = c;	
}

//=======================================================
void uart1_putROMString(rom char* str)
{
    unsigned char c;
    while( c = *str++ )
	{
		while(!TXSTA1bits.TRMT);
        uart1_putc(c);
	}
}

//=======================================================
void uart1_putString(unsigned char *s)
{
    unsigned char c;

    while( (c = *s++) )
	{
		while(!TXSTA1bits.TRMT);
        uart1_putc(c);
	}
}

//=======================================================
unsigned char uart1_getc(void)
{
	return RCREG1; 
}

//=======================================================
void init_uart2(void)
{
	unsigned int i = 0;
	// Config baudrate
	TXSTA2bits.BRGH = 1;
	BAUDCON2bits.BRG16 = 0;
	TXSTA2bits.SYNC = 0;
	SPBRGH2 = BRGVAL_UART2>>8;
	SPBRG2 = BRGVAL_UART2;

	// Set direction for I/O port
	TRISGbits.TRISG1 = 0;		// PIN RG1 is output (TX pin)
	TRISGbits.TRISG2 = 1;		// PIN RG2 is input (RX pin)

	//Enable asynchronous serial port
	RCSTA2bits.SPEN = 1;	
	// Choose 8-bits transmit mode
	TXSTA2bits.TX9 = 0;		
	//Disable transmit interrupt
	PIE3bits.TX2IE = 0;
	//Enable receive interrupt
	PIE3bits.RC2IE = 1;
	INTCONbits.GIE = 1;
	INTCONbits.PEIE = 1;
	IPR3bits.TX2IP = 0;
	//Set high priority for receive interrupt
	IPR3bits.RC2IP = 1;
	// Clear interrupt flag
	PIR3bits.RC2IF = 0;	
	//Enable the transmition
	TXSTA2bits.TXEN = 1;
	//Enable reception
	RCSTA2bits.CREN = 1;	
	
	/* wait at least 104 usec (1/9600) before sending first char */ 
    for(i = 0; i < 1664; i++) 
    { 
     	Nop(); 
    }                    
}

void deinit_uart2(void)
{
	//Disable transmit interrupt
	PIE3bits.TX2IE = 0;
	//Enable receive interrupt
	PIE3bits.RC2IE = 0;	
	//Enable asynchronous serial port
	RCSTA2bits.SPEN = 0;
	//Enable reception
	RCSTA2bits.CREN = 0;
	//Enable the transmition
	TXSTA2bits.TXEN = 0;
}

//=======================================================

void uart2_putc(char c)
{
	while ( !TXSTA2bits.TRMT); // wait while Tx buffer full
	TXREG2 = c;
//	while ( !TXSTA1bits.TRMT); // wait while Tx buffer full
}

//=======================================================
void uart2_putROMString(rom char* str)
{
    unsigned char c;
    while( c = *str++ )
	{
		while(!TXSTA2bits.TRMT);
        uart2_putc(c);
	}
}

//=======================================================
void uart2_putString(char *s)
{
    char c;

    while( (c = *s++) )
	{
		while(!TXSTA2bits.TRMT);
        uart2_putc(c);
	}
}

//=======================================================
unsigned char uart2_getc(void)
{
	return RCREG2; 
}

//=======================================================
/*
void print_digit(unsigned char number)
{
   	unsigned char print_var;
	uart2_putROMString("0x");
   	print_var = (number>>4)&0x0F;
    uart2_putc(CharacterArray[print_var]);
   	print_var = (number&0x0F);
    uart2_putc(CharacterArray[print_var]);
}
*/
