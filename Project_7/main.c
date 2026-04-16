//==============================================================================
// File Name: main.c
// Description: Main routine for Project 7 -- Black Line Circle Following.
//
//              The car starts inside a 36-inch diameter circle of black tape
//              on white paper and executes the following sequence:
//
//                1. P7_IDLE          -- Wait for SW1. Display live ADC. IR OFF.
//                2. P7_CALIBRATE     -- Multi-phase IR calibration:
//                                       Ambient (IR off), White, Black.
//                                       Compute threshold = (white+black)/2.
//                3. P7_CAL_COMPLETE  -- Display thresholds. User repositions
//                                       car to circle center. Press SW1.
//                4. P7_WAIT_START    -- 2-second delay, then drive forward.
//                5. P7_FORWARD       -- Drive toward black line; IR on.
//                6. P7_DETECTED_STOP -- Stop on line detection (~1 sec).
//                7. P7_TURNING       -- Spin to align both detectors on line.
//                8. P7_FOLLOW_LINE   -- PD line following for 2 laps.
//                9. P7_EXIT_TURN     -- Spin toward circle center.
//               10. P7_EXIT_FORWARD  -- Drive briefly into center.
//               11. P7_DONE          -- Stopped. Timer frozen.
//
//              Timer: Timer B0 fires every 5ms (200 Hz).
//                     ONE_SEC = 200 ticks. TWO_SEC = 400 ticks.
//                     elapsed_tenths counts 0.2-second increments.
//              Motors: Timer B3 hardware PWM via CCR1-CCR4.
//              Power:  SAC3 DAC drives Wheels Power Board (Init_DAC required).
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
void Run_Project7(void);
void Update_P7_Display(void);
void Follow_Line(void);

//==============================================================================
// Global Variables
//==============================================================================
volatile char slow_input_down;                  // Debounce flag (legacy)
extern char display_line[4][11];                // LCD display buffer (4x11)
extern char *display[4];                        // Pointers to display lines
unsigned char display_mode;                     // Display mode selector
extern volatile unsigned char display_changed;  // Flag: LCD content updated
extern volatile unsigned char update_display;   // Flag: timer says refresh LCD
extern volatile unsigned int update_display_count;
extern volatile unsigned int Time_Sequence;     // Master timer
extern volatile char one_time;                  // One-time flag

//------------------------------------------------------------------------------
// Project 5/6 legacy variables (kept for timers.c extern compatibility)
//------------------------------------------------------------------------------
volatile unsigned int p5_timer   = 0;
volatile unsigned int p5_running = 0;
unsigned int p5_step  = 0;
char p5_started = 0;
char step_init  = 1;

volatile unsigned int p6_state   = P6_IDLE;
volatile unsigned int p6_timer   = 0;
volatile unsigned int p6_running = 0;

//------------------------------------------------------------------------------
// Project 7 state machine variables
//------------------------------------------------------------------------------
volatile unsigned int p7_state   = P7_IDLE;
volatile unsigned int p7_timer   = 0;
volatile unsigned int p7_running = 0;

// 0.2-second display clock
volatile unsigned int elapsed_tenths   = 0;
volatile unsigned int p7_timer_running = 0;

//------------------------------------------------------------------------------
// Calibration results
//------------------------------------------------------------------------------
unsigned int ambient_left   = 0;
unsigned int ambient_right  = 0;
unsigned int white_left     = 0;
unsigned int white_right    = 0;
unsigned int black_left     = 0;
unsigned int black_right    = 0;
unsigned int threshold_left  = 0;
unsigned int threshold_right = 0;

// Calibration sub-phase tracker
unsigned int calibrate_phase = CAL_AMBIENT;

// Circle direction: 0 = CW, 1 = CCW
unsigned int circle_direction = 0;

// elapsed_tenths when P7_FOLLOW_LINE was entered (for lap timing)
unsigned int follow_start_tenths = 0;

// PD control: last error for derivative term
int last_error = 0;

//==============================================================================
// Function: main
//==============================================================================
void main(void){

    PM5CTL0 &= ~LOCKLPM5;   // Unlock GPIO (required after reset on FR devices)

    Init_Ports();            // Configure all GPIO (motor pins -> Timer B3)
    Init_Clocks();           // 8 MHz MCLK/SMCLK
    Init_Conditions();       // Clear display buffers, enable global interrupts
    Init_Timers();           // Timer B0 (5ms) + Timer B3 (hardware PWM)
    Init_LCD();              // SPI LCD
    Init_ADC();              // 12-bit ADC: A2->A3->A5 round-robin
    Init_DAC();              // SAC3 DAC for Wheels Power Board
                             // MUST call after Init_Timers (uses Timer B0 ISR)

    // Safety: all motors off
    Wheels_All_Off();

    // IR emitter OFF at startup (turned on during calibration/driving)
    P2OUT &= ~IR_LED;
    ir_emitter_on = 0;

    // LCD backlight ON
    P6OUT |= LCD_BACKLITE;

    // Initial display
    strcpy(display_line[0], " Project7 ");
    strcpy(display_line[1], "  Circle  ");
    strcpy(display_line[2], " Follower ");
    strcpy(display_line[3], " Press SW1");
    display_changed = TRUE;

    //==========================================================================
    // Main loop
    //==========================================================================
    while(ALWAYS){

        Display_Process();       // Refresh LCD when update_display is set

        if(update_display){
            Update_P7_Display(); // Update timer and ADC lines
            // Note: update_display cleared by Display_Process after this
        }

        Switches_Process();      // Poll SW1/SW2 with debounce

        Run_Project7();          // Execute P7 state machine

        P3OUT ^= TEST_PROBE;     // Toggle test probe (oscilloscope heartbeat)
    }
}

//==============================================================================
// Function: Update_P7_Display
// Description: Updates display lines based on current state.
//              Called every ~200ms (when update_display is set).
//              - Lines 0-1: live ADC values during IDLE and FOLLOW_LINE.
//              - Line 2: current state name.
//              - Line 3: 0.2-second elapsed timer (when running).
//
// Format for line 3 timer: "XXX.Xs  " where
//   XXX = whole seconds (elapsed_tenths / 5)
//   X   = fractional tenths digit: 0,2,4,6,8 (elapsed_tenths % 5) * 2
//==============================================================================
void Update_P7_Display(void){

    unsigned int whole_sec;
    unsigned int frac;

    //--------------------------------------------------------------------------
    // Lines 0-1: live ADC in IDLE and FOLLOW_LINE
    //--------------------------------------------------------------------------
    if(p7_state == P7_IDLE || p7_state == P7_FOLLOW_LINE){

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
    }

    //--------------------------------------------------------------------------
    // Line 2: state name
    //--------------------------------------------------------------------------
    switch(p7_state){
        case P7_IDLE:
            strcpy(display_line[2], " P7 IDLE  ");
            break;
        case P7_CALIBRATE:
            strcpy(display_line[2], "Calibrate ");
            break;
        case P7_CAL_COMPLETE:
            strcpy(display_line[2], "Cal Done  ");
            break;
        case P7_WAIT_START:
            strcpy(display_line[2], "Wait Start");
            break;
        case P7_FORWARD:
            strcpy(display_line[2], " Forward  ");
            break;
        case P7_DETECTED_STOP:
            strcpy(display_line[2], "LineDetect");
            break;
        case P7_TURNING:
            strcpy(display_line[2], " Turning  ");
            break;
        case P7_FOLLOW_LINE:
            strcpy(display_line[2], " Follow   ");
            break;
        case P7_EXIT_TURN:
            strcpy(display_line[2], "Exit Turn ");
            break;
        case P7_EXIT_FORWARD:
            strcpy(display_line[2], "Exit Fwd  ");
            break;
        case P7_DONE:
            strcpy(display_line[2], "  DONE!   ");
            break;
        default:
            strcpy(display_line[2], "          ");
            break;
    }

    //--------------------------------------------------------------------------
    // Line 3: elapsed timer (when car is active or done)
    //--------------------------------------------------------------------------
    if(p7_state >= P7_FORWARD){
        whole_sec = elapsed_tenths / 5;
        frac      = (elapsed_tenths % 5) * 2;   // 0, 2, 4, 6, 8

        if(whole_sec > 999){
            whole_sec = 999;
        }

        display_line[3][0]  = (char)((whole_sec / 100) + '0');
        display_line[3][1]  = (char)(((whole_sec / 10) % 10) + '0');
        display_line[3][2]  = (char)((whole_sec % 10) + '0');
        display_line[3][3]  = '.';
        display_line[3][4]  = (char)(frac + '0');
        display_line[3][5]  = 's';
        display_line[3][6]  = ' ';
        display_line[3][7]  = ' ';
        display_line[3][8]  = ' ';
        display_line[3][9]  = ' ';
        display_line[3][10] = '\0';
    }

    display_changed = TRUE;
}

//==============================================================================
// Function: Follow_Line
// Description: PD line-following controller.
//              Called from P7_FOLLOW_LINE every main loop iteration.
//
//   error       = left_reading - right_reading
//   correction  = (Kp * error + Kd * delta_error) / PD_SCALE_DIVISOR
//   left_speed  = BASE_FOLLOW_SPEED - correction
//   right_speed = BASE_FOLLOW_SPEED + correction
//
//   If both detectors are off the line: reverse to re-find it.
//   Speeds clamped to [0, MAX_FOLLOW_SPEED].
//==============================================================================
void Follow_Line(void){

    int left_reading  = (int)ADC_Left_Detect;
    int right_reading = (int)ADC_Right_Detect;

    unsigned int left_on_line  = (ADC_Left_Detect  > threshold_left);
    unsigned int right_on_line = (ADC_Right_Detect > threshold_right);

    // --- Both off line: REVERSE to re-find it ---
    if(!left_on_line && !right_on_line){
        LEFT_FORWARD_SPEED  = WHEEL_OFF;
        RIGHT_FORWARD_SPEED = WHEEL_OFF;
        LEFT_REVERSE_SPEED  = REVERSE_SPEED;
        RIGHT_REVERSE_SPEED = REVERSE_SPEED;
        return;  // Don't update last_error -- preserve for re-acquisition
    }

    // --- At least one detector sees line: PD forward control ---
    LEFT_REVERSE_SPEED  = WHEEL_OFF;   // H-bridge safety
    RIGHT_REVERSE_SPEED = WHEEL_OFF;

    int base_error  = left_reading - right_reading;
    int delta_error = base_error - last_error;
    last_error      = base_error;

    int correction  = (KP_VALUE * base_error) + (KD_VALUE * delta_error);
    correction      = correction / PD_SCALE_DIVISOR;

    int left_speed  = (int)BASE_FOLLOW_SPEED - correction;
    int right_speed = (int)BASE_FOLLOW_SPEED + correction;

    if(left_speed  < 0)                         left_speed  = 0;
    if(right_speed < 0)                         right_speed = 0;
    if(left_speed  > (int)MAX_FOLLOW_SPEED)     left_speed  = (int)MAX_FOLLOW_SPEED;
    if(right_speed > (int)MAX_FOLLOW_SPEED)     right_speed = (int)MAX_FOLLOW_SPEED;

    LEFT_FORWARD_SPEED  = (unsigned int)left_speed;
    RIGHT_FORWARD_SPEED = (unsigned int)right_speed;
}

//==============================================================================
// Function: Run_Project7
// Description: 11-state state machine for the circle-following sequence.
//
//   p7_timer is incremented by Timer0_B0_ISR every 5ms when p7_running == 1.
//   Reset p7_timer = 0 when entering a new state (for delay timing).
//   elapsed_tenths increments every 200ms when p7_timer_running == 1.
//==============================================================================
void Run_Project7(void){

    switch(p7_state){

    //--------------------------------------------------------------------------
    // IDLE: wait for SW1. ADC shown by Update_P7_Display. IR off.
    //--------------------------------------------------------------------------
    case P7_IDLE:
        break;

    //--------------------------------------------------------------------------
    // CALIBRATE: three-phase IR calibration.
    //   CAL_AMBIENT:     IR OFF, wait 1s, sample ambient
    //   CAL_WHITE:       IR ON,  wait 1s, sample white surface
    //   CAL_WAIT_BLACK:  prompt user, wait for SW1
    //   CAL_BLACK:       SW1 set this; sample black, compute thresholds
    //--------------------------------------------------------------------------
    case P7_CALIBRATE:
        switch(calibrate_phase){

            case CAL_AMBIENT:
                if(p7_timer >= CAL_SAMPLE_DELAY){
                    ambient_left   = ADC_Left_Detect;
                    ambient_right  = ADC_Right_Detect;
                    p7_timer       = 0;
                    calibrate_phase = CAL_WHITE;

                    // Turn IR ON for white measurement
                    P2OUT        |= IR_LED;
                    ir_emitter_on  = 1;

                    strcpy(display_line[0], "Place on  ");
                    strcpy(display_line[1], "  white   ");
                    strcpy(display_line[2], "Calibrate ");
                    strcpy(display_line[3], "  White   ");
                    display_changed = TRUE;
                }
                break;

            case CAL_WHITE:
                if(p7_timer >= CAL_SAMPLE_DELAY){
                    white_left    = ADC_Left_Detect;
                    white_right   = ADC_Right_Detect;
                    p7_timer      = 0;
                    calibrate_phase = CAL_WAIT_BLACK;

                    strcpy(display_line[0], "PlaceBlack");
                    strcpy(display_line[1], "  on tape ");
                    strcpy(display_line[2], " Press SW1");
                    strcpy(display_line[3], "          ");
                    display_changed = TRUE;
                }
                break;

            case CAL_WAIT_BLACK:
                // Waiting for SW1 press -- handled in Switches_Process
                break;

            case CAL_BLACK:
                // SW1 in Switches_Process set calibrate_phase = CAL_BLACK
                // Sample black tape and compute thresholds immediately
                black_left  = ADC_Left_Detect;
                black_right = ADC_Right_Detect;

                threshold_left  = (white_left  + black_left)  / 2;
                threshold_right = (white_right + black_right) / 2;

                calibrate_phase = CAL_DONE;
                p7_running      = 0;
                p7_state        = P7_CAL_COMPLETE;

                // Display calibration results and reposition prompt
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

                strcpy(display_line[3], "Reposition");
                display_changed = TRUE;
                break;

            default:
                calibrate_phase = CAL_AMBIENT;
                break;
        }
        break;

    //--------------------------------------------------------------------------
    // CAL_COMPLETE: thresholds computed, car needs repositioning.
    //   Operator picks up car, places at circle center, then presses SW1.
    //   Switches_Process handles SW1 -> P7_WAIT_START transition.
    //--------------------------------------------------------------------------
    case P7_CAL_COMPLETE:
        // Nothing to poll here -- SW1 handler in Switches_Process drives next step
        break;

    //--------------------------------------------------------------------------
    // WAIT_START: 2-second delay after operator presses SW1.
    //   Gives operator time to step back before motors start.
    //   Then turn on IR and drive forward.
    //--------------------------------------------------------------------------
    case P7_WAIT_START:
        if(p7_timer >= P7_WAIT_START_TIME){
            p7_timer = 0;
            p7_state = P7_FORWARD;

            // Turn on IR emitter
            P2OUT        |= IR_LED;
            ir_emitter_on  = 1;

            // Start display timer at P7_FORWARD entry
            elapsed_tenths   = 0;
            p7_timer_running = 1;
            last_error       = 0;

            Forward_On();

            strcpy(display_line[0], "Intercept ");
            strcpy(display_line[1], " Scanning ");
            strcpy(display_line[2], " Forward  ");
            strcpy(display_line[3], "0.0s      ");
            display_changed = TRUE;
        }
        break;

    //--------------------------------------------------------------------------
    // FORWARD: drive forward until either detector sees the black line.
    //   Guard: ignore detection for first 3 ticks (15ms) to let car start.
    //--------------------------------------------------------------------------
    case P7_FORWARD:
        if(p7_timer >= 3 &&
           ((ADC_Left_Detect  > threshold_left) ||
            (ADC_Right_Detect > threshold_right))){

            Wheels_All_Off();

            // Record which side detected stronger (determines turn direction)
            if(ADC_Left_Detect > ADC_Right_Detect){
                circle_direction = 0;  // Left detected first -> spin CW
            } else {
                circle_direction = 1;  // Right detected first -> spin CCW
            }

            p7_timer = 0;
            p7_state = P7_DETECTED_STOP;

            strcpy(display_line[0], "Black Line");
            strcpy(display_line[1], " Detected ");
            display_changed = TRUE;
        }
        break;

    //--------------------------------------------------------------------------
    // DETECTED_STOP: pause on line for 1 second to confirm detection.
    //--------------------------------------------------------------------------
    case P7_DETECTED_STOP:
        if(p7_timer >= P7_DETECT_STOP_TIME){
            p7_timer = 0;
            p7_state = P7_TURNING;

            if(circle_direction == 0){
                Spin_CW_On();    // Left saw line first -> spin CW
            } else {
                Spin_CCW_On();   // Right saw line first -> spin CCW
            }

            strcpy(display_line[0], " Turning  ");
            strcpy(display_line[1], "  to align");
            display_changed = TRUE;
        }
        break;

    //--------------------------------------------------------------------------
    // TURNING: spin to align both detectors over the line.
    //   Stops when both detectors are on the line OR timer expires.
    //   TUNE P7_INITIAL_TURN_TIME in macros.h.
    //--------------------------------------------------------------------------
    case P7_TURNING:
        if((ADC_Left_Detect  > threshold_left) &&
           (ADC_Right_Detect > threshold_right)){
            // Both on line -- stop early
            Wheels_All_Off();
            follow_start_tenths = elapsed_tenths;
            last_error  = 0;
            p7_timer    = 0;
            p7_state    = P7_FOLLOW_LINE;

            strcpy(display_line[0], " Following");
            strcpy(display_line[1], "  Circle  ");
            display_changed = TRUE;
        } else if(p7_timer >= P7_INITIAL_TURN_TIME){
            // Timer expired -- proceed anyway
            Wheels_All_Off();
            follow_start_tenths = elapsed_tenths;
            last_error  = 0;
            p7_timer    = 0;
            p7_state    = P7_FOLLOW_LINE;

            strcpy(display_line[0], " Following");
            strcpy(display_line[1], "  Circle  ");
            display_changed = TRUE;
        }
        break;

    //--------------------------------------------------------------------------
    // FOLLOW_LINE: PD line following for two laps.
    //   Calls Follow_Line() every iteration for smooth PD control.
    //   Lap count is time-based: elapsed_tenths - follow_start_tenths.
    //--------------------------------------------------------------------------
    case P7_FOLLOW_LINE:
        Follow_Line();

        if((elapsed_tenths - follow_start_tenths) >= TWO_LAP_TIME_TENTHS){
            Wheels_All_Off();
            p7_timer = 0;
            p7_state = P7_EXIT_TURN;

            // Start exit spin toward circle center
            if(circle_direction == 0){
                Spin_CCW_On();   // CW circle -> turn CCW to face center
            } else {
                Spin_CW_On();    // CCW circle -> turn CW to face center
            }

            strcpy(display_line[0], " 2 Laps!  ");
            strcpy(display_line[1], " Exiting  ");
            display_changed = TRUE;
        }
        break;

    //--------------------------------------------------------------------------
    // EXIT_TURN: spin toward circle center for EXIT_TURN_TIME ticks.
    //   TUNE EXIT_TURN_TIME in macros.h.
    //--------------------------------------------------------------------------
    case P7_EXIT_TURN:
        if(p7_timer >= EXIT_TURN_TIME){
            Wheels_All_Off();
            p7_timer = 0;
            p7_state = P7_EXIT_FORWARD;

            Forward_On();

            strcpy(display_line[0], " Entering ");
            strcpy(display_line[1], "  Center  ");
            display_changed = TRUE;
        }
        break;

    //--------------------------------------------------------------------------
    // EXIT_FORWARD: drive into circle center for EXIT_FORWARD_TIME ticks.
    //--------------------------------------------------------------------------
    case P7_EXIT_FORWARD:
        if(p7_timer >= EXIT_FORWARD_TIME){
            Wheels_All_Off();

            // Freeze display timer
            p7_timer_running = 0;
            p7_running       = 0;

            // Turn off IR emitter
            P2OUT        &= ~IR_LED;
            ir_emitter_on  = 0;

            p7_state = P7_DONE;

            strcpy(display_line[0], " Complete ");
            strcpy(display_line[1], "  Stopped ");
            display_changed = TRUE;
        }
        break;

    //--------------------------------------------------------------------------
    // DONE: car stopped, timer frozen. Update_P7_Display shows frozen time.
    //--------------------------------------------------------------------------
    case P7_DONE:
        break;

    default:
        Wheels_All_Off();   // Safety: unknown state
        break;
    }
}
