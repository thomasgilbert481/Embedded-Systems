// ------------------------------------------------------------------------------
//
//  Description: This file contains the Clock Initialization
//
//  Thomas Gilbert
//  Feb 2026
//  Built with Code Composer Studio
//
//  Globals Defined: none
//  Description: Configures MCLK = 8MHz (DCO), ACLK = 32768Hz (XT1),
//               SMCLK = 500kHz (MCLK / 16) for HW05 scope demonstration.
//               Note: changing SMCLK from 8MHz to 500kHz affects any
//               peripherals sourced from SMCLK (LCD SPI, Timer_B0).
//               Timer_B0 interval adjusted in macros.h accordingly.
// ------------------------------------------------------------------------------

#include "msp430.h"
#include "functions.h"
#include "LCD.h"
#include "ports.h"
#include "macros.h"

// MACROS========================================================================
#define MCLK_FREQ_MHZ      (8)      // MCLK = 8MHz
#define CLEAR_REGISTER     (0x0000)

void Init_Clocks(void);
void Software_Trim(void);

//==============================================================================
// Function: Init_Clocks
// Description: Initializes all system clocks.
//
//              ACLK  = XT1CLK = 32768 Hz
//              MCLK  = DCOCLKDIV = 8 MHz
//              SMCLK = MCLK / 16 = 500 kHz changed from 8MHz for HW05
//
//              Disables watchdog timer.
//              Disables GPIO power-on high-impedance mode after configuration.
//
// Passed:    none
// Returns:   none
// Globals:   none
//==============================================================================
void Init_Clocks(void){

    WDTCTL = WDTPW | WDTHOLD;      // Disable watchdog timer

    do{
        CSCTL7 &= ~XT1OFFG;         // Clear XT1 fault flag
        CSCTL7 &= ~DCOFFG;          // Clear DCO fault flag
        SFRIFG1 &= ~OFIFG;
    } while (SFRIFG1 & OFIFG);      // Test oscillator fault flag

    __bis_SR_register(SCG0);        // Disable FLL

    CSCTL1 = DCOFTRIMEN_1;
    CSCTL1 |= DCOFTRIM0;
    CSCTL1 |= DCOFTRIM1;            // DCOFTRIM = 3
    CSCTL1 |= DCORSEL_3;            // DCO Range = 8MHz

    CSCTL2 = FLLD_0 + 243;          // DCODIV = 8MHz

    CSCTL3 |= SELREF__XT1CLK;       // Set XT1CLK as FLL reference source
    __delay_cycles(3);
    __bic_SR_register(SCG0);        // Enable FLL
    Software_Trim();                // Software trim for best DCOFTRIM value

    CSCTL4 = SELA__XT1CLK;          // ACLK = XT1CLK = 32768 Hz
    CSCTL4 |= SELMS__DCOCLKDIV;     // DCOCLKDIV sources both MCLK and SMCLK

    CSCTL5 |= DIVM__1;              // MCLK  = DCOCLKDIV / 1  = 8 MHz
    CSCTL5 |= DIVS__16;             // SMCLK = MCLK     / 16  = 500 kHz
                                    // *** HW05: changed from DIVS__1 (8MHz)
                                    // to DIVS__16 (500kHz) for scope demo ***
                                    // TB0CCR0_INTERVAL updated to 2500 in macros.h

    PM5CTL0 &= ~LOCKLPM5;           // Disable GPIO power-on high-impedance mode
                                    // Activates previously configured port settings
}

//==============================================================================
// Function: Software_Trim
// Description: Texas Instruments provided software trim routine.
//              Finds the optimal DCOFTRIM value by searching around DCOTAP = 256.
//              Called once during Init_Clocks after FLL is re-enabled.
//
// Passed:    none
// Returns:   none
// Globals:   none
// Source:    TI BSD_EX Copyright (c) 2014, Texas Instruments Incorporated
//==============================================================================
void Software_Trim(void){
    unsigned int oldDcoTap = 0xffff;
    unsigned int newDcoTap = 0xffff;
    unsigned int newDcoDelta = 0xffff;
    unsigned int bestDcoDelta = 0xffff;
    unsigned int csCtl0Copy = 0;
    unsigned int csCtl1Copy = 0;
    unsigned int csCtl1Read = 0;
    unsigned int csCtl0Read = 0;
    unsigned int dcoFreqTrim = 3;
    unsigned char endLoop = 0;

    do{
        CSCTL0 = 0x100;                         // DCO Tap = 256
        do{
            CSCTL7 &= ~DCOFFG;                  // Clear DCO fault flag
        }while(CSCTL7 & DCOFFG);                // Test DCO fault flag
        // Wait FLL lock status (FLLUNLOCK) to be stable
        // Suggest to wait 24 cycles of divided FLL reference clock
        __delay_cycles((unsigned int)3000 * MCLK_FREQ_MHZ);
        while((CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1)) &&
             ((CSCTL7 & DCOFFG) == 0));
        csCtl0Read = CSCTL0;                    // Read CSCTL0
        csCtl1Read = CSCTL1;                    // Read CSCTL1
        oldDcoTap = newDcoTap;                  // Record previous DCOTAP value
        newDcoTap = csCtl0Read & 0x01ff;        // Get current DCOTAP value
        dcoFreqTrim = (csCtl1Read & 0x0070)>>4; // Get DCOFTRIM value
        if(newDcoTap < 256){                    // DCOTAP < 256
            newDcoDelta = 256 - newDcoTap;
            if((oldDcoTap != 0xffff) &&
               (oldDcoTap >= 256)){             // DCOTAP crossed 256
                endLoop = 1;
            }else{
                dcoFreqTrim--;
                CSCTL1 = (csCtl1Read & (~DCOFTRIM)) | (dcoFreqTrim<<4);
            }
        }else{                                  // DCOTAP >= 256
            newDcoDelta = newDcoTap - 256;
            if(oldDcoTap < 256){                // DCOTAP crossed 256
                endLoop = 1;
            }else{
                dcoFreqTrim++;
                CSCTL1 = (csCtl1Read & (~DCOFTRIM)) | (dcoFreqTrim<<4);
            }
        }
        if(newDcoDelta < bestDcoDelta){         // Record DCOTAP closest to 256
            csCtl0Copy = csCtl0Read;
            csCtl1Copy = csCtl1Read;
            bestDcoDelta = newDcoDelta;
        }
    }while(endLoop == 0);                       // Poll until endLoop == 1

    CSCTL0 = csCtl0Copy;                        // Reload locked DCOTAP
    CSCTL1 = csCtl1Copy;                        // Reload locked DCOFTRIM
    while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1));  // Poll until FLL is locked
}
