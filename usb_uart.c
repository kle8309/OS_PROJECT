#include "tm4c123gh6pm.h"
#include "usb_uart.h"

#include "interpreter.h"
#include "fifo.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>



/*
========================================================================================================================
==========                                             CONSTANTS                                              ==========
========================================================================================================================
*/

#define FIFOSIZE   64         // size of the FIFOs (must be power of 2)
#define FIFOSUCCESS 1         // return value on success
#define FIFOFAIL    0         // return value on failure

/*
========================================================================================================================
==========                                          GLOBAL VARIABLES                                          ==========
========================================================================================================================
*/

bool USB_BufferReady = false;

/*
========================================================================================================================
==========                                         USB UART FUNCTIONS                                         ==========
========================================================================================================================
*/

// create index implementation FIFO (see FIFO.h)
AddPointerFifo(Rx, FIFOSIZE, char, FIFOSUCCESS, FIFOFAIL)
AddPointerFifo(Tx, FIFOSIZE, char, FIFOSUCCESS, FIFOFAIL)

/*
===================================================================================================
  USB_UART :: USB_UART_Init
  
   - initializes the UART to use PA0,PA1 at 115200 baud
===================================================================================================
*/
void USB_UART_Init(void){
  TxFifo_Init();
  RxFifo_Init();
  
  // enable UART0
  SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R0; // activate UART0 clock gating
  while ((SYSCTL_PRUART_R & SYSCTL_PRUART_R0) == 0) {}; // wait for UART0 to activate
  
  // enable PORTA
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R0; // activate PORTA clock gating
  while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R0) == 0) {}; // wait for PORTA to activate; PG.404
  
  // configure PORTB pins for use with UART0
  GPIO_PORTA_AFSEL_R |= 0x03; // pg 668 gpio alternate function select 0b11 is for PA0 and PA1
  GPIO_PORTA_DEN_R   |= 0x03; // digital enable
  GPIO_PORTA_PCTL_R  |= (GPIO_PCTL_PA0_U0RX|GPIO_PCTL_PA1_U0TX ); //pg 686 port mux control
  GPIO_PORTA_DR2R_R  |= 0x03; // select current drive 2mA pg670
  
  // configure UART0 for 115200bps operation
  // IBRD = 80e6/(16*115200) = 43.4027 = 43
  // FBRD = integer(.402777*64 + 0.5)  = 26   
  UART0_CTL_R &= ~UART_CTL_UARTEN;                  // clear UART0 enable bit during config
  UART0_IBRD_R = 43;                                // set integer portion of BRD
  UART0_FBRD_R = 26;                                // set fraction portion of BRD
  UART0_LCRH_R = (UART_LCRH_WLEN_8|UART_LCRH_FEN);  // 8 bit word length, 1 stop, no parity, FIFOs enabled
  UART0_CC_R   = 0x00;                              // use system clock
  UART0_IFLS_R &= ~0x3F;                            // clear TX and RX interrupt FIFO level fields
  UART0_IFLS_R |= UART_IFLS_RX2_8;                  // RX FIFO interrupt threshold >= 2/8th full; Required >=2/8 for escape sequence to work.
  UART0_IFLS_R |= UART_IFLS_TX1_8;                  // TX FIFO interrupt threshold <= 1/8th full
  UART0_IM_R  |= (UART_IM_RXIM | UART_IM_RTIM);     // enable interupt on RX and RX timeout
 //UART0_IM_R  |= UART_IM_TXIM;                     // enable interrupt on TX
	UART0_IM_R  &= ~UART_IM_TXIM;                      // disable interrupt on TX
  UART0_CTL_R |= UART_CTL_UARTEN;                   // set UART0 enable bit    
}



/*
===================================================================================================
  USB_UART :: USB_UART_Enable_Interrupt
  
   - enables the UART0 interrupt
===================================================================================================
*/
void USB_UART_Enable_Interrupt(void){
	//NVIC_ENx_R where x=int(IRQ/4)
	NVIC_EN0_R |= NVIC_EN0_INT5; //UART0
  //NVIC_EN0_R |= NVIC_EN0_INT6; //UART1
}

/*
===================================================================================================
  USB_UART :: USB_UART_DisableRXInterrupt
  
   - disables the UART0 RX interrupt
===================================================================================================
*/
void USB_UART_DisableRXInterrupt(void){
  NVIC_EN0_R &= ~NVIC_EN0_INT5; 
	//NVIC_EN0_R &= ~NVIC_EN0_INT6;    	
}

/*
===================================================================================================
  USB_UART :: UART0_Handler
  
   - handles incoming and outgoing UART0 interrupts
===================================================================================================
*/

int count_t=0;
int count_b=0;
int top=0;
int bottom=0;
int trigger_pt=0;
volatile uint32_t RAW_INT_STAT;

void UART0_Handler(void){
	
	//debug raw int status register
	RAW_INT_STAT=UART0_RIS_R; 							
	count_t++;
	top++;
	
	// RX FIFO >= 2/8 full 
  if(UART0_RIS_R & UART_RIS_RXRIS){       
		RAW_INT_STAT= UART0_RIS_R;							// debug raw int status register
    UART0_ICR_R = UART_ICR_RXIC;          // acknowledge interrupt
    USB_UART_HandleRXBuffer();            // copy from hardware RX FIFO to software RX FIFO
  }

	// RX TIME-OUT
  if(UART0_RIS_R&UART_RIS_RTRIS){         
    RAW_INT_STAT= UART0_RIS_R;							//debug raw int status register
		UART0_ICR_R = UART_ICR_RTIC;          // acknowledge receiver time
    USB_UART_HandleRXBuffer();            // copy from hardware RX FIFO to software RX FIFO
  }
	
	//debug raw int status register
	RAW_INT_STAT= UART0_RIS_R;								
	count_b++;
	bottom++;
}

/*
===================================================================================================
  USB_UART :: USB_UART_PrintChar
  
   - outputs a character via UART0
===================================================================================================
*/
void USB_UART_PrintChar(char input){
	
  while (UART0_FR_R & UART_FR_TXFF) {}//spin when fifo is full until fifo is not full; FR flag register
		UART0_DR_R = input;
}

/*
===================================================================================================
  USB_UART :: USB_UART_HandleRXBuffer
  
   - puts a character in the software FIFO, with a few special execptions
===================================================================================================
*/
// Use for setting the RX status of the 3 escape sequence codes
#define INVALID_CODE false
#define VALID_CODE true
	
//arrow key enum
typedef enum e_KEY
{
		INVALID_KEY=0,
    LEFT_ARROW_KEY,
    RIGHT_ARROW_KEY,
    DOWN_ARROW_KEY,
		UP_ARROW_KEY
	  
} KeyType;


KeyType key = INVALID_KEY;				//delare debug variable

//int count=0;       								//interrupt count
void Decode_ESC_SEQ(char letter);
static uint8_t read_counter=0; 					//fifo read count
void USB_UART_HandleRXBuffer(void){
		char letter;
		
	  //count++; //uart interrupt count DEBUG
	
		while((UART0_FR_R & UART_FR_RXFE) == 0){					// if UART Receive FIFO is not Empty (1 means empty)
				letter = UART0_DR_R; 														// copy from RX HW fifo to memory
				USB_UART_PrintChar(letter);                     // echo typed character back to user terminal
				
				// take a character from the hardware fifo
				if (letter =='\r') {   
					
					// new line, don't put in buffer
					RxFifo_Put(0);                                // null terminate buffer
					USB_BufferReady = true;                       // toggle buffer processing semaphore
					
				} else if (letter == '\n' || letter == 12) {    // ctrl-L is ASCII 12, form feed
					 // do nothing
				} else if (letter == 127) {                    // handle backspace
					RxFifo_Pop();                                 // remove a char from the end of the fifo
				}	else {
					// START ESCAPE SEQUENCE CODE
					// 27 91 'D' or 'C' or 'B' or 'A'
					// 				Left   Right  Down   Up
			
					//First code filter to reduce bandwidth (i.e. this is not used as often as other keys)
					//if first letter is not the starter code we can skip the rest
					// on non-zero read_counter letter is allowed to have number other than 27
					if(letter != 27 && read_counter == 0){ 
					  RxFifo_Put(letter);          // if not one of the escape codes put char in fifo
																				 // 'ESC' '[' follow by 'A' 'B' 'C' 'D' are not supported as commands
																				 // these code are ignored in SW FIFO
																				 // if need to support for some odd reason, a patch is needed
					} else{
						Decode_ESC_SEQ(letter);
					}
					// END ESCAPE SEQUENCE CODE					
	
				} // END letter condition
				
				
		} //end HW FIFO read while
		
} //end RX handle

// decode escape sequence function
void Decode_ESC_SEQ(char letter){

		if (letter == 27 && read_counter == 0){																											
				read_counter++;														// first match												
																										
		} else if (letter == 91 && read_counter == 1){ 	// move on to second code																															// if first and second code in sequence are correct
				read_counter++;	// second match
																																																	
		}	else if (read_counter == 2 )	{ 						// third and last code check
																									// previous two codes matched escape sequence
																									// reset counter to prevent false validation due to previous valid state	
			read_counter=0; 
			switch(letter){
				case 68:							
					key=LEFT_ARROW_KEY;
					RxFifo_Shift_L();
				//TODO: add code to move fifo pointer similar to backspace code
					break;
				case 67:								
					key=RIGHT_ARROW_KEY;
					RxFifo_Shift_R();
				//TODO: add code to move fifo pointer similar to backspace code
					break;
				case 66:							
					key=DOWN_ARROW_KEY;
					//TODO: add code.  Need 2D SW fifo array
					break;
				case 65:						
					key=UP_ARROW_KEY;
					//TODO: add code.  Need 2D SW fifo array
					break;
				default:	
					key = INVALID_KEY;
					break;					
			}
		}	else{
				read_counter=0;		// reset
				key = INVALID_KEY;		// either code1 or code2 is wrong so skip to next iteration inside while loop 
		}										
}

/*
===================================================================================================
  USB_UART :: USB_UART_HandleTXBuffer
  
   - does... something....
===================================================================================================
*/

