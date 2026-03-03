//******************************************************************************
// File Name:    main.c
// Description:  Main routine for Homework 06 -- Timer B0 and Switch Interrupts.
//
//               Timer B0 CCR0 fires every 200ms:
//                 - Toggles LCD backlight (2.5 Hz blink)
//                 - Sets update_display flag to refresh the LCD
//
//               SW1 (P4.1) and SW2 (P2.3) are fully interrupt-driven:
//                 - Press   -> backlight off, display updated, CCR1/CCR2 starts
//                 - ~1 sec  -> CCR1/CCR2 re-enables switch and backlight timer
//
// Author:       Thomas Gilbert
// Date:         Mar 2026
// Compiler:     Code Composer Studio
// Target:       MSP430FR2355
//******************************************************************************

#include "msp430.h"
#include <string.h>
#include "functions.h"
#include "LCD.h"
#include "ports.h"
#include "macros.h"

//==============================================================================
// External variables (defined in LCD.obj and timers.c)
//==============================================================================
extern char display_line[4][11];
extern char *display[4];
extern volatile unsigned char display_changed;
extern volatile unsigned char update_display;

//==============================================================================
// Function: main
// Description: Initializes all peripherals and loops forever.
//              Display is refreshed at most every 200ms, gated by update_display
//              (set in Timer0_B0_ISR).  Switch handling is entirely in the port
//              ISRs (interrupt_ports.c) with debounce via CCR1/CCR2.
//
// Globals used:    update_display, display_changed
// Globals changed: display_line, display_changed
// Local variables: none
//==============================================================================
void main(void){
//------------------------------------------------------------------------------

    PM5CTL0 &= ~LOCKLPM5;  // Unlock GPIO (required after reset on FR devices)

    Init_Ports();           // Configure all GPIO pins
    Init_Clocks();          // Configure clock system (SMCLK = 8 MHz)
    Init_Conditions();      // Initialize variables, enable global interrupts
    Init_Timers();          // Start Timer B0 (200ms CCR0, CCR1/CCR2 for debounce)
    Init_LCD();             // Initialize LCD over SPI

    // Initial display
    strcpy(display_line[0], "  HW 06   ");
    strcpy(display_line[1], " Timer B0 ");
    strcpy(display_line[2], "SW1 or SW2");
    strcpy(display_line[3], " to test  ");
    display_changed = TRUE;

    //==========================================================================
    // Main loop
    //==========================================================================
    while(ALWAYS){

        // Refresh LCD when the 200ms timer fires
        Display_Process();

        P3OUT ^= TEST_PROBE;  // Oscilloscope heartbeat
    }
//------------------------------------------------------------------------------
}
