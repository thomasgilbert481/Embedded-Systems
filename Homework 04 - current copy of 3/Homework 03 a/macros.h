//==============================================================================
// File Name: macros.h
// Description: Contains all #define macros for the project
// Author: Thomas Gilbert
// Date: 2/4/2026
// Compiler: Code Composer Studio [version]
//==============================================================================

#ifndef MACROS_H_
#define MACROS_H_

#define ALWAYS                  (1)
#define RESET_STATE             (0)
#define RED_LED              (0x01) // RED LED 0
#define GRN_LED              (0x40) // GREEN LED 1
#define TEST_PROBE           (0x01) // 0 TEST PROBE
#define TRUE                 (0x01) //

// State Machine States
#define NONE                 ('N')
#define WAIT                 ('W')
#define START                ('S')
#define RUN                  ('R')
#define END                  ('E')

//PROJECT 4 ADDITIONS////////////////////////////////////////////////////////////////

// Shape Events
#define CIRCLE               ('C')
#define FIGURE8              ('8')
#define TRIANGLE             ('T')

// Timing Constants
#define WAITING2START        (50)   // ~2.5 sec delay before shape starts
#define WHEEL_COUNT_TIME     (10)   // PWM duty cycle period

// CIRCLE: both wheels on, one slower => car curves
// Tune these on your actual car!
#define CIRCLE_RIGHT_COUNT   (7)    // Right motor duty (out of WHEEL_COUNT_TIME)
#define CIRCLE_LEFT_COUNT    (10)   // Left motor duty (outer wheel, full speed)
#define CIRCLE_TRAVEL        (100)  // Segments for one full circle

// FIGURE-8: alternate turning direction
#define FIG8_RIGHT_COUNT_A   (7)    // Phase A: turn right
#define FIG8_LEFT_COUNT_A    (10)
#define FIG8_RIGHT_COUNT_B   (10)   // Phase B: turn left
#define FIG8_LEFT_COUNT_B    (7)
#define FIG8_HALF_TRAVEL     (100)  // Segments per half-loop

// TRIANGLE: straight legs + sharp turns
#define TRI_STRAIGHT_RIGHT   (9)    // Near-equal speed for straight
#define TRI_STRAIGHT_LEFT    (10)
#define TRI_STRAIGHT_TRAVEL  (40)   // Length of one leg

#define TRI_TURN_RIGHT       (3)    // Tight turn: right much slower
#define TRI_TURN_LEFT        (10)
#define TRI_TURN_TRAVEL      (25)   // ~120 degree turn duration

//end of project 4 additions////////////////////////////////////////////////////////////////
#endif /* MACROS_H_ */

