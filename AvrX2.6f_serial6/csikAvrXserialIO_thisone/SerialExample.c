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

/****************************************************************************************
 ***** 					TO DO														*****
 ***** Just realized that a blocked i2c routine stops the whole scene.  Must		*****
 ***** make that a semaphore to another routine, and watch to see that it isn't		*****
 ***** taking too long, or kill it and kick an error.								*****
 ****************************************************************************************/



#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avrx-signal.h>
#include <stdio.h>
#include "avrx.h"
#include <avr/signal.h>	// include "signal" names (interrupt names)
#include <avr/interrupt.h>	// include interrupt support
#include <stdlib.h>


#include "servo.h"
#include "a2d.h"		// include A/D converter function library
#include "timer.h"		// include timer function library (timing, PWM, etc)
#include "i2c.h"		// include i2c support
#include "i2c.c"		// should be fixed in make?

// Uncomment this to override "AvrXSerialIo.h and just use one channel
#define USART_CHANNELS (1<1)	// 0 - USART0, 1 = USART1

#include "AvrXSerialIo.h"

#include "parserconf.h"
#include "parser.h"
#include "parser.c"		//should be fixed in make?

//I2C address definitions
#define LOCAL_ADDR	0xA0
#define TARGET_ADDR	0xA0


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
#define LEFT_SERVO_CHAN 	4
#define RIGHT_SERVO_CHAN 	3
#define THROTTLE_SERVO_CHAN 0
#define CAM_PAN_SERVO_CHAN  1
#define CAM_TILT_SERVO_CHAN 2

#define DEBUG 0

void setLeftServo(void);
void setRightServo(void);
void setThrottleServo(void);
void setCamPanServo(void);
void setCamTiltServo(void);

void 	i2cSetup(void);
void  	i2cSlaveReceiveService(u08 receiveDataLength, u08* receiveData);
u08 	i2cSlaveTransmitService(u08 receiveDataLength, u08* receiveData);
void 	i2cMasterSendDiag(u08 deviceAddr, u08 length, u08* data);

void 	i2cMaster_Receive();
void 	i2cMaster_Send();
void 	i2cMaster_Auto_Send(u08 message);
void 	i2cMaster_Auto_Receive(u08 command);

// I2C buffers
u08 slaveBuffer[] = "Pascal is cool!!Pascal is Cool!!";
u08 slaveBufferLength = 0x20;

unsigned char masterBuffer[] = "This one is the Master board";
unsigned char masterBufferLength = 0x20;

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
AVRX_GCC_TASKDEF(getUAVStatus, 120, 4)
{	
	TimerControlBlock timer;
	int roll = 0;
	char updown = 1;
	char task_divisor = 0;
	while(1)
	{
		if(task_divisor>=10)
			task_divisor = 0;
//		printf_P(PSTR("1"));
		
		// info on all these protocols is in groundsoft/interfaces.txt
		AvrXDelay(&timer, 8000);				// these should execute at around 10hz
		i2cMaster_Auto_Send('a');
		AvrXDelay(&timer, 1000);
		i2cMaster_Auto_Receive('a');
		AvrXDelay(&timer, 1000);
		i2cMaster_Send('o');
		AvrXDelay(&timer, 1000);
		i2cMaster_Auto_Receive('o');
		/*
		printf_P(PSTR("q %d,"), 23); 
		printf_P(PSTR("w %d,"), 5);
		printf_P(PSTR("h %d,"), 80);
		if(updown==1)
		{
			roll++;
			if(roll >= 90)
				updown = 0;
		}
		else
		{
			roll--;
			if(roll<= -90)
				updown = 1;
		}
		printf_P(PSTR("w %d,"), roll);
		putchar('\r');
		putchar('\n');	
		
		if(task_divisor == 0)
		{
			printf_P(PSTR("c %d,"), 120);   	// c/3 evaluates to an int, even it c is not a multiple of
			printf_P(PSTR("a 072.5759E,")); 
			printf_P(PSTR("o 043.3223n,")); 
			printf_P(PSTR("s %d,"), 3);  
			putchar('\r');
			putchar('\n');
		}
      	task_divisor++;
		*/

	}
}







AVRX_GCC_TASKDEF(getCommands, 200, 1)
{	
	int c;		
	TimerControlBlock timer;
	
	while (1)
	{
		while ((c = getchar()) != EOF)
		{	
			parserInputFunc(c);
		}
		AvrXDelay(&timer, 5);
	}
}
/*

AVRX_GCC_TASKDEF(init, 50, 3)
{	
	int c;		
	TimerControlBlock timer;

}
*/	


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
    fdevopen(put_c0, get_c0);		// Set up standard I/O

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
	
	parserAddCommand("2", 		i2cMaster_Send);
	parserAddCommand("3", 		i2cMaster_Receive);
	
	
	
	// initialize the timer system -- FROM AVRLIB
	timerInit();
	
	
	//////////////////////////////////////////////////I2C/////////////////////////////
	i2cSetup();
	
	
	//////////////////////////////////////////////////Servos//////////////////////////
	servoInit();
	// setup servo output channel-to-I/Opin mapping
	// format is servoSetChannelIO( CHANNEL#, PORT, PIN );
	servoSetChannelIO(0, _SFR_IO_ADDR(PORTC), PC2);
	servoSetChannelIO(1, _SFR_IO_ADDR(PORTC), PC3);
	servoSetChannelIO(2, _SFR_IO_ADDR(PORTC), PC4);
	servoSetChannelIO(3, _SFR_IO_ADDR(PORTC), PC5);
	servoSetChannelIO(4, _SFR_IO_ADDR(PORTC), PC6);
	servoSetChannelIO(5, _SFR_IO_ADDR(PORTC), PC7);
	// set port pins to output
	outb(DDRC, 0xFC);
	

	#define SPEED_SERVO	1

	//////////////////////////////////////////////////////////////////////////////////
	printf("**********************powerup*************************");

	AvrXRunTask(TCB(getCommands));
	AvrXRunTask(TCB(getUAVStatus));
	//AvrXRunTask(TCB(init));
	
	Epilog();
	return(0);
}
void i2cSetup(void)
{
	// initialize i2c function library
	i2cInit();
	// set local device address and allow response to general call
	i2cSetLocalDeviceAddr(LOCAL_ADDR, TRUE);
	// set the Slave Receive Handler function
	// (this function will run whenever a master somewhere else on the bus
	//  writes data to us as a slave)
	i2cSetSlaveReceiveHandler( i2cSlaveReceiveService );
	// set the Slave Transmit Handler function
	// (this function will run whenever a master somewhere else on the bus
	//  attempts to read data from us as a slave)
	i2cSetSlaveTransmitHandler( i2cSlaveTransmitService );
}
// slave operations
void i2cSlaveReceiveService(u08 receiveDataLength, u08* receiveData)
{
	u08 i;

	// this function will run when a master somewhere else on the bus
	// addresses us and wishes to write data to us

	// copy the received data to a local buffer
	for(i=0; i<receiveDataLength; i++)
	{
		slaveBuffer[i] = *receiveData++;
	}
	slaveBufferLength = receiveDataLength;
	// print the buffer
//	for(i=0;i<slaveBufferLength;i++)
//	{
//		putchar(slaveBuffer[i]);
//	}
}

u08 i2cSlaveTransmitService(u08 transmitDataLengthMax, u08* transmitData)
{
	u08 i;

	// this function will run when a master somewhere else on the bus
	// addresses us and wishes to read data from us

	cbi(PORTB, PB7);

	// copy the local buffer to the transmit buffer
	for(i=0; i<slaveBufferLength; i++)
	{
		*transmitData++ = slaveBuffer[i];
	}

	slaveBuffer[0]++;

	return slaveBufferLength;
}

void i2cMasterSendDiag(u08 deviceAddr, u08 length, u08* data)
{
	// this function is equivalent to the i2cMasterSendNI() in the I2C library
	// except it will print information about transmission progress to the terminal

	// disable TWI interrupt
	cbi(TWCR, TWIE);

	// send start condition
	i2cSendStart();
	i2cWaitForComplete();
	//printf_P(PSTR(("STA-")));

	// send device address with write
	i2cSendByte( deviceAddr&0xFE );
	i2cWaitForComplete();
	//printf_P(PSTR(("SLA+W-")));
	
	// send data
	while(length)
	{
		i2cSendByte( *data++ );
		i2cWaitForComplete();
		//printf_P(PSTR(("DATA-")));
		length--;
	}
	
	// transmit stop condition
	// leave with TWEA on for slave receiving
	i2cSendStop();
	while( !(inb(TWCR) & BV(TWSTO)) );
	//printf_P(PSTR(("STO")));

	// enable TWI interrupt
	sbi(TWCR, TWIE);
}

void i2cMaster_Send(void)
{
	u08 sendData[2];
	sendData[0] = *parserGetArgStr();
	sendData[1] = '\0';
	i2cMasterSend(TARGET_ADDR, 0x02, sendData);
//	putchar(sendData[0]);
}
void i2cMaster_Auto_Send(u08 message)
{
	u08 sendData[2];
	sendData[0] = message;
	sendData[1] = '\0';
	i2cMasterSend(TARGET_ADDR, 0x02, sendData);
}
void i2cMaster_Receive(void)
{	
	u08 command = *parserGetArgStr();
	i2cMasterReceive(TARGET_ADDR, slaveBufferLength, slaveBuffer);
	slaveBuffer[0x19] = 0;
	printf("%c %s,",command,slaveBuffer); 
}
void i2cMaster_Auto_Receive(u08 command)
{
	i2cMasterReceive(TARGET_ADDR, slaveBufferLength, slaveBuffer);
	slaveBuffer[0x19] = 0;
	printf("%c %s,",command,slaveBuffer); 
}
void setLeftServo(void)
{	
	leftServoPos = parserGetArgInt();
	servoSetPosition(LEFT_SERVO_CHAN, (char)leftServoPos * 2);
	if (DEBUG)
	{	
		printf("e0");
		putchar('\r');
		putchar('\n');
	}
}

void setRightServo(void)
{	
	rightServoPos = parserGetArgInt();
	servoSetPosition(RIGHT_SERVO_CHAN, (char)rightServoPos * 2);
	if (DEBUG)
	{	
		printf("e0");
		putchar('\r');
		putchar('\n');
	}
}

void setThrottleServo(void)
{
	throttleServoPos = parserGetArgInt();
	servoSetPosition(THROTTLE_SERVO_CHAN, (char)throttleServoPos * 2);
	if (DEBUG)
	{	
		printf("e0");
		putchar('\r');
		putchar('\n');
	}
}

void setCamPanServo(void)
{
	int rawPan;
	
	rawPan = parserGetArgInt();
	
	if( (0<= rawPan) && (rawPan <= 2) )
	{
		camPanServoPos = camPanServoPos + (rawPan - 1);
		if(camPanServoPos < 1) camPanServoPos = 1;
		if(camPanServoPos > 255) camPanServoPos = 255;
	
		servoSetPosition(CAM_PAN_SERVO_CHAN, (char)camPanServoPos);
		if (DEBUG)
		{	
			printf("e0");
			putchar('\r');
			putchar('\n');
		}
	}
}

void setCamTiltServo(void)
{
	int rawTilt;
	
	rawTilt = parserGetArgInt();
	if(( 0 <= rawTilt) && (rawTilt <= 2) )
	{
		camTiltServoPos = camTiltServoPos + (rawTilt - 1);
		if(camTiltServoPos < 1) camTiltServoPos = 1;
		if(camTiltServoPos > 255) camTiltServoPos = 255;
	
		servoSetPosition(CAM_TILT_SERVO_CHAN, (char)camTiltServoPos);
		if (DEBUG)
		{	
			printf("Camera Tilt Servo Set: %d", camTiltServoPos);
			putchar('\r');
			putchar('\n');
		}
	}
}


