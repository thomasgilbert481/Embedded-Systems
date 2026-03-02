//==============================================================================
// File Name: timers.c
// Description: Timer initialization and interrupt service routines for Project 5.
//              This file REPLACES the precompiled Timersb0.obj from Carlson.
//              Project 5 requires: "Time must be from interrupts. You cannot
//              use any existing functions provided from me."
//
//              Timer_B0 CCR0: fires every 5ms (8MHz SMCLK / 40000 = 200 Hz)
//                             => 200 ticks = 1 second
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
// External variables used by the ISR
//==============================================================================
extern volatile unsigned int Time_Sequence;      // Master timer (for display, Carlson SM)
extern volatile char one_time;                   // One-time execution flag
extern volatile unsigned char update_display;    // Flag to refresh LCD
extern volatile unsigned int update_display_count; // Counter for display refresh rate

// Project 5 timing
extern volatile unsigned int p5_timer;           // Movement timing counter
extern volatile unsigned int p5_running;         // Flag: is P5 timer active?

// Defined HERE (were previously in Carlson's timer .obj)
volatile unsigned int Time_Sequence = 0;
volatile char one_time = 0;
// Defined in LCD.obj — do NOT redefine, just extern
extern volatile unsigned char update_display;
extern volatile unsigned int update_display_count;

//==============================================================================
// Function: Init_Timers
// Description: Calls all individual timer init functions.
//==============================================================================
void Init_Timers(void){
    Init_Timer_B0();
    // Init_Timer_B1();  // Add these if you need them later
    // Init_Timer_B2();
    // Init_Timer_B3();
}

//==============================================================================
// Function: Init_Timer_B0
// Description: Configures Timer_B0 CCR0 to generate an interrupt every 5ms.
//
//              SMCLK = 8 MHz
//              TB0CCR0 = 40000
//              Interrupt period = 40000 / 8,000,000 = 0.005 sec = 5ms
//              That means 200 interrupts per second => ONE_SEC = 200
//
//              Timer mode: Up mode (counts from 0 to TB0CCR0, then resets)
//==============================================================================
void Init_Timer_B0(void){
    TB0CTL  = TBSSEL__SMCLK;    // Clock source = SMCLK (8 MHz)
    TB0CTL |= MC__UP;           // Up mode: count to TB0CCR0
    TB0CTL |= TBCLR;            // Clear the timer counter

    TB0CCR0 = TB0CCR0_INTERVAL; // Set the compare value (defined in macros.h)

    TB0CCTL0 |= CCIE;          // Enable CCR0 interrupt
}

//==============================================================================
// ISR: Timer0_B0_ISR
// Description: Fires every 5ms (200 times per second).
//              Handles:
//              - Time_Sequence: master counter for display timing and Carlson SM
//              - update_display: flags LCD refresh every ~200ms
//              - p5_timer: Project 5 movement timing (only when p5_running == 1)
//              - one_time: reset each time Time_Sequence changes by 50
//==============================================================================
#pragma vector = TIMER0_B0_VECTOR
__interrupt void Timer0_B0_ISR(void){

    //--------------------------------------------------------------------------
    // Master time counter (keeps existing behavior from Carlson's code)
    //--------------------------------------------------------------------------
    Time_Sequence++;

    // one_time flag: allows the Carlson state machine to run once per 50-tick window
    // (kept for compatibility, not critical for Project 5)
    if (Time_Sequence % 50 == 0){
        one_time = 1;
    }

    //--------------------------------------------------------------------------
    // Display update timing — refresh LCD every ~200ms (every 40 ticks at 5ms each)
    //--------------------------------------------------------------------------
    update_display_count++;
    if(update_display_count >= 40){     // 40 * 5ms = 200ms between LCD refreshes
        update_display_count = 0;
        update_display = 1;             // Tell Display_Process() to refresh
    }

    //--------------------------------------------------------------------------
    // Project 5 movement timer — only counts when a movement is active
    //--------------------------------------------------------------------------
    if(p5_running){
        p5_timer++;                     // Incremented every 5ms
                                        // ONE_SEC (200) ticks = 1.0 second
                                        // TWO_SEC (400) ticks = 2.0 seconds
                                        // THREE_SEC (600) ticks = 3.0 seconds
    }

    TB0CCTL0 &= ~CCIFG;                // Clear the interrupt flag
}
