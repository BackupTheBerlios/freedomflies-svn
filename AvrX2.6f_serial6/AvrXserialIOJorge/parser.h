

#ifndef PARSER_H
#define PARSER_H

// constants/macros/typdefs
typedef void (*ParserFuncPtrType)(void);

// functions

//! initalize the command line system
void parserInit(void);

//! add a new command to the database of known commands
// newCmdString should be a null-terminated command string with no whitespace
// newCmdFuncPtr should be a pointer to the function to execute when
//   the user enters the corresponding command tring
void parserAddCommand(unsigned char* newCmdString, ParserFuncPtrType newCmdFuncPtr);

//! sets the function used for sending characters to the user terminal
void parserSetOutputFunc(void (*output_func)(unsigned char c));

//! call this function to pass input charaters from the user terminal
void parserInputFunc(unsigned char c);

// internal commands
void parserProcessInputString(void);

// argument retrieval commands
//! returns a string pointer to the argument on the command line
unsigned char* parserGetArgStr(void);

//return argument as a long
long parserGetArgInt(void);

#endif
//@}
