/*
 * stdio_fast.h
 *
 * Created: 22/06/2015 11:29:21
 *  Author: paul.qureshi
 */ 


#ifndef STDIO_FAST_H_
#define STDIO_FAST_H_


#define STDIO_USART				USARTC0
#define STDIO_USART_BSEL		0
#define STDIO_USART_BSCALE		0
#define STDIO_USART_CLK2X		USART_CLK2X_bm
#define STDIO_NEWLINE_RN


extern void STDIO_init(void);


/**************************************************************************************************
* putchar() replacement
*/
#undef	putchar
#define putchar	__putchar
static inline int __putchar(int character)
{
	while(!(STDIO_USART.STATUS & USART_DREIF_bm));
	STDIO_USART.DATA = character;
	return character;
}


/**************************************************************************************************
* getchar() replacement
*/
#undef	getchar
#define getchar	__getchar
static inline int __getchar(void)
{
	if (STDIO_USART.STATUS & USART_RXCIF_bm)
		return STDIO_USART.DATA;
	else
		return EOF;
}

/**************************************************************************************************
* puts_P() replacement
*/
inline int puts_P(const char * __str)
{
	const __flash char * s = __str;
	
	while (*s != '\0')
	{
		while(!(STDIO_USART.STATUS & USART_DREIF_bm));
		STDIO_USART.DATA = *s++;
	}

	// newline
#ifdef STDIO_NEWLINE_RN
	while(!(STDIO_USART.STATUS & USART_DREIF_bm));
	STDIO_USART.DATA = '\r';
#endif
	while(!(STDIO_USART.STATUS & USART_DREIF_bm));
	STDIO_USART.DATA = '\n';
	
	return 1;
}


/**************************************************************************************************
* puts() replacement
*/
inline int puts(const char * __str)
{
	while (*__str != '\0')
	{
		while(!(STDIO_USART.STATUS & USART_DREIF_bm));
		STDIO_USART.DATA = *__str++;
	}

	// newline
#ifdef STDIO_NEWLINE_RN
	while(!(STDIO_USART.STATUS & USART_DREIF_bm));
	STDIO_USART.DATA = '\r';
#endif
	while(!(STDIO_USART.STATUS & USART_DREIF_bm));
	STDIO_USART.DATA = '\n';
	
	return 1;
}



#endif /* STDIO_FAST_H_ */