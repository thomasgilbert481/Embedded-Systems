/*
 * globals.h
 *
 *  Created on: Feb 19, 2026
 *      Author: noah
 */

#ifndef GLOBALS_H_
#define GLOBALS_H_

// Debounce
extern volatile unsigned int sw1_debounce_count;
extern volatile unsigned int sw2_debounce_count;
extern volatile char         sw1_debounce_active;
extern volatile char         sw2_debounce_active;

// Timing
extern volatile unsigned int cycle_time;
extern volatile unsigned int Last_Time_Sequence;
extern volatile unsigned int time_change;
extern volatile unsigned int Time_Sequence;
extern volatile char         one_time;
extern volatile unsigned char display_changed;
extern volatile unsigned char update_display;

// Motor control
extern volatile unsigned int left_motor_count;
extern volatile unsigned int right_motor_count;
extern volatile unsigned int segment_count;
extern volatile unsigned int delay_start;

// State machine
extern volatile char state;
extern volatile char event;

// ADC
extern volatile char         adc_ready;
extern unsigned int          ADC_Thumb;
extern unsigned int          ADC_Right_Det;
extern unsigned int          ADC_Left_Det;
extern unsigned char         ADC_Channel;
extern unsigned int          DAC_data;
extern char                  adc_char[];
extern int                   i;

// Display
extern char display_line[4][11];

// Navigation
extern volatile unsigned int lap_count;
extern volatile unsigned int on_line_flag;

// Calibration
extern unsigned int  BLACK_CAL_L;
extern unsigned int  BLACK_CAL_R;
extern unsigned int  WHITE_CAL_L;
extern unsigned int  WHITE_CAL_R;
extern unsigned int  IR_THRESHOLD;
extern unsigned char cal_state;

extern volatile unsigned int ir_cnt;

extern volatile unsigned int display_timer;  // counts in 200ms ticks

// Serial communications (serial.c)
extern volatile char          current_baud;      // BAUD_115200 / BAUD_9600 / BAUD_460800
extern volatile char          pc_ready;           // OFF until first PC char received
extern volatile char          tx1_busy;           // UCA1 TX string in progress
extern volatile char          uca1_tx_buf[32];    // UCA1 TX string buffer
extern volatile char          rx1_buf[11];        // UCA1 RX display buffer
extern volatile unsigned int  rx1_idx;
extern volatile char          rx1_update;
extern const char             str_115200[];       // "  115,200 "
extern const char             str_460800[];       // "  460,800 "
extern const char             str_baud[];         // "   Baud   "

// SW action flags — set by ISR, cleared by main
extern volatile char          sw1_action_pending;
extern volatile char          sw2_action_pending;

// Project 8 UCA0 message buffers (serial.c)
extern volatile char          uca0_rx_buf[11];
extern volatile char          uca0_rx_done;
extern volatile char          uca0_tx_busy;

// Project 9 command parser (serial.c)
extern volatile char          cmd_buf[];
extern volatile char          cmd_active;
extern volatile char          cmd_ready;

// Project 9 IOT network info (serial.c)
extern char                   iot_ssid[];
extern char                   iot_ip[];
extern volatile unsigned int  motor_cmd_ticks;


// Timing constants not in macros.h
#define WHEEL_COUNT_TIME    (50)
#define RIGHT_COUNT_TIME    (25)
#define LEFT_COUNT_TIME     (45)
#define TRAVEL_DISTANCE     (100)
#define ONE_HALF_SEC       (9)
#define ONE_SEC             (20)
#define TWO_SEC             (40)
#define THREE_SEC           (60)
#define FIFTY_MS_COUNT      (4)


#endif /* GLOBALS_H_ */
