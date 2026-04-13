//==============================================================================
// File:        iot.h
// Description: AT command state machine + +IPD command parser for the
//              ESP32 IOT module (Project 9 Part 2).
//
// Author: Thomas Gilbert
// Date: Mar 2026
// Compiler: Code Composer Studio
// Target: MSP430FR2355
//==============================================================================

#ifndef IOT_H_
#define IOT_H_

//==============================================================================
// Network info populated by the state machine after AT+CIFSR succeeds
//==============================================================================
extern char car_ssid[12];   // 10 chars + NUL + pad (LCD line is 10 chars)
extern char car_ip[20];     // e.g. "10.152.15.74" + NUL

//==============================================================================
// Active vehicle command state (read by main / display, written by Vehicle_Cmd_Tick)
//==============================================================================
extern volatile unsigned int  cmd_remaining_ms;   // 0 = no command active
extern volatile char          cmd_active_dir;     // 'F'/'B'/'R'/'L' or NUL
extern volatile unsigned int  cmd_active_time;    // original time-units value

//==============================================================================
// Function prototypes
//==============================================================================
void IOT_State_Machine(void);
void Parse_IPD_Command(char *line);
void Display_Network_Info(void);
void Vehicle_Cmd_Tick(void);       // Timer B0 CCR0 ISR every 200 ms
void Process_Vehicle_Queue(void);  // Main loop -- starts next queued cmd

#endif /* IOT_H_ */
