//==============================================================================
// File Name: switches.c
// Description: Switch processing stub for Project 7.
//              Switch handling is fully interrupt-driven via interrupt_ports.c:
//                SW1 -- interrupt_ports.c switch1_interrupt (PORT4_VECTOR)
//                SW2 -- interrupt_ports.c switch2_interrupt (PORT2_VECTOR)
//              This file remains as a stub to satisfy the function call in main.c.
// Author: Thomas Gilbert
// Date: Mar 2026
// Compiler: Code Composer Studio
//==============================================================================

#include "msp430.h"
#include "functions.h"
#include "macros.h"
#include "ports.h"
#include <string.h>

//==============================================================================
// External variables (defined in main.c)
//==============================================================================
extern volatile unsigned int p7_state;
extern volatile unsigned int p7_timer;
extern volatile unsigned int p7_running;
extern unsigned int          ir_emitter_on;

//==============================================================================
// Function: Switches_Process
// Description: Stub -- switch handling is done in interrupt_ports.c ISRs.
//              Kept for compatibility with main.c call site.
//==============================================================================
void Switches_Process(void){
    // All switch logic handled by interrupt-driven ISRs in interrupt_ports.c
}
