//==============================================================================
// File Name: main.c
// Description: Main routine for Homework 4 — Motor Test Point Verification
//              Uses SW1 to cycle through motor on/off tests for voltmeter checks.
// Author: Thomas Gilbert
// Date: Feb 16, 2026
// Compiler: Code Composer Studio
//==============================================================================

#include "msp430.h"
#include <string.h>
#include "functions.h"
#include "LCD.h"
#include "ports.h"
#include "macros.h"

//==============================================================================
// Function Prototypes
//==============================================================================
void main(void);
void Init_Conditions(void);
void Display_Process(void);
void Init_LEDs(void);
void Motor_Safety_Check(void);

//==============================================================================
// Global Variables
//==============================================================================
volatile char slow_input_down;
extern char display_line[4][11];
extern char *display[4];
unsigned char display_mode;
extern volatile unsigned char display_changed;
extern volatile unsigned char update_display;
extern volatile unsigned int update_display_count;
extern volatile unsigned int Time_Sequence;
extern volatile char one_time;
unsigned int test_value;
char chosen_direction;
char change;

//==============================================================================
// Function: main
//==============================================================================
void main(void){

    PM5CTL0 &= ~LOCKLPM5;

    Init_Ports();
    Init_Clocks();
    Init_Conditions();
    Init_Timers();
    Init_LCD();

    // Initial display
    strcpy(display_line[0], " ALL OFF  ");
    strcpy(display_line[1], "  Ready   ");
    strcpy(display_line[2], "  HW 4   ");
    strcpy(display_line[3], "SW1=next  ");
    display_changed = TRUE;

    // Make sure all motors start off
    Motors_Off();

    while(ALWAYS) {

        Motor_Safety_Check();       // Step 12: prevent forward+reverse on same side

        Switches_Process();         // Check for SW1 press -> Motor_Test_Advance()

        Display_Process();          // Refresh LCD

        P3OUT ^= TEST_PROBE;       // Toggle test probe
    }
}
