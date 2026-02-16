//==============================================================================
// File Name: switches.c
// Description: Switch handling for Homework 4 — Motor Test Point Verification
//              SW1 cycles through motor on/off test steps
//              SW2 emergency stop — turns all motors off
// Author: Thomas Gilbert
// Date: Feb 16, 2026
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
// Debounce variables
//==============================================================================
static unsigned int sw1_debounce = 0;
static unsigned int sw2_debounce = 0;
#define DEBOUNCE_THRESHOLD  (500)

//==============================================================================
// Function: Switches_Process
// Description: SW1 advances motor test step, SW2 emergency stop
//==============================================================================
void Switches_Process(void) {

    //--------------------------------------------------------------------------
    // SW1 — Advance to next motor test step
    //--------------------------------------------------------------------------
    if (sw1_debounce > 0) {
        sw1_debounce--;
    } else {
        if (!(P4IN & SW1)) {
            sw1_debounce = DEBOUNCE_THRESHOLD;
            Motor_Test_Advance();
        }
    }

    //--------------------------------------------------------------------------
    // SW2 — Emergency stop: all motors off immediately
    //--------------------------------------------------------------------------
    if (sw2_debounce > 0) {
        sw2_debounce--;
    } else {
        if (!(P2IN & SW2)) {
            sw2_debounce = DEBOUNCE_THRESHOLD;

            Motors_Off();

            strcpy(display_line[0], "  E-STOP  ");
            strcpy(display_line[1], " ALL OFF  ");
            strcpy(display_line[2], "          ");
            strcpy(display_line[3], "SW1=reset ");
            display_changed = TRUE;
        }
    }
}
