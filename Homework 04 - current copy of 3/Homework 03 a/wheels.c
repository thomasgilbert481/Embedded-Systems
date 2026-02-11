//==============================================================================
// File Name: wheels.c
// Description: Wheel motor control functions
// Author: Thomas
// Date: Feb 6, 2026
// Compiler: Code Composer Studio
//==============================================================================

#include "msp430.h"
#include "functions.h"
#include "macros.h"
#include "ports.h"

// External variables
extern volatile unsigned char one_time;
extern volatile unsigned int Time_Sequence;
extern char display_line[4][11];
extern volatile unsigned char display_changed;

//==============================================================================
// Function: Wheels_Forward
// Description: Turn on both wheels to move forward
// Sets L_FORWARD and R_FORWARD motors to HIGH
//==============================================================================
void Wheels_Forward(void) {
    // Right motor forward
    P6OUT |= R_FORWARD;      // R_FORWARD = 1 (wheel on)
    P6DIR |= R_FORWARD;      // Direction = output

    // Left motor forward
    P6OUT |= L_FORWARD;      // L_FORWARD = 1 (wheel on)
    P6DIR |= L_FORWARD;      // Direction = output
}

//==============================================================================
// Function: Wheels_Off
// Description: Turn off both wheels to stop
// Sets L_FORWARD and R_FORWARD motors to LOW
//==============================================================================
void Wheels_Off(void) {
    // Right motor off
    P6OUT &= ~R_FORWARD;     // R_FORWARD = 0 (wheel off)
    P6DIR &= ~R_FORWARD;     // Direction = input

    // Left motor off
    P6OUT &= ~L_FORWARD;     // L_FORWARD = 0 (wheel off)
    P6DIR &= ~L_FORWARD;     // Direction = input
}

//==============================================================================
// Function: Carlson_StateMachine
// Description: Time-based state machine with wheel demo
//==============================================================================
void Wheels_test(void) {
    switch(Time_Sequence) {
        case 250:
            // Reset the timer
            Time_Sequence = 0;
            break;

        case 200:
            // Turn wheels ON
            if(one_time) {
                Wheels_Forward();
                one_time = 0;
            }
            break;

        case 100:
            // Turn wheels OFF
            if(one_time) {
                Wheels_Off();
                one_time = 0;
            }
            break;

        default:
            break;
    }
}
