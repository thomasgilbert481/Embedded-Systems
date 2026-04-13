/*******************************************************************************
 * File:        project6.c
 * Description: Bang-bang line following / circle navigation
 * Author:      Noah Cartwright
 * Date:        Mar 2026
 * Class:       ECE 306
 ******************************************************************************/

#include "msp430.h"
#include <string.h>
#include "functions.h"
#include "LCD.h"
#include "ports.h"
#include "macros.h"
#include "globals.h"

char display_line[4][11];

// Calibration defaults (overwritten by SW2 calibration routine)
unsigned int BLACK_CAL_L = 730;
unsigned int BLACK_CAL_R = 760;
unsigned int WHITE_CAL_L = 275;
unsigned int WHITE_CAL_R = 290;
unsigned int IR_THRESHOLD = 100;
unsigned char cal_state = 0;

// ir_cnt: incremented in bang-bang when both sensors see white (lost line)
volatile unsigned int ir_cnt = 0;
volatile unsigned int display_timer = 0;

//------------------------------------------------------------------------------
// Bang_Bang_Control
// Called from Circle_Navigation() when in CIRCLING state.
//
// Thresholds (set in macros.h):
//   BLACK_THRESHOLD ~500  — above this = black
//   WHITE_THRESHOLD ~350  — below this = white
//   anything between = gray zone
//------------------------------------------------------------------------------
void Bang_Bang_Control(void)
{
    // booleans basically - these will be 1 or 0 depending on what the sensors read
    char left_black, right_black;
    char left_white, right_white;


    // if ADC value is high enough -> its seeing black
    // if ADC value is low enough -> its seeing white
    // anything in between = gray zone (neither flag gets set)
    left_black = (ADC_Left_Det >= BLACK_THRESHOLD);
    right_black = (ADC_Right_Det >= BLACK_THRESHOLD);
    left_white = (ADC_Left_Det <= WHITE_THRESHOLD);
    right_white = (ADC_Right_Det <= WHITE_THRESHOLD);

    // stop both motors before we decide what to do
    // this prevents conflicting PWM signals from the last iteration
    motors_reset();

    // both sensors see black = we are sitting right on the line
    // just go straight and reset the lost-line counter
    if (left_black && right_black)
    {
        motors_forward();
        ir_cnt = 0; // not lost anymore
    }

    // left sees black, right sees white

    else if (left_black && right_white)
    {
        pivot_left_pwm(SLOW);
        ir_cnt = 0;
    }

    // opposite of above
    // right sees black, left sees white = drifted too far left
    // pivot right to correct
    else if (left_white && right_black)
    {
        pivot_right_pwm(SLOW);
        ir_cnt = 0;
    }

    // both sensors see white = we totally lost the line
    // first try reversing for a bit (SEARCH_FWD ticks) to go back where we came from
    // ir_cnt keeps track of how long weve been lost
    else if (left_white && right_white)
    {
        if (ir_cnt < SEARCH_FWD)
        {
            motors_reverse(); // back up first
        }
        else
        {
            motors_forward(); // gave up backing up, try going forward
        }
        ir_cnt++; // been lost for one more tick
    }

    // gray zone - one or both sensors are between WHITE_THRESHOLD and BLACK_THRESHOLD
    // use whichever sensor has a definitive reading and steer toward the black side
    else
    {
        if (left_black)
        {
            // left is definitely on black, right is somewhere in the middle
            // line is probably to the left so pivot that way
            pivot_left_pwm(SLOW);
        }
        else if (right_black)
        {
            // right is definitely on black, line is to the right
            pivot_right_pwm(SLOW);
        }
        else if (left_white)
        {
            // left is definitely off the line (white), so line must be to the right
            pivot_right_pwm(SLOW);
        }
        else if (right_white)
        {
            // right is definitely off the line, so line must be to the left
            pivot_left_pwm(SLOW);
        }
        else
        {
            // both sensors are in the gray band, no idea whats going on
            // just keep moving and hope next reading is cleaner
            motors_forward();
        }
        ir_cnt = 0; // we have at least some info so we're not "lost"
    }

    // line 0: show raw ADC readings so we can see what the sensors are actually doing

    display_line[0][0] = 'L';
    display_line[0][1] = ':';
    display_line[0][2] = (ADC_Left_Det / 100) % 10 + '0';  // hundreds digit
    display_line[0][3] = (ADC_Left_Det / 10) % 10 + '0';   // tens digit
    display_line[0][4] = (ADC_Left_Det) % 10 + '0';        // ones digit
    display_line[0][5] = ' ';
    display_line[0][6] = 'R';
    display_line[0][7] = ':';
    display_line[0][8] = (ADC_Right_Det / 100) % 10 + '0';
    display_line[0][9] = (ADC_Right_Det / 10) % 10 + '0';
    display_line[0][10] = '\0';

    // line 1: elapsed time display
    // display_timer gets incremented every time this function runs which is every 200ms
    unsigned int whole = display_timer / 5;
    unsigned int tenths = (display_timer % 5) * 2;


    display_line[1][0] = 'T';
    display_line[1][1] = ':';
    display_line[1][2] = (whole / 100) % 10 + '0';  // hundreds of seconds
    display_line[1][3] = (whole / 10) % 10 + '0';   // tens of seconds
    display_line[1][4] = (whole) % 10 + '0';         // ones of seconds
    display_line[1][5] = '.';
    display_line[1][6] = tenths + '0';
    display_line[1][7] = 's';
    display_line[1][8] = ' ';
    display_line[1][9] = ' ';
    display_line[1][10] = '\0';


    display_changed = TRUE;
    update_display = TRUE;
}

//------------------------------------------------------------------------------
// Circle_Navigation - main state machine, called from main loop
//------------------------------------------------------------------------------
void Circle_Navigation(void)
{
    switch (state)
    {

    case IDLE:
        break;

        //----------------------------------------------------------------------
        // DELAY: SW1 was pressed — wait ~3 seconds before starting,
        //        then enable IR LED and begin driving toward the circle
        //----------------------------------------------------------------------
    case DELAY:
        if (Time_Sequence >= THREE_SEC)
        {
            P2OUT |= IR_LED;
            motors_forward();
            RIGHT_FORWARD_SPEED = SLOW;
            LEFT_FORWARD_SPEED = SLOW;
            strcpy(display_line[2], "Driving   ");
            display_changed = TRUE;
            update_display = TRUE;
            state = DRIVING;
        }
        break;

        //----------------------------------------------------------------------
        // DRIVING: heading toward circle — stop when either sensor hits black
        //----------------------------------------------------------------------
    case DRIVING:
        if (ADC_Right_Det > BLACK_THRESHOLD || ADC_Left_Det > BLACK_THRESHOLD)
        {
            motors_reset();
            Time_Sequence = 0;
            strcpy(display_line[2], "Found     ");
            display_changed = TRUE;
            update_display = TRUE;
            state = LINE_FOUND;
        }
        break;

        //----------------------------------------------------------------------
        // LINE_FOUND: brief pause, then spin to align tangent to circle
        //----------------------------------------------------------------------
    case LINE_FOUND:
        if (Time_Sequence >= ONE_SEC)
        {
            Spin_CCW_On();
            Time_Sequence = 0;
            strcpy(display_line[2], "Aligning  ");
            display_changed = TRUE;
            update_display = TRUE;
            state = TURNING;
        }
        break;

        //----------------------------------------------------------------------
        // TURNING: spin for TURN_TICKS then start bang-bang circle following
        //----------------------------------------------------------------------
    case TURNING:
        if (Time_Sequence >= TURN_TICKS)
        {
            motors_reset();
            Time_Sequence = 0;
            ir_cnt = 0;
            strcpy(display_line[2], "Circling  ");
            display_changed = TRUE;
            update_display = TRUE;
            state = CIRCLING;
        }
        break;

        //----------------------------------------------------------------------
        // CIRCLING: bang-bang line following
        //           exit after CIRCLE_TIME ticks
        //----------------------------------------------------------------------
    case CIRCLING:
        Bang_Bang_Control();
        display_timer++;
        if (Time_Sequence >= THREE_SEC * 12)
        {
            motors_reset();
            Time_Sequence = 0;
            strcpy(display_line[2], "Exiting   ");
            display_changed = TRUE;
            update_display = TRUE;
            state = EXITING;
        }
        break;

        //----------------------------------------------------------------------
        // EXITING: spin CW to face outward, then drive straight off the circle
        //----------------------------------------------------------------------
    case EXITING:
        display_timer++;
        if (Time_Sequence < ONE_HALF_SEC)
        {
            Spin_CCW_On();
        }
        else if (Time_Sequence < ONE_SEC * 3)
        {
            motors_forward();
        }
        else
        {
            motors_reset();
            P2OUT &= ~IR_LED;
            display_line[1][0] = 'T';
            display_line[1][1] = ':';
            unsigned int w = display_timer / 5;
            unsigned int t = (display_timer % 5) * 2;
            display_line[1][2] = (w / 100) % 10 + '0';
            display_line[1][3] = (w / 10) % 10 + '0';
            display_line[1][4] = (w) % 10 + '0';
            display_line[1][5] = '.';
            display_line[1][6] = t + '0';
            display_line[1][7] = 's';
            display_line[1][8] = ' ';
            display_line[1][9] = ' ';
            display_line[1][10] = '\0';
            strcpy(display_line[2], "Stopped   ");
            display_changed = TRUE;
            update_display = TRUE;
            state = STOPPED;
        }
        break;

    case STOPPED:
        break;

    default:
        break;
    }
}
