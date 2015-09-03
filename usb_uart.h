#ifndef USB_UART_H
#define USB_UART_H

#define FIFO_SIZE 64 					// need to sort this out for the rest of the files
#define SIZE_DEPTH   8        // size of the FIFOs (must be power of 2)
#define SIZE_WIDTH   64       // size of the FIFOs (must be power of 2)
#define FIFOSUCCESS 1         // return value on success
#define FIFOFAIL    0         // return value on failure

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
extern long Fifo_Depth;


#endif
