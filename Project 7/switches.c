//******************************************************************************
// File Name:    switches.c
// Description:  Switch handling stub for Homework 06.
//               Switch 1 (P4.1) and Switch 2 (P2.3) are now handled entirely
//               by interrupt service routines in interrupt_ports.c:
//                 switch1_interrupt -- PORT4_VECTOR
//                 switch2_interrupt -- PORT2_VECTOR
//
//               Interrupt-driven debouncing replaces the software polling loop
//               that was used in Project 6.  Switches_Process() is retained as
//               an empty stub to avoid breaking any remaining call sites.
//
// Author:       Thomas Gilbert
// Date:         Mar 2026
// Compiler:     Code Composer Studio
// Target:       MSP430FR2355
//******************************************************************************

#include "msp430.h"
#include "functions.h"
#include "macros.h"
#include "ports.h"
#include <string.h>

//==============================================================================
// External variables
//==============================================================================
extern char display_line[4][11];
extern volatile unsigned char display_changed;

// Project 6 state variables (defined in main.c)
extern volatile unsigned int p6_state;
extern volatile unsigned int p6_timer;
extern volatile unsigned int p6_running;

//==============================================================================
// Function: Switches_Process
// Description: Software switch polling stub.
//              In HW06 switch debouncing is fully interrupt-driven via
//              interrupt_ports.c (PORT4/PORT2 ISRs) and interrupts_timers.c
//              (CCR1/CCR2 debounce timers).  This function is intentionally
//              empty; the switch ISRs handle all button press logic.
//
// Globals used:    none
// Globals changed: none
// Local variables: none
//==============================================================================
void Switches_Process(void){
//------------------------------------------------------------------------------
    // Switch handling is now interrupt-driven -- see interrupt_ports.c
//------------------------------------------------------------------------------
}
