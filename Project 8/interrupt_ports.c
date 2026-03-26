//==============================================================================
// File: interrupt_ports.c
// Description: Port interrupt service routines for Project 8.
//
//   switch1_interrupt (PORT4_VECTOR, SW1 = P4.1):
//     Sets sw1_pressed flag → main loop calls Serial_Transmit(command_to_send)
//     and updates LCD to "Transmit  ".
//
//   switch2_interrupt (PORT2_VECTOR, SW2 = P2.3):
//     Sets sw2_pressed flag → main loop cycles baud rate and reinitializes UCA0.
//
//   Both ISRs immediately disable their respective port interrupt and start the
//   Timer B0 CCR1/CCR2 debounce countdown. TIMER0_B1_ISR (interrupts_timers.c)
//   re-enables the port interrupt after DEBOUNCE_THRESHOLD × 200 ms (~1 second).
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

//==============================================================================
// External variables
//==============================================================================
// Debounce counters (defined in interrupts_timers.c)
extern volatile unsigned int sw1_debounce_count;
extern volatile unsigned int sw2_debounce_count;

// Switch press flags (defined in main.c, processed in main loop)
extern volatile unsigned int sw1_pressed;
extern volatile unsigned int sw2_pressed;

//==============================================================================
// ISR: switch1_interrupt
// SW1 pressed (P4.1 high-to-low edge).
//   - Disables SW1 interrupt and starts CCR1 debounce timer
//   - Sets sw1_pressed flag; main loop calls Serial_Transmit(command_to_send)
//==============================================================================
#pragma vector = PORT4_VECTOR
__interrupt void switch1_interrupt(void){

    if(P4IFG & SW1){
        P4IE   &= ~SW1;   // 1. Disable SW1 interrupt to prevent bounce retriggering
        P4IFG  &= ~SW1;   // 2. Clear the interrupt flag

        // Start CCR1 debounce timer
        sw1_debounce_count = RESET_STATE;        // 3. Reset debounce counter
        TB0CCTL1 &= ~CCIFG;                      // 4. Clear any pending CCR1 flag
        TB0CCR1   = TB0R + TB0CCR1_INTERVAL;     // 5. Schedule first CCR1 event
        TB0CCTL1 |=  CCIE;                       // 6. Enable CCR1 interrupt

        // Signal main loop to transmit the stored command
        sw1_pressed = TRUE;
    }
}

//==============================================================================
// ISR: switch2_interrupt
// SW2 pressed (P2.3 high-to-low edge).
//   - Disables SW2 interrupt and starts CCR2 debounce timer
//   - Sets sw2_pressed flag; main loop cycles baud rate and reinits UCA0
//==============================================================================
#pragma vector = PORT2_VECTOR
__interrupt void switch2_interrupt(void){

    if(P2IFG & SW2){
        P2IE   &= ~SW2;   // 1. Disable SW2 interrupt
        P2IFG  &= ~SW2;   // 2. Clear the interrupt flag

        // Start CCR2 debounce timer
        sw2_debounce_count = RESET_STATE;        // 3. Reset debounce counter
        TB0CCTL2 &= ~CCIFG;                      // 4. Clear any pending CCR2 flag
        TB0CCR2   = TB0R + TB0CCR2_INTERVAL;     // 5. Schedule first CCR2 event
        TB0CCTL2 |=  CCIE;                       // 6. Enable CCR2 interrupt

        // Signal main loop to cycle baud rate
        sw2_pressed = TRUE;
    }
}
