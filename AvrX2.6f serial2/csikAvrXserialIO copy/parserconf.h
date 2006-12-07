#ifndef PARSERCONF_H
#define PARSERCONF_H




// size of command database
// (maximum number of commands the parser system can handle)
#define MAX_COMMANDS	10

// maximum length (number of characters) of each command string
// (quantity must include one additional byte for a null terminator)
#define MAX_CMD_LENGTH	15

// allotted buffer size for command entry
// (must be enough chars for typed commands and the arguments that follow)
#define BUFFERSIZE		15

#endif

