//==============================================================================
// File Name: main.c
// Description: Main routine for Project 5 — Forward/Reverse/Spin Sequence
//              Uses interrupt-based timing (NOT Carlson's provided functions).
//              SW1 starts the movement sequence.
// Author: Thomas Gilbert
// Date: Feb 2026
// Compiler: Code Composer Studio
//==============================================================================

#include "msp430.h"       // MSP430 register definitions
#include <string.h>       // For strcpy() to write LCD display strings
#include "functions.h"    // Function prototypes for all modules
#include "LCD.h"          // LCD control functions and constants
#include "ports.h"        // Port pin definitions
#include "macros.h"       // Project-wide #define constants

//==============================================================================
// Function Prototypes
//==============================================================================
void main(void);
void Init_Conditions(void);
void Display_Process(void);
void Init_LEDs(void);
void Run_Project5(void);

//==============================================================================
// Global Variables
//==============================================================================
volatile char slow_input_down;                  // Debounce flag for switch inputs
extern char display_line[4][11];                // LCD display buffer
extern char *display[4];                        // Pointers to each display line
unsigned char display_mode;                     // Display mode selector
extern volatile unsigned char display_changed;  // Flag: LCD content updated
extern volatile unsigned char update_display;   // Flag: timer says refresh LCD
extern volatile unsigned int update_display_count; // Counter for display timing
extern volatile unsigned int Time_Sequence;     // Master timer counter from ISR
extern volatile char one_time;                  // One-time execution flag

// Project 5 timing — these are incremented in the Timer ISR
volatile unsigned int p5_timer = 0;             // Counts ISR ticks for movement timing
volatile unsigned int p5_running = 0;           // Flag: is the P5 timer counting?

// Project 5 state
unsigned int p5_step = 0;                       // Which step of the sequence (0 = not started)
char p5_started = 0;                            // Has SW1 been pressed to start?
char step_init = 1;                             // Flag: first entry into current step

unsigned int test_value;
char chosen_direction;
char change;

//==============================================================================
// Function: main
//==============================================================================
void main(void){

    PM5CTL0 &= ~LOCKLPM5;    // Disable GPIO power-on high-impedance mode

    Init_Ports();             // Configure all GPIO pins
    Init_Clocks();            // Configure clock system (8 MHz MCLK)
    Init_Conditions();        // Initialize variables, enable interrupts
    Init_Timers();            // Set up Timer B0 (and others)
    Init_LCD();               // Initialize LCD display

    // Initial display — show we're ready
    strcpy(display_line[0], " Project5 ");
    strcpy(display_line[1], "          ");
    strcpy(display_line[2], " Press SW1");
    strcpy(display_line[3], " to Start ");
    display_changed = TRUE;

    // Make sure all motors start off
    Wheels_All_Off();

    //==========================================================================
    // Main loop — runs forever
    //==========================================================================
    while(ALWAYS){

        Display_Process();          // Refresh LCD when flagged

        Switches_Process();         // Check for SW1/SW2 button presses
                                    // SW1 handler should set p5_started = 1

        // Run the Project 5 sequence once started
        if(p5_started){
            Run_Project5();
        }

        P3OUT ^= TEST_PROBE;       // Toggle test probe (heartbeat)
    }
}

//==============================================================================
// Function: Run_Project5
// Description: Sequential state machine for the Project 5 movement sequence.
//              Each step initializes on first entry (step_init == 1), then
//              waits for p5_timer to reach the target count before moving on.
//
//              p5_timer is incremented in the Timer ISR when p5_running == 1.
//
// Sequence:
//   1. Forward 1 sec  ->  Pause 1 sec
//   2. Reverse 2 sec  ->  Pause 1 sec
//   3. Forward 1 sec  ->  Pause 1 sec
//   4. Spin CW 3 sec  ->  Pause 2 sec
//   5. Spin CCW 3 sec ->  Pause 2 sec
//   6. Done
//==============================================================================
void Run_Project5(void){

    switch(p5_step){

    //--- Step 1: Forward for 1 second -----------------------------------------
    case P5_FWD1:
        if(step_init){
            step_init = 0;
            strcpy(display_line[0], " Forward  ");
            strcpy(display_line[1], " 1 second ");
            strcpy(display_line[2], "          ");
            strcpy(display_line[3], "  Step 1  ");
            display_changed = 1;
            Wheels_All_Off();         // Safety: all off before direction change
            Forward_On();             // Both wheels forward
            p5_timer = 0;             // Reset the interrupt counter
            p5_running = 1;           // Start counting in ISR
        }
        if(p5_timer >= ONE_SEC){
            Wheels_All_Off();         // Stop moving
            p5_step = P5_PAUSE1;      // Go to pause
            step_init = 1;            // Next step needs initialization
        }
        break;

    //--- Pause 1 second -------------------------------------------------------
    case P5_PAUSE1:
        if(step_init){
            step_init = 0;
            strcpy(display_line[0], "  Pause   ");
            strcpy(display_line[1], " 1 second ");
            strcpy(display_line[3], "          ");
            display_changed = 1;
            p5_timer = 0;             // Reset counter for pause timing
        }
        if(p5_timer >= ONE_SEC){
            p5_step = P5_REV;
            step_init = 1;
        }
        break;

    //--- Step 2: Reverse for 2 seconds ----------------------------------------
    case P5_REV:
        if(step_init){
            step_init = 0;
            strcpy(display_line[0], " Reverse  ");
            strcpy(display_line[1], " 2 seconds");
            strcpy(display_line[3], "  Step 2  ");
            display_changed = 1;
            Reverse_On();             // Both wheels reverse (forward turned off inside)
            p5_timer = 0;
        }
        if(p5_timer >= TWO_SEC){
            Wheels_All_Off();
            p5_step = P5_PAUSE2;
            step_init = 1;
        }
        break;

    //--- Pause 1 second -------------------------------------------------------
    case P5_PAUSE2:
        if(step_init){
            step_init = 0;
            strcpy(display_line[0], "  Pause   ");
            strcpy(display_line[1], " 1 second ");
            strcpy(display_line[3], "          ");
            display_changed = 1;
            p5_timer = 0;
        }
        if(p5_timer >= ONE_SEC){
            p5_step = P5_FWD2;
            step_init = 1;
        }
        break;

    //--- Step 3: Forward for 1 second -----------------------------------------
    case P5_FWD2:
        if(step_init){
            step_init = 0;
            strcpy(display_line[0], " Forward  ");
            strcpy(display_line[1], " 1 second ");
            strcpy(display_line[3], "  Step 3  ");
            display_changed = 1;
            Forward_On();
            p5_timer = 0;
        }
        if(p5_timer >= ONE_SEC){
            Wheels_All_Off();
            p5_step = P5_PAUSE3;
            step_init = 1;
        }
        break;

    //--- Pause 1 second -------------------------------------------------------
    case P5_PAUSE3:
        if(step_init){
            step_init = 0;
            strcpy(display_line[0], "  Pause   ");
            strcpy(display_line[1], " 1 second ");
            strcpy(display_line[3], "          ");
            display_changed = 1;
            p5_timer = 0;
        }
        if(p5_timer >= ONE_SEC){
            p5_step = P5_SPIN_CW;
            step_init = 1;
        }
        break;

    //--- Step 4: Spin clockwise for 3 seconds ---------------------------------
    case P5_SPIN_CW:
        if(step_init){
            step_init = 0;
            strcpy(display_line[0], " Spin CW  ");
            strcpy(display_line[1], " 3 seconds");
            strcpy(display_line[3], "  Step 4  ");
            display_changed = 1;
            Spin_CW_On();             // Left fwd + Right rev
            p5_timer = 0;
        }
        if(p5_timer >= THREE_SEC){
            Wheels_All_Off();
            p5_step = P5_PAUSE4;
            step_init = 1;
        }
        break;

    //--- Pause 2 seconds -----------------------------------------------------
    case P5_PAUSE4:
        if(step_init){
            step_init = 0;
            strcpy(display_line[0], "  Pause   ");
            strcpy(display_line[1], " 2 seconds");
            strcpy(display_line[3], "          ");
            display_changed = 1;
            p5_timer = 0;
        }
        if(p5_timer >= TWO_SEC){
            p5_step = P5_SPIN_CCW;
            step_init = 1;
        }
        break;

    //--- Step 5: Spin counter-clockwise for 3 seconds -------------------------
    case P5_SPIN_CCW:
        if(step_init){
            step_init = 0;
            strcpy(display_line[0], " Spin CCW ");
            strcpy(display_line[1], " 3 seconds");
            strcpy(display_line[3], "  Step 5  ");
            display_changed = 1;
            Spin_CCW_On();            // Right fwd + Left rev
            p5_timer = 0;
        }
        if(p5_timer >= THREE_SEC){
            Wheels_All_Off();
            p5_step = P5_PAUSE5;
            step_init = 1;
        }
        break;

    //--- Pause 2 seconds -----------------------------------------------------
    case P5_PAUSE5:
        if(step_init){
            step_init = 0;
            strcpy(display_line[0], "  Pause   ");
            strcpy(display_line[1], " 2 seconds");
            strcpy(display_line[3], "          ");
            display_changed = 1;
            p5_timer = 0;
        }
        if(p5_timer >= TWO_SEC){
            p5_step = P5_DONE;
            step_init = 1;
        }
        break;

    //--- Done! ----------------------------------------------------------------
    case P5_DONE:
        Wheels_All_Off();
        p5_running = 0;               // Stop the ISR counter
        strcpy(display_line[0], "   Done!  ");
        strcpy(display_line[1], "          ");
        strcpy(display_line[2], "          ");
        strcpy(display_line[3], "          ");
        display_changed = 1;
        p5_started = 0;               // Prevent re-running
        break;

    default:
        Wheels_All_Off();
        break;
    }
}
