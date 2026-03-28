//==============================================================================
// File: main.c
// Description: Main routine for Project 9 Part 1 — IOT Passthrough.
//
//   Transparent serial bridge: Termite (PC) ↔ MSP430 ↔ ESP32 IOT module.
//   Data path:
//     Termite (PC) → UCA1 RX ISR → UCA0 TXBUF → ESP32
//     ESP32        → UCA0 RX ISR → UCA1 TXBUF → Termite (PC)
//
//   No state machine, no IOT_Process drain, no movement commands.
//   Main loop is unchanged from Project 8 except for initialization.
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
extern char                   display_line[4][11];
extern char                  *display[4];
extern volatile unsigned char display_changed;
extern volatile unsigned char update_display;
extern volatile unsigned int  Time_Sequence;
extern volatile char          one_time;

//==============================================================================
// Switch press flags
//==============================================================================
volatile unsigned int sw1_pressed = RESET_STATE;
volatile unsigned int sw2_pressed = RESET_STATE;

//==============================================================================
// Command storage
//==============================================================================
char command_to_send[SERIAL_MSG_LENGTH + 1] = "          ";

//==============================================================================
// Function: main
//==============================================================================
void main(void){

    PM5CTL0 &= ~LOCKLPM5;    // Unlock GPIO (required after POR on FR devices)

    Init_Ports();             // Configure all GPIO pins (includes IOT control pins)
    Init_Clocks();            // 8 MHz MCLK and SMCLK via DCO
    Init_Conditions();        // Clear display buffers; enable global interrupts
    Init_Timers();            // Timer B0: 200 ms CCR0 base clock + debounce CCR1/CCR2
    Init_LCD();               // Initialize LCD via SPI (UCB1)

    // 1. Init UCA1 first (PC backchannel) — this resets pc_ok_to_tx to FALSE
    Init_Serial_UCA1(BAUD_115200);

    // 2. Init UCA0 (IOT port) — P1.6/P1.7 must already be configured in Init_Ports()
    Init_Serial_UCA0(BAUD_115200);

    // 3. Release IOT from reset — must happen AFTER both serial ports are initialized
    {
      volatile unsigned long dly;
      IOT_EN_PORT &= ~IOT_EN_PIN;           // Ensure IOT_EN is LOW first
      for(dly = IOT_RESET_DELAY; dly > 0; dly--);  // Hold LOW >= 100ms
      IOT_EN_PORT |=  IOT_EN_PIN;           // Release reset — ESP32 boots
    }

    // LCD backlight ON
    P6OUT |= LCD_BACKLITE;

    // Startup display
    strcpy(display_line[LCD_LINE1_SERIAL], "IOT Pass  ");
    strcpy(display_line[LCD_LINE2_CMD],    "          ");
    strcpy(display_line[LCD_LINE3_BAUD],   " 115,200  ");
    strcpy(display_line[LCD_LINE4_RX],     "          ");
    display_changed = TRUE;

    //==========================================================================
    // Main loop
    //==========================================================================
    while(ALWAYS){

        //----------------------------------------------------------------------
        // Serial processing: drain the ring buffer one byte per loop pass.
        //----------------------------------------------------------------------
        IOT_Process();

        //----------------------------------------------------------------------
        // New command received
        //----------------------------------------------------------------------
        if(command_ready){
            command_ready = 0;

            strcpy(display_line[LCD_LINE1_SERIAL], "Received  ");

            {
                unsigned int i;
                char temp[11] = "          ";
                for(i = BEGINNING; i < SERIAL_MSG_LENGTH && received_command[i] != '\0'; i++){
                    temp[i] = received_command[i];
                }
                temp[10] = '\0';
                strcpy(display_line[LCD_LINE4_RX], temp);
            }

            strncpy(command_to_send, received_command, SERIAL_MSG_LENGTH);
            command_to_send[SERIAL_MSG_LENGTH] = '\0';

            display_changed = TRUE;
        }

        //----------------------------------------------------------------------
        // SW1 pressed — transmit stored command
        //----------------------------------------------------------------------
        if(sw1_pressed){
            sw1_pressed = RESET_STATE;

            strcpy(display_line[LCD_LINE1_SERIAL], "Transmit  ");

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

            Serial_Transmit(command_to_send);
        }

        //----------------------------------------------------------------------
        // SW2 pressed — cycle baud rate (kept from Project 8)
        //----------------------------------------------------------------------
        if(sw2_pressed){
            sw2_pressed = RESET_STATE;

            if(baud_rate_index == BAUD_115200){
                baud_rate_index = BAUD_9600;
            } else {
                baud_rate_index = BAUD_115200;
            }

            Clear_Serial_Buffers();

            Init_Serial_UCA0(baud_rate_index);

            Update_Baud_Display();

            strcpy(display_line[LCD_LINE1_SERIAL], "IOT Pass  ");
            strcpy(display_line[LCD_LINE4_RX],     "          ");
            display_changed = TRUE;
        }

        //----------------------------------------------------------------------
        // Refresh LCD when display content has changed
        //----------------------------------------------------------------------
        Display_Process();

        Switches_Process();

        P3OUT ^= TEST_PROBE;  // Toggle test probe (oscilloscope heartbeat)
    }
}
