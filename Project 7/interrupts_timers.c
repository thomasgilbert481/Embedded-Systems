//******************************************************************************
// File Name:    interrupts_timers.c
// Description:  Timer B0 interrupt service routines for Homework 06.
//               Contains two ISRs:
//                 Timer0_B0_ISR -- CCR0, fires every 200ms
//                 TIMER0_B1_ISR -- CCR1, CCR2, and overflow (shared vector)
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
// Global variables defined here (accessed externally via extern declarations)
//==============================================================================
volatile unsigned int sw1_debounce_count = RESET_STATE;  // SW1 debounce counter
volatile unsigned int sw2_debounce_count = RESET_STATE;  // SW2 debounce counter

//==============================================================================
// External variables
//==============================================================================
// From timers.c (were previously in Carlson's timerB0.obj)
extern volatile unsigned int Time_Sequence;
extern volatile char         one_time;

// From LCD.obj (display subsystem)
extern volatile unsigned char update_display;

// From main.c (Project 6 state machine)
extern volatile unsigned int p6_timer;
extern volatile unsigned int p6_running;

//==============================================================================
// ISR: Timer0_B0_ISR
// Description:  Timer B0 CCR0 interrupt -- fires every 200ms.
//               Toggles LCD backlight, signals display update, advances the
//               Time_Sequence counter, and increments the Project 6 timer.
//
// Interrupt Source:  Timer B0 CCR0
// Trigger:           TB0R reaches TB0CCR0 value (in continuous mode)
//
// Globals used:    Time_Sequence, p6_running, p6_timer
// Globals changed: Time_Sequence, one_time, update_display, p6_timer
// Local variables: none
//==============================================================================
#pragma vector = TIMER0_B0_VECTOR
__interrupt void Timer0_B0_ISR(void){
//------------------------------------------------------------------------------

    // Toggle LCD backlight (creates 2.5 Hz blink -- 200ms on, 200ms off)
    P6OUT ^= LCD_BACKLITE;

    // Signal the main loop that it is time to refresh the display
    update_display = TRUE;

    // Advance the master time counter; wrap to prevent overflow
    if(Time_Sequence >= TIME_SEQ_MAX){
        Time_Sequence = RESET_STATE;
    } else {
        Time_Sequence++;
    }

    // one_time flag: fires once per TIME_SEQ_MAX window (used by legacy SM)
    if(Time_Sequence == RESET_STATE){
        one_time = TRUE;
    }

    // Project 6 state machine timer -- only counts when the sequence is active
    if(p6_running){
        p6_timer++;
    }

    // Re-arm CCR0 for the next interrupt (continuous mode requires manual re-arm)
    TB0CCR0 += TB0CCR0_INTERVAL;
//------------------------------------------------------------------------------
}

//==============================================================================
// ISR: TIMER0_B1_ISR
// Description:  Timer B0 CCR1, CCR2, and overflow interrupt -- shared vector.
//               Disambiguated using TB0IV.
//
//               CCR1 (case 2): SW1 debounce countdown.
//                 After DEBOUNCE_THRESHOLD interrupts (~1 second), re-enables
//                 the SW1 port interrupt and the CCR0 backlight interrupt.
//
//               CCR2 (case 4): SW2 debounce countdown.
//                 Same as CCR1 but for SW2 (Port 2 / P2.3).
//
// Interrupt Source:  Timer B0 CCR1, CCR2, or Overflow
// Trigger:           TB0R reaches TB0CCR1 / TB0CCR2, or timer overflows
//
// Globals used:    sw1_debounce_count, sw2_debounce_count, DEBOUNCE_THRESHOLD
// Globals changed: sw1_debounce_count, sw2_debounce_count
// Local variables: none
//==============================================================================
#pragma vector = TIMER0_B1_VECTOR
__interrupt void TIMER0_B1_ISR(void){
//------------------------------------------------------------------------------

    switch(__even_in_range(TB0IV, 14)){

        case 0:                               // No pending interrupt
            break;

        case 2:                               // CCR1 -- SW1 debounce timer
            sw1_debounce_count++;
            if(sw1_debounce_count >= DEBOUNCE_THRESHOLD){
                TB0CCTL1  &= ~CCIE;           // Disable CCR1 interrupt
                P4IFG     &= ~SW1;            // Clear any pending SW1 flag
                P4IE      |=  SW1;            // Re-enable SW1 port interrupt
                TB0CCTL0  |=  CCIE;           // Re-enable backlight (CCR0) interrupt
            }
            TB0CCR1 += TB0CCR1_INTERVAL;      // Re-arm CCR1 for next 200ms
            break;

        case 4:                               // CCR2 -- SW2 debounce timer
            sw2_debounce_count++;
            if(sw2_debounce_count >= DEBOUNCE_THRESHOLD){
                TB0CCTL2  &= ~CCIE;           // Disable CCR2 interrupt
                P2IFG     &= ~SW2;            // Clear any pending SW2 flag
                P2IE      |=  SW2;            // Re-enable SW2 port interrupt
                TB0CCTL0  |=  CCIE;           // Re-enable backlight (CCR0) interrupt
            }
            TB0CCR2 += TB0CCR2_INTERVAL;      // Re-arm CCR2 for next 200ms
            break;

        case 14:                              // Timer overflow -- not used
            break;

        default:
            break;
    }
//------------------------------------------------------------------------------
}
