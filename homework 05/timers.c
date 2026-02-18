//==============================================================================
// File Name: timers.c
// Description: Timer initialization and interrupt service routines for HW05.
//              Duplicated from Project 04 per HW05 instructions.
//
//              Timer_B0 CCR0: fires every 5ms
//              SMCLK is set to 500kHz for HW05 scope demonstration.
//              500,000 Hz * 0.005 sec = 2,500 counts per 5ms interval
//              => 200 ticks = 1 second
//
// Global Variables Used:
//              Time_Sequence       - Master timer counter (defined here)
//              one_time            - One-time execution flag (defined here)
//              update_display      - LCD refresh flag (defined in LCD.obj)
//              update_display_count - LCD refresh counter (defined in LCD.obj)
//
// Author: Thomas Gilbert
// Date: Feb 2026
// Compiler: Code Composer Studio
//==============================================================================

#include "msp430.h"
#include "functions.h"
#include "macros.h"
#include "ports.h"

//==============================================================================
// Definitions (previously provided by Carlson's Timersb0.obj)
//==============================================================================
volatile unsigned int Time_Sequence = 0;
volatile char one_time = 0;

//==============================================================================
// External variables from LCD.obj — do NOT redefine
//==============================================================================
extern volatile unsigned char update_display;
extern volatile unsigned int update_display_count;

//==============================================================================
// Function: Init_Timers
// Description: Calls all individual timer initialization functions.
//              Add additional timer inits here as needed in future projects.
//
// Passed:    none
// Returns:   none
// Globals:   none
//==============================================================================
void Init_Timers(void){
    Init_Timer_B0();
}

//==============================================================================
// Function: Init_Timer_B0
// Description: Configures Timer_B0 CCR0 to generate an interrupt every 5ms.
//
//              SMCLK = 500 kHz (changed from 8MHz for HW05 scope demo)
//              TB0CCR0 = 2500
//              Interrupt period = 2500 / 500,000 = 0.005 sec = 5ms
//              200 interrupts per second => ONE_SEC = 200 ticks
//
//              Timer mode: Up mode (counts 0 to TB0CCR0, then resets)
//
// Passed:    none
// Returns:   none
// Globals:   none
//==============================================================================
void Init_Timer_B0(void){
    TB0CTL  = TBSSEL__SMCLK;        // Clock source = SMCLK (500 kHz for HW05)
    TB0CTL |= MC__UP;               // Up mode: count up to TB0CCR0
    TB0CTL |= TBCLR;                // Clear the timer counter

    TB0CCR0 = TB0CCR0_INTERVAL;     // Set compare value (defined in macros.h)
                                    // 2500 for 500kHz SMCLK -> 5ms interrupt

    TB0CCTL0 |= CCIE;               // Enable CCR0 compare interrupt
}

//==============================================================================
// ISR: Timer0_B0_ISR
// Description: Fires every 5ms (200 times per second).
//
//              Handles:
//              - Time_Sequence: master counter incremented every 5ms
//              - one_time: set every 50 ticks (250ms), for state machine use
//              - update_display: flags LCD refresh every 40 ticks (200ms)
//              - update_display_count: counter for LCD refresh timing
//
// Passed:    none (interrupt driven)
// Returns:   none
// Globals Modified: Time_Sequence, one_time, update_display, update_display_count
//==============================================================================
#pragma vector = TIMER0_B0_VECTOR
__interrupt void Timer0_B0_ISR(void){

    //--------------------------------------------------------------------------
    // Master time counter — incremented every 5ms
    //--------------------------------------------------------------------------
    Time_Sequence++;

    //--------------------------------------------------------------------------
    // one_time flag — set every 50 ticks (250ms)
    // Used by Carlson state machine for single-execution-per-window behavior
    //--------------------------------------------------------------------------
    if(Time_Sequence % 50 == 0){
        one_time = 1;
    }

    //--------------------------------------------------------------------------
    // Display refresh timing — update LCD every 40 ticks (200ms)
    //--------------------------------------------------------------------------
    update_display_count++;
    if(update_display_count >= 40){     // 40 ticks * 5ms = 200ms
        update_display_count = 0;
        update_display = 1;             // Signal Display_Process() to refresh
    }

    TB0CCTL0 &= ~CCIFG;                // Clear the interrupt flag
}
