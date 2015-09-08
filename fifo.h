// FIFO.h
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
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2014
      Programs 3.7, 3.8., 3.9 and 3.10 in Section 3.7

 Copyright 2014 by Jonathan W. Valvano, valvano@mail.utexas.edu
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

#ifndef __FIFO_H__
#define __FIFO_H__

#include <string.h>

long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value



// macro to create an index FIFO
#define AddIndexFifo(NAME,SIZE,TYPE,SUCCESS,FAIL) \
uint32_t volatile NAME ## PutI;    \
uint32_t volatile NAME ## GetI;    \
TYPE static NAME ## Fifo [SIZE];        \
void NAME ## Fifo_Init(void){ long sr;  \
  sr = StartCritical();                 \
  NAME ## PutI = NAME ## GetI = 0;      \
  EndCritical(sr);                      \
}                                       \
int NAME ## Fifo_Put (TYPE data){       \
  if(( NAME ## PutI - NAME ## GetI ) & ~(SIZE-1)){  \
    return(FAIL);      \
  }                    \
  NAME ## Fifo[ NAME ## PutI &(SIZE-1)] = data; \
  NAME ## PutI ## ++;  \
  return(SUCCESS);     \
}                      \
int NAME ## Fifo_Get (TYPE *datapt){  \
  if( NAME ## PutI == NAME ## GetI ){ \
    return(FAIL);      \
  }                    \
  *datapt = NAME ## Fifo[ NAME ## GetI &(SIZE-1)];  \
  NAME ## GetI ## ++;  \
  return(SUCCESS);     \
}                      \
unsigned short NAME ## Fifo_Size (void){  \
 return ((unsigned short)( NAME ## PutI - NAME ## GetI ));  \
}
// e.g.,
// AddIndexFifo(Tx,32,unsigned char, 1,0)
// SIZE must be a power of two
// creates TxFifo_Init() TxFifo_Get() and TxFifo_Put()

#define NUM_PTR 2							// get and put ptrs

// macro to create a pointer FIFO--------------------------------
#define AddPointerFifo(NAME,SIZE_DEPTH,SIZE_WIDTH,TYPE,SUCCESS,FAIL) \
TYPE volatile *NAME ## PutPt;    \
TYPE volatile *NAME ## GetPt;    \
TYPE volatile *NAME ## Fifo_Ptr [SIZE_DEPTH][NUM_PTR];        \
TYPE static NAME ## Fifo [SIZE_DEPTH][SIZE_WIDTH];        \
TYPE static *NAME ## Fifo_Level_Min;   \
TYPE static *NAME ## Fifo_Level_Max;   \
void NAME ## Fifo_Init(void){ long sr;  \
  sr = StartCritical();                 \
  NAME ## PutPt = NAME ## GetPt = &NAME ## Fifo[0][0]; \
	NAME ## Fifo_Level_Min=&NAME ## Fifo[0][0]; \
	NAME ## Fifo_Level_Max=&NAME ## Fifo[0][SIZE_WIDTH]; \
	EndCritical(sr);                      \
}                                       \
void NAME ## Fifo_New_Level(uint32_t Fifo_Level){ long sr;  \
  sr = StartCritical();                 \
	NAME ## PutPt = NAME ## GetPt = &NAME ## Fifo[Fifo_Level][0]; \
	NAME ## Fifo_Level_Min=&NAME ## Fifo[Fifo_Level][0];\
	NAME ## Fifo_Level_Max=&NAME ## Fifo[Fifo_Level][SIZE_WIDTH];	\
	EndCritical(sr);                      \
}                                       \
void NAME ## Fifo_Pointer_RST(void){ long sr;  \
  sr = StartCritical();                 \
  NAME ## PutPt = NAME ## GetPt = &NAME ## Fifo[0][0]; \
	EndCritical(sr);                      \
}  \
void NAME ## Fifo_Clear(void){ long sr;  \
  sr = StartCritical();                 \
	memset ( NAME ## Fifo, 0, SIZE_WIDTH*SIZE_DEPTH); \
	EndCritical(sr);                      \
}  																			\
int NAME ## Fifo_Put (TYPE data){       \
  TYPE volatile *nextPutPt;             \
  nextPutPt = NAME ## PutPt + 1;        \
  if(nextPutPt == NAME ## Fifo_Level_Max){ \
    nextPutPt = NAME ## Fifo_Level_Min;       \
  }                                     \
  if(nextPutPt == NAME ## GetPt ){      \
    return(FAIL);                       \
  }                                     \
  else{                                 \
    *( NAME ## PutPt ) = data;          \
    NAME ## PutPt = nextPutPt;          \
    return(SUCCESS);                    \
  }                                     \
}                                       \
int NAME ## Fifo_Pop (void) {           \
  if(NAME## PutPt <= NAME ## Fifo_Level_Min ){   \
    return(FAIL);                       \
  }                                     \
	*(--NAME ## PutPt) = 32;             \
  return(SUCCESS);                      \
}                                       \
int NAME ## Fifo_Shift_L (void) {       \
  if(NAME## PutPt <= NAME ## Fifo_Level_Min){   \
		return(FAIL);                       \
  }                                     \
  NAME ## PutPt--;            						\
  return(SUCCESS);                      \
}                                       \
int NAME ## Fifo_Shift_R (void) {       \
  if(NAME## PutPt == NAME ## Fifo_Level_Max ){   \
    return(FAIL);                       \
  }                                     \
  NAME ## PutPt++;					            \
  return(SUCCESS);                      \
}                                       \
int NAME ## Fifo_Get (TYPE *datapt){    \
  if( NAME ## PutPt == NAME ## GetPt ){ \
    return(FAIL);                       \
  }                                     \
  *datapt = *( NAME ## GetPt ## ++);    \
  if( NAME ## GetPt == NAME ## Fifo_Level_Max){ \
    NAME ## GetPt = NAME ## Fifo_Level_Min;   \
  }                                     \
  return(SUCCESS);                      \
}                                       \
unsigned short NAME ## Fifo_Size (void){\
  if( NAME ## PutPt < NAME ## GetPt ){  \
    return ((unsigned short)( NAME ## PutPt - NAME ## GetPt + (SIZE_WIDTH*sizeof(TYPE)))/sizeof(TYPE)); \
  }                                     \
  return ((unsigned short)( NAME ## PutPt - NAME ## GetPt )/sizeof(TYPE)); \
}
// e.g.,
// AddPointerFifo(Rx,32,unsigned char, 1,0)
// SIZE can be any size
// creates RxFifo_Init() RxFifo_Get() and RxFifo_Put()
// note that since PutPt and GetPt are being reused for each FIFO level
// Fifo_Size will return the size of the current FIFO level
#endif //  __FIFO_H__
