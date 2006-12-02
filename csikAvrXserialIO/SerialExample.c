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
		AvrXDelay(&timer, 100);
	}
}

AVRX_GCC_TASKDEF(getCompassHeading, 76, 4)
{
	TimerControlBlock timer;
	
	while(1)
	{
		int c = 0;
		
		while (c<360)
		{	printf_P(PSTR("c %d"), c/3);   	// c/3 evaluates to an int, even it c is not a multiple of
			putchar('\r');					// 3.  This way, the heading is sent as ONE char instead of
			if (DEBUG) putchar('\n');		// one, two, or three.
			c++;
			AvrXDelay(&timer, 1000);
		}
		while (c>0)
		{	printf_P(PSTR("c %d"), c/3);
			putchar('\r');
			if (DEBUG) putchar('\n');		//We only want a newline if we are debugging on
			c--;							//a terminal.  Otherwise, we are communicating with
			AvrXDelay(&timer, 1000);		//the ground station and it should not be sent.
		}
	}
}

AVRX_GCC_TASKDEF(getAirspeed, 76, 4)
{
	TimerControlBlock timer;
	int s = 0;
	
	while(1)
	{	
		while (s<25)
		{	printf_P(PSTR("s %d"), s);   
			putchar('\r');
			if (DEBUG) putchar('\n');	
			s++;							
			AvrXDelay(&timer, 1000);
		}
		while (s>15)
		{	printf_P(PSTR("s %d"), s);
			putchar('\r');
			if (DEBUG) putchar('\n');
			s--;
			AvrXDelay(&timer, 1000);
		}
	}
}

AVRX_GCC_TASKDEF(getGroundspeed, 76, 4)
{
	TimerControlBlock timer;
	int g = 18;
	
	while(1)
	{	
		while (g<22)
		{	printf_P(PSTR("g %d"), g);   
			putchar('\r');
			if (DEBUG) putchar('\n');	
			g++;							
			AvrXDelay(&timer, 30*1000);
		}
		while (g>18)
		{	printf_P(PSTR("g %d"), g);
			putchar('\r');
			if (DEBUG) putchar('\n');
			g--;
			AvrXDelay(&timer, 30*1000);
		}
	}
}

AVRX_GCC_TASKDEF(getGPSData, 76, 4)
{
	TimerControlBlock timer;
	int z = 0;
	
	while(1)
	{	
		// Since latitude and longitude cannot be expressed as simple integers,
		// we cannot fake the measurements by cycling through some numbers.
		printf_P(PSTR("a 067.5759E"));   
		putchar('\r');
		if (DEBUG) putchar('\n');
		
		printf_P(PSTR("o 89.12345N"));	
		putchar('\r');
		if (DEBUG) putchar('\n');
		
		printf_P(PSTR("z %d"), z);
		putchar('\r');
		if (DEBUG) putchar('\n');
		
		if (z<100) z++;
		AvrXDelay(&timer, 1000);
		
		
		
		printf_P(PSTR("a 101.4512W"));   
		putchar('\r');
		if (DEBUG) putchar('\n');
		
		printf_P(PSTR("o 05.98765S"));   
		putchar('\r');
		if (DEBUG) putchar('\n');
		
		printf_P(PSTR("z %d"), z);
		putchar('\r');
		if (DEBUG) putchar('\n');
		
		if (z<100) z++;	
		AvrXDelay(&timer, 1000);
		
	}		
}

AVRX_GCC_TASKDEF(getFuelAndBattery, 76, 4)
{
	TimerControlBlock timer;
	int b = 100;
	int f = 100;
	
	while(1)
	{	
		printf_P(PSTR("b %d"), b);   
		putchar('\r');
		if (DEBUG) putchar('\n');	
		printf_P(PSTR("f %d"), f);
		putchar('\r');
		if (DEBUG) putchar('\n');							
		b--;
		f--;
		AvrXDelay(&timer, 30*1000);
	}
}


AVRX_GCC_TASKDEF(getPitchAndRoll, 76, 4)
{
	TimerControlBlock timer;
	int q = 0;
	int w = 127;
	
	while(1)
	{	
		while (q<127)
		{	printf_P(PSTR("q %d"), q);   
			putchar('\r');
			if (DEBUG) putchar('\n');	
			printf_P(PSTR("w %d"), w);
			putchar('\r');
			if (DEBUG) putchar('\n');
			q++;					
			w--;		
			AvrXDelay(&timer, 33);		//try 33Hz
		}
		while (q>0)
		{	printf_P(PSTR("q %d"), q);
			putchar('\r');
			if (DEBUG) putchar('\n');
			printf_P(PSTR("w %d"), w);
			putchar('\r');
			if (DEBUG) putchar('\n');
			q--;
			w++;
			AvrXDelay(&timer, 33);
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
        printf_P(PSTR("%d "),c);
        c++;
        AvrXDelay(&timer2, 1000); //supposed to be a 1000 ms delay
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

    InitSerial0(BAUD(9600));
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
//	AvrXRunTask(TCB(marktribe));
	AvrXRunTask(TCB(getUAVStatus));
	AvrXRunTask(TCB(getAirspeed));
	AvrXRunTask(TCB(getGroundspeed));
	AvrXRunTask(TCB(getCompassHeading));
	AvrXRunTask(TCB(getGPSData));
	AvrXRunTask(TCB(getPitchAndRoll));
	AvrXRunTask(TCB(getFuelAndBattery));
	//AvrXRunTask(TCB(servos));
    
	Epilog();
	return(0);
}

void setLeftServo(void)
{	
	leftServoPos = parserGetArgInt();
	servoSetPosition(LEFT_SERVO_CHAN, (char)leftServoPos);
	if (DEBUG)
	{	printf("Left Servo Set: %d", leftServoPos);
		putchar('\r');
		putchar('\n');
	}
}

void setRightServo(void)
{	
	rightServoPos = parserGetArgInt();
	servoSetPosition(RIGHT_SERVO_CHAN, (char)rightServoPos);
	if (DEBUG)
	{	printf("Right Servo Set: %d", rightServoPos);
		putchar('\r');
		putchar('\n');
	}
}

void setThrottleServo(void)
{
	throttleServoPos = parserGetArgInt();
	servoSetPosition(THROTTLE_SERVO_CHAN, (char)throttleServoPos);
	if (DEBUG)
	{	printf("Throttle Servo Set: %d", throttleServoPos);
		putchar('\r');
		putchar('\n');
	}
}

void setCamPanServo(void)
{
	camPanServoPos = parserGetArgInt();
	servoSetPosition(CAM_PAN_SERVO_CHAN, (char)camPanServoPos);
	if (DEBUG)
	{	printf("Camera Pan Servo Set: %d", camPanServoPos);
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
