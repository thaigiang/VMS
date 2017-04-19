#ifndef _UART_H_
#define _UART_H_

//=======================================================
void init_uart1(void);
void deinit_uart1(void);
void uart1_putc(char c);
void uart1_putROMString(rom char* str);
void uart1_putString(unsigned char *s);
unsigned char uart1_getc(void);

void init_uart2(void);
void deinit_uart2(void);
void uart2_putc(char c);
void uart2_putROMString(rom char* str);
void uart2_putString(char *s);
unsigned char uart2_getc(void);
//void print_digit(unsigned char number);
#endif
