//==============================================================================
// File:        wheels.c
// Description: Motor control for Project 9 Part 2 using Timer B3 hardware PWM.
//              H-Bridge safety: NEVER drive forward AND reverse simultaneously
//              on the same wheel. Always clear the opposite direction first.
//
//              PWM CCR mapping (defined in ports.h):
//                RIGHT_FORWARD_SPEED = TB3CCR1  (P6.1, R_FORWARD)
//                LEFT_FORWARD_SPEED  = TB3CCR2  (P6.2, L_FORWARD)
//                RIGHT_REVERSE_SPEED = TB3CCR3  (P6.3, R_REVERSE)
//                LEFT_REVERSE_SPEED  = TB3CCR4  (P6.4, L_REVERSE)
//
//              Speed range: WHEEL_OFF (0) to WHEEL_PERIOD_VAL (50005)
//              Drive speed: FOLLOW_SPEED  Spin speed: SPIN_SPEED
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
// Wheels_All_Off -- Kills all motor PWM outputs immediately.
//                   Call before ANY direction change (H-bridge safety).
//==============================================================================
void Wheels_All_Off(void){
    LEFT_FORWARD_SPEED  = WHEEL_OFF;
    RIGHT_FORWARD_SPEED = WHEEL_OFF;
    LEFT_REVERSE_SPEED  = WHEEL_OFF;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;
}

//==============================================================================
// Forward_On -- Drive both wheels forward at FOLLOW_SPEED.
//==============================================================================
void Forward_On(void){
    LEFT_REVERSE_SPEED  = WHEEL_OFF;       // Clear reverse first (SAFETY)
    RIGHT_REVERSE_SPEED = WHEEL_OFF;
    LEFT_FORWARD_SPEED  = FOLLOW_SPEED;
    RIGHT_FORWARD_SPEED = FOLLOW_SPEED;
}

//==============================================================================
// Forward_Off -- Stop forward motion without engaging reverse.
//==============================================================================
void Forward_Off(void){
    LEFT_FORWARD_SPEED  = WHEEL_OFF;
    RIGHT_FORWARD_SPEED = WHEEL_OFF;
}

//==============================================================================
// Reverse_On -- Drive both wheels reverse at FOLLOW_SPEED.
//==============================================================================
void Reverse_On(void){
    LEFT_FORWARD_SPEED  = WHEEL_OFF;       // Clear forward first (SAFETY)
    RIGHT_FORWARD_SPEED = WHEEL_OFF;
    LEFT_REVERSE_SPEED  = FOLLOW_SPEED;
    RIGHT_REVERSE_SPEED = FOLLOW_SPEED;
}

//==============================================================================
// Reverse_Off -- Stop reverse motion without engaging forward.
//==============================================================================
void Reverse_Off(void){
    LEFT_REVERSE_SPEED  = WHEEL_OFF;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;
}

//==============================================================================
// Spin_CW_On  -- Spin clockwise in place (Right turn).
//==============================================================================
void Spin_CW_On(void){
    Wheels_All_Off();                       // All off first (SAFETY)
    LEFT_FORWARD_SPEED  = SPIN_SPEED;
    RIGHT_REVERSE_SPEED = SPIN_SPEED;
}

//==============================================================================
// Spin_CCW_On -- Spin counter-clockwise in place (Left turn).
//==============================================================================
void Spin_CCW_On(void){
    Wheels_All_Off();                       // All off first (SAFETY)
    RIGHT_FORWARD_SPEED = SPIN_SPEED;
    LEFT_REVERSE_SPEED  = SPIN_SPEED;
}
