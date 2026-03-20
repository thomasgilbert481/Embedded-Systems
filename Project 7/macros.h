//==============================================================================
// File Name: macros.h
// Description: Contains all #define macros for Project 7 (Circle Following).
//              Includes all Project 5/6 defines for compatibility, plus all
//              Project 7 state machine, timing, PD control, DAC, and speed
//              defines.
// Author: Thomas Gilbert
// Date: Mar 2026
// Compiler: Code Composer Studio
//==============================================================================

#ifndef MACROS_H_
#define MACROS_H_

#define ALWAYS                  (1)
#define RESET_STATE             (0)
#define RED_LED              (0x01)
#define GRN_LED              (0x40)
#define TEST_PROBE           (0x01)
#define TRUE                 (0x01)
#define FALSE                (0x00)

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

// Timing constants (5ms per ISR tick)
#define ONE_SEC              (200)   // 200 * 5ms = 1.0 second
#define TWO_SEC              (400)   // 400 * 5ms = 2.0 seconds
#define THREE_SEC            (600)   // 600 * 5ms = 3.0 seconds

// end of Project 5 additions //////////////////////////////////////////////////

// Project 6 additions /////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
// ADC Channel Sequence Indices
//------------------------------------------------------------------------------
#define ADC_CHANNEL_SEQ_LEFT     (0)
#define ADC_CHANNEL_SEQ_RIGHT    (1)
#define ADC_CHANNEL_SEQ_THUMB    (2)

//------------------------------------------------------------------------------
// Black line detection threshold (kept for P6 compatibility)
//------------------------------------------------------------------------------
#define BLACK_LINE_THRESHOLD     (0x0300)

//------------------------------------------------------------------------------
// Project 6 State Machine
//------------------------------------------------------------------------------
#define P6_IDLE            (0)
#define P6_WAIT_1SEC       (1)
#define P6_FORWARD         (2)
#define P6_DETECTED_STOP   (3)
#define P6_TURNING         (4)
#define P6_ALIGNED         (5)

//------------------------------------------------------------------------------
// Project 6 Timing Constants (5ms per ISR tick)
//------------------------------------------------------------------------------
#define DETECT_DELAY_1SEC  (200)   // 200 * 5ms = 1.0 second
#define DETECT_STOP_TIME   (600)   // 600 * 5ms = 3.0 seconds
#define TURN_TIME          (200)   // 200 * 5ms = 1.0 second

//------------------------------------------------------------------------------
// Software PWM (P6 -- kept for reference)
//------------------------------------------------------------------------------
#define MOTOR_PWM_PERIOD   (10)
#define MOTOR_DUTY_CYCLE   (5)

// end of Project 6 additions //////////////////////////////////////////////////

// ===== Project 7 additions ===================================================

//------------------------------------------------------------------------------
// State Machine States
//------------------------------------------------------------------------------
#define P7_IDLE              (0)   // Waiting for SW1. Show live ADC. IR OFF.
#define P7_CALIBRATE         (1)   // Multi-phase: ambient -> white -> black
#define P7_CAL_COMPLETE      (2)   // Cal done. User repositions. Press SW1.
#define P7_WAIT_START        (3)   // 2-sec delay after SW1 before driving
#define P7_FORWARD           (4)   // Drive forward toward black line
#define P7_DETECTED_STOP     (5)   // Briefly stop on line detection (~1 sec)
#define P7_TURNING           (6)   // Rotate to align both detectors
#define P7_FOLLOW_LINE       (7)   // PD line following for two laps
#define P7_EXIT_TURN         (8)   // After 2 laps, spin toward circle center
#define P7_EXIT_FORWARD      (9)   // Drive briefly into center
#define P7_DONE              (10)  // Stopped. Timer frozen.

//------------------------------------------------------------------------------
// Calibration sub-phases
//------------------------------------------------------------------------------
#define CAL_AMBIENT          (0)   // IR OFF: sample ambient
#define CAL_WHITE            (1)   // IR ON: sample white surface
#define CAL_WAIT_BLACK       (2)   // Waiting for SW1 to place on black tape
#define CAL_BLACK            (3)   // Sample black tape, compute thresholds
#define CAL_DONE             (4)   // Calibration complete

//------------------------------------------------------------------------------
// Calibration timing (5ms ISR ticks)
//------------------------------------------------------------------------------
#define CAL_SAMPLE_DELAY     (200)   // 200 * 5ms = 1 second

//------------------------------------------------------------------------------
// DAC Motor Voltage Control
//------------------------------------------------------------------------------
#define DAC_INITIAL_VALUE    (1500)  // Safe startup (low motor voltage)
#define DAC_TARGET_VALUE     (1200)  // Target (~6V motor supply -- TUNE THIS)
#define DAC_RAMP_STEP        (50)    // Subtract from DAC_data per timer tick
#define DAC_ENB_DELAY        (3)     // ISR ticks before enabling DAC_ENB

//------------------------------------------------------------------------------
// PD Line Following Control (TUNE ALL ON HARDWARE)
//   Raw correction = (Kp * error) + (Kd * delta_error)
//   Divided by PD_SCALE_DIVISOR to fit PWM range.
//   Start with Kp=1, Kd=5, divisor=10 and adjust on hardware.
//------------------------------------------------------------------------------
#define KP_VALUE             (1)
#define KD_VALUE             (3)
#define PD_SCALE_DIVISOR     (20)

//------------------------------------------------------------------------------
// Motor speeds (Timer B3 PWM -- must be < WHEEL_PERIOD_VAL = 50005)
//------------------------------------------------------------------------------
#define WHEEL_PERIOD_VAL     (50005) // Timer B3 period count (~160 Hz at 8 MHz)
#define BASE_FOLLOW_SPEED    (20000) // Nominal PD following speed
#define MAX_FOLLOW_SPEED     (35000) // Max speed after PD correction
#define FOLLOW_SPEED         (25000) // Default forward/reverse speed
#define SPIN_SPEED           (20000) // Speed for spin turns
#define REVERSE_SPEED        (15000) // Reverse when line lost
#define PERCENT_80           (40004) // 80% of WHEEL_PERIOD_VAL

//------------------------------------------------------------------------------
// Wait/delay times (5ms ticks)
//------------------------------------------------------------------------------
#define P7_WAIT_START_TIME   (1000)  // 1000 * 5ms = 5 sec before driving
#define P7_DETECT_STOP_TIME  (200)   // 200 * 5ms = 1 sec on detection
#define P7_INITIAL_TURN_TIME (200)   // 200 * 5ms = 1 sec alignment (TUNE)

//------------------------------------------------------------------------------
// Lap timing (0.2s "tenths" units; 1 tenth = 0.2 seconds)
//------------------------------------------------------------------------------
#define ONE_LAP_TIME_TENTHS  (100)   // ~20 sec per lap (TUNE on hardware)
#define TWO_LAP_TIME_TENTHS  (200)   // ~40 sec for 2 laps (TUNE on hardware)

//------------------------------------------------------------------------------
// Exit maneuver (5ms ticks)
//------------------------------------------------------------------------------
#define EXIT_TURN_TIME       (300)   // 300 * 5ms = 1.5 sec spin (TUNE)
#define EXIT_FORWARD_TIME    (200)   // 200 * 5ms = 1 sec drive in (TUNE)

//------------------------------------------------------------------------------
// Display timer
//   DISPLAY_TIMER_TICKS: how many 5ms ISR ticks = one 0.2-second elapsed_tenth
//   40 ticks * 5ms = 200ms = 0.2s
//------------------------------------------------------------------------------
#define DISPLAY_TIMER_TICKS  (40)    // 40 * 5ms = 200ms per elapsed_tenth

// ===== end of Project 7 additions ============================================

#endif /* MACROS_H_ */
