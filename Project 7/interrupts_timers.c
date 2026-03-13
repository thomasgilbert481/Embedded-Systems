//******************************************************************************
// File Name:    interrupts_timers.c
// Description:  Timer B0 interrupt service routines for Project 7.
//               Contains two ISRs:
//                 Timer0_B0_ISR -- CCR0, fires every 200ms
//                 TIMER0_B1_ISR -- CCR1, CCR2, and overflow (shared vector)
//
//               Project 7 additions to CCR0 ISR:
//                 - p7_timer: incremented when p7_running is TRUE
//                 - elapsed_tenths: incremented when p7_timer_running is TRUE
//                   (each 200ms tick = 0.2s = 1 elapsed_tenth)
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

// From main.c (Project 7 state machine timing)
extern volatile unsigned int p7_timer;          // P7 state delay counter
extern volatile unsigned int p7_running;        // TRUE = ISR increments p7_timer

// From main.c (Project 7 display clock)
extern volatile unsigned int elapsed_tenths;    // 0.2-second display counter
extern volatile unsigned int p7_timer_running;  // TRUE = ISR increments elapsed_tenths

//==============================================================================
// ISR: Timer0_B0_ISR
// Description:  Timer B0 CCR0 interrupt -- fires every 200ms.
//               Toggles LCD backlight, signals display update, advances the
//               Time_Sequence counter, and increments Project 7 timers.
//
//               Each tick = 200ms = 0.2 seconds.
//               p7_timer counts 200ms ticks for state machine delays.
//               elapsed_tenths counts 0.2-second display increments.
//
// Interrupt Source:  Timer B0 CCR0
// Trigger:           TB0R reaches TB0CCR0 value (in continuous mode)
//
// Globals used:    Time_Sequence, p7_running, p7_timer, p7_timer_running,
//                  elapsed_tenths
// Globals changed: Time_Sequence, one_time, update_display, p7_timer,
//                  elapsed_tenths
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

    //--------------------------------------------------------------------------
    // Project 7 state machine timer -- only counts when sequence is active
    // Incremented every 200ms tick when p7_running is TRUE.
    //--------------------------------------------------------------------------
    if(p7_running){
        p7_timer++;
    }

    //--------------------------------------------------------------------------
    // Project 7 display clock (0.2-second resolution)
    // Each ISR tick IS 200ms = 0.2s, so one elapsed_tenth per tick.
    // Only increments when p7_timer_running is TRUE (car is moving).
    //--------------------------------------------------------------------------
    if(p7_timer_running){
        elapsed_tenths++;
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
