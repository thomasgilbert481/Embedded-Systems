//==============================================================================
// File Name: macros.h
// Description: All #define constants for Project 9 Part 1 -- IOT Passthrough
// Author: Thomas Gilbert
// Date: Mar 2026
// Compiler: Code Composer Studio
// Target: MSP430FR2355
//==============================================================================

#ifndef MACROS_H_
#define MACROS_H_

//------------------------------------------------------------------------------
// General
//------------------------------------------------------------------------------
#define ALWAYS              (1)
#define RESET_STATE         (0)
#define TRUE                (0x01)
#define FALSE               (0x00)
#define BEGINNING           (0)
#define RED_LED             (0x01)   // P1.0
#define GRN_LED             (0x40)   // P6.6
#define TEST_PROBE          (0x01)   // P3.0

//------------------------------------------------------------------------------
// Timer B0 -- continuous mode, SMCLK/8/8 = 125 kHz
//   25,000 counts = 200 ms per CCR0 interrupt
//   5 interrupts = 1 second
//------------------------------------------------------------------------------
#define TB0CCR0_INTERVAL    (25000)  // 200 ms
#define TB0CCR1_INTERVAL    (25000)  // SW1 debounce base period
#define TB0CCR2_INTERVAL    (25000)  // SW2 debounce base period

// Number of 200-ms CCR1/CCR2 ticks before re-enabling a switch interrupt
// 5 x 200 ms = 1.0 second total debounce window
#define DEBOUNCE_THRESHOLD  (5)

// Time_Sequence wrap value (250 x 200 ms = 50 seconds)
#define TIME_SEQ_MAX        (250)

// Timing aliases
#define ONE_SEC             (5)      // 5 x 200 ms = 1 s
#define TWO_SEC             (10)     // 10 x 200 ms = 2 s

//------------------------------------------------------------------------------
// Serial / UART Constants
//------------------------------------------------------------------------------
// All commands sent from the AD2 are exactly 10 printable characters
// (followed by CR+LF which is stripped by IOT_Process)
#define SERIAL_MSG_LENGTH   (10)

//--------------------------------------------------------------
// Baud rate selector constants for Init_Serial_UCA0 / UCA1
//--------------------------------------------------------------
#define BAUD_115200  (0)
#define BAUD_9600    (1)

//--------------------------------------------------------------
// IOT Ring Buffer size
//--------------------------------------------------------------
#define IOT_RING_SIZE   (128)
#define USB_RING_SIZE   (128)

//------------------------------------------------------------------------------
// UCA0 register values for each baud rate (BRCLK = SMCLK = 8 MHz)
//
// BRCLK  Baud    UCOS16  UCBRx  UCFx  UCSx   BRW   MCTLW
// 8 MHz   9600     1      52     1    0x49    52    0x4911
// 8 MHz  115200    1       4     5    0x55     4    0x5551
//------------------------------------------------------------------------------
#define BAUD_9600_BRW       (52)
#define BAUD_9600_MCTLW     (0x4911)
#define BAUD_115200_BRW     (4)
#define BAUD_115200_MCTLW   (0x5551)

//------------------------------------------------------------------------------
// Special characters used in serial framing
//------------------------------------------------------------------------------
#define SERIAL_CR           (0x0D)   // Carriage return
#define SERIAL_LF           (0x0A)   // Line feed
#define SERIAL_NULL         (0x00)   // Null terminator

//--------------------------------------------------------------
// IOT Control Pin Defines
//--------------------------------------------------------------
// P3.7 = IOT_EN  -- active-low reset line
#define IOT_EN_DIR      P3DIR
#define IOT_EN_PORT     P3OUT
#define IOT_EN_PIN      BIT7

// P5.4 = IOT_BOOT -- must stay HIGH always (never program mode)
#define IOT_BOOT_DIR    P5DIR
#define IOT_BOOT_PORT   P5OUT
#define IOT_BOOT_PIN    BIT4

// P2.4 = IOT_RUN LED
#define IOT_RUN_DIR     P2DIR
#define IOT_RUN_PORT    P2OUT
#define IOT_RUN_PIN     BIT4

// P3.6 = IOT_LINK LED
#define IOT_LINK_DIR    P3DIR
#define IOT_LINK_PORT   P3OUT
#define IOT_LINK_PIN    BIT6

//--------------------------------------------------------------
// IOT reset hold time (loop count ~ 100ms at 8 MHz)
//--------------------------------------------------------------
#define IOT_RESET_DELAY (200000u)

//------------------------------------------------------------------------------
// LCD line index assignments (0-indexed to match display_line[4][11] array)
//------------------------------------------------------------------------------
#define LCD_LINE1_SERIAL    (0)   // display_line[0]: operational state
#define LCD_LINE2_CMD       (1)   // display_line[1]: last transmitted command
#define LCD_LINE3_BAUD      (2)   // display_line[2]: current baud rate
#define LCD_LINE4_RX        (3)   // display_line[3]: last received command

#endif /* MACROS_H_ */
