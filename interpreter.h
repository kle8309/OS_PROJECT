#ifndef INTER_H
#define INTER_H

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "usb_uart.h"

void INTER_HandleBuffer(void);
//external dependencies
//error if place it here
//usb_uart.c(40): error:  #147-D: declaration is incompatible with "uint32_t RxFifo_Get(char *, long)"
//extern uint32_t RxFifo_Get(char* letter,uint32_t fifodepth);


#endif

