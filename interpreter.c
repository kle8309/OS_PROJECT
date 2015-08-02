#include "interpreter.h"
#include "pwm.h"
#include "command.h"
#include "debug.h"
#include "fifo.h"

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

/*
========================================================================================================================
==========                                             CONSTANTS                                              ==========
========================================================================================================================
*/

#define BUFFER_SIZE 64
#define MAX_TOKENS 16

/*
========================================================================================================================
==========                                          GLOBAL VARIABLES                                          ==========
========================================================================================================================
*/

char INTER_CmdBuffer[BUFFER_SIZE];

/*
========================================================================================================================
==========                                       INTERPRETER FUNCTIONS                                        ==========
========================================================================================================================
*/

/*
===================================================================================================
  INTERPRETER FUNCTION :: INTER_HandleBuffer
  
   - grabs a null terminated string from the RxFifo, tokenizes it, and
     attempts to locate an associated handler for a command
===================================================================================================
*/
void INTER_HandleBuffer(void){
    
  char delim[] = " "; 
  char* tokens[MAX_TOKENS];
  char* tokenPtr;
  int numTokens = 0;

  int i = 0;
  // retrieve buffer from fifo into INTER_CmdBuffer
  do {
    char letter;
    RxFifo_Get(&letter);
    INTER_CmdBuffer[i] = letter;
  } while ( INTER_CmdBuffer[i++] != 0 && i < BUFFER_SIZE);

  
  // break buffer into tokens
  tokenPtr = strtok(INTER_CmdBuffer, delim);
  while ( tokenPtr != NULL ){
    tokens[numTokens++] = tokenPtr;  // numTokens is post-incremented after tokens[] is updated
    tokenPtr = strtok(NULL, delim);  // In subsequent calls, strtok expects a null pointer
  }
    
//    printf("parsed tokens:\n");
//    for (int i = 0; i < numTokens; i++){
//        printf("%d - %s\n", i, tokens[i]); 
//    }

   i = 0;
   bool cmdFound = false;
   while (mainCommands[i].function != NULL){									//if function pointer is not NULL
																															
																															// mainCommands[] array is pre-defined in command.c
        if (strcmp(tokens[0], mainCommands[i].key) == 0){     // check the first user token to key
																															// example: tokens[] pointer array points to 
																															// 0) adcCollect 1) channel 2) freq 3) nSamples
																															// if token matches with the ith command
																															// then execute the ith command's function using
																														  // the token[1].  
																															// mainCommands is a Command struct with a function pointer
              mainCommands[i].function(tokens, numTokens); // '1' is the index which will be used by token
					
              cmdFound = true;								
        }
        i++;
    }
    if (!cmdFound) printf ("ERROR: No matching command found.\n");
}
