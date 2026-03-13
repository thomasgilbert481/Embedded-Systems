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
// Project 6 Timing Constants (200ms per ISR tick, 5 ticks = 1 second)
//------------------------------------------------------------------------------
#define DETECT_DELAY_1SEC  (5)     // 5 x 200ms = 1.0 second
#define DETECT_STOP_TIME   (15)    // 15 x 200ms = 3.0 seconds
#define TURN_TIME          (2)     // 2 x 200ms = 0.4 second (adjust by testing)

// end of Project 6 additions //////////////////////////////////////////////////

// Project 7 additions /////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
// Project 7 State Machine States
//------------------------------------------------------------------------------
#define P7_IDLE              (0)  // Waiting for SW1
#define P7_CALIBRATE         (1)  // Three-phase calibration: ambient, white, black
#define P7_WAIT_START        (2)  // Wait for SW1 press to reposition car before start
#define P7_FORWARD           (3)  // Drive forward from circle center toward black line
#define P7_DETECTED_STOP     (4)  // Brief stop on line detection (~1 sec) to confirm
#define P7_TURNING           (5)  // Rotate to align both detectors over the line
#define P7_FOLLOW_LINE       (6)  // PID line following -- core following state
#define P7_EXIT_TURN         (7)  // After 2 laps, turn into circle center
#define P7_DONE              (8)  // Stopped inside circle; timer frozen
#define P7_ARMED             (9)  // Calibration done; waiting 2s then start forward

//------------------------------------------------------------------------------
// Calibration sub-phases (used within P7_CALIBRATE)
//------------------------------------------------------------------------------
#define CAL_AMBIENT          (0)  // Emitter OFF: sample ambient light
#define CAL_WHITE            (1)  // Emitter ON: sample white surface
#define CAL_WAIT_BLACK       (2)  // Waiting for SW1 press to place on black tape
#define CAL_BLACK            (3)  // Sample black line readings
#define CAL_DONE             (4)  // Calibration complete

//------------------------------------------------------------------------------
// Calibration timing (in 200ms ISR ticks; ONE_SEC = 5 ticks)
//------------------------------------------------------------------------------
#define CAL_SAMPLE_DELAY     (5)   // 5 x 200ms = 1 second wait before sampling

//------------------------------------------------------------------------------
// Line following speeds (for hardware PWM via Timer B3)
//   WHEEL_PERIOD_VAL is the Timer B3 CCR0 period (50005 cycles at 8 MHz SMCLK)
//   Duty cycle = speed value / WHEEL_PERIOD_VAL
//   WHEEL_OFF = 0 (0% duty cycle -- motor stopped)
//   Must be less than WHEEL_PERIOD_VAL.
//   NEVER set a forward AND reverse CCR to nonzero simultaneously.
//------------------------------------------------------------------------------
#define WHEEL_PERIOD_VAL     (50005) // Timer B3 PWM period (~160 Hz at 8 MHz)
#define FOLLOW_FAST          (35000) // ~70% duty -- straight-ahead burst speed
#define FOLLOW_SPEED         (25000) // ~50% duty -- normal following speed
#define FOLLOW_SLOW          (20000) // ~40% duty -- inner-wheel correction (TUNED: was 12000)
#define FOLLOW_SEARCH        (25000) // ~50% duty -- both-off search/recovery (TUNED: was 15000)
#define SPIN_SPEED           (25000) // ~50% duty -- spin turns and alignment

//------------------------------------------------------------------------------
// PID line following constants
//   FOLLOW_BASE: base speed applied to both wheels (0% correction = straight)
//   FOLLOW_KP:   proportional gain; correction = FOLLOW_KP * error
//                error range: [-100, 100], so max correction = FOLLOW_KP * 100
//                TUNE FOLLOW_KP on hardware (start low ~100, increase if sluggish)
//------------------------------------------------------------------------------
#define FOLLOW_BASE          (25000) // Base PWM: ~50% duty
#define FOLLOW_KP            (100)   // Proportional gain (TUNE ON HARDWARE)
// Maximum PWM output for PID clamp.
// MUST be < 32768 (fits in signed 16-bit int) AND < WHEEL_PERIOD_VAL (50005).
// Writing CCRx = WHEEL_PERIOD_VAL = CCR0 with OUTMOD_7 gives 0% duty -- avoid!
#define FOLLOW_MAX_PWM       (32000) // Safe PID output ceiling (fits in int)

//------------------------------------------------------------------------------
// Wait / delay times (in 200ms ISR ticks; ONE_SEC = 5 ticks)
//------------------------------------------------------------------------------
#define P7_WAIT_START_TIME   (10)    // 10 x 200ms = 2-second operator step-back delay
#define P7_DETECT_STOP_TIME  (5)     // 5 x 200ms = 1-second pause after line detection
#define P7_INITIAL_TURN_TIME (5)     // 5 x 200ms = 1-second alignment spin (TUNE THIS)

//------------------------------------------------------------------------------
// Line-following lap timing
//   All in units of elapsed_tenths (0.2-second increments).
//   Each CCR0 ISR tick = 200ms = 0.2s, so 1 elapsed_tenth = 1 tick.
//   ONE_LAP_TIME_TENTHS: approximate time for one lap (TUNE ON HARDWARE)
//   TWO_LAP_TIME_TENTHS: total for two laps before exit
//------------------------------------------------------------------------------
#define ONE_LAP_TIME_TENTHS  (100)   // 100 x 0.2s = 20 seconds per lap (TUNE THIS)
#define TWO_LAP_TIME_TENTHS  (200)   // 200 x 0.2s = 40 seconds for 2 laps (TUNE THIS)

//------------------------------------------------------------------------------
// Exit maneuver timing (in 200ms ISR ticks)
//------------------------------------------------------------------------------
#define EXIT_TURN_TIME       (8)     // 8 x 200ms = 1.6-second spin into center (TUNE)
#define EXIT_FORWARD_TIME    (5)     // 5 x 200ms = 1-second drive into center (TUNE)

//------------------------------------------------------------------------------
// 0.2-second display timer
//   Timer B0 CCR0 fires every 200ms.  Each ISR tick IS one 0.2-second increment.
//   DISPLAY_TIMER_TICKS = 1 means elapsed_tenths increments on every ISR tick.
//------------------------------------------------------------------------------
#define DISPLAY_TIMER_TICKS  (1)     // Each 200ms ISR tick = 1 elapsed_tenth (0.2s)

// end of Project 7 additions //////////////////////////////////////////////////

// DAC Motor Power //////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
// DAC Voltage Level Constants (SAC3, DACSREF_0 = VCC/AVCC reference)
//
// CRITICAL: The DAC feeds the FB_DAC pin of an LT1935 Buck-Boost converter.
//           The converter output voltage is INVERSELY proportional to the
//           DAC feedback voltage.
//             LOWER register value  -->  HIGHER output voltage to motors
//             HIGHER register value -->  LOWER output voltage to motors
//
//   Vout_dac = n/4096 * VCC  (VCC = 3.3V on FR2355 LaunchPad)
//   DAC_Begin  (1500) --> ~1.21V DAC out --> lower motor voltage, safe start
//   DAC_Limit  (1200) --> ~0.97V DAC out --> ~6V motor supply (ramp stops)
//   DAC_Adjust (1200) --> ~0.97V DAC out --> ~6V motor supply (operating pt)
//   (Per instructor: "somewhere around 1200 will be about 6V")
//
// Ramp sequence (Timer B0 overflow ISR, ~0.52s per tick):
//   Phase 1: count DAC_ENABLE_TICKS overflows, then set P2OUT |= DAC_ENB
//   Phase 2: decrement DAC_data by DAC_RAMP_STEP each tick until DAC_Limit
//            When done: set DAC_Adjust, disable TBIE, turn RED LED OFF
//------------------------------------------------------------------------------
#define DAC_Begin        (1500)  // Safe startup value (motors at low voltage)
#define DAC_Limit        (1200)  // Ramp stop threshold: ~6V motor supply
#define DAC_Adjust       (1200)  // Final operating point: ~6V motor supply
#define DAC_RAMP_STEP    (50)    // Decrement per overflow tick (per instructor)
#define DAC_ENABLE_TICKS (3)     // Overflow ticks before enabling DAC_ENB (~1.6s)

// end of DAC Motor Power ///////////////////////////////////////////////////////

#endif /* MACROS_H_ */
