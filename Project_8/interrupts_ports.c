//==============================================================================
// File Name: interrupt_ports.c
// Description: Port interrupt service routines for Homework 8.
//
//   switch1_interrupt (PORT4_VECTOR, SW1 = P4.1):
//     Sets sw1_pressed flag -> main loop switches to 115,200 baud.
//
//   switch2_interrupt (PORT2_VECTOR, SW2 = P2.3):
//     Sets sw2_pressed flag -> main loop switches to 460,800 baud.
//
//   Both switches use Timer B0 CCR1/CCR2 for ~1-second debounce.
//   TIMER0_B1_ISR (interrupts_timers.c) re-enables the switch interrupt
//   after DEBOUNCE_THRESHOLD ticks.
//
// Author: Thomas Gilbert
// Date: Mar 2026
// Compiler: Code Composer Studio
// Target: MSP430FR2355
//==============================================================================

#include "msp430.h"
#include "functions.h"
#include "macros.h"
#include "ports.h"
#include <string.h>

//==============================================================================
// External variables
//==============================================================================
// From interrupts_timers.c
extern volatile unsigned int sw1_debounce_count;
extern volatile unsigned int sw2_debounce_count;

// From main.c (switch press flags, processed in main loop)
extern volatile unsigned int sw1_pressed;
extern volatile unsigned int sw2_pressed;

//==============================================================================
// ISR: switch1_interrupt
// SW1 pressed (P4.1 high-to-low edge).
// Sets sw1_pressed flag -- main loop calls Set_Baud_Rate(BAUD_115200).
//==============================================================================
#pragma vector = PORT4_VECTOR
__interrupt void switch1_interrupt(void){

    if(P4IFG & SW1){
        P4IE   &= ~SW1;                       // Disable SW1 interrupt
        P4IFG  &= ~SW1;                       // Clear SW1 flag

        // Start CCR1 debounce timer
        sw1_debounce_count = RESET_STATE;
        TB0CCTL1 &= ~CCIFG;
        TB0CCR1   = TB0R + TB0CCR1_INTERVAL;
        TB0CCTL1 |=  CCIE;

        // Signal main loop to switch to 115,200 baud
        sw1_pressed = TRUE;
    }
}

//==============================================================================
// ISR: switch2_interrupt
// SW2 pressed (P2.3 high-to-low edge).
// Sets sw2_pressed flag -- main loop calls Set_Baud_Rate(BAUD_460800).
//==============================================================================
#pragma vector = PORT2_VECTOR
__interrupt void switch2_interrupt(void){

    if(P2IFG & SW2){
        P2IE   &= ~SW2;                       // Disable SW2 interrupt
        P2IFG  &= ~SW2;                       // Clear SW2 flag

        // Start CCR2 debounce timer
        sw2_debounce_count = RESET_STATE;
        TB0CCTL2 &= ~CCIFG;
        TB0CCR2   = TB0R + TB0CCR2_INTERVAL;
        TB0CCTL2 |=  CCIE;

        // Signal main loop to switch to 460,800 baud
        sw2_pressed = TRUE;
    }
}
