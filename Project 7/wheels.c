//==============================================================================
// File Name: wheels.c
// Description: Motor control functions for Project 7 using Timer B3 hardware PWM.
//              H-Bridge safety: NEVER set a forward AND reverse CCR to nonzero
//              simultaneously on the same wheel. Always clear the opposite
//              direction before engaging the desired direction.
//
//              PWM CCR register mapping (defined in ports.h):
//                RIGHT_FORWARD_SPEED = TB3CCR1  (P6.1, R_FORWARD)
//                LEFT_FORWARD_SPEED  = TB3CCR2  (P6.2, L_FORWARD)
//                RIGHT_REVERSE_SPEED = TB3CCR3  (P6.3, R_REVERSE)
//                LEFT_REVERSE_SPEED  = TB3CCR4  (P6.4, L_REVERSE)
//
//              Speed range: WHEEL_OFF (0) to WHEEL_PERIOD_VAL (50005)
//              Default drive speed: FOLLOW_SPEED (25000, ~50% duty)
//              Spin speed:          SPIN_SPEED   (25000, ~50% duty)
//
// Author: Thomas Gilbert
// Date: Mar 2026
// Compiler: Code Composer Studio
//==============================================================================

#include "msp430.h"
#include "functions.h"
#include "macros.h"
#include "ports.h"

//==============================================================================
// Function: Wheels_All_Off
// Description: Kills all motor PWM outputs immediately.
//              Call this before ANY direction change (H-bridge safety).
//==============================================================================
void Wheels_All_Off(void){
    LEFT_FORWARD_SPEED  = WHEEL_OFF;   // Left forward PWM off
    RIGHT_FORWARD_SPEED = WHEEL_OFF;   // Right forward PWM off
    LEFT_REVERSE_SPEED  = WHEEL_OFF;   // Left reverse PWM off
    RIGHT_REVERSE_SPEED = WHEEL_OFF;   // Right reverse PWM off
}

//==============================================================================
// Function: Forward_On
// Description: Drives both wheels forward at FOLLOW_SPEED.
//              SAFETY: Clears reverse CCRs first.
//==============================================================================
void Forward_On(void){
    LEFT_REVERSE_SPEED  = WHEEL_OFF;       // Clear reverse first (SAFETY)
    RIGHT_REVERSE_SPEED = WHEEL_OFF;       // Clear reverse first (SAFETY)
    LEFT_FORWARD_SPEED  = FOLLOW_SPEED;    // Set left wheel forward
    RIGHT_FORWARD_SPEED = FOLLOW_SPEED;    // Set right wheel forward
}

//==============================================================================
// Function: Forward_Off
// Description: Stops forward motion without engaging reverse.
//==============================================================================
void Forward_Off(void){
    LEFT_FORWARD_SPEED  = WHEEL_OFF;   // Stop left forward
    RIGHT_FORWARD_SPEED = WHEEL_OFF;   // Stop right forward
}

//==============================================================================
// Function: Reverse_On
// Description: Drives both wheels in reverse at FOLLOW_SPEED.
//              SAFETY: Clears forward CCRs first.
//==============================================================================
void Reverse_On(void){
    LEFT_FORWARD_SPEED  = WHEEL_OFF;       // Clear forward first (SAFETY)
    RIGHT_FORWARD_SPEED = WHEEL_OFF;       // Clear forward first (SAFETY)
    LEFT_REVERSE_SPEED  = FOLLOW_SPEED;    // Set left wheel reverse
    RIGHT_REVERSE_SPEED = FOLLOW_SPEED;    // Set right wheel reverse
}

//==============================================================================
// Function: Reverse_Off
// Description: Stops reverse motion without engaging forward.
//==============================================================================
void Reverse_Off(void){
    LEFT_REVERSE_SPEED  = WHEEL_OFF;   // Stop left reverse
    RIGHT_REVERSE_SPEED = WHEEL_OFF;   // Stop right reverse
}

//==============================================================================
// Function: Spin_CW_On
// Description: Spins clockwise in place (left forward, right reverse).
//              SAFETY: All-off first, then set opposite wheels.
//==============================================================================
void Spin_CW_On(void){
    Wheels_All_Off();                      // Everything off first (SAFETY)
    LEFT_FORWARD_SPEED  = SPIN_SPEED;     // Left wheel forward
    RIGHT_REVERSE_SPEED = SPIN_SPEED;     // Right wheel reverse
}

//==============================================================================
// Function: Spin_CCW_On
// Description: Spins counter-clockwise in place (right forward, left reverse).
//              SAFETY: All-off first, then set opposite wheels.
//==============================================================================
void Spin_CCW_On(void){
    Wheels_All_Off();                      // Everything off first (SAFETY)
    RIGHT_FORWARD_SPEED = SPIN_SPEED;     // Right wheel forward
    LEFT_REVERSE_SPEED  = SPIN_SPEED;     // Left wheel reverse
}
