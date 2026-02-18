//==============================================================================
// File Name: switches.c
// Description: Switch (button) handling for HW05.
//              Duplicated from Project 04 per HW05 instructions.
//
//              SW1 (P4.1): Toggles SMCLK between 500kHz and 8MHz
//              SW2 (P2.3): Forces reset back to 8MHz GP I/O mode
//
// Global Variables Used:
//              display_line     - LCD display buffer (defined in main.c)
//              display_changed  - LCD update flag (defined in main.c)
//
// Author: Thomas Gilbert
// Date: Feb 2026
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
// Debounce counters and toggle state — static, file scope
//==============================================================================
static unsigned int sw1_debounce = 0;
static unsigned int sw2_debounce = 0;
static char smclk_500 = 0;         // 0 = currently 8MHz, 1 = currently 500kHz

//==============================================================================
// Function: Switches_Process
// Description: Called from the main loop. Checks SW1 and SW2 with debouncing.
//
//              SW1 (P4.1, active low): Toggles between 500kHz and 8MHz
//                  - First press:  500kHz, SMCLK output on P3.4
//                  - Second press: 8MHz,   GPIO on P3.4
//                  - Alternates each press
//
//              SW2 (P2.3, active low): Forces reset to 8MHz GPIO mode
//
// Passed:    none
// Returns:   none
// Globals Modified: display_line, display_changed
//==============================================================================
void Switches_Process(void) {

    //--------------------------------------------------------------------------
    // SW1 Processing — toggle SMCLK between 500kHz and 8MHz
    //--------------------------------------------------------------------------
    if(sw1_debounce > 0){
        sw1_debounce--;
    } else {
        if(!(P4IN & SW1)){                      // SW1 pressed (active low)
            sw1_debounce = DEBOUNCE_THRESHOLD;

            if(smclk_500 == 0){
                //--------------------------------------------------------------
                // Currently 8MHz — switch TO 500kHz
                //--------------------------------------------------------------
                CSCTL0_H = CS_UNLOCK;           // Unlock CS registers
                CSCTL5 |= DIVM__4;              // MCLK  = 8MHz / 4 = 2MHz
                CSCTL5 |= DIVS__4;              // SMCLK = 2MHz / 4 = 500kHz
                CSCTL0_H = CS_LOCK;             // Lock CS registers

                Init_Port3(USE_SMCLK);          // P3.4 -> SMCLK output

                strcpy(display_line[0], "  SMCLK ");
                strcpy(display_line[1], " OUTPUT ");
                strcpy(display_line[2], " 500kHz ");
                strcpy(display_line[3], "  P3.4  ");

                smclk_500 = 1;                  // Record new state

            } else {
                //--------------------------------------------------------------
                // Currently 500kHz — switch BACK to 8MHz
                //--------------------------------------------------------------
                CSCTL0_H = CS_UNLOCK;           // Unlock CS registers
                CSCTL5 &= ~DIVM__4;             // MCLK  = 8MHz / 1 = 8MHz
                CSCTL5 &= ~DIVS__4;             // SMCLK = 8MHz / 1 = 8MHz
                CSCTL0_H = CS_LOCK;             // Lock CS registers

                Init_Port3(USE_GPIO);           // P3.4 -> GP I/O

                strcpy(display_line[0], "  GPIO  ");
                strcpy(display_line[1], "  MODE  ");
                strcpy(display_line[2], "  8MHz  ");
                strcpy(display_line[3], "  P3.4  ");

                smclk_500 = 0;                  // Record new state
            }

            display_changed = TRUE;
        }
    }

    //--------------------------------------------------------------------------
    // SW2 Processing — force reset to 8MHz GP I/O mode
    //--------------------------------------------------------------------------
    if(sw2_debounce > 0){
        sw2_debounce--;
    } else {
        if(!(P2IN & SW2)){                      // SW2 pressed (active low)
            sw2_debounce = DEBOUNCE_THRESHOLD;

            CSCTL0_H = CS_UNLOCK;               // Unlock CS registers
            CSCTL5 &= ~DIVM__4;                 // MCLK  = 8MHz / 1 = 8MHz
            CSCTL5 &= ~DIVS__4;                 // SMCLK = 8MHz / 1 = 8MHz
            CSCTL0_H = CS_LOCK;                 // Lock CS registers

            Init_Port3(USE_GPIO);               // P3.4 -> GP I/O

            strcpy(display_line[0], "  GPIO  ");
            strcpy(display_line[1], "  MODE  ");
            strcpy(display_line[2], "  8MHz  ");
            strcpy(display_line[3], "  P3.4  ");

            smclk_500 = 0;                      // Sync toggle state
            display_changed = TRUE;
        }
    }
}
