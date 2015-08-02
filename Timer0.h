#include <stdint.h>
// initialize timer0 based on sampling frequency
// and timer prescaler (8 bits are LSB bits in count down mode)
// Input: fs sampling frequency
//				prescaler timer prescaler.  new timer period=(prescaler+1)/fbus
// number of cylces to achieve fs is = fbus/(fs(prescaler+1))
// this function calculates the number of cycles automatically base on the inputs
// make sure that the prescaler is large enough to achieve the longest time
// example for 16-bit timer
// 2^16*(prescaler+1)/fbus=10.65ms so this is safe for fs=100 Hz
void Timer0_Init(unsigned int fsampling,uint32_t 	prescaler);
