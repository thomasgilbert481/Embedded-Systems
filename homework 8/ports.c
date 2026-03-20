//==============================================================================
// File Name: ports.c
// Description: Port initialization for Homework 8 -- UART Serial Communications
//              Configures all GPIO pins for the MSP430FR2355.
//
//              Key changes from Project 7:
//                P1.6 (UCA0RXD) and P1.7 (UCA0TXD) configured as UART function
//                  for eUSCI_A0 (IOT port / J9).
//                P4.2 (UCA1RXD) and P4.3 (UCA1TXD) remain as UART function
//                  for eUSCI_A1 (PC backchannel / J14).
//                Motor pins (P6.1-P6.4) configured as GPIO outputs LOW --
//                  no Timer B3 PWM used in HW8.
//
// Author: Thomas Gilbert
// Date: Mar 2026
// Compiler: Code Composer Studio
// Target: MSP430FR2355
//==============================================================================

#include "msp430.h"
#include "functions.h"
#include "LCD.h"
#include "ports.h"
#include "macros.h"

//==============================================================================
// Function: Init_Ports
//==============================================================================
void Init_Ports(void){
    Init_Port1();
    Init_Port2();
    Init_Port3();
    Init_Port4();
    Init_Port5();
    Init_Port6();
}

//==============================================================================
// Function: Init_Port1
// P1.0: RED_LED (GPIO output)
// P1.2: V_DETECT_L (ADC A2 -- kept as analog input even though unused in HW8)
// P1.3: V_DETECT_R (ADC A3 -- same)
// P1.5: V_THUMB (ADC A5 -- same)
// P1.6: UCA0RXD -- UART function (SEL0=1, SEL1=0) for eUSCI_A0 IOT port
// P1.7: UCA0TXD -- UART function (SEL0=1, SEL1=0) for eUSCI_A0 IOT port
//==============================================================================
void Init_Port1(void){
    P1OUT = 0x00;
    P1DIR = 0x01;

    // P1.0 -- RED_LED (GPIO output)
    P1SEL0 &= ~RED_LED;
    P1SEL1 &= ~RED_LED;
    P1OUT  &= ~RED_LED;
    P1DIR  |=  RED_LED;

    // P1.1 -- A1_SEEED (GPIO input)
    P1SEL0 &= ~A1_SEEED;
    P1SEL1 &= ~A1_SEEED;
    P1OUT  &= ~A1_SEEED;
    P1DIR  &= ~A1_SEEED;

    // P1.2 -- V_DETECT_L (ADC analog input)
    P1SEL0 |=  V_DETECT_L;
    P1SEL1 |=  V_DETECT_L;
    P1OUT  &= ~V_DETECT_L;
    P1DIR  &= ~V_DETECT_L;

    // P1.3 -- V_DETECT_R (ADC analog input)
    P1SEL0 |=  V_DETECT_R;
    P1SEL1 |=  V_DETECT_R;
    P1OUT  &= ~V_DETECT_R;
    P1DIR  &= ~V_DETECT_R;

    // P1.4 -- A4_SEEED (GPIO input)
    P1SEL0 &= ~A4_SEEED;
    P1SEL1 &= ~A4_SEEED;
    P1OUT  &= ~A4_SEEED;
    P1DIR  &= ~A4_SEEED;

    // P1.5 -- V_THUMB (ADC analog input)
    P1SEL0 |=  V_THUMB;
    P1SEL1 |=  V_THUMB;
    P1OUT  &= ~V_THUMB;
    P1DIR  &= ~V_THUMB;

    // P1.6 -- UCA0RXD: eUSCI_A0 UART receive (IOT port J9)
    // UART function: SEL0 = 1, SEL1 = 0
    P1SEL0 |=  UCA0RXD;
    P1SEL1 &= ~UCA0RXD;

    // P1.7 -- UCA0TXD: eUSCI_A0 UART transmit (IOT port J9)
    // UART function: SEL0 = 1, SEL1 = 0
    P1SEL0 |=  UCA0TXD;
    P1SEL1 &= ~UCA0TXD;
}

//==============================================================================
// Function: Init_Port2
// P2.2: IR_LED (GPIO output, OFF at startup)
// P2.3: SW2 (GPIO input, pull-up, interrupt enabled)
// P2.5: DAC_ENB (GPIO output, LOW -- buck-boost disabled)
// P2.6-P2.7: LFXOUT/LFXIN (crystal)
//==============================================================================
void Init_Port2(void){
    P2OUT = 0x00;
    P2DIR = 0x00;

    P2SEL0 &= ~SLOW_CLK;
    P2SEL1 &= ~SLOW_CLK;
    P2OUT  &= ~SLOW_CLK;
    P2DIR  |=  SLOW_CLK;

    P2SEL0 &= ~CHECK_BAT;
    P2SEL1 &= ~CHECK_BAT;
    P2OUT  &= ~CHECK_BAT;
    P2DIR  |=  CHECK_BAT;

    // P2.2 -- IR_LED (GPIO output, OFF)
    P2SEL0 &= ~IR_LED;
    P2SEL1 &= ~IR_LED;
    P2OUT  &= ~IR_LED;
    P2DIR  |=  IR_LED;

    // P2.3 -- SW2 (GPIO input, pull-up, interrupt)
    P2SEL0 &= ~SW2;
    P2SEL1 &= ~SW2;
    P2OUT  |=  SW2;
    P2DIR  &= ~SW2;
    P2REN  |=  SW2;

    // P2.4 -- IOT_RUN_RED
    P2SEL0 &= ~IOT_RUN_RED;
    P2SEL1 &= ~IOT_RUN_RED;
    P2OUT  &= ~IOT_RUN_RED;
    P2DIR  |=  IOT_RUN_RED;

    // P2.5 -- DAC_ENB (GPIO output, LOW -- buck-boost stays disabled in HW8)
    P2SEL0 &= ~DAC_ENB;
    P2SEL1 &= ~DAC_ENB;
    P2OUT  &= ~DAC_ENB;
    P2DIR  |=  DAC_ENB;

    // P2.6-P2.7 -- LFXOUT/LFXIN (crystal function)
    P2SEL0 &= ~LFXOUT;
    P2SEL1 |=  LFXOUT;
    P2SEL0 &= ~LFXIN;
    P2SEL1 |=  LFXIN;

    // SW2 interrupt: high-to-low edge, enable
    P2IES |=  SW2;
    P2IFG &= ~SW2;
    P2IE  |=  SW2;
}

//==============================================================================
// Function: Init_Port3
// P3.0: TEST_PROBE (GPIO output, heartbeat toggle)
//==============================================================================
void Init_Port3(void){
    P3OUT = 0x00;
    P3DIR = 0x01;

    P3SEL0 &= ~TEST_PROBE;
    P3SEL1 &= ~TEST_PROBE;
    P3OUT  &= ~TEST_PROBE;
    P3DIR  |=  TEST_PROBE;

    P3SEL0 &= ~OA2O;
    P3SEL1 &= ~OA2O;
    P3OUT  &= ~OA2O;
    P3DIR  &= ~OA2O;

    P3SEL0 &= ~OA2N;
    P3SEL1 &= ~OA2N;
    P3OUT  &= ~OA2N;
    P3DIR  |=  OA2N;

    P3SEL0 &= ~OA2P;
    P3SEL1 &= ~OA2P;
    P3OUT  &= ~OA2P;
    P3DIR  &= ~OA2P;

    P3SEL0 &= ~SMCLK;
    P3SEL1 &= ~SMCLK;
    P3OUT  &= ~SMCLK;
    P3DIR  &= ~SMCLK;

    P3SEL0 &= ~DAC_CNTL;
    P3SEL1 &= ~DAC_CNTL;
    P3OUT  &= ~DAC_CNTL;
    P3DIR  &= ~DAC_CNTL;

    P3SEL0 &= ~IOT_LINK_CPU;
    P3SEL1 &= ~IOT_LINK_CPU;
    P3OUT  &= ~IOT_LINK_CPU;
    P3DIR  &= ~IOT_LINK_CPU;

    P3SEL0 &= ~IOT_EN_CPU;
    P3SEL1 &= ~IOT_EN_CPU;
    P3OUT  &= ~IOT_EN_CPU;
    P3DIR  &= ~IOT_EN_CPU;
}

//==============================================================================
// Function: Init_Port4
// P4.0: RESET_LCD (GPIO output)
// P4.1: SW1 (GPIO input, pull-up, interrupt)
// P4.2: UCA1RXD -- UART function (SEL0=1, SEL1=0) for eUSCI_A1 PC backchannel
// P4.3: UCA1TXD -- UART function (SEL0=1, SEL1=0) for eUSCI_A1 PC backchannel
// P4.4-P4.7: SPI for LCD
//==============================================================================
void Init_Port4(void){
    P4OUT = 0x00;
    P4DIR = 0x00;

    // P4.0 -- RESET_LCD (GPIO output)
    P4SEL0 &= ~RESET_LCD;
    P4SEL1 &= ~RESET_LCD;
    P4OUT  &= ~RESET_LCD;
    P4DIR  |=  RESET_LCD;

    // P4.1 -- SW1 (GPIO input, pull-up, interrupt)
    P4SEL0 &= ~SW1;
    P4SEL1 &= ~SW1;
    P4OUT  |=  SW1;
    P4DIR  &= ~SW1;
    P4REN  |=  SW1;

    // P4.2 -- UCA1RXD: eUSCI_A1 UART receive (PC backchannel J14)
    P4SEL0 |=  UCA1RXD;
    P4SEL1 &= ~UCA1RXD;

    // P4.3 -- UCA1TXD: eUSCI_A1 UART transmit (PC backchannel J14)
    P4SEL0 |=  UCA1TXD;
    P4SEL1 &= ~UCA1TXD;

    // P4.4 -- UCB1_CS_LCD (SPI chip select, GPIO output HIGH = deselected)
    P4SEL0 &= ~UCB1_CS_LCD;
    P4SEL1 &= ~UCB1_CS_LCD;
    P4OUT  |=  UCB1_CS_LCD;
    P4DIR  |=  UCB1_CS_LCD;

    // P4.5 -- UCB1CLK (SPI clock)
    P4SEL0 |=  UCB1CLK;
    P4SEL1 &= ~UCB1CLK;

    // P4.6 -- UCB1SIMO (SPI MOSI)
    P4SEL0 |=  UCB1SIMO;
    P4SEL1 &= ~UCB1SIMO;

    // P4.7 -- UCB1SOMI (SPI MISO)
    P4SEL0 |=  UCB1SOMI;
    P4SEL1 &= ~UCB1SOMI;

    // SW1 interrupt: high-to-low edge, enable
    P4IES |=  SW1;
    P4IFG &= ~SW1;
    P4IE  |=  SW1;
}

//==============================================================================
// Function: Init_Port5
//==============================================================================
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
    P5DIR  &= ~V_DAC;

    P5SEL0 &= ~V_3_3;
    P5SEL1 &= ~V_3_3;
    P5OUT  &= ~V_3_3;
    P5DIR  &= ~V_3_3;

    P5SEL0 &= ~IOT_BOOT_CPU;
    P5SEL1 &= ~IOT_BOOT_CPU;
    P5OUT  &= ~IOT_BOOT_CPU;
    P5DIR  &= ~IOT_BOOT_CPU;
}

//==============================================================================
// Function: Init_Port6
// P6.0: LCD_BACKLITE (GPIO output)
// P6.1-P6.4: Motor pins -- configured as GPIO outputs LOW (no PWM in HW8)
// P6.6: GRN_LED (GPIO output)
//==============================================================================
void Init_Port6(void){
    P6OUT = 0x00;
    P6DIR = 0x00;

    // P6.0 -- LCD Backlight (GPIO output)
    P6SEL0 &= ~LCD_BACKLITE;
    P6SEL1 &= ~LCD_BACKLITE;
    P6OUT  &= ~LCD_BACKLITE;
    P6DIR  |=  LCD_BACKLITE;

    // P6.1-P6.4 -- Motor pins: GPIO output LOW (motors off, no Timer B3 in HW8)
    P6SEL0 &= ~R_FORWARD;
    P6SEL1 &= ~R_FORWARD;
    P6OUT  &= ~R_FORWARD;
    P6DIR  |=  R_FORWARD;

    P6SEL0 &= ~L_FORWARD;
    P6SEL1 &= ~L_FORWARD;
    P6OUT  &= ~L_FORWARD;
    P6DIR  |=  L_FORWARD;

    P6SEL0 &= ~R_REVERSE;
    P6SEL1 &= ~R_REVERSE;
    P6OUT  &= ~R_REVERSE;
    P6DIR  |=  R_REVERSE;

    P6SEL0 &= ~L_REVERSE;
    P6SEL1 &= ~L_REVERSE;
    P6OUT  &= ~L_REVERSE;
    P6DIR  |=  L_REVERSE;

    // P6.5 -- unused GPIO input
    P6SEL0 &= ~P6_5;
    P6SEL1 &= ~P6_5;
    P6OUT  &= ~P6_5;
    P6DIR  &= ~P6_5;

    // P6.6 -- GRN_LED (GPIO output)
    P6SEL0 &= ~GRN_LED;
    P6SEL1 &= ~GRN_LED;
    P6OUT  &= ~GRN_LED;
    P6DIR  |=  GRN_LED;
}
