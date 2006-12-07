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
//#include "../../avrlibdefs.h"
//#include <avrlibdefs.h>
// global AVRLIB types definitions
//#include "../../avrlibtypes.h"
//#include <avrlibdefs.h>

#include "global.h"

//#include "timer.h"
///#include "timer.c"
//#include "servo.h"
//#include "servo.c"
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
#define CAM_PAN_SERVO_CHAN  3
#define CAM_TILT_SERVO_CHAN 4

#define DEBUG 1

void setLeftServo(void);
void setRightServo(void);
void setThrottleServo(void);
void setCamPanServo(void);
void setCamTiltServo(void);

int leftServoPos = 50;		//0 seems to be beyond its reach
int rightServoPos;
int throttleServoPos;
int camPanServoPos;
int camTiltServoPos;

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

//tell the ground station I am OK by sending a "1" 10 times a second
AVRX_GCC_TASKDEF(getUAVStatus, 76, 4)
{	
	TimerControlBlock timer;
	
	while(1)
	{
		printf_P(PSTR("1"));
		putchar('\r');
		if (DEBUG) putchar('\n');
		AvrXDelay(&timer, 250);
		printf_P(PSTR("c %d,"), 120);   	// c/3 evaluates to an int, even it c is not a multiple of
		printf_P(PSTR("a 072.5759E,")); 
		printf_P(PSTR("a 043.3223n,")); 
		AvrXDelay(&timer, 250);
		printf_P(PSTR("s %d,"), 3);  
		printf_P(PSTR("g %d,"), 5);
		printf_P(PSTR("f %d,"), 80); 
		printf_P(PSTR("b %d,"), 35); 
		printf_P(PSTR("q %d,"), 23);  
		printf_P(PSTR("w %d,"), 70);
	}
}







AVRX_GCC_TASKDEF(getCommands, 100, 1)
{	
	int c;		
	TimerControlBlock timer;
	
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
		AvrXDelay(&timer, 5);
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
    fdevopen(put_char0, get_c0,0);		// Set up standard I/O

	// initialize parser system
	parserInit();
	// direct output to uart (serial port)
	parserSetOutputFunc(put_char0);
	// add commands to the command database
	parserAddCommand("l",		setLeftServo);
	parserAddCommand("r",		setRightServo);
    parserAddCommand("t", 		setThrottleServo);
	parserAddCommand("p", 		setCamPanServo);
	parserAddCommand("i", 		setCamTiltServo);
	
	// initialize the timer system -- FROM AVRLIB
	//timerInit();
	
	//////////////////////////////////////////////////Servos//////////////////////////
	servoInit();
	// setup servo output channel-to-I/Opin mapping
	// format is servoSetChannelIO( CHANNEL#, PORT, PIN );
	servoSetChannelIO(0, _SFR_IO_ADDR(PORTC), PC0);
	servoSetChannelIO(1, _SFR_IO_ADDR(PORTC), PC1);
	servoSetChannelIO(2, _SFR_IO_ADDR(PORTC), PC2);
	servoSetChannelIO(3, _SFR_IO_ADDR(PORTC), PC3);
	servoSetChannelIO(4, _SFR_IO_ADDR(PORTC), PC4);

	// set port pins to output
	outb(DDRC, 0x1F);

	
	#define SPEED_SERVO	1
	//////////////////////////////////////////////////////////////////////////////////
	
	AvrXRunTask(TCB(getCommands));
	AvrXRunTask(TCB(getUAVStatus));

    
	Epilog();
	return(0);
}

void setLeftServo(void)
{	
	leftServoPos = parserGetArgInt();
	servoSetPosition(LEFT_SERVO_CHAN, (char)leftServoPos);
	if (DEBUG)
	{	printf("e0");
		putchar('\r');
		putchar('\n');
	}
}

void setRightServo(void)
{	
	rightServoPos = parserGetArgInt();
	servoSetPosition(RIGHT_SERVO_CHAN, (char)rightServoPos);
	if (DEBUG)
	{	printf("e0");
		putchar('\r');
		putchar('\n');
	}
}

void setThrottleServo(void)
{
	throttleServoPos = parserGetArgInt();
	servoSetPosition(THROTTLE_SERVO_CHAN, (char)throttleServoPos);
	if (DEBUG)
	{	printf("e0");
		putchar('\r');
		putchar('\n');
	}
}

void setCamPanServo(void)
{
	camPanServoPos = parserGetArgInt();
	servoSetPosition(CAM_PAN_SERVO_CHAN, (char)camPanServoPos);
	if (DEBUG)
	{	printf("e0");
		putchar('\r');
		putchar('\n');
	}
}

void setCamTiltServo(void)
{
	camTiltServoPos = parserGetArgInt();
	servoSetPosition(CAM_TILT_SERVO_CHAN, (char)camTiltServoPos);
	if (DEBUG)
	{	printf("Camera Tilt Servo Set: %d", camTiltServoPos);
		putchar('\r');
		putchar('\n');
	}
}
