//==============================================================================
// File:        dac.c
// Description: DAC initialization -- Motor Power via SAC3 (Project 9 Part 2).
//              Ported verbatim from Project 7.
//
//              Configures the SAC3 12-bit DAC in op-amp buffer mode using VCC
//              as the reference (DACSREF_0).  The DAC output on P3.5 feeds the
//              FB_DAC pin of the LT1935 Buck-Boost converter, which provides
//              the motor supply voltage.
//
//              IMPORTANT -- output is INVERTED:
//                Lower SAC3DAT value  -->  higher buck-boost output voltage
//                Higher SAC3DAT value -->  lower buck-boost output voltage
//
//              Startup sequence:
//                1. Init_DAC() sets SAC3DAT = DAC_Begin (1500) -- safe start
//                2. Init_DAC() enables Timer B0 overflow interrupt (TBIE)
//                3. Phase 1 -- overflow ISR counts DAC_ENABLE_TICKS (3) ticks:
//                   After 3 ticks (~1.6s), ISR enables DAC_ENB and RED LED ON
//                4. Phase 2 -- ISR decrements DAC_data by DAC_RAMP_STEP (50)
//                   each tick until DAC_data <= DAC_Limit (1200, ~6V)
//                5. ISR sets DAC_Adjust, clears TBIE, turns RED LED OFF
//                6. Motor supply is now ~6V; PWM controls direction/speed
//
//              P3.5 (DAC_CNTL) is switched to analog mode via P3SELC.
//              P2.5 (DAC_ENB) starts LOW (from Init_Port2); overflow ISR
//              drives it HIGH at the end of Phase 1.
//
// Author: Thomas Gilbert
// Date: Mar 2026
// Compiler: Code Composer Studio
// Target: MSP430FR2355
//==============================================================================

#include "msp430.h"
#include "functions.h"
#include "macros.h"
#include "ports.h"

//==============================================================================
// Global variables
//==============================================================================
volatile unsigned int DAC_data;          // Current 12-bit DAC code (0-4095)
volatile unsigned int dac_startup_ticks; // Overflow tick counter for DAC_ENB

//==============================================================================
// Function: Init_DAC
//==============================================================================
void Init_DAC(void){

    // 1. Pre-load DAC register with safe startup value.
    DAC_data = DAC_Begin;
    SAC3DAT  = DAC_data;

    // 2. DAC control: VCC reference and latch mode.
    SAC3DAC  = DACSREF_0;
    SAC3DAC |= DACLSEL_0;

    // 3. Op-amp control: configure MUX and signal routing.
    SAC3OA   = NMUXEN;
    SAC3OA  |= PMUXEN;
    SAC3OA  |= PSEL_1;               // + input = 12-bit DAC output
    SAC3OA  |= NSEL_1;               // - input = OA output (feedback)
    SAC3OA  |= OAPM;                 // Low-power OA mode

    // 4. PGA mode: unity-gain buffer.
    SAC3PGA  = MSEL_1;

    // 5. Enable SAC and OA (order: SACEN first, then OAEN).
    SAC3OA  |= SACEN;
    SAC3OA  |= OAEN;

    // 6. Route P3.5 (DAC_CNTL) to analog function.
    P3OUT  &= ~DAC_CNTL;
    P3DIR  &= ~DAC_CNTL;
    P3SELC |=  DAC_CNTL;

    // 7. Enable the DAC core last.
    SAC3DAC |= DACEN;

    // 8. Start two-phase ramp sequence via Timer B0 overflow interrupt.
    dac_startup_ticks = 0;
    TB0CTL  |= TBIE;
}
