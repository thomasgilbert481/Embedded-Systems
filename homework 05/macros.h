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

// HW05 additions
#define USE_GPIO                (0x00)
#define USE_SMCLK               (0x01)
#define DEBOUNCE_THRESHOLD      (500)

#define TB0CCR0_INTERVAL_8MHZ   (40000)
#define TB0CCR0_INTERVAL_500KHZ (2500)
#define TB0CCR0_INTERVAL        TB0CCR0_INTERVAL_500KHZ
#define CS_UNLOCK    (0xA5)     // Clock system password — write to CSCTL0_H to unlock
#define CS_LOCK      (0x00)     // Write to CSCTL0_H to lock clock registers

#endif /* MACROS_H_ */
