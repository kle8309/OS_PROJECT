#ifndef USB_UART_H
#define USB_UART_H

#define FIFO_SIZE 64

#include "stdint.h"
#include "stdbool.h"

extern bool USB_BufferReady;

void USB_UART_Init(void);
void USB_UART_PrintChar(char iput);
void USB_UART_Enable_Interrupt(void);
void USB_UART_DisableRXInterrupt(void);
void USB_UART_HandleRXBuffer(void);
void USB_UART_HandleTXBuffer(void);

void UART0_Handler(void);

#endif
