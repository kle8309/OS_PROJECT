#ifndef COMMAND_H
#define COMMAND_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "defs.h"

/*
========================================================================================================================
==========                                       EXTERNAL DEPENDENCIES                                        ==========
========================================================================================================================
*/

extern uint32_t RxFifo_Get(char*,long);

/*
========================================================================================================================
==========                                     DATA STRUCTURE DEFINITIONS                                     ==========
========================================================================================================================
*/

// defines command handler function signature
typedef int (*cmdFunc)(char**, uint8_t);

// command data structure definition
typedef struct command_t {
    char* key;                      // string used to identify command
    cmdFunc function;               // pointer to function that handles command exectuion
    struct command_t* childArray;   // pointer to array of child functions (e.g, the commands associated with set)
    char* helpText;                 // text to print for help display
} Command;

/*
========================================================================================================================
==========                                        PUBLIC DATA OBJECTS                                         ==========
========================================================================================================================
*/

extern Command mainCommands[];
extern Command setCommands[];
extern Command getCommands[];
extern Command runCommands[];

/*
========================================================================================================================
==========                                     COMMAND HANDLER PROTOTYPES                                     ==========
========================================================================================================================
*/

// main command prototypes
int setHandler(char** tokens, uint8_t numTokens);
int getHandler(char** tokens, uint8_t numTokens);
int runHandler(char** tokens, uint8_t numTokens);
int helpHandler(char** tokens, uint8_t numTokens);

// set command prototypes
int pwmFreqSetter(char** tokens, uint8_t numTokens);
int pwmPeriodHandler(char** tokens, uint8_t numTokens);
int pwmDutySetter(char** tokens, uint8_t numTokens);
int pwmDutyTimeHandler(char** tokens, uint8_t numTokens);

// get command prototypes
int pwmFreqGetter(char** tokens, uint8_t numTokens);

// run command prototypes
int adcTestHandler(char** tokens, uint8_t numTokens);
int ledTogglerHandler(char** tokens, uint8_t numTokens);
int ledDisablerHandler(char** tokens, uint8_t numTokens);
int helloTopScreenHandler(char** tokens, uint8_t numTokens);
int helloBottomScreenHandler(char** tokens, uint8_t numTokens);

// tasks (for now)
void ledTogglerTask(void);

// helpers
void printHelp(Command* commandArray, char* indent);

#endif
