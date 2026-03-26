//==============================================================================
// File: main.c
// Description: Main routine for Project 8 — Serial Communication via UCA0.
//
//   The vehicle communicates with an Analog Discovery 2 (AD2) over J9 using
//   eUSCI_A0 UART (P1.6 = RXD, P1.7 = TXD).
//
//   Operation:
//     - Vehicle waits in "Waiting   " state; LCD shows current baud rate.
//     - AD2 sends a 10-character command terminated with CR+LF.
//     - IOT_Process() assembles bytes from the ring buffer into received_command.
//     - When command_ready is set, LCD updates: line 1 = "Received  ",
//       line 4 = the received command.
//     - SW1 press: transmit stored command back to AD2 via Serial_Transmit();
//       LCD: line 1 = "Transmit  ", line 2 = command being sent.
//     - SW2 press: cycle baud rate (9,600 ↔ 115,200); reinitialize UCA0;
//       LCD line 3 updates to show new rate.
//
//   LCD layout (after splash):
//     Line 1 (display_line[0]): "Waiting   " / "Received  " / "Transmit  "
//     Line 2 (display_line[1]): Last transmitted command (blank until SW1)
//     Line 3 (display_line[2]): Current baud rate "  9,600   " / " 115,200  "
//     Line 4 (display_line[3]): Last received command (blank until first RX)
//
//   Demo: 6 distinct 10-character commands, 3 at each of 2 baud rates.
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

//==============================================================================
// Function prototype
//==============================================================================
void main(void);

//==============================================================================
// External globals (from LCD.obj and timers.c)
//==============================================================================
extern char                   display_line[4][11];  // LCD line buffers
extern char                  *display[4];            // Pointers into display_line
extern volatile unsigned char display_changed;       // Triggers LCD hardware refresh
extern volatile unsigned char update_display;        // Set every 200 ms by CCR0 ISR
extern volatile unsigned int  Time_Sequence;         // Master timer counter
extern volatile char          one_time;              // One-shot flag

//==============================================================================
// Switch press flags
//   Set by ISRs in interrupt_ports.c; cleared and acted on in the main loop.
//   Using flags keeps ISRs short and moves baud/transmit logic out of ISR context.
//==============================================================================
volatile unsigned int sw1_pressed = RESET_STATE;  // SW1: transmit stored command
volatile unsigned int sw2_pressed = RESET_STATE;  // SW2: cycle baud rate

//==============================================================================
// Command storage
//   received_command is defined in serial.c and assembled by IOT_Process().
//   command_to_send is a local copy kept for retransmission via SW1.
//==============================================================================
char command_to_send[SERIAL_MSG_LENGTH + 1] = "          "; // Copy of last received

//==============================================================================
// Function: main
//==============================================================================
void main(void){

    PM5CTL0 &= ~LOCKLPM5;    // Unlock GPIO (required after POR on FR devices)

    Init_Ports();             // Configure all GPIO pins
    Init_Clocks();            // 8 MHz MCLK and SMCLK via DCO
    Init_Conditions();        // Clear display buffers; enable global interrupts
    Init_Timers();            // Timer B0: 200 ms CCR0 base clock + debounce CCR1/CCR2
    Init_LCD();               // Initialize LCD via SPI (UCB1)

    Init_Serial_UCA0(baud_rate_index);  // Configure UCA0 at starting baud rate (9,600)

    // LCD backlight ON
    P6OUT |= LCD_BACKLITE;

    // Startup display
    strcpy(display_line[LCD_LINE1_SERIAL], "Waiting   ");
    strcpy(display_line[LCD_LINE2_CMD],    "          ");
    strcpy(display_line[LCD_LINE3_BAUD],   "  9,600   "); // Matches BAUD_9600 default
    strcpy(display_line[LCD_LINE4_RX],     "          ");
    display_changed = TRUE;

    //==========================================================================
    // Main loop
    //==========================================================================
    while(ALWAYS){

        //----------------------------------------------------------------------
        // Serial processing: drain the ring buffer one byte per loop pass.
        // Sets command_ready when SERIAL_MSG_LENGTH chars or CR are received.
        //----------------------------------------------------------------------
        IOT_Process();

        //----------------------------------------------------------------------
        // New command received from AD2
        //----------------------------------------------------------------------
        if(command_ready){
            command_ready = 0;   // Clear flag immediately to allow next reception

            // Update state display
            strcpy(display_line[LCD_LINE1_SERIAL], "Received  ");

            // Show received command on line 4 (pad/truncate to exactly 10 chars)
            {
                unsigned int i;
                char temp[11] = "          ";
                for(i = BEGINNING; i < SERIAL_MSG_LENGTH && received_command[i] != '\0'; i++){
                    temp[i] = received_command[i];
                }
                temp[10] = '\0';
                strcpy(display_line[LCD_LINE4_RX], temp);
            }

            // Copy into command_to_send so SW1 can retransmit it later
            strncpy(command_to_send, received_command, SERIAL_MSG_LENGTH);
            command_to_send[SERIAL_MSG_LENGTH] = '\0';

            display_changed = TRUE;
        }

        //----------------------------------------------------------------------
        // SW1 pressed — transmit stored command back to AD2
        //----------------------------------------------------------------------
        if(sw1_pressed){
            sw1_pressed = RESET_STATE;

            // Update state display
            strcpy(display_line[LCD_LINE1_SERIAL], "Transmit  ");

            // Show the command being transmitted on line 2
            {
                unsigned int i;
                char temp[11] = "          ";
                for(i = BEGINNING; i < SERIAL_MSG_LENGTH && command_to_send[i] != '\0'; i++){
                    temp[i] = command_to_send[i];
                }
                temp[10] = '\0';
                strcpy(display_line[LCD_LINE2_CMD], temp);
            }

            display_changed = TRUE;

            // Kick off interrupt-driven transmission of command_to_send
            Serial_Transmit(command_to_send);
        }

        //----------------------------------------------------------------------
        // SW2 pressed — cycle to next baud rate and reinitialize UCA0
        //----------------------------------------------------------------------
        if(sw2_pressed){
            sw2_pressed = RESET_STATE;

            // Advance baud rate index, wrapping back to BAUD_9600 after the last
            baud_rate_index++;
            if(baud_rate_index >= NUM_BAUD_RATES){
                baud_rate_index = BEGINNING;
            }

            // Flush all RX/TX buffer state before changing rates.
            // Any bytes received at the old baud rate are now garbage — discard them.
            Clear_Serial_Buffers();

            // Reinitialize UCA0 with the new rate (briefly puts UCA0 into reset)
            Init_Serial_UCA0(baud_rate_index);

            // Refresh the baud rate display on line 3
            Update_Baud_Display();

            // Return to waiting state and clear the received command display
            strcpy(display_line[LCD_LINE1_SERIAL], "Waiting   ");
            strcpy(display_line[LCD_LINE4_RX],     "          ");
            display_changed = TRUE;
        }

        //----------------------------------------------------------------------
        // Refresh LCD when display content has changed
        //----------------------------------------------------------------------
        Display_Process();

        Switches_Process();   // Stub — switch handling is interrupt-driven

        P3OUT ^= TEST_PROBE;  // Toggle test probe (oscilloscope heartbeat)
    }
}
