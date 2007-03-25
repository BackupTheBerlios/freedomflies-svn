/*
  	Non-multitasking version of flight control software -- currently not doing much of anything.

*/

/****************************************************************************************
 ***** 					TO DO														*****
 ***** Just realized that a blocked i2c routine stops the whole scene.  Must		*****
 ***** make that a semaphore to another routine, and watch to see that it isn't		*****
 ***** taking too long, or kill it and kick an error.								*****
 ****************************************************************************************/



#include <avr/io.h>		// include I/O definitions (port names, pin names, etc)
#include <avr/interrupt.h>	// include interrupt support

#include "global.h"
#include "uart.h"		// include uart function library
#include "rprintf.h"	// include printf function library
#include "timer.h"		// include timer function library (timing, PWM, etc)
#include "vt100.h"		// include VT100 terminal support


#include "servo.h"
#include "a2d.h"		// include A/D converter function library

#include "i2c.h"		// include i2c support
#include "i2c.c"		// should be fixed in make?

#include "string.h"

#include "parserconf.h"
#include "parser.h"
#include "parser.c"		//should be fixed in make?

#define BAUDRATE 57600



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

#undef DEBUG

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

char buffer[20]; //used for sprintfing

int leftServoPos = 50;		//0 seems to be beyond its reach
int rightServoPos;
int throttleServoPos;
int camPanServoPos;
int camTiltServoPos;
u16 baudrate = 57600;

int main(void)
{
 
	int c;		
	u16 loopthrough =0;
	// initialize the UART (serial port)
	uartInit();
	// set the baud rate of the UART for our debug/reporting output
	uartSetBaudRate(57600);
	// initialize the timer system
	timerInit();

	// initialize rprintf system
	// - use uartSendByte as the output for all rprintf statements
	//   this will cause all rprintf library functions to direct their
	//   output to the uart
	// - rprintf can be made to output to any device which takes characters.
	//   You must write a function which takes an unsigned char as an argument
	//   and then pass this to rprintfInit like this: rprintfInit(YOUR_FUNCTION);
	rprintfInit(uartSendByte);

	// initialize vt100 library
	vt100Init();
	
	// clear the terminal screen
	vt100ClearScreen();
	
	
	
	// initialize parser system
	parserInit();
	
	// send a CR to cmdline input to stimulate a prompt
	cmdlineInputFunc('\r');
	// direct output to uart (serial port)
	parserSetOutputFunc(uartSendByte);
	// add commands to the command database
	parserAddCommand("l",		setLeftServo);
	parserAddCommand("r",		setRightServo);
    parserAddCommand("t", 		setThrottleServo);
	parserAddCommand("p", 		setCamPanServo);
	parserAddCommand("i", 		setCamTiltServo);
	
	parserAddCommand("2", 		i2cMaster_Send);
	parserAddCommand("3", 		i2cMaster_Receive);
	
	
	

	
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
	rprintf("**********************powerup*************************");

	while (1)
	{
		while ((c = uartGetByte()) != -1)
		{	
			parserInputFunc(c);
		}
		loopthrough++;
		if(loopthrough >= 20000)
		{
			rprintf("testing...\r\n");
			loopthrough =0;
		}
	}
	
	
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
	rprintf("%c %s,",command,slaveBuffer); 
}
void i2cMaster_Auto_Receive(u08 command)
{
	i2cMasterReceive(TARGET_ADDR, slaveBufferLength, slaveBuffer);
	
	slaveBuffer[0x19] = 0;
	rprintf("%c %s,",command,slaveBuffer);
}
void setLeftServo(void)
{	
	leftServoPos = parserGetArgInt();
	servoSetPosition(LEFT_SERVO_CHAN, (char)leftServoPos * 2);
#ifdef DEBUG	
		rprintf("e0\r\n");
#endif DEBUG
}

void setRightServo(void)
{	
	rightServoPos = parserGetArgInt();
	servoSetPosition(RIGHT_SERVO_CHAN, (char)rightServoPos * 2);
#ifdef DEBUG
		rprintf("e0\r\n");
#endif DEBUG
}

void setThrottleServo(void)
{
	throttleServoPos = parserGetArgInt();
	servoSetPosition(THROTTLE_SERVO_CHAN, (char)throttleServoPos * 2);
#ifdef DEBUG
		rprintf("e0\r\n");
#endif DEBUG
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
#ifdef DEBUG
			rprintf("e0\r\n");
#endif DEBUG

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
#ifdef DEBUG	
			rprintf("e0\r\n");
#endif DEBUG

	}
}


