//******************************************************************************
// File Name:    interrupt_ports.c
// Description:  Port interrupt service routines for Project 7.
//               Contains two ISRs:
//                 switch1_interrupt -- PORT4_VECTOR (SW1 on P4.1)
//                 switch2_interrupt -- PORT2_VECTOR (SW2 on P2.3)
//
//               SW1 behavior (Project 7):
//                 - P7_IDLE:          Start calibration sequence
//                 - P7_CALIBRATE with CAL_WAIT_BLACK: Advance to black sample
//                 - P7_WAIT_START:    Arm car (start 2-second countdown)
//                 - All other states: Ignored
//
//               SW2 behavior (Project 7):
//                 - Any state: Emergency stop -- motors off, IR off, reset to idle
//
//               After the debounce period (~1 second), TIMER0_B1_ISR re-enables
//               the switch interrupt and backlight timer automatically.
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
#include "adc.h"
#include <string.h>

//==============================================================================
// External variables
//==============================================================================
// From LCD.obj
extern char                   display_line[4][11];
extern volatile unsigned char display_changed;

// From interrupts_timers.c
extern volatile unsigned int  sw1_debounce_count;
extern volatile unsigned int  sw2_debounce_count;

// From main.c (Project 7 state machine)
extern volatile unsigned int  p7_state;
extern volatile unsigned int  p7_timer;
extern volatile unsigned int  p7_running;
extern volatile unsigned int  p7_timer_running;
extern volatile unsigned int  elapsed_tenths;
extern unsigned int           calibrate_phase;

//==============================================================================
// ISR: switch1_interrupt
// Description:  Port 4 interrupt for Switch 1 (P4.1, active low).
//               Disables SW1 for the debounce duration (~1 second via CCR1).
//               Turns off the LCD backlight and halts the backlight timer.
//
//               Project 7 actions:
//                 P7_IDLE:              Start calibration sequence
//                 P7_CALIBRATE (waiting for black placement): Advance to sample
//                 All other states:     Ignore SW1
//
// Interrupt Source:  Port 4 GPIO (SW1 = P4.1)
// Trigger:           High-to-low edge on P4.1
//
// Globals used:    p7_state, calibrate_phase
// Globals changed: sw1_debounce_count, p7_state, p7_timer, p7_running,
//                  calibrate_phase, display_line, display_changed
// States handled:  P7_IDLE, P7_CALIBRATE (CAL_WAIT_BLACK), P7_WAIT_START
// Local variables: none
//==============================================================================
#pragma vector = PORT4_VECTOR
__interrupt void switch1_interrupt(void){
//------------------------------------------------------------------------------

    if(P4IFG & SW1){                          // Confirm SW1 caused the interrupt

        P4IE   &= ~SW1;                       // 1. Disable SW1 interrupt
        P4IFG  &= ~SW1;                       // 2. Clear SW1 interrupt flag

        // Start CCR1 debounce timer
        sw1_debounce_count = RESET_STATE;     // 3. Reset debounce counter
        TB0CCTL1 &= ~CCIFG;                   // 4. Clear any pending CCR1 flag
        TB0CCR1   = TB0R + TB0CCR1_INTERVAL;  // 5. Schedule first CCR1 event
        TB0CCTL1 |=  CCIE;                    // 6. Enable CCR1 interrupt

        // Project 7 SW1 actions
        switch(p7_state){

            case P7_IDLE:
                // Start calibration from idle
                p7_timer        = RESET_STATE;
                p7_running      = TRUE;
                calibrate_phase = CAL_AMBIENT;
                p7_state        = P7_CALIBRATE;

                strcpy(display_line[0], "Calibrate ");
                strcpy(display_line[1], " Starting ");
                strcpy(display_line[2], "          ");
                strcpy(display_line[3], "          ");
                display_changed = TRUE;
                break;

            case P7_CALIBRATE:
                // If waiting for operator to place car on black tape, advance
                if(calibrate_phase == CAL_WAIT_BLACK){
                    p7_timer        = RESET_STATE;
                    calibrate_phase = CAL_BLACK;
                    // Display updated in Run_Project7 CAL_BLACK case
                }
                break;

            case P7_WAIT_START:
                // Operator has repositioned car; start 2-second armed countdown
                p7_timer = RESET_STATE;
                p7_state = P7_ARMED;
                // Display set on first entry of P7_ARMED case in Run_Project7
                display_changed = TRUE;
                break;

            default:
                // Ignore SW1 during active driving states
                break;
        }

        strcpy(display_line[3], "Switch 1  ");
        display_changed = TRUE;
    }
//------------------------------------------------------------------------------
}

//==============================================================================
// ISR: switch2_interrupt
// Description:  Port 2 interrupt for Switch 2 (P2.3, active low).
//               Emergency stop: kills all motors, turns off IR emitter,
//               resets the Project 7 state machine to idle.
//
// Interrupt Source:  Port 2 GPIO (SW2 = P2.3)
// Trigger:           High-to-low edge on P2.3
//
// Globals used:    none
// Globals changed: sw2_debounce_count, p7_state, p7_timer, p7_running,
//                  p7_timer_running, elapsed_tenths, calibrate_phase,
//                  ir_emitter_on, display_line, display_changed
// Local variables: none
//==============================================================================
#pragma vector = PORT2_VECTOR
__interrupt void switch2_interrupt(void){
//------------------------------------------------------------------------------

    if(P2IFG & SW2){                          // Confirm SW2 caused the interrupt

        P2IE   &= ~SW2;                       // 1. Disable SW2 interrupt
        P2IFG  &= ~SW2;                       // 2. Clear SW2 interrupt flag

        // Start CCR2 debounce timer
        sw2_debounce_count = RESET_STATE;     // 3. Reset debounce counter
        TB0CCTL2 &= ~CCIFG;                   // 4. Clear any pending CCR2 flag
        TB0CCR2   = TB0R + TB0CCR2_INTERVAL;  // 5. Schedule first CCR2 event
        TB0CCTL2 |=  CCIE;                    // 6. Enable CCR2 interrupt

        // Emergency stop -- kill all motor PWM outputs immediately
        Wheels_All_Off();

        // Turn off IR emitter to conserve power
        P2OUT        &= ~IR_LED;
        ir_emitter_on  = FALSE;

        // Reset Project 7 state machine to idle
        p7_state         = P7_IDLE;
        p7_running       = FALSE;
        p7_timer         = RESET_STATE;
        p7_timer_running = FALSE;
        elapsed_tenths   = RESET_STATE;
        calibrate_phase  = CAL_AMBIENT;

        strcpy(display_line[0], "  STOPPED ");
        strcpy(display_line[1], "          ");
        strcpy(display_line[2], " Press SW1");
        strcpy(display_line[3], " to start ");
        display_changed = TRUE;
    }
//------------------------------------------------------------------------------
}
