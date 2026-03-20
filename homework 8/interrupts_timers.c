//==============================================================================
// File Name: interrupts_timers.c
// Description: Timer B0 interrupt service routines for Homework 8.
//
//   Timer0_B0_ISR (CCR0, every 200ms):
//     - Sets update_display flag
//     - Advances Time_Sequence
//     - Increments splash_timer until splash is done
//     - Increments transmit_wait while transmit is pending
//
//   TIMER0_B1_ISR (CCR1/CCR2):
//     - CCR1: SW1 debounce countdown
//     - CCR2: SW2 debounce countdown
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
// Global variables
//==============================================================================
volatile unsigned int sw1_debounce_count = RESET_STATE;
volatile unsigned int sw2_debounce_count = RESET_STATE;

//==============================================================================
// External variables
//==============================================================================
// From timers.c
extern volatile unsigned int Time_Sequence;
extern volatile char         one_time;

// From LCD.obj
extern volatile unsigned char update_display;

// From main.c (splash and transmit timing)
extern volatile unsigned int splash_timer;
extern unsigned int          splash_done;
extern volatile unsigned int transmit_wait;
extern volatile unsigned int transmit_pending;

//==============================================================================
// ISR: Timer0_B0_ISR
// Fires every 200ms. Signals display update, advances timing counters.
//==============================================================================
#pragma vector = TIMER0_B0_VECTOR
__interrupt void Timer0_B0_ISR(void){

    update_display = TRUE;

    if(Time_Sequence >= TIME_SEQ_MAX){
        Time_Sequence = RESET_STATE;
    } else {
        Time_Sequence++;
    }

    if(Time_Sequence == RESET_STATE){
        one_time = TRUE;
    }

    // Advance splash timer until splash screen is dismissed
    if(!splash_done){
        splash_timer++;
    }

    // Count up while waiting to transmit after a baud rate change
    if(transmit_pending){
        transmit_wait++;
    }

    // Re-arm CCR0 for next 200ms interrupt
    TB0CCR0 += TB0CCR0_INTERVAL;
}

//==============================================================================
// ISR: TIMER0_B1_ISR
// Handles CCR1 (SW1 debounce) and CCR2 (SW2 debounce).
//==============================================================================
#pragma vector = TIMER0_B1_VECTOR
__interrupt void TIMER0_B1_ISR(void){

    switch(__even_in_range(TB0IV, 14)){

        case 0:                               // No pending interrupt
            break;

        case 2:                               // CCR1 -- SW1 debounce timer
            sw1_debounce_count++;
            if(sw1_debounce_count >= DEBOUNCE_THRESHOLD){
                TB0CCTL1  &= ~CCIE;           // Disable CCR1 interrupt
                P4IFG     &= ~SW1;            // Clear any pending SW1 flag
                P4IE      |=  SW1;            // Re-enable SW1 port interrupt
            }
            TB0CCR1 += TB0CCR1_INTERVAL;      // Re-arm CCR1 for next 200ms
            break;

        case 4:                               // CCR2 -- SW2 debounce timer
            sw2_debounce_count++;
            if(sw2_debounce_count >= DEBOUNCE_THRESHOLD){
                TB0CCTL2  &= ~CCIE;           // Disable CCR2 interrupt
                P2IFG     &= ~SW2;            // Clear any pending SW2 flag
                P2IE      |=  SW2;            // Re-enable SW2 port interrupt
            }
            TB0CCR2 += TB0CCR2_INTERVAL;      // Re-arm CCR2 for next 200ms
            break;

        case 14:                              // Overflow -- not used in HW8
            break;

        default:
            break;
    }
}
