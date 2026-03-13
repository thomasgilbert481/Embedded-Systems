//==============================================================================
// File Name: functions.h
// Description: Global function prototypes for Project 7 -- Circle Following.
// Author: Thomas Gilbert
// Date: Mar 2026
// Compiler: Code Composer Studio
//==============================================================================

#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

//------------------------------------------------------------------------------
// Initialization functions
//------------------------------------------------------------------------------
void Init_Ports(void);
void Init_Port1(void);
void Init_Port2(void);
void Init_Port3(void);
void Init_Port4(void);
void Init_Port5(void);
void Init_Port6(void);

void Init_Clocks(void);
void Init_Conditions(void);
void enable_interrupts(void);

void Init_Timers(void);
void Init_Timer_B0(void);
void Init_Timer_B3(void);        // Hardware PWM for motor control (Project 7)

void Init_LEDs(void);
void Init_LCD(void);
void Init_ADC(void);

//------------------------------------------------------------------------------
// LCD / Display functions
//------------------------------------------------------------------------------
void Display_Process(void);
void Display_Update(void);

//------------------------------------------------------------------------------
// ADC / Conversion functions
//------------------------------------------------------------------------------
void HexToBCD(int hex_value);

//------------------------------------------------------------------------------
// Motor control functions (hardware PWM via Timer B3)
//------------------------------------------------------------------------------
void Wheels_All_Off(void);
void Forward_On(void);
void Forward_Off(void);
void Reverse_On(void);
void Reverse_Off(void);
void Spin_CW_On(void);
void Spin_CCW_On(void);

//------------------------------------------------------------------------------
// Switch polling (stub -- switch handling is interrupt-driven)
//------------------------------------------------------------------------------
void Switches_Process(void);

//------------------------------------------------------------------------------
// Project 7 state machine and display
//------------------------------------------------------------------------------
void Run_Project7(void);
void Update_P7_Display(void);

#endif /* FUNCTIONS_H_ */
