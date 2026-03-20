//==============================================================================
// File: main.c
// Description: Main routine for Homework 8 -- UART Serial Communications.
//
//   Configures two UART ports on the MSP430FR2355:
//     eUSCI_A0 (UCA0) -- IOT module connector J9 (P1.6=RXD, P1.7=TXD)
//     eUSCI_A1 (UCA1) -- PC backchannel USB J14  (P4.2=RXD, P4.3=TXD)
//
//   Demo configuration (Configuration 3):
//     J14 in pass-thru mode  -- connects MSP430 UCA1 to PC (Termite)
//     J9 loopback jumper     -- connects UCA0 TX back to UCA0 RX
//
//   Echo path:
//     Termite -> UCA1 RX -> UCA0 TX -> J9 loopback -> UCA0 RX -> UCA1 TX -> Termite
//
//   Controls:
//     SW1 -- switch to 115,200 baud; 2s later transmit "NCSU  #1" out UCA1
//     SW2 -- switch to 460,800 baud; 2s later transmit "NCSU  #1" out UCA1
//
//   LCD display (after 5-second splash):
//     Line 1: Characters received from J9 loopback (updated each 200ms tick)
//     Line 2: (blank)
//     Line 3: "   Baud   "
//     Line 4: " 115,200  " or " 460,800  "
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

//==============================================================================
// Function Prototype
//==============================================================================
void main(void);

//==============================================================================
// Global Variables
//==============================================================================
extern char display_line[4][11];            // LCD display buffer (from LCD.obj)
extern char *display[4];                    // Pointers to display lines
extern volatile unsigned char display_changed;  // Flag: LCD content updated
extern volatile unsigned char update_display;   // Flag: timer says refresh LCD
extern volatile unsigned int  Time_Sequence;    // Master timer counter
extern volatile char          one_time;         // One-time flag

// From serial.c -- read by main loop to update LCD line 1
extern volatile char         received_string[11]; // Characters received from UCA0
extern volatile unsigned int received_index;       // Number of received characters

// Switch press flags -- set by ISRs in interrupt_ports.c, cleared here
volatile unsigned int sw1_pressed = RESET_STATE;  // SW1 sets to 115,200 baud
volatile unsigned int sw2_pressed = RESET_STATE;  // SW2 sets to 460,800 baud

// Current active baud rate
unsigned int current_baud = BAUD_115200;

// Transmit timing: 2-second wait after baud change before sending "NCSU  #1"
volatile unsigned int transmit_wait    = RESET_STATE; // Counts 200ms ticks
volatile unsigned int transmit_pending = RESET_STATE; // TRUE while waiting

// Splash screen timing (5 seconds = 25 ticks at 200ms per tick)
volatile unsigned int splash_timer = RESET_STATE;     // Counts 200ms ticks
unsigned int          splash_done  = FALSE;            // FALSE until splash expires

// String to transmit after baud rate change (2 spaces between U and # per spec)
char NCSU_str[] = "NCSU  #1";

//==============================================================================
// Function: main
//==============================================================================
void main(void){

    PM5CTL0 &= ~LOCKLPM5;   // Unlock GPIO (required after POR on FR devices)

    Init_Ports();            // Configure all GPIO (UCA0/UCA1 pins for UART)
    Init_Clocks();           // 8 MHz MCLK and SMCLK
    Init_Conditions();       // Clear display buffers, enable global interrupts
    Init_Timers();           // Timer B0: 200ms CCR0 base clock
    Init_LCD();              // Initialize LCD via SPI

    Init_Serial_UCA0();      // Configure eUSCI_A0 (IOT port, J9)
    Init_Serial_UCA1();      // Configure eUSCI_A1 (PC backchannel, J14)

    // LCD backlight ON
    P6OUT |= LCD_BACKLITE;

    // Splash screen (displayed for SPLASH_TIME x 200ms = 5 seconds)
    strcpy(display_line[0], "  ECE 306 ");
    strcpy(display_line[1], " HW8 UART ");
    strcpy(display_line[2], " T.Gilbert");
    strcpy(display_line[3], " 115,200  ");
    display_changed = TRUE;

    //==========================================================================
    // Main loop
    //==========================================================================
    while(ALWAYS){

        //----------------------------------------------------------------------
        // Splash screen: after 5 seconds switch to baud rate display
        //----------------------------------------------------------------------
        if(!splash_done && splash_timer >= SPLASH_TIME){
            splash_done = TRUE;
            strcpy(display_line[0], "          ");
            strcpy(display_line[1], "          ");
            strcpy(display_line[2], "   Baud   ");
            strcpy(display_line[3], " 115,200  ");
            display_changed = TRUE;
        }

        //----------------------------------------------------------------------
        // SW1 pressed -- switch to 115,200 baud
        //----------------------------------------------------------------------
        if(sw1_pressed){
            sw1_pressed = RESET_STATE;

            current_baud = BAUD_115200;
            Set_Baud_Rate(BAUD_115200);

            strcpy(display_line[0], "          ");
            strcpy(display_line[1], "          ");
            strcpy(display_line[2], "   Baud   ");
            strcpy(display_line[3], " 115,200  ");
            display_changed = TRUE;

            // Clear received buffer
            received_index = RESET_STATE;
            memset((char*)received_string, ' ', 10);
            received_string[10] = '\0';

            // Start 2-second wait before transmitting
            transmit_wait    = RESET_STATE;
            transmit_pending = TRUE;
        }

        //----------------------------------------------------------------------
        // SW2 pressed -- switch to 460,800 baud
        //----------------------------------------------------------------------
        if(sw2_pressed){
            sw2_pressed = RESET_STATE;

            current_baud = BAUD_460800;
            Set_Baud_Rate(BAUD_460800);

            strcpy(display_line[0], "          ");
            strcpy(display_line[1], "          ");
            strcpy(display_line[2], "   Baud   ");
            strcpy(display_line[3], " 460,800  ");
            display_changed = TRUE;

            // Clear received buffer
            received_index = RESET_STATE;
            memset((char*)received_string, ' ', 10);
            received_string[10] = '\0';

            // Start 2-second wait before transmitting
            transmit_wait    = RESET_STATE;
            transmit_pending = TRUE;
        }

        //----------------------------------------------------------------------
        // Transmit "NCSU  #1" after 2-second delay following baud rate change
        //----------------------------------------------------------------------
        if(transmit_pending && transmit_wait >= TWO_SECOND_COUNT){
            transmit_pending = RESET_STATE;
            transmit_wait    = RESET_STATE;
            Transmit_String_UCA1(NCSU_str);
        }

        //----------------------------------------------------------------------
        // Update LCD line 1 with received characters (every 200ms display tick)
        //----------------------------------------------------------------------
        if(update_display && splash_done){
            if(received_index > RESET_STATE){
                unsigned int i;
                char temp[11] = "          ";
                for(i = RESET_STATE; i < received_index && i < 10; i++){
                    temp[i] = received_string[i];
                }
                temp[10] = '\0';
                strcpy(display_line[0], temp);
                display_changed = TRUE;
            }
        }

        Display_Process();       // Push display_line[] to LCD when display_changed

        Switches_Process();      // Stub -- switch handling is interrupt-driven

        P3OUT ^= TEST_PROBE;     // Toggle test probe (oscilloscope heartbeat)
    }
}
