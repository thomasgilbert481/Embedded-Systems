//******************************************************************************
// File Name:    dac.c
// Description:  DAC initialization for Project DAC -- Motor Power Demo.
//               Configures the internal 2.5V reference (PMM module) and
//               SAC3 in 12-bit DAC buffer mode to generate an analog output
//               on P3.5 (OA1O).  The external DAC board amplifies this signal
//               to drive the motors.
//
//               Key hardware:
//                 SAC3DAT  -- 12-bit DAC data register (0x000 = 0V, 0xFFF = Vref)
//                 P3.5     -- DAC analog output (enabled via P3SELC)
//                 P2.5     -- DAC_ENB: HIGH enables DAC board, LOW disables it
//
// Safety notes:
//   - Init_REF() contains a blocking busy-wait on REFGENRDY.  Call it before
//     interrupts are enabled so the wait cannot be preempted unexpectedly.
//   - Init_DAC() explicitly drives DAC_ENB LOW after configuring the hardware
//     to prevent an uncontrolled motor surge at startup.  The demo state machine
//     (Run_DAC_Demo in main.c) enables DAC_ENB only after the settling delay.
//   - DAC_data is declared volatile because it is written by the CCR0 ISR and
//     read by the main loop; both accesses are 16-bit and therefore atomic on
//     the MSP430, but volatile ensures the compiler does not cache the value.
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
// Global variable
//==============================================================================
volatile unsigned int DAC_data;  // Current 12-bit DAC code (0–4095)
                                 // Written by CCR0 ISR, read by main loop

//==============================================================================
// Function: Init_REF
// Description: Unlock the PMM registers, enable the internal reference module,
//              select the 2.5V output, and block until the reference is stable.
//
//              CALL BEFORE Init_Ports() / enable_interrupts() so the blocking
//              while loop completes before any ISR can preempt it.
//
// Globals used:    none
// Globals changed: none (PMM hardware registers only)
// Local variables: none
//==============================================================================
void Init_REF(void){
//------------------------------------------------------------------------------

    PMMCTL0_H = PMMPW_H;            // Unlock PMM registers (password byte)
    PMMCTL2   = INTREFEN;           // Enable internal reference module
    PMMCTL2  |= REFVSEL_2;          // Select 2.5V reference output

    // Poll until the reference generator signals ready (~30 µs typical).
    // TI recommends this exact pattern; see MSP430FR2355 data sheet.
    // Note: no exit strategy -- acceptable per instructor guidance and TI docs.
    while(!(PMMCTL2 & REFGENRDY));  // Wait for REFGENRDY flag

//------------------------------------------------------------------------------
}

//==============================================================================
// Function: Init_DAC
// Description: Configure SAC3 as a 12-bit DAC in op-amp buffer mode using the
//              internal 2.5V reference.  The op-amp buffers the DAC output to
//              improve drive strength on P3.5.
//
//              Initialization order (must follow this sequence):
//                1. Load DAC data register (SAC3DAT)
//                2. Configure DAC control (SAC3DAC) -- reference, latch mode
//                3. Configure OA control (SAC3OA)   -- MUX, source selection
//                4. Set PGA mode (SAC3PGA)           -- buffer
//                5. Enable DAC (DACEN), then SAC (SACEN), then OA (OAEN)
//                6. Configure P3.5 for analog output via P3SELC
//                7. Assert DAC_ENB LOW to keep DAC board disabled at startup
//
// Globals used:    DAC_data
// Globals changed: DAC_data (set to DAC_INIT_VALUE)
// Local variables: none
//==============================================================================
void Init_DAC(void){
//------------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    // 1. Pre-load DAC register with starting value
    // -------------------------------------------------------------------------
    DAC_data = DAC_INIT_VALUE;       // 4000 out of 4096 -- near full scale
    SAC3DAT  = DAC_data;             // Write to hardware DAC register

    // -------------------------------------------------------------------------
    // 2. DAC control: reference source and latch mode
    // -------------------------------------------------------------------------
    SAC3DAC  = DACSREF_1;            // Internal Vref (2.5V) as DAC reference
    SAC3DAC |= DACLSEL_0;            // Latch DAC data when SAC3DAT is written
    SAC3DAC |= DACEN;                // Enable the 12-bit DAC core

    // -------------------------------------------------------------------------
    // 3. Op-amp control: configure MUX and signal routing
    // -------------------------------------------------------------------------
    SAC3OA   = NMUXEN;               // Enable SAC negative input MUX
    SAC3OA  |= PMUXEN;               // Enable SAC positive input MUX
    SAC3OA  |= PSEL_1;               // Positive input = 12-bit DAC output
    SAC3OA  |= NSEL_1;               // Negative input = OA output (feedback)
    SAC3OA  |= OAPM;                 // Low speed / low power OA mode

    // -------------------------------------------------------------------------
    // 4. PGA mode: unity-gain buffer (DAC output follows OA output)
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
    P3OUT   &= ~DAC_CNTL;            // Ensure pin is driven low
    P3DIR   &= ~DAC_CNTL;            // Direction: input (DAC peripheral drives it)
    P3SELC  |=  DAC_CNTL;            // Enable analog function on P3.5 (OA1O)

    // -------------------------------------------------------------------------
    // 7. Keep DAC board disabled at startup
    //    ports.c initializes DAC_ENB (P2.5) HIGH, which would enable the board
    //    immediately and cause an uncontrolled motor surge.  Override that here.
    //    Run_DAC_Demo() will set DAC_ENB HIGH after the startup settling delay.
    // -------------------------------------------------------------------------
    P2OUT   &= ~DAC_ENB;             // DAC board OFF -- enabled later by demo SM

//------------------------------------------------------------------------------
}
