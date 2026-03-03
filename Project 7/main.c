//******************************************************************************
// File Name:    main.c
// Description:  Main routine for Homework 06 -- Timer B0 and Switch Interrupts.
//
//               Built on Project 6 (ADC Black Line Detection).
//               New features in HW06:
//                 1. Timer B0 in continuous mode -- CCR0 fires every 200ms,
//                    toggling the LCD backlight at 2.5 Hz and triggering
//                    display updates.
//                 2. Interrupt-driven switch debouncing via CCR1 (SW1) and
//                    CCR2 (SW2) -- replaces software polling in Switches_Process.
//                 3. timerB0.obj removed; Init_Timer_B0 written in timers.c.
//
//               Timer tick basis: 200ms (CCR0 interval)
//                 ONE_SEC = 5 ticks, THREE_SEC = 15 ticks
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
extern volatile unsigned int Time_Sequence;     // Master timer counter from ISR
extern volatile char one_time;                  // One-time execution flag

// Project 5 timing (kept for compatibility -- not used in P6/HW06 sequence)
volatile unsigned int p5_timer   = RESET_STATE;
volatile unsigned int p5_running = FALSE;
unsigned int p5_step    = RESET_STATE;
char p5_started = FALSE;
char step_init  = TRUE;

// Project 6 state machine
volatile unsigned int p6_state   = P6_IDLE;    // Current state
volatile unsigned int p6_timer   = RESET_STATE; // State timer (incremented in CCR0 ISR)
volatile unsigned int p6_running = FALSE;       // TRUE = ISR increments p6_timer

// Which side first detected the line -- used to choose turn direction
// 0 = left triggered first, 1 = right triggered first
static unsigned int first_detect_side = RESET_STATE;

unsigned int test_value;
char chosen_direction;
char change;

//==============================================================================
// Function: main
// Description: System entry point. Initializes all peripherals, then loops
//              forever handling display updates, the P6 state machine, and
//              oscilloscope heartbeat.
//
//              Switch handling is entirely interrupt-driven (interrupt_ports.c).
//              Display is updated at most every 200ms, gated by update_display
//              (set by Timer0_B0_ISR in interrupts_timers.c).
//
// Globals used:    update_display, p6_state, display_changed
// Globals changed: display_line, display_changed, ir_emitter_on
// Local variables: none
//==============================================================================
void main(void){
//------------------------------------------------------------------------------

    PM5CTL0 &= ~LOCKLPM5;  // Unlock GPIO (required after reset on FR devices)

    Init_Ports();           // Configure all GPIO pins (ADC pins set in ports.c)
    Init_Clocks();          // Configure clock system (8 MHz MCLK/SMCLK)
    Init_Conditions();      // Initialize variables, enable global interrupts
    Init_Timers();          // Set up Timer B0 (200ms CCR0, CCR1/CCR2 for debounce)
    Init_LCD();             // Initialize LCD display (SPI)
    Init_ADC();             // Initialize 12-bit ADC, start cycling A2->A3->A5

    // Safety: ensure all motors start off
    Wheels_All_Off();

    // IR emitter ON from startup so idle screen shows meaningful L:/R: readings
    P2OUT |= IR_LED;
    ir_emitter_on = TRUE;

    // Initial display
    strcpy(display_line[0], " HW06     ");
    strcpy(display_line[1], "TimerB0+SW");
    strcpy(display_line[2], " Press SW1");
    strcpy(display_line[3], " to Start ");
    display_changed = TRUE;

    //==========================================================================
    // Main loop -- runs forever
    //==========================================================================
    while(ALWAYS){

        //----------------------------------------------------------------------
        // ADC display update: when the 200ms timer fires AND we are in a state
        // that shows live ADC readings, build the display buffer and mark it
        // dirty.  Do NOT clear update_display here -- Display_Process does it.
        //----------------------------------------------------------------------
        if(update_display && (p6_state == P6_IDLE || p6_state == P6_ALIGNED)){

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
            // update_display is intentionally NOT cleared here;
            // Display_Process() clears it and calls Display_Update()
        }

        //----------------------------------------------------------------------
        // Display_Process: clears update_display, writes LCD if display_changed.
        // Called AFTER ADC update so display_changed is already set when needed.
        //----------------------------------------------------------------------
        Display_Process();

        //----------------------------------------------------------------------
        // Project 6 black line detection state machine
        //----------------------------------------------------------------------
        Run_Project6();

        P3OUT ^= TEST_PROBE;  // Toggle test probe (oscilloscope heartbeat)
    }
//------------------------------------------------------------------------------
}

//==============================================================================
// Function: Run_Project6
// Description: State machine for the black line detection sequence.
//
//   P6_IDLE          -- Display ADC values; wait for SW1 (now handled in ISR)
//   P6_WAIT_1SEC     -- 1-second delay before driving forward (5 x 200ms ticks)
//   P6_FORWARD       -- Drive forward; monitor ADC for black line
//   P6_DETECTED_STOP -- Stopped; show "Black Line / Detected" for ~3 seconds
//   P6_TURNING       -- Turn to align detectors over the line
//   P6_ALIGNED       -- Final state; display live black line ADC values
//
//   p6_timer is incremented by Timer0_B0_ISR every 200ms when p6_running == 1.
//   Reset p6_timer = 0 whenever entering a new state.
//
// Globals used:    p6_state, p6_timer, p6_running, ADC_Left_Detect,
//                  ADC_Right_Detect, first_detect_side
// Globals changed: p6_state, p6_timer, p6_running, display_line, display_changed
// Local variables: none
//==============================================================================
void Run_Project6(void){
//------------------------------------------------------------------------------

    switch(p6_state){

    //--------------------------------------------------------------------------
    // IDLE: do nothing -- SW1 ISR (interrupt_ports.c) starts the sequence.
    //--------------------------------------------------------------------------
    case P6_IDLE:
        break;

    //--------------------------------------------------------------------------
    // WAIT_1SEC: 1-second countdown (5 x 200ms ticks) before moving forward.
    // IR emitter is already on from main startup (or re-enabled separately).
    //--------------------------------------------------------------------------
    case P6_WAIT_1SEC:
        if(!ir_emitter_on){
            P2OUT |= IR_LED;
            ir_emitter_on = TRUE;
        }
        if(p6_timer >= DETECT_DELAY_1SEC){
            p6_timer = RESET_STATE;
            p6_state = P6_FORWARD;

            Forward_On();

            strcpy(display_line[0], " Forward  ");
            strcpy(display_line[1], " Scanning ");
            strcpy(display_line[2], "  for line");
            strcpy(display_line[3], "          ");
            display_changed = TRUE;
        }
        break;

    //--------------------------------------------------------------------------
    // FORWARD: drive forward at full speed; monitor ADC for black line.
    // Detection: EITHER detector reads above BLACK_LINE_THRESHOLD.
    // Note: software PWM removed -- timer granularity is 200ms (too coarse).
    //--------------------------------------------------------------------------
    case P6_FORWARD:
        Forward_On();   // Full speed forward

        if((ADC_Left_Detect  > BLACK_LINE_THRESHOLD) ||
           (ADC_Right_Detect > BLACK_LINE_THRESHOLD)){

            Wheels_All_Off();   // Stop immediately

            // Record which side triggered first (determines turn direction)
            if(ADC_Left_Detect > ADC_Right_Detect){
                first_detect_side = RESET_STATE;  // Left detected stronger
            } else {
                first_detect_side = TRUE;          // Right detected stronger
            }

            p6_timer = RESET_STATE;
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
    // (~3 seconds at 200ms per tick) so the TA can confirm, then turn.
    //--------------------------------------------------------------------------
    case P6_DETECTED_STOP:
        if(p6_timer >= DETECT_STOP_TIME){
            p6_timer = RESET_STATE;
            p6_state = P6_TURNING;

            if(first_detect_side == RESET_STATE){
                Spin_CW_On();    // Left detected first -> spin right
            } else {
                Spin_CCW_On();   // Right detected first -> spin left
            }

            strcpy(display_line[0], " Turning  ");
            strcpy(display_line[1], "  to align");
            strcpy(display_line[2], "          ");
            strcpy(display_line[3], "          ");
            display_changed = TRUE;
        }
        break;

    //--------------------------------------------------------------------------
    // TURNING: spin for TURN_TIME ticks (~0.4 seconds), then stop.
    //--------------------------------------------------------------------------
    case P6_TURNING:
        if(p6_timer >= TURN_TIME){
            Wheels_All_Off();
            p6_running = FALSE;     // Stop the ISR counter
            p6_state   = P6_ALIGNED;

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
    // ALIGNED: final state -- main loop ADC block shows live detector readings.
    //--------------------------------------------------------------------------
    case P6_ALIGNED:
        break;

    default:
        Wheels_All_Off();   // Safety: unknown state -- stop motors
        break;
    }
//------------------------------------------------------------------------------
}
