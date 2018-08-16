/*
 * stdio_fast.c
 *
 * Created: 22/06/2015 11:26:17
 *  Author: kuro68k
 */ 


#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "stdio_fast.h"


// from xtoa-fast.h from avr-libc
extern char * __ultoa_invert (unsigned long val, char *s, int base);
#define XTOA_PREFIX	0x0100	/* put prefix for octal or hex	*/
#define XTOA_UPPER	0x0200	/* use upper case letters	*/

// from ftoa_engine.h from avr-libc
int __ftoa_engine (double val, char *buf,
                   unsigned char prec, unsigned char maxdgs);

#define	FTOA_MINUS	1
#define	FTOA_ZERO	2
#define	FTOA_INF	4
#define	FTOA_NAN	8
#define	FTOA_CARRY	16	/* Carry was to master position.	*/



int main(void)
{
	STDIO_init();
	/*
	puts_P(PSTR("test"));
	puts("test");
	putchar('t');
	*/
	printf("test %u", 42);
	asm("nop");
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

/**************************************************************************************************
* Faster printf()
*/
#if PRINTF_LEVEL <= PRINTF_MIN

#define FL_PGMSTRING	0x01
#define FL_ALTHEX	0x04
#define FL_ALT		0x10
#define FL_ALTLWR	0x20
#define FL_NEGATIVE	0x40
#define FL_LONG 	0x80

int printf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	unsigned char c;		/* holds a char from the format string */
	unsigned char flags;
	unsigned char buf[11];	/* size for -1 in octal, without '\0'	*/
	int char_count = 0;

	for (;;) {

		for (;;) {
			c = *fmt++;
			if (!c) goto ret;
			if (c == '%') {
			c = *fmt++;
			if (c != '%') break;
			}
		__putchar(c);
		char_count++;
	}

	for (flags = 0; !(flags & FL_LONG);	c = *fmt++)		/* 'll' will detect as error */
	{
		if (c && strchr_P (PSTR(" +-.0123456789h"), c))
			continue;
		if (c == '#') {
			flags |= FL_ALT;
			continue;
		}
		if (c == 'l') {
			flags |= FL_LONG;
			continue;
		}
		break;
	}

	/* Only a format character is valid.	*/

	if (c && strchr_P (PSTR("EFGefg"), c)) {
		(void) va_arg (ap, double);
		__putchar('?');
		char_count++;
		continue;
	}

	{
		const char * pnt;

		switch (c) {

			case 'c':
				__putchar(va_arg (ap, int));
				char_count++;
				continue;

			case 'S':
				flags |= FL_PGMSTRING;
				pnt = va_arg (ap, char *);
				while ( (c = *(const __flash char *)pnt++) != 0)
				{
					__putchar(c);
					char_count++;
				}
				continue;

			case 's':
				pnt = va_arg (ap, char *);
				while ( (c = *pnt++) != 0)
				{
					__putchar(c);
					char_count++;
				}
				continue;
		}
	}

	if (c == 'd' || c == 'i') {
		long x = (flags & FL_LONG) ? va_arg(ap,long) : va_arg(ap,int);
		flags &= ~FL_ALT;
		if (x < 0) {
			x = -x;
			/* `putc ('-', stream)' will considarably inlarge stack size.
			   So flag is used.	*/
			flags |= FL_NEGATIVE;
		}
		c = __ultoa_invert (x, (char *)buf, 10) - (char *)buf;

	} else {
		int base;

		switch (c) {
			case 'u':
				flags &= ~FL_ALT;
				base = 10;
				goto ultoa;
			case 'o':
				base = 8;
				goto ultoa;
			case 'p':
				flags |= FL_ALT;
			/* no break */
			case 'x':
				flags |= (FL_ALTHEX | FL_ALTLWR);
				base = 16;
				goto ultoa;
			case 'X':
				flags |= FL_ALTHEX;
				base = 16 | XTOA_UPPER;
			ultoa:
				c = __ultoa_invert ((flags & FL_LONG)
					? va_arg(ap, unsigned long)
					: va_arg(ap, unsigned int),
					(char *)buf, base)  -  (char *)buf;
			break;

			default:
				goto ret;
		}
	}

	/* Integer number output.	*/
	if (flags & FL_NEGATIVE)
	{
		__putchar('-');
		char_count++;
	}
	if ((flags & FL_ALT) && (buf[c-1] != '0')) {
		__putchar('0');
		char_count++;
		if (flags & FL_ALTHEX)
		{
			__putchar('X' + (flags & FL_ALTLWR));
			char_count++;
		}
	}
	do {
		__putchar(buf[--c]);
		char_count++;
	} while (c);

	} /* for (;;) */

	va_end(ap);

ret:
	return char_count;
}

#else	// i.e. PRINTF_LEVEL > PRINTF_MIN

#define FL_ZFILL	0x01
#define FL_PLUS		0x02
#define FL_SPACE	0x04
#define FL_LPAD		0x08
#define FL_ALT		0x10
#define FL_WIDTH	0x20
#define FL_PREC		0x40
#define FL_LONG		0x80

#define FL_PGMSTRING	FL_LONG
#define FL_NEGATIVE	FL_LONG

#define FL_ALTUPP	FL_PLUS
#define FL_ALTHEX	FL_SPACE

#define	FL_FLTUPP	FL_ALT
#define FL_FLTEXP	FL_PREC
#define	FL_FLTFIX	FL_LONG

int printf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

    unsigned char c;		/* holds a char from the format string */
    unsigned char flags;
    unsigned char width;
    unsigned char prec;
    unsigned char buf[11];	/* size for -1 in octal, without '\0'	*/
	int char_count = 0;

    for (;;) {
		for (;;) {
			c = *fmt++;
			if (!c) goto ret;
			if (c == '%') {
			c = *fmt++;
			if (c != '%') break;
		}
		__putchar(c);
		char_count++;
	}

	flags = 0;
	width = 0;
	prec = 0;
	
	do {
		if (flags < FL_WIDTH) {
			switch (c) {
				case '0':
					flags |= FL_ZFILL;
					continue;
				case '+':
					flags |= FL_PLUS;
					/* FALLTHROUGH */
				case ' ':
					flags |= FL_SPACE;
					continue;
				case '-':
					flags |= FL_LPAD;
					continue;
				case '#':
					flags |= FL_ALT;
					continue;
			}
	    }

	    if (flags < FL_LONG) {
			if (c >= '0' && c <= '9') {
				c -= '0';
				if (flags & FL_PREC) {
					prec = 10*prec + c;
					continue;
				}
		    width = 10*width + c;
		    flags |= FL_WIDTH;
		    continue;
			}
			if (c == '.') {
				if (flags & FL_PREC)
				goto ret;
				flags |= FL_PREC;
				continue;
			}
			if (c == 'l') {
				flags |= FL_LONG;
				continue;
			}
			if (c == 'h')
				continue;
		}
	    
		break;
	} while ( (c = *fmt++) != 0);

	/* Only a format character is valid.	*/

#if	'F' != 'E'+1  ||  'G' != 'F'+1  ||  'f' != 'e'+1  ||  'g' != 'f'+1
# error
#endif

#if PRINTF_LEVEL >= PRINTF_FLT
	if (c >= 'E' && c <= 'G') {
	    flags |= FL_FLTUPP;
	    c += 'e' - 'E';
	    goto flt_oper;

	} else if (c >= 'e' && c <= 'g') {

	    int exp;		/* exponent of master decimal digit	*/
	    int n;
	    unsigned char vtype;	/* result of float value parse	*/
	    unsigned char sign;		/* sign character (or 0)	*/
# define ndigs	c		/* only for this block, undef is below	*/

	    flags &= ~FL_FLTUPP;

	  flt_oper:
	    if (!(flags & FL_PREC))
		prec = 6;
	    flags &= ~(FL_FLTEXP | FL_FLTFIX);
	    if (c == 'e')
		flags |= FL_FLTEXP;
	    else if (c == 'f')
		flags |= FL_FLTFIX;
	    else if (prec > 0)
		prec -= 1;

	    if (flags & FL_FLTFIX) {
		vtype = 7;		/* 'prec' arg for 'ftoa_engine'	*/
		ndigs = prec < 60 ? prec + 1 : 60;
	    } else {
		if (prec > 7) prec = 7;
		vtype = prec;
		ndigs = 0;
	    }
	    exp = __ftoa_engine (va_arg(ap,double), (char *)buf, vtype, ndigs);
	    vtype = buf[0];
    
	    sign = 0;
	    if ((vtype & FTOA_MINUS) && !(vtype & FTOA_NAN))
		sign = '-';
	    else if (flags & FL_PLUS)
		sign = '+';
	    else if (flags & FL_SPACE)
		sign = ' ';

	    if (vtype & (FTOA_NAN | FTOA_INF)) {
		const char *p;
		ndigs = sign ? 4 : 3;
		if (width > ndigs) {
		    width -= ndigs;
		    if (!(flags & FL_LPAD)) {
			do {
			    __putchar(' ');
				char_count++;
			} while (--width);
		    }
		} else {
		    width = 0;
		}
		if (sign)
		{
		    __putchar(sign);
			char_count++;
		}
		p = PSTR("inf");
		if (vtype & FTOA_NAN)
		    p = PSTR("nan");
# if ('I'-'i' != 'N'-'n') || ('I'-'i' != 'F'-'f') || ('I'-'i' != 'A'-'a')
#  error
# endif
		while ( (ndigs = pgm_read_byte(p)) != 0) {
		    if (flags & FL_FLTUPP)
			ndigs += 'I' - 'i';
		    __putchar(ndigs);
			char_count++;
		    p++;
		}
		goto tail;
	    }

	    /* Output format adjustment, number of decimal digits in buf[] */
	    if (flags & FL_FLTFIX) {
		ndigs += exp;
		if ((vtype & FTOA_CARRY) && buf[1] == '1')
		    ndigs -= 1;
		if ((signed char)ndigs < 1)
		    ndigs = 1;
		else if (ndigs > 8)
		    ndigs = 8;
	    } else if (!(flags & FL_FLTEXP)) {		/* 'g(G)' format */
		if (exp <= prec && exp >= -4)
		    flags |= FL_FLTFIX;
		while (prec && buf[1+prec] == '0')
		    prec--;
		if (flags & FL_FLTFIX) {
		    ndigs = prec + 1;		/* number of digits in buf */
		    prec = prec > exp
			   ? prec - exp : 0;	/* fractional part length  */
		}
	    }
    
	    /* Conversion result length, width := free space length	*/
	    if (flags & FL_FLTFIX)
		n = (exp>0 ? exp+1 : 1);
	    else
		n = 5;		/* 1e+00 */
	    if (sign) n += 1;
	    if (prec) n += prec + 1;
	    width = width > n ? width - n : 0;
    
	    /* Output before first digit	*/
	    if (!(flags & (FL_LPAD | FL_ZFILL))) {
		while (width) {
		    __putchar(' ');
			char_count++;
		    width--;
		}
	    }
	    if (sign) {
			__putchar(sign);
			char_count++;
		}
	    if (!(flags & FL_LPAD)) {
		while (width) {
		    __putchar('0');
			char_count++;
		    width--;
		}
	    }
    
	    if (flags & FL_FLTFIX) {		/* 'f' format		*/

		n = exp > 0 ? exp : 0;		/* exponent of left digit */
		do {
		    if (n == -1)
			__putchar('.');
			char_count++;
		    flags = (n <= exp && n > exp - ndigs)
			    ? buf[exp - n + 1] : '0';
		    if (--n < -prec)
			break;
		    __putchar(flags);
			char_count++;
		} while (1);
		if (n == exp
		    && (buf[1] > '5'
		        || (buf[1] == '5' && !(vtype & FTOA_CARRY))) )
		{
		    flags = '1';
		}
		__putchar(flags);
		char_count++;
	
	    } else {				/* 'e(E)' format	*/

		/* mantissa	*/
		if (buf[1] != '1')
		    vtype &= ~FTOA_CARRY;
		__putchar(buf[1]);
		char_count++;
		if (prec) {
		    __putchar('.');
			char_count++;
		    sign = 2;
		    do {
			__putchar(buf[sign++]);
			char_count++;
		    } while (--prec);
		}

		/* exponent	*/
		__putchar(flags & FL_FLTUPP ? 'E' : 'e');
		char_count++;
		ndigs = '+';
		if (exp < 0 || (exp == 0 && (vtype & FTOA_CARRY) != 0)) {
		    exp = -exp;
		    ndigs = '-';
		}
		__putchar(ndigs);
		char_count++;
		for (ndigs = '0'; exp >= 10; exp -= 10)
		    ndigs += 1;
		__putchar(ndigs);
		char_count++;
		__putchar('0' + exp);
		char_count++;
	    }

	    goto tail;
# undef ndigs
	}

#else		/* to: PRINTF_LEVEL >= PRINTF_FLT */
	if ((c >= 'E' && c <= 'G') || (c >= 'e' && c <= 'g')) {
	    (void) va_arg (ap, double);
	    buf[0] = '?';
	    goto buf_addr;
	}

#endif

	{
	    const char * pnt;
	    size_t size;

	    switch (c) {

	      case 'c':
		buf[0] = va_arg (ap, int);
#if  PRINTF_LEVEL < PRINTF_FLT
	      buf_addr:
#endif
		pnt = (char *)buf;
		size = 1;
		goto no_pgmstring;

	      case 's':
		pnt = va_arg (ap, char *);
		size = strnlen (pnt, (flags & FL_PREC) ? prec : ~0);
	      no_pgmstring:
		flags &= ~FL_PGMSTRING;
		goto str_lpad;

	      case 'S':
	        pnt = va_arg (ap, char *);
		size = strnlen_P (pnt, (flags & FL_PREC) ? prec : ~0);
		flags |= FL_PGMSTRING;

	      str_lpad:
		if (!(flags & FL_LPAD)) {
		    while (size < width) {
			__putchar(' ');
			char_count++;
			width--;
		    }
		}
		while (size) {
		    __putchar(*(const __flash char *)pnt);
			char_count++;
		    if (width) width -= 1;
		    size -= 1;
		}
		goto tail;
	    }
	}

	if (c == 'd' || c == 'i') {
	    long x = (flags & FL_LONG) ? va_arg(ap,long) : va_arg(ap,int);
	    flags &= ~(FL_NEGATIVE | FL_ALT);
	    if (x < 0) {
		x = -x;
		flags |= FL_NEGATIVE;
	    }
	    c = __ultoa_invert (x, (char *)buf, 10) - (char *)buf;

	} else {
	    int base;

	    if (c == 'u') {
		flags &= ~FL_ALT;
		base = 10;
		goto ultoa;
	    }

	    flags &= ~(FL_PLUS | FL_SPACE);

	    switch (c) {
	      case 'o':
	        base = 8;
		goto ultoa;
	      case 'p':
	        flags |= FL_ALT;
		/* no break */
	      case 'x':
		if (flags & FL_ALT)
		    flags |= FL_ALTHEX;
	        base = 16;
		goto ultoa;
	      case 'X':
		if (flags & FL_ALT)
		    flags |= (FL_ALTHEX | FL_ALTUPP);
	        base = 16 | XTOA_UPPER;
	      ultoa:
		c = __ultoa_invert ((flags & FL_LONG)
				    ? va_arg(ap, unsigned long)
				    : va_arg(ap, unsigned int),
				    (char *)buf, base)  -  (char *)buf;
		flags &= ~FL_NEGATIVE;
		break;

	      default:
	        goto ret;
	    }
	}

	{
	    unsigned char len;

	    len = c;
	    if (flags & FL_PREC) {
		flags &= ~FL_ZFILL;
		if (len < prec) {
		    len = prec;
		    if ((flags & FL_ALT) && !(flags & FL_ALTHEX))
			flags &= ~FL_ALT;
		}
	    }
	    if (flags & FL_ALT) {
		if (buf[c-1] == '0') {
		    flags &= ~(FL_ALT | FL_ALTHEX | FL_ALTUPP);
		} else {
		    len += 1;
		    if (flags & FL_ALTHEX)
		    	len += 1;
		}
	    } else if (flags & (FL_NEGATIVE | FL_PLUS | FL_SPACE)) {
		len += 1;
	    }

	    if (!(flags & FL_LPAD)) {
		if (flags & FL_ZFILL) {
		    prec = c;
		    if (len < width) {
			prec += width - len;
			len = width;
		    }
		}
		while (len < width) {
		    __putchar(' ');
			char_count++;
		    len++;
		}
	    }
	
	    width =  (len < width) ? width - len : 0;

	    if (flags & FL_ALT) {
		__putchar('0');
		char_count++;
		if (flags & FL_ALTHEX)
		{
		    __putchar(flags & FL_ALTUPP ? 'X' : 'x');
			char_count++;
		}
	    } else if (flags & (FL_NEGATIVE | FL_PLUS | FL_SPACE)) {
		unsigned char z = ' ';
		if (flags & FL_PLUS) z = '+';
		if (flags & FL_NEGATIVE) z = '-';
		__putchar(z);
		char_count++;
	    }
		
	    while (prec > c) {
		__putchar('0');
		char_count++;
		prec--;
	    }
	
	    do {
		__putchar(buf[--c]);
		char_count++;
	    } while (c);
	}
	
      tail:
	/* Tail is possible.	*/
	while (width) {
	    __putchar(' ');
		char_count++;
	    width--;
	}
    } /* for (;;) */

	va_end(ap);

ret:
    return char_count;
}

#endif
