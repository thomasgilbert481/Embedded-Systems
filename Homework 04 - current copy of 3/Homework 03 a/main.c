//==============================================================================
// File Name: main.c
// Description: Main routine for Project 4 — Shape-making car
//              Uses a "While" (Back Ground / Fore Ground) operating system.
//              Button presses trigger shapes: Circle, Figure-8, Triangle.
//              The car executes each shape using software PWM motor control.
// Author: Thomas Gilbert
// Date: Feb 11, 2026
// Compiler: Code Composer Studio
//==============================================================================

#include "msp430.h"       // MSP430 register definitions
#include <string.h>       // For strcpy() to write LCD display strings
#include "functions.h"    // Function prototypes for all modules
#include "LCD.h"          // LCD control functions and constants
#include "ports.h"        // Port pin definitions
#include "macros.h"       // Project-wide #define constants (states, shapes, timing)

//==============================================================================
// Function Prototypes — declared here so main() can call them
//==============================================================================
void main(void);                    // Main entry point
void Init_Conditions(void);         // Initialize variables and conditions
void Display_Process(void);         // Update LCD display when flagged
void Init_LEDs(void);               // Configure LED initial states
void Carlson_StateMachine(void);    // LED blinking state machine (from Project 3)
void Update_Timers(void);           // Detect timer ticks for shape state machine
void Run_Shapes(void);              // Master shape dispatcher
void Start_Shape(char shape);       // Initiate a shape from button press

//==============================================================================
// Global Variables
//==============================================================================
volatile char slow_input_down;                  // Debounce flag for switch inputs
extern char display_line[4][11];                // LCD display buffer — 4 lines of 10 chars + null
extern char *display[4];                        // Pointers to each display line
unsigned char display_mode;                     // Display mode selector (unused currently)
extern volatile unsigned char display_changed;  // Flag: set to 1 when LCD content has been updated
extern volatile unsigned char update_display;   // Flag: set by timer ISR to trigger display refresh
extern volatile unsigned int update_display_count;  // Counter for display update timing
extern volatile unsigned int Time_Sequence;     // Master timer counter — incremented by Timer ISR
extern volatile char one_time;                  // One-shot flag for Carlson state machine
unsigned int test_value;                        // General purpose test variable
char chosen_direction;                          // Direction selector (unused in Project 4)
char change;                                    // Change flag (unused in Project 4)

unsigned int wheel_move;                        // Wheel movement flag (legacy from Project 3)
char forward;                                   // Forward flag (legacy from Project 3)

// --- Project 4 additions: shape state machine variables (defined in wheels.c) ---
extern char event;                              // Current shape event (NONE/CIRCLE/FIGURE8/TRIANGLE)
extern char state;                              // Current state within shape (WAIT/START/RUN/END)
extern unsigned int Last_Time_Sequence;         // Previous Time_Sequence value for change detection
extern unsigned int time_change;                // Flag: 1 when a new timer tick occurred

//==============================================================================
// Function: main
// Description: Program entry point. Initializes all hardware subsystems,
//              then enters the infinite "while" operating system loop.
//              Each iteration of the loop:
//                1. Checks for timer changes (Update_Timers)
//                2. Runs the shape state machine if active (Run_Shapes)
//                3. Runs the LED state machine (Carlson_StateMachine)
//                4. Checks for button presses (Switches_Process)
//                5. Refreshes the LCD if needed (Display_Process)
//                6. Toggles the test probe for timing measurement
//==============================================================================
void main(void){

    //--------------------------------------------------------------------------
    // Hardware Initialization — runs once at power-on/reset
    //--------------------------------------------------------------------------
    PM5CTL0 &= ~LOCKLPM5;      // Disable GPIO high-impedance mode (required after reset)
                                // Without this, port settings configured below won't take effect

    Init_Ports();               // Configure all GPIO pins (P1-P6) per hardware schematic
    Init_Clocks();              // Set up clock system: MCLK=8MHz, SMCLK=8MHz, ACLK=32768Hz
    Init_Conditions();          // Initialize global variables, display buffers, enable interrupts
    Init_Timers();              // Configure Timer B0 (and others) for periodic interrupts
    Init_LCD();                 // Initialize the LCD controller via SPI

    //--------------------------------------------------------------------------
    // Set initial LCD display content
    //--------------------------------------------------------------------------
    strcpy(display_line[0], "   NCSU   ");   // Line 1: "NCSU" centered
    strcpy(display_line[1], " WOLFPACK ");   // Line 2: "WOLFPACK" centered
    strcpy(display_line[2], "  ECE306  ");   // Line 3: "ECE306" centered
    strcpy(display_line[3], " Press SW ");   // Line 4: Prompt user to press switch
    display_changed = TRUE;                  // Flag LCD to refresh with new content

    //--------------------------------------------------------------------------
    // Initialize Project 4 variables
    //--------------------------------------------------------------------------
    wheel_move = 0;                 // No wheel movement at startup
    forward = TRUE;                 // Default direction is forward
    Last_Time_Sequence = 0;         // Initialize timer change detection variable
    // event and state are initialized to NONE in wheels.c
    // The car will NOT move until a button is pressed

    //--------------------------------------------------------------------------
    // Beginning of the "While" Operating System
    // This loop runs forever. Each iteration processes all system tasks.
    // The loop runs as fast as the CPU allows (no OS scheduler — bare metal).
    //--------------------------------------------------------------------------
    while(ALWAYS) {                         // ALWAYS = 1, so this loops forever

        Update_Timers();                    // Step 1: Check if Time_Sequence changed
                                            //   If yes: increment cycle_time, set time_change = 1
                                            //   This drives the shape state machine's timing

        Run_Shapes();                       // Step 2: Run the active shape state machine
                                            //   If event == NONE, this does nothing
                                            //   If a shape is active, it processes WAIT/START/RUN/END
                                            //   The RUN state uses software PWM to control wheel speeds

        Carlson_StateMachine();             // Step 3: Run the LED blinking state machine
                                            //   Toggles red/green LEDs based on Time_Sequence
                                            //   Visual indicator that the system is running

        Switches_Process();                 // Step 4: Check for button presses (SW1 and SW2)
                                            //   If a button was pressed, it triggers Start_Shape()
                                            //   which sets event and state to begin a shape

        Display_Process();                  // Step 5: If display update is due, refresh the LCD
                                            //   Checks update_display flag (set by timer ISR)
                                            //   If display_changed is also set, writes buffer to LCD

        P3OUT ^= TEST_PROBE;               // Step 6: Toggle test probe pin (P3.0)
                                            //   This creates a square wave on the test point
                                            //   Useful for measuring main loop execution speed
                                            //   with an oscilloscope
    }
    //--------------------------------------------------------------------------
    //
    //--------------------------------------------------------------------------
}
