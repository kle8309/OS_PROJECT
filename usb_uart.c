#include "tm4c123gh6pm.h"
#include "interpreter.h"
#include "fifo.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "usb_uart.h"
#include <string.h>


/*
========================================================================================================================
==========                                             CONSTANTS                                              ==========
========================================================================================================================
*/



/*
========================================================================================================================
==========                                          GLOBAL VARIABLES                                          ==========
========================================================================================================================
*/

bool USB_BufferReady = false;

/*
========================================================================================================================
==========                                         FIFO Objects 					                                    ==========
========================================================================================================================
*/

// create index implementation FIFO (see FIFO.h)
AddPointerFifo(Rx, SIZE_DEPTH,SIZE_WIDTH, TYPE, FIFOSUCCESS, FIFOFAIL)
AddPointerFifo(Tx, SIZE_DEPTH,SIZE_WIDTH, TYPE, FIFOSUCCESS, FIFOFAIL)

AddPointerFifo(Work,WORK_FIFO_LEVEL,SIZE_WIDTH, TYPE, FIFOSUCCESS, FIFOFAIL)// FYI: WORK_FIFO_LEVEL=1
TYPE volatile *WorkPutPt_Max;	// for storing temporary max index

/*
===================================================================================================
  USB_UART :: USB_UART_Init
  
   - initializes the UART to use PA0,PA1 at 115200 baud
===================================================================================================
*/
void USB_UART_Init(void){
  TxFifo_Init();
  RxFifo_Init();
	// working fifo
	WorkFifo_Init();
  
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

	USB_UART_PrintChar(12); 				 //echo to terminal
  printf("Guardian Terminal v1.0\r\n");
	printf("@user >> ");
	WorkPutPt_Max=WorkPutPt; //init max to current work ptr

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

uint32_t Current_Fifo_Level = 0;
uint32_t Next_Fifo_Level = 0;



void USB_UART_HandleRXBuffer(void){
		char letter;
		
	  //count++; //uart interrupt count DEBUG
	
		while((UART0_FR_R & UART_FR_RXFE) == 0){					// if UART Receive FIFO is not Empty (1 means empty)
				letter = UART0_DR_R; 														// copy from RX HW fifo to memory
				//USB_UART_PrintChar(letter);                     // echo typed character back to user terminal
				
				// cmd Entered************************************************
			//**************************************************************
				if (letter =='\r') {   
					
						USB_UART_PrintChar(letter); 				 //echo to terminal
						USB_UART_PrintChar('\n'); 				   //echo to terminal	unless set to implicit form feed \n for \r		
					
						// don't put \r in sw fifo
						// \r indicates user has pressed Enter key

						/*
						//copy temporary fifo to free fifo level
						if(key==DOWN_ARROW_KEY || key==UP_ARROW_KEY){
								key=INVALID_KEY; // reset key to default state
							// if we edit over the the termination then we will need new termination
							// else the old termination is still valid		
						}	else{
							 // new cmd always need termination
							 WorkFifo_Put(0);                  // 0 null terminate buffer
						}
						*/					
						if(WorkFifo[0][0]!='\0'){
							
							
							
							
								// new cmd detected when working put ptr is beyond previously saved put ptr
								// Next_Fifo_Level index can range from the first index up to the Current_Fifo_Level
								// Current_Fifo_Level always points to the next available buffer index in RxFifo to save data		


//bug here workput ptr is not greater than next at first cmd so it goes to else and update the wrong max
								//if(WorkPutPt>RxFifo_Ptr [Next_Fifo_Level][PUT_PTR]){	
							//TODO: might not need to terminate with NULL since default is NULL for all elements
							//when wrap around we should reset array to null before overwriting
								if(WorkPutPt>WorkPutPt_Max){
										WorkFifo_Put(0);          												  // 0 null terminate buffer
									  //"string\0"_ after put the put ptr is the the right of null terminator so we subtract 1
										WorkPutPt_Max=WorkPutPt-1;
									//RxFifo_Ptr [Current_Fifo_Level][PUT_PTR]=WorkPutPt_Max; // save new put pointer
									
								}else{
									// the max index is updated else where (backspace handler code)									
									// RxFifo_Ptr [Current_Fifo_Level][PUT_PTR]=RxFifo_Ptr [Next_Fifo_Level][PUT_PTR];  
									//RxFifo_Ptr [Current_Fifo_Level][PUT_PTR]=WorkPutPt_Max;
								}

								RxFifo_Ptr [Current_Fifo_Level][PUT_PTR]=WorkPutPt_Max;
								
								
								
								// save workingFifo GET pointer to RxFifo_Ptr
								// TODO: need to update the get ptr to pt towards the fifo array (not array elements)
								// one cmd per array
								RxFifo_Ptr [Current_Fifo_Level][GET_PTR]=WorkGetPt;
											
								// save WorkFifo buffer to RxFifo buffer (this has the cmd)
								// rewrite this as a save function for readability
								memcpy ( RxFifo[Current_Fifo_Level],WorkFifo, SIZE_WIDTH );
								printf("Saved %d/%d: '%s'\n\r",Current_Fifo_Level+1,SIZE_DEPTH,RxFifo[Current_Fifo_Level]);
								// move get and put pointers to next fifo level
								// Current_Fifo_Level is index of the fifo level that is free
								RxFifo_New_Level(++Current_Fifo_Level);
								
								//Fifo_Depth will be at least 1 at this point
								//b/c the increment
								if(Current_Fifo_Level==SIZE_DEPTH){										// fifo wraparound when greater than max fifo depth
									Current_Fifo_Level=0; 
									Next_Fifo_Level=SIZE_DEPTH;
								}	
								//TODO: prob don't need >0 condition
								// just the statement is enough? Next_Fifo_Level=Current_Fifo_Level;-----------------------------------
								else if(Current_Fifo_Level>0){
									Next_Fifo_Level=Current_Fifo_Level; // reset next index to current fifo index
																											// this takes care of the history lookup issue
																											// when the Next_Fifo_Level index is not modified by
																											// history lookup function on the previous call
								} else{
									//last_Fifo=0;
								}
						}//end if empty string
						
						// TODO: reset workingFifo pointers to 0 position 
						WorkFifo_Pointer_RST();
						WorkPutPt_Max=WorkPutPt;
						// wipe Fifo clean
						//void * memset ( void * ptr, int value, size_t num );
						//memset ( WorkFifo[0], 0, SIZE_WIDTH);
						WorkFifo_Clear();
						
						key=INVALID_KEY; // reset key to default state
					                 // toggle buffer processing semaphore
						printf("@user >> ");		// ready for next cmd			
						USB_BufferReady = true;      
					
				} else if (letter == 12) {    // ctrl-L is ASCII 12, form feed
						USB_UART_PrintChar(letter); 				 //echo to terminal	
						// reset workingFifo pointers to 0 position 
						WorkFifo_Pointer_RST();	
						WorkPutPt_Max=WorkPutPt;					
						// wipe Fifo clean
						//memset ( WorkFifo[0], 0, SIZE_WIDTH);
					  WorkFifo_Clear();
						key=INVALID_KEY; // reset key to default state
						printf("@user >> ");		// ready for next cmd			

				} else if (letter == 127) {              // handle backspace*********************************************
						if(WorkPutPt>WorkFifo_Level_Min){
								USB_UART_PrintChar(letter); 		 //echo to terminal	
								
								//dtm if at end of string
								if(*WorkPutPt=='\0'){
										// pop with null terminator
										WorkFifo_Pop(0); 
										//WorkFifo_Pop(NULL); 
										//WorkFifo_Pop('\0'); 
									  // save this ptr
										WorkPutPt_Max=WorkPutPt;
										//printf("\n\r@work >> %s",WorkFifo[0]);
								}else{
										// pop with ascii spacebar 
										WorkFifo_Pop(32);
									  //printf("\n\r@work >> %s",WorkFifo[0]);
								}
						}
						
				}	else {
					// START ESCAPE SEQUENCE CODE
					// 27 91 'D' or 'C' or 'B' or 'A'
					// 				Left   Right  Down   Up
			
					//First code filter to reduce bandwidth (i.e. this is not used as often as other keys)
					//if first letter is not the starter code we can skip the rest
					// on non-zero read_counter letter is allowed to have number other than 27
						if(letter != 27 && read_counter == 0){
							
							
								//****************************************************************
								// 																			handle char
								USB_UART_PrintChar(letter); 				 //echo to terminal
								// TODO:
							// change put function to put to temporary fifo
							// only when return key is pressed when we actually add to permanent fifo	
								WorkFifo_Put(letter);       // if not one of the escape codes put char in temporary working fifo
																										 // 'ESC' '[' follow by 'A' 'B' 'C' 'D' are not supported as commands
																										 // these code are ignored in SW FIFO
																										 // if need to support for some odd reason, a patch is needed
								if(WorkPutPt>WorkPutPt_Max){  //this code is need for intial unsaved state where right arrow key is used
									WorkPutPt_Max=WorkPutPt;    // save new max
									//RxFifo_Ptr [Current_Fifo_Level][PUT_PTR]=WorkPutPt_Max; //used Current instead of Next just in case enter key is not pressed
																																			//bc during query and editing mode it can overite the previous pointer
																																			//note:this code pairs with the right arrow code below
																																			//the backspace pairs witht he left arrow code below
																																			//notice how the "arrow key" conditions below is related to the pointer saved here
								}		
						} else{
							
							Decode_ESC_SEQ(letter);
						}
						// END ESCAPE SEQUENCE CODE					
	
				} // END letter condition
				
				
		} //end HW FIFO read while
				
} //end RX handle

// decode escape sequence function
uint32_t volatile get_offset;
uint32_t volatile put_offset;
uint32_t volatile cursor_pos;

void Decode_ESC_SEQ(char letter){

		if (letter == 27 && read_counter == 0){
		
				read_counter++;														// first match												
																										
		} else if (letter == 91 && read_counter == 1){ 	// move on to second code
			// if first and second code in sequence are correct
				read_counter++;	// second match
																																																	
		}	else if (read_counter == 2 )	{ 						// third and last code check
																									// previous two codes matched escape sequence
																									// reset counter to prevent false validation due to previous valid state	
			read_counter=0; 
			switch(letter){
				case 68:
					key=LEFT_ARROW_KEY; 							 //----LEFT----------
					if(WorkPutPt>WorkFifo_Level_Min){
						WorkFifo_Shift_L();	
						USB_UART_PrintChar(27); 				 //echo to terminal
						USB_UART_PrintChar(91); 				 //echo to terminal
						USB_UART_PrintChar(68); 				 //echo to terminal

					}
				  //cursor_pos= WorkPutPt-&WorkFifo[0][0];
				//TODO: add code to move fifo pointer similar to backspace code
					break;
				case 67:															//----RIHGT----------

					key=RIGHT_ARROW_KEY;
				  // force user to use spacebar to delimit string args
				  // or else we need to support multi string parsing (multi null \0)
				  //
				  // next is different from current only when down or up keys is pressed
				  //
				
				  if(WorkPutPt<WorkPutPt_Max){
							WorkFifo_Shift_R();
							USB_UART_PrintChar(27); 				 //echo to terminal
							USB_UART_PrintChar(91); 				 //echo to terminal
							USB_UART_PrintChar(67); 				 //echo to terminal
					}
				
				/*
				  if(WorkPutPt<RxFifo_Ptr [Next_Fifo_Level][PUT_PTR] || WorkPutPt<RxFifo_Ptr [Current_Fifo_Level][PUT_PTR]){
							WorkFifo_Shift_R();
							USB_UART_PrintChar(27); 				 //echo to terminal
							USB_UART_PrintChar(91); 				 //echo to terminal
							USB_UART_PrintChar(67); 				 //echo to terminal
					}
					*/
				//TODO: add code to move fifo pointer similar to backspace code
					break;
				case 66:							
					key=DOWN_ARROW_KEY;									//----DOWN----------
					//TODO: add code.  Need 2D SW fifo array
				  if(Next_Fifo_Level>0){ 
						Next_Fifo_Level--;
				  }else if (Current_Fifo_Level==0){
							break; // don't reset to max index if at initial state
					}else{
							//next == 0 and current !=0 so we reset to max index "current-1" for wrap-around
							Next_Fifo_Level=Current_Fifo_Level-1; 
					}

					// void * memcpy ( void * destination, const void * source, size_t num );
					// copy current cmd selection to temporary fifo
				  memcpy ( WorkFifo, RxFifo[Next_Fifo_Level], SIZE_WIDTH );

					// TODO: load workingFifo pointers according to RxFifo pointers *RELATIVE* positions
					
					put_offset=(RxFifo_Ptr [Next_Fifo_Level][PUT_PTR]-&WorkFifo[0][0]);
					
					get_offset=(RxFifo_Ptr [Next_Fifo_Level][GET_PTR]-&WorkFifo[0][0]);
					WorkPutPt = &WorkFifo[0][0]+put_offset;
					WorkGetPt = &WorkFifo[0][0]+get_offset;
					
					WorkPutPt_Max=WorkPutPt; // load max at load time i.e. end of string index
					
					//reference: http://www.termsys.demon.co.uk/vtansi.htm
					//<ESC>[2K Erases the entire current line.
					printf("%c[2K\r",0x1B);//extra \r to cover the case where cursor is not at 0
																						
					printf("@user >> %s",WorkFifo[0]); //print previous command
					// if we don't decrement the put ptr we need to print out the termination so
					// that it will match the terminal cursor position
					//printf("%s ",WorkFifo[0]); //print previous command
					break;
				case 65:														
					key=UP_ARROW_KEY;											//----UP----------
					//TODO:need to rectify this -1 bug at initial state where this condition is always true (0-1=2^32-1)
					if(Next_Fifo_Level<Current_Fifo_Level-1 && Current_Fifo_Level!=0 ){ // "Current_Fifo_Level!=0" => avoid query at initial state
																																							// doesn't support query when current fifo index wraps around
																																							// history of query reset when current fifo index wraps
																																							// perhaps will support in future release																																						
						Next_Fifo_Level++;
				  }else if(Current_Fifo_Level==0){
						break; // don't reset to min index if at initial state
					}else{
							Next_Fifo_Level=0; // wrap around
					}
					
					
					
					
					// copy current cmd selection to temporary fifo
				  memcpy ( WorkFifo, RxFifo[Next_Fifo_Level], SIZE_WIDTH );
					
					// TODO: load workingFifo pointers according to RxFifo pointers *RELATIVE* positions
					put_offset=(RxFifo_Ptr [Next_Fifo_Level][PUT_PTR]-&WorkFifo[0][0]); 
					get_offset=(RxFifo_Ptr [Next_Fifo_Level][GET_PTR]-&WorkFifo[0][0]);
					WorkPutPt = &WorkFifo[0][0]+put_offset;
					WorkGetPt = &WorkFifo[0][0]+get_offset;
					
					WorkPutPt_Max=WorkPutPt; // save max at load time i.e. end of string index
					
					//<ESC>[2K Erases the entire current line.
					printf("%c[2K\r",0x1B);//extra \r to cover the case where cursor is not at 0
																									
					printf("@user >> %s",WorkFifo[0]); //print previous command
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

