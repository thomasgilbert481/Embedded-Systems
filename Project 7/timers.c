//******************************************************************************
// File Name:    timers.c
// Description:  Timer initialization for Project 7.
//               Configures Timer B0 in continuous mode for:
//                 CCR0 -- 200ms backlight blink + display update + P7 timers
//                 CCR1 -- SW1 interrupt-driven debounce
//                 CCR2 -- SW2 interrupt-driven debounce
//               Also configures Timer B3 for hardware PWM motor control.
//
//               Clock math (SMCLK = 8 MHz, ID__8, TBIDEX__8):
//                 Effective clock = 8,000,000 / 8 / 8 = 125,000 Hz
//                 TB0CCR0 = 125,000 / 5 = 25,000 counts = 200ms
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

//==============================================================================
// Global definitions (previously provided by Carlson's timerB0.obj)
//==============================================================================
volatile unsigned int Time_Sequence = RESET_STATE;
volatile char         one_time      = FALSE;

//==============================================================================
// Function: Init_Timers
// Description: Calls individual timer initialization functions.
//==============================================================================
void Init_Timers(void){
//------------------------------------------------------------------------------
    Init_Timer_B0();
    Init_Timer_B3();   // Hardware PWM for motor control (Project 7)
}

//==============================================================================
// Function: Init_Timer_B0
// Description: Configures Timer B0 in continuous mode with three capture/
//              compare registers:
//                CCR0 -- 200ms period for backlight toggle and display trigger
//                CCR1 -- SW1 software debounce (started on button press)
//                CCR2 -- SW2 software debounce (started on button press)
//
//              Timer source:  SMCLK = 8 MHz
//              ID divider:    /8  (ID__8)
//              TBIDEX divider: /8 (TBIDEX__8)
//              Effective clock: 8,000,000 / 8 / 8 = 125,000 Hz
//              CCR0 count:    125,000 / 5 = 25,000  =>  200ms interval
//
// Globals used:    none
// Globals changed: none
// Local variables: none
//==============================================================================
void Init_Timer_B0(void){
//------------------------------------------------------------------------------

    TB0CTL  = TBSSEL__SMCLK;      // Clock source = SMCLK (8 MHz)
    TB0CTL |= MC__CONTINUOUS;     // Continuous mode: counts 0 -> 0xFFFF, repeats
    TB0CTL |= ID__8;              // Input divider: divide clock by 8
    TB0EX0  = TBIDEX__8;          // Additional divider: divide by 8
    TB0CTL |= TBCLR;              // Clear TB0R, clock divider, count direction
    // NOTE: TBCLR must come AFTER ID and TBIDEX to reset all divider flip-flops

    //--------------------------------------------------------------------------
    // CCR0 -- Backlight blink + display update (200ms interval)
    //--------------------------------------------------------------------------
    TB0CCR0  = TB0CCR0_INTERVAL;  // First interrupt at 25,000 counts (200ms)
    TB0CCTL0 &= ~CCIFG;           // Clear any pending CCR0 interrupt flag
    TB0CCTL0 |=  CCIE;            // Enable CCR0 interrupt

    //--------------------------------------------------------------------------
    // CCR1 -- SW1 debounce timer (disabled until SW1 is pressed)
    //--------------------------------------------------------------------------
    TB0CCR1  = TB0CCR1_INTERVAL;  // Load initial value (re-armed on SW1 press)
    TB0CCTL1 &= ~CCIFG;           // Clear any pending CCR1 interrupt flag
    TB0CCTL1 &= ~CCIE;            // Disable CCR1 interrupt (enabled in SW1 ISR)

    //--------------------------------------------------------------------------
    // CCR2 -- SW2 debounce timer (disabled until SW2 is pressed)
    //--------------------------------------------------------------------------
    TB0CCR2  = TB0CCR2_INTERVAL;  // Load initial value (re-armed on SW2 press)
    TB0CCTL2 &= ~CCIFG;           // Clear any pending CCR2 interrupt flag
    TB0CCTL2 &= ~CCIE;            // Disable CCR2 interrupt (enabled in SW2 ISR)

    //--------------------------------------------------------------------------
    // Overflow -- not used
    //--------------------------------------------------------------------------
    TB0CTL &= ~TBIE;              // Disable overflow interrupt
    TB0CTL &= ~TBIFG;             // Clear overflow interrupt flag
}

//==============================================================================
// Function: Init_Timer_B3
// Description: Configures Timer B3 for hardware PWM on motor pins.
//              Up mode: counts from 0 to TB3CCR0, then resets.
//              All motor channels use output mode 7 (reset/set):
//                pin HIGH at period start, LOW when CCR count reached.
//              Setting a CCR to WHEEL_OFF (0) keeps the output LOW (motor off).
//
//   Pin mapping (Port 6, SEL0=1 SEL1=0 required in ports.c):
//     TB3CCR1 -> P6.1 -> R_FORWARD   (RIGHT_FORWARD_SPEED)
//     TB3CCR2 -> P6.2 -> L_FORWARD   (LEFT_FORWARD_SPEED)
//     TB3CCR3 -> P6.3 -> R_REVERSE   (RIGHT_REVERSE_SPEED)
//     TB3CCR4 -> P6.4 -> L_REVERSE   (LEFT_REVERSE_SPEED)
//     TB3CCR5 -> not used (P6.5 kept as GPIO input; backlight on P6.0)
//
//   PWM period: WHEEL_PERIOD_VAL = 50005 cycles at 8 MHz SMCLK
//              => ~6.25 ms period => ~160 Hz PWM frequency
//
// Globals used:    none
// Globals changed: none
// Local variables: none
//==============================================================================
void Init_Timer_B3(void){
//------------------------------------------------------------------------------

    TB3CTL  = TBSSEL__SMCLK;          // Clock source = SMCLK (8 MHz), no dividers
    TB3CTL |= MC__UP;                 // Up mode: count 0 -> TB3CCR0, then reset
    TB3CTL |= TBCLR;                  // Clear the timer counter and dividers

    TB3CCR0 = WHEEL_PERIOD_VAL;       // PWM period: 50005 cycles = ~6.25 ms

    // CCR1: Right Forward (P6.1, R_FORWARD)
    TB3CCTL1 = OUTMOD_7;              // Output mode 7: reset/set
    RIGHT_FORWARD_SPEED = WHEEL_OFF;  // Start with motor off (0% duty cycle)

    // CCR2: Left Forward (P6.2, L_FORWARD)
    TB3CCTL2 = OUTMOD_7;              // Output mode 7: reset/set
    LEFT_FORWARD_SPEED = WHEEL_OFF;   // Start with motor off

    // CCR3: Right Reverse (P6.3, R_REVERSE)
    TB3CCTL3 = OUTMOD_7;              // Output mode 7: reset/set
    RIGHT_REVERSE_SPEED = WHEEL_OFF;  // Start with motor off

    // CCR4: Left Reverse (P6.4, L_REVERSE)
    TB3CCTL4 = OUTMOD_7;              // Output mode 7: reset/set
    LEFT_REVERSE_SPEED = WHEEL_OFF;   // Start with motor off

    // CCR5: Not used -- P6.5 is left as GPIO input in ports.c.
    // LCD backlight is toggled on P6.0 (GPIO) in the CCR0 ISR.
//------------------------------------------------------------------------------
}
