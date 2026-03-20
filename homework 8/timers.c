//==============================================================================
// File Name: timers.c
// Description: Timer initialization for Homework 8.
//              Configures Timer B0 in continuous mode for:
//                CCR0 -- 200ms display update + splash/transmit timing
//                CCR1 -- SW1 interrupt-driven debounce
//                CCR2 -- SW2 interrupt-driven debounce
//
//              No Timer B3 (no motor PWM needed in HW8).
//
//              Clock math (SMCLK = 8 MHz, ID__8, TBIDEX__8):
//                Effective clock = 8,000,000 / 8 / 8 = 125,000 Hz
//                TB0CCR0 = 125,000 / 5 = 25,000 counts = 200ms
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
// Global definitions
//==============================================================================
volatile unsigned int Time_Sequence = RESET_STATE;
volatile char         one_time      = FALSE;

//==============================================================================
// Function: Init_Timers
//==============================================================================
void Init_Timers(void){
    Init_Timer_B0();
}

//==============================================================================
// Function: Init_Timer_B0
// Description: Configures Timer B0 in continuous mode.
//   CCR0 -- 200ms period for display update and timing
//   CCR1 -- SW1 debounce (started on button press)
//   CCR2 -- SW2 debounce (started on button press)
//
//   Timer source:   SMCLK = 8 MHz
//   ID divider:     /8
//   TBIDEX divider: /8
//   Effective:      125,000 Hz
//   CCR0 count:     25,000 => 200ms interval
//==============================================================================
void Init_Timer_B0(void){
    TB0CTL  = TBSSEL__SMCLK;      // Clock source = SMCLK (8 MHz)
    TB0CTL |= MC__CONTINUOUS;     // Continuous mode: counts 0 -> 0xFFFF
    TB0CTL |= ID__8;              // Input divider: /8
    TB0EX0  = TBIDEX__8;          // Additional divider: /8
    TB0CTL |= TBCLR;              // Clear TB0R and dividers

    // CCR0 -- Display update (200ms)
    TB0CCR0  = TB0CCR0_INTERVAL;
    TB0CCTL0 &= ~CCIFG;
    TB0CCTL0 |=  CCIE;

    // CCR1 -- SW1 debounce (disabled until SW1 pressed)
    TB0CCR1  = TB0CCR1_INTERVAL;
    TB0CCTL1 &= ~CCIFG;
    TB0CCTL1 &= ~CCIE;

    // CCR2 -- SW2 debounce (disabled until SW2 pressed)
    TB0CCR2  = TB0CCR2_INTERVAL;
    TB0CCTL2 &= ~CCIFG;
    TB0CCTL2 &= ~CCIE;

    // Overflow -- not used in HW8
    TB0CTL &= ~TBIE;
    TB0CTL &= ~TBIFG;
}
