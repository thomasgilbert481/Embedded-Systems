//==============================================================================
// File: modes.h  -- Calibration + line-follow helpers for Project 9 Part 2
//==============================================================================

#ifndef MODES_H_
#define MODES_H_

//------------------------------------------------------------------------------
// Calibration results (populated by Calibration_Run, consumed by Line_Follow_Run)
//------------------------------------------------------------------------------
extern unsigned int white_left;
extern unsigned int white_right;
extern unsigned int black_left;
extern unsigned int black_right;
extern unsigned int threshold_left;
extern unsigned int threshold_right;
extern unsigned int calibration_done;   // 1 once white+black both captured

//------------------------------------------------------------------------------
// Mode flags
//   mode_cal_active     -- calibration state machine running
//   mode_line_active    -- line-follow in progress (driven by cmd_remaining_ms)
//------------------------------------------------------------------------------
extern volatile unsigned char mode_cal_active;
extern volatile unsigned char mode_line_active;

//------------------------------------------------------------------------------
// Called from +IPD parser when a Q / C / N command arrives
//------------------------------------------------------------------------------
void Quit_Everything(void);      // Q: abort cmd/queue/cal/line
void Calibration_Start(void);    // C: kick off calibration state machine
void Line_Follow_Start(unsigned int seconds);  // N: begin line follow
void Line_Follow_Begin_Exit(void);             // G during follow: exit sequence

//------------------------------------------------------------------------------
// Called from main loop every iteration
//------------------------------------------------------------------------------
void Calibration_Tick(void);     // Advance calibration state machine
void Line_Follow_Tick(void);     // Update motor PWM from ADC + thresholds

#endif /* MODES_H_ */
