//==============================================================================
// File Name: main.c
// Description: Main routine for Project 6 -- ADC Black Line Detection.
//
//              New features over Project 5:
//                1. Thumbwheel ADC reading displayed on LCD (validates ADC).
//                2. IR emitter + left/right phototransistor detectors used to
//                   detect a black line on a white surface.
//
//              Demo sequence:
//                - Press SW1 -> 1-second delay -> car drives forward
//                - When EITHER detector reads above BLACK_LINE_THRESHOLD, stop
//                - Display "Black Line / Detected" for ~3 seconds
//                - Turn until both detectors are over the line
//                - Display live black line ADC values
//
//              Timer: Timer_B0 fires every 5ms (200 Hz)
//                     ONE_SEC = 200 ticks
//
// Author: Thomas Gilbert
// Date: Mar 2026
// Compiler: Code Composer Studio
//==============================================================================

#include "msp430.h"
#include <string.h>
#include "functions.h"
#include "LCD.h"
#include "ports.h"
#include "macros.h"
#include "adc.h"

//==============================================================================
// Function Prototypes (local to main.c)
//==============================================================================
void main(void);
void Run_Project6(void);

//==============================================================================
// Global Variables
//==============================================================================
volatile char slow_input_down;                  // Debounce flag (legacy)
extern char display_line[4][11];                // LCD display buffer (4 lines x 10 chars)
extern char *display[4];                        // Pointers to each display line
unsigned char display_mode;                     // Display mode selector
extern volatile unsigned char display_changed;  // Flag: LCD content updated
extern volatile unsigned char update_display;   // Flag: timer says refresh LCD
extern volatile unsigned int update_display_count; // Counter for display timing
extern volatile unsigned int Time_Sequence;     // Master timer counter from ISR
extern volatile char one_time;                  // One-time execution flag

// Project 5 timing (kept for compatibility -- not used in P6 sequence)
volatile unsigned int p5_timer   = 0;
volatile unsigned int p5_running = 0;
unsigned int p5_step    = 0;
char p5_started = 0;
char step_init  = 1;

// Project 6 state machine
volatile unsigned int p6_state   = P6_IDLE;    // Current state
volatile unsigned int p6_timer   = 0;           // State timer (incremented in ISR)
volatile unsigned int p6_running = 0;           // 1 = ISR increments p6_timer

// Which side first detected the line -- used to choose turn direction
// 0 = left triggered first, 1 = right triggered first
static unsigned int first_detect_side = 0;

unsigned int test_value;
char chosen_direction;
char change;

//==============================================================================
// Function: main
//==============================================================================
void main(void){

    PM5CTL0 &= ~LOCKLPM5;  // Unlock GPIO (required after reset on FR devices)

    Init_Ports();           // Configure all GPIO pins (ADC pins set in ports.c)
    Init_Clocks();          // Configure clock system (8 MHz MCLK/SMCLK)
    Init_Conditions();      // Initialize variables, enable interrupts
    Init_Timers();          // Set up Timer B0 (5ms tick, 200 Hz)
    Init_LCD();             // Initialize LCD display (SPI)
    Init_ADC();             // Initialize 12-bit ADC, start cycling A2->A3->A5

    // Safety: ensure all motors start off
    Wheels_All_Off();

    // IR emitter ON from startup -- needed so IDLE calibration screen shows
    // meaningful L:/R: readings (phototransistors require active IR illumination)
    P2OUT |= IR_LED;
    ir_emitter_on = 1;

    // Initial display
    strcpy(display_line[0], " Project6 ");
    strcpy(display_line[1], "ADC+IRLine");
    strcpy(display_line[2], " Press SW1");
    strcpy(display_line[3], " to Start ");
    display_changed = TRUE;

    //==========================================================================
    // Main loop -- runs forever
    //==========================================================================
    while(ALWAYS){

        Display_Process();    // Refresh LCD when flagged by timer ISR

        Switches_Process();   // Poll SW1 (start P6) and SW2 (emergency stop)

        Run_Project6();       // Execute the black line detection state machine

        // Update ADC display during IDLE and ALIGNED states so the operator
        // can verify detector readings before and after the run
        if(update_display && (p6_state == P6_IDLE || p6_state == P6_ALIGNED)){
            update_display = 0;

            // Line 0: Left detector  -- format "L:XXXX    "
            HexToBCD((int)ADC_Left_Detect);
            display_line[0][0] = 'L';
            display_line[0][1] = ':';
            display_line[0][2] = thousands;
            display_line[0][3] = hundreds;
            display_line[0][4] = tens;
            display_line[0][5] = ones;
            display_line[0][6] = ' ';
            display_line[0][7] = ' ';
            display_line[0][8] = ' ';
            display_line[0][9] = ' ';
            display_line[0][10] = '\0';

            // Line 1: Right detector -- format "R:XXXX    "
            HexToBCD((int)ADC_Right_Detect);
            display_line[1][0] = 'R';
            display_line[1][1] = ':';
            display_line[1][2] = thousands;
            display_line[1][3] = hundreds;
            display_line[1][4] = tens;
            display_line[1][5] = ones;
            display_line[1][6] = ' ';
            display_line[1][7] = ' ';
            display_line[1][8] = ' ';
            display_line[1][9] = ' ';
            display_line[1][10] = '\0';

            // Line 2: Thumbwheel -- format "T:XXXX    "
            HexToBCD((int)ADC_Thumb);
            display_line[2][0] = 'T';
            display_line[2][1] = ':';
            display_line[2][2] = thousands;
            display_line[2][3] = hundreds;
            display_line[2][4] = tens;
            display_line[2][5] = ones;
            display_line[2][6] = ' ';
            display_line[2][7] = ' ';
            display_line[2][8] = ' ';
            display_line[2][9] = ' ';
            display_line[2][10] = '\0';

            // Line 3: IR emitter status -- "IR:ON " or "IR:OFF"
            display_line[3][0] = 'I';
            display_line[3][1] = 'R';
            display_line[3][2] = ':';
            if(ir_emitter_on){
                display_line[3][3] = 'O';
                display_line[3][4] = 'N';
                display_line[3][5] = ' ';
            } else {
                display_line[3][3] = 'O';
                display_line[3][4] = 'F';
                display_line[3][5] = 'F';
            }
            display_line[3][6]  = ' ';
            display_line[3][7]  = ' ';
            display_line[3][8]  = ' ';
            display_line[3][9]  = ' ';
            display_line[3][10] = '\0';

            display_changed = TRUE;
        }

        P3OUT ^= TEST_PROBE;  // Toggle test probe (oscilloscope heartbeat)
    }
}

//==============================================================================
// Function: Run_Project6
// Description: State machine for the black line detection sequence.
//
//   P6_IDLE          -- Display ADC values; wait for SW1 (handled in switches.c)
//   P6_WAIT_1SEC     -- 1-second delay before driving forward
//   P6_FORWARD       -- Drive forward; monitor ADC for black line
//   P6_DETECTED_STOP -- Stopped; show "Black Line / Detected" for ~3 seconds
//   P6_TURNING       -- Turn to align detectors over the line
//   P6_ALIGNED       -- Final state; display live black line ADC values
//
//   IR emitter is turned ON when leaving P6_WAIT_1SEC and stays ON during
//   forward motion and turning.  It is turned OFF on SW2 (emergency stop).
//
//   p6_timer is incremented by Timer0_B0_ISR every 5ms when p6_running == 1.
//   Reset p6_timer = 0 whenever entering a new state.
//==============================================================================
void Run_Project6(void){

    switch(p6_state){

    //--------------------------------------------------------------------------
    // IDLE: do nothing -- SW1 handler sets state to P6_WAIT_1SEC
    //--------------------------------------------------------------------------
    case P6_IDLE:
        // ADC display is handled in the main loop when update_display is set.
        // No motor action here.
        break;

    //--------------------------------------------------------------------------
    // WAIT_1SEC: 1-second countdown before moving forward
    // IR emitter is turned on here so it has ~1 second of warm-up time before
    // we rely on its readings for black line detection.
    //--------------------------------------------------------------------------
    case P6_WAIT_1SEC:
        if(!ir_emitter_on){
            // Ensure IR emitter is on (guards against timer race on entry)
            P2OUT |= IR_LED;
            ir_emitter_on = 1;
        }
        if(p6_timer >= DETECT_DELAY_1SEC){
            p6_timer = 0;
            p6_state = P6_FORWARD;

            // Begin moving forward
            Forward_On();

            strcpy(display_line[0], " Forward  ");
            strcpy(display_line[1], " Scanning ");
            strcpy(display_line[2], "  for line");
            strcpy(display_line[3], "          ");
            display_changed = TRUE;
        }
        break;

    //--------------------------------------------------------------------------
    // FORWARD: drive forward and monitor detectors for the black line.
    // Detection condition: EITHER detector reads above BLACK_LINE_THRESHOLD.
    // (At least one sensor crossing the line is sufficient to trigger.)
    //--------------------------------------------------------------------------
    case P6_FORWARD:
        if((ADC_Left_Detect  > BLACK_LINE_THRESHOLD) ||
           (ADC_Right_Detect > BLACK_LINE_THRESHOLD)){

            // STOP immediately -- H-bridge safety: all-off before any state change
            Wheels_All_Off();

            // Record which side triggered first (for turn direction decision)
            if(ADC_Left_Detect > ADC_Right_Detect){
                first_detect_side = 0;  // Left side detected stronger signal
            } else {
                first_detect_side = 1;  // Right side detected stronger signal
            }

            p6_timer = 0;
            p6_state = P6_DETECTED_STOP;

            strcpy(display_line[0], "Black Line");
            strcpy(display_line[1], " Detected ");
            strcpy(display_line[2], "          ");
            strcpy(display_line[3], "          ");
            display_changed = TRUE;
        }
        break;

    //--------------------------------------------------------------------------
    // DETECTED_STOP: display "Black Line / Detected" for DETECT_STOP_TIME ticks
    // (~3 seconds) so the TA can confirm, then transition to turning.
    //--------------------------------------------------------------------------
    case P6_DETECTED_STOP:
        if(p6_timer >= DETECT_STOP_TIME){
            p6_timer = 0;
            p6_state = P6_TURNING;

            // Turn based on which detector saw the line most strongly.
            // Goal: rotate so BOTH detectors end up over the line.
            // If left triggered first, spin right (rotate car left);
            // if right triggered first, spin left (rotate car right).
            if(first_detect_side == 0){
                Spin_CW_On();   // Spin right to bring left side over line
            } else {
                Spin_CCW_On();  // Spin left to bring right side over line
            }

            strcpy(display_line[0], " Turning  ");
            strcpy(display_line[1], "  to align");
            strcpy(display_line[2], "          ");
            strcpy(display_line[3], "          ");
            display_changed = TRUE;
        }
        break;

    //--------------------------------------------------------------------------
    // TURNING: spin until p6_timer reaches TURN_TIME, then stop and declare
    // aligned.  TUNE TURN_TIME in macros.h based on your hardware.
    //--------------------------------------------------------------------------
    case P6_TURNING:
        if(p6_timer >= TURN_TIME){
            Wheels_All_Off();
            p6_running = 0;     // Stop the ISR counter -- sequence is complete
            p6_state   = P6_ALIGNED;

            // Build "aligned" display showing final detector readings
            HexToBCD((int)ADC_Left_Detect);
            display_line[2][0] = 'L';
            display_line[2][1] = ':';
            display_line[2][2] = thousands;
            display_line[2][3] = hundreds;
            display_line[2][4] = tens;
            display_line[2][5] = ones;
            display_line[2][6] = ' ';
            display_line[2][7] = ' ';
            display_line[2][8] = ' ';
            display_line[2][9] = ' ';
            display_line[2][10] = '\0';

            HexToBCD((int)ADC_Right_Detect);
            display_line[3][0] = 'R';
            display_line[3][1] = ':';
            display_line[3][2] = thousands;
            display_line[3][3] = hundreds;
            display_line[3][4] = tens;
            display_line[3][5] = ones;
            display_line[3][6] = ' ';
            display_line[3][7] = ' ';
            display_line[3][8] = ' ';
            display_line[3][9] = ' ';
            display_line[3][10] = '\0';

            strcpy(display_line[0], "Black Line");
            strcpy(display_line[1], " Aligned  ");
            display_changed = TRUE;
        }
        break;

    //--------------------------------------------------------------------------
    // ALIGNED: final state -- the main loop's update_display handler shows
    // live ADC readings continuously.  No motor action.
    //--------------------------------------------------------------------------
    case P6_ALIGNED:
        // Live ADC updates handled in the main loop (update_display block).
        break;

    default:
        Wheels_All_Off();   // Safety: unknown state -- stop motors
        break;
    }
}

// Display_Process() is defined in display.c
// Init_Conditions() is defined in init.c
// Init_LEDs()       is defined in LED.c
