//==============================================================================
// File Name: macros.h
// Description: All #define constants for Project 9 Part 2 -- IOT TCP Server
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
#define SERIAL_CARET        (0x5E)   // '^' -- FRAM-only command prefix

//------------------------------------------------------------------------------
// FRAM command characters (used after the '^' prefix in UCA1 RX ISR)
//------------------------------------------------------------------------------
#define FRAM_CMD_PING       ('^')    // ^^  -> FRAM responds "I'm here"
#define FRAM_CMD_FAST       ('F')    // ^F  -> switch UCA0 to 115,200 baud
#define FRAM_CMD_SLOW       ('S')    // ^S  -> switch UCA0 to 9,600 baud

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
#define LCD_LINE1_SSID      (0)   // display_line[0]: SSID (e.g. "ncsu")
#define LCD_LINE2_LABEL     (1)   // display_line[1]: "IP address"
#define LCD_LINE3_IP_HI     (2)   // display_line[2]: first two IP octets
#define LCD_LINE4_IP_LO     (3)   // display_line[3]: last two IP octets

//------------------------------------------------------------------------------
// IOT response parse buffer -- multi-line scratch space populated by IOT_Process
// Each row holds one CR/LF-terminated line from the ESP32 (null-terminated).
//------------------------------------------------------------------------------
#define IOT_DATA_LINES      (6)
#define IOT_DATA_COLS       (96)

//------------------------------------------------------------------------------
// IOT AT-command transmit buffer -- staged by Serial_Transmit, drained by TX ISR.
// Sized large enough for "AT+CIPSERVER=1,65535\r\n" + null.
//------------------------------------------------------------------------------
#define IOT_TX_BUF_SIZE     (40)

//------------------------------------------------------------------------------
// Vehicle command protocol  ^<PIN><dir><time>
//   PIN     : 4 ASCII digits
//   dir     : 'F' | 'B' | 'R' | 'L'
//   time    : 4 ASCII digits, units of CMD_TIME_UNIT_MS milliseconds
//   Example : "^1234F0010" -> Forward for 10 * 100ms = 1.0 seconds
//------------------------------------------------------------------------------
#define CMD_PIN_0           ('1')
#define CMD_PIN_1           ('2')
#define CMD_PIN_2           ('3')
#define CMD_PIN_3           ('4')
#define CMD_PIN_LEN         (4)
#define CMD_DIR_OFFSET      (5)        // payload[5] (after '^' + 4 PIN digits)
#define CMD_TIME_OFFSET     (6)        // payload[6..9]
#define CMD_TIME_DIGITS     (4)
#define CMD_DIR_FORWARD     ('F')
#define CMD_DIR_BACKWARD    ('B')
#define CMD_DIR_RIGHT       ('R')
#define CMD_DIR_LEFT        ('L')
#define CMD_TIME_UNIT_MS    (100)      // each time-unit digit = 100 ms
#define CMD_PAYLOAD_LEN     (10)       // ^ + 4 PIN + 1 dir + 4 time

//------------------------------------------------------------------------------
// Motor command countdown -- decrement step in CCR0 ISR (every 200 ms)
// Each Timer B0 CCR0 tick = 200 ms = TB0_TICK_MS
//------------------------------------------------------------------------------
#define TB0_TICK_MS         (200)

//------------------------------------------------------------------------------
// Timer B3 -- hardware PWM for motors (SMCLK = 8 MHz, no dividers)
//   WHEEL_PERIOD_VAL = 50005 cycles -> ~6.25 ms period -> ~160 Hz PWM
//   FOLLOW_SPEED      drive duty   ~ 50%
//   SPIN_SPEED        spin duty    ~ 50%
//------------------------------------------------------------------------------
#define WHEEL_PERIOD_VAL    (50005)
#define FOLLOW_SPEED        (25000)
#define SPIN_SPEED          (25000)

//------------------------------------------------------------------------------
// IOT state-machine timeouts (counted in main-loop iterations -- coarse)
//------------------------------------------------------------------------------
#define IOT_TIMEOUT_READY   (40000u)   // ~ESP32 boot ready
#define IOT_TIMEOUT_AT      (20000u)   // AT/OK round-trip
#define IOT_TIMEOUT_WIFI    (60000u)   // Wi-Fi join + DHCP
#define IOT_TIMEOUT_GENERIC (15000u)   // Other AT commands

//------------------------------------------------------------------------------
// Default TCP port -- TA recommends 8181 (less likely blocked on NCSU WiFi)
//------------------------------------------------------------------------------
#define IOT_TCP_PORT        (8181)

#endif /* MACROS_H_ */
