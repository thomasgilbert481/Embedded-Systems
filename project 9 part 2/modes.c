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

// Active-command state (in iot.c) -- used by Line_Follow_Start to set up
// the auto-stop countdown.
extern volatile unsigned int  cmd_remaining_ms;
extern volatile char          cmd_active_dir;
extern volatile unsigned int  cmd_active_time;

//------------------------------------------------------------------------------
// Helper: print a decimal uint via USB_transmit_string
//------------------------------------------------------------------------------
static void usb_print_uint(unsigned int v){
    char    buf[8];
    char    out[9];
    unsigned int i = 0;
    unsigned int j;

    if(v == 0){
        buf[i++] = '0';
    } else {
        while(v > 0 && i < sizeof(buf)){
            buf[i++] = (char)('0' + (v % 10));
            v /= 10;
        }
    }
    for(j = 0; j < i; j++){
        out[j] = buf[i - 1 - j];
    }
    out[i] = SERIAL_NULL;
    USB_transmit_string(out);
}

//------------------------------------------------------------------------------
// Helper: write "AA:dddd  " into display_line[line_idx] where AA is a 2-char
// label and dddd is a zero-padded 4-digit decimal value.  Pads to 10 chars.
//------------------------------------------------------------------------------
static void lcd_write_value(unsigned int line_idx, const char *label, unsigned int value){
    unsigned int thousands, hundreds, tens, ones;

    if(line_idx > 3) return;

    // Clamp to 4 digits
    if(value > 9999) value = 9999;

    thousands = value / 1000;        value -= thousands * 1000;
    hundreds  = value / 100;         value -= hundreds  * 100;
    tens      = value / 10;          value -= tens      * 10;
    ones      = value;

    display_line[line_idx][0] = label[0];
    display_line[line_idx][1] = label[1];
    display_line[line_idx][2] = ':';
    display_line[line_idx][3] = (char)('0' + thousands);
    display_line[line_idx][4] = (char)('0' + hundreds);
    display_line[line_idx][5] = (char)('0' + tens);
    display_line[line_idx][6] = (char)('0' + ones);
    display_line[line_idx][7] = ' ';
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
static unsigned long cal_settle_cnt = 0;

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
    cal_settle_cnt  = 0;
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
                cal_settle_cnt = 0;
                strcpy(display_line[2], " Sampling ");
                display_changed = TRUE;
                cal_sub_state   = CAL_ST_SAMPLE_WHITE;
            }
            break;

        case CAL_ST_SAMPLE_WHITE:
            // Spin main-loop iterations to let the ADC settle on the new surface.
            // At ~8000 iter/s, 8000 = ~1 s.
            if(++cal_settle_cnt > 8000){
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
                cal_settle_cnt = 0;
                strcpy(display_line[2], " Sampling ");
                display_changed = TRUE;
                cal_sub_state   = CAL_ST_SAMPLE_BLACK;
            }
            break;

        case CAL_ST_SAMPLE_BLACK:
            if(++cal_settle_cnt > 8000){
                black_left  = ADC_Left_Detect;
                black_right = ADC_Right_Detect;

                // Midpoint thresholds
                threshold_left  = (white_left  + black_left)  / 2;
                threshold_right = (white_right + black_right) / 2;

                calibration_done = 1;
                USB_transmit_string("CAL done\r\n");
                dump_cal_values();   // Show everything we just captured

                cal_sub_state = CAL_ST_FINISH;
                cal_settle_cnt = 0;
            }
            break;

        case CAL_ST_FINISH:
            // Show the 4 captured values on the LCD for a few seconds so the
            // user can verify them before returning to the IP layout.
            lcd_show_cal_values();
            // ~5 s of main-loop iterations at ~8000 iter/s
            if(++cal_settle_cnt > 40000){
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
//==============================================================================
void Line_Follow_Start(unsigned int seconds){
    if(!calibration_done){
        USB_transmit_string("ERR: not calibrated\r\n");
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

    USB_transmit_string("LINE start\r\n");
}

//------------------------------------------------------------------------------
// Periodic LCD update during line-follow.  Every ~0.25 s of main-loop ticks
// redraw the four lines with current ADC readings and normalized values:
//   Line 0: "L :dddd   "  raw left  ADC
//   Line 1: "R :dddd   "  raw right ADC
//   Line 2: "Ln:dddd   "  left  normalized [0,100]
//   Line 3: "Rn:dddd   "  right normalized [0,100]
// (line_dbg_cnt / LINE_DBG_INTERVAL declared above so Line_Follow_Start can
// force an immediate redraw.)
//------------------------------------------------------------------------------
static void line_follow_display(int left_norm, int right_norm){
    if(++line_dbg_cnt < LINE_DBG_INTERVAL){
        return;
    }
    line_dbg_cnt = 0;
    lcd_write_value(0, "L ", ADC_Left_Detect);
    lcd_write_value(1, "R ", ADC_Right_Detect);
    lcd_write_value(2, "Ln", (unsigned int)(left_norm  < 0 ? 0 : left_norm));
    lcd_write_value(3, "Rn", (unsigned int)(right_norm < 0 ? 0 : right_norm));
    display_changed = TRUE;
}

//==============================================================================
// Line_Follow_Tick -- called from main loop every iteration while active.
// Proportional steering.  ADC_Left_Detect/Right_Detect updated by ADC ISR.
// cmd_remaining_ms countdown (and motor stop) is handled by Vehicle_Cmd_Tick.
//==============================================================================
void Line_Follow_Tick(void){
    unsigned int left_range;
    unsigned int right_range;
    int left_norm;
    int right_norm;
    int error;
    int correction;
    int left_speed;
    int right_speed;

    if(!mode_line_active){
        return;
    }
    if(cmd_remaining_ms == 0){
        // Countdown expired -- Vehicle_Cmd_Tick already stopped motors.
        mode_line_active = 0;
        Display_Network_Info();   // Restore IP layout on the LCD
        return;
    }

    left_range  = (black_left  > white_left)  ? (black_left  - white_left)  : 1;
    right_range = (black_right > white_right) ? (black_right - white_right) : 1;

    // Normalize each sensor to [0, 100]: 0 = white, 100 = black
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

    error      = right_norm - left_norm;
    correction = FOLLOW_KP * error;

    line_follow_display(left_norm, right_norm);

    left_speed  = (int)FOLLOW_BASE + correction;
    right_speed = (int)FOLLOW_BASE - correction;

    if(left_speed  < 0)                       left_speed  = 0;
    if(right_speed < 0)                       right_speed = 0;
    if(left_speed  > (int)FOLLOW_MAX_PWM) left_speed  = (int)FOLLOW_MAX_PWM;
    if(right_speed > (int)FOLLOW_MAX_PWM) right_speed = (int)FOLLOW_MAX_PWM;

    // Drive forward only -- clear reverse channels first (safety)
    LEFT_REVERSE_SPEED  = 0;
    RIGHT_REVERSE_SPEED = 0;
    LEFT_FORWARD_SPEED  = (unsigned int)left_speed;
    RIGHT_FORWARD_SPEED = (unsigned int)right_speed;
}
