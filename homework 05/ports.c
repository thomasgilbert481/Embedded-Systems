//==============================================================================
// File Name: ports.c
// Description: Port initialization for MSP430FR2355
//              Configures all GPIO pins based on hardware connections
// Author: Thomas Gilbert
// Date: 2/4/2026
// Compiler: Code Composer Studio [version]
//
// Pin configurations based on:
// - MSP-EXP430FR2355 LaunchPad schematic
// - Control Module schematic
//==============================================================================

#include "msp430.h"
#include "functions.h"
#include "LCD.h"
#include "ports.h"
#include "macros.h"

//==============================================================================
// Function: Init_Ports
// Description: Calls all individual port initialization functions
//==============================================================================
void Init_Ports(void){ //Init all ports
    Init_Port1();
    Init_Port2();
    Init_Port3(USE_SMCLK);   // or USE_GPIO — swap to test;
    Init_Port4();
    Init_Port5();
    Init_Port6();

}

    void Init_Port1(void){ // Configure Port 1
            //------------------------------------------------------------------------------
             P1OUT = 0x00; // P1 set Low
             P1DIR = 0x01; // Set P1 direction to input

             P1SEL0 &= ~RED_LED; //  GPIO operation
             P1SEL1 &= ~RED_LED; //  GPIO operation
             P1OUT &= ~RED_LED; // Initial Value = Low / Off
             P1DIR |= RED_LED; // Direction = output

             P1SEL0 &= ~A1_SEEED; //  GPIO operation
             P1SEL1 &= ~A1_SEEED; //  GPIO operation
             P1OUT &= ~A1_SEEED; // Initial Value = Low / Off
             P1DIR &= ~A1_SEEED; // Direction = input

             P1SEL0 &= ~V_DETECT_L; //  GPIO operation
             P1SEL1 &= ~V_DETECT_L; //  GPIO operation
             P1OUT &= ~V_DETECT_L; // Initial Value = Low / Off
             P1DIR &= ~V_DETECT_L; // Direction = output

             P1SEL0 &= ~V_DETECT_R; //  Operation
             P1SEL1 &= ~V_DETECT_R; //  Operation
             P1OUT &= ~V_DETECT_R; // Configure pullup resistor
             P1DIR &= ~V_DETECT_R; // Direction = input

             P1SEL0 &= ~A4_SEEED; //  GPIO operation
             P1SEL1 &= ~A4_SEEED; //  GPIO operation
             P1OUT &= ~A4_SEEED; // Initial Value = Low / Off
             P1DIR &= ~A4_SEEED; // Direction = input

             P1SEL0 &= ~V_THUMB; //  GPIO operation
             P1SEL1 &= ~V_THUMB; //  GPIO operation
             P1OUT &= ~V_THUMB; // Initial Value = low
             P1DIR &= ~V_THUMB; // Direction = input

             P1SEL0 &= ~UCA0RXD; // LFXOUT Clock operation
             P1SEL1 &= ~UCA0RXD; // LFXOUT Clock operation
             P1OUT &= ~UCA0RXD; // Initial Value = low
             P1DIR &= ~UCA0RXD; // Direction = input

             P1SEL0 &= ~UCA0TXD; // LFXIN Clock operation
             P1SEL1 &= ~UCA0TXD; // LFXIN Clock operation
             P1OUT &= ~UCA0TXD; // Initial Value = low
             P1DIR &= ~UCA0TXD; // Direction = input
             //------------------------------------------------------------------------------
            }

    void Init_Port2(void){ // Configure Port 2
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
         P2OUT &= ~IR_LED; // Initial Value = Low / Off
         P2DIR |= IR_LED; // Direction = output

         P2SEL0 &= ~SW2; // SW2 Operation
         P2SEL1 &= ~SW2; // SW2 Operation
         P2OUT |= SW2; // Configure pullup resistor
         P2DIR &= ~SW2; // Direction = input
         P2REN |= SW2; // Enable pullup resistor

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



    void Init_Port3(char smclk_mode){ // Configure Port 3
                //------------------------------------------------------------------------------
                 P3OUT = 0x00; // P3 set Low
                 P3DIR = 0x01; // Set P3 direction to input

                 P3SEL0 &= ~TEST_PROBE; //  GPIO operation
                 P3SEL1 &= ~TEST_PROBE; //  GPIO operation
                 P3OUT &= ~TEST_PROBE; // Initial Value = Low / Off
                 P3DIR |= TEST_PROBE; // Direction = output

                 P3SEL0 &= ~OA2O; //  GPIO operation
                 P3SEL1 &= ~OA2O; //  GPIO operation
                 P3OUT &= ~OA2O; // Initial Value = Low / Off
                 P3DIR &= ~OA2O; // Direction = input

                 P3SEL0 &= ~OA2N; //  GPIO operation
                 P3SEL1 &= ~OA2N; //  GPIO operation
                 P3OUT &= ~OA2N; // Initial Value = Low / Off
                 P3DIR &= ~OA2N; // Direction = output

                 P3SEL0 &= ~OA2P; //  Operation
                 P3SEL1 &= ~OA2P; //  Operation
                 P3OUT &= ~OA2P; // Configure pullup resistor
                 P3DIR &= ~OA2P; // Direction = input

                 // P3.4 - SMCLK or GPIO depending on argument
                   if (smclk_mode == USE_SMCLK) {
                       // Route SMCLK to P3.4 (alternate function: SEL0=1, SEL1=0)
                       P3SEL0 |=  SMCLK;   // SMCLK alternate function
                       P3SEL1 &= ~SMCLK;
                       P3DIR  |=  SMCLK;   // Direction = output
                   } else {
                       // USE_GPIO: P3.4 as regular GPIO
                       P3SEL0 &= ~SMCLK;   // GPIO operation
                       P3SEL1 &= ~SMCLK;
                       P3OUT  &= ~SMCLK;   // Initial value = Low
                       P3DIR  |=  SMCLK;   // Direction = output
                   }


                 P3SEL0 &= ~DAC_CNTL; //  GPIO operation
                 P3SEL1 &= ~DAC_CNTL; //  GPIO operation
                 P3OUT &= ~DAC_CNTL; // Initial Value = low
                 P3DIR &= ~DAC_CNTL; // Direction = input

                 P3SEL0 &= ~IOT_LINK_CPU; // LFXOUT Clock operation
                 P3SEL1 &= ~IOT_LINK_CPU; // LFXOUT Clock operation
                 P3OUT &= ~IOT_LINK_CPU; // Initial Value = low
                 P3DIR &= ~IOT_LINK_CPU; // Direction = input

                 P3SEL0 &= ~IOT_EN_CPU; // LFXIN Clock operation
                 P3SEL1 &= ~IOT_EN_CPU; // LFXIN Clock operation
                 P3OUT &= ~IOT_EN_CPU; // Initial Value = low
                 P3DIR &= ~IOT_EN_CPU; // Direction = input
                 //------------------------------------------------------------------------------
                }


    void Init_Port4(void){ // Configure PORT 4
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

     P4SEL0 |= UCA1TXD; // USCI_A1 UART operation
     P4SEL1 &= ~UCA1TXD; // USCI_A1 UART operation

     P4SEL0 |= UCA1RXD; // USCI_A1 UART operation
     P4SEL1 &= ~UCA1RXD; // USCI_A1 UART operation

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
     //------------------------------------------------------------------------------
    }

    void  Init_Port5(void){ // Configure Port 5
                   //------------------------------------------------------------------------------
                    P5OUT = 0x00; // P5 set Low
                    P5DIR = 0x01; // Set P5 direction to input

                    P5SEL0 &= ~V_BAT; //  GPIO operation
                    P5SEL1 &= ~V_BAT; //  GPIO operation
                    P5OUT &= ~V_BAT; // Initial Value = Low / Off
                    P5DIR &= ~V_BAT; // Direction = output

                    P5SEL0 &= ~V_5; //  GPIO operation
                    P5SEL1 &= ~V_5; //  GPIO operation
                    P5OUT &= ~V_5; // Initial Value = Low / Off
                    P5DIR &= ~V_5; // Direction = input

                    P5SEL0 &= ~V_DAC; //  GPIO operation
                    P5SEL1 &= ~V_DAC; //  GPIO operation
                    P5OUT &= ~V_DAC; // Initial Value = Low / Off
                    P5DIR &= ~V_DAC; // Direction = output

                    P5SEL0 &= ~V_3_3; //  Operation
                    P5SEL1 &= ~V_3_3; //  Operation
                    P5OUT &= ~V_3_3; // Configure pullup resistor
                    P5DIR &= ~V_3_3; // Direction = input

                    P5SEL0 &= ~IOT_BOOT_CPU; //  GPIO operation
                    P5SEL1 &= ~IOT_BOOT_CPU; //  GPIO operation
                    P5OUT &= ~IOT_BOOT_CPU; // Initial Value = Low / Off
                    P5DIR &= ~IOT_BOOT_CPU; // Direction = input

                    //------------------------------------------------------------------------------
                   }

    void Init_Port6(void){
        P6OUT = 0x00;
        P6DIR = 0x00; //direction to OUTPUT

        // LCD Backlight
        P6SEL0 &= ~LCD_BACKLITE;
        P6SEL1 &= ~LCD_BACKLITE;
        P6OUT &= ~LCD_BACKLITE;       // Backlight OFF (save battery)
        P6DIR |= LCD_BACKLITE;       // Output

        // Right Forward Motor
        P6SEL0 &= ~R_FORWARD;
        P6SEL1 &= ~R_FORWARD;
        P6OUT &= ~R_FORWARD;         // Initially OFF
        P6DIR |= R_FORWARD;          // *** OUTPUT ***

        // Left Forward Motor
        P6SEL0 &= ~L_FORWARD;
        P6SEL1 &= ~L_FORWARD;
        P6OUT &= ~L_FORWARD;         // Initially OFF
        P6DIR |= L_FORWARD;          // *** OUTPUT ***

        // Right Reverse Motor
        P6SEL0 &= ~R_REVERSE;
        P6SEL1 &= ~R_REVERSE;
        P6OUT &= ~R_REVERSE;         // OFF
        P6DIR |= R_REVERSE;          // *** OUTPUT ***

        // Left Reverse Motor
        P6SEL0 &= ~L_REVERSE;
        P6SEL1 &= ~L_REVERSE;
        P6OUT &= ~L_REVERSE;         // OFF
        P6DIR |= L_REVERSE;          // *** OUTPUT ***

        // P6.5 unused
        P6SEL0 &= ~P6_5;
        P6SEL1 &= ~P6_5;
        P6OUT &= ~P6_5;
        P6DIR &= ~P6_5;              // Input (unused)

        // Green LED
        P6SEL0 &= ~GRN_LED;
        P6SEL1 &= ~GRN_LED;
        P6OUT &= ~GRN_LED;
        P6DIR |= GRN_LED;            // Output
    }

