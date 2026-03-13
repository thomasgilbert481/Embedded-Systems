//==============================================================================
// File Name: switches.c
// Description: Switch polling for Project 7.
//              SW1 drives the calibration -> reposition -> start flow.
//              SW2 emergency stop.
//
//   SW1 behavior by state:
//     P7_IDLE:                   Start calibration sequence
//     P7_CALIBRATE (CAL_WAIT_BLACK): Sample black tape, compute thresholds
//     P7_CAL_COMPLETE:           Car repositioned, start 2-second countdown
//     Other states:              Ignored
//
//   SW2: Emergency stop -- kills motors, IR, resets to P7_IDLE.
//
// Author: Thomas Gilbert
// Date: Mar 2026
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

// Project 7 state machine (defined in main.c)
extern volatile unsigned int p7_state;
extern volatile unsigned int p7_timer;
extern volatile unsigned int p7_running;
extern volatile unsigned int elapsed_tenths;
extern volatile unsigned int p7_timer_running;
extern unsigned int          calibrate_phase;
extern unsigned int          ir_emitter_on;

// Project 5/6 legacy (defined in main.c)
extern volatile unsigned int p5_timer;
extern volatile unsigned int p5_running;
extern volatile unsigned int p6_state;
extern volatile unsigned int p6_timer;
extern volatile unsigned int p6_running;

//==============================================================================
// Debounce counters
//==============================================================================
static unsigned int sw1_debounce = 0;
static unsigned int sw2_debounce = 0;
#define DEBOUNCE_THRESHOLD  (500)

//==============================================================================
// Function: Switches_Process
// Description: Polls SW1 and SW2 with software debouncing.
//              Called every main loop iteration.
//==============================================================================
void Switches_Process(void){

    //--------------------------------------------------------------------------
    // SW1 (P4.1, active low)
    //--------------------------------------------------------------------------
    if(sw1_debounce > 0){
        sw1_debounce--;
    } else {
        if(!(P4IN & SW1)){              // SW1 pressed?

            sw1_debounce = DEBOUNCE_THRESHOLD;

            if(p7_state == P7_IDLE){
                // Start calibration sequence
                p7_timer        = 0;
                p7_running      = 1;
                calibrate_phase = CAL_AMBIENT;
                p7_state        = P7_CALIBRATE;

                // Turn off IR for ambient measurement
                P2OUT        &= ~IR_LED;
                ir_emitter_on  = 0;

                strcpy(display_line[0], "Calibrate ");
                strcpy(display_line[1], " Ambient  ");
                strcpy(display_line[2], "  IR OFF  ");
                strcpy(display_line[3], " sampling ");
                display_changed = TRUE;

            } else if(p7_state == P7_CALIBRATE &&
                      calibrate_phase == CAL_WAIT_BLACK){
                // Operator placed car on black tape -- sample it
                calibrate_phase = CAL_BLACK;

                strcpy(display_line[0], "Calibrate ");
                strcpy(display_line[1], "  Black   ");
                strcpy(display_line[2], " sampling ");
                strcpy(display_line[3], "          ");
                display_changed = TRUE;

            } else if(p7_state == P7_CAL_COMPLETE){
                // Operator repositioned car at circle center -- start countdown
                p7_timer   = 0;
                p7_running = 1;
                p7_state   = P7_WAIT_START;

                strcpy(display_line[0], " Wait 2s  ");
                strcpy(display_line[1], "  then    ");
                strcpy(display_line[2], " driving  ");
                strcpy(display_line[3], "          ");
                display_changed = TRUE;
            }
            // All other states: ignore SW1
        }
    }

    //--------------------------------------------------------------------------
    // SW2 (P2.3, active low) -- Emergency stop
    //--------------------------------------------------------------------------
    if(sw2_debounce > 0){
        sw2_debounce--;
    } else {
        if(!(P2IN & SW2)){              // SW2 pressed?

            sw2_debounce = DEBOUNCE_THRESHOLD;

            // Kill all motor PWM outputs (H-bridge safe)
            Wheels_All_Off();

            // Turn off IR emitter
            P2OUT        &= ~IR_LED;
            ir_emitter_on  = 0;

            // Reset Project 7 state machine
            p7_state         = P7_IDLE;
            p7_running       = 0;
            p7_timer         = 0;
            p7_timer_running = 0;
            elapsed_tenths   = 0;
            calibrate_phase  = CAL_AMBIENT;

            // Reset P5/P6 legacy
            p5_running = 0;
            p5_timer   = 0;
            p6_state   = P6_IDLE;
            p6_running = 0;
            p6_timer   = 0;

            strcpy(display_line[0], "  STOPPED ");
            strcpy(display_line[1], "          ");
            strcpy(display_line[2], " Press SW1");
            strcpy(display_line[3], " to start ");
            display_changed = TRUE;
        }
    }
}
