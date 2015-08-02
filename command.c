#include "command.h"
#include "adc.h"
#include "os.h"
#include "debug.h"
#include "st7735.h"
#include "pwm.h"

/*
========================================================================================================================
==========                                          GLOBAL VARIABLES                                          ==========
========================================================================================================================
*/

volatile bool ledTogglerEnabled = false;

/*
========================================================================================================================
==========                                           COMMAND ARRAYS                                           ==========
========================================================================================================================
*/

// array of main commands
Command mainCommands[] = { 
  { "set", setHandler, setCommands, "[listed variable] : sets an environment variable"},
  { "get", getHandler, getCommands, "[listed variable] : returns an environment variable"},
  { "run", runHandler, runCommands, "[listed command] : runs a command"},
  { "help", helpHandler, NULL, "[set,get,run] : prints help text"},

  { 0, NULL, NULL, 0} // array terminator
};

// array of set commands
Command setCommands[] = { 
  { "pwmFreq", pwmFreqSetter, NULL, "[frequency] : sets PWM0A frequency"},
  //{ "pwmPeriod", pwmPeriodHandler, NULL, "[period] : sets PWM0A period (in 25ns units)"},
  { "pwmDuty", pwmDutySetter, NULL, "[duty cycle] : sets PWM0A duty cycle (in integer percent)"},
  //{ "pwmDutyTime", pwmDutyTimeHandler, NULL, "[duty time] : sets PWM0A duty time (in 25ns units)"},

  { 0, NULL, NULL, 0} // array terminator
};

// array of get commands
Command getCommands[] = { 
  { "pwmFreq", pwmFreqGetter, NULL, ": gets current PWM0A frequency"},
    
  { 0, NULL, NULL, 0} // array terminator
};

// array of run commands
Command runCommands[] = {
  { "adcCollect", adcTestHandler, NULL, "[channelNum] [frequency] [numSamples] : runs adc collection"},
  { "ledToggler", ledTogglerHandler, NULL, "[frequency in ms] : turns on led periodic task"},
  { "ledDisabler", ledDisablerHandler, NULL, ": turns off led periodic task"},
  { "helloTop", helloTopScreenHandler, NULL, ": says hello from the top screen"},
  { "helloBottom", helloBottomScreenHandler, NULL, ": says hello from the bottom screen"},

  { 0, NULL, NULL, 0} // array terminator
};

/*
========================================================================================================================
==========                                     COMMAND HANDLER FUNCTIONS                                      ==========
========================================================================================================================
*/

/*
===================================================================================================
  COMMAND HANDLER :: setHandler
  
   - parses for set commands and attempts to execute them
   - return success value
===================================================================================================
*/
int setHandler(char** tokens, uint8_t numTokens){
  int i = 0;
  //printf ("you made it to setHandler!\n");
  if (numTokens < 2){
    printf("ERROR: SET needs more args.\n\n");
    printf("Available SET commands:\n");
    printHelp(setCommands, " ");
    return CMD_FAILURE;
  }
  while (setCommands[i].function != NULL){      // the setCommands array 
    if (strcmp(tokens[1], setCommands[i].key) == 0){
      setCommands[i].function(tokens, numTokens);
      return CMD_SUCCESS;
    }
    i++;
  }
  printf("ERROR: No matching SET command found.\n");
  return CMD_FAILURE;}

/*
===================================================================================================
  COMMAND HANDLER :: getHandler
  
   - parses for get commands and attempts to execute them
   - return success value
===================================================================================================
*/
int getHandler(char** tokens, uint8_t numTokens){
  int i = 0;
  //printf ("you made it to getHandler!\n");
  if (numTokens < 2){
    printf("ERROR: GET needs more args.\n\n");
    printf("Available GET commands:\n");
    printHelp(getCommands, " ");
    return CMD_FAILURE;
  }
  while (getCommands[i].function != NULL){      // the getCommands array 
    if (strcmp(tokens[1], getCommands[i].key) == 0){
      getCommands[i].function(tokens, numTokens);
      return CMD_SUCCESS;
    }
    i++;
  }
  printf("ERROR: No matching GET command found.\n");
  return CMD_FAILURE;
}
    
/*
===================================================================================================
  COMMAND HANDLER :: runHandler
  
   - parses for run commands and attempts to execute them
   - return success value
===================================================================================================
*/
int runHandler(char** tokens, uint8_t numTokens){
  int i = 0;
  //printf ("you made it to runHandler!\n");
  if (numTokens < 2){
    printf("ERROR: RUN needs more args.\n\n");
    printf("Available run commands:\n");
    printHelp(runCommands, " ");
    return CMD_FAILURE;
  }
  while (runCommands[i].function != NULL){      // the runCommands array 
    if (strcmp(tokens[1], runCommands[i].key) == 0){
      runCommands[i].function(tokens, numTokens);
      return CMD_SUCCESS;
    }
    i++;
  }
  printf("ERROR: No matching RUN command found.\n");
  return CMD_FAILURE;
}

/*
===================================================================================================
  COMMAND HANDLER :: helpHandler
  
   - prints help output
   - return success value
===================================================================================================
*/
int helpHandler(char** tokens, uint8_t numTokens){
  int i = 0;
  //printf ("you made it to helpHandler!\n");
  if (numTokens > 1){
    while (mainCommands[i].function != NULL){
      if ((strcmp(tokens[1], mainCommands[i].key) == 0) && (mainCommands[i].childArray != NULL)){
        printHelp(mainCommands[i].childArray, " ");
        printf("\n");
        return CMD_SUCCESS;
      }
      i++;
    }
  } else {
    printf("\nAvailable commands:\n\n");
    while (mainCommands[i].function != NULL){
      printf("%s %s\n", mainCommands[i].key, mainCommands[i].helpText);
      if (mainCommands[i].childArray != NULL){
        printHelp(mainCommands[i].childArray, " - ");
      }
      i++;
    }
    printf("\n");
  }
    
  return CMD_FAILURE;
}
    
/*
===================================================================================================
  COMMAND HANDLER :: pwmFreqSetter
  
   - attempts to set PWM0A frequency with arguments from the command line
   - return success value
===================================================================================================
*/
int pwmFreqSetter(char** tokens, uint8_t numTokens){
  // verify correct number of argument tokens, show help if invalid
  if (numTokens < 3) {
    printf("ERROR: Incorrect number of args.\n\n");
    printf("  Usage: set pwmFreq [frequency]\n\n");
    return CMD_FAILURE;
  }
  
  // attempt to parse arguments
  int frequency = atoi(tokens[2]);
  
  // verify frequency range and set if valid
  if (frequency >= 625 && frequency < 100000){
    printf("  Setting PWM0A frequency to %dHz...\n\n", frequency);
    PWM0A_SetFrequency(frequency);
  } else {
    // print error and fail
    printf("ERROR: pwmFreq must be between 625 and 100,000.\n");
    return CMD_FAILURE;
  }
  return CMD_SUCCESS;
}

/*
===================================================================================================
  COMMAND HANDLER :: pwmPeriodHandler
  
   - NYI
   - return success value
===================================================================================================
*/
int pwmPeriodHandler(char** tokens, uint8_t numTokens){
  // NYI
  return CMD_SUCCESS;
}

/*
===================================================================================================
  COMMAND HANDLER :: pwmDutyPercentHandler
  
   - attempts to set PWM0A duty cycle with arguments from the command line
   - return success value
===================================================================================================
*/
int pwmDutySetter(char** tokens, uint8_t numTokens){
  // verify correct number of argument tokens, show help if invalid
  if (numTokens < 3) {
    printf("ERROR: Incorrect number of args.\n\n");
    printf("  Usage: set pwmDuty [percent]\n\n");
    return CMD_FAILURE;
  }
  
  // attempt to parse arguments
  int duty = atoi(tokens[2]);
  
  // verify frequency range and set if valid
  if (duty >= 0 && duty < 99){
    printf("  Setting PWM0A duty cycle to %d%%...\n\n", duty);
    PWM0A_SetDutyPercent(duty);
  } else {
    // print error and fail
    printf("ERROR: Duty cycle must be betwee 0 and 99.\n\n");
    return CMD_FAILURE;
  }
  return CMD_SUCCESS;
}

/*
===================================================================================================
  COMMAND HANDLER :: pwmDutyTimeHandler
  
   - NYI
   - return success value
===================================================================================================
*/
int pwmDutyTimeHandler(char** tokens, uint8_t numTokens){
    // NYI
    return CMD_SUCCESS;
}    

/*
===================================================================================================
  COMMAND HANDLER :: adcTestHandler
  
   - parses the command line and begins a adc sampler task
   - return success value
===================================================================================================
*/
int adcTestHandler(char** tokens, uint8_t numTokens){
  // verify correct number of argument tokens, show help if invalid
  if (numTokens < 5) {
    printf("ERROR: Incorrect number of args.\n\n");
    printf("  Usage: adcCollect [channel, 0-11] [frequency, 100-10000] [numSamples]\n\n");
    return CMD_FAILURE;
  }

  // attempt to parse arguments
  int channel = atoi(tokens[2]);
  int frequency = atoi(tokens[3]);
  int numSamples = atoi(tokens[4]);

  //printf("channel = %d, freq = %d, numSamples = %d\n", channel, frequency, numSamples);

  // verify channel range
  if (channel < 0 || channel > 11){
    printf("ERROR: Channel number out of range!\n\n");
    return CMD_FAILURE;
  } 
  
  // verify frequency range
  if (frequency < 100 || frequency > 10000){
    printf("ERROR: Sample frequency out of range!\n\n");
    return CMD_FAILURE;
  } 

  // attempt to malloc a buffer, complain and fail if memory unavailable
  ADCBufferPointer = malloc(numSamples * sizeof(*ADCBufferPointer));
  if (ADCBufferPointer == NULL){
    printf("ERROR: Could not allocate sample buffer memory!\n\n");
    return CMD_FAILURE;
  } 
    
  // start adc collection task
  ADC_Collect(channel, frequency, ADCBufferPointer, numSamples);

  return CMD_SUCCESS;
}

/*
===================================================================================================
  COMMAND HANDLER :: ledTogglerHandler
  
   - enables the periodic led toggler task 
   - return success value
===================================================================================================
*/
int ledTogglerHandler(char** tokens, uint8_t numTokens){
  if (ledTogglerEnabled){
    printf("ERROR: Toggler already running!\n\n");
    return CMD_FAILURE;
  }
    
	// verify correct number of arguments
  if (numTokens < 3) {
    printf("ERROR: Incorrect number of args.\n\n");
    printf("  Usage: ledToggler [period in ms]\n\n");
    return CMD_FAILURE;
  }

	// parse arguments to integers
  int period = atoi(tokens[2]);
	
	// verify argument range
	if (period < 1 || period > 1000000){
    printf("ERROR: Period must be positive and less than 1,000,000 ms.\n\n");
    return CMD_FAILURE;
  }
	
	// actually do what we want
	OS_AddPeriodicThread(ledTogglerTask, period, 0);
  printf("  Enabling PF3 toggler with a %dms period...\n\n", period);
	
  ledTogglerEnabled = true;
	return CMD_SUCCESS;
}

/*
===================================================================================================
  COMMAND HANDLER :: ledDisablerHandler
  
   - disables the periodic led toggler task 
   - return success value
===================================================================================================
*/
int ledDisablerHandler(char** tokens, uint8_t numTokens){
	// actually do what we want
  printf("  Disabling PF3 toggler...\n");
	PF3 = 0x00;
  ledTogglerEnabled = false;
	if (OS_RemovePeriodicThread(ledTogglerTask)){
      printf("  ERROR: Could not remove task.\n\n");
      return CMD_FAILURE;
  }
  printf("\n");
	return CMD_SUCCESS;
}

/*
===================================================================================================
  COMMAND TASK ::  ledTogglerTask
  
   - toggles PF3 (usually called as a periodic task)
===================================================================================================
*/
void ledTogglerTask(void){
	debug_ledToggle(PF3);		// toggle green led
}

/*
===================================================================================================
  COMMAND HANDLER :: helloTopScreenHandler
  
   - test print for upper screen
   - return success value
===================================================================================================
*/
int helloTopScreenHandler(char** tokens, uint8_t numTokens){
  printf("  Writing something to the top screen...\n\n");
  ST7735_Message(0, 0, "Hello top screen!", 42);
  return CMD_SUCCESS;
}

/*
===================================================================================================
  COMMAND HANDLER :: helloBottomScreenHandler
  
   - test print for bottom screen
   - return success value
===================================================================================================
*/
int helloBottomScreenHandler(char** tokens, uint8_t numTokens){
  printf("  Writing something to the bottom screen...\n\n");
  ST7735_Message(1, 0, "Hello bottom screen!", 42);
  return CMD_SUCCESS;
} 

/*
===================================================================================================
  COMMAND HELPER :: printHelp
  
   - prints help text of a specified commandArray, with optional indentation
   - return success value
===================================================================================================
*/
void printHelp(Command* commandArray, char* indent){
  int i = 0;
  // iterate through the command array printing each commands help text
  while (commandArray[i].function != NULL){
    printf("%s%s %s\n", indent, commandArray[i].key, commandArray[i].helpText);
    i++;
  }
  printf("\n");
}

/*
===================================================================================================
  COMMAND GETTER :: pwmFreqGetter
  
   - 
   - return success value
===================================================================================================
*/
int pwmFreqGetter(char** tokens, uint8_t numTokens){
  printf("\nPWM0A frequency is: %d Hz\n\n", PWM0A_GetFrequency());
  return CMD_SUCCESS;
}
