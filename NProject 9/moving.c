/*
 * moving.c
 *
 *  Created on: Feb 9, 2026
 *      Author: noah
 */
#include  "msp430.h"
#include  <string.h>
#include  "functions.h"
#include  "LCD.h"
#include  "ports.h"
#include  "macros.h"
#include  "globals.h"

volatile unsigned int lap_count    = 0;
volatile unsigned int on_line_flag = 0;
volatile unsigned int nav_timer    = 0;

void motors_reset(void) {
    RIGHT_FORWARD_SPEED = WHEEL_OFF;
    LEFT_FORWARD_SPEED  = WHEEL_OFF;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;
    LEFT_REVERSE_SPEED  = WHEEL_OFF;
}

void motors_forward(void) {
    RIGHT_REVERSE_SPEED = WHEEL_OFF;
    LEFT_REVERSE_SPEED  = WHEEL_OFF;
    RIGHT_FORWARD_SPEED = SLOW;
    LEFT_FORWARD_SPEED  = SLOW;
}

void motors_reverse(void) {
    RIGHT_FORWARD_SPEED = WHEEL_OFF;
    LEFT_FORWARD_SPEED  = WHEEL_OFF;
    RIGHT_REVERSE_SPEED = SLOW;
    LEFT_REVERSE_SPEED  = SLOW;
}

void Spin_CW_On(void) {
    motors_reset();
    RIGHT_FORWARD_SPEED = SPIN;
    LEFT_REVERSE_SPEED  = SPIN;
}

void Spin_CCW_On(void) {
    motors_reset();
    LEFT_FORWARD_SPEED  = SPIN;
    RIGHT_REVERSE_SPEED = SPIN;
}

// Right wheel only forward — pivots LEFT
void pivot_left_pwm(unsigned int speed) {
    RIGHT_FORWARD_SPEED = WHEEL_OFF;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;
    LEFT_REVERSE_SPEED  = WHEEL_OFF;
    LEFT_FORWARD_SPEED  = speed;
}

// Left wheel only forward — pivots RIGHT
void pivot_right_pwm(unsigned int speed) {
    LEFT_FORWARD_SPEED  = WHEEL_OFF;
    LEFT_REVERSE_SPEED  = WHEEL_OFF;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;
    RIGHT_FORWARD_SPEED = speed;
}

void wait_case(void) {
    if (time_change) {
        time_change = 0;
        if (delay_start++ >= WAITING2START) {
            delay_start = 0;
            state = START;
        }
    }
}

void start_case(void) {
    cycle_time        = 0;
    right_motor_count = 0;
    left_motor_count  = 0;
    segment_count     = 0;
    state = RUN;
}

void end_case(void) {
    motors_reset();
    state = WAIT;
    event = NONE;
}
