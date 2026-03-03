//==============================================================================
// File Name: macros.h
// Description: Contains all #define macros for the project
// Author: Thomas Gilbert
// Date: Feb 2026
// Compiler: Code Composer Studio
//==============================================================================

#ifndef MACROS_H_
#define MACROS_H_

#define ALWAYS                  (1)
#define RESET_STATE             (0)
#define RED_LED              (0x01) // RED LED 0
#define GRN_LED              (0x40) // GREEN LED 1
#define TEST_PROBE           (0x01) // 0 TEST PROBE
#define TRUE                 (0x01) //

// Project 5 additions /////////////////////////////////////////////////////////

// Timer B0 CCR0 interval
// SMCLK = 8 MHz, so 40000 counts = 5ms per interrupt
// 200 interrupts = 1 second
#define TB0CCR0_INTERVAL     (40000)

// Movement sequence step numbers
#define P5_FWD1              (1)
#define P5_PAUSE1            (2)
#define P5_REV               (3)
#define P5_PAUSE2            (4)
#define P5_FWD2              (5)
#define P5_PAUSE3            (6)
#define P5_SPIN_CW           (7)
#define P5_PAUSE4            (8)
#define P5_SPIN_CCW          (9)
#define P5_PAUSE5            (10)
#define P5_DONE              (11)

// Timing constants (based on 5ms per ISR tick)
// Adjust these if your timing is off during testing!
#define ONE_SEC              (200)   // 200 * 5ms = 1.0 second
#define TWO_SEC              (400)   // 400 * 5ms = 2.0 seconds
#define THREE_SEC            (600)   // 600 * 5ms = 3.0 seconds

// end of Project 5 additions //////////////////////////////////////////////////

// Project 6 additions /////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
// ADC Channel Sequence Indices (used in ADC_ISR switch to track which channel
// was just converted -- these index into the adc_channel counter, NOT the
// hardware channel numbers)
//------------------------------------------------------------------------------
#define ADC_CHANNEL_SEQ_LEFT     (0)  // adc_channel value when A2 just finished
#define ADC_CHANNEL_SEQ_RIGHT    (1)  // adc_channel value when A3 just finished
#define ADC_CHANNEL_SEQ_THUMB    (2)  // adc_channel value when A5 just finished

//------------------------------------------------------------------------------
// Black line detection threshold -- TUNE ON YOUR HARDWARE
//   IR behavior (emitter ON):
//     White surface -> more reflected IR -> phototransistor pulls voltage DOWN
//                    -> lower ADC reading
//     Black surface -> less reflected IR -> voltage stays HIGH
//                    -> higher ADC reading
//   Set threshold midway between your white and black readings.
//   Example: white ~0x0100, black ~0x0500 -> threshold ~0x0300
//------------------------------------------------------------------------------
#define BLACK_LINE_THRESHOLD     (0x0300)  // Adjust after hardware calibration

//------------------------------------------------------------------------------
// Project 6 State Machine -- Black Line Detection Sequence
//------------------------------------------------------------------------------
#define P6_IDLE            (0)  // Waiting for SW1 press
#define P6_WAIT_1SEC       (1)  // 1-second delay before moving (fingers clear)
#define P6_FORWARD         (2)  // Driving forward, monitoring ADC for black line
#define P6_DETECTED_STOP   (3)  // Stopped -- showing "Black Line / Detected"
#define P6_TURNING         (4)  // Turning to align detectors over the line
#define P6_ALIGNED         (5)  // Aligned -- displaying live black line ADC values

//------------------------------------------------------------------------------
// Project 6 Timing Constants (5ms per ISR tick, 200 ticks = 1 second)
//------------------------------------------------------------------------------
// 1-second delay after SW1 before moving forward
#define DETECT_DELAY_1SEC  (200)   // 200 * 5ms = 1.0 second

// Duration to display "Black Line / Detected" message before turning
// ~3 seconds allows the TA to see the display
#define DETECT_STOP_TIME   (600)   // 600 * 5ms = 3.0 seconds

// Turn duration -- TUNE ON YOUR HARDWARE
// Start with ~1 second and adjust until both detectors end up over the line
#define TURN_TIME          (50)   // 50 * 5ms = 0.25 second (adjust by testing)

//------------------------------------------------------------------------------
// Forward speed software PWM -- TUNE ON YOUR HARDWARE
//   The ISR fires every 5ms.  Each PWM "period" is MOTOR_PWM_PERIOD ticks.
//   The motor is ON for MOTOR_DUTY_CYCLE ticks, OFF for the rest.
//
//   Speed % = MOTOR_DUTY_CYCLE / MOTOR_PWM_PERIOD
//   Examples (MOTOR_PWM_PERIOD = 10):
//     MOTOR_DUTY_CYCLE = 10  -> 100% (full speed, same as before)
//     MOTOR_DUTY_CYCLE =  7  ->  70%
//     MOTOR_DUTY_CYCLE =  5  ->  50%
//     MOTOR_DUTY_CYCLE =  3  ->  30%
//------------------------------------------------------------------------------
#define MOTOR_PWM_PERIOD   (10)   // PWM window in ticks (10 * 5ms = 50ms cycle)
#define MOTOR_DUTY_CYCLE   (3)    // Ticks motor is ON per window -- tune this

// end of Project 6 additions //////////////////////////////////////////////////

#endif /* MACROS_H_ */
