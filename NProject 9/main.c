//------------------------------------------------------------------------------
//
//  Description: This file contains the Main Routine - "While" Operating System
//               Homework 9 - Scrolling Menus
//
//  Noah Cartwright
//  March 31, 2026
//  Built with Code Composer Version: CCS12.4.0.00007_win64
//
//  Startup behaviour:
//    The LCD is placed in lcd_BIG_mid() mode and displays:
//      Line 1 (small top)  : "   Noah   "
//      BIG centre line     : "Homework 9"
//      Line 3 (small bot)  : "Cartwright"
//    This splash remains until either push-button is pressed, at which
//    point Menu_Process() transitions to the 3-item main menu.
//
//  Main loop:
//    Menu_Process()    - all menu logic (menus.c)
//    Switches_Process()- debounce / flag handling (switches.c)
//    Display_Process() - LCD refresh when flagged
//    TEST_PROBE toggle - P3.0 timing marker
//
//------------------------------------------------------------------------------

#include  "msp430.h"
#include  <string.h>
#include  "functions.h"
#include  "LCD.h"
#include  "ports.h"
#include  "macros.h"
#include  "globals.h"

// Function prototypes (this file)
void main(void);
void Init_Conditions(void);
void Display_Process(void);
void Init_LEDs(void);

// ─── Globals defined here — declared extern in globals.h ─────────────────────
extern char display_line[4][11];
extern char *display[4];

extern volatile unsigned char display_changed;
extern volatile unsigned char update_display;

extern volatile unsigned int  Time_Sequence;

volatile unsigned int cycle_time        = 0;
volatile unsigned int Last_Time_Sequence = 0;
volatile unsigned int time_change       = 0;
volatile unsigned int delay_start       = 0;
volatile unsigned int left_motor_count  = 0;
volatile unsigned int right_motor_count = 0;
volatile unsigned int segment_count     = 0;

volatile char         state             = IDLE;

//------------------------------------------------------------------------------
// Function: main
// Author:   Noah Cartwright
// Date:     March 31, 2026
// Description:
//   Hardware initialisation followed by the background / foreground
//   "while" operating system loop.  The splash screen is shown in
//   lcd_BIG_mid() mode until the user presses a button; after that,
//   Menu_Process() drives the full menu state machine.
// Parameters:  None
// Returns:     None (does not return)
// Globals Modified: display_line[][], display_changed, update_display
//------------------------------------------------------------------------------
void main(void)
{
    PM5CTL0 &= ~LOCKLPM5;           // Unlock GPIO (required on FR-series)

    Init_Ports();                   // Port directions, pull-ups, module sel.
    Init_Clocks();                  // MCLK = 8 MHz, SMCLK = 8 MHz, ACLK = 32.768 kHz
    Init_Conditions();              // Zero buffers, enable global interrupts
    Init_Timer_B0();                // 50 ms tick, switch debounce
    Init_Timer_B3();                // PWM for motors and LCD backlight
    Init_LCD();                     // SPI + LCD controller init (defaults 4-line)
    Init_ADC();                     // 12-bit ADC: thumb (A5), IR sensors (A2/A3)
    Init_DAC();                     // SAC3 DAC for LED fade (TB0 overflow ISR)

    Init_Serial_UCA0(BAUD_115200);  // IOT module UART (J9) — 115,200 baud
    Init_Serial_UCA1(BAUD_115200);  // PC back-channel UART (J14) — 115,200 baud

    // ── Splash screen briefly, then IOT reset ─────────────────────────────────
    lcd_BIG_mid();
    strcpy(display_line[BIG_TOP], "   Noah   ");
    strcpy(display_line[BIG_MID], "Homework 9");
    strcpy(display_line[BIG_BOT], "Cartwright");
    Display_Update(0, 0, 0, 0);    // Show splash immediately

    IOT_Init();                     // Hardware reset pulse — LCD shows IOT status

//------------------------------------------------------------------------------
// "While" Operating System — Background / Foreground loop
//------------------------------------------------------------------------------
    while (ALWAYS)
    {
        Serial_Process();           // Auto-transmit to PC every 1s (Project 9)
        Menu_Process();             // All menu logic (menus.c)
        Switches_Process();         // Consume debounced switch flags
        Display_Process();          // Flush display buffer to LCD when flagged
        P3OUT ^= TEST_PROBE;        // Toggle timing marker on oscilloscope
    }
}

//------------------------------------------------------------------------------
// Function: Display_Process
// Author:   Noah Cartwright
// Date:     March 31, 2026
// Description:
//   Called every main-loop iteration.  When the 50 ms timer ISR sets
//   update_display, and menus.c (or any other module) sets display_changed,
//   this function calls Display_Update() to write all four display_line[]
//   buffers to the LCD hardware.
// Parameters:  None
// Returns:     None
// Globals Modified: update_display, display_changed
//------------------------------------------------------------------------------
void Display_Process(void)
{
    if (update_display)
    {
        update_display = 0;
        if (display_changed)
        {
            display_changed = 0;
            Display_Update(0, 0, 0, 0);
        }
    }
}
