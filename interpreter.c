#include "interpreter.h"
//#include "command2.h"
extern uint32_t RxFifo_Get(char*,long);

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
    RxFifo_Get(&letter,Fifo_Depth);
    INTER_CmdBuffer[i] = letter;
  } while ( INTER_CmdBuffer[i++] != 0 && i < BUFFER_SIZE);

  
  // break buffer into tokens
	// strtok expects a string as an arg on the first call
	// point where the last token was found is kept internally 
	// In subsequent calls, strtok expects a null pointer as an arg
  tokenPtr = strtok(INTER_CmdBuffer, delim);
  while ( tokenPtr != NULL ){
    tokens[numTokens++] = tokenPtr;  // numTokens is post-incremented after tokens[] is updated
    tokenPtr = strtok(NULL, delim);  // In subsequent calls, strtok expects a null pointer
  }
    
    printf("parsed tokens:\n");
    for (int i = 0; i < numTokens; i++){
        printf("%d - %s\n", i, tokens[i]); 
    }

}
