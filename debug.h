// Zed's Awesome Debug Macros (modified)
// Original: http://c.learncodethehardway.org/book/ex20.html

#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <errno.h>
#include <string.h>

#define BOB

#define PF1 (*((volatile uint32_t *)0x40025008)) // red
#define PF2 (*((volatile uint32_t *)0x40025010)) // blue
#define PF3 (*((volatile uint32_t *)0x40025020)) // green

#ifdef BOB
    #define debug(M, ...) printf("DEBUG %s:%d: " M "\n")
    #define debug_err(M, ...) printf("[ERROR] (%s:%d: errno: %s) " M "\n")
    #define debug_warn(M, ...) printf("[WARN] (%s:%d: errno: %s) " M "\n")
    #define debug_info(M, ...) printf("[INFO] (%s:%d) " M "\n")
		#define debug_ledToggle(M) M ^= 0xFF
    #define debug_ledOn(M) M |= 0xFF
    #define debug_ledOff(M) M &= 0x00
#else
    #define debug(M, ...)
    #define debug_err(M, ...)
    #define debug_warn(M, ...)
    #define debug_info(M, ...)
  	#define debug_ledToggle(M)
    #define debug_ledOn(M)
    #define debug_ledOff(M)
#endif

void DEBUG_Init(void);



#endif
