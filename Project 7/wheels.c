//==============================================================================
// File Name: wheels.c
// Description: Motor control for Project 7 using Timer B3 hardware PWM.
//              H-Bridge safety: NEVER set a forward AND reverse CCR nonzero
//              simultaneously on the same wheel.
//
//   CCR register mapping (ports.h):
//     RIGHT_FORWARD_SPEED = TB3CCR1  (P6.1, R_FORWARD)
//     LEFT_FORWARD_SPEED  = TB3CCR2  (P6.2, L_FORWARD)
//     RIGHT_REVERSE_SPEED = TB3CCR3  (P6.3, R_REVERSE)
//     LEFT_REVERSE_SPEED  = TB3CCR4  (P6.4, L_REVERSE)
//
// Author: Thomas Gilbert
// Date: Mar 2026
// Compiler: Code Composer Studio
//==============================================================================

#include "msp430.h"
#include "ports.h"
#include "macros.h"

void Wheels_All_Off(void){
    RIGHT_FORWARD_SPEED = WHEEL_OFF;
    LEFT_FORWARD_SPEED  = WHEEL_OFF;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;
    LEFT_REVERSE_SPEED  = WHEEL_OFF;
}

void Forward_On(void){
    LEFT_REVERSE_SPEED  = WHEEL_OFF;    // Safety: clear reverse first
    RIGHT_REVERSE_SPEED = WHEEL_OFF;
    LEFT_FORWARD_SPEED  = FOLLOW_SPEED;
    RIGHT_FORWARD_SPEED = FOLLOW_SPEED;
}

void Forward_Off(void){
    LEFT_FORWARD_SPEED  = WHEEL_OFF;
    RIGHT_FORWARD_SPEED = WHEEL_OFF;
}

void Reverse_On(void){
    LEFT_FORWARD_SPEED  = WHEEL_OFF;    // Safety: clear forward first
    RIGHT_FORWARD_SPEED = WHEEL_OFF;
    LEFT_REVERSE_SPEED  = FOLLOW_SPEED;
    RIGHT_REVERSE_SPEED = FOLLOW_SPEED;
}

void Reverse_Off(void){
    LEFT_REVERSE_SPEED  = WHEEL_OFF;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;
}

void Spin_CW_On(void){
    RIGHT_FORWARD_SPEED = WHEEL_OFF;    // Safety: all off first
    LEFT_REVERSE_SPEED  = WHEEL_OFF;
    LEFT_FORWARD_SPEED  = SPIN_SPEED;
    RIGHT_REVERSE_SPEED = SPIN_SPEED;
}

void Spin_CCW_On(void){
    LEFT_FORWARD_SPEED  = WHEEL_OFF;    // Safety: all off first
    RIGHT_REVERSE_SPEED = WHEEL_OFF;
    RIGHT_FORWARD_SPEED = SPIN_SPEED;
    LEFT_REVERSE_SPEED  = SPIN_SPEED;
}
