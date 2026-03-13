//==============================================================================
// File Name: main.c
// Description: Main routine for Project 7 -- Black Line Circle Following.
//
//              The car starts inside a 36-inch diameter circle made of black
//              electrical tape on white paper and executes the following sequence:
//                1. P7_IDLE       -- Wait for SW1. Display live ADC values.
//                2. P7_CALIBRATE  -- Three-phase calibration:
//                                    Ambient (emitter off), White, Black.
//                3. P7_WAIT_START -- Wait for SW1 to reposition car.
//                   P7_ARMED     -- 2-second countdown, then drive forward.
//                4. P7_FORWARD    -- Drive forward until black line detected.
//                5. P7_DETECTED_STOP -- Pause 1 sec on line.
//                6. P7_TURNING    -- Align both detectors over the line.
//                7. P7_FOLLOW_LINE -- Bang-bang line following for 2 laps.
//                8. P7_EXIT_TURN  -- Spin into circle center.
//                9. P7_DONE       -- Drive to center and stop. Timer frozen.
//
//              Timer: Timer B0 fires every 200ms (125kHz clock / 25000).
//                     ONE_SEC = 5 ticks.
//                     Each tick = 1 elapsed_tenth (0.2 seconds).
//
//              Motor control: Timer B3 hardware PWM via CCR1-CCR4.
//
// Author: Thomas Gilbert
// Date: Mar 2026
// Compiler: Code Composer Studio
// Target: MSP430FR2355
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
void Run_Project7(void);
void Update_P7_Display(void);

//==============================================================================
// Global Variables
//==============================================================================
volatile char slow_input_down;                  // Debounce flag (legacy)
extern char display_line[4][11];                // LCD display buffer
extern char *display[4];                        // Pointers to display lines
unsigned char display_mode;                     // Display mode selector
extern volatile unsigned char display_changed;  // Flag: LCD content updated
extern volatile unsigned char update_display;   // Flag: timer says refresh LCD
extern volatile unsigned int Time_Sequence;     // Master timer counter from ISR
extern volatile char one_time;                  // One-time execution flag

//------------------------------------------------------------------------------
// Project 7 state machine variables (defined here, extern'd in other files)
//------------------------------------------------------------------------------
volatile unsigned int p7_state   = P7_IDLE;    // Current state
volatile unsigned int p7_timer   = RESET_STATE; // State timer (200ms ticks)
volatile unsigned int p7_running = FALSE;       // TRUE = ISR increments p7_timer

// 0.2-second display clock
volatile unsigned int elapsed_tenths   = RESET_STATE; // Counts 0.2s increments
volatile unsigned int p7_timer_running = FALSE;        // TRUE = ISR increments elapsed_tenths

//------------------------------------------------------------------------------
// Calibration results (computed from calibration phases)
//------------------------------------------------------------------------------
unsigned int ambient_left   = RESET_STATE;  // Emitter OFF, left detector
unsigned int ambient_right  = RESET_STATE;  // Emitter OFF, right detector
unsigned int white_left     = RESET_STATE;  // White surface, left detector
unsigned int white_right    = RESET_STATE;  // White surface, right detector
unsigned int black_left     = RESET_STATE;  // Black tape, left detector
unsigned int black_right    = RESET_STATE;  // Black tape, right detector
unsigned int threshold_left  = RESET_STATE; // Midpoint: (white_left + black_left) / 2
unsigned int threshold_right = RESET_STATE; // Midpoint: (white_right + black_right) / 2

// Calibration sub-phase tracker
unsigned int calibrate_phase = CAL_AMBIENT;

// Circle direction: 0 = clockwise (exit turn CW), 1 = counter-clockwise (exit CCW)
unsigned int circle_direction = RESET_STATE;

// elapsed_tenths value when P7_FOLLOW_LINE was entered (for lap timing)
unsigned int follow_start_tenths = RESET_STATE;

//==============================================================================
// Function: main
//==============================================================================
void main(void){

    PM5CTL0 &= ~LOCKLPM5;  // Unlock GPIO (required after reset on FR devices)

    Init_Ports();           // Configure all GPIO (motor pins set for Timer B3)
    Init_Clocks();          // Configure clock system (8 MHz MCLK/SMCLK)
    Init_Conditions();      // Initialize variables, enable global interrupts
    Init_Timers();          // Timer B0 (200ms) + Timer B3 (hardware PWM)
    Init_LCD();             // Initialize LCD display (SPI)
    Init_ADC();             // Initialize 12-bit ADC, start cycling A2->A3->A5

    // Init_DAC MUST come after Init_Timers -- it enables the Timer B0 overflow
    // interrupt (TBIE) to begin the DAC ramp-down sequence (DAC_Begin -> DAC_Adjust).
    // RED LED turns ON here and OFF when the ramp completes (~10 seconds).
    Init_DAC();

    // Safety: ensure all motors start off via hardware PWM
    Wheels_All_Off();

    // IR emitter OFF at startup -- saves battery; turned on during calibration
    P2OUT &= ~IR_LED;
    ir_emitter_on = FALSE;

    // LCD backlight ON (no blinking)
    P6OUT |= LCD_BACKLITE;

    // Initial display
    strcpy(display_line[0], " Project7 ");
    strcpy(display_line[1], "  Circle  ");
    strcpy(display_line[2], " Follower ");
    strcpy(display_line[3], " Press SW1");
    display_changed = TRUE;

    //==========================================================================
    // Main loop -- runs forever
    //==========================================================================
    while(ALWAYS){

        // Update display when timer fires (every 200ms)
        if(update_display){
            Update_P7_Display();
            // Note: update_display is cleared by Display_Process() below
        }

        Display_Process();    // Refresh LCD when update_display is set

        Switches_Process();   // Stub -- switch handling is interrupt-driven

        Run_Project7();       // Execute the Project 7 state machine

        P3OUT ^= TEST_PROBE;  // Toggle test probe (oscilloscope heartbeat)
    }
}

//==============================================================================
// Function: Update_P7_Display
// Description: Updates display_line[] based on the current P7 state.
//              Called when update_display is set (every 200ms).
//              Sets display_changed to trigger LCD refresh.
//              Does NOT clear update_display -- Display_Process() handles that.
//
// Globals used:    p7_state, elapsed_tenths, ADC_Left_Detect, ADC_Right_Detect
// Globals changed: display_line, display_changed
// Local variables: whole_sec, frac, time_str
//==============================================================================
void Update_P7_Display(void){

    char time_str[11];
    unsigned int whole_sec;
    unsigned int frac;
    unsigned int i;                               // ULP 14.1: use unsigned int for index

    // Build the 0.2-second timer string for states where car is active
    // Format: "TTT.Ts    " where TTT = seconds (0-999), T = tenths (0/2/4/6/8)
    if(p7_state >= P7_FORWARD && p7_state <= P7_DONE){
        whole_sec = elapsed_tenths / 5;           // Integer seconds
        frac      = (elapsed_tenths % 5) * 2;     // Tenths digit: 0,2,4,6,8

        for(i = 9; i < 10; i--) time_str[i] = ' '; // ULP 13.1: count down
        time_str[10] = '\0';

        // Cap display at 999.8 seconds
        if(whole_sec > 999){ whole_sec = 999; }

        time_str[0] = (char)((whole_sec / 100) + '0');
        time_str[1] = (char)(((whole_sec / 10) % 10) + '0');
        time_str[2] = (char)((whole_sec % 10) + '0');
        time_str[3] = '.';
        time_str[4] = (char)(frac + '0');
        time_str[5] = 's';

        strcpy(display_line[3], time_str);
    }

    // State-specific top display lines
    switch(p7_state){

        case P7_IDLE:
            // Show live ADC values so operator can verify sensors
            HexToBCD((int)ADC_Left_Detect);
            display_line[0][0]  = 'L';
            display_line[0][1]  = ':';
            display_line[0][2]  = thousands;
            display_line[0][3]  = hundreds;
            display_line[0][4]  = tens;
            display_line[0][5]  = ones;
            display_line[0][6]  = ' ';
            display_line[0][7]  = ' ';
            display_line[0][8]  = ' ';
            display_line[0][9]  = ' ';
            display_line[0][10] = '\0';

            HexToBCD((int)ADC_Right_Detect);
            display_line[1][0]  = 'R';
            display_line[1][1]  = ':';
            display_line[1][2]  = thousands;
            display_line[1][3]  = hundreds;
            display_line[1][4]  = tens;
            display_line[1][5]  = ones;
            display_line[1][6]  = ' ';
            display_line[1][7]  = ' ';
            display_line[1][8]  = ' ';
            display_line[1][9]  = ' ';
            display_line[1][10] = '\0';

            strcpy(display_line[2], " Press SW1");
            strcpy(display_line[3], "  to cal  ");
            break;

        case P7_FOLLOW_LINE:
            // Show live ADC values during line following for debugging
            HexToBCD((int)ADC_Left_Detect);
            display_line[0][0]  = 'L';
            display_line[0][1]  = ':';
            display_line[0][2]  = thousands;
            display_line[0][3]  = hundreds;
            display_line[0][4]  = tens;
            display_line[0][5]  = ones;
            display_line[0][6]  = ' ';
            display_line[0][7]  = ' ';
            display_line[0][8]  = ' ';
            display_line[0][9]  = ' ';
            display_line[0][10] = '\0';

            HexToBCD((int)ADC_Right_Detect);
            display_line[1][0]  = 'R';
            display_line[1][1]  = ':';
            display_line[1][2]  = thousands;
            display_line[1][3]  = hundreds;
            display_line[1][4]  = tens;
            display_line[1][5]  = ones;
            display_line[1][6]  = ' ';
            display_line[1][7]  = ' ';
            display_line[1][8]  = ' ';
            display_line[1][9]  = ' ';
            display_line[1][10] = '\0';

            strcpy(display_line[2], " Circling ");
            // Line 3 = timer (set above)
            break;

        case P7_DONE:
            strcpy(display_line[0], " Complete ");
            strcpy(display_line[1], "  Stopped ");
            strcpy(display_line[2], "          ");
            // Line 3 keeps the frozen timer string
            break;

        default:
            // Other states set their display directly in Run_Project7()
            break;
    }

    display_changed = TRUE;
}

//==============================================================================
// Function: Run_Project7
// Description: State machine for the circle-following sequence.
//
//   p7_timer is incremented by Timer0_B0_ISR every 200ms when p7_running == 1.
//   Reset p7_timer = 0 whenever entering a new state.
//   elapsed_tenths is incremented every ISR tick when p7_timer_running == 1.
//
// Globals used:    p7_state, p7_timer, p7_running, elapsed_tenths,
//                  p7_timer_running, calibrate_phase, ADC_Left_Detect,
//                  ADC_Right_Detect, white_left, white_right, black_left,
//                  black_right, circle_direction, follow_start_tenths
// Globals changed: p7_state, p7_timer, p7_running, elapsed_tenths,
//                  p7_timer_running, calibrate_phase, threshold_left,
//                  threshold_right, black_left, black_right, white_left,
//                  white_right, ambient_left, ambient_right, circle_direction,
//                  follow_start_tenths, display_line, display_changed
//==============================================================================
void Run_Project7(void){

    switch(p7_state){

    //--------------------------------------------------------------------------
    // IDLE: Do nothing -- SW1 ISR (interrupt_ports.c) starts calibration.
    //       Update_P7_Display() shows live ADC values in IDLE.
    //--------------------------------------------------------------------------
    case P7_IDLE:
        break;

    //--------------------------------------------------------------------------
    // CALIBRATE: Three-phase IR calibration sequence.
    //   CAL_AMBIENT: emitter OFF, wait 1s, sample ambient
    //   CAL_WHITE:   emitter ON, wait 1s, sample white surface
    //   CAL_WAIT_BLACK: prompt user to place on black tape, wait for SW1
    //   CAL_BLACK:   wait 1s, sample black tape, compute thresholds
    //--------------------------------------------------------------------------
    case P7_CALIBRATE:
        switch(calibrate_phase){

            case CAL_AMBIENT:
                if(p7_timer == RESET_STATE){
                    P2OUT &= ~IR_LED;        // Emitter OFF for ambient measurement
                    ir_emitter_on = FALSE;
                    strcpy(display_line[0], "Calibrate ");
                    strcpy(display_line[1], " Ambient  ");
                    strcpy(display_line[2], " sampling ");
                    strcpy(display_line[3], "          ");
                    display_changed = TRUE;
                }
                if(p7_timer >= CAL_SAMPLE_DELAY){
                    ambient_left   = ADC_Left_Detect;
                    ambient_right  = ADC_Right_Detect;
                    p7_timer       = RESET_STATE;
                    calibrate_phase = CAL_WHITE;
                }
                break;

            case CAL_WHITE:
                if(p7_timer == RESET_STATE){
                    P2OUT |= IR_LED;         // Emitter ON for white surface measurement
                    ir_emitter_on = TRUE;
                    strcpy(display_line[0], "Calibrate ");
                    strcpy(display_line[1], "  White   ");
                    strcpy(display_line[2], " sampling ");
                    strcpy(display_line[3], "          ");
                    display_changed = TRUE;
                }
                if(p7_timer >= CAL_SAMPLE_DELAY){
                    white_left    = ADC_Left_Detect;
                    white_right   = ADC_Right_Detect;
                    p7_timer      = RESET_STATE;
                    calibrate_phase = CAL_WAIT_BLACK;

                    strcpy(display_line[0], "Place on  ");
                    strcpy(display_line[1], "black line");
                    strcpy(display_line[2], "Press SW1 ");
                    strcpy(display_line[3], "          ");
                    display_changed = TRUE;
                }
                break;

            case CAL_WAIT_BLACK:
                // Waiting for SW1 press -- handled in interrupt_ports.c
                // switch1_interrupt will set calibrate_phase = CAL_BLACK and
                // reset p7_timer = 0 when SW1 is pressed in this sub-phase.
                break;

            case CAL_BLACK:
                if(p7_timer == RESET_STATE){
                    strcpy(display_line[0], "Calibrate ");
                    strcpy(display_line[1], "  Black   ");
                    strcpy(display_line[2], " sampling ");
                    strcpy(display_line[3], "          ");
                    display_changed = TRUE;
                }
                if(p7_timer >= CAL_SAMPLE_DELAY){
                    black_left  = ADC_Left_Detect;
                    black_right = ADC_Right_Detect;

                    // Threshold = midpoint between white and black readings
                    threshold_left  = (white_left  + black_left)  / 2;
                    threshold_right = (white_right + black_right) / 2;

                    calibrate_phase = CAL_DONE;
                    p7_timer = RESET_STATE;
                    p7_state = P7_WAIT_START;

                    // Show calibration results briefly
                    strcpy(display_line[0], " Cal Done ");

                    HexToBCD((int)threshold_left);
                    display_line[1][0]  = 'L';
                    display_line[1][1]  = 'T';
                    display_line[1][2]  = ':';
                    display_line[1][3]  = thousands;
                    display_line[1][4]  = hundreds;
                    display_line[1][5]  = tens;
                    display_line[1][6]  = ones;
                    display_line[1][7]  = ' ';
                    display_line[1][8]  = ' ';
                    display_line[1][9]  = ' ';
                    display_line[1][10] = '\0';

                    HexToBCD((int)threshold_right);
                    display_line[2][0]  = 'R';
                    display_line[2][1]  = 'T';
                    display_line[2][2]  = ':';
                    display_line[2][3]  = thousands;
                    display_line[2][4]  = hundreds;
                    display_line[2][5]  = tens;
                    display_line[2][6]  = ones;
                    display_line[2][7]  = ' ';
                    display_line[2][8]  = ' ';
                    display_line[2][9]  = ' ';
                    display_line[2][10] = '\0';

                    strcpy(display_line[3], "Press SW1 ");
                    display_changed = TRUE;
                }
                break;

            default:
                calibrate_phase = CAL_AMBIENT;
                break;
        }
        break;

    //--------------------------------------------------------------------------
    // WAIT_START: Calibration done -- wait for operator to press SW1 to confirm
    //             car is repositioned at the center of the circle.
    //             SW1 ISR (interrupt_ports.c) advances to P7_ARMED.
    //--------------------------------------------------------------------------
    case P7_WAIT_START:
        // Nothing to poll -- SW1 ISR drives the transition to P7_ARMED.
        // Display is set once when CAL_BLACK completes (shows "Press SW1").
        break;

    //--------------------------------------------------------------------------
    // ARMED: 2-second countdown after SW1 repositioning press.
    //        Gives operator time to remove hands before motors start.
    //--------------------------------------------------------------------------
    case P7_ARMED:
        if(p7_timer == RESET_STATE){
            strcpy(display_line[0], " Armed!   ");
            strcpy(display_line[1], " Starting ");
            strcpy(display_line[2], "  in 2s   ");
            strcpy(display_line[3], "          ");
            display_changed = TRUE;
        }
        if(p7_timer >= P7_WAIT_START_TIME){
            p7_timer = RESET_STATE;
            p7_state = P7_FORWARD;

            // Start the 0.2-second display clock
            elapsed_tenths   = RESET_STATE;
            p7_timer_running = TRUE;

            Forward_On();   // Begin driving toward the circle

            strcpy(display_line[0], "Intercept ");
            strcpy(display_line[1], " Scanning ");
            strcpy(display_line[2], "          ");
            strcpy(display_line[3], "0.0s      ");
            display_changed = TRUE;
        }
        break;

    //--------------------------------------------------------------------------
    // FORWARD: Drive forward until either IR detector sees the black line.
    //--------------------------------------------------------------------------
    case P7_FORWARD:
        if((ADC_Left_Detect  > threshold_left) ||
           (ADC_Right_Detect > threshold_right)){

            Wheels_All_Off();   // Stop immediately (H-bridge safe)

            // Record which side detected stronger -- determines turn direction
            if(ADC_Left_Detect > ADC_Right_Detect){
                circle_direction = RESET_STATE;  // Left stronger -> spin CW
            } else {
                circle_direction = TRUE;          // Right stronger -> spin CCW
            }

            p7_timer = RESET_STATE;
            p7_state = P7_DETECTED_STOP;

            strcpy(display_line[0], "Black Line");
            strcpy(display_line[1], " Detected ");
            strcpy(display_line[2], "          ");
            display_changed = TRUE;
        }
        break;

    //--------------------------------------------------------------------------
    // DETECTED_STOP: Pause on the line for confirmation before turning.
    //--------------------------------------------------------------------------
    case P7_DETECTED_STOP:
        if(p7_timer >= P7_DETECT_STOP_TIME){
            p7_timer = RESET_STATE;
            p7_state = P7_TURNING;

            // Spin to align both detectors over the line
            if(circle_direction == RESET_STATE){
                Spin_CW_On();    // Left detected first -> spin CW (rotate right)
            } else {
                Spin_CCW_On();   // Right detected first -> spin CCW (rotate left)
            }

            strcpy(display_line[0], " Turning  ");
            strcpy(display_line[1], "  Align   ");
            strcpy(display_line[2], "          ");
            display_changed = TRUE;
        }
        break;

    //--------------------------------------------------------------------------
    // TURNING: Spin for P7_INITIAL_TURN_TIME to align both detectors on line.
    // TUNE P7_INITIAL_TURN_TIME in macros.h until both sensors end on the line.
    //--------------------------------------------------------------------------
    case P7_TURNING:
        if(p7_timer >= P7_INITIAL_TURN_TIME){
            Wheels_All_Off();
            p7_timer = RESET_STATE;
            p7_state = P7_FOLLOW_LINE;

            follow_start_tenths = elapsed_tenths;   // Record lap start time

            strcpy(display_line[2], " Circling ");
            display_changed = TRUE;
        }
        break;

    //--------------------------------------------------------------------------
    // FOLLOW_LINE: Proportional (P-only) line following for two laps.
    //
    //   Each sensor is normalized independently to [0, 100]:
    //     0   = pure white (full reflection, emitter response)
    //     100 = pure black (no reflection)
    //   Normalization uses the per-sensor calibration data so that sensor
    //   hardware differences are cancelled out.
    //
    //   error = right_norm - left_norm
    //     Positive error: right sensor sees more black -> steer right
    //                     (increase left wheel, decrease right wheel)
    //     Negative error: left sensor sees more black  -> steer left
    //
    //   correction = FOLLOW_KP * error
    //   left_speed  = FOLLOW_BASE + correction
    //   right_speed = FOLLOW_BASE - correction
    //   Both clamped to [WHEEL_OFF, WHEEL_PERIOD_VAL].
    //
    //   Lap tracking: time-based using elapsed_tenths.
    //   After TWO_LAP_TIME_TENTHS, transition to exit.
    //--------------------------------------------------------------------------
    case P7_FOLLOW_LINE:
    {
        unsigned int left_range;
        unsigned int right_range;
        int left_norm;
        int right_norm;
        int error;
        int correction;
        int left_speed;
        int right_speed;

        // Guard against divide-by-zero if calibration values are equal
        left_range  = (black_left  > white_left)  ? (black_left  - white_left)  : 1;
        right_range = (black_right > white_right) ? (black_right - white_right) : 1;

        // Normalize left sensor to [0, 100]
        if(ADC_Left_Detect <= white_left){
            left_norm = 0;
        } else if(ADC_Left_Detect >= black_left){
            left_norm = 100;
        } else {
            left_norm = (int)(((unsigned long)(ADC_Left_Detect - white_left) * 100UL)
                              / (unsigned long)left_range);
        }

        // Normalize right sensor to [0, 100]
        if(ADC_Right_Detect <= white_right){
            right_norm = 0;
        } else if(ADC_Right_Detect >= black_right){
            right_norm = 100;
        } else {
            right_norm = (int)(((unsigned long)(ADC_Right_Detect - white_right) * 100UL)
                               / (unsigned long)right_range);
        }

        // Proportional control
        error      = right_norm - left_norm;
        correction = FOLLOW_KP * error;

        left_speed  = (int)FOLLOW_BASE + correction;
        right_speed = (int)FOLLOW_BASE - correction;

        // Clamp to valid PWM range
        if(left_speed  < (int)WHEEL_OFF)         left_speed  = (int)WHEEL_OFF;
        if(right_speed < (int)WHEEL_OFF)         right_speed = (int)WHEEL_OFF;
        if(left_speed  > (int)WHEEL_PERIOD_VAL)  left_speed  = (int)WHEEL_PERIOD_VAL;
        if(right_speed > (int)WHEEL_PERIOD_VAL)  right_speed = (int)WHEEL_PERIOD_VAL;

        // Write PWM -- ensure reverse is always off (H-bridge safety)
        LEFT_REVERSE_SPEED  = WHEEL_OFF;
        RIGHT_REVERSE_SPEED = WHEEL_OFF;
        LEFT_FORWARD_SPEED  = (unsigned int)left_speed;
        RIGHT_FORWARD_SPEED = (unsigned int)right_speed;

        // Check if two laps are complete (time-based)
        if((elapsed_tenths - follow_start_tenths) >= TWO_LAP_TIME_TENTHS){
            Wheels_All_Off();
            p7_timer = RESET_STATE;
            p7_state = P7_EXIT_TURN;

            strcpy(display_line[0], " 2 Laps!  ");
            strcpy(display_line[1], " Exiting  ");
            strcpy(display_line[2], "          ");
            display_changed = TRUE;
        }
        break;
    }

    //--------------------------------------------------------------------------
    // EXIT_TURN: Spin toward the circle center for EXIT_TURN_TIME ticks,
    //            then transition to P7_DONE for the short forward drive in.
    //--------------------------------------------------------------------------
    case P7_EXIT_TURN:
        if(p7_timer == RESET_STATE){
            // Start spin toward circle center
            // If car circled CW (circle_direction == 0), exit by spinning CW
            // (turning right faces the center for a CW circle).
            // If car circled CCW (circle_direction == 1), exit by spinning CCW.
            if(circle_direction == RESET_STATE){
                Spin_CW_On();    // Exit toward center (CW circle -> turn right)
            } else {
                Spin_CCW_On();   // Exit toward center (CCW circle -> turn left)
            }
        }
        if(p7_timer >= EXIT_TURN_TIME){
            Wheels_All_Off();
            p7_timer = RESET_STATE;
            p7_state = P7_DONE;   // Drive forward into center in P7_DONE

            strcpy(display_line[0], " Entering ");
            strcpy(display_line[1], "  Center  ");
            strcpy(display_line[2], "          ");
            display_changed = TRUE;
        }
        break;

    //--------------------------------------------------------------------------
    // DONE: Drive forward briefly into the center, then stop and freeze timer.
    //--------------------------------------------------------------------------
    case P7_DONE:
        if(p7_timer == RESET_STATE){
            Forward_On();        // Drive into center
        }
        if(p7_timer >= EXIT_FORWARD_TIME){
            Wheels_All_Off();    // Full stop

            p7_running       = FALSE;   // Stop state timer
            p7_timer_running = FALSE;   // Freeze the display clock

            // Turn off IR emitter to save battery
            P2OUT        &= ~IR_LED;
            ir_emitter_on  = FALSE;

            strcpy(display_line[0], " Complete ");
            strcpy(display_line[1], "  Stopped ");
            strcpy(display_line[2], "          ");
            // display_line[3] retains the frozen timer from Update_P7_Display
            display_changed = TRUE;

            // Advance p7_timer beyond threshold to prevent re-entry
            p7_timer = EXIT_FORWARD_TIME + TRUE;
        }
        break;

    default:
        Wheels_All_Off();   // Safety: unknown state -- stop motors
        break;
    }
}
