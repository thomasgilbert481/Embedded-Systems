//==============================================================================
// File: main.c
// Description: Main routine for Project 9 Part 2 -- IOT TCP Server / Vehicle.
//
//   On boot, after IOT_EN releases, the IOT state machine drives the ESP32
//   through:  AT  ->  WIFI GOT IP  ->  CIPMUX=1  ->  CIPSERVER=1,<port>
//             ->  CIFSR  ->  parse + display IP on LCD  ->  RUNNING.
//
//   While RUNNING, every "+IPD,<conn>,<len>:^<PIN><dir><time>\r\n" packet
//   from the TCP client is decoded and executes a timed motor command
//   (auto-stops via Vehicle_Cmd_Tick in the 200 ms Timer B0 ISR).
//
//   The PC backchannel (UCA1) still runs the FRAM '^' command set
//   (^^ ping, ^F/^S baud) and forwards everything else as passthrough,
//   so Termite can still observe and inject AT commands.
//
// Author: Thomas Gilbert
// Date: Mar 2026
// Compiler: Code Composer Studio
// Target: MSP430FR2355
//==============================================================================

#include "msp430.h"
#include <string.h>
#include "functions.h"
#include "LCD.h"
#include "ports.h"
#include "macros.h"
#include "serial.h"
#include "iot.h"

void main(void);

//==============================================================================
// External globals (LCD.obj, timers.c)
//==============================================================================
extern char                   display_line[4][11];
extern char                  *display[4];
extern volatile unsigned char display_changed;
extern volatile unsigned char update_display;
extern volatile unsigned int  Time_Sequence;
extern volatile char          one_time;

//==============================================================================
// Switch press flags (set by port ISRs in interrupts_ports.c)
//==============================================================================
volatile unsigned int sw1_pressed = RESET_STATE;
volatile unsigned int sw2_pressed = RESET_STATE;

//==============================================================================
void main(void){

    PM5CTL0 &= ~LOCKLPM5;     // Unlock GPIO

    Init_Ports();              // GPIO + IOT control pins (IOT_EN low, IOT_BOOT high)
    Init_Clocks();             // 8 MHz MCLK / SMCLK
    Init_Conditions();         // Clear display buffers + global IE
    Init_Timers();             // Timer B0 (200 ms) + Timer B3 (motor PWM)
    Init_LCD();                // SPI LCD init

    // 1. UCA1 (PC backchannel) -- resets pc_ok_to_tx to FALSE
    Init_Serial_UCA1(BAUD_115200);

    // 2. UCA0 (IOT port)
    Init_Serial_UCA0(BAUD_115200);

    // 3. Release IOT from reset (>= 100 ms low) -- AFTER both UARTs are up
    {
        volatile unsigned long dly;
        IOT_EN_PORT &= ~IOT_EN_PIN;
        for(dly = IOT_RESET_DELAY; dly > 0; dly--);
        IOT_EN_PORT |=  IOT_EN_PIN;
    }

    // LCD backlight ON
    P6OUT |= LCD_BACKLITE;

    // Splash until the state machine populates the network info
    strcpy(display_line[LCD_LINE1_SSID],  "  ECE 306 ");
    strcpy(display_line[LCD_LINE2_LABEL], "  P9 Pt2  ");
    strcpy(display_line[LCD_LINE3_IP_HI], "Connecting");
    strcpy(display_line[LCD_LINE4_IP_LO], "to NCSU...");
    display_changed = TRUE;

    //==========================================================================
    // Main loop
    //==========================================================================
    while(ALWAYS){

        IOT_Process();          // Drain UCA0 RX ring into IOT_Data[][]
        IOT_State_Machine();    // Drive AT command sequence / parse +IPD

        Display_Process();      // Refresh LCD if display_changed
        Switches_Process();     // (no-op stub from Project 8)

        P3OUT ^= TEST_PROBE;    // Heartbeat
    }
}
