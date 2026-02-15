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


// Project 4 additons

// State Machine States///////////////////////////////////////////////////////
#define NONE                 ('N')
#define WAIT                 ('W')
#define START                ('S')
#define RUN                  ('R')
#define END                  ('E')

// Shape Events
#define CIRCLE               ('C')
#define FIGURE8              ('8')
#define TRIANGLE             ('T')

// Timing Constants
#define WAITING2START        (50)   // ~2.5 sec delay before shape starts
#define WHEEL_COUNT_TIME     (20)   // PWM duty cycle period

// CIRCLE: both wheels on, one slower => car curves
// Tune these on your actual car!
#define CIRCLE_RIGHT_COUNT   (1)    // Right motor duty (out of WHEEL_COUNT_TIME)
#define CIRCLE_LEFT_COUNT    (20)   // Left motor duty (outer wheel, full speed)
#define CIRCLE_TRAVEL        (30)  // Segments for one full circle

#define CIRCLE_L             ('D')   // Left-turning circle event ('D' for direction 2)
// Left-turning circle: flip the speeds
#define CIRCLE_L_RIGHT_COUNT (20)    // Right motor full speed (outer wheel)
#define CIRCLE_L_LEFT_COUNT  (1)     // Left motor slow (inner wheel)
#define CIRCLE_L_TRAVEL      (25)    // Same travel as your right circle to start


// FIGURE-8: alternate turning direction
#define FIG8_RIGHT_COUNT_A   (20)    // Phase A: turn right
#define FIG8_LEFT_COUNT_A    (1)
#define FIG8_RIGHT_COUNT_B   (1)   // Phase B: turn left
#define FIG8_LEFT_COUNT_B    (20)
#define FIG8_HALF_TRAVEL     (27)  // Segments per half-loop

// TRIANGLE: straight legs + sharp turns
#define TRI_STRAIGHT_RIGHT   (10)    // Near-equal speed for straight
#define TRI_STRAIGHT_LEFT    (10)
#define TRI_STRAIGHT_TRAVEL  (7)   // Length of one leg

#define TRI_TURN_RIGHT       (1)    // Tight turn: right much slower
#define TRI_TURN_LEFT        (20)
#define TRI_TURN_TRAVEL      (11)   // ~120 degree turn duration

// end of project 4 additions///////////////////////////////////////////////////////

#endif /* MACROS_H_ */
