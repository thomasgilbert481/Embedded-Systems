//==============================================================================
// File Name: switches.c
// Description: Switch (button) handling for HW05.
//              Duplicated from Project 04 per HW05 instructions.
//
//              SW1 (P4.1): Reconfigures P3.4 as SMCLK output (USE_SMCLK)
//              SW2 (P2.3): Reconfigures P3.4 as GP I/O pin  (USE_GPIO)
//
//              This verifies the two Init_Port3() argument cases required
//              by HW05, observable on the oscilloscope at the SMCLK test point.
//
// Global Variables Used:
//              display_line     - LCD display buffer (defined in main.c)
//              display_changed  - LCD update flag (defined in main.c)
//
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

//==============================================================================
// Debounce counters — local to this file
// Prevents a single button press from registering multiple times.
// Each counter is loaded with DEBOUNCE_THRESHOLD on press and
// must count down to 0 before another press is recognized.
//==============================================================================
static unsigned int sw1_debounce = 0;
static unsigned int sw2_debounce = 0;

//==============================================================================
// Function: Switches_Process
// Description: Called from the main loop. Checks SW1 and SW2 state with
//              software debouncing and calls Init_Port3() with the appropriate
//              argument to toggle P3.4 between SMCLK output and GP I/O.
//
//              SW1 (P4.1, active low) - Init_Port3(USE_SMCLK)
//                  P3.4 outputs SMCLK signal (visible on scope at SMCLK test point)
//
//              SW2 (P2.3, active low) - Init_Port3(USE_GPIO)
//                  P3.4 reverts to standard GP I/O output (no clock signal)
//
// Passed:    none
// Returns:   none
// Globals Modified: display_line, display_changed
//==============================================================================
void Switches_Process(void) {

    //--------------------------------------------------------------------------
    // SW1 Processing — Reconfigure P3.4 as SMCLK output
    //--------------------------------------------------------------------------
    if(sw1_debounce > 0){
        sw1_debounce--;
    } else {
        if(!(P4IN & SW1)){                      // SW1 pressed (active low)
            sw1_debounce = DEBOUNCE_THRESHOLD;

            Init_Port3(USE_SMCLK);              // P3.4 -> SMCLK output

            strcpy(display_line[0], "  SMCLK ");
            strcpy(display_line[1], " OUTPUT ");
            strcpy(display_line[2], " 500kHz ");
            strcpy(display_line[3], "  P3.4  ");
            display_changed = TRUE;
        }
    }

    //--------------------------------------------------------------------------
    // SW2 Processing — Reconfigure P3.4 as GP I/O
    //--------------------------------------------------------------------------
    if(sw2_debounce > 0){
        sw2_debounce--;
    } else {
        if(!(P2IN & SW2)){                      // SW2 pressed (active low)
            sw2_debounce = DEBOUNCE_THRESHOLD;

            Init_Port3(USE_GPIO);               // P3.4 -> GP I/O

            strcpy(display_line[0], "  GPIO  ");
            strcpy(display_line[1], "  MODE  ");
            strcpy(display_line[2], "  P3.4  ");
            strcpy(display_line[3], "        ");
            display_changed = TRUE;
        }
    }
}
