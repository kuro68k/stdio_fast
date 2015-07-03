# stdio_fast
Fast stdio output functions for AVR UARTs

AVR Libc provides stdio.h with functions that can be reidrected. Unortunately it is somewhat slow. For example, printf_P() does not optimize simple strings to a puts_P(), and even if it did puts_P() is somewhat slow due to having to call a user supplied character output function multiple times.

printf_P(PSTR("test"));     ->      604 cycles
puts_P(PSTR("test"));       ->      246 cycles
usart_write(PSTR("test"));  ->      123 cycles

printf("test %u", 42);		->		1525 cycles (avr-libc, no floats)
printf("test %u", 42);		->		1082 cycles (fast version, no floats)

The included functions set up the AVR's USART and redirect stdio.h functions to use it. They also replace some of the functions (putchar, getchar, puts and puts_P) with faster versions which use inline code.

Note that puts() and puts_P() append a newline character to every string. This behaviour can be configured in stdio_fast.h.

To use, simple delete main(), include stdio_fast.c and stdio_fast.h in your project and set the baud rate. The current USART code is written for XMEGA devices, trivial modifications are required for other members of the AVR family.
