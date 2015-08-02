#include "tm4c123gh6pm.h"
#include "os.h"
#include "debug.h"
#include "systick.h"
#include "defs.h"

#define MAX_PERIODIC_TASKS 8

#define DEFAULT_PRIORITY 0xFFFFFFFF
#define DEFAULT_PERIOD   0xFFFFFFFF

uint32_t OS_Timer;

PeriodicTask OS_PeriodicTasks[MAX_PERIODIC_TASKS];

// inits SysTick and OS_Timer stuff
void OS_InitPeriodicClock(uint32_t period){
  OS_Timer = 0;
  
  // TODO: maybe this doesn't go here
  for (int i = 0; i < MAX_PERIODIC_TASKS; i++){
      OS_PeriodicTasks[i].period = DEFAULT_PERIOD;
      OS_PeriodicTasks[i].priority = DEFAULT_PRIORITY;
      OS_PeriodicTasks[i].task = NULL;
  }
  
  SysTick_Init(period);
}

// resets time counter
void OS_ClearPeriodicTime(void){
  OS_Timer = 0;
}

// attempts to add a task to the periodic task list
uint32_t OS_AddPeriodicThread(void(*task)(void), uint32_t period, uint32_t priority){
  int i = 0;
  
  // find open spot in periodic task array
  while (OS_PeriodicTasks[i].task != NULL && i < MAX_PERIODIC_TASKS){
    i++;
  }
  
  // return fail if we can't find an open spot
  if (i == MAX_PERIODIC_TASKS){
    return CMD_FAILURE;
  }
  
  // add task to periodic task array
  PeriodicTask newTask = {task, period, priority};
  OS_PeriodicTasks[i] = newTask;
  
  return CMD_SUCCESS;
}    

// removes a task from periodic execution
uint32_t OS_RemovePeriodicThread(void(*task)(void)){
  for (int i = 0; i < MAX_PERIODIC_TASKS; i++){
    if (task == OS_PeriodicTasks[i].task){
        OS_PeriodicTasks[i].task = NULL;
        OS_PeriodicTasks[i].period = DEFAULT_PERIOD;
        OS_PeriodicTasks[i].priority = DEFAULT_PRIORITY;
        return CMD_SUCCESS;
    }
  }  
  return CMD_FAILURE;  
}
 
// returns value of OS_Timer
uint32_t OS_ReadPeriodicTime(void){
  return OS_Timer;
}

// this is called every time the systick generates an interrupt
void SysTick_Handler(void){
	debug_ledToggle(PF2);
  OS_Timer++;

  for (int i = 0; i < MAX_PERIODIC_TASKS; i++){
    if ((OS_Timer % OS_PeriodicTasks[i].period) == 0){
        OS_PeriodicTasks[i].task();
    }
  }
	debug_ledToggle(PF2);
	
	
}

