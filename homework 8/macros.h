//==============================================================================
// File Name: macros.h
// Description: Contains all #define macros for Homework 8
// Author: Thomas Gilbert
// Date: Mar 2026
// Compiler: Code Composer Studio
// Target: MSP430FR2355
//==============================================================================

#ifndef MACROS_H_
#define MACROS_H_

#define ALWAYS                  (1)
#define RESET_STATE             (0)
#define RED_LED              (0x01) // RED LED P1.0
#define GRN_LED              (0x40) // GREEN LED P6.6
#define TEST_PROBE           (0x01) // P3.0 TEST PROBE
#define TRUE                 (0x01)
#define FALSE                (0x00)

//------------------------------------------------------------------------------
// Timer B0 CCR0 interval -- Continuous mode, ID__8, TBIDEX__8 = 125kHz
// 8,000,000 / 8 / 8 / (1 / 0.200) = 25,000 counts = 200ms per interrupt
// 5 interrupts = 1 second
//------------------------------------------------------------------------------
#define TB0CCR0_INTERVAL     (25000)  // 200ms: 8MHz/8/8/(1/0.2) = 25,000

// Timer B0 CCR1/CCR2 intervals for interrupt-driven switch debounce
#define TB0CCR1_INTERVAL     (25000)  // 200ms base for SW1 debounce
#define TB0CCR2_INTERVAL     (25000)  // 200ms base for SW2 debounce

// Debounce threshold: 5 x 200ms = 1.0 second total debounce period
#define DEBOUNCE_THRESHOLD   (5)

// Time_Sequence wrap value (250 x 200ms = 50 seconds)
#define TIME_SEQ_MAX         (250)

// Timing constants (200ms per ISR tick)
#define ONE_SEC              (5)     // 5 x 200ms = 1.0 second
#define TWO_SEC              (10)    // 10 x 200ms = 2.0 seconds
#define THREE_SEC            (15)    // 15 x 200ms = 3.0 seconds

//------------------------------------------------------------------------------
// Homework 8 Serial Communication Defines
//------------------------------------------------------------------------------
#define BEGINNING            (0)
#define SMALL_RING_SIZE      (16)    // Ring buffer size

// Baud rate selections
#define BAUD_115200          (0)
#define BAUD_460800          (1)

// Display line length (10 chars + null)
#define DISPLAY_LINE_LEN     (11)

// Timing: 2 seconds after baud change before transmitting "NCSU  #1"
// Timer B0 CCR0 fires every 200ms => 10 ticks = 2 seconds
#define TWO_SECOND_COUNT     (10)

// Splash screen duration: 5 seconds = 25 ticks at 200ms per tick
#define SPLASH_TIME          (25)

#endif /* MACROS_H_ */
