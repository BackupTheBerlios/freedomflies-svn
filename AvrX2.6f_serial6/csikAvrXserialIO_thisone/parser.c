#include <avr/io.h>			// include I/O definitions (port names, pin names, etc)
#include <avr/interrupt.h>	// include interrupt support
#include <avr/pgmspace.h>	// include AVR program memory support
#include <string.h>			// include standard C string functions
#include <stdlib.h>			// include stdlib for string conversion functions


#include "parser.h"
#include "parserconf.h"



// command list
char CommandList[MAX_COMMANDS][MAX_CMD_LENGTH];
// command function pointer list
ParserFuncPtrType ParserFunctionList[MAX_COMMANDS];
// number of commands currently registered
unsigned char parserNumCommands;
//current length of input buffer
unsigned char parserBufferLength;
//buffer into which commands are written as they come through
unsigned char parserBuffer[BUFFERSIZE];

ParserFuncPtrType ParserExecFunction;

// function pointer to single character output routine
void (*parserOutputFunc)(unsigned char c);


void parserInit(void)
{
	// initialize input buffer
	parserBufferLength = 0;
	// initialize executing function
	ParserExecFunction = 0;
	// initialize command list
	parserNumCommands = 0;
}


void parserAddCommand(unsigned char* newCmdString, ParserFuncPtrType newCmdFuncPtr)
{
	// add command string to end of command list
	strcpy(CommandList[parserNumCommands], newCmdString);
	// add command function ptr to end of function list
	ParserFunctionList[parserNumCommands] = newCmdFuncPtr;
	// increment number of registered commands
	parserNumCommands++;
}


void parserSetOutputFunc(void (*output_func)(unsigned char c))
{
	// set new output function
	parserOutputFunc = output_func;
}


void parserInputFunc(unsigned char c)
{
	// process the received character
	
	if (c != '\r')		//anything other than return character must be a part of the command
						//change to '\r' for terminal apz
	{	
		// echo character to the output
//		parserOutputFunc(c);
		// add it to the command line buffer
		parserBuffer[parserBufferLength] = c;
		// update buffer length
		parserBufferLength++;
	}
	else				//return character -> process command
	{
		// add null termination to command
		parserBuffer[parserBufferLength] = 0;
		// command is complete, process it
		parserProcessInputString();
		// reset buffer
		parserBufferLength = 0;
	}
}


void parserProcessInputString(void)
{
	unsigned char cmdIndex;

	// search command list for match with entered command
	for(cmdIndex=0; cmdIndex<parserNumCommands; cmdIndex++)
	{
		if( !strncmp(CommandList[cmdIndex], parserBuffer, 1) )		//command is first char of buffer
		{
			// user-entered command matched a command in the list (database)
			ParserExecFunction = ParserFunctionList[cmdIndex];
			// run the corresponding function
			ParserExecFunction();
			// reset
			ParserExecFunction = 0;
		}
	}
}

// return string pointer to argument [argnum]
unsigned char* parserGetArgStr(void)
{
	return &parserBuffer[2];		//spec states that commands are 1 char followed by a space followed by the arg, so the
}									//arg must start at idx 2

//return argument as a long
int parserGetArgInt(void)
{
	char* endptr;
	return atoi(parserGetArgStr());
}
