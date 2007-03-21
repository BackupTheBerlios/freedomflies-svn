#include <avr/io.h>		// include I/O definitions (port names, pin names, etc)
#include <avr/signal.h>	// include "signal" names (interrupt names)
#include <avr/interrupt.h>	// include interrupt support
#include "uart.h"		// include uart function library
#include "rprintf.h"	// include printf function library
#include "timer.h"		// include timer function library
#include "global.h"		// include our global settings
#include "string.h"		// use for strlen()
#include "video.h"  //user video thingy


#define CLEARSCREEN "A"
#define RESET "U"				//Reset registers to default
#define CHAR_BG_DISPLAY "B"
#define TOGGLE_BLINK "T"
#define GET_CHAR "E"
#define ASCII_MODE "K"
#define pX_AXIS "X"			//Xnn/r 00..27
#define pY_AXIS "Y"			//Ynn/r 00..10
#define pTYPE	"T"			//T/r[text]/r types until next return, then re-enters command mode
#define pCHAR_COLOR "ZE"		//ZEnn/r 00=Black 01=Blue 02=Green 03=Cyan 04=Red 05=Magenta 06=Red 07=White


#define pBAUD "W"				//Baud Rate 00=2400 [<-DEFAULT] 01=4800 02=9600 03=14200 04=38400

#define FULL_PAGE_MODE "N"
#define MIXED_MODE "M"

#define ASCII_MODE "K"
#define NUMERIC_MODE "J"

#define BG_ON "ZJ"
#define BG_OFF "ZH"
#define BLINK_ON "ZJ"
#define BLINK_OFF "ZH"

#define pFONT_SIZE	"V"		//Size1		Size2		Size3		Size4  Each row may be set indep of each other!
						//row0	00		01			02			03
						//row1-9 04		05			06			07
						//row10	08		09			10			11
						
////////////////////////////////////Fields for STVSER//////////////////////////////////////////
#define LAT_FIELD_X 0
#define LAT_FIELD_Y 1
#define LONG_FIELD_X 17
#define LONG_FIELD_Y 1
#define TIME_FIELD_X 10
#define TIME_FIELD_Y 1
#define ALTITUDE_FIELD_X 14
#define ALTITUDE_FIELD_Y 1
#define COMPASS_FIELD_X 15
#define COMPASS_FIELD_Y 10
#define ERROR_FIELD_X 26
#define ERROR_FIELD_Y 10


void initVideo(void)
{
	rprintf("%s\r",RESET);
	rprintf("%s\r",CLEARSCREEN);
	rprintf("%s\r",ASCII_MODE);
	rprintf("%s\r",MIXED_MODE);
	rprintf("%s07\r",pCHAR_COLOR); //set char color to white
	rprintf("%s01\r",pBAUD); //set baud at 4800 to work with gps unit
}


void writeVideoScreen(float LAT, float LONG, float TIME, float ALTITUDE, float COMPASS, char ERROR)
{
	rprintf("X%d\rY%d\rS%f\r",LAT_FIELD_X,LAT_FIELD_Y,LAT);
	rprintf("X%d\rY%d\rS%f\r",LONG_FIELD_X,LONG_FIELD_X,LONG);
	rprintf("X%d\rY%d\rS%f\r",TIME_FIELD_X,TIME_FIELD_Y,TIME);
	rprintf("X%d\rY%d\rS%f\r",TIME_FIELD_X,TIME_FIELD_Y,ALTITUDE);
	rprintf("X%d\rY%d\rS%f\r",COMPASS_FIELD_X,COMPASS_FIELD_X,COMPASS);
	rprintf("X%d\rY%d\rS%f\r",ERROR_FIELD_X,ERROR_FIELD_X,ERROR);
}
