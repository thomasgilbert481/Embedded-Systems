//******************************************************************************
// File Name:    timers.c
// Description:  Timer initialization for Homework 06.
//               Configures Timer B0 in continuous mode for:
//                 CCR0 -- 200ms backlight blink + display update
//                 CCR1 -- SW1 interrupt-driven debounce
//                 CCR2 -- SW2 interrupt-driven debounce
//
//               Clock math (SMCLK = 8 MHz, ID__8, TBIDEX__8):
//                 Effective clock = 8,000,000 / 8 / 8 = 125,000 Hz
//                 TB0CCR0 = 125,000 / (1 / 0.200s) = 25,000 counts = 200ms
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

//==============================================================================
// Global definitions (previously provided by Carlson's timerB0.obj)
//==============================================================================
volatile unsigned int Time_Sequence = RESET_STATE;
volatile char         one_time      = FALSE;

//==============================================================================
// Function: Init_Timers
// Description: Calls individual timer initialization functions.
//              Add Init_Timer_B1/B2/B3 calls here if they are needed later.
//==============================================================================
void Init_Timers(void){
//------------------------------------------------------------------------------
// Globals used:    none
// Globals changed: none
//------------------------------------------------------------------------------
    Init_Timer_B0();
}

//==============================================================================
// Function: Init_Timer_B0
// Description: Configures Timer B0 in continuous mode with three capture/
//              compare registers:
//                CCR0 -- 200ms period for backlight toggle and display trigger
//                CCR1 -- SW1 software debounce (started on button press)
//                CCR2 -- SW2 software debounce (started on button press)
//
//              Timer source:  SMCLK = 8 MHz
//              ID divider:    /8  (ID__8)
//              TBIDEX divider: /8 (TBIDEX__8)
//              Effective clock: 8,000,000 / 8 / 8 = 125,000 Hz
//              CCR0 count:    125,000 / 5 = 25,000  =>  200ms interval
//
//              CRITICAL: TBCLR must be written AFTER ID and TBIDEX are set
//              so that all internal divider flip-flops are reset correctly.
//
// Globals used:    none
// Globals changed: none
// Local variables: none
//==============================================================================
void Init_Timer_B0(void){
//------------------------------------------------------------------------------

    TB0CTL  = TBSSEL__SMCLK;      // Clock source = SMCLK (8 MHz)
    TB0CTL |= MC__CONTINUOUS;     // Continuous mode: counts 0 -> 0xFFFF, repeats
    TB0CTL |= ID__8;              // Input divider: divide clock by 8
    TB0EX0  = TBIDEX__8;          // Additional divider: divide by 8
    TB0CTL |= TBCLR;              // Clear TB0R, clock divider, count direction
    // NOTE: TBCLR must come AFTER ID and TBIDEX to reset all divider flip-flops

    //--------------------------------------------------------------------------
    // CCR0 -- Backlight blink + display update (200ms interval)
    //--------------------------------------------------------------------------
    TB0CCR0  = TB0CCR0_INTERVAL;  // First interrupt at 25,000 counts (200ms)
    TB0CCTL0 &= ~CCIFG;           // Clear any pending CCR0 interrupt flag
    TB0CCTL0 |=  CCIE;            // Enable CCR0 interrupt

    //--------------------------------------------------------------------------
    // CCR1 -- SW1 debounce timer (disabled until SW1 is pressed)
    //--------------------------------------------------------------------------
    TB0CCR1  = TB0CCR1_INTERVAL;  // Load initial value (re-armed on SW1 press)
    TB0CCTL1 &= ~CCIFG;           // Clear any pending CCR1 interrupt flag
    TB0CCTL1 &= ~CCIE;            // Disable CCR1 interrupt (enabled in SW1 ISR)

    //--------------------------------------------------------------------------
    // CCR2 -- SW2 debounce timer (disabled until SW2 is pressed)
    //--------------------------------------------------------------------------
    TB0CCR2  = TB0CCR2_INTERVAL;  // Load initial value (re-armed on SW2 press)
    TB0CCTL2 &= ~CCIFG;           // Clear any pending CCR2 interrupt flag
    TB0CCTL2 &= ~CCIE;            // Disable CCR2 interrupt (enabled in SW2 ISR)

    //--------------------------------------------------------------------------
    // Overflow -- not used
    //--------------------------------------------------------------------------
    TB0CTL &= ~TBIE;              // Disable overflow interrupt
    TB0CTL &= ~TBIFG;             // Clear overflow interrupt flag
}
