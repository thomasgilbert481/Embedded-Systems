/*
 * ports.c
 *
 *  Created on: Feb 6, 2026
 *      Author: noah
 */

#include  "msp430.h"
#include  <string.h>
#include  "functions.h"
#include  "LCD.h"
#include  "ports.h"
#include "globals.h"
#include "macros.h"

void Init_Ports(void)
{
    Init_Port1();
    Init_Port2();
    Init_Port3(USE_SMCLK);
    Init_Port4();
    Init_Port5();
    Init_Port6();

}

void Init_Port1(void)
{ // Configure port 1
//-----------------------------------------------------------------------------
    P1OUT = 0x00;
    P1DIR = 0x00;

    P1SEL0 &= ~RED_LED; // RED_LED GPIO OPERATION
    P1SEL1 &= ~RED_LED; // RED_LED GPIO OPERATION
    P1OUT &= ~RED_LED; // Initial value = low / off
    P1DIR |= RED_LED; // Direction = output

    P1SEL0 |= A1_SEEED;
    P1SEL1 |= A1_SEEED;
    P1OUT &= ~A1_SEEED; //initial value = low/off
    P1DIR &= ~A1_SEEED; // Direction = input

    P1SEL0 |= V_DETECT_L;
    P1SEL1 |= V_DETECT_L;
    P1OUT &= ~V_DETECT_L;
    P1DIR &= ~V_DETECT_L;

    P1SEL0 |= V_DETECT_R;
    P1SEL1 |= V_DETECT_R;
    P1OUT &= ~V_DETECT_R;
    P1DIR &= ~V_DETECT_R;

    P1SEL0 |= A4_SEEED;
    P1SEL1 |= A4_SEEED;
    P1OUT &= ~A4_SEEED; //initial value = low/off
    P1DIR &= ~A4_SEEED; // Direction = input

    P1SEL0 |= V_THUMB;
    P1SEL1 |= V_THUMB;
    P1OUT &= ~V_THUMB; //initial value = low/off
    P1DIR &= ~V_THUMB; // Direction = input

    P1SEL0 |= UCA0RXD;
    P1SEL1 &= ~UCA0RXD;
    P1OUT &= ~UCA0RXD; //initial value = low/off
    P1DIR &= ~UCA0RXD; // Direction = input

    P1SEL0 |= UCA0TXD;
    P1SEL1 &= ~UCA0TXD;
    P1OUT &= ~UCA0TXD; //initial value = low/off
    P1DIR |= UCA0TXD;  // Direction = output (UART TX must drive the line)

}

void Init_Port2(void)
{ // Configure Port 2
//------------------------------------------------------------------------------
    P2OUT = 0x00; // P2 set Low
    P2DIR = 0x00; // Set P2 direction to output

    P2SEL0 &= ~SLOW_CLK; // SLOW_CLK GPIO operation
    P2SEL1 &= ~SLOW_CLK; // SLOW_CLK GPIO operation
    P2OUT &= ~SLOW_CLK; // Initial Value = Low / Off
    P2DIR |= SLOW_CLK; // Direction = output

    P2SEL0 &= ~CHECK_BAT; // CHECK_BAT GPIO operation
    P2SEL1 &= ~CHECK_BAT; // CHECK_BAT GPIO operation
    P2OUT &= ~CHECK_BAT; // Initial Value = Low / Off
    P2DIR |= CHECK_BAT; // Direction = output

    P2SEL0 &= ~IR_LED; // P2_2 GPIO operation
    P2SEL1 &= ~IR_LED; // P2_2 GPIO operation
    P2OUT |= IR_LED; // Initial Value = Low / Off
    P2DIR |= IR_LED; // Direction = output

    P2SEL0 &= ~SW2; // SW2 Operation
    P2SEL1 &= ~SW2; // SW2 Operation
    P2OUT |= SW2; // Configure pullup resistor
    P2DIR &= ~SW2; // Direction = input
    P2REN |= SW2; // Enable pullup resistor
    P2IES |= SW2;    // SW2 Hi/Lo edge interrupt
    P2IFG &= ~SW2;   // IFG SW2 cleared
    P2IE |= SW2;    // SW2 interrupt Enabled

    P2SEL0 &= ~IOT_RUN_RED; // IOT_RUN_CPU GPIO operation
    P2SEL1 &= ~IOT_RUN_RED; // IOT_RUN_CPU GPIO operation
    P2OUT &= ~IOT_RUN_RED; // Initial Value = Low / Off
    P2DIR |= IOT_RUN_RED; // Direction = output

    P2SEL0 &= ~DAC_ENB; // DAC_ENB GPIO operation
    P2SEL1 &= ~DAC_ENB; // DAC_ENB GPIO operation
    P2OUT |= DAC_ENB; // Initial Value = High
    P2DIR |= DAC_ENB; // Direction = output

    P2SEL0 &= ~LFXOUT; // LFXOUT Clock operation
    P2SEL1 |= LFXOUT; // LFXOUT Clock operation

    P2SEL0 &= ~LFXIN; // LFXIN Clock operation
    P2SEL1 |= LFXIN; // LFXIN Clock operation
    //------------------------------------------------------------------------------
}

void Init_Port3(unsigned char mode)
{
    //------------------------------------------------------------------
    P3OUT = 0x00;
    P3DIR = 0x00;

    P3SEL0 &= ~TEST_PROBE;
    P3SEL1 &= ~TEST_PROBE;
    P3OUT &= ~TEST_PROBE;
    P3DIR |= TEST_PROBE;

    P3SEL0 &= ~OA2O;
    P3SEL1 &= ~OA2O;
    P3OUT &= ~OA2O;
    P3DIR &= ~OA2O;

    P3SEL0 &= ~OA2N;
    P3SEL1 &= ~OA2N;
    P3OUT &= ~OA2N;
    P3DIR &= ~OA2N;

    P3SEL0 &= ~OA2P;
    P3SEL1 &= ~OA2P;
    P3OUT &= ~OA2P;
    P3DIR &= ~OA2P;

    if (mode == USE_SMCLK)
    { //if true then SMCLK
        //given by carlson (correct)
        P3SEL0 |= SMCLK_OUT;
        P3SEL1 &= ~SMCLK_OUT;
        P3DIR |= SMCLK_OUT;

    }
    else
    { //else use GPIO

        P3SEL0 &= ~SMCLK_OUT;
        P3SEL1 &= ~SMCLK_OUT;
        P3OUT &= ~SMCLK_OUT;
        P3DIR &= ~SMCLK_OUT;
    }

    //what i had originially
    //P3SEL0 &= ~SMCLK;
    //P3SEL1 &= ~SMCLK;
    //P3OUT &= ~SMCLK;
    //P3DIR &= ~SMCLK;

    //P3SEL0 &= ~DAC_CNTL;
    //P3SEL1 &= ~DAC_CNTL;
    //P3OUT &= ~DAC_CNTL;
    //P3DIR &= ~DAC_CNTL;
    P3SELC |= DAC_CNTL;      // DAC operation

    P3SEL0 &= ~IOT_LINK_CPU;  // IOT_LINK_CPU (P3.6) GPIO
    P3SEL1 &= ~IOT_LINK_CPU;
    P3OUT &= ~IOT_LINK_CPU;   // Initial value = low
    P3DIR |=  IOT_LINK_CPU;   // Direction = output

    // IOT_EN (P3.7): active-low reset — held LOW on startup to keep IOT in reset.
    // Release HIGH after port init + min 100 ms reset time in software.
    P3SEL0 &= ~IOT_RN;        // IOT_EN (P3.7) GPIO
    P3SEL1 &= ~IOT_RN;
    P3OUT &= ~IOT_RN;         // Initial value = LOW (IOT held in reset)
    P3DIR |=  IOT_RN;         // Direction = output
}

void Init_Port4(void)
{ // Configure PORT 4
//------------------------------------------------------------------------------
    P4OUT = 0x00; // P4 set Low
    P4DIR = 0x00; // Set P4 direction to output

    P4SEL0 &= ~RESET_LCD; // RESET_LCD GPIO operation
    P4SEL1 &= ~RESET_LCD; // RESET_LCD GPIO operation
    P4OUT &= ~RESET_LCD; // Initial Value = Low / Off
    P4DIR |= RESET_LCD; // Direction = output

    P4SEL0 &= ~SW1; // SW1 GPIO operation
    P4SEL1 &= ~SW1; // SW1 GPIO operation
    P4OUT |= SW1; // Configure pullup resistor
    P4DIR &= ~SW1; // Direction = input
    P4REN |= SW1; // Enable pullup resistor

    P4SEL0 |= UCA1TXD;  // USCI_A1 UART operation
    P4SEL1 &= ~UCA1TXD;
    P4DIR  |= UCA1TXD;  // Direction = output (UART TX must drive the line)

    P4SEL0 |= UCA1RXD;  // USCI_A1 UART operation
    P4SEL1 &= ~UCA1RXD;
    P4DIR  &= ~UCA1RXD; // Direction = input

    P4SEL0 &= ~UCB1_CS_LCD; // UCB1_CS_LCD GPIO operation
    P4SEL1 &= ~UCB1_CS_LCD; // UCB1_CS_LCD GPIO operation
    P4OUT |= UCB1_CS_LCD; // Set SPI_CS_LCD Off [High]
    P4DIR |= UCB1_CS_LCD; // Set SPI_CS_LCD direction to output

    P4SEL0 |= UCB1CLK; // UCB1CLK SPI BUS operation
    P4SEL1 &= ~UCB1CLK; // UCB1CLK SPI BUS operation

    P4SEL0 |= UCB1SIMO; // UCB1SIMO SPI BUS operation
    P4SEL1 &= ~UCB1SIMO; // UCB1SIMO SPI BUS operation

    P4SEL0 |= UCB1SOMI; // UCB1SOMI SPI BUS operation
    P4SEL1 &= ~UCB1SOMI; // UCB1SOMI SPI BUS operation

    P4SEL0 &= ~SW1; // SW1 set as I/0
    P4SEL1 &= ~SW1; // SW1 set as I/0
    P4DIR &= ~SW1; // SW1 Direction = input
    P4PUD |= SW1; // Configure pull-up resistor SW1
    P4REN |= SW1; // Enable pull-up resistor SW1
    P4IES |= SW1; // SW1 Hi/Lo edge interrupt
    P4IFG &= ~SW1; // IFG SW1 cleared
    P4IE |= SW1; // SW1 interrupt Enabled
    //----------------------------------------------------------------------------
}

void Init_Port5(void)
{
    P5OUT = 0x00;
    P5DIR = 0x00;

    P5SEL0 &= ~V_BAT;
    P5SEL1 &= ~V_BAT;
    P5OUT &= ~V_BAT;
    P5DIR &= ~V_BAT;

    P5SEL0 &= ~V_5_0;
    P5SEL1 &= ~V_5_0;
    P5OUT &= ~V_5_0;
    P5DIR &= ~V_5_0;

    P5SEL0 &= ~V_DAC;
    P5SEL1 &= ~V_DAC;
    P5OUT &= ~V_DAC;
    P5DIR &= ~V_DAC;

    P5SEL0 &= ~V_3_3;
    P5SEL1 &= ~V_3_3;
    P5OUT &= ~V_3_3;
    P5DIR &= ~V_3_3;

    // IOT_BOOT (P5.4): must be HIGH during normal operation.
    // LOW would trigger ESP32 firmware download mode on reset.
    P5SEL0 &= ~IOT_BOOT;      // IOT_BOOT GPIO
    P5SEL1 &= ~IOT_BOOT;
    P5OUT |=  IOT_BOOT;       // Initial value = HIGH (normal run mode)
    P5DIR |=  IOT_BOOT;       // Direction = output
}

void Init_Port6(void)
{ //all motors go to outputs for dir
    P6OUT = 0x00;
    P6DIR = 0x00;

    P6SEL0 |= LCD_BACKLITE;
    P6SEL1 &= ~LCD_BACKLITE;
    P6OUT &= ~LCD_BACKLITE;
    P6DIR |= LCD_BACKLITE;

    // R_FORWARD - TB3.2
    P6SEL0 |= R_FORWARD;
    P6SEL1 &= ~R_FORWARD;
    P6OUT &= ~R_FORWARD;
    P6DIR |= R_FORWARD;

    // L_FORWARD - TB3.3
    P6SEL0 |= L_FORWARD;
    P6SEL1 &= ~L_FORWARD;
    P6OUT &= ~L_FORWARD;
    P6DIR |= L_FORWARD;

    // R_REVERSE - TB3.4
    P6SEL0 |= R_REVERSE;
    P6SEL1 &= ~R_REVERSE;
    P6OUT &= ~R_REVERSE;
    P6DIR |= R_REVERSE;

    // L_REVERSE - TB3.5
    P6SEL0 |= L_REVERSE;
    P6SEL1 &= ~L_REVERSE;
    P6OUT &= ~L_REVERSE;
    P6DIR |= L_REVERSE;

    // P6_5 - unused
    P6SEL0 &= ~P6_5;
    P6SEL1 &= ~P6_5;
    P6OUT &= ~P6_5;
    P6DIR &= ~P6_5;

    // GRN_LED - 6.6
    P6SEL0 &= ~GRN_LED;
    P6SEL1 &= ~GRN_LED;
    P6OUT &= ~GRN_LED;
    P6DIR |= GRN_LED;

}
