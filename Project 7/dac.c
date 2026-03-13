//==============================================================================
// File Name: dac.c
// Description: DAC initialization for motor voltage control via SAC3.
//              The DAC output feeds the Wheels Power System switching regulator.
//              Without this, motors will not run in Project 7+.
//
//              Uses AVCC as DAC reference (DACSREF_0).
//              Initial DAC_data = 1500 (low voltage, safe startup).
//              The DAC_ENB pin (P2.5) must be asserted HIGH after a short
//              timer delay to enable the power board output.
//              The DAC value is then ramped up to the target motor voltage
//              (approximately 1200 for ~6V -- TUNE ON HARDWARE).
//
// Author: Thomas Gilbert
// Date: Mar 2026
//==============================================================================

#include "msp430.h"
#include "functions.h"
#include "macros.h"
#include "ports.h"

unsigned int DAC_data = DAC_INITIAL_VALUE;

//==============================================================================
// Function: Init_DAC
// Description: Configure SAC3 as a DAC in buffer mode.
//              P3.5 (DAC_CNTL) must be set to DAC function via P3SELC.
//              DAC_ENB (P2.5) is NOT enabled here -- that happens later
//              in a timer overflow interrupt after 2-3 overflows to allow
//              the DAC output to stabilize.
//==============================================================================
void Init_DAC(void){

    DAC_data  = DAC_INITIAL_VALUE;         // Safe startup value
    SAC3DAT   = DAC_data;                  // Load initial DAC data

    SAC3DAC   = DACSREF_0;                 // Select AVCC as DAC reference
    SAC3DAC  |= DACLSEL_0;                 // DAC latch loads when DACDAT written

    SAC3OA    = NMUXEN;                    // SAC Negative input MUX control
    SAC3OA   |= PMUXEN;                    // SAC Positive input MUX control
    SAC3OA   |= PSEL_1;                    // 12-bit reference DAC source selected
    SAC3OA   |= NSEL_1;                    // Select negative pin input
    SAC3OA   |= OAPM;                      // Select low speed and low power mode
    SAC3PGA   = MSEL_1;                    // Set OA as buffer mode
    SAC3OA   |= SACEN;                     // Enable SAC
    SAC3OA   |= OAEN;                      // Enable OA

    // Configure P3.5 (DAC_CNTL) for DAC output function
    P3OUT    &= ~DAC_CNTL;                 // Set output to Low
    P3DIR    &= ~DAC_CNTL;                 // Set direction to input
    P3SELC   |=  DAC_CNTL;                 // DAC_CNTL DAC operation

    SAC3DAC  |=  DACEN;                    // Enable DAC
}
