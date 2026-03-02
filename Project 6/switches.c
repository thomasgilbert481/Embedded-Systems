//==============================================================================
// File Name: switches.c
// Description: Switch (button) handling for Project 5
//              SW1 starts the forward/reverse/spin sequence
//              SW2 is emergency stop
// Author: Thomas Gilbert
// Date: Feb 2026
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
extern unsigned int p5_step;
extern char p5_started;
extern char step_init;
extern volatile unsigned int p5_timer;
extern volatile unsigned int p5_running;

//==============================================================================
// Debounce variables
//==============================================================================
static unsigned int sw1_debounce = 0;
static unsigned int sw2_debounce = 0;
#define DEBOUNCE_THRESHOLD  (500)

//==============================================================================
// Function: Switches_Process
//==============================================================================
void Switches_Process(void) {

    //--------------------------------------------------------------------------
    // SW1 Processing — Starts the Project 5 sequence
    //--------------------------------------------------------------------------
    if (sw1_debounce > 0) {
        sw1_debounce--;
    } else {
        if (!(P4IN & SW1)) {                    // SW1 pressed? (active low)

            sw1_debounce = DEBOUNCE_THRESHOLD;

            //------------------------------------------------------------------
            // Only start if not already running
            //------------------------------------------------------------------
            if (!p5_started) {
                p5_step = P5_FWD1;              // Start at first movement step
                p5_started = 1;                 // Tell main loop to run sequence
                step_init = 1;                  // First entry into the step
                p5_timer = 0;                   // Reset the timer
                p5_running = 0;                 // Will be set by Run_Project5
            }
        }
    }

    //--------------------------------------------------------------------------
    // SW2 Processing — Emergency stop
    //--------------------------------------------------------------------------
    if (sw2_debounce > 0) {
        sw2_debounce--;
    } else {
        if (!(P2IN & SW2)) {                    // SW2 pressed? (active low)

            sw2_debounce = DEBOUNCE_THRESHOLD;

            // Emergency stop: kill ALL motors (forward AND reverse)
            P6OUT &= ~R_FORWARD;
            P6OUT &= ~L_FORWARD;
            P6OUT &= ~R_REVERSE;                // NEW: also kill reverse motors
            P6OUT &= ~L_REVERSE;                // NEW: also kill reverse motors

            // Reset Project 5 state
            p5_started = 0;
            p5_running = 0;
            p5_step = 0;

            strcpy(display_line[0], "  STOPPED ");
            strcpy(display_line[1], "          ");
            strcpy(display_line[2], " Press SW1");
            strcpy(display_line[3], " to start ");
            display_changed = TRUE;
        }
    }
}
