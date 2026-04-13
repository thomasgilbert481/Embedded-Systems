//==============================================================================
// File:        dac.c
// Description: Simplified DAC setup for Project 9 Part 2 motor power.
//
//   The SAC3 12-bit DAC on P3.5 feeds the FB_DAC pin of the LT1935 buck-boost
//   converter, which generates the motor supply rail.  Output is INVERTED:
//     lower SAC3DAT  -->  higher motor voltage
//     higher SAC3DAT -->  lower motor voltage
//
//   Project 7 starts at DAC_Begin (low voltage) and ramps down to DAC_Adjust
//   (~6V) over several Timer B0 overflows.  For P9P2 we don't need the ramp;
//   we just program DAC_Adjust directly so the motor rail comes up as soon
//   as Init_DAC returns.
//
//   Call order (main.c):
//     Init_Ports();     // DAC_ENB (P2.5) already driven HIGH in Init_Port2
//     Init_Clocks();
//     Init_Conditions();
//     Init_Timers();
//     Init_LCD();
//     Init_DAC();       // <-- adds ~6V motor rail
//     Init_Serial_*;
//
// Author: Thomas Gilbert
// Date: Mar 2026
// Compiler: Code Composer Studio
// Target: MSP430FR2355
//==============================================================================

#include "msp430.h"
#include "macros.h"
#include "ports.h"
#include "functions.h"

//==============================================================================
// DAC operating point -- chosen empirically by instructor; ~6V motor supply.
// "Inverted" output: lower code = higher motor voltage.
//==============================================================================
#define DAC_OPERATING_CODE  (1200)

void Init_DAC(void){

    // 1. Pre-load DAC register with the final operating code.
    SAC3DAT  = DAC_OPERATING_CODE;

    // 2. DAC control: VCC reference, latch immediately on SAC3DAT write.
    SAC3DAC  = DACSREF_0;
    SAC3DAC |= DACLSEL_0;

    // 3. Op-amp routing: DAC -> positive input, OA output fed back to negative.
    SAC3OA   = NMUXEN;
    SAC3OA  |= PMUXEN;
    SAC3OA  |= PSEL_1;          // + input from 12-bit DAC
    SAC3OA  |= NSEL_1;          // - input from OA output (unity gain)
    SAC3OA  |= OAPM;            // Low-power OA mode

    // 4. Unity-gain buffer.
    SAC3PGA  = MSEL_1;

    // 5. Enable the SAC, then the op-amp.
    SAC3OA  |= SACEN;
    SAC3OA  |= OAEN;

    // 6. Route P3.5 (DAC_CNTL) to analog function.
    P3OUT  &= ~DAC_CNTL;
    P3DIR  &= ~DAC_CNTL;
    P3SELC |=  DAC_CNTL;

    // 7. Enable the DAC core last.
    SAC3DAC |= DACEN;

    // Buck-boost reference voltage is now settled.  DAC_ENB (P2.5) is already
    // HIGH (set by Init_Port2), so the motor supply rail is live.
}
