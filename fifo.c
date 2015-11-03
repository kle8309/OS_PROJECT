// FIFO.c
// Runs on any LM3Sxxx
// Provide functions that initialize a FIFO, put data in, get data out,
// and return the current size.  The file includes a transmit FIFO
// using index implementation and a receive FIFO using pointer
// implementation.  Other index or pointer implementation FIFOs can be
// created using the macros supplied at the end of the file.
// Daniel Valvano
// June 16, 2011

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to the Arm Cortex M3",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2011
   Programs 3.7, 3.8., 3.9 and 3.10 in Section 3.7

 Copyright 2011 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

#include "FIFO.h"
#include <stdint.h>

// Two-pointer implementation of the receive FIFO
// can hold 0 to RXFIFOSIZE-1 elements
#define RXFIFOSIZE 10 // can be any size
#define RXFIFOSUCCESS 1
#define RXFIFOFAIL    0

typedef char rxDataType;
rxDataType volatile *RxPutPt; // put next
rxDataType volatile *RxGetPt; // get next
rxDataType static RxFifo[RXFIFOSIZE];


#define FIFO_SIZE 	 		64 			  // need to sort this out for the rest of the files
#define SIZE_DEPTH   		16       	// size of the FIFOs 
#define SIZE_WIDTH   		64       	// size of the FIFOs 
#define GET_PTR 		 		0  				// index of get pointer within fifo buffer 
#define PUT_PTR 		 		1  				// index of put pointer within fifo buffer 
#define WORK_FIFO_LEVEL 1     		// temporary fifo buffer has only 1 level 
#define FIFOSUCCESS 		1         // return value on success
#define FIFOFAIL    		0         // return value on failure
#define TYPE char									// type def for fifo elements used in sw fifo init

typedef struct {
	
	uint8_t volatile * p_put; //
	uint8_t volatile * p_get; //
	uint8_t          * fifo_level_min; // address to fifo min   
	uint8_t          * fifo_level_max; // address to fifo max
	
} fifo_t;

// TODO KL:
// *********************************************************************************************
void Fifo_Init (fifo_t * p_fifo)
{ 
	uint8_t volatile * p_array [SIZE_DEPTH][NUM_PTR]; // pointers for each array row  
	uint8_t static     fifo[SIZE_DEPTH][SIZE_WIDTH];  // fixed-size 2d array 
	fifo_t  
	long sr;  
  sr = StartCritical();                 
  p_fifo->p_put = p_get = &NAME ## Fifo[0][0]; 
	NAME ## Fifo_Level_Min=&NAME ## Fifo[0][0]; 
	NAME ## Fifo_Level_Max=&NAME ## Fifo[0][SIZE_WIDTH]; 
	EndCritical(sr);                      
}   


// initialize pointer FIFO
void RxFifo_Init(void){ long sr;
  sr = StartCritical();      // make atomic
  RxPutPt = RxGetPt = &RxFifo[0]; // Empty
  EndCritical(sr);
}
// add element to end of pointer FIFO
// return RXFIFOSUCCESS if successful
int RxFifo_Put(rxDataType data){
  rxDataType volatile *nextPutPt;
  nextPutPt = RxPutPt+1;
  if(nextPutPt == &RxFifo[RXFIFOSIZE]){
    nextPutPt = &RxFifo[0];  // wrap
  }
  if(nextPutPt == RxGetPt){
    return(RXFIFOFAIL);      // Failed, fifo full
  }
  else{
    *(RxPutPt) = data;       // Put
    RxPutPt = nextPutPt;     // Success, update
    return(RXFIFOSUCCESS);
  }
}
// remove element from front of pointer FIFO
// return RXFIFOSUCCESS if successful
int RxFifo_Get(rxDataType *datapt){
  if(RxPutPt == RxGetPt ){
    return(RXFIFOFAIL);      // Empty if PutPt=GetPt
  }
  *datapt = *(RxGetPt++);
  if(RxGetPt == &RxFifo[RXFIFOSIZE]){
     RxGetPt = &RxFifo[0];   // wrap
  }
  return(RXFIFOSUCCESS);
}
// number of elements in pointer FIFO
// 0 to RXFIFOSIZE-1
unsigned short RxFifo_Size(void){
  if(RxPutPt < RxGetPt){
    return ((unsigned short)(RxPutPt-RxGetPt+(RXFIFOSIZE*sizeof(rxDataType)))/sizeof(rxDataType));
  }
  return ((unsigned short)(RxPutPt-RxGetPt)/sizeof(rxDataType));
}
