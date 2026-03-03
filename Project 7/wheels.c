//==============================================================================
// File Name: wheels.c
// Description: Wheel motor control for Project 5
//              Full H-Bridge: forward, reverse, and spin functions
//              CRITICAL: Always turn off opposite direction BEFORE engaging!
// Author: Thomas Gilbert
// Date: Feb 2026
// Compiler: Code Composer Studio
//==============================================================================

#include "msp430.h"       // MSP430 register definitions
#include "functions.h"    // Function prototypes
#include "macros.h"       // Project-wide #define constants
#include "ports.h"        // Port pin definitions (R_FORWARD, L_FORWARD, etc.)

//==============================================================================
// Function: Wheels_All_Off
// Description: Turns off ALL motor pins (forward AND reverse, both wheels).
//              This is the safest function — call it before any direction change.
//==============================================================================
void Wheels_All_Off(void){
    P6OUT &= ~R_FORWARD;     // Right forward OFF
    P6OUT &= ~L_FORWARD;     // Left forward OFF
    P6OUT &= ~R_REVERSE;     // Right reverse OFF
    P6OUT &= ~L_REVERSE;     // Left reverse OFF
}

//==============================================================================
// Function: Forward_On
// Description: Drives both wheels forward.
//              SAFETY: Turns off reverse pins FIRST to prevent H-bridge damage.
//==============================================================================
void Forward_On(void){
    P6OUT &= ~R_REVERSE;     // Right reverse OFF first (SAFETY)
    P6OUT &= ~L_REVERSE;     // Left reverse OFF first (SAFETY)
    P6OUT |= R_FORWARD;      // Right forward ON
    P6OUT |= L_FORWARD;      // Left forward ON
}

//==============================================================================
// Function: Forward_Off
// Description: Turns off both forward motor pins.
//==============================================================================
void Forward_Off(void){
    P6OUT &= ~R_FORWARD;     // Right forward OFF
    P6OUT &= ~L_FORWARD;     // Left forward OFF
}

//==============================================================================
// Function: Reverse_On
// Description: Drives both wheels in reverse.
//              SAFETY: Turns off forward pins FIRST to prevent H-bridge damage.
//==============================================================================
void Reverse_On(void){
    P6OUT &= ~R_FORWARD;     // Right forward OFF first (SAFETY)
    P6OUT &= ~L_FORWARD;     // Left forward OFF first (SAFETY)
    P6OUT |= R_REVERSE;      // Right reverse ON
    P6OUT |= L_REVERSE;      // Left reverse ON
}

//==============================================================================
// Function: Reverse_Off
// Description: Turns off both reverse motor pins.
//==============================================================================
void Reverse_Off(void){
    P6OUT &= ~R_REVERSE;     // Right reverse OFF
    P6OUT &= ~L_REVERSE;     // Left reverse OFF
}

//==============================================================================
// Function: Spin_CW_On
// Description: Spins the vehicle clockwise (in place).
//              Left wheel goes forward, Right wheel goes reverse.
//              SAFETY: Calls Wheels_All_Off() first.
//==============================================================================
void Spin_CW_On(void){
    Wheels_All_Off();         // Everything off first (SAFETY)
    P6OUT |= L_FORWARD;      // Left wheel forward
    P6OUT |= R_REVERSE;      // Right wheel reverse
}

//==============================================================================
// Function: Spin_CCW_On
// Description: Spins the vehicle counter-clockwise (in place).
//              Right wheel goes forward, Left wheel goes reverse.
//              SAFETY: Calls Wheels_All_Off() first.
//==============================================================================
void Spin_CCW_On(void){
    Wheels_All_Off();         // Everything off first (SAFETY)
    P6OUT |= R_FORWARD;      // Right wheel forward
    P6OUT |= L_REVERSE;      // Left wheel reverse
}

//==============================================================================
// These old Project 4 functions are kept as stubs so nothing breaks if
// referenced. You can remove them once you've cleaned up all references.
//==============================================================================
void Forward_Move(void){
    // Not used in Project 5 — software PWM no longer needed
}

void Wheels_Forward(void){
    Forward_On();  // Just alias to safe version
}

void Wheels_Off(void){
    Wheels_All_Off();  // Just alias to safe version
}
