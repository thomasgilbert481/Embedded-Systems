//==============================================================================
// File Name: ports.c
// Description: Port initialization for MSP430FR2355 -- Project 7.
//              Motor pins P6.1-P6.4 configured for Timer B3 hardware PWM
//              output mode (SEL0=1, SEL1=0).
//              P2.5 (DAC_ENB) starts LOW; Timer B0 ISR enables it after DAC
//              output stabilizes (DAC_ENB_DELAY ticks).
//              SW1/SW2 are GPIO inputs with pull-ups (polling, no port ISRs).
// Author: Thomas Gilbert
// Date: Mar 2026
// Compiler: Code Composer Studio
//==============================================================================

#include "msp430.h"
#include "functions.h"
#include "LCD.h"
#include "ports.h"
#include "macros.h"

void Init_Ports(void){
    Init_Port1();
    Init_Port2();
    Init_Port3();
    Init_Port4();
    Init_Port5();
    Init_Port6();
}

void Init_Port1(void){
    P1OUT = 0x00;
    P1DIR = 0x01;

    P1SEL0 &= ~RED_LED;
    P1SEL1 &= ~RED_LED;
    P1OUT  &= ~RED_LED;
    P1DIR  |=  RED_LED;          // Output

    P1SEL0 &= ~A1_SEEED;
    P1SEL1 &= ~A1_SEEED;
    P1OUT  &= ~A1_SEEED;
    P1DIR  &= ~A1_SEEED;         // Input

    // V_DETECT_L -- P1.2 as ADC input (channel A2, Left IR detector)
    P1SEL0 |=  V_DETECT_L;      // ADC function: SEL0=1, SEL1=1
    P1SEL1 |=  V_DETECT_L;
    P1OUT  &= ~V_DETECT_L;
    P1DIR  &= ~V_DETECT_L;      // Input (ADC)

    // V_DETECT_R -- P1.3 as ADC input (channel A3, Right IR detector)
    P1SEL0 |=  V_DETECT_R;
    P1SEL1 |=  V_DETECT_R;
    P1OUT  &= ~V_DETECT_R;
    P1DIR  &= ~V_DETECT_R;      // Input (ADC)

    P1SEL0 &= ~A4_SEEED;
    P1SEL1 &= ~A4_SEEED;
    P1OUT  &= ~A4_SEEED;
    P1DIR  &= ~A4_SEEED;         // Input

    // V_THUMB -- P1.5 as ADC input (channel A5, Thumbwheel)
    P1SEL0 |=  V_THUMB;
    P1SEL1 |=  V_THUMB;
    P1OUT  &= ~V_THUMB;
    P1DIR  &= ~V_THUMB;         // Input (ADC)

    P1SEL0 &= ~UCA0RXD;
    P1SEL1 &= ~UCA0RXD;
    P1OUT  &= ~UCA0RXD;
    P1DIR  &= ~UCA0RXD;         // Input

    P1SEL0 &= ~UCA0TXD;
    P1SEL1 &= ~UCA0TXD;
    P1OUT  &= ~UCA0TXD;
    P1DIR  &= ~UCA0TXD;         // Input
}

void Init_Port2(void){
    P2OUT = 0x00;
    P2DIR = 0x00;

    P2SEL0 &= ~SLOW_CLK;
    P2SEL1 &= ~SLOW_CLK;
    P2OUT  &= ~SLOW_CLK;
    P2DIR  |=  SLOW_CLK;         // Output

    P2SEL0 &= ~CHECK_BAT;
    P2SEL1 &= ~CHECK_BAT;
    P2OUT  &= ~CHECK_BAT;
    P2DIR  |=  CHECK_BAT;        // Output

    P2SEL0 &= ~IR_LED;
    P2SEL1 &= ~IR_LED;
    P2OUT  &= ~IR_LED;           // IR emitter OFF at startup
    P2DIR  |=  IR_LED;           // Output

    P2SEL0 &= ~SW2;
    P2SEL1 &= ~SW2;
    P2OUT  |=  SW2;              // Enable pull-up resistor
    P2DIR  &= ~SW2;              // Input
    P2REN  |=  SW2;              // Pull-up enabled

    P2SEL0 &= ~IOT_RUN_RED;
    P2SEL1 &= ~IOT_RUN_RED;
    P2OUT  &= ~IOT_RUN_RED;
    P2DIR  |=  IOT_RUN_RED;      // Output

    // DAC_ENB (P2.5) -- starts LOW so buck-boost is disabled at startup.
    // Timer B0 ISR enables it after DAC_ENB_DELAY ticks (~15ms) so the DAC
    // output has time to stabilize before the power board is enabled.
    P2SEL0 &= ~DAC_ENB;
    P2SEL1 &= ~DAC_ENB;
    P2OUT  &= ~DAC_ENB;          // Initial Value = Low (disabled)
    P2DIR  |=  DAC_ENB;          // Output

    P2SEL0 &= ~LFXOUT;
    P2SEL1 |=  LFXOUT;           // Crystal function

    P2SEL0 &= ~LFXIN;
    P2SEL1 |=  LFXIN;            // Crystal function
}

void Init_Port3(void){
    P3OUT = 0x00;
    P3DIR = 0x01;

    P3SEL0 &= ~TEST_PROBE;
    P3SEL1 &= ~TEST_PROBE;
    P3OUT  &= ~TEST_PROBE;
    P3DIR  |=  TEST_PROBE;       // Output

    P3SEL0 &= ~OA2O;
    P3SEL1 &= ~OA2O;
    P3OUT  &= ~OA2O;
    P3DIR  &= ~OA2O;             // Input

    P3SEL0 &= ~OA2N;
    P3SEL1 &= ~OA2N;
    P3OUT  &= ~OA2N;
    P3DIR  |=  OA2N;             // Output

    P3SEL0 &= ~OA2P;
    P3SEL1 &= ~OA2P;
    P3OUT  &= ~OA2P;
    P3DIR  &= ~OA2P;             // Input

    P3SEL0 &= ~SMCLK;
    P3SEL1 &= ~SMCLK;
    P3OUT  &= ~SMCLK;
    P3DIR  &= ~SMCLK;            // Input

    // DAC_CNTL (P3.5) -- Init_DAC() sets P3SELC to enable analog DAC output.
    // Here we set GPIO defaults; Init_DAC() takes over after timers are running.
    P3SEL0 &= ~DAC_CNTL;
    P3SEL1 &= ~DAC_CNTL;
    P3OUT  &= ~DAC_CNTL;
    P3DIR  &= ~DAC_CNTL;         // Input (DAC takes over in Init_DAC)

    P3SEL0 &= ~IOT_LINK_CPU;
    P3SEL1 &= ~IOT_LINK_CPU;
    P3OUT  &= ~IOT_LINK_CPU;
    P3DIR  &= ~IOT_LINK_CPU;     // Input

    P3SEL0 &= ~IOT_EN_CPU;
    P3SEL1 &= ~IOT_EN_CPU;
    P3OUT  &= ~IOT_EN_CPU;
    P3DIR  &= ~IOT_EN_CPU;       // Input
}

void Init_Port4(void){
    P4OUT = 0x00;
    P4DIR = 0x00;

    P4SEL0 &= ~RESET_LCD;
    P4SEL1 &= ~RESET_LCD;
    P4OUT  &= ~RESET_LCD;
    P4DIR  |=  RESET_LCD;        // Output

    // SW1 (P4.1) -- GPIO input with pull-up (polled in Switches_Process)
    P4SEL0 &= ~SW1;
    P4SEL1 &= ~SW1;
    P4OUT  |=  SW1;              // Enable pull-up resistor
    P4DIR  &= ~SW1;              // Input
    P4REN  |=  SW1;              // Pull-up enabled

    P4SEL0 |=  UCA1TXD;
    P4SEL1 &= ~UCA1TXD;          // USCI_A1 UART

    P4SEL0 |=  UCA1RXD;
    P4SEL1 &= ~UCA1RXD;          // USCI_A1 UART

    P4SEL0 &= ~UCB1_CS_LCD;
    P4SEL1 &= ~UCB1_CS_LCD;
    P4OUT  |=  UCB1_CS_LCD;      // SPI CS LCD Off (High)
    P4DIR  |=  UCB1_CS_LCD;      // Output

    P4SEL0 |=  UCB1CLK;
    P4SEL1 &= ~UCB1CLK;          // SPI clock

    P4SEL0 |=  UCB1SIMO;
    P4SEL1 &= ~UCB1SIMO;         // SPI MOSI

    P4SEL0 |=  UCB1SOMI;
    P4SEL1 &= ~UCB1SOMI;         // SPI MISO
}

void Init_Port5(void){
    P5OUT = 0x00;
    P5DIR = 0x01;

    P5SEL0 &= ~V_BAT;
    P5SEL1 &= ~V_BAT;
    P5OUT  &= ~V_BAT;
    P5DIR  &= ~V_BAT;

    P5SEL0 &= ~V_5;
    P5SEL1 &= ~V_5;
    P5OUT  &= ~V_5;
    P5DIR  &= ~V_5;

    P5SEL0 &= ~V_DAC;
    P5SEL1 &= ~V_DAC;
    P5OUT  &= ~V_DAC;
    P5DIR  |=  V_DAC;

    P5SEL0 &= ~V_3_3;
    P5SEL1 &= ~V_3_3;
    P5OUT  &= ~V_3_3;
    P5DIR  &= ~V_3_3;

    P5SEL0 &= ~IOT_BOOT_CPU;
    P5SEL1 &= ~IOT_BOOT_CPU;
    P5OUT  &= ~IOT_BOOT_CPU;
    P5DIR  &= ~IOT_BOOT_CPU;
}

void Init_Port6(void){
    P6OUT = 0x00;
    P6DIR = 0x00;

    // LCD Backlight (P6.0) -- GPIO output (toggled by main loop or left on)
    P6SEL0 &= ~LCD_BACKLITE;
    P6SEL1 &= ~LCD_BACKLITE;
    P6OUT  &= ~LCD_BACKLITE;     // Backlight OFF at startup
    P6DIR  |=  LCD_BACKLITE;     // Output

    // Right Forward Motor (P6.1) -- Timer B3 CCR1 PWM output
    P6SEL0 |=  R_FORWARD;        // Timer B3 function: SEL0=1
    P6SEL1 &= ~R_FORWARD;        // Timer B3 function: SEL1=0
    P6DIR  |=  R_FORWARD;        // Output (required for timer output)

    // Left Forward Motor (P6.2) -- Timer B3 CCR2 PWM output
    P6SEL0 |=  L_FORWARD;
    P6SEL1 &= ~L_FORWARD;
    P6DIR  |=  L_FORWARD;

    // Right Reverse Motor (P6.3) -- Timer B3 CCR3 PWM output
    P6SEL0 |=  R_REVERSE;
    P6SEL1 &= ~R_REVERSE;
    P6DIR  |=  R_REVERSE;

    // Left Reverse Motor (P6.4) -- Timer B3 CCR4 PWM output
    P6SEL0 |=  L_REVERSE;
    P6SEL1 &= ~L_REVERSE;
    P6DIR  |=  L_REVERSE;

    // P6.5 unused -- GPIO input
    P6SEL0 &= ~P6_5;
    P6SEL1 &= ~P6_5;
    P6OUT  &= ~P6_5;
    P6DIR  &= ~P6_5;             // Input

    // Green LED (P6.6) -- GPIO output
    P6SEL0 &= ~GRN_LED;
    P6SEL1 &= ~GRN_LED;
    P6OUT  &= ~GRN_LED;
    P6DIR  |=  GRN_LED;          // Output
}
