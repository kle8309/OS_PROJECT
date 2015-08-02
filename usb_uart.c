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
  
  // enable UART0
  SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R0; // activate UART0 clock gating
  while ((SYSCTL_PRUART_R & SYSCTL_PRUART_R0) == 0) {}; // wait for UART0 to activate
  
  // enable PORTA
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R0; // activate PORTA clock gating
  while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R0) == 0) {}; // wait for PORTA to activate
  
  // configure PORTA pins for use with UART0
  GPIO_PORTA_AFSEL_R |= 0x03;
  GPIO_PORTA_DEN_R   |= 0x03;
  GPIO_PORTA_PCTL_R  |= 0x11;
  GPIO_PORTA_DR2R_R  |= 0x03;
  
  // configure UART0 for 115200bps operation
  // IBRD = 80e6/(16*115200) = 43.4027 = 43
  // FBRD = integer(.402777*64 + 0.5)  = 26   
  UART0_CTL_R &= ~0x01;                             // clear UART0 enable bit during config
  UART0_IBRD_R = 43;                                // set integer portion of BRD
  UART0_FBRD_R = 26;                                // set fraction portion of BRD
  UART0_LCRH_R = (UART_LCRH_WLEN_8|UART_LCRH_FEN);  // 8 bit word length, 1 stop, no parity, FIFOs enabled
  UART0_CC_R   = 0x00;                              // use system clock
  UART0_IFLS_R &= ~0x3F;                            // clear TX and RX interrupt FIFO level fields
  UART0_IFLS_R |= UART_IFLS_RX1_8;                  // RX FIFO interrupt threshold >= 1/8th full
  UART0_IFLS_R |= UART_IFLS_TX1_8;                  // TX FIFO interrupt threshold <= 4/8th full
  UART0_IM_R  |= (UART_IM_RXIM | UART_IM_RTIM);     // enable interupt on RX and RX transmission end
  UART0_IM_R  |= UART_IM_TXIM;                      // enable interrupt on TX
  UART0_CTL_R |= UART_CTL_UARTEN;                   // set UART0 enable bit    
}



/*
===================================================================================================
  USB_UART :: USB_UART_Enable_Interrupt
  
   - enables the UART0 interrupt
===================================================================================================
*/
void USB_UART_Enable_Interrupt(void){
  NVIC_EN0_R |= NVIC_EN0_INT5;
}

/*
===================================================================================================
  USB_UART :: USB_UART_DisableRXInterrupt
  
   - disables the UART0 RX interrupt
===================================================================================================
*/
void USB_UART_DisableRXInterrupt(void){
  NVIC_EN0_R &= ~NVIC_EN0_INT5;    
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
	
	
	RAW_INT_STAT=UART0_RIS_R;
	count_t++;
	top++;
	
	// RX FIFO >= 1/8 full 
  if(UART0_RIS_R & UART_RIS_RXRIS){       
		RAW_INT_STAT=UART0_RIS_R;							//debug raw int status register
    UART0_ICR_R = UART_ICR_RXIC;          // acknowledge interrupt
    USB_UART_HandleRXBuffer();            // copy from hardware RX FIFO to software RX FIFO
  }

	// receiver TIME-OUT
  if(UART0_RIS_R&UART_RIS_RTRIS){         
    RAW_INT_STAT=UART0_RIS_R;
		UART0_ICR_R = UART_ICR_RTIC;          // acknowledge receiver time
    USB_UART_HandleRXBuffer();            // copy from hardware RX FIFO to software RX FIFO
  }
 
	// hardware TX FIFO <= 2 items
  if(UART0_RIS_R&UART_RIS_TXRIS){         
		RAW_INT_STAT=UART0_RIS_R;
    UART0_ICR_R = UART_ICR_TXIC;          // acknowledge TX FIFO
		
  }
	
	RAW_INT_STAT=UART0_RIS_R;
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
	
  while (UART0_FR_R & UART_FR_TXFF) {}//spin when fifo is full until fifo is not full}; FR flag register
		
	for(int i=0;i<4;i++){	
		UART0_DR_R = input;
		trigger_pt++;
	}
}

/*
===================================================================================================
  USB_UART :: USB_UART_HandleRXBuffer
  
   - puts a character in the software FIFO, with a few special execptions
===================================================================================================
*/
void USB_UART_HandleRXBuffer(void){
  char letter;
  while((UART0_FR_R & UART_FR_RXFE) == 0){					// if UART Receive FIFO is not Empty (1 means empty)
    letter = UART0_DR_R; 

		
		/*
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
		*/
		
    USB_UART_PrintChar(letter);                     // echo typed character back to user terminal
		USB_BufferReady = true;    
		
		
		
		
  }
}

/*
===================================================================================================
  USB_UART :: USB_UART_HandleTXBuffer
  
   - does... something....
===================================================================================================
*/
void USB_UART_HandleTXBuffer(void){
  char letter;
  while(((UART0_FR_R&UART_FR_TXFF) == 0) && (TxFifo_Size() > 0)){ //while not full put copy from SW to HW fifo
		
    TxFifo_Get(&letter);
    UART0_DR_R = letter;
		
  }
}

