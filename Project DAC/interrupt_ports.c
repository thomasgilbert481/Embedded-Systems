//******************************************************************************
// File Name:    interrupt_ports.c
// Description:  Port interrupt service routines for Project DAC.
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
//               Project DAC changes:
//                 SW1 -- starts (or restarts) the DAC motor demo state machine
//                 SW2 -- emergency stop: motors off, DAC board disabled, SM reset
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

// From main.c / dac.c (Project DAC state machine)
extern volatile unsigned int  dac_state;
extern volatile unsigned int  dac_timer;
extern volatile unsigned int  dac_running;
extern volatile unsigned int  DAC_data;

//==============================================================================
// ISR: switch1_interrupt
// Description:  Port 4 interrupt for Switch 1 (P4.1, active low).
//               Disables SW1 for the debounce duration (~1 second via CCR1),
//               turns off the LCD backlight and halts the backlight timer,
//               and starts (or restarts) the DAC motor demo.
//
// Interrupt Source:  Port 4 GPIO (SW1 = P4.1)
// Trigger:           High-to-low edge on P4.1 (press with pull-up resistor)
//
// Globals used:    dac_state
// Globals changed: sw1_debounce_count, dac_state, dac_timer, dac_running,
//                  DAC_data, display_line, display_changed
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
        P6OUT    &= ~LCD_BACKLITE;            // 7. LCD backlight OFF
        TB0CCTL0 &= ~CCIE;                    // 8. Disable CCR0 backlight interrupt

        // Start (or restart) the DAC demo when idle or after previous run
        if(dac_state == DAC_IDLE || dac_state == DAC_DONE){
            DAC_data    = DAC_INIT_VALUE;     // Reset DAC to 4000 (full power)
            SAC3DAT     = DAC_data;           // Pre-load hardware register
            dac_timer   = RESET_STATE;        // Reset settling delay counter
            dac_running = TRUE;               // Enable CCR0 ISR to increment dac_timer
            dac_state   = DAC_STARTUP;        // Enter 600ms settling delay
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
//               Emergency stop: immediately cuts all motor outputs, disables
//               the DAC board, resets the DAC demo state machine to idle,
//               and updates the display.
//
// Interrupt Source:  Port 2 GPIO (SW2 = P2.3)
// Trigger:           High-to-low edge on P2.3 (press with pull-up resistor)
//
// Globals used:    none
// Globals changed: sw2_debounce_count, dac_state, dac_timer, dac_running,
//                  DAC_data, display_line, display_changed
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
        P6OUT    &= ~LCD_BACKLITE;            // 7. LCD backlight OFF
        TB0CCTL0 &= ~CCIE;                    // 8. Disable CCR0 backlight interrupt

        // Emergency stop -- cut all motor outputs immediately
        Wheels_All_Off();

        // Disable DAC board to remove motor power
        P2OUT   &= ~DAC_ENB;                  // DAC board OFF

        // Reset DAC demo state machine to idle
        dac_state   = DAC_IDLE;
        dac_running = RESET_STATE;
        dac_timer   = RESET_STATE;
        DAC_data    = DAC_INIT_VALUE;         // Pre-set for next SW1 press
        SAC3DAT     = DAC_data;

        // Update display line 4 to identify this switch press
        strcpy(display_line[3], "Switch 2  ");
        display_changed = TRUE;
    }
//------------------------------------------------------------------------------
}
