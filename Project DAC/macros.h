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

// Timer B0 CCR0 interval -- HW06: continuous mode, ID__8, TBIDEX__8 = 125kHz
// 8,000,000 / 8 / 8 / (1 / 0.200) = 25,000 counts = 200ms per interrupt
// 5 interrupts = 1 second
#define TB0CCR0_INTERVAL     (25000)  // 200ms: 8MHz/8/8/(1/0.2) = 25,000

// Timer B0 CCR1/CCR2 intervals for interrupt-driven switch debounce
// Same divider chain as CCR0 => same 200ms per interrupt
#define TB0CCR1_INTERVAL     (25000)  // 200ms base for SW1 debounce
#define TB0CCR2_INTERVAL     (25000)  // 200ms base for SW2 debounce

// Debounce threshold: number of CCR1/CCR2 interrupts before re-enabling switch
// 5 x 200ms = 1.0 second total debounce period
#define DEBOUNCE_THRESHOLD   (5)      // 5 x 200ms = 1 second

// Time_Sequence wrap value (250 x 200ms = 50 seconds)
#define TIME_SEQ_MAX         (250)

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

// Timing constants (based on 200ms per ISR tick -- HW06 CCR0 interval)
#define ONE_SEC              (5)     // 5 x 200ms = 1.0 second
#define TWO_SEC              (10)    // 10 x 200ms = 2.0 seconds
#define THREE_SEC            (15)    // 15 x 200ms = 3.0 seconds

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
#define DETECT_DELAY_1SEC  (5)     // 5 x 200ms = 1.0 second

// Duration to display "Black Line / Detected" message before turning
// ~3 seconds allows the TA to see the display
#define DETECT_STOP_TIME   (15)    // 15 x 200ms = 3.0 seconds

// Turn duration -- TUNE ON YOUR HARDWARE
// Start with ~2 ticks and adjust until both detectors end up over the line
#define TURN_TIME          (2)     // 2 x 200ms = 0.4 second (adjust by testing)

//------------------------------------------------------------------------------
// Forward speed software PWM -- TUNE ON YOUR HARDWARE
//   The ISR fires every 5ms.  Each PWM "period" is MOTOR_PWM_PERIOD ticks.
//   The motor is ON for MOTOR_DUTY_CYCLE ticks, OFF for the rest.
//
//   HW06 note: software PWM removed from FORWARD state -- timer now fires at
//   200ms intervals, which is too coarse for motor duty cycle control.
//   Motors run at full speed in the FORWARD state.
//------------------------------------------------------------------------------

// end of Project 6 additions //////////////////////////////////////////////////

// Project DAC additions ///////////////////////////////////////////////////////

//------------------------------------------------------------------------------
// DAC Demo State Machine States
//   dac_state is shared between main.c, interrupt_ports.c, and
//   interrupts_timers.c -- all accesses are 16-bit and atomic on MSP430.
//------------------------------------------------------------------------------
#define DAC_IDLE          (0)   // Waiting for SW1 press
#define DAC_STARTUP       (1)   // DAC settling delay (DAC_STARTUP_TICKS x 200ms)
#define DAC_DRIVE         (2)   // Motors running; DAC stepping down each tick
#define DAC_DONE          (3)   // Demo complete; motors stopped

//------------------------------------------------------------------------------
// DAC Tuning Constants -- adjust after hardware measurement
//   DAC_INIT_VALUE : starting DAC code (0x000-0xFFF), near full scale (4000/4096)
//   DAC_MIN_VALUE  : stop threshold; instructor notes ~1200 = ~6V on DAC board
//   DAC_STEP       : codes subtracted per 200ms tick (50 = ~56 steps = ~11 sec)
//   DAC_STARTUP_TICKS : 200ms ticks to wait after SW1 before enabling DAC board
//
// Voltage estimate (Vref = 2.5V):
//   Vout_dac = Vref * n / 4096
//   n=4000 -> ~2.44V DAC output  (DAC board amplifies to higher motor voltage)
//   n=1200 -> ~0.73V DAC output  (DAC board output ~6V per instructor)
//------------------------------------------------------------------------------
#define DAC_INIT_VALUE    (4000)   // Starting code -- near full scale
#define DAC_MIN_VALUE     (1200)   // Stop threshold (~6V on DAC board)
#define DAC_STEP          (50)     // Decrement per 200ms CCR0 tick
#define DAC_STARTUP_TICKS (3)      // 3 x 200ms = 600ms settling time

// end of Project DAC additions ////////////////////////////////////////////////

#endif /* MACROS_H_ */
