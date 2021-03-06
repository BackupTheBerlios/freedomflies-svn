//*****************************************************************************
// File Name	: cmdlinetest.c
// 
// Title		: example usage of cmdline (command line) functions
// Revision		: 1.0
// Notes		:	
// Target MCU	: Atmel AVR series
// Editor Tabs	: 4
// 
// Revision History:
// When			Who			Description of change
// -----------	-----------	-----------------------
// 21-Jul-2003	pstang		Created the program
//*****************************************************************************


//	TODO:
//		Rationalize master/slave relationship.  Reduce number of calls.  Ask for SPI dump rather than individual data,
//		even if master only returns them one by one
//
//		
//
//

//----- Include Files ---------------------------------------------------------
#include <avr/io.h>			// include I/O definitions (port names, pin names, etc)
#include <avr/interrupt.h>	// include interrupt support

#include "global.h"		// include our global settings
#include "uart.h"		// include uart function library
#include "rprintf.h"	// include printf function library
#include "a2d.h"		// include A/D converter function library
#include "timer.h"		// include timer function library (timing, PWM, etc)
#include "vt100.h"		// include vt100 terminal support
#include "cmdline.h"	// include cmdline function library
#include "servo.h"
#include "i2c.h"
#include "string.h"
#include "servoconf.h"
//#include <pulseb.h>
//#include <pulseb.c>

//I2C address definitions
#define LOCAL_ADDR	0xA0
#define TARGET_ADDR	0xA0

// Peripheral initialization

#define TCNT0_INIT (0xFF-CPUCLK/256/TICKRATE)
#define LEFT_SERVO_CHAN 	4
#define RIGHT_SERVO_CHAN 	3
#define THROTTLE_SERVO_CHAN 0
#define CAM_PAN_SERVO_CHAN  1
#define CAM_TILT_SERVO_CHAN 2

#define SPEED_SERVO	1
#undef DEBUG

// Pulse defines
#define MULTIPLIER 100

void setLeftServo(void);
void setRightServo(void);
void setThrottleServo(void);
void setCamPanServo(void);
void setCamTiltServo(void);
void setFeedbackInterval(void);


void 	i2cSetup(void);
void  	i2cSlaveReceiveService(u08 receiveDataLength, u08* receiveData);
u08 	i2cSlaveTransmitService(u08 receiveDataLength, u08* receiveData);
void 	i2cMasterSendDiag(u08 deviceAddr, u08 length, u08* data);

void 	i2cMaster_Receive();
void 	i2cMaster_Send();
void 	i2cMaster_Auto_Send(u08 message);
void 	i2cMaster_Auto_Receive(u08 command);

// A2D routine
void getA2D(void);

// I2C buffers
u08 slaveBuffer[] = "Restore Habeas Corp!";
u08 slaveBufferLength = 0x20;

unsigned char masterBuffer[] = "This one is the Master board";
unsigned char masterBufferLength = 0x20;
// global variables
u08 Run;

// functions
void goCmdline(void);
void dumpArgsStr(void);
void dumpArgsInt(void);
void dumpArgsHex(void);

u08 leftServoPos = 50 * MULTIPLIER;		//0 seems to be beyond its reach
//below for boat only
u16	multiplier = MULTIPLIER;

u08 rightServoPos;
u08 throttleServoPos;
u08 camPanServoPos;
u08 camTiltServoPos;

long numTimesThrough = 0;  // number of times through mainloop to send feedback

//----- Begin Code ------------------------------------------------------------
int main(void)
{
	// initialize our libraries
	// initialize the UART (serial port)
	uartInit();
	uartSetBaudRate(57600);
	// make all rprintf statements use uart for output
	rprintfInit(uartSendByte);
	// turn on and initialize A/D converter
	//////////////////////////////////////////////////A2D///////////////////////////
	a2dInit();
	// configure a2d port (PORTA) as input
	// so we can receive analog signals
	DDRA = 0x00;
	// make sure pull-up resistors are turned off
	PORTA = 0x00;

	// set the a2d prescaler (clock division ratio)
	// - a lower prescale setting will make the a2d converter go faster
	// - a higher setting will make it go slower but the measurements
	//   will be more accurate
	// - other allowed prescale values can be found in a2d.h
	a2dSetPrescaler(ADC_PRESCALE_DIV32);

	// set the a2d reference
	// - the reference is the voltage against which a2d measurements are made
	// - other allowed reference values can be found in a2d.h
	a2dSetReference(ADC_REFERENCE_AVCC);

	// use a2dConvert8bit(channel#) to get an 8bit a2d reading
	// use a2dConvert10bit(channel#) to get a 10bit a2d reading
	
	//////////////////////////////////////////////////TIMER////////////////////////////////
	// initialize the timer system
	timerInit();
	//////////////////////////////////////////////////TERMINAL/////////////////////////////
	
	// initialize vt100 terminal
	vt100Init();

	// wait for hardware to power up
	timerPause(100);

	//////////////////////////////////////////////////I2C/////////////////////////////
	i2cSetup();
	//Setup I2c to speak to other boards.  Currently, other board is the gps/text board
	
	//////////////////////////////////////////////////Servos//////////////////////////
	servoInit();
	// setup servo output channel-to-I/Opin mapping
	// format is servoSetChannelIO( CHANNEL#, PORT, PIN );
	servoSetChannelIO(0, _SFR_IO_ADDR(PORTC), PC2);
	servoSetChannelIO(1, _SFR_IO_ADDR(PORTC), PC3);
	servoSetChannelIO(2, _SFR_IO_ADDR(PORTC), PC4);
	servoSetChannelIO(3, _SFR_IO_ADDR(PORTC), PC5);
	servoSetChannelIO(4, _SFR_IO_ADDR(PORTC), PC6);
	// set port pins to output
	outb(DDRC, 0xFC);
	



	//////////////////////////////////////////////////////////////////////////////////
	rprintf("**********************powerup*************************");
	

	// start command line
	goCmdline();

	return 0;
}

void goCmdline(void)
{
	u08 c;
	long timesThrough =0;
	// print welcome message
	vt100ClearScreen();
	vt100SetCursorPos(1,0);
	rprintfProgStrM("\r\nWelcome to the Command Line Test Suite!\r\n");

	// initialize cmdline system
	cmdlineInit();

	// direct cmdline output to uart (serial port)
	cmdlineSetOutputFunc(uartSendByte);

	// add commands to the command database


	cmdlineAddCommand("l",		setLeftServo);
	cmdlineAddCommand("r",		setRightServo);
    cmdlineAddCommand("t", 		setThrottleServo);
	cmdlineAddCommand("p", 		setCamPanServo);
	cmdlineAddCommand("i", 		setCamTiltServo);
	cmdlineAddCommand("f", 		setFeedbackInterval);
	cmdlineAddCommand("b",		getA2D);  //returns packed values -- see code
	cmdlineAddCommand("2", 		i2cMaster_Send);
	cmdlineAddCommand("3", 		i2cMaster_Receive);
	cmdlineAddCommand("das",	dumpArgsStr);
	cmdlineAddCommand("dai",	dumpArgsInt);
	// send a CR to cmdline input to stimulate a prompt
	cmdlineInputFunc('\r');

	// set state to run
	Run = TRUE;

	// main loop
	while(Run)
	{
		// pass characters received on the uart (serial port)
		// into the cmdline processor
		while(uartReceiveByte(&c)) cmdlineInputFunc(c);

		// run the cmdline execution functions
		cmdlineMainLoop();
		timesThrough++;
		if(numTimesThrough != 0)
		{
			if((timesThrough >= numTimesThrough)) 
			{
				rprintf("times through!\r\n");
				timesThrough = 0;
			}
		}
	}

	rprintfCRLF();
	rprintf("Exited program!\r\n");
}

void exitFunction(void)
{
	// to exit, we set Run to FALSE
	Run = FALSE;
}

void helpFunction(void)
{
	rprintfCRLF();

	rprintf("Available commands are:\r\n");
	rprintf("help      - displays available commands\r\n");
	rprintf("dumpargs1 - dumps command arguments as strings\r\n");
	rprintf("dumpargs2 - dumps command arguments as decimal integers\r\n");
	rprintf("dumpargs3 - dumps command arguments as hex integers\r\n");

	rprintfCRLF();
}

void dumpArgsStr(void)
{
	rprintfCRLF();
	rprintf("Dump arguments as strings\r\n");

	rprintfProgStrM("Arg0: "); rprintfStr(cmdlineGetArgStr(0)); rprintfCRLF();
	rprintfProgStrM("Arg1: "); rprintfStr(cmdlineGetArgStr(1)); rprintfCRLF();
	rprintfProgStrM("Arg2: "); rprintfStr(cmdlineGetArgStr(2)); rprintfCRLF();
	rprintfProgStrM("Arg3: "); rprintfStr(cmdlineGetArgStr(3)); rprintfCRLF();
	rprintfCRLF();
}

void dumpArgsInt(void)
{
	rprintfCRLF();
	rprintf("Dump arguments as integers\r\n");

	// printf %d will work but only if your numbers are less than 16-bit values
	//rprintf("Arg1 as int: %d\r\n", cmdlineGetArgInt(1));
	//rprintf("Arg2 as int: %d\r\n", cmdlineGetArgInt(2));
	//rprintf("Arg3 as int: %d\r\n", cmdlineGetArgInt(3));

	// printfNum is good for longs too
	rprintf("Arg1 as int: "); rprintfNum(10, 10, TRUE, ' ', cmdlineGetArgInt(1)); rprintfCRLF();
	rprintf("Arg2 as int: "); rprintfNum(10, 10, TRUE, ' ', cmdlineGetArgInt(2)); rprintfCRLF();
	rprintf("Arg3 as int: "); rprintfNum(10, 10, TRUE, ' ', cmdlineGetArgInt(3)); rprintfCRLF();
	rprintfCRLF();
}

void dumpArgsHex(void)
{
	rprintfCRLF();
	rprintf("Dump arguments as hex integers\r\n");

	rprintf("Arg1 as hex: "); rprintfNum(16, 8, FALSE, ' ', cmdlineGetArgHex(1)); rprintfCRLF();
	rprintf("Arg2 as hex: "); rprintfNum(16, 8, FALSE, ' ', cmdlineGetArgHex(2)); rprintfCRLF();
	rprintf("Arg3 as hex: "); rprintfNum(16, 8, FALSE, ' ', cmdlineGetArgHex(3)); rprintfCRLF();
	rprintfCRLF();
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
	//rprintf(("STA-")));

	// send device address with write
	i2cSendByte( deviceAddr&0xFE );
	i2cWaitForComplete();
	//rprintf(("SLA+W-")));
	
	// send data
	while(length)
	{
		i2cSendByte( *data++ );
		i2cWaitForComplete();
		//rprintf(("DATA-")));
		length--;
	}
	
	// transmit stop condition
	// leave with TWEA on for slave receiving
	i2cSendStop();
	while( !(inb(TWCR) & BV(TWSTO)) );
	//rprintf(("STO")));

	// enable TWI interrupt
	sbi(TWCR, TWIE);
}

void i2cMaster_Send(void)
{
	u08 sendData[20];
	strcpy(sendData, cmdlineGetArgStr(1));
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
	u08 command[20];
	strcpy(command, cmdlineGetArgStr(1));
	
	i2cMasterReceive(TARGET_ADDR, slaveBufferLength, slaveBuffer);
	slaveBuffer[0x19] = 0;
	//rprintfStr(command);
	//rprintf(" ");
	rprintfStr(slaveBuffer);
	rprintf(",");
	
}
void i2cMaster_Auto_Receive(u08 command)
{
	i2cMasterReceive(TARGET_ADDR, slaveBufferLength, slaveBuffer);
	
	slaveBuffer[0x19] = 0;
	//rprintfStr(command);
	//rprintf(" ");
	rprintfStr(slaveBuffer);
	rprintf(",");
}

	//BELOW is the original left servo code -- would be right for plane, not for boat

void setLeftServo(void)
{	
	leftServoPos = (unsigned char) cmdlineGetArgInt(1);
	servoSetPosition(LEFT_SERVO_CHAN, leftServoPos);
#ifdef DEBUG	
		rprintf("e0\r\n");
#endif DEBUG
}
	
void setRightServo(void)
{	
	rightServoPos = (unsigned char) cmdlineGetArgInt(1);
	servoSetPosition(RIGHT_SERVO_CHAN, rightServoPos);
#ifdef DEBUG
		rprintf("e0\r\n");
#endif DEBUG
}

void setThrottleServo(void)
{
	throttleServoPos = (unsigned char) cmdlineGetArgInt(1);
	servoSetPosition(THROTTLE_SERVO_CHAN, throttleServoPos);
#ifdef DEBUG
		rprintf("e0\r\n");
#endif DEBUG
}
void setFeedbackInterval(void)
{
	numTimesThrough = cmdlineGetArgInt(1);
}

void setCamPanServo(void)
{
	int rawPan;
	
	rawPan = cmdlineGetArgInt(1);
	
	if( (0<= rawPan) && (rawPan <= 2) )
	{
		camPanServoPos = camPanServoPos + (rawPan - 1);
		if(camPanServoPos < 1) camPanServoPos = 1;
		if(camPanServoPos > 255) camPanServoPos = 255;
	
		servoSetPosition(CAM_PAN_SERVO_CHAN, (char)camPanServoPos);
#ifdef DEBUG
			rprintf("e0\r\n");
#endif DEBUG

	}
}

void setCamTiltServo(void)
{
	int rawTilt;
	
	rawTilt = cmdlineGetArgInt(1);
	if(( 0 <= rawTilt) && (rawTilt <= 2) )
	{
		camTiltServoPos = camTiltServoPos + (rawTilt - 1);
		if(camTiltServoPos < 1) camTiltServoPos = 1;
		if(camTiltServoPos > 255) camTiltServoPos = 255;
	
		servoSetPosition(CAM_TILT_SERVO_CHAN, (char)camTiltServoPos);
#ifdef DEBUG	
			rprintf("e0\r\n");
#endif DEBUG

	}
}

//////////////////////////////////////////////////A2D ROUTINE/////////////////////////////
//	Returns part of a full byte with analog values for each pin.  Might be overkill, but will hopefully help
// 	battery life!
//	A0 = main battery
// 	A1 = secondary battery
//	A2 = temperature

void getA2D()
{
#define NUM_A2D_SENSORS 3
	u08 i;	
	rprintf("b ");

	for(i=0; i<NUM_A2D_SENSORS; i++)
	{
		rprintf("%s", i, a2dConvert8bit(i));
	}
	rprintf(",");
	
}