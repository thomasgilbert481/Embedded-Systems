//==============================================================================
// File: timers.c
// Description: Timer initialization for Project 8 -- Serial Communication.
//              Configures Timer B0 in continuous mode for:
//                CCR0 -- 200 ms display update tick
//                CCR1 -- SW1 interrupt-driven debounce
//                CCR2 -- SW2 interrupt-driven debounce
//
//              No Timer B3 (no motor PWM needed in Project 8).
//
//              Clock math (SMCLK = 8 MHz, ID__8, TBIDEX__8):
//                Effective clock = 8,000,000 / 8 / 8 = 125,000 Hz
//                TB0CCR0_INTERVAL = 25,000 counts = 200 ms
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
    Init_Timer_B3();    // Hardware PWM for motor control
}

//==============================================================================
// Function: Init_Timer_B3
// Description: Configures Timer B3 for hardware PWM on motor pins (P6.1-P6.4).
//              Up mode: counts from 0 to TB3CCR0 = WHEEL_PERIOD_VAL.
//              All four motor channels use output mode 7 (reset/set):
//                pin HIGH at period start, LOW when CCR count reached.
//              Setting a CCR to WHEEL_OFF (0) keeps the output LOW.
//==============================================================================
void Init_Timer_B3(void){
    TB3CTL  = TBSSEL__SMCLK;          // Clock source = SMCLK (8 MHz), no divider
    TB3CTL |= MC__UP;                 // Up mode: count 0 -> TB3CCR0
    TB3CTL |= TBCLR;                  // Clear timer counter and dividers

    TB3CCR0 = WHEEL_PERIOD_VAL;       // PWM period

    // CCR1 (P6.1) -- not wired to any H-bridge on this PCB; still configured
    // as OUTMOD_7 for consistency and kept at WHEEL_OFF.
    TB3CCTL1 = OUTMOD_7;              // P6.1 (unused on this car)
    TB3CCR1  = WHEEL_OFF;

    // CCR2 (P6.2) -- LEFT_FORWARD
    TB3CCTL2 = OUTMOD_7;
    LEFT_FORWARD_SPEED  = WHEEL_OFF;

    // CCR3 (P6.3) -- RIGHT_FORWARD (empirical)
    TB3CCTL3 = OUTMOD_7;
    RIGHT_FORWARD_SPEED = WHEEL_OFF;

    // CCR4 (P6.4) -- LEFT_REVERSE
    TB3CCTL4 = OUTMOD_7;
    LEFT_REVERSE_SPEED  = WHEEL_OFF;

    // CCR5 (P6.5) -- RIGHT_REVERSE on this car (TB3.5 PWM)
    TB3CCTL5 = OUTMOD_7;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;
}

//==============================================================================
// Function: Init_Timer_B0
// Description: Configures Timer B0 in continuous mode.
//   CCR0 -- 200 ms period for display update
//   CCR1 -- SW1 debounce (started on button press by ISR)
//   CCR2 -- SW2 debounce (started on button press by ISR)
//
//   Timer source:    SMCLK = 8 MHz
//   ID divider:      /8
//   TBIDEX divider:  /8
//   Effective:       125,000 Hz
//   CCR0 count:      25,000 => 200 ms interval
//==============================================================================
void Init_Timer_B0(void){
    TB0CTL  = TBSSEL__SMCLK;      // Clock source = SMCLK (8 MHz)
    TB0CTL |= MC__CONTINUOUS;     // Continuous mode: counts 0 -> 0xFFFF
    TB0CTL |= ID__8;              // Input divider: /8
    TB0EX0  = TBIDEX__8;          // Additional divider: /8
    TB0CTL |= TBCLR;              // Clear TB0R and dividers

    // CCR0 -- Display update (200 ms)
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

    // Overflow -- not used in Project 8
    TB0CTL &= ~TBIE;
    TB0CTL &= ~TBIFG;
}
