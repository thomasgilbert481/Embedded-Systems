//==============================================================================
// File Name: timers.c
// Description: Timer initialization and ISR for Project 7.
//
//              Timer B0 CCR0: fires every 5ms (8MHz SMCLK / 40000 = 200 Hz)
//                             => 200 ticks = 1 second
//              Timer B3:      hardware PWM for motor control (~160 Hz)
//
//              ISR handles:
//                - update_display: LCD refresh every ~200ms (40 ticks)
//                - p7_timer: state machine delay counter
//                - elapsed_tenths: 0.2-second display clock
//                - DAC ramp-up: enable DAC_ENB and step DAC_data to target
//
// Author: Thomas Gilbert
// Date: Mar 2026
// Compiler: Code Composer Studio
//==============================================================================

#include "msp430.h"
#include "functions.h"
#include "macros.h"
#include "ports.h"

//==============================================================================
// Globals defined here (previously provided by Carlson's timerB0.obj)
//==============================================================================
volatile unsigned int Time_Sequence = 0;
volatile char         one_time      = 0;

// Defined in LCD.obj -- extern only, NEVER redefine
extern volatile unsigned char  update_display;
extern volatile unsigned int   update_display_count;

// Project 7 state machine timers (defined in main.c)
extern volatile unsigned int p7_timer;
extern volatile unsigned int p7_running;

// Project 7 display clock (defined in main.c)
extern volatile unsigned int elapsed_tenths;
extern volatile unsigned int p7_timer_running;

// DAC ramp globals -- defined here, used in ISR
extern unsigned int DAC_data;               // Defined in dac.c
volatile unsigned int dac_ramp_active  = 1; // 1 = ramp in progress
volatile unsigned int dac_ramp_counter = 0; // Counts ISR ticks since Init_DAC

// Keep P5/P6 externals for compatibility
extern volatile unsigned int p5_timer;
extern volatile unsigned int p5_running;
extern volatile unsigned int p6_timer;
extern volatile unsigned int p6_running;

//==============================================================================
// Function: Init_Timers
//==============================================================================
void Init_Timers(void){
    Init_Timer_B0();
    Init_Timer_B3();
}

//==============================================================================
// Function: Init_Timer_B0
// Description: Configures Timer B0 CCR0 to fire every 5ms.
//              SMCLK = 8 MHz, TB0CCR0 = 40000 => 5ms interval.
//              Mode: Up (counts 0 to CCR0, then resets).
//==============================================================================
void Init_Timer_B0(void){
    TB0CTL  = TBSSEL__SMCLK;    // Clock source = SMCLK (8 MHz)
    TB0CTL |= MC__UP;           // Up mode: count to TB0CCR0
    TB0CTL |= TBCLR;            // Clear the timer counter

    TB0CCR0  = TB0CCR0_INTERVAL; // 40000 counts = 5ms

    TB0CCTL0 |= CCIE;           // Enable CCR0 interrupt
}

//==============================================================================
// Function: Init_Timer_B3
// Description: Configures Timer B3 for hardware PWM motor control.
//              Up mode: counts 0 to TB3CCR0 (WHEEL_PERIOD_VAL = 50005).
//              All motor channels use OUTMOD_7 (reset/set).
//              Setting CCRx = WHEEL_OFF (0) keeps the output LOW (motor off).
//
//   Pin mapping (Port 6, SEL0=1 SEL1=0 set in ports.c):
//     TB3CCR1 -> P6.1 -> RIGHT_FORWARD_SPEED
//     TB3CCR2 -> P6.2 -> LEFT_FORWARD_SPEED
//     TB3CCR3 -> P6.3 -> RIGHT_REVERSE_SPEED
//     TB3CCR4 -> P6.4 -> LEFT_REVERSE_SPEED
//==============================================================================
void Init_Timer_B3(void){
    TB3CTL  = TBSSEL__SMCLK;        // Clock source = SMCLK (8 MHz)
    TB3CTL |= MC__UP;               // Up mode: count 0 -> TB3CCR0, then reset
    TB3CTL |= TBCLR;                // Clear timer counter

    PWM_PERIOD = WHEEL_PERIOD_VAL;  // 50005 counts = ~6.25ms period (~160 Hz)

    TB3CCTL1 = OUTMOD_7;            // Output mode 7: reset/set
    RIGHT_FORWARD_SPEED = WHEEL_OFF;

    TB3CCTL2 = OUTMOD_7;
    LEFT_FORWARD_SPEED = WHEEL_OFF;

    TB3CCTL3 = OUTMOD_7;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;

    TB3CCTL4 = OUTMOD_7;
    LEFT_REVERSE_SPEED = WHEEL_OFF;
}

//==============================================================================
// ISR: Timer0_B0_ISR
// Description: Fires every 5ms (200 times per second).
//              Handles display clock, P7 timers, and DAC ramp.
//==============================================================================
#pragma vector = TIMER0_B0_VECTOR
__interrupt void Timer0_B0_ISR(void){

    //--------------------------------------------------------------------------
    // Master time counter
    //--------------------------------------------------------------------------
    Time_Sequence++;
    if(Time_Sequence % 50 == 0){
        one_time = 1;
    }

    //--------------------------------------------------------------------------
    // Display update -- refresh LCD every ~200ms (every 40 ticks at 5ms each)
    //--------------------------------------------------------------------------
    update_display_count++;
    if(update_display_count >= 40){
        update_display_count = 0;
        update_display = 1;
    }

    //--------------------------------------------------------------------------
    // Project 5 timer (legacy compatibility)
    //--------------------------------------------------------------------------
    if(p5_running){
        p5_timer++;
    }

    //--------------------------------------------------------------------------
    // Project 6 timer (legacy compatibility)
    //--------------------------------------------------------------------------
    if(p6_running){
        p6_timer++;
    }

    //--------------------------------------------------------------------------
    // Project 7 state machine timer
    //--------------------------------------------------------------------------
    if(p7_running){
        p7_timer++;
    }

    //--------------------------------------------------------------------------
    // 0.2-second display clock
    //   DISPLAY_TIMER_TICKS = 40 (40 * 5ms = 200ms = one 0.2s elapsed_tenth)
    //--------------------------------------------------------------------------
    if(p7_timer_running){
        static unsigned int display_timer_count = 0;
        display_timer_count++;
        if(display_timer_count >= DISPLAY_TIMER_TICKS){
            display_timer_count = 0;
            elapsed_tenths++;
        }
    }

    //--------------------------------------------------------------------------
    // DAC motor voltage ramp (only during startup)
    //--------------------------------------------------------------------------
    if(dac_ramp_active){
        dac_ramp_counter++;
        if(dac_ramp_counter == DAC_ENB_DELAY){
            P2OUT |= DAC_ENB;                    // Enable DAC power board
        }
        if(dac_ramp_counter > DAC_ENB_DELAY){
            if(DAC_data > DAC_TARGET_VALUE){
                DAC_data -= DAC_RAMP_STEP;
                if(DAC_data < DAC_TARGET_VALUE){
                    DAC_data = DAC_TARGET_VALUE;
                }
                SAC3DAT = DAC_data;
            } else {
                dac_ramp_active = 0;
            }
        }
    }

    TB0CCTL0 &= ~CCIFG;                         // Clear interrupt flag
}
