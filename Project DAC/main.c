//******************************************************************************
// File Name:    main.c
// Description:  Main routine for Project DAC -- DAC Motor Power Demo.
//
//               Demonstrates that the DAC board successfully powers the motors
//               using the MSP430FR2355's SAC3 12-bit DAC in buffer mode with
//               the internal 2.5V reference.
//
//               Demo sequence:
//                 1. Boot: LCD shows "DAC Demo / Ready / Press SW1 / to Start"
//                 2. Press SW1 --> DAC_STARTUP: 600ms settling delay
//                 3. DAC_ENB enabled, both motors drive forward (Forward_On)
//                 4. DAC_DRIVE: CCR0 ISR steps DAC_data from 4000 down by 50
//                    each 200ms tick until DAC_data reaches DAC_MIN_VALUE (1200)
//                    -- approximately 56 steps = ~11 seconds of forward motion
//                 5. DAC_DONE: motors stop, LCD shows "Done / Press SW1 to repeat"
//                 6. Press SW2 at any time for immediate emergency stop
//
//               DAC voltage estimate (Vref = 2.5V):
//                 Vout_dac = 2.5V * n / 4096
//                 n=4000 --> ~2.44V (amplified by DAC board to high motor voltage)
//                 n=1200 --> ~0.73V (DAC board output ~6V per instructor)
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
void Run_DAC_Demo(void);

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

// DAC demo state machine (Project DAC)
// dac_state and dac_timer are also accessed by interrupts_timers.c and
// interrupt_ports.c -- all variables are volatile for ISR safety.
// All accesses are 16-bit and therefore atomic on the MSP430.
volatile unsigned int dac_state   = DAC_IDLE;    // Current demo state
volatile unsigned int dac_timer   = RESET_STATE; // Tick counter (incremented by CCR0 ISR)
volatile unsigned int dac_running = FALSE;        // TRUE = ISR increments dac_timer

//==============================================================================
// Function: main
// Description: System entry point.  Initializes all peripherals in the correct
//              order, then loops forever servicing the display and DAC demo
//              state machine.
//
//              Initialization order:
//                1. PM5CTL0  -- unlock GPIO (must be first on FR devices)
//                2. Init_REF -- 2.5V reference (blocking; before interrupts)
//                3. Init_Ports -- GPIO configuration
//                4. Init_Clocks -- 8 MHz MCLK/SMCLK
//                5. Init_Conditions -- variables + enable global interrupts
//                6. Init_Timers -- Timer B0 CCR0/CCR1/CCR2
//                7. Init_LCD   -- SPI display
//                8. Init_ADC   -- 12-bit ADC (channels A2, A3, A5)
//                9. Init_DAC   -- SAC3 buffer mode + DAC_ENB = LOW (safe startup)
//
//              Switch handling is interrupt-driven (interrupt_ports.c).
//              DAC data is stepped by Timer0_B0_ISR (interrupts_timers.c).
//              State transitions are handled by Run_DAC_Demo() in this file.
//
// Globals used:    update_display, dac_state, display_changed
// Globals changed: display_line, display_changed
// Local variables: none
//==============================================================================
void main(void){
//------------------------------------------------------------------------------

    PM5CTL0 &= ~LOCKLPM5;  // Unlock GPIO (required after reset on FR devices)

    // Init_REF must run before interrupts are enabled -- it contains a blocking
    // busy-wait (while REFGENRDY is not set) that must complete uninterrupted.
    Init_REF();             // Configure internal 2.5V reference; wait for stable

    Init_Ports();           // Configure all GPIO pins
    Init_Clocks();          // Configure clock system (8 MHz MCLK/SMCLK)
    Init_Conditions();      // Initialize variables, enable global interrupts
    Init_Timers();          // Set up Timer B0 (200ms CCR0, CCR1/CCR2 for debounce)
    Init_LCD();             // Initialize LCD display (SPI)
    Init_ADC();             // Initialize 12-bit ADC, start cycling A2->A3->A5

    // Init_DAC configures SAC3 hardware and asserts DAC_ENB LOW.
    // DAC_ENB goes HIGH only in Run_DAC_Demo() after the startup settling delay.
    Init_DAC();

    // Safety: ensure all motors start off before any DAC output is enabled
    Wheels_All_Off();

    // Initial display -- prompt user to press SW1
    strcpy(display_line[0], " DAC Demo ");
    strcpy(display_line[1], "  Ready   ");
    strcpy(display_line[2], "Press SW1 ");
    strcpy(display_line[3], " to Start ");
    display_changed = TRUE;

    //==========================================================================
    // Main loop -- runs forever
    //==========================================================================
    while(ALWAYS){

        //----------------------------------------------------------------------
        // Display_Process: writes LCD if display_changed; clears update_display.
        //----------------------------------------------------------------------
        Display_Process();

        //----------------------------------------------------------------------
        // DAC demo state machine
        //----------------------------------------------------------------------
        Run_DAC_Demo();

        P3OUT ^= TEST_PROBE;  // Toggle test probe (oscilloscope heartbeat)
    }
//------------------------------------------------------------------------------
}

//==============================================================================
// Function: Run_DAC_Demo
// Description: State machine for the DAC motor power demonstration.
//
//   DAC_IDLE    -- Waiting for SW1 press (SW1 ISR in interrupt_ports.c
//                  transitions to DAC_STARTUP and sets dac_running = TRUE).
//
//   DAC_STARTUP -- 600ms settling delay (3 x 200ms CCR0 ticks) to allow
//                  the DAC reference and output to stabilize before enabling
//                  the DAC board and starting the motors.
//                  Transitions: dac_timer >= DAC_STARTUP_TICKS
//                    -> enable DAC_ENB (P2.5 HIGH)
//                    -> Forward_On() (both motors forward via H-bridge)
//                    -> dac_state = DAC_DRIVE
//
//   DAC_DRIVE   -- Motors running.  CCR0 ISR steps DAC_data down by
//                  DAC_STEP (50 codes) each 200ms tick and updates SAC3DAT.
//                  This state only watches for the stop condition.
//                  Transitions: DAC_data <= DAC_MIN_VALUE (1200)
//                    -> Wheels_All_Off()
//                    -> DAC_ENB LOW (remove motor power from DAC board)
//                    -> dac_state = DAC_DONE
//
//   DAC_DONE    -- Demo complete; motors stopped.  LCD prompts for repeat.
//                  SW1 ISR restarts the demo; SW2 provides emergency stop.
//
// Globals used:    dac_state, dac_timer, dac_running, DAC_data
// Globals changed: dac_state, dac_running, display_line, display_changed
// Local variables: none
//==============================================================================
void Run_DAC_Demo(void){
//------------------------------------------------------------------------------

    switch(dac_state){

    //--------------------------------------------------------------------------
    // IDLE: waiting for SW1 press.
    // SW1 ISR (interrupt_ports.c) handles the transition to DAC_STARTUP.
    //--------------------------------------------------------------------------
    case DAC_IDLE:
        break;

    //--------------------------------------------------------------------------
    // STARTUP: wait for DAC reference and output to settle (3 x 200ms = 600ms),
    // then enable the DAC board and start both motors driving forward.
    //--------------------------------------------------------------------------
    case DAC_STARTUP:
        if(dac_timer >= DAC_STARTUP_TICKS){
            P2OUT  |= DAC_ENB;         // Enable DAC board (HIGH = enabled)
            Forward_On();              // Both motors forward via H-bridge pins
            dac_timer = RESET_STATE;   // Reset counter for DAC_DRIVE phase
            dac_state = DAC_DRIVE;

            strcpy(display_line[0], " DAC Demo ");
            strcpy(display_line[1], "Fwd Ramp  ");
            strcpy(display_line[2], "4000->1200");
            strcpy(display_line[3], "          ");
            display_changed = TRUE;
        }
        break;

    //--------------------------------------------------------------------------
    // DRIVE: CCR0 ISR steps DAC_data down each 200ms tick (interrupts_timers.c).
    // This state only watches for the minimum DAC value (stop condition).
    //--------------------------------------------------------------------------
    case DAC_DRIVE:
        if(DAC_data <= DAC_MIN_VALUE){
            Wheels_All_Off();          // Stop both motors immediately
            P2OUT      &= ~DAC_ENB;   // Disable DAC board (remove motor power)
            dac_running = FALSE;       // Stop ISR from stepping DAC_data further
            dac_state   = DAC_DONE;

            strcpy(display_line[0], " DAC Demo ");
            strcpy(display_line[1], "  Done!   ");
            strcpy(display_line[2], "Press SW1 ");
            strcpy(display_line[3], " to repeat");
            display_changed = TRUE;
        }
        break;

    //--------------------------------------------------------------------------
    // DONE: demo complete.
    // SW1 ISR restarts from here; SW2 ISR provides emergency stop from any state.
    //--------------------------------------------------------------------------
    case DAC_DONE:
        break;

    default:
        Wheels_All_Off();              // Safety: unknown state -- stop motors
        P2OUT      &= ~DAC_ENB;        // Disable DAC board
        dac_running = FALSE;
        dac_state   = DAC_IDLE;
        break;
    }
//------------------------------------------------------------------------------
}
