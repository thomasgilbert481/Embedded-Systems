//==============================================================================
// File: interrupts_timers.c
// Description: Timer B0 interrupt service routines for Project 8.
//
//   Timer0_B0_ISR (CCR0, every 200 ms):
//     - Sets update_display flag to trigger LCD refresh
//     - Advances Time_Sequence counter
//
//   TIMER0_B1_ISR (CCR1/CCR2):
//     - CCR1: SW1 debounce countdown -- re-enables SW1 interrupt after
//             DEBOUNCE_THRESHOLD x 200 ms (~1 second)
//     - CCR2: SW2 debounce countdown -- re-enables SW2 interrupt after
//             DEBOUNCE_THRESHOLD x 200 ms (~1 second)
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
// Global variables (declared extern in functions.h)
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

// From dac.c
extern volatile unsigned int DAC_data;
extern volatile unsigned int dac_startup_ticks;

//==============================================================================
// ISR: Timer0_B0_ISR
// Fires every 200 ms. Sets update_display flag and advances Time_Sequence.
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

    // Service the timed-vehicle-command countdown (auto-stop motors).
    Vehicle_Cmd_Tick();

    // Re-arm CCR0 for next 200 ms interrupt
    TB0CCR0 += TB0CCR0_INTERVAL;
}

//==============================================================================
// ISR: TIMER0_B1_ISR
// Handles CCR1 (SW1 debounce) and CCR2 (SW2 debounce).
// After DEBOUNCE_THRESHOLD ticks, disables the CCR interrupt and
// re-enables the port interrupt so the switch can be pressed again.
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
            TB0CCR1 += TB0CCR1_INTERVAL;      // Re-arm CCR1 for next 200 ms
            break;

        case 4:                               // CCR2 -- SW2 debounce timer
            sw2_debounce_count++;
            if(sw2_debounce_count >= DEBOUNCE_THRESHOLD){
                TB0CCTL2  &= ~CCIE;           // Disable CCR2 interrupt
                P2IFG     &= ~SW2;            // Clear any pending SW2 flag
                P2IE      |=  SW2;            // Re-enable SW2 port interrupt
            }
            TB0CCR2 += TB0CCR2_INTERVAL;      // Re-arm CCR2 for next 200 ms
            break;

        case 14:                              // Timer overflow -- DAC enable + ramp
            // Phase 1: wait DAC_ENABLE_TICKS overflows before enabling buck-boost.
            //   This lets the DAC output settle before the LT1935 sees it.
            // Phase 2: decrement DAC_data by DAC_RAMP_STEP each tick.
            //   Lower DAC value = higher motor supply voltage (inverted).
            //   Stop when DAC_data <= DAC_Limit: set DAC_Adjust, clear TBIE.
            if(!(P2OUT & DAC_ENB)){
                // Phase 1 -- settling delay
                dac_startup_ticks++;
                if(dac_startup_ticks >= DAC_ENABLE_TICKS){
                    P2OUT |= DAC_ENB;         // Enable buck-boost converter
                    P1OUT |= RED_LED;         // RED LED ON -- ramp starting
                }
            } else {
                // Phase 2 -- ramp down toward motor operating voltage
                if(DAC_data >= (DAC_Limit + DAC_RAMP_STEP)){
                    DAC_data -= DAC_RAMP_STEP;
                } else {
                    DAC_data = DAC_Adjust;
                }
                SAC3DAT = DAC_data;
                if(DAC_data <= DAC_Limit){
                    DAC_data = DAC_Adjust;
                    SAC3DAT  = DAC_data;
                    TB0CTL  &= ~TBIE;         // Disable overflow interrupt
                    P1OUT   &= ~RED_LED;      // RED LED OFF -- ramp done
                }
            }
            break;

        default:
            break;
    }
}
