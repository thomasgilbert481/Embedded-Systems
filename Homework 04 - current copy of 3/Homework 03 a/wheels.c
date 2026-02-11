//==============================================================================
// File Name: wheels.c
// Description: Wheel motor control and shape state machines for Project 4
//              Implements circle, figure-8, and triangle shapes using
//              software PWM duty-cycling on the forward H-bridge pins.
// Author: Thomas Gilbert
// Date: Feb 11, 2026
// Compiler: Code Composer Studio
//==============================================================================

#include "msp430.h"       // MSP430 register definitions
#include "functions.h"    // Function prototypes
#include "macros.h"       // Project-wide #define constants
#include "ports.h"        // Port pin definitions (R_FORWARD, L_FORWARD, etc.)
#include <string.h>       // For strcpy() to write LCD strings

//==============================================================================
// External variables — defined elsewhere, used here
//==============================================================================
extern volatile unsigned int Time_Sequence;      // Incremented by timer ISR every ~10ms
extern char display_line[4][11];                 // LCD display buffer (4 lines, 10 chars + null)
extern volatile unsigned char display_changed;   // Flag to tell Display_Process to refresh LCD

//==============================================================================
// Shape state machine global variables
//==============================================================================
char event = NONE;                  // Which shape is currently requested (NONE/CIRCLE/FIGURE8/TRIANGLE)
char state = NONE;                  // Current state within the shape state machine (WAIT/START/RUN/END)
unsigned int cycle_time;            // Counts up each time Time_Sequence changes — used as PWM timebase
unsigned int time_change;           // Flag: set to 1 when Time_Sequence changes, cleared after processing
unsigned int Last_Time_Sequence;    // Stores previous Time_Sequence value to detect changes
unsigned int delay_start;           // Counter for the WAIT state delay (keeps fingers safe)
unsigned int segment_count;         // Counts how many PWM periods have completed — controls travel distance
unsigned int right_motor_count;     // Counts up within one PWM period for right motor duty cycle
unsigned int left_motor_count;      // Counts up within one PWM period for left motor duty cycle
unsigned int shape_phase;           // Tracks sub-phases within a shape (e.g., which leg of triangle)
unsigned int shape_repeat;          // Tracks how many times the shape has been repeated (unused but available)

//==============================================================================
// Function: Forward_On
// Description: Turns BOTH forward motors ON by setting their port pins HIGH.
//              Called at the beginning of movement to start both wheels.
//==============================================================================
void Forward_On(void) {
    P6OUT |= R_FORWARD;   // Set P6.1 HIGH — right wheel spins forward
    P6OUT |= L_FORWARD;   // Set P6.2 HIGH — left wheel spins forward
}

//==============================================================================
// Function: Forward_Off
// Description: Turns BOTH forward motors OFF by clearing their port pins LOW.
//              Called when a shape is complete to stop all movement.
//==============================================================================
void Forward_Off(void) {
    P6OUT &= ~R_FORWARD;  // Set P6.1 LOW — right wheel stops
    P6OUT &= ~L_FORWARD;  // Set P6.2 LOW — left wheel stops
}

//==============================================================================
// Function: Forward_Move
// Description: Re-enables both motors at the START of a new PWM cycle.
//              During duty-cycling, motors get turned off partway through
//              each period. This function turns them back on for the next period.
//==============================================================================
void Forward_Move(void) {
    P6OUT |= R_FORWARD;   // Turn right motor back on for new PWM cycle
    P6OUT |= L_FORWARD;   // Turn left motor back on for new PWM cycle
}

//==============================================================================
// Function: Update_Timers
// Description: Detects when Time_Sequence (incremented by timer ISR) has changed.
//              Each change represents one "tick" (~10ms depending on timer config).
//              Increments cycle_time and sets time_change flag so the state
//              machine knows a new tick has occurred.
//              MUST be called every iteration of the main while loop.
//==============================================================================
void Update_Timers(void) {
    if (Last_Time_Sequence != Time_Sequence) {  // Has the timer ISR incremented Time_Sequence?
        Last_Time_Sequence = Time_Sequence;     // Update our saved copy so we detect the NEXT change
        cycle_time++;                           // Increment the PWM period counter
        time_change = 1;                        // Set flag — the RUN states check this flag
    }
}

//==============================================================================
// Function: wait_case
// Description: WAIT state — provides a 2-3 second delay after a button press
//              before the car starts moving. This gives you time to move your
//              fingers out of the way of the wheels.
//              delay_start counts up each tick. When it reaches WAITING2START
//              (defined in macros.h, e.g. 50 ticks = ~2.5 sec), it transitions
//              to the START state.
//==============================================================================
void wait_case(void) {
    if (time_change) {                          // Only process on new timer ticks
        time_change = 0;                        // Clear the flag so we don't process twice
        if (delay_start++ >= WAITING2START) {   // Increment delay counter and check if done
            delay_start = 0;                    // Reset delay counter for next use
            state = START;                      // Transition to START state
        }
    }
}

//==============================================================================
// Function: start_case
// Description: START state — initializes all counters to zero and begins
//              forward movement. This is a one-shot state that immediately
//              transitions to RUN.
//==============================================================================
void start_case(void) {
    cycle_time = 0;                // Reset PWM period counter to zero
    right_motor_count = 0;         // Reset right motor duty cycle counter
    left_motor_count = 0;          // Reset left motor duty cycle counter
    segment_count = 0;             // Reset travel distance counter
    shape_phase = 0;               // Reset sub-phase counter (for fig-8 and triangle)
    Forward_On();                  // Turn on both motors to begin moving
    state = RUN;                   // Transition to RUN state — movement begins
}

//==============================================================================
// Function: end_case
// Description: END state — stops both motors, clears the state machine back
//              to NONE so no further movement occurs. Updates LCD to show "Done!"
//==============================================================================
void end_case(void) {
    Forward_Off();                             // Turn off both motors — car stops
    state = NONE;                              // Reset state to idle
    event = NONE;                              // Reset event to idle — allows new shape to be started
    strcpy(display_line[3], "  Done!   ");     // Write "Done!" to LCD line 4
    display_changed = TRUE;                    // Tell Display_Process to refresh the LCD
}

//==============================================================================
// Function: run_circle
// Description: RUN state for CIRCLE shape.
//
// HOW SOFTWARE PWM WORKS:
//   - Each "tick" (time_change), right_motor_count and left_motor_count increment
//   - When right_motor_count reaches CIRCLE_RIGHT_COUNT, the right motor is turned OFF
//   - When left_motor_count reaches CIRCLE_LEFT_COUNT, the left motor is turned OFF
//   - When cycle_time reaches WHEEL_COUNT_TIME, one full PWM period is complete:
//     both counters reset, segment_count increments, and motors turn back on
//   - The motor that has a LOWER count threshold runs for LESS of the period = SLOWER
//   - A slower right wheel makes the car curve to the RIGHT
//
// The car does 2 complete circles (total_travel = CIRCLE_TRAVEL * 2)
// Then transitions to END state.
//==============================================================================
void run_circle(void) {
    unsigned int total_travel = CIRCLE_TRAVEL * 2;  // 2 circles worth of segments

    if (time_change) {                              // Only act on new timer ticks
        time_change = 0;                            // Clear flag

        if (segment_count <= total_travel) {        // Still have distance to travel?
            // --- Software PWM duty cycle control ---
            if (right_motor_count++ >= CIRCLE_RIGHT_COUNT) {  // Right motor: increment and check
                P6OUT &= ~R_FORWARD;                          // Exceeded threshold — turn right motor OFF
            }
            if (left_motor_count++ >= CIRCLE_LEFT_COUNT) {    // Left motor: increment and check
                P6OUT &= ~L_FORWARD;                          // Exceeded threshold — turn left motor OFF
            }

            // --- End of one PWM period ---
            if (cycle_time >= WHEEL_COUNT_TIME) {   // Has one full PWM period elapsed?
                cycle_time = 0;                     // Reset period counter
                right_motor_count = 0;              // Reset right motor duty counter
                left_motor_count = 0;               // Reset left motor duty counter
                segment_count++;                    // One more segment of travel complete
                Forward_Move();                     // Turn both motors back ON for next period
            }
        } else {
            state = END;                            // All segments done — stop the car
        }
    }
}

//==============================================================================
// Function: run_figure8
// Description: RUN state for FIGURE-8 shape.
//
// A figure-8 is two circles in opposite directions:
//   Phase 0 (even): right motor slower => car curves RIGHT
//   Phase 1 (odd):  left motor slower  => car curves LEFT
//
// One figure-8 = 2 half-loops (phases 0 and 1)
// We do 2 figure-8s = 4 total half-loops (phases 0, 1, 2, 3)
//
// shape_phase tracks which half-loop we're on.
// segment_count tracks distance within each half-loop.
// When segment_count exceeds FIG8_HALF_TRAVEL, we advance to the next phase.
//==============================================================================
void run_figure8(void) {
    unsigned int right_limit;                       // Right motor duty threshold for current phase
    unsigned int left_limit;                        // Left motor duty threshold for current phase
    unsigned int total_phases = 4;                  // 2 figure-8s × 2 half-loops each = 4 phases

    if (time_change) {                              // Only act on new timer ticks
        time_change = 0;                            // Clear flag

        if (shape_phase < total_phases) {           // Still have phases left to do?

            // --- Select turning direction based on phase ---
            if (shape_phase % 2 == 0) {             // Even phases (0, 2): curve in direction A
                right_limit = FIG8_RIGHT_COUNT_A;   // Right motor slower (e.g., 7 out of 10)
                left_limit  = FIG8_LEFT_COUNT_A;    // Left motor faster (e.g., 10 out of 10)
            } else {                                // Odd phases (1, 3): curve in direction B
                right_limit = FIG8_RIGHT_COUNT_B;   // Right motor faster (e.g., 10 out of 10)
                left_limit  = FIG8_LEFT_COUNT_B;    // Left motor slower (e.g., 7 out of 10)
            }

            // --- Still traveling within this half-loop? ---
            if (segment_count <= FIG8_HALF_TRAVEL) {
                // Software PWM: turn off motor when its count exceeds threshold
                if (right_motor_count++ >= right_limit) {   // Right motor duty cycle check
                    P6OUT &= ~R_FORWARD;                    // Turn right motor OFF for rest of period
                }
                if (left_motor_count++ >= left_limit) {     // Left motor duty cycle check
                    P6OUT &= ~L_FORWARD;                    // Turn left motor OFF for rest of period
                }

                // End of PWM period
                if (cycle_time >= WHEEL_COUNT_TIME) {       // Full PWM period elapsed?
                    cycle_time = 0;                         // Reset period counter
                    right_motor_count = 0;                  // Reset right duty counter
                    left_motor_count = 0;                   // Reset left duty counter
                    segment_count++;                        // One more segment complete
                    Forward_Move();                         // Turn both motors back ON
                }
            } else {
                // --- This half-loop is complete, advance to next phase ---
                segment_count = 0;                  // Reset distance counter for next half-loop
                cycle_time = 0;                     // Reset PWM period counter
                right_motor_count = 0;              // Reset right duty counter
                left_motor_count = 0;               // Reset left duty counter
                shape_phase++;                      // Move to next phase (next half-loop)
                Forward_Move();                     // Ensure motors are on entering new phase
            }
        } else {
            state = END;                            // All 4 half-loops done — stop
        }
    }
}

//==============================================================================
// Function: run_triangle
// Description: RUN state for TRIANGLE shape.
//
// A triangle consists of 3 straight legs with 3 turns between them:
//   Phase 0: straight leg 1
//   Phase 1: turn (~120 degrees)
//   Phase 2: straight leg 2
//   Phase 3: turn (~120 degrees)
//   Phase 4: straight leg 3
//   Phase 5: turn (~120 degrees) — brings car back to start
//
// That's 6 phases per triangle. We do 2 triangles = 12 total phases.
//
// Even phases = straight (both wheels near-equal speed)
// Odd phases  = turn (right wheel much slower than left)
//
// For straight: slight speed difference is OK (the project says so).
// For turns: big speed difference creates a sharp turn.
//==============================================================================
void run_triangle(void) {
    unsigned int right_limit;                       // Right motor duty threshold
    unsigned int left_limit;                        // Left motor duty threshold
    unsigned int travel_limit;                      // How many segments for this phase
    unsigned int total_phases = 12;                 // 2 triangles × 6 phases each = 12

    if (time_change) {                              // Only act on new timer ticks
        time_change = 0;                            // Clear flag

        if (shape_phase < total_phases) {           // Still have phases left?

            // --- Determine if this phase is a straight leg or a turn ---
            if (shape_phase % 2 == 0) {             // Even phases: STRAIGHT
                right_limit  = TRI_STRAIGHT_RIGHT;  // Right motor duty (e.g., 9 out of 10)
                left_limit   = TRI_STRAIGHT_LEFT;   // Left motor duty (e.g., 10 out of 10)
                travel_limit = TRI_STRAIGHT_TRAVEL;  // How far to go straight (e.g., 40 segments)
            } else {                                // Odd phases: TURN
                right_limit  = TRI_TURN_RIGHT;      // Right motor much slower (e.g., 3 out of 10)
                left_limit   = TRI_TURN_LEFT;       // Left motor full speed (e.g., 10 out of 10)
                travel_limit = TRI_TURN_TRAVEL;     // How long to turn (e.g., 25 segments)
            }

            // --- Still traveling within this phase? ---
            if (segment_count <= travel_limit) {
                // Software PWM duty cycle control
                if (right_motor_count++ >= right_limit) {   // Right motor: check duty threshold
                    P6OUT &= ~R_FORWARD;                    // Turn right motor OFF
                }
                if (left_motor_count++ >= left_limit) {     // Left motor: check duty threshold
                    P6OUT &= ~L_FORWARD;                    // Turn left motor OFF
                }

                // End of one PWM period
                if (cycle_time >= WHEEL_COUNT_TIME) {       // Full period elapsed?
                    cycle_time = 0;                         // Reset period counter
                    right_motor_count = 0;                  // Reset right duty counter
                    left_motor_count = 0;                   // Reset left duty counter
                    segment_count++;                        // Another segment of this phase done
                    Forward_Move();                         // Re-enable both motors for next period
                }
            } else {
                // --- This phase (straight or turn) is complete ---
                segment_count = 0;                  // Reset distance for next phase
                cycle_time = 0;                     // Reset PWM counter
                right_motor_count = 0;              // Reset right duty counter
                left_motor_count = 0;               // Reset left duty counter
                shape_phase++;                      // Advance to next phase
                Forward_Move();                     // Ensure motors start ON in new phase
            }
        } else {
            state = END;                            // All 12 phases complete — stop
        }
    }
}

//==============================================================================
// Function: Run_Circle
// Description: Top-level state machine for circle shape.
//              Dispatches to the correct state handler based on current state.
//              State flow: WAIT -> START -> RUN -> END
//==============================================================================
void Run_Circle(void) {
    switch (state) {                    // Check current state
        case WAIT:  wait_case();   break;  // Delay before starting
        case START: start_case();  break;  // Initialize and begin movement
        case RUN:   run_circle();  break;  // Execute circle movement logic
        case END:   end_case();    break;  // Stop motors and clean up
        default: break;                    // Unknown state — do nothing
    }
}

//==============================================================================
// Function: Run_Figure8
// Description: Top-level state machine for figure-8 shape.
//              Same structure as Run_Circle but calls run_figure8 in RUN state.
//==============================================================================
void Run_Figure8(void) {
    switch (state) {                    // Check current state
        case WAIT:  wait_case();    break;  // Delay before starting
        case START: start_case();   break;  // Initialize and begin movement
        case RUN:   run_figure8();  break;  // Execute figure-8 movement logic
        case END:   end_case();     break;  // Stop motors and clean up
        default: break;                     // Unknown state — do nothing
    }
}

//==============================================================================
// Function: Run_Triangle
// Description: Top-level state machine for triangle shape.
//              Same structure as Run_Circle but calls run_triangle in RUN state.
//==============================================================================
void Run_Triangle(void) {
    switch (state) {                    // Check current state
        case WAIT:  wait_case();     break;  // Delay before starting
        case START: start_case();    break;  // Initialize and begin movement
        case RUN:   run_triangle();  break;  // Execute triangle movement logic
        case END:   end_case();      break;  // Stop motors and clean up
        default: break;                      // Unknown state — do nothing
    }
}

//==============================================================================
// Function: Run_Shapes
// Description: Master shape dispatcher — called every main loop iteration.
//              Checks which shape event is active and runs its state machine.
//              When event is NONE, this function does nothing (no shape active).
//==============================================================================
void Run_Shapes(void) {
    switch (event) {                            // Which shape are we running?
        case CIRCLE:                            // Circle was requested
            Run_Circle();                       // Run the circle state machine
            break;
        case FIGURE8:                           // Figure-8 was requested
            Run_Figure8();                      // Run the figure-8 state machine
            break;
        case TRIANGLE:                          // Triangle was requested
            Run_Triangle();                     // Run the triangle state machine
            break;
        default:                                // NONE or unknown — no shape active
            break;                              // Do nothing
    }
}

//==============================================================================
// Function: Start_Shape
// Description: Called when a button is pressed to initiate a shape.
//              Sets the event and state variables to kick off the state machine.
//              Also updates the LCD to display the name of the shape being made.
//              Only starts if no shape is currently running (event == NONE).
// Parameter: shape — which shape to start (CIRCLE, FIGURE8, or TRIANGLE)
//==============================================================================
void Start_Shape(char shape) {
    if (event == NONE) {                                // Only start if car is idle
        event = shape;                                  // Set the active shape event
        state = WAIT;                                   // Begin in WAIT state (safety delay)
        delay_start = 0;                                // Reset the delay counter

        // --- Update LCD to show which shape is being executed ---
        switch (shape) {
            case CIRCLE:                                // Circle selected
                strcpy(display_line[0], "  Circle  ");  // Write "Circle" to LCD line 1
                break;
            case FIGURE8:                               // Figure-8 selected
                strcpy(display_line[0], " Figure 8 ");  // Write "Figure 8" to LCD line 1
                break;
            case TRIANGLE:                              // Triangle selected
                strcpy(display_line[0], " Triangle ");  // Write "Triangle" to LCD line 1
                break;
        }
        strcpy(display_line[1], "          ");          // Clear LCD line 2
        strcpy(display_line[2], "          ");          // Clear LCD line 3
        strcpy(display_line[3], " Running  ");          // Write "Running" to LCD line 4
        display_changed = TRUE;                         // Flag LCD to refresh
    }
}
