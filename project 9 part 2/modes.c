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
// Forward declarations for static helpers defined later in this file.
//------------------------------------------------------------------------------
static void lf_enter_p7_pins(void);
static void lf_exit_p7_pins(void);
static void lf_motors_forward(unsigned int left, unsigned int right);
static void lf_motors_reverse(unsigned int speed);
static void lf_motors_stop(void);

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
// (line_dbg_cnt and LINE_DBG_INTERVAL removed -- no longer showing IR
// diagnostic during follow; using event-based status messages instead.)

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
    // If line-follow was active it may have been using the Project_7 pin
    // layout; restore P9P2 pins so F/B/R/L commands work correctly after.
    lf_exit_p7_pins();
    cmd_remaining_ms = 0;
    cmd_active_dir   = SERIAL_NULL;
    cmd_active_time  = 0;
    mode_cal_active  = 0;
    mode_line_active = 0;
    USB_transmit_string("QUIT\r\n");
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
// Sub-states for the line-follow sequence.  Each "WAIT" state = motors off,
// display the event message, sit for LF_EVENT_PAUSE ticks so the TA can read.
#define LF_SEEK_WHITE      (0)   // Arc: confirm white board first
#define LF_SEEK            (1)   // "BL Start" - hunting for black line
#define LF_INTERCEPT_WAIT  (2)   // "Intercept" - stopped 10 s
#define LF_TURN            (3)   // "BL Turn"   - spinning to align
#define LF_TURN_WAIT       (4)   // "BL Turn"   - stopped 10 s after turn
#define LF_TRAVEL          (5)   // "BL Travel" - following the line
#define LF_TRAVEL_WAIT     (6)   // "BL Travel" - stopped 10 s
#define LF_CIRCLE          (7)   // "BL Circle" - following around the circle
#define LF_EXIT_WAIT       (8)   // "BL Exit"   - stopped 10 s
#define LF_EXIT_TURN       (9)   // "BL Exit"   - spinning left
#define LF_EXIT_FWD        (10)  // "BL Exit"   - driving forward
#define LF_EXIT_DONE       (11)  // "BL Stop"   + custom message + time

static unsigned char lf_sub_state  = LF_SEEK;
static unsigned int  lf_phase_tick = 0;         // Time_Sequence when phase began
static unsigned char lf_spin_cw    = 0;         // 1 = left sensor saw line first
static int           lf_last_error = 0;         // PD state: previous error term
// Seek mode: 0 = straight (^N), 1 = right arc (^H), 2 = left arc (^J)
static unsigned char lf_seek_mode  = 0;
static unsigned int  lf_white_cnt  = 0;   // Consecutive white-surface iterations

// Elapsed-time tracking for the seconds counter on LCD line 4.
static unsigned int  lf_last_ts    = 0;   // Last seen Time_Sequence value
static unsigned long lf_ticks      = 0;   // 200 ms ticks since autonomous start
// (lf_off_line_cnt and lf_diag_mode were used by the old
// normalized-PD implementation; Project_7's port doesn't need them.)

void Line_Follow_Start(unsigned int seconds, unsigned char seek_mode){
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

    // NOTE: we do NOT switch to Project_7 pin layout here -- the SEEK/PAUSE/
    // ALIGN phases use the P9P2 mapping (via wheels.c helpers) so that both
    // wheels actually drive forward during SEEK.  The switch to P7 pins
    // happens later, just before entering LF_FOLLOW.

    P2OUT |= IR_LED;
    ir_emitter_on = 1;

    // seconds=0 means "follow indefinitely until ^Q or ^G".
    // Use max unsigned int (~65 seconds at 200ms tick) as the countdown;
    // Vehicle_Cmd_Tick will decrement but the user sends ^Q long before
    // it expires.  If they don't, the car just stops after ~65 s.
    if(seconds == 0){
        cmd_remaining_ms = 65000u;  // ~65 s fallback
    } else {
        cmd_remaining_ms = seconds * 1000u;
    }

    cmd_active_dir   = CMD_DIR_LINE_FOLLOW;
    cmd_active_time  = (seconds == 0) ? 650u : seconds * 10;
    mode_line_active = 1;
    lf_last_error    = 0;
    lf_seek_mode     = seek_mode;

    // Start elapsed timer
    lf_last_ts = Time_Sequence;
    lf_ticks   = 0;

    // Arc seeks start in LF_SEEK_WHITE; straight goes to LF_SEEK.
    lf_white_cnt  = 0;
    lf_sub_state  = (lf_seek_mode != 0) ? LF_SEEK_WHITE : LF_SEEK;
    lf_phase_tick = Time_Sequence;

    lf_set_display(" BL Start ");

    switch(lf_seek_mode){
        case 1:  // Right arc: left=outer(fast), right=inner(slow)
            LEFT_REVERSE_SPEED  = WHEEL_OFF;
            LEFT_FORWARD_SPEED  = ARC_OUTER_SPEED;
            RIGHT_REVERSE_SPEED = WHEEL_OFF;
            RIGHT_FORWARD_SPEED = ARC_INNER_SPEED;
            USB_transmit_string("LINE seek R\r\n");
            break;
        case 2:  // Left arc: right=outer(fast), left=inner(slow)
            LEFT_REVERSE_SPEED  = WHEEL_OFF;
            LEFT_FORWARD_SPEED  = ARC_INNER_SPEED;
            RIGHT_REVERSE_SPEED = WHEEL_OFF;
            RIGHT_FORWARD_SPEED = ARC_OUTER_SPEED;
            USB_transmit_string("LINE seek L\r\n");
            break;
        default: // Straight
            Forward_On();
            USB_transmit_string("LINE seek\r\n");
            break;
    }
}

//==============================================================================
// Line_Follow_Begin_Exit -- called by ^G while line-follow is active.
// Switches from P7 pins back to P9P2 so the exit sequence (spin + forward)
// can use both wheels, then enters the LF_EXIT_STOP sub-state.  The exit
// sequence runs entirely inside Line_Follow_Tick.
//==============================================================================
void Line_Follow_Begin_Exit(void){
    lf_motors_stop();            // halt P7-layout motors
    lf_exit_p7_pins();           // restore P9P2 pin layout
    Wheels_All_Off();            // defensive: zero P9P2 CCRs too
    cmd_remaining_ms = 65000u;   // extend countdown so exit sequence has time
    lf_set_display(" BL Exit  ");
    lf_sub_state  = LF_EXIT_WAIT;
    lf_phase_tick = Time_Sequence;
    USB_transmit_string("LINE exit\r\n");
}

//------------------------------------------------------------------------------
// Periodic LCD update during line-follow.  Every ~0.25 s redraw with:
//   Line 0: "Er:ddddd   "  |raw ADC error| = |ADC_L - ADC_R|
//   Line 1: "Cr:ddddd   "  |computed PD correction|
//   Line 2: "Ls:ddddd   "  left  PWM commanded via direct CCR write
//   Line 3: "Rs:ddddd   "  right PWM commanded via direct CCR write
//------------------------------------------------------------------------------
static unsigned int lf_last_left_spd  = 0;
static unsigned int lf_last_right_spd = 0;
static int          lf_last_ln        = 0;
static int          lf_last_rn        = 0;

//------------------------------------------------------------------------------
// Helper: set LCD to show ONLY the event string (centered on line 1, all
// other lines blank).  Then blink the LCD twice so the TA notices the change.
//
// Blink is done by toggling the backlight off/on twice with a busy-wait
// delay.  The delay is crude but short (~100ms per toggle) and only fires
// on state transitions (not every main-loop iter).
//------------------------------------------------------------------------------
static void lf_blink_delay(void){
    volatile unsigned long d;
    for(d = 80000UL; d > 0; d--);   // ~100 ms at 8 MHz
}

static void lf_set_display(const char *event){
    // Line 1: event message.  Lines 2-4: blank.
    strcpy(display_line[0], event);
    strcpy(display_line[1], "          ");
    strcpy(display_line[2], "          ");
    strcpy(display_line[3], "          ");
    display_changed = TRUE;

    // Force an immediate LCD refresh so the text is visible before the blink.
    Display_Process();

    // Blink backlight twice: OFF-ON-OFF-ON.
    P6OUT &= ~LCD_BACKLITE;   lf_blink_delay();
    P6OUT |=  LCD_BACKLITE;   lf_blink_delay();
    P6OUT &= ~LCD_BACKLITE;   lf_blink_delay();
    P6OUT |=  LCD_BACKLITE;
}

//------------------------------------------------------------------------------
// Pin / CCR mode switch.  Line-follow uses Project_7's layout; everything else
// (F/B/R/L via wheels.c) uses the P9P2 layout.  The switch happens at
// Line_Follow_Start entry and at Line_Follow_Exit cleanup.
//
//   P9P2 layout (wheels.c operates on this):
//     CCR1 = unused / dead      CCR2 = LEFT_FWD
//     CCR3 = RIGHT_FWD          CCR4 = LEFT_REV
//     CCR5 = RIGHT_REV           P6.5 is TB3.5 output
//
//   Project_7 layout (this file operates on this during line-follow):
//     CCR1 = RIGHT_FWD (dead)   CCR2 = LEFT_FWD
//     CCR3 = RIGHT_REV          CCR4 = LEFT_REV
//     CCR5 = unused              P6.5 is GPIO input
//------------------------------------------------------------------------------
static void lf_enter_p7_pins(void){
    // Stop everything first (both layouts' pins, defensively).
    TB3CCR1 = 0;
    TB3CCR2 = 0;
    TB3CCR3 = 0;
    TB3CCR4 = 0;
    TB3CCR5 = 0;
    // P6.5 -> GPIO input (Project_7 config).
    P6SEL0 &= ~P6_5;
    P6SEL1 &= ~P6_5;
    P6OUT  &= ~P6_5;
    P6DIR  &= ~P6_5;
}

static void lf_exit_p7_pins(void){
    // Zero all CCRs so no leftover PWM blips when the P9P2 mapping resumes.
    TB3CCR1 = 0;
    TB3CCR2 = 0;
    TB3CCR3 = 0;
    TB3CCR4 = 0;
    TB3CCR5 = 0;
    // P6.5 -> TB3.5 output (P9P2 config; used for RIGHT_REVERSE).
    P6SEL0 |=  P6_5;
    P6SEL1 &= ~P6_5;
    P6DIR  |=  P6_5;
}

//------------------------------------------------------------------------------
// Direct-CCR motor helpers used DURING line-follow (Project_7 layout).
// Each one writes CCR1..CCR4 with the H-bridge mutex preserved -- forward
// and reverse pins of the same wheel are never both non-zero at the same
// time.
//------------------------------------------------------------------------------
static void lf_motors_forward(unsigned int left, unsigned int right){
    TB3CCR3 = 0;           // right reverse off first
    TB3CCR4 = 0;           // left  reverse off first
    TB3CCR1 = right;       // right forward (P6.1 -- dead on this car, harmless)
    TB3CCR2 = left;        // left  forward
}

static void lf_motors_reverse(unsigned int speed){
    TB3CCR1 = 0;           // forward off first
    TB3CCR2 = 0;
    TB3CCR3 = speed;       // right reverse
    TB3CCR4 = speed;       // left  reverse
}

static void lf_motors_stop(void){
    TB3CCR1 = 0; TB3CCR2 = 0; TB3CCR3 = 0; TB3CCR4 = 0;
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
        lf_exit_p7_pins();
        mode_line_active = 0;
        Display_Network_Info();
        return;
    }

    // Keep the elapsed-time counter running (200 ms per tick).
    if(Time_Sequence != lf_last_ts){
        lf_ticks++;
        lf_last_ts = Time_Sequence;
    }

    phase_elapsed = (unsigned int)(Time_Sequence - lf_phase_tick);

    switch(lf_sub_state){

    //----------------------------------------------------------------------
    // "BL Start" — arc-seek: confirm white board first
    //----------------------------------------------------------------------
    case LF_SEEK_WHITE:
        if((ADC_Left_Detect  < (white_left  + LF_WHITE_MARGIN)) &&
           (ADC_Right_Detect < (white_right + LF_WHITE_MARGIN))){
            lf_white_cnt++;
            if(lf_white_cnt >= LF_WHITE_CONFIRM_COUNT){
                lf_sub_state  = LF_SEEK;
                lf_phase_tick = Time_Sequence;
            }
        } else {
            lf_white_cnt = 0;
        }
        break;

    //----------------------------------------------------------------------
    // "BL Start" — hunting for the black line
    //----------------------------------------------------------------------
    case LF_SEEK:
        if(phase_elapsed >= LF_SEEK_GUARD_TICKS &&
           ((ADC_Left_Detect  > threshold_left) ||
            (ADC_Right_Detect > threshold_right))){
            Wheels_All_Off();

            if(lf_seek_mode == 1){
                lf_spin_cw = 0;
            } else if(lf_seek_mode == 2){
                lf_spin_cw = 1;
            } else {
                lf_spin_cw = (ADC_Left_Detect > ADC_Right_Detect) ? 1 : 0;
            }

            lf_set_display("Intercept ");
            lf_sub_state  = LF_INTERCEPT_WAIT;
            lf_phase_tick = Time_Sequence;
        }
        break;

    //----------------------------------------------------------------------
    // "Intercept" — stopped, 10 s pause for TA
    //----------------------------------------------------------------------
    case LF_INTERCEPT_WAIT:
        if(phase_elapsed >= LF_EVENT_PAUSE){
            // Start the alignment turn.
            lf_set_display(" BL Turn  ");
            if(lf_spin_cw){
                Spin_CW_On();
            } else {
                Spin_CCW_On();
            }
            lf_sub_state  = LF_TURN;
            lf_phase_tick = Time_Sequence;
        }
        break;

    //----------------------------------------------------------------------
    // "BL Turn" — spinning to align on line
    //----------------------------------------------------------------------
    case LF_TURN:
        if((ADC_Left_Detect  > threshold_left) &&
           (ADC_Right_Detect > threshold_right)){
            Wheels_All_Off();
            lf_sub_state  = LF_TURN_WAIT;
            lf_phase_tick = Time_Sequence;
        } else if(phase_elapsed >= P7_INITIAL_TURN_TIME){
            Wheels_All_Off();
            lf_sub_state  = LF_TURN_WAIT;
            lf_phase_tick = Time_Sequence;
        }
        break;

    //----------------------------------------------------------------------
    // "BL Turn" — stopped after turn, 10 s pause for TA
    //----------------------------------------------------------------------
    case LF_TURN_WAIT:
        if(phase_elapsed >= LF_EVENT_PAUSE){
            // Switch to P7 pin layout and start PD following.
            lf_enter_p7_pins();
            lf_last_error = 0;
            lf_set_display("BL Travel ");
            lf_sub_state  = LF_TRAVEL;
            lf_phase_tick = Time_Sequence;
        }
        break;

    //----------------------------------------------------------------------
    // "BL Travel" — following the line, after LF_TRAVEL_TO_CIRCLE ticks
    //               stop for the BL Circle pause.
    //----------------------------------------------------------------------
    case LF_TRAVEL:
    {
        int left_reading  = (int)ADC_Left_Detect;
        int right_reading = (int)ADC_Right_Detect;
        unsigned int left_on_line  = (ADC_Left_Detect  > threshold_left);
        unsigned int right_on_line = (ADC_Right_Detect > threshold_right);
        int base_err, delta_err;

        if(phase_elapsed >= LF_TRAVEL_TO_CIRCLE){
            // Time to pause for "BL Circle" transition.
            lf_motors_stop();
            lf_exit_p7_pins();              // back to P9P2 for the pause
            lf_set_display("BL Travel ");   // keep showing Travel during pause
            lf_sub_state  = LF_TRAVEL_WAIT;
            lf_phase_tick = Time_Sequence;
            break;
        }

        // PD control (Project_7 style, direct CCR writes).
        if(!left_on_line && !right_on_line){
            lf_motors_reverse(P7_REVERSE_SPEED);
            break;
        }
        base_err      = left_reading - right_reading;
        delta_err     = base_err - lf_last_error;
        lf_last_error = base_err;
        correction = (P7_KP * base_err) + (P7_KD * delta_err);
        correction = correction / P7_PD_DIVISOR;
        left_speed  = (int)P7_BASE_SPEED - correction;
        right_speed = (int)P7_BASE_SPEED + correction;
        if(left_speed  < 0)                 left_speed  = 0;
        if(right_speed < 0)                 right_speed = 0;
        if(left_speed  > (int)P7_MAX_SPEED) left_speed  = (int)P7_MAX_SPEED;
        if(right_speed > (int)P7_MAX_SPEED) right_speed = (int)P7_MAX_SPEED;
        lf_motors_forward((unsigned int)left_speed, (unsigned int)right_speed);
    } break;

    //----------------------------------------------------------------------
    // "BL Travel" — stopped, 10 s pause before "BL Circle"
    //----------------------------------------------------------------------
    case LF_TRAVEL_WAIT:
        if(phase_elapsed >= LF_EVENT_PAUSE){
            lf_enter_p7_pins();
            lf_last_error = 0;
            lf_set_display("BL Circle ");
            lf_sub_state  = LF_CIRCLE;
            lf_phase_tick = Time_Sequence;
        }
        break;

    //----------------------------------------------------------------------
    // "BL Circle" — following the circle indefinitely until ^G exit.
    //----------------------------------------------------------------------
    case LF_CIRCLE:
    {
        int left_reading  = (int)ADC_Left_Detect;
        int right_reading = (int)ADC_Right_Detect;
        unsigned int left_on_line  = (ADC_Left_Detect  > threshold_left);
        unsigned int right_on_line = (ADC_Right_Detect > threshold_right);
        int base_err, delta_err;

        if(!left_on_line && !right_on_line){
            lf_motors_reverse(P7_REVERSE_SPEED);
            break;
        }
        base_err      = left_reading - right_reading;
        delta_err     = base_err - lf_last_error;
        lf_last_error = base_err;
        correction = (P7_KP * base_err) + (P7_KD * delta_err);
        correction = correction / P7_PD_DIVISOR;
        left_speed  = (int)P7_BASE_SPEED - correction;
        right_speed = (int)P7_BASE_SPEED + correction;
        if(left_speed  < 0)                 left_speed  = 0;
        if(right_speed < 0)                 right_speed = 0;
        if(left_speed  > (int)P7_MAX_SPEED) left_speed  = (int)P7_MAX_SPEED;
        if(right_speed > (int)P7_MAX_SPEED) right_speed = (int)P7_MAX_SPEED;
        lf_motors_forward((unsigned int)left_speed, (unsigned int)right_speed);
    } break;

    //----------------------------------------------------------------------
    // EXIT SEQUENCE (triggered by ^G).  P9P2 pins already restored by
    // Line_Follow_Begin_Exit.
    //----------------------------------------------------------------------
    case LF_EXIT_WAIT:
        // "BL Exit" — stopped, 10 s pause for TA.
        if(phase_elapsed >= LF_EVENT_PAUSE){
            lf_set_display(" BL Exit  ");
            Spin_CCW_On();
            lf_phase_tick = Time_Sequence;
            lf_sub_state  = LF_EXIT_TURN;
        }
        break;

    case LF_EXIT_TURN:
        if(phase_elapsed >= LF_EXIT_TURN_TIME){
            Wheels_All_Off();
            Forward_On();
            lf_phase_tick = Time_Sequence;
            lf_sub_state  = LF_EXIT_FWD;
        }
        break;

    case LF_EXIT_FWD:
        if(phase_elapsed >= LF_EXIT_FWD_TIME){
            Wheels_All_Off();
            lf_phase_tick = Time_Sequence;
            lf_sub_state  = LF_EXIT_DONE;
            {
                unsigned int secs = (unsigned int)(lf_ticks / 5);
                unsigned int h, t, o;
                if(secs > 999) secs = 999;
                h = secs / 100; secs -= h * 100;
                t = secs / 10;  secs -= t * 10;
                o = secs;
                // Use lf_set_display for the blink, then overwrite lines 2-4
                // with the custom finish message + time.
                lf_set_display(" BL Stop  ");
                strcpy(display_line[1], " That was ");
                strcpy(display_line[2], " easy! :) ");
                strcpy(display_line[3], "Time: XXXs");
                display_line[3][6] = (char)('0' + h);
                display_line[3][7] = (char)('0' + t);
                display_line[3][8] = (char)('0' + o);
                display_changed = TRUE;
            }
        }
        break;

    case LF_EXIT_DONE:
        // "BL Stop" stays on screen indefinitely.  Car is idle.
        if(phase_elapsed >= 5){
            mode_line_active = 0;
            cmd_remaining_ms = 0;
            cmd_active_dir   = SERIAL_NULL;
        }
        break;

    default:
        mode_line_active = 0;
        Wheels_All_Off();
        break;
    }

    P2OUT ^= IOT_RUN_RED;
}
