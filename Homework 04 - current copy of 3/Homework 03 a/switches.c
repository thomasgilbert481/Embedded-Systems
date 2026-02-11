//==============================================================================
// File Name: switches.c
// Description: Switch (button) handling for Project 4
//              SW1 cycles through shapes: Circle -> Figure-8 -> Triangle
//              SW2 is available as an emergency stop
//              Includes debouncing to prevent multiple triggers from one press
// Author: Thomas Gilbert
// Date: Feb 11, 2026
// Compiler: Code Composer Studio
//==============================================================================

#include "msp430.h"       // MSP430 register definitions
#include "functions.h"    // Function prototypes
#include "macros.h"       // Project constants (CIRCLE, FIGURE8, TRIANGLE, NONE)
#include "ports.h"        // Port pin definitions (SW1, SW2)
#include <string.h>       // For strcpy() to update LCD strings

//==============================================================================
// External variables — defined in other files, used here
//==============================================================================
extern char event;                              // Current shape event (from wheels.c)
extern char state;                              // Current shape state (from wheels.c)
extern char display_line[4][11];                // LCD display buffer (from init.c)
extern volatile unsigned char display_changed;  // LCD refresh flag (from init.c)

//==============================================================================
// Module-level variables for switch handling
//==============================================================================
static char next_shape = CIRCLE;        // Tracks which shape to start on next SW1 press
                                        // Static: retains value between function calls
                                        // Starts at CIRCLE, advances each press:
                                        //   CIRCLE -> FIGURE8 -> TRIANGLE -> CIRCLE -> ...

static unsigned int sw1_debounce = 0;   // Debounce counter for SW1
static unsigned int sw2_debounce = 0;   // Debounce counter for SW2
#define DEBOUNCE_THRESHOLD  (500)       // How many loop iterations to ignore after a press
                                        // Prevents one physical press from registering multiple times

//==============================================================================
// Function: Switches_Process
// Description: Main switch processing function — called every main loop iteration.
//              Checks both SW1 and SW2 for presses and handles debouncing.
//              SW1: starts the next shape in the sequence (Circle -> Fig8 -> Triangle)
//              SW2: emergency stop — immediately halts any running shape
//==============================================================================
void Switches_Process(void) {

    //--------------------------------------------------------------------------
    // SW1 Processing — Shape trigger button
    //--------------------------------------------------------------------------
    if (sw1_debounce > 0) {             // Are we in the debounce cooldown period?
        sw1_debounce--;                 // Yes — decrement counter and skip this press check
    } else {                            // No — button is ready to be read

        // SW1 is active LOW (pulled high by resistor, goes low when pressed)
        // P4IN reads the current state of Port 4 pins
        // If the SW1 bit is 0, the button is being pressed
        if (!(P4IN & SW1)) {            // Is SW1 currently pressed? (active low)

            sw1_debounce = DEBOUNCE_THRESHOLD;  // Start debounce cooldown

            //------------------------------------------------------------------
            // Only start a new shape if no shape is currently running
            // event == NONE means the car is idle and ready
            //------------------------------------------------------------------
            if (event == NONE) {                // Is the car idle?

                Start_Shape(next_shape);        // Start the next shape in the sequence
                                                // Start_Shape() (in wheels.c) sets event, state,
                                                // and updates the LCD with the shape name

                //--------------------------------------------------------------
                // Advance next_shape to the following shape for the NEXT press
                // This creates the cycle: Circle -> Figure-8 -> Triangle -> Circle
                // The car remembers where it is in the sequence even after
                // completing a shape — you can't reset the software (per project rules)
                //--------------------------------------------------------------
                switch (next_shape) {
                    case CIRCLE:                // Just started a circle
                        next_shape = FIGURE8;   // Next press will start figure-8
                        break;
                    case FIGURE8:               // Just started a figure-8
                        next_shape = TRIANGLE;  // Next press will start triangle
                        break;
                    case TRIANGLE:              // Just started a triangle
                        next_shape = CIRCLE;    // Next press will cycle back to circle
                        break;
                    default:                    // Safety: if somehow invalid
                        next_shape = CIRCLE;    // Reset to circle
                        break;
                }
            }
            // If event != NONE, a shape is already running — button press is ignored
            // This prevents accidentally starting a new shape mid-execution
        }
    }

    //--------------------------------------------------------------------------
    // SW2 Processing — Emergency stop button
    //--------------------------------------------------------------------------
    if (sw2_debounce > 0) {             // Are we in debounce cooldown?
        sw2_debounce--;                 // Yes — decrement and skip
    } else {                            // No — button is ready

        // SW2 is also active LOW (P2.3, pulled high by internal resistor)
        if (!(P2IN & SW2)) {            // Is SW2 currently pressed? (active low)

            sw2_debounce = DEBOUNCE_THRESHOLD;  // Start debounce cooldown

            //------------------------------------------------------------------
            // Emergency stop: immediately kill motors and reset state machine
            // This is useful during testing — if the car is heading off the table
            // or doing something unexpected, press SW2 to stop immediately
            //------------------------------------------------------------------
            P6OUT &= ~R_FORWARD;                // Turn off right motor immediately
            P6OUT &= ~L_FORWARD;                // Turn off left motor immediately
            event = NONE;                       // Clear the shape event
            state = NONE;                       // Clear the state machine state

            // Update LCD to show the car has been stopped
            strcpy(display_line[0], "  STOPPED ");      // Line 1: "STOPPED"
            strcpy(display_line[1], "          ");      // Line 2: blank
            strcpy(display_line[2], " Press SW1");      // Line 3: instruction
            strcpy(display_line[3], " to start ");      // Line 4: instruction
            display_changed = TRUE;                     // Flag LCD to refresh
        }
    }
}
