//==============================================================================
// File Name: switches.c
// Description: Switch (button) handling for Project 6
//              SW1 — starts the black line detection sequence (P6 state machine)
//              SW2 — emergency stop (kills motors, resets both P5 and P6 state)
// Author: Thomas Gilbert
// Date: Mar 2026
// Compiler: Code Composer Studio
//==============================================================================

#include "msp430.h"
#include "functions.h"
#include "macros.h"
#include "ports.h"
#include <string.h>

//==============================================================================
// External variables
//==============================================================================
extern char display_line[4][11];
extern volatile unsigned char display_changed;

// Project 5 state variables (defined in main.c)
extern volatile unsigned int p5_timer;
extern volatile unsigned int p5_running;

// Project 6 state variables (defined in main.c)
extern volatile unsigned int p6_state;
extern volatile unsigned int p6_timer;
extern volatile unsigned int p6_running;
extern unsigned int ir_emitter_on;

//==============================================================================
// Debounce variables
//==============================================================================
static unsigned int sw1_debounce = 0;
static unsigned int sw2_debounce = 0;
#define DEBOUNCE_THRESHOLD  (500)

//==============================================================================
// Function: Switches_Process
// Description: Polls SW1 and SW2 with software debouncing.
//
//   SW1 (P4.1, active low):
//     - If Project 6 is idle, starts the P6 detection sequence (1-second delay
//       state, then forward movement toward the black line circle).
//
//   SW2 (P2.3, active low):
//     - Emergency stop: immediately kills all motors and resets both P5 and P6
//       state machines.  IR emitter is turned off.
//==============================================================================
void Switches_Process(void) {

    //--------------------------------------------------------------------------
    // SW1 — Start Project 6 black line detection sequence
    //--------------------------------------------------------------------------
    if (sw1_debounce > 0) {
        sw1_debounce--;
    } else {
        if (!(P4IN & SW1)) {                     // SW1 pressed? (active low)

            sw1_debounce = DEBOUNCE_THRESHOLD;

            // Only start if the P6 state machine is idle (not already running)
            if (p6_state == P6_IDLE) {
                p6_timer   = 0;                  // Reset the state machine timer
                p6_running = 1;                  // Enable timer counting in ISR
                p6_state   = P6_WAIT_1SEC;       // Enter 1-second pre-move delay

                strcpy(display_line[0], " Starting ");
                strcpy(display_line[1], "  1 sec   ");
                strcpy(display_line[2], "  delay   ");
                strcpy(display_line[3], "          ");
                display_changed = TRUE;
            }
        }
    }

    //--------------------------------------------------------------------------
    // SW2 — Emergency stop (kills everything)
    //--------------------------------------------------------------------------
    if (sw2_debounce > 0) {
        sw2_debounce--;
    } else {
        if (!(P2IN & SW2)) {                     // SW2 pressed? (active low)

            sw2_debounce = DEBOUNCE_THRESHOLD;

            // Immediately cut all motor outputs (never forward+reverse together)
            P6OUT &= ~R_FORWARD;
            P6OUT &= ~L_FORWARD;
            P6OUT &= ~R_REVERSE;
            P6OUT &= ~L_REVERSE;

            // Turn off IR emitter to save battery
            P2OUT &= ~IR_LED;
            ir_emitter_on = 0;

            // Reset Project 5 timer/running flags
            p5_running = 0;
            p5_timer   = 0;

            // Reset Project 6 state
            p6_state   = P6_IDLE;
            p6_running = 0;
            p6_timer   = 0;

            strcpy(display_line[0], "  STOPPED ");
            strcpy(display_line[1], "          ");
            strcpy(display_line[2], " Press SW1");
            strcpy(display_line[3], " to start ");
            display_changed = TRUE;
        }
    }
}
