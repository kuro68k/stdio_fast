/*
 * stdio_fast.c
 *
 * Created: 22/06/2015 11:26:17
 *  Author: mojo-chan
 */ 


#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include "stdio_fast.h"


int main(void)
{
	STDIO_init();
	puts_P(PSTR("test"));
	puts("test");
	putchar('t');
	for(;;);
}




/**************************************************************************************************
** Output a character to the debug USART
*/
int fast_putc(char c, FILE *stream)
{
	while(!(STDIO_USART.STATUS & USART_DREIF_bm));
	STDIO_USART.DATA = c;
	return 0;
}

FILE usart_str = FDEV_SETUP_STREAM(fast_putc, NULL, _FDEV_SETUP_WRITE);


/**************************************************************************************************
* Set up stdio redirection and USART
*/
void STDIO_init(void)
{
	STDIO_USART.BAUDCTRLA = (uint8_t)STDIO_USART_BSEL;
	STDIO_USART.BAUDCTRLB = (STDIO_USART_BSCALE << 4) | (STDIO_USART_BSEL >> 8);
	STDIO_USART.CTRLA = 0;
	STDIO_USART.CTRLB = USART_TXEN_bm | STDIO_USART_CLK2X;
	STDIO_USART.CTRLC = USART_CHSIZE_8BIT_gc;
	STDIO_USART.CTRLD = 0;
	
	stdout = &usart_str;
}