//Integration test of GPs nmea parse code with No.6
//Last Modified 19Sep06 jlev
 
//----- Include Files ---------------------------------------------------------
#include <avr/io.h>		// include I/O definitions (port names, pin names, etc)
#include <avr/signal.h>	// include "signal" names (interrupt names)
#include <avr/interrupt.h>	// include interrupt support
#include "uart.h"		// include uart function library
#include "rprintf.h"	// include printf function library
#include "timer.h"		// include timer function library
#include "global.h"		// include our global settings
#include "string.h"		// use for strlen()
#include "video.h"  //user video thingy


//our nmea includes
#include "nmeap.h"
#include "nmeap_def.h"
#include "nmeap01.c"
#include "gpsmath.c"

//software uart includes
#include "uartswconf.h"
#include "uartsw.h"
#include "uartsw.c"

//i2c includes
#include "i2c.h"		// include i2c support
#define LOCAL_ADDR		0xA0
//#define RPRINTF_FLOAT	TRUE	//This allows rprintf to print floats -- useful for lats/longs

//#define USE_VID_CARD
#define USE_VOICE_BOX
unsigned char voiceBoxCue;

unsigned char localBuffer[] = "This should be filling with data!";
unsigned char localBufferLength = 0x20;

static nmeap_context_t nmea;	//parser context
static int user_data;			//not sure what this is for...
static nmeap_gga_t     gga;     //data buffer for GGA sentences
static nmeap_rmc_t     rmc;     //data buffer for RMC sentences

static cBuffer gpsBuffer; //serial receive buffer
static char gpsBufferData[UART_RX_BUFFER_SIZE]; //allocate space in ram for buffer
void addToBuffer(unsigned char ch); //the interrupt handler

//i2c routines
void i2cSlaveReceiveService(u08 receiveDataLength, u08* receiveData);
u08 i2cSlaveTransmitService(u08 transmitDataLengthMax, u08* transmitData);
u08 message_sent;
void initVoiceBox(void);

//printing to array routines.  these are used to print floats to the i2c, since sprintf doesn't work properly with %f.  
//instead, we set rprintfinit(print_to_string), then at the end have a string that we send to the i2c.  
#define stackem_length 0x20
u08 stackem[stackem_length];
u08 stackem_pointer = 0;
void print_to_string(u08 the_char);
void sendTextMessage(u08);

//For voice box
#define CTRLA = 0x01

//----- Begin Code ------------------------------------------------------------
int main(void)
{
	int status;
    // Test of csik's gps math code
    struct waypoint from,to;
    //from.lat = 50.0912;
    //from.lon = -001.50;
    // set when we get valid GPS
	to.lat = 42.359051;
    to.lon = -71.093355;
	// 77 Mass Ave, taken off Google Maps
    
	//initialize i2c message_sent to 0 to see if we're getting messages...
	message_sent = 0;
 
   
    //set uart receive rate to output from GPS

	// initialize our libraries
	// initialize the UART (serial port)
	uartInit();
	timerInit();
 	timerPause(500);


#ifdef USE_VOICE_BOX
	////PRE_INIT FOR THE V8699A VOICE SYNTH//////////////////////////////
	uartSetBaudRate(4800);
	// make all rprintf statements use uart for output
	rprintfInit(uartSendByte);
	initVoiceBox();
	voiceBoxCue = 'x';
#endif /* USE_VOICE_BOX */

////// initialize the timer system


#ifdef USE_VIDEO_CARD
	////PRE_INIT FOR THE VIDEO CARD//////////////////////////////
	uartSetBaudRate(2400);
	// make all rprintf statements use uart for output
	rprintfInit(uartSendByte);
	initVideo();
	//////////////////////////////////////////////////////////////
	uartSetBaudRate(4800);
	// make all rprintf statements use uart for output
	rprintfInit(uartSendByte);
	// initialize the timer syste
#endif /* USE_VIDEO_CARD */
	
	// print a little intro message so we know things are working
#ifndef USE_VID_CARD 
	// print an intro message
	rprintf("\r\nWelcome to GPS subsystem!\r\n");
#endif /* USE_VID_CARD */



////// initialize software uart
	uartswInit();
	uartswSetBaudRate(19200);

////// initialize i2c///////////////////////////////////////////////////////////////////
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
	
	
   	//initialize the gps receive buffer
   	bufferInit(&gpsBuffer,gpsBufferData,UART_RX_BUFFER_SIZE);

	uartSetRxHandler(addToBuffer); //custom handler	
	
	//rprintf("set handler\r\n");
	
	//initialize gps library
	status = nmeap_init(&nmea,&user_data);
	if (status != 0) {
        rprintf("nmeap_init %d\n",status);
    }
   	// initialize gps parsers 
	status = nmeap_addParser(&nmea,"GPGGA",nmeap_gpgga,0,&gga);
	if (status != 0) {
        rprintf("nmeap_add GGA %d\n",status);
    }
    status = nmeap_addParser(&nmea,"GPRMC",nmeap_gprmc,0,&rmc);
    if (status != 0) {
        rprintf("nmeap_add RMC %d\n",status);
    }
	// WHAT IS THIS?????
	//rprintf("gga.latitude = ");
	//rprintfFloat(8,gga.latitude);
	//rprintf("\r\n");
	
	for(;;) {
		//set up variables for access by spi routine
		float latitude, longitude, altitude, speed;
		//make sure we have data
		char ch = bufferGetFromFront(&gpsBuffer);
		if(ch != 0) {
				//rprintf("%c",ch); //debug
				status = nmeap_parse(&nmea,ch);
				switch(status) {
					case NMEAP_GPGGA:
#ifdef USE_VID_CARD
						writeVideoScreen(gga.latitude, gga.longitude, gga.time, gga.altitude, rmc.course, rmc.speed, 'n');
#endif /* end USE_VID_CARD*/
#ifdef USE_TERMINAL
						rprintf("GGA");
						rprintf("\tLat: ");
						rprintfFloat(8,gga.latitude);
						rprintf(" Lon: ");
						rprintfFloat(8,gga.longitude);
						rprintf(" Alt: ");
						rprintfFloat(8,gga.altitude);
						rprintf("\r\n");
						
						//print distance to 77 Mass Ave
						from.lat = gga.latitude;
						from.lon = gga.longitude;
						rprintf("Dist to 77 (km): ");
						rprintfFloat(8,dist(from,to));
						rprintf("\r\n");
						rprintf("Bearing to 77 (deg): ");
						rprintfFloat(8,bearing(from,to));
						rprintf("\r\n\r\n");
#endif /* end USE_TERMINAL */
						break;
					case NMEAP_GPRMC:
#ifdef USE_VID_CARD
						writeVideoScreen(gga.latitude, gga.longitude, gga.time, gga.altitude, rmc.course, rmc.speed, 'n');
#endif /* USE_VID_CARD */

#ifdef USE_TERMINAL
						rprintf("RMC");
						//rprintf("\tLat: ");
						//rprintfFloat(8,rmc.latitude);
						//rprintf(" Lon: ");
						//rprintfFloat(8,rmc.longitude);
						rprintf(" Vel (kt): ");
						rprintfFloat(8,rmc.speed);
						rprintf(" Course over ground (deg): ");
						rprintfFloat(8,rmc.course);
						rprintf("\r\n");
#endif /* end using terminal */
						break;
					default:
						break;
			        } //end switch	
			
			}//	end if(ch != 0)
			
			//  here one would write to the video and the i2c buffer... set a mutex?  in other words, we want to write new values to this area but not allow the i2c to access it in the middle of a write.
			// use two data objects, write back and forth and switch in a single operation?
			// write a make a readable write b make b readable
			// writeVideoScreen(gga.latitude, gga.longitude, gga.altitude, speed, char ERROR)
			
			// OR, should the master give a character and then receive a value?
			// that would seem to make more sense, but it means sort of immediate updates...
			//rprintf("%c\r\n",voiceBoxCue);
			//This should cue for the voice box...
			if(voiceBoxCue != 'x')
			{
				sendTextMessage(voiceBoxCue);
			}

			if(message_sent != 0)
			{
				message_sent = 1;
				rprintf("\r\nsent i2c!");
			}
	} //end for
	
	return 0;
}
/*
#define  h "Restore Habeas Corpus Now!"
#define  m "Close Guantanamo Now!  It is illegal, immoral, and hurting our national security!"
#define  n "What do we want?  Heybeas corpuss!  When do we want it?  We already have the right, but the right is being illegaly violated!"
#define  p "We come in peace!  We mean you no harm."
*/
void sendTextMessage(unsigned char theMessage)
{	
	switch(theMessage)
	{
		case 'h':
			rprintf("\r\n Restore heybeas corpuss! Stop the torture!\r\n ");
			break;
		case 'm':
			rprintf("\r\n Close Guantanamo Now!  It is illegal, immoral, and hurting our national security!\r\n ");
			break;
		case 'n':
			rprintf("\r\nWhat do we want?  Heybeas corpuss!  When do we want it?  We already have the right, but the right is being illegaly violated!\r\n ");
			break;
		case 'p':
			rprintf("\r\n We come in peace!  We mean you no harm.\r\n ");
			break;
		case 22:
			rprintf("\r\n We come in peace!  We mean you no harm.\r\n ");
			break;
		default:
			break;
	}
	voiceBoxCue = 'x';
}


void initVoiceBox(void){
	rprintfCRLF();
	rprintfu08(1);//rprintfu08(small);((u08) 1);  //ctrl A = ascii code 1 -- sets voicebox to accept a command
	rprintf("4O\r\n"); //4o is the voice to use -- richochet randy!
	rprintfu08(1); //rprintfChar((u08) 1);  //ctrl A = ascii code 1 -- sets voicebox to accept a command
	rprintf("2P\r\n"); //4o is the voice to use -- richochet randy!
	
	rprintf("Close Guantanamo now!\r\n");
	rprintfCRLF();
	rprintf("%c4O\r\n",0x01);
	rprintf("%c2P\r\n",0x01);
	rprintf("Restore heybeas corpuss\r\n");
}

void addToBuffer(unsigned char ch) {
	bufferAddToEnd(&gpsBuffer,ch);
}

// slave operations
void i2cSlaveReceiveService(u08 receiveDataLength, u08* receiveData)
{
	u08 i;

	// this function will run when a master somewhere else on the bus
	// addresses us and wishes to write data to us

	//showByte(*receiveData);
	cbi(PORTB, PB6);

	// copy the received data to a local buffer
	for(i=0; i<receiveDataLength; i++)
	{
		localBuffer[i] = *receiveData++;
	}
	localBufferLength = receiveDataLength;
	
}

/*
#define	 q "pitch"
#define	 w "Roll"
#define	 c "Heading"
#define	 a "Latitude"
#define	 o "Longitude"
#define	 z "Altitude"
#define	 b "Battery"
#define	 f "Fuel"
#define	 s "Airspeed"
#define	 g "Groundspeed"
#define  h "Restore Habeas Corpus Now!"
#define  m "Close Guantanamo Now!  It is illegal, immoral, and hurting our national security!"
#define  n "What do we want?  Heybeas corpuss!  When do we want it?  We already have the right, but the right is being illegaly violated!"
#define  p "We come in peace!  We mean you no harm."
*/

u08 i2cSlaveTransmitService(u08 transmitDataLengthMax, u08* transmitData)
{
	u08 i;
	// this function will run when a master somewhere else on the bus
	// addresses us and wishes to read data from us
	
	// I believe that this might be the place to load the response...
	
	//showByte(*transmitData);	
	cbi(PORTB, PB7);
	rprintfInit(print_to_string);
	// temp for debugging.... CHANGE!
	float test = -71.093355;
	double test2 = 42.359051;
	stackem_pointer = 0;
	switch(localBuffer[0])
	{
		case 'a': //latitude
			rprintf("a ");
			rprintfFloat(8,gga.latitude);
			//rprintfFloat(8,test2);	
			break;
		case 'o': //longitude
			rprintf("o ");
			rprintfFloat(8,gga.longitude);
			//rprintfFloat(8,test);	
			break;
		case 'g': //groundspeed
			rprintf("g ");
			rprintfFloat(8,rmc.speed);
			break;
//		case 'c':
//			rprintf("c ");
//			rprintfFloat(8,rmc.course);
//			break;
		case 'c':  // RETURNS compass, roll, and pitch
			rprintf("c"); //ghetto -- no space before the data for some annoying reason -- blame uartsw.c
			sentence[0] = ' ';
			rprintfStr(sentence);
			break;
		case 'd':
			rprintf("d ");
			rprintfFloat(9,rmc.date);
			break;
		case 's':
			rprintf("s ");
			rprintfFloat(7,rmc.speed);
			break;
		case 'h':
			voiceBoxCue = 'h';
			rprintf("h ");
			break;
		case 'n':
			voiceBoxCue = 'n';
			rprintf("n ");
			break;
		case 'm':
			voiceBoxCue = 'm';
			rprintf("m ");
			break;
		case 'p':
			voiceBoxCue = 22;
			rprintf("p ");
			break;
		default:
		//	rprintf(" E");  //CE = Command Error
			break;
	}
	rprintfInit(uartSendByte);
	//sprintf(temp_buffer, "%f,%f",gga.latitude,gga.longitude);
	
	// copy the local buffer to the transmit buffer -- this buffer size is set in i2c.conf
	for(i=0; i<=stackem_pointer; i++)
	{
		*transmitData++ = stackem[i];
	}
	
	
	//localBuffer[0]++;
	//message_sent = 1;

	return stackem_pointer+1;
}
void print_to_string(u08 the_char)
{
	if(stackem_pointer<(stackem_length-1))  //-1 to allow for null char to still fit
	{
		stackem[stackem_pointer] = the_char;
		stackem_pointer++;
		stackem[stackem_pointer] = 0;
	}
}

void getCompass(void)
{
	u08 data;
	
	while(1)
	{
		if(uartswReceiveByte(&data))
			rprintfChar(data);
	}
}

void setCompassBaud(void)
{
	sentence[0]=' ';
	//uartswSetBaudRate(cmdlineGetArgInt(1));
	rprintfStr(sentence);
	rprintf("\r\n");
	
}

