#ifndef USB_UART_H
#define USB_UART_H

#define FIFO_SIZE 	 		64 			  // need to sort this out for the rest of the files
#define SIZE_DEPTH   		16       	// size of the FIFOs 
#define SIZE_WIDTH   		64       	// size of the FIFOs 
#define GET_PTR 		 		0  				// index of get pointer within fifo buffer 
#define PUT_PTR 		 		1  				// index of put pointer within fifo buffer 
#define WORK_FIFO_LEVEL 1     		// temporary fifo buffer has only 1 level 
#define FIFOSUCCESS 		1         // return value on success
#define FIFOFAIL    		0         // return value on failure
#define TYPE char									// type def for fifo elements used in sw fifo init

#include "stdint.h"
#include "stdbool.h"
#include "fifo.h"

extern bool USB_BufferReady;

void USB_UART_Init(void);
void USB_UART_PrintChar(char iput);
void USB_UART_Enable_Interrupt(void);
void USB_UART_DisableRXInterrupt(void);
void USB_UART_HandleRXBuffer(void);
void USB_UART_HandleTXBuffer(void);

void UART0_Handler(void);


/*
========================================================================================================================
==========                                         USB UART FUNCTIONS                                         ==========
========================================================================================================================
*/
extern uint32_t Current_Fifo_Level;


#endif
