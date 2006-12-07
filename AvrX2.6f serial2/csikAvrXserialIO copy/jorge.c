/*
    Serial interface Demo for AvrXFifo's

    Also illustrates the use of Frame based variables
    at the top level tasking (switched from NAKED to
    NORETURN function attribute)

	When linked with simple serialio there is no buffering
	of charactors so only two charactors can be received
	during the delay (Hardware buffering within the USART).

	When linked with the buffered IO up to 31 (or whatever
	the buffer size - 1 is) charactors can be received while
	delaying.
*/
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <stdlib.h>
#include "avrx.h"

// Uncomment this to override "AvrXSerialIo.h and just use one channel
//#define USART_CHANNELS (1<1)	// 0 - USART0, 1 = USART1

#include "AvrXSerialIo.h"

#include "parser.h"

enum
{
	FALSE,
	TRUE
};

typedef unsigned char BOOL;


void testFunctionA(void);
void testFunctionB(void);

long funcAParam;
long funcBParam;


// Peripheral initialization

#define TCNT0_INIT (0xFF-CPUCLK/256/TICKRATE)

/*
 Timer 0 Overflow Interrupt Handler

 Prototypical Interrupt handler:
 . Switch to kernel context
 . handle interrupt
 . switch back to interrupted context.
 */
AVRX_SIGINT(SIG_OVERFLOW0)
{
    IntProlog();                // Switch to kernel stack/context
    TCNT0 += TCNT0_INIT;		// Add to pre-load to account for any missed clocks
    AvrXTimerHandler();         // Call Time queue manager
    Epilog();                   // Return to tasks
}

// Super simple string printers...

// PutString from RAM
void myputs(int (*putch)(char), const uint8_t * psz)
{
	while (*psz != 0)
		(*putch)(*psz++);
}

// PutString from FLASH
void myputs_P(int (*putch)(char), const uint8_t * psz)
{
	while (__LPM(psz) != 0)
		(*putch)(__LPM(psz++));
}

// This task uses GCC Libc stdio facility and needs an additional 60-80 bytes of stack
// for processing the strings.  Longer strings probably need more stack.

AVRX_GCC_TASKDEF(task0, 76, 1)
{
	TimerControlBlock timer;
	
	while(1)
	{
		int c = 0;
		
		while (c<360)
		{	printf_P(PSTR("comp %d"), c);
			putchar('\r'),putchar('\n');
			c++;
			AvrXDelay(&timer, 100);
		}
		while (c > 0)
		{	printf_P(PSTR("comp %d"), c);
			putchar('\r'),putchar('\n');
			c--;
			AvrXDelay(&timer, 100);
		}
	}
}

AVRX_GCC_TASKDEF(task1, 76, 1)
{
	TimerControlBlock timer;
	
	while(1)
	{
		printf_P(PSTR("batt 100"));
		putchar('\r'),putchar('\n');
		AvrXDelay(&timer, 2*1000);
	}
}

AVRX_GCC_TASKDEF(task2, 76, 2)
{	
	int c;		
	
	while (1)
	{
		while ((c = getchar()) != EOF)
		{	
			if (c == '\r')
			{	putchar('\r');
				putchar('\n');
			}
			parserInputFunc(c);
		}
	}
}


int main(void)
{
    AvrXSetKernelStack(0);

	MCUCR = _BV(SE);
	TCNT0 = TCNT0_INIT;
#if defined (__AVR_ATmega103__) || defined (__ATmega103__)
	TCCR0 =  ((1<<CS02) | (1<<CS01));
#elif defined (__AVR_ATmega128__) || defined (__ATmega128__) || defined (__AVR_ATmega64__) || defined (__ATmega64__)
	TCCR0 =  ((1<<CS2) | (1<<CS1));
#else	// Most other chips...  Note: some are TCCR0 and some are TCCR0B...
	TCCR0 =  (1<<CS02);
#endif
	TIMSK = _BV(TOIE0);

	InitSerial0(BAUD(57600));
    fdevopen(put_char0, get_c0);
	
	// initialize parser system
	parserInit();
	// direct output to uart (serial port)
	parserSetOutputFunc(putchar);
	// add commands to the command database
	parserAddCommand("a",		testFunctionA);
	parserAddCommand("b",		testFunctionB);

	AvrXRunTask(TCB(task0));
	AvrXRunTask(TCB(task1));
	AvrXRunTask(TCB(task2));

	Epilog();
}

void testFunctionA(void)
{	
	char* endptr;
	
	funcAParam = parserGetArgInt();
	printf("test successful: 'a' function called with argument %d", funcAParam);
	putchar('\r');
	putchar('\n');
}

void testFunctionB(void)
{	
	funcBParam = parserGetArgInt();
	printf("test successful: 'b' function called with argument %d", funcBParam);
	putchar('\r');
	putchar('\n');
}
