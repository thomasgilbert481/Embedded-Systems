//******************************************************************************
// File Name:    dac.c
// Description:  DAC initialization for Project 7 -- Motor Power via SAC3.
//               Configures the SAC3 12-bit DAC in op-amp buffer mode using VCC
//               as the reference (DACSREF_0).  The DAC output on P3.5 feeds the
//               FB_DAC pin of the LT1935 Buck-Boost converter, which provides
//               the motor supply voltage.
//
//               IMPORTANT -- output is INVERTED:
//                 Lower SAC3DAT value  -->  higher buck-boost output voltage
//                 Higher SAC3DAT value -->  lower buck-boost output voltage
//
//               Startup sequence (per instructor guide):
//                 1. Init_DAC() sets SAC3DAT = DAC_Begin (1500) -- safe start
//                 2. Init_DAC() enables Timer B0 overflow interrupt (TBIE)
//                 3. Phase 1 -- overflow ISR counts DAC_ENABLE_TICKS (3) ticks:
//                    After 3 ticks (~1.6s), ISR enables DAC_ENB and RED LED ON
//                 4. Phase 2 -- ISR decrements DAC_data by DAC_RAMP_STEP (50)
//                    each tick until DAC_data <= DAC_Limit (1200, ~6V)
//                 5. ISR sets DAC_Adjust, clears TBIE, turns RED LED OFF
//                 6. Motor supply is now ~6V; PWM controls direction/speed
//
//               P3.5 (DAC_CNTL) is switched to analog mode via P3SELC.
//               P2.5 (DAC_ENB) is left HIGH from ports.c -- buck-boost always on.
//
// Author:       Thomas Gilbert
// Date:         Mar 2026
// Compiler:     Code Composer Studio
// Target:       MSP430FR2355
//******************************************************************************

#include "msp430.h"
#include "functions.h"
#include "macros.h"
#include "ports.h"

//==============================================================================
// Global variables
//==============================================================================
volatile unsigned int DAC_data;          // Current 12-bit DAC code (0-4095)
                                         // Written by overflow ISR and Init_DAC
volatile unsigned int dac_startup_ticks; // Overflow tick counter for DAC_ENB
                                         // settling delay (counts up to DAC_ENABLE_TICKS)

//==============================================================================
// Function: Init_DAC
// Description: Configure SAC3 as a 12-bit DAC in op-amp buffer mode using VCC
//              as the reference voltage.  Starts the automatic ramp-down
//              sequence by enabling the Timer B0 overflow interrupt.
//
//              Init_Timers() MUST be called before Init_DAC() because this
//              function enables TB0CTL TBIE -- Timer B0 must already be running.
//
//              Initialization order (must follow this sequence):
//                1. Set initial DAC register to safe startup value (DAC_Begin)
//                2. Configure DAC control (SAC3DAC) -- VCC reference, latch mode
//                3. Configure OA control (SAC3OA)   -- MUX, source selection
//                4. Set PGA mode (SAC3PGA)           -- buffer
//                5. Enable SAC (SACEN), then OA (OAEN)
//                6. Configure P3.5 for analog output via P3SELC
//                7. Enable DACEN last
//                8. Enable Timer B0 overflow interrupt to start the ramp
//
// Globals used:    DAC_data
// Globals changed: DAC_data (set to DAC_Begin)
// Local variables: none
//==============================================================================
void Init_DAC(void){
//------------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    // 1. Pre-load DAC register with safe startup value
    //    DAC_Begin = 2725 --> ~2.0V output --> motor supply too low to spin
    // -------------------------------------------------------------------------
    DAC_data = DAC_Begin;            // Safe starting point
    SAC3DAT  = DAC_data;             // Write to hardware DAC register

    // -------------------------------------------------------------------------
    // 2. DAC control: VCC reference and latch mode
    //    DACSREF_0: VCC as reference (NOT the internal 2.5V ref)
    //    DACLSEL_0: Latch DAC data immediately when SAC3DAT is written
    // -------------------------------------------------------------------------
    SAC3DAC  = DACSREF_0;            // VCC as DAC reference (required for buck-boost)
    SAC3DAC |= DACLSEL_0;            // Latch DAC data when SAC3DAT is written

    // -------------------------------------------------------------------------
    // 3. Op-amp control: configure MUX and signal routing
    // -------------------------------------------------------------------------
    SAC3OA   = NMUXEN;               // Enable SAC negative input MUX
    SAC3OA  |= PMUXEN;               // Enable SAC positive input MUX
    SAC3OA  |= PSEL_1;               // Positive input = 12-bit DAC output
    SAC3OA  |= NSEL_1;               // Negative input = OA output (feedback)
    SAC3OA  |= OAPM;                 // Low speed / low power OA mode

    // -------------------------------------------------------------------------
    // 4. PGA mode: unity-gain buffer
    // -------------------------------------------------------------------------
    SAC3PGA  = MSEL_1;               // Set OA as buffer (unity gain)

    // -------------------------------------------------------------------------
    // 5. Enable SAC and OA (order: SACEN first, then OAEN)
    // -------------------------------------------------------------------------
    SAC3OA  |= SACEN;                // Enable Smart Analog Combo module
    SAC3OA  |= OAEN;                 // Enable operational amplifier

    // -------------------------------------------------------------------------
    // 6. Configure P3.5 as DAC analog output
    //    P3SEL0 and P3SEL1 are already cleared in ports.c (GPIO mode).
    //    Setting P3SELC selects the analog/DAC function on this pin.
    // -------------------------------------------------------------------------
    P3OUT   &= ~DAC_CNTL;            // Ensure pin output register is low
    P3DIR   &= ~DAC_CNTL;            // Direction: input (DAC peripheral drives it)
    P3SELC  |=  DAC_CNTL;            // Enable analog function on P3.5

    // -------------------------------------------------------------------------
    // 7. Enable the DAC core last
    // -------------------------------------------------------------------------
    SAC3DAC |= DACEN;                // Enable 12-bit DAC core

    // -------------------------------------------------------------------------
    // 8. Start two-phase ramp sequence via Timer B0 overflow interrupt
    //
    //    Phase 1 (DAC_ENABLE_TICKS ticks, ~1.6s):
    //      dac_startup_ticks increments each overflow.  When it reaches
    //      DAC_ENABLE_TICKS, the ISR sets P2OUT |= DAC_ENB and turns RED LED ON.
    //
    //    Phase 2 (ramp ticks):
    //      Each overflow: DAC_data -= DAC_RAMP_STEP (50), until DAC_data <=
    //      DAC_Limit.  Then DAC_Adjust is written, TBIE is cleared, RED LED OFF.
    //
    //    P2OUT DAC_ENB is kept LOW (disabled) by ports.c; ISR enables it.
    // -------------------------------------------------------------------------
    dac_startup_ticks = 0;           // Reset startup counter
    TB0CTL  |= TBIE;                 // Enable Timer B0 overflow interrupt

//------------------------------------------------------------------------------
}
