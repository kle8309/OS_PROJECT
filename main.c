//***********************  Lab 1  **************************
// Program written by:
// - Steven Prickett   EID: srp2389
// - Kelvin Le 		   EID: kl24956

// Date Created: 1/22/2015
// Date Modified: 1/25/2015
// 
// Lab number: 1
//
// Brief description of the program:
// - 
//
// Hardware connections:
// - Outputs:
//   - PA2     Screen SSI SCK           (SSI0Clk)
//   - PA3     Screen SSI CS            (SSI0Fss)
//   - PA5     Screen SSI TX            (SSI0Tx)
//   - PA6     Screen Data/Command Pin  (GPIO)
//   - PA7     Screen RESET Pin         (GPIO)
// - Inputs:
//   - 
//
//  Peripherals used:
//   - TIMER0   ADC0 trigger
//*********************************************************

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "tm4c123gh6pm.h"

#include "debug.h"

#include "pll.h"
#include "st7735.h"
#include "usb_uart.h"
#include "systick.h"
#include "pwm.h"
#include "adc.h"
#include "timer0.h"
#include "interpreter.h"
#include "debug.h"

#define LCD_WIDTH 128
#define LCD_HEIGHT 160

#define OUTPUT_UART_ENABLED     (true)
#define OUTPUT_SCREEN_ENABLED   (false)

// prototypes for functions defined in startup.s
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

// global variables
uint32_t msElapsed = 0;
char stringBuffer[16];
bool updateScreen = false;
uint32_t dd=0;
uint32_t loopcount=0;
int main(void){  
  DisableInterrupts();
  //////////////////////// perhipheral initialization ////////////////////////
  // init PLL to 80Mhz
  PLL_Init();
  
  // init screen, use white as text color
  Output_Init();
  Output_Color(ST7735_WHITE);  
  //Output_Clear();
  
  // init usb uart, generate interrupts when data received via usb
  USB_UART_Init();
  USB_UART_Enable_Interrupt();
    
  // init systick to generate an interrupt every 1ms (every 80000 cycles)  
  //SysTick_Init(80000);
  
  // init and enable PWM
 // PWM0A_Init(40000, 20000);
 // PWM0A_Enable();

  // init debug LEDs
  DEBUG_Init();

  // global enable interrupts
  EnableInterrupts();
  
  //////////////////////// main loop ////////////////////////    
	
  while(1){
		
    WaitForInterrupt();
/*
    if (updateScreen){
      updateScreen = false;
      // screen stuff here
    }
    */
    if (USB_BufferReady){
      USB_BufferReady = false;
      //INTER_HandleBuffer();
			
			loopcount++;
			dd++;
    }
    
		
		
  }
}


// this is used for printf to output to both the screen and the usb uart
int fputc(int ch, FILE *f){
  if (OUTPUT_SCREEN_ENABLED) ST7735_OutChar(ch);       // output to screen
  if (OUTPUT_UART_ENABLED) USB_UART_PrintChar(ch);     // output via usb uart
  return 1;
}
