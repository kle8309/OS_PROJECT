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
  
   - initializes the UART to use PA0,1 at 115200 baud
===================================================================================================
*/
void USB_UART_Init(void){
  TxFifo_Init();
  RxFifo_Init();
  
  // enable UART1
  SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R1; // activate UART1 clock gating
  while ((SYSCTL_PRUART_R & SYSCTL_PRUART_R1) == 0) {}; // wait for UART1 to activate
  
  // enable PORTB
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1; // activate PORTB clock gating
  while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R1) == 0) {}; // wait for PORTB to activate
  
  // configure PORTB pins for use with UART1
  GPIO_PORTB_AFSEL_R |= 0x03; // pg 668 gpio alternate function select 0b11 is for PB0 and PB1
  GPIO_PORTB_DEN_R   |= 0x03; // digital enable
  GPIO_PORTB_PCTL_R  |= (GPIO_PCTL_PB0_U1RX|GPIO_PCTL_PB1_U1TX ); //pg 686 port mux control
  GPIO_PORTB_DR2R_R  |= 0x03; // select current drive 2mA pg670
  
  // configure UART1 for 115200bps operation
  // IBRD = 80e6/(16*115200) = 43.4027 = 43
  // FBRD = integer(.402777*64 + 0.5)  = 26   
  UART1_CTL_R &= ~UART_CTL_UARTEN;                  // clear UART1 enable bit during config
  UART1_IBRD_R = 43;                                // set integer portion of BRD
  UART1_FBRD_R = 26;                                // set fraction portion of BRD
  UART1_LCRH_R = (UART_LCRH_WLEN_8|UART_LCRH_FEN);  // 8 bit word length, 1 stop, no parity, FIFOs enabled
  UART1_CC_R   = 0x00;                              // use system clock
  UART1_IFLS_R &= ~0x3F;                            // clear TX and RX interrupt FIFO level fields
  UART1_IFLS_R |= UART_IFLS_RX2_8;                  // RX FIFO interrupt threshold >= 2/8th full
  UART1_IFLS_R |= UART_IFLS_TX1_8;                  // TX FIFO interrupt threshold <= 1/8th full
  UART1_IM_R  |= (UART_IM_RXIM | UART_IM_RTIM);     // enable interupt on RX and RX timeout
 //UART1_IM_R  |= UART_IM_TXIM;                     // enable interrupt on TX
	UART1_IM_R  &= ~UART_IM_TXIM;                      // disable interrupt on TX
  UART1_CTL_R |= UART_CTL_UARTEN;                   // set UART1 enable bit    
}



/*
===================================================================================================
  USB_UART :: USB_UART_Enable_Interrupt
  
   - enables the UART1 interrupt
===================================================================================================
*/
void USB_UART_Enable_Interrupt(void){
	//NVIC_ENx_R where x=int(IRQ/4)
	//NVIC_EN0_R |= NVIC_EN0_INT5; //UART1
  NVIC_EN0_R |= NVIC_EN0_INT6; //UART1
}

/*
===================================================================================================
  USB_UART :: USB_UART_DisableRXInterrupt
  
   - disables the UART1 RX interrupt
===================================================================================================
*/
void USB_UART_DisableRXInterrupt(void){
  //NVIC_EN0_R &= ~NVIC_EN0_INT5; 
	NVIC_EN0_R &= ~NVIC_EN0_INT6;    	
}

/*
===================================================================================================
  USB_UART :: UART1_Handler
  
   - handles incoming and outgoing UART1 interrupts
===================================================================================================
*/

int count_t=0;
int count_b=0;
int top=0;
int bottom=0;
int trigger_pt=0;
volatile uint32_t RAW_INT_STAT;

void UART1_Handler(void){
	
	//debug raw int status register
	RAW_INT_STAT=UART1_RIS_R; 							
	count_t++;
	top++;
	
	// RX FIFO >= 2/8 full 
  if(UART1_RIS_R & UART_RIS_RXRIS){       
		RAW_INT_STAT=UART1_RIS_R;							//debug raw int status register
    UART1_ICR_R = UART_ICR_RXIC;          // acknowledge interrupt
    USB_UART_HandleRXBuffer();            // copy from hardware RX FIFO to software RX FIFO
  }

	// RX TIME-OUT
  if(UART1_RIS_R&UART_RIS_RTRIS){         
    RAW_INT_STAT=UART1_RIS_R;							//debug raw int status register
		UART1_ICR_R = UART_ICR_RTIC;          // acknowledge receiver time
    USB_UART_HandleRXBuffer();            // copy from hardware RX FIFO to software RX FIFO
  }
	
	//debug raw int status register
	RAW_INT_STAT=UART1_RIS_R;								
	count_b++;
	bottom++;
	
	
}

/*
===================================================================================================
  USB_UART :: USB_UART_PrintChar
  
   - outputs a character via UART1
===================================================================================================
*/
void USB_UART_PrintChar(char input){
	
  while (UART1_FR_R & UART_FR_TXFF) {}//spin when fifo is full until fifo is not full; FR flag register
		UART1_DR_R = input;
}

/*
===================================================================================================
  USB_UART :: USB_UART_HandleRXBuffer
  
   - puts a character in the software FIFO, with a few special execptions
===================================================================================================
*/
void USB_UART_HandleRXBuffer(void){
		char letter;
		while((UART1_FR_R & UART_FR_RXFE) == 0){					// if UART Receive FIFO is not Empty (1 means empty)
				letter = UART1_DR_R; 														// copy from RX HW fifo to memory
				USB_UART_PrintChar(letter);                     // echo typed character back to user terminal
				
				// take a character from the hardware fifo
				if (letter =='\r') {   
					
					// new line, don't put in buffer
					RxFifo_Put(0);                                // null terminate buffer
					USB_BufferReady = true;                       // toggle buffer processing semaphore
					
				} else if (letter == '\n' || letter == 12) {    // ctrl-L is ASCII 12, form feed
					 // do nothing
				} else if (letter == 8) {                       // handle backspace
					RxFifo_Pop();                                 // remove a char from the end of the fifo
					USB_UART_PrintChar(8);                        // return a backspace to the user
					USB_UART_PrintChar(' ');                      // clear char on uart
				} else {
					RxFifo_Put(letter);                           // put char in fifo
				}
		
		}
}

/*
===================================================================================================
  USB_UART :: USB_UART_HandleTXBuffer
  
   - does... something....
===================================================================================================
*/

