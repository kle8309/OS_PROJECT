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
//extern uint32_t RxFifo_Get(char*);

#endif
