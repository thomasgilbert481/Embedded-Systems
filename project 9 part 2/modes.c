//==============================================================================
// File: modes.c
// Description: Calibration + line-follow logic for Project 9 Part 2.
//
//   CALIBRATION (^1234C0000):
//     1. LCD prompts "Place on WHITE, press SW1".
//     2. User presses SW1; after ~1 s settle the ADC values are captured.
//     3. LCD prompts "Place on BLACK, press SW1".
//     4. User presses SW1; after ~1 s settle the black values are captured
//        and threshold = (white + black) / 2 per sensor.
//     5. LCD shows "Cal Done" then returns to the IP layout.
//     Calibration can be cancelled at any time with ^1234Q0000.
//
//   LINE FOLLOW (^1234N<time>):
//     Requires calibration_done.  Drives forward with proportional-gain
//     steering based on normalized sensor values until the cmd auto-stop
//     fires (time * 100 ms).  ^1234Q0000 also cancels immediately.
//
// Author: Thomas Gilbert (with Project 7 logic ported in)
// Date: Mar 2026
//==============================================================================

#include "msp430.h"
#include <string.h>
#include "macros.h"
#include "ports.h"
#include "functions.h"
#include "serial.h"
#include "iot.h"
#include "adc.h"
#include "modes.h"

//------------------------------------------------------------------------------
// Globals (calibration results)
//------------------------------------------------------------------------------
unsigned int white_left      = 0;
unsigned int white_right     = 0;
unsigned int black_left      = 0;
unsigned int black_right     = 0;
unsigned int threshold_left  = 0;
unsigned int threshold_right = 0;
unsigned int calibration_done = 0;

volatile unsigned char mode_cal_active  = 0;
volatile unsigned char mode_line_active = 0;

//------------------------------------------------------------------------------
// External LCD globals
//------------------------------------------------------------------------------
extern char                  display_line[4][11];
extern volatile unsigned char display_changed;

// Switch-press flags (set by interrupts_ports.c, cleared here)
extern volatile unsigned int sw1_pressed;
extern volatile unsigned int sw2_pressed;

// Timer B0 CCR0 tick counter (+1 per 200 ms) -- used for deterministic
// calibration-settle delay the same way Project 7's p7_timer does.
extern volatile unsigned int Time_Sequence;

// Active-command state (in iot.c) -- used by Line_Follow_Start to set up
// the auto-stop countdown.
extern volatile unsigned int  cmd_remaining_ms;
extern volatile char          cmd_active_dir;
extern volatile unsigned int  cmd_active_time;

//------------------------------------------------------------------------------
// Helper: write "AA:dddd  " into display_line[line_idx] where AA is a 2-char
// label and dddd is a zero-padded 4-digit decimal value.  Pads to 10 chars.
//------------------------------------------------------------------------------
static void lcd_write_value(unsigned int line_idx, const char *label, unsigned int value){
    unsigned int tenk, thousands, hundreds, tens, ones;

    if(line_idx > 3) return;

    // unsigned int is 16-bit on MSP430 (max 65535), so values > 99999 can't
    // even be expressed.  Clamp to 5-digit printable range (<=65535) which
    // comfortably covers every motor PWM we use (CCR0 = 50000).
    // Overflow-past-65535 is impossible by type.

    tenk      = value / 10000u;      value -= tenk      * 10000u;
    thousands = value / 1000u;       value -= thousands * 1000u;
    hundreds  = value / 100u;        value -= hundreds  * 100u;
    tens      = value / 10u;         value -= tens      * 10u;
    ones      = value;

    display_line[line_idx][0] = label[0];
    display_line[line_idx][1] = label[1];
    display_line[line_idx][2] = ':';
    display_line[line_idx][3] = (char)('0' + tenk);
    display_line[line_idx][4] = (char)('0' + thousands);
    display_line[line_idx][5] = (char)('0' + hundreds);
    display_line[line_idx][6] = (char)('0' + tens);
    display_line[line_idx][7] = (char)('0' + ones);
    display_line[line_idx][8] = ' ';
    display_line[line_idx][9] = ' ';
    display_line[line_idx][10] = SERIAL_NULL;
}

//------------------------------------------------------------------------------
// Line-follow LCD update counters (declared early so Line_Follow_Start can
// reset the counter to force an immediate redraw).
//------------------------------------------------------------------------------
static unsigned long line_dbg_cnt = 0;
#define LINE_DBG_INTERVAL 2000

//------------------------------------------------------------------------------
// Show all four calibration values on the LCD (used at end of cal)
//------------------------------------------------------------------------------
static void lcd_show_cal_values(void){
    lcd_write_value(0, "WL", white_left);
    lcd_write_value(1, "WR", white_right);
    lcd_write_value(2, "BL", black_left);
    lcd_write_value(3, "BR", black_right);
    display_changed = TRUE;
}

//------------------------------------------------------------------------------
// Calibration sub-states
//------------------------------------------------------------------------------
#define CAL_ST_PROMPT_WHITE (0)
#define CAL_ST_WAIT_WHITE   (1)
#define CAL_ST_SAMPLE_WHITE (2)
#define CAL_ST_PROMPT_BLACK (3)
#define CAL_ST_WAIT_BLACK   (4)
#define CAL_ST_SAMPLE_BLACK (5)
#define CAL_ST_FINISH       (6)

static unsigned int cal_sub_state = CAL_ST_PROMPT_WHITE;
static unsigned int cal_start_tick = 0;  // Time_Sequence when sampling began

// Number of 200 ms timer ticks to wait after SW1 before reading the ADC
// (matches Project 7's CAL_SAMPLE_DELAY = 5 ticks = 1 second exactly).
#define CAL_SETTLE_TICKS    (5)

//==============================================================================
// Quit_Everything -- ^1234Q0000 arrived.  Abort everything.
//==============================================================================
void Quit_Everything(void){
    Wheels_All_Off();
    cmd_remaining_ms = 0;
    cmd_active_dir   = SERIAL_NULL;
    cmd_active_time  = 0;
    mode_cal_active  = 0;
    mode_line_active = 0;
    // Clear LCD back to a known state; IP will be repainted once the state
    // machine sees Display_Network_Info() is safe to re-call from main
    USB_transmit_string("QUIT\r\n");
    // Repaint the network info LCD layout
    Display_Network_Info();
}

//==============================================================================
// Calibration_Start -- ^1234C0000 arrived
//==============================================================================
void Calibration_Start(void){
    // Stop any motion first (safety)
    Wheels_All_Off();
    cmd_remaining_ms = 0;
    cmd_active_dir   = SERIAL_NULL;
    mode_line_active = 0;

    // Turn IR emitter ON for calibration
    P2OUT |= IR_LED;
    ir_emitter_on = 1;

    cal_sub_state   = CAL_ST_PROMPT_WHITE;
    cal_start_tick  = Time_Sequence;
    mode_cal_active = 1;
    USB_transmit_string("CAL start\r\n");
}

//==============================================================================
// Calibration_Tick -- called from main loop each iteration when cal is active
//==============================================================================
void Calibration_Tick(void){
    if(!mode_cal_active){
        return;
    }

    switch(cal_sub_state){

        case CAL_ST_PROMPT_WHITE:
            strcpy(display_line[0], " Place on ");
            strcpy(display_line[1], "  WHITE   ");
            strcpy(display_line[2], " Press SW1");
            strcpy(display_line[3], "          ");
            display_changed = TRUE;
            sw1_pressed     = 0;    // Consume any stale press
            cal_sub_state   = CAL_ST_WAIT_WHITE;
            break;

        case CAL_ST_WAIT_WHITE:
            if(sw1_pressed){
                sw1_pressed    = 0;
                cal_start_tick = Time_Sequence;
                strcpy(display_line[2], " Sampling ");
                display_changed = TRUE;
                cal_sub_state   = CAL_ST_SAMPLE_WHITE;
            }
            break;

        case CAL_ST_SAMPLE_WHITE:
            // Deterministic 1 s settle based on Timer B0 CCR0 (200 ms ticks).
            // Matches Project 7's CAL_SAMPLE_DELAY flow exactly.
            if((unsigned int)(Time_Sequence - cal_start_tick) >= CAL_SETTLE_TICKS){
                white_left  = ADC_Left_Detect;
                white_right = ADC_Right_Detect;
                USB_transmit_string("CAL white captured\r\n");
                cal_sub_state = CAL_ST_PROMPT_BLACK;
            }
            break;

        case CAL_ST_PROMPT_BLACK:
            strcpy(display_line[0], " Place on ");
            strcpy(display_line[1], "  BLACK   ");
            strcpy(display_line[2], " Press SW1");
            strcpy(display_line[3], "          ");
            display_changed = TRUE;
            sw1_pressed     = 0;
            cal_sub_state   = CAL_ST_WAIT_BLACK;
            break;

        case CAL_ST_WAIT_BLACK:
            if(sw1_pressed){
                sw1_pressed    = 0;
                cal_start_tick = Time_Sequence;
                strcpy(display_line[2], " Sampling ");
                display_changed = TRUE;
                cal_sub_state   = CAL_ST_SAMPLE_BLACK;
            }
            break;

        case CAL_ST_SAMPLE_BLACK:
            if((unsigned int)(Time_Sequence - cal_start_tick) >= CAL_SETTLE_TICKS){
                black_left  = ADC_Left_Detect;
                black_right = ADC_Right_Detect;

                // Midpoint thresholds
                threshold_left  = (white_left  + black_left)  / 2;
                threshold_right = (white_right + black_right) / 2;

                calibration_done = 1;
                USB_transmit_string("CAL done\r\n");

                cal_sub_state  = CAL_ST_FINISH;
                cal_start_tick = Time_Sequence;
            }
            break;

        case CAL_ST_FINISH:
            // Show the 4 captured values on the LCD for ~5 s before returning
            // to the IP layout.  25 ticks * 200 ms = 5 s.
            lcd_show_cal_values();
            if((unsigned int)(Time_Sequence - cal_start_tick) >= 25){
                mode_cal_active = 0;
                Display_Network_Info();
            }
            break;

        default:
            mode_cal_active = 0;
            break;
    }
}

//==============================================================================
// Line_Follow_Start -- ^1234N<time> arrived.
// seconds == 0 -> use LINE_FOLLOW_DEFAULT_SECONDS
//
// Kicks off the Project-7 style pre-sequence:
//   LF_SEEK   -> drive forward until an IR sensor crosses its threshold
//   LF_PAUSE  -> 1 s stop after line detection
//   LF_ALIGN  -> 1 s spin toward the line so both sensors straddle it
//   LF_FOLLOW -> proportional control (the "working" P7 algorithm)
//==============================================================================

// Sub-states for the line-follow sequence
#define LF_SEEK     (0)
#define LF_PAUSE    (1)
#define LF_ALIGN    (2)
#define LF_FOLLOW   (3)

static unsigned char lf_sub_state  = LF_SEEK;
static unsigned int  lf_phase_tick = 0;         // Time_Sequence when phase began
static unsigned char lf_spin_cw    = 0;         // 1 = left sensor saw line first
static int           lf_last_error = 0;         // PD state: previous error term
static unsigned int  lf_off_line_cnt = 0;       // Debounce counter for "both off line"
static unsigned long lf_print_cnt   = 0;        // Throttle for Termite diagnostic
static char          lf_diag_mode   = ' ';      // 'P'=PD, 'C'=center, 'R'=reverse

//------------------------------------------------------------------------------
// Helper: print one line of PD diagnostic to Termite.
//   mode  -- 'P' PD active, 'C' centered/deadband, 'R' reverse-reacquire
//   ln,rn -- normalized [0,100] sensor readings
//   ls,rs -- final clamped wheel speeds
//------------------------------------------------------------------------------
static void usb_print_int(int v){
    char buf[8];
    char out[10];
    unsigned int i = 0, j;
    unsigned int u;
    char neg = 0;

    if(v < 0){ neg = 1; v = -v; }
    u = (unsigned int)v;
    if(u == 0){ buf[i++] = '0'; }
    else { while(u > 0 && i < sizeof(buf)){ buf[i++] = (char)('0' + (u % 10)); u /= 10; } }
    j = 0;
    if(neg){ out[j++] = '-'; }
    while(i > 0){ out[j++] = buf[--i]; }
    out[j] = SERIAL_NULL;
    USB_transmit_string(out);
}

static void lf_print_diag(char mode, int ln, int rn, int ls, int rs){
    char m[2] = {mode, 0};
    USB_transmit_string("LF[");
    USB_transmit_string(m);
    USB_transmit_string("] Ln=");
    usb_print_int(ln);
    USB_transmit_string(" Rn=");
    usb_print_int(rn);
    USB_transmit_string(" Ls=");
    usb_print_int(ls);
    USB_transmit_string(" Rs=");
    usb_print_int(rs);
    USB_transmit_string("\r\n");
}

void Line_Follow_Start(unsigned int seconds){
    if(!calibration_done){
        USB_transmit_string("ERR: not calibrated\r\n");
        return;
    }

    // Wait for the DAC ramp to finish.  TB0CTL TBIE is cleared by the
    // overflow ISR (interrupts_timers.c case 14) once DAC_data reaches
    // DAC_Adjust -- that's when the motor buck-boost rail is at ~6V.
    // If we start line-follow before the ramp is done, the SEEK/ALIGN
    // phases run on ~2V which is too weak to overcome wheel friction,
    // and when the ramp completes mid-sequence the motors suddenly
    // jump to full torque -- produces the "spin past the line" symptom.
    // Red LED is ON during ramp, OFF when done, so the user has a
    // visible indicator too.
    if(TB0CTL & TBIE){
        USB_transmit_string("ERR: motor pwr ramping, wait for RED LED off\r\n");
        return;
    }

    Wheels_All_Off();
    mode_cal_active = 0;

    P2OUT |= IR_LED;
    ir_emitter_on = 1;

    if(seconds == 0){
        seconds = LINE_FOLLOW_DEFAULT_SECONDS;
    }

    cmd_active_dir   = CMD_DIR_LINE_FOLLOW;
    cmd_active_time  = seconds * 10;        // record in time-units
    cmd_remaining_ms = seconds * 1000u;     // ms
    mode_line_active = 1;
    line_dbg_cnt     = LINE_DBG_INTERVAL;   // force first LCD update right away

    // Begin with the SEEK phase (drive forward hunting for the line).
    lf_sub_state  = LF_SEEK;
    lf_phase_tick = Time_Sequence;
    Forward_On();                            // Start driving forward

    USB_transmit_string("LINE seek\r\n");
}

//------------------------------------------------------------------------------
// Periodic LCD update during line-follow.  Every ~0.25 s redraw with the
// diagnostic most useful for the current problem:
//   Line 0: "Ln:ddddd   "  normalized left  sensor (0..100)
//   Line 1: "Rn:ddddd   "  normalized right sensor (0..100)
//   Line 2: "Ls:ddddd   "  commanded left  PWM speed
//   Line 3: "Rs:ddddd   "  commanded right PWM speed
//
// Read this as:
//   Ln==Rn==100 -> both sensors fully on line; controller sees no error
//                  (this is why Ls and Rs came back identical -- the line
//                  covers both sensors simultaneously).
//   Ln and Rn differing as the car drifts -> PD can produce a correction.
//   Ls != Rs -> PD IS outputting differential; watch the wheels.
//------------------------------------------------------------------------------
static unsigned int lf_last_left_spd  = 0;
static unsigned int lf_last_right_spd = 0;
static int          lf_last_ln        = 0;
static int          lf_last_rn        = 0;

static void line_follow_display(int unused_l, int unused_r){
    (void)unused_l;
    (void)unused_r;
    if(++line_dbg_cnt < LINE_DBG_INTERVAL){
        return;
    }
    line_dbg_cnt = 0;
    lcd_write_value(0, "Ln", (unsigned int)(lf_last_ln < 0 ? 0 : lf_last_ln));
    lcd_write_value(1, "Rn", (unsigned int)(lf_last_rn < 0 ? 0 : lf_last_rn));
    lcd_write_value(2, "Ls", lf_last_left_spd);
    lcd_write_value(3, "Rs", lf_last_right_spd);
    display_changed = TRUE;
}

//==============================================================================
// Line_Follow_Tick -- called from main loop every iteration while active.
// Full Project-7 line-following sequence:
//   1. LF_SEEK   -- drive forward until a sensor crosses threshold
//   2. LF_PAUSE  -- 1 s stop after detection
//   3. LF_ALIGN  -- 1 s spin toward line (direction based on which sensor saw it)
//   4. LF_FOLLOW -- proportional control (byte-identical to P7_FOLLOW_LINE)
//
// Full-time countdown (cmd_remaining_ms) covers the entire sequence.
// Vehicle_Cmd_Tick handles the final motor stop when the timer expires.
//==============================================================================
void Line_Follow_Tick(void){
    int  correction;
    int  left_speed;
    int  right_speed;
    unsigned int phase_elapsed;

    if(!mode_line_active){
        return;
    }
    if(cmd_remaining_ms == 0){
        // Overall countdown expired -- Vehicle_Cmd_Tick already stopped motors.
        mode_line_active = 0;
        Display_Network_Info();
        return;
    }

    phase_elapsed = (unsigned int)(Time_Sequence - lf_phase_tick);

    switch(lf_sub_state){

    //--------------------------------------------------------------------------
    // LF_SEEK -- P7_FORWARD equivalent.  Drive forward until either sensor
    // crosses its calibrated threshold.  Ignore sensors for the first
    // LF_SEEK_GUARD_TICKS to let the car start moving past any residual
    // reading.
    //--------------------------------------------------------------------------
    case LF_SEEK:
        if(phase_elapsed >= LF_SEEK_GUARD_TICKS &&
           ((ADC_Left_Detect  > threshold_left) ||
            (ADC_Right_Detect > threshold_right))){
            // Line found -- snapshot which side saw it stronger.
            lf_spin_cw = (ADC_Left_Detect > ADC_Right_Detect) ? 1 : 0;
            Wheels_All_Off();
            USB_transmit_string("LINE detected\r\n");
            lf_sub_state  = LF_PAUSE;
            lf_phase_tick = Time_Sequence;
        }
        // (Forward_On() was already called in Line_Follow_Start; motors keep running)
        break;

    //--------------------------------------------------------------------------
    // LF_PAUSE -- P7_DETECTED_STOP equivalent.  Motors off, wait ~1 s.
    //--------------------------------------------------------------------------
    case LF_PAUSE:
        if(phase_elapsed >= P7_DETECT_STOP_TIME){
            // Spin toward the side that saw the line (to center both sensors).
            if(lf_spin_cw){
                Spin_CW_On();
            } else {
                Spin_CCW_On();
            }
            USB_transmit_string("LINE align\r\n");
            lf_sub_state  = LF_ALIGN;
            lf_phase_tick = Time_Sequence;
        }
        break;

    //--------------------------------------------------------------------------
    // LF_ALIGN -- P7_TURNING equivalent.  Spin to center the line between
    // the two sensors.  EARLY EXIT: if both sensors cross their thresholds
    // (both on line) before the time limit, stop immediately.  Otherwise
    // fall back to the P7_INITIAL_TURN_TIME timeout.
    //--------------------------------------------------------------------------
    case LF_ALIGN:
        if((ADC_Left_Detect  > threshold_left) &&
           (ADC_Right_Detect > threshold_right)){
            // Both sensors already on the line -- done aligning.
            Wheels_All_Off();
            USB_transmit_string("LINE follow\r\n");
            lf_last_error    = 0;       // reset PD state
            lf_off_line_cnt  = 0;       // reset debounce
            lf_sub_state  = LF_FOLLOW;
            lf_phase_tick = Time_Sequence;
        } else if(phase_elapsed >= P7_INITIAL_TURN_TIME){
            Wheels_All_Off();
            USB_transmit_string("LINE follow\r\n");
            lf_last_error    = 0;
            lf_off_line_cnt  = 0;
            lf_sub_state  = LF_FOLLOW;
            lf_phase_tick = Time_Sequence;
        }
        break;

    //--------------------------------------------------------------------------
    // LF_FOLLOW -- byte-for-byte port of Project_7/Follow_Line:
    //   If BOTH sensors off the line -> reverse at REVERSE_SPEED (re-acquire).
    //   Else PD forward:
    //     err       = left_reading - right_reading
    //     d_err     = err - lf_last_error
    //     correction= (KP*err + KD*d_err) / PD_SCALE_DIVISOR
    //     left      = BASE - correction  (clamped)
    //     right     = BASE + correction  (clamped)
    //--------------------------------------------------------------------------
    case LF_FOLLOW:
    {
        // Hysteresis: the sensor is considered CLEARLY off the line only if
        // its reading is below threshold by more than LF_LINE_LOST_MARGIN.
        // This prevents flapping when the raw reading hovers around the
        // threshold boundary.
        unsigned int left_on_line  = (ADC_Left_Detect  + LF_LINE_LOST_MARGIN
                                       > threshold_left);
        unsigned int right_on_line = (ADC_Right_Detect + LF_LINE_LOST_MARGIN
                                       > threshold_right);
        unsigned int left_range;
        unsigned int right_range;
        int left_norm;
        int right_norm;
        int base_err;
        int delta_err;
        int abs_err;

        // Show raw ADC values on LCD at the display rate
        line_follow_display(0, 0);

        //----------------------------------------------------------------------
        // Hysteresis: only trigger reverse-reacquire if BOTH sensors have been
        // below threshold for LF_OFF_LINE_CONFIRM consecutive passes.  Single
        // noise dips are absorbed.
        //----------------------------------------------------------------------
        if(!left_on_line && !right_on_line){
            lf_off_line_cnt++;
            if(lf_off_line_cnt >= LF_OFF_LINE_CONFIRM){
                LEFT_FORWARD_SPEED  = WHEEL_OFF;
                LEFT_REVERSE_SPEED  = REVERSE_SPEED;
                RIGHT_FORWARD_SPEED = WHEEL_OFF;
                RIGHT_REVERSE_SPEED = REVERSE_SPEED;
                lf_diag_mode = 'R';   // Reverse re-acquire
                break;
            }
            // Inside debounce window -- keep previous motor state
            break;
        }
        lf_off_line_cnt = 0;

        //----------------------------------------------------------------------
        // Normalize each sensor to [0, 100] using its own white/black
        // calibration.  This removes per-sensor bias so err=0 is truly
        // "centered on the line" regardless of mismatched calibrations.
        //----------------------------------------------------------------------
        left_range  = (black_left  > white_left)  ? (black_left  - white_left)  : 1;
        right_range = (black_right > white_right) ? (black_right - white_right) : 1;

        if(ADC_Left_Detect <= white_left){
            left_norm = 0;
        } else if(ADC_Left_Detect >= black_left){
            left_norm = 100;
        } else {
            left_norm = (int)(((unsigned long)(ADC_Left_Detect - white_left) * 100UL)
                              / (unsigned long)left_range);
        }
        if(ADC_Right_Detect <= white_right){
            right_norm = 0;
        } else if(ADC_Right_Detect >= black_right){
            right_norm = 100;
        } else {
            right_norm = (int)(((unsigned long)(ADC_Right_Detect - white_right) * 100UL)
                               / (unsigned long)right_range);
        }

        base_err = left_norm - right_norm;            // range ~[-100, +100]
        abs_err  = (base_err < 0) ? -base_err : base_err;

        // Snapshot for LCD
        lf_last_ln = left_norm;
        lf_last_rn = right_norm;

        if(abs_err < LF_ERR_DEADBAND){
            // Essentially centered on the line -- straight ahead.
            lf_last_error = 0;
            left_speed  = (int)BASE_FOLLOW_SPEED;
            right_speed = (int)BASE_FOLLOW_SPEED;
            lf_diag_mode = 'C';   // Centered (deadband)
        } else {
            delta_err  = base_err - lf_last_error;
            lf_last_error = base_err;

            correction = (KP_VALUE * base_err) + (KD_VALUE * delta_err);
            correction = correction / PD_SCALE_DIVISOR;

            left_speed  = (int)BASE_FOLLOW_SPEED - correction;
            right_speed = (int)BASE_FOLLOW_SPEED + correction;
            lf_diag_mode = 'P';   // PD active
        }

        if(left_speed  < 0)                         left_speed  = 0;
        if(right_speed < 0)                         right_speed = 0;
        if(left_speed  > (int)MAX_FOLLOW_SPEED)     left_speed  = (int)MAX_FOLLOW_SPEED;
        if(right_speed > (int)MAX_FOLLOW_SPEED)     right_speed = (int)MAX_FOLLOW_SPEED;

        // H-bridge mutex per wheel.
        LEFT_REVERSE_SPEED  = WHEEL_OFF;
        LEFT_FORWARD_SPEED  = (unsigned int)left_speed;
        RIGHT_REVERSE_SPEED = WHEEL_OFF;
        RIGHT_FORWARD_SPEED = (unsigned int)right_speed;

        // Snapshot the commanded speeds so the LCD update can show them.
        lf_last_left_spd  = (unsigned int)left_speed;
        lf_last_right_spd = (unsigned int)right_speed;

        // Periodic Termite diagnostic so we can see what the controller
        // is actually doing.  Fires roughly twice per second.
        lf_print_cnt++;
        if(lf_print_cnt >= LINE_DBG_INTERVAL){
            lf_print_cnt = 0;
            lf_print_diag(lf_diag_mode, left_norm, right_norm,
                          left_speed, right_speed);
        }
    } break;

    default:
        // Shouldn't happen; bail safely.
        mode_line_active = 0;
        Wheels_All_Off();
        break;
    }

    // Rate diagnostic (toggles every tick while line-follow is active).
    P2OUT ^= IOT_RUN_RED;
}
