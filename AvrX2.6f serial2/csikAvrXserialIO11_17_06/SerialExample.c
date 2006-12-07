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
#include <avrx-signal.h>
#include <stdio.h>
#include "avrx.h"
#include <avr/signal.h>	// include "signal" names (interrupt names)
#include <avr/interrupt.h>	// include interrupt support
#include <stdlib.h>

// Uncomment this to override "AvrXSerialIo.h and just use one channel
//#define USART_CHANNELS (1<1)	// 0 - USART0, 1 = USART1

#include "AvrXSerialIo.h"

#include "parserconf.h"
#include "parser.h"
#include "parser.c"


// global AVRLIB defines
#include <../../avrlibdefs.h>
// global AVRLIB types definitions
#include <../../avrlibtypes.h>
#include "global.h"

#include "timer.h"
#include "timer.c"
#include "servo.h"
#include "servo.c"
/*
enum
{
	FALSE,
	TRUE
};

typedef unsigned char BOOL;
*/

// Peripheral initialization

#define TCNT0_INIT (0xFF-CPUCLK/256/TICKRATE)
#define LEFT_SERVO_CHAN 	0
#define RIGHT_SERVO_CHAN 	1
#define THROTTLE_SERVO_CHAN 2


void testFunctionA(void);
void testFunctionB(void);

int funcBParam;
int funcAParam;

//long funcAParam;
//long funcBParam;


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

//#if (USART_CHANNELS & CHANNEL_0)

// This task uses GCC Libc stdio facility and needs an additional 60-80 bytes of stack
// for processing the strings.  Longer strings probably need more stack.
AVRX_GCC_TASKDEF(task0, 76, 4)
{
	TimerControlBlock timer;
	
	while(1)
	{
		int c = 0;
		
		while (c<360)
		{	printf_P(PSTR("c %d"), c);
			putchar('\r'),putchar('\n');
			c++;
			AvrXDelay(&timer, 100);
		}
		while (c > 0)
		{	printf_P(PSTR("c %d"), c);
			putchar('\r'),putchar('\n');
			c--;
			AvrXDelay(&timer, 100);
		}
	}
}
AVRX_GCC_TASKDEF(getCommands, 100, 5)
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

AVRX_GCC_TASKDEF(marktribe, 70, 3)
{
	TimerControlBlock timer2;
    char c = 'a';
    printf_P(PSTR("I founded Rhizome!\r\n"));
    while(1)
    {    
        printf_P(PSTR("%c "),c);
        c++;
        AvrXDelay(&timer2, 10000); //supposed to be a 1000 ms delay
    }
    
}
	
/*
AVRX_GCC_TASKDEF(servos, 120, 1)
{
	TimerControlBlock timer3;
	
	u08 pos;
	u08 channel;

	// do some examples
	// initialize RC servo system
	servoInit();
	// setup servo output channel-to-I/Opin mapping
	// format is servoSetChannelIO( CHANNEL#, PORT, PIN );
	servoSetChannelIO(0, _SFR_IO_ADDR(PORTC), PC0);


	// set port pins to output
	outb(DDRC, 0x01);

	pos = 0;
	
	#define SPEED_SERVO	1

	// spin servos sequentially back and forth between their limits
	while(1)
	{
		for(channel=0; channel<SERVO_NUM_CHANNELS; channel++)
		{
			for(pos=0; pos<SERVO_POSITION_MAX; pos++)
			{
				servoSetPosition(channel,pos);
				AvrXDelay(&timer3, 1000);;
			}
		}

		for(channel=0; channel<SERVO_NUM_CHANNELS; channel++)
		{
			for(pos=SERVO_POSITION_MAX; pos>=1; pos--)
			{
				servoSetPosition(channel,pos);
				AvrXDelay(&timer3, 1000);;
			}
		}
	}
}
*/
//#endif // USART_CHANNELS & CHANNEL_0

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
    fdevopen(put_char0, get_c0,0);		// Set up standard I/O

	// initialize parser system
	parserInit();
	// direct output to uart (serial port)
	parserSetOutputFunc(put_char0);
	// add commands to the command database
	parserAddCommand("a",		testFunctionA);
	parserAddCommand("b",		testFunctionB);
    
	// initialize the timer system -- FROM AVRLIB
	//timerInit();
	
	//////////////////////////////////////////////////Servos//////////////////////////
	servoInit();
	// setup servo output channel-to-I/Opin mapping
	// format is servoSetChannelIO( CHANNEL#, PORT, PIN );
	servoSetChannelIO(0, _SFR_IO_ADDR(PORTC), PC0);
	servoSetChannelIO(1, _SFR_IO_ADDR(PORTC), PC1);

	// set port pins to output
	outb(DDRC, 0x03);

	
	#define SPEED_SERVO	1
	//////////////////////////////////////////////////////////////////////////////////
	
	AvrXRunTask(TCB(getCommands));
	AvrXRunTask(TCB(marktribe));
	AvrXRunTask(TCB(task0));
	//AvrXRunTask(TCB(servos));
    
	Epilog();
	return(0);
}

void testFunctionA(void)
{	

	funcAParam = parserGetArgInt();
	printf("test successful: 'a' function called with argument %d", funcAParam);
	servoSetPosition(LEFT_SERVO_CHAN,(char) funcAParam);
	putchar('\r');
	putchar('\n');
}

void testFunctionB(void)
{	
	
	
	funcBParam = parserGetArgInt();
	printf("test successful: 'b' function called with argument %d", funcBParam);
	servoSetPosition(RIGHT_SERVO_CHAN,(char) funcBParam);
	putchar('\r');
	putchar('\n');
}
