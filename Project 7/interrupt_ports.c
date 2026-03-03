//******************************************************************************
// File Name:    interrupt_ports.c
// Description:  Port interrupt service routines for Homework 06.
//               Contains two ISRs:
//                 switch1_interrupt -- PORT4_VECTOR (SW1 on P4.1)
//                 switch2_interrupt -- PORT2_VECTOR (SW2 on P2.3)
//
//               On button press each ISR:
//                 1. Disables the switch port interrupt (prevents re-trigger)
//                 2. Starts the corresponding CCR debounce timer
//                 3. Turns off the LCD backlight and disables the CCR0 timer
//                 4. Updates the display to identify which switch was pressed
//
//               After the debounce period (~1 second), TIMER0_B1_ISR (in
//               interrupts_timers.c) re-enables the switch interrupt and the
//               backlight timer automatically.
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
#include "adc.h"
#include <string.h>

//==============================================================================
// External variables
//==============================================================================
// From LCD.obj
extern char                   display_line[4][11];
extern volatile unsigned char display_changed;

// From interrupts_timers.c
extern volatile unsigned int  sw1_debounce_count;
extern volatile unsigned int  sw2_debounce_count;

// From main.c (Project 6 state machine)
extern volatile unsigned int  p6_state;
extern volatile unsigned int  p6_timer;
extern volatile unsigned int  p6_running;

//==============================================================================
// ISR: switch1_interrupt
// Description:  Port 4 interrupt for Switch 1 (P4.1, active low).
//               Disables SW1 for the debounce duration (~1 second via CCR1),
//               turns off the LCD backlight and halts the backlight timer,
//               and updates the display to show "Switch 1".
//               Also starts the Project 6 state machine if it is idle.
//
// Interrupt Source:  Port 4 GPIO (SW1 = P4.1)
// Trigger:           High-to-low edge on P4.1 (press with pull-up resistor)
//
// Globals used:    p6_state
// Globals changed: sw1_debounce_count, p6_state, p6_timer, p6_running,
//                  display_line, display_changed
// Local variables: none
//==============================================================================
#pragma vector = PORT4_VECTOR
__interrupt void switch1_interrupt(void){
//------------------------------------------------------------------------------

    if(P4IFG & SW1){                          // Confirm SW1 caused the interrupt

        P4IE   &= ~SW1;                       // 1. Disable SW1 interrupt
        P4IFG  &= ~SW1;                       // 2. Clear SW1 interrupt flag

        // Start CCR1 debounce timer
        sw1_debounce_count = RESET_STATE;     // 3. Reset debounce counter
        TB0CCTL1 &= ~CCIFG;                   // 4. Clear any pending CCR1 flag
        TB0CCR1   = TB0R + TB0CCR1_INTERVAL;  // 5. Schedule first CCR1 event
        TB0CCTL1 |=  CCIE;                    // 6. Enable CCR1 interrupt

        // Turn off backlight and halt the backlight timer during debounce
        P6OUT    &= ~LCD_BACKLITE;            // 7.  LCD backlight OFF
        TB0CCTL0 &= ~CCIE;                    // 8.  Disable CCR0 backlight interrupt

        // Start Project 6 detection sequence if idle
        if(p6_state == P6_IDLE){
            p6_timer   = RESET_STATE;         // Reset state machine timer
            p6_running = TRUE;                // Enable timer in CCR0 ISR
            p6_state   = P6_WAIT_1SEC;        // Begin 1-second pre-move delay
        }

        // Update display line 4 to identify this switch press
        strcpy(display_line[3], "Switch 1  ");
        display_changed = TRUE;
    }
//------------------------------------------------------------------------------
}

//==============================================================================
// ISR: switch2_interrupt
// Description:  Port 2 interrupt for Switch 2 (P2.3, active low).
//               Disables SW2 for the debounce duration (~1 second via CCR2),
//               turns off the LCD backlight and halts the backlight timer,
//               performs an emergency stop (all motors off, IR emitter off),
//               resets the Project 6 state machine, and updates the display.
//
// Interrupt Source:  Port 2 GPIO (SW2 = P2.3)
// Trigger:           High-to-low edge on P2.3 (press with pull-up resistor)
//
// Globals used:    none
// Globals changed: sw2_debounce_count, p6_state, p6_timer, p6_running,
//                  ir_emitter_on, display_line, display_changed
// Local variables: none
//==============================================================================
#pragma vector = PORT2_VECTOR
__interrupt void switch2_interrupt(void){
//------------------------------------------------------------------------------

    if(P2IFG & SW2){                          // Confirm SW2 caused the interrupt

        P2IE   &= ~SW2;                       // 1. Disable SW2 interrupt
        P2IFG  &= ~SW2;                       // 2. Clear SW2 interrupt flag

        // Start CCR2 debounce timer
        sw2_debounce_count = RESET_STATE;     // 3. Reset debounce counter
        TB0CCTL2 &= ~CCIFG;                   // 4. Clear any pending CCR2 flag
        TB0CCR2   = TB0R + TB0CCR2_INTERVAL;  // 5. Schedule first CCR2 event
        TB0CCTL2 |=  CCIE;                    // 6. Enable CCR2 interrupt

        // Turn off backlight and halt the backlight timer during debounce
        P6OUT    &= ~LCD_BACKLITE;            // 7.  LCD backlight OFF
        TB0CCTL0 &= ~CCIE;                    // 8.  Disable CCR0 backlight interrupt

        // Emergency stop -- cut all motor outputs immediately
        Wheels_All_Off();

        // Turn off IR emitter to conserve power
        P2OUT        &= ~IR_LED;
        ir_emitter_on  = FALSE;

        // Reset Project 6 state machine to idle
        p6_state   = P6_IDLE;
        p6_running = RESET_STATE;
        p6_timer   = RESET_STATE;

        // Update display line 4 to identify this switch press
        strcpy(display_line[3], "Switch 2  ");
        display_changed = TRUE;
    }
//------------------------------------------------------------------------------
}
