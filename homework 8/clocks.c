// ------------------------------------------------------------------------------
//
//  Description: This file contains the Clock Initialization
//
//  Jim Carlson
//  Jan 2021
//  Built with IAR Embedded Workbench Version: V7.12.1
// ------------------------------------------------------------------------------
#include "msp430.h"
#include "functions.h"
#include "LCD.h"
#include "ports.h"
#include "macros.h"


// MACROS========================================================================
#define MCLK_FREQ_MHZ           (8) // MCLK = 8MHz
#define CLEAR_REGISTER     (0X0000)

void Init_Clocks(void);
void Software_Trim(void);

void Init_Clocks(void){
// -----------------------------------------------------------------------------
// Clock Configurations
// Configure ACLK = 32768Hz,
//           MCLK = DCO + XT1CLK REF = 8MHz,
//           SMCLK = MCLK = 8MHz.
// -----------------------------------------------------------------------------
  WDTCTL = WDTPW | WDTHOLD;  // Disable watchdog

  do{
    CSCTL7 &= ~XT1OFFG;      // Clear XT1 fault flag
    CSCTL7 &= ~DCOFFG;       // Clear DCO fault flag
    SFRIFG1 &= ~OFIFG;
  } while (SFRIFG1 & OFIFG); // Test oscillator fault flag
  __bis_SR_register(SCG0);   // disable FLL

  CSCTL1 = DCOFTRIMEN_1;
  CSCTL1 |= DCOFTRIM0;
  CSCTL1 |= DCOFTRIM1;       // DCOFTRIM=3
  CSCTL1 |= DCORSEL_3;       // DCO Range = 8MHz

  CSCTL2 = FLLD_0 + 243;     // DCODIV = 8MHz

  CSCTL3 |= SELREF__XT1CLK;  // Set XT1CLK as FLL reference source
  __delay_cycles(3);
  __bic_SR_register(SCG0);   // enable FLL
  Software_Trim();            // Software Trim to get the best DCOFTRIM value

  CSCTL4 = SELA__XT1CLK;     // Set ACLK = XT1CLK = 32768Hz
  CSCTL4 |= SELMS__DCOCLKDIV;// DCOCLK = MCLK and SMCLK source

  CSCTL5 |= DIVM__1;         // MCLK = DCOCLK = 8MHz
  CSCTL5 |= DIVS__1;         // SMCLK = MCLK = 8MHz

  PM5CTL0 &= ~LOCKLPM5;      // Unlock GPIO
}

void Software_Trim(void){
  unsigned int oldDcoTap = 0xffff;
  unsigned int newDcoTap = 0xffff;
  unsigned int newDcoDelta = 0xffff;
  unsigned int bestDcoDelta = 0xffff;
  unsigned int csCtl0Copy = 0;
  unsigned int csCtl1Copy = 0;
  unsigned int csCtl0Read = 0;
  unsigned int csCtl1Read = 0;
  unsigned int dcoFreqTrim = 3;
  unsigned char endLoop = 0;
  do{
    CSCTL0 = 0x100;
    do{
      CSCTL7 &= ~DCOFFG;
    }while (CSCTL7 & DCOFFG);
    __delay_cycles((unsigned int)3000 * MCLK_FREQ_MHZ);
    while((CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1)) &&
         ((CSCTL7 & DCOFFG) == 0));
    csCtl0Read = CSCTL0;
    csCtl1Read = CSCTL1;
    oldDcoTap = newDcoTap;
    newDcoTap = csCtl0Read & 0x01ff;
    dcoFreqTrim = (csCtl1Read & 0x0070)>>4;
    if(newDcoTap < 256){
      newDcoDelta = 256 - newDcoTap;
      if((oldDcoTap != 0xffff) && (oldDcoTap >= 256)){
        endLoop = 1;
      }else{
        dcoFreqTrim--;
        CSCTL1 = (csCtl1Read & (~DCOFTRIM)) | (dcoFreqTrim<<4);
      }
    }else{
      newDcoDelta = newDcoTap - 256;
      if(oldDcoTap < 256){
        endLoop = 1;
      }else{
        dcoFreqTrim++;
        CSCTL1 = (csCtl1Read & (~DCOFTRIM)) | (dcoFreqTrim<<4);
      }
    }
    if(newDcoDelta < bestDcoDelta){
      csCtl0Copy = csCtl0Read;
      csCtl1Copy = csCtl1Read;
      bestDcoDelta = newDcoDelta;
    }
  }while(endLoop == 0);
  CSCTL0 = csCtl0Copy;
  CSCTL1 = csCtl1Copy;
  while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1));
}
