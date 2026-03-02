//==============================================================================
// File Name: adc.c
// Description: ADC initialization, interrupt service routine, and BCD
//              conversion for Project 6 — Black Line Detection.
//
//              The ADC cycles through three channels every conversion:
//                Channel A2 (P1.2, V_DETECT_L) — Left IR phototransistor
//                Channel A3 (P1.3, V_DETECT_R) — Right IR phototransistor
//                Channel A5 (P1.5, V_THUMB)    — Thumbwheel potentiometer
//
//              IR detector behavior (with emitter ON):
//                White surface → high IR reflection → lower ADC reading
//                Black surface → low  IR reflection → higher ADC reading
//
// Author: Thomas Gilbert
// Date: Mar 2026
// Compiler: Code Composer Studio
//==============================================================================

#include "msp430.h"
#include "functions.h"
#include "macros.h"
#include "ports.h"
#include "adc.h"

//==============================================================================
// Global ADC Result Variables
//==============================================================================
volatile unsigned int ADC_Left_Detect  = 0; // A2 — Left IR detector reading
volatile unsigned int ADC_Right_Detect = 0; // A3 — Right IR detector reading
volatile unsigned int ADC_Thumb        = 0; // A5 — Thumbwheel potentiometer reading

// Channel counter: tracks which channel the NEXT ISR read belongs to
//   0 → just completed A2 conversion (Left Detect)
//   1 → just completed A3 conversion (Right Detect)
//   2 → just completed A5 conversion (Thumbwheel)
volatile unsigned int adc_channel = 0;

//==============================================================================
// BCD Digit Output (ASCII) — populated by HexToBCD()
//==============================================================================
char thousands = '0';
char hundreds  = '0';
char tens      = '0';
char ones      = '0';

//==============================================================================
// IR Emitter State (0 = OFF, 1 = ON)
//==============================================================================
unsigned int ir_emitter_on = 0;

//==============================================================================
// Function: Init_ADC
// Description: Configure the MSP430FR2355 12-bit ADC for sequential channel
//              scanning of Left Detector (A2), Right Detector (A3), and
//              Thumbwheel (A5).  Starts the first conversion immediately.
//
//   ADCCTL0: ADCSHT_2 (16-clock sample), ADCMSC (multiple sample), ADCON
//   ADCCTL1: ADCSHS_0 (ADCSC trigger), ADCSHP (sample timer)
//   ADCCTL2: ADCRES_2 (12-bit resolution, 0x0000–0x0FFF)
//   ADCMCTL0: ADCINCH_2 (start on A2), ADCSREF_0 (AVCC/AVSS reference)
//==============================================================================
void Init_ADC(void){

    // --- ADCCTL0 ---
    ADCCTL0 = 0;            // Reset all ADC control bits
    ADCCTL0 |= ADCSHT_2;   // Sample-and-hold time = 16 ADC clocks
    ADCCTL0 |= ADCMSC;     // Enable multiple sample and conversion
    ADCCTL0 |= ADCON;      // Turn the ADC core ON

    // --- ADCCTL1 ---
    ADCCTL1 = 0;            // Reset
    ADCCTL1 |= ADCSHS_0;   // Sample-and-hold source = ADCSC bit (software trigger)
    ADCCTL1 |= ADCSHP;     // SAMPCON sourced from the internal sampling timer

    // --- ADCCTL2 ---
    ADCCTL2 = 0;            // Reset
    ADCCTL2 |= ADCRES_2;   // 12-bit resolution (0x0000 to 0x0FFF)

    // --- ADCMCTL0: input channel and voltage reference ---
    ADCMCTL0 = 0;           // Reset
    ADCMCTL0 |= ADCINCH_2; // Start with channel A2 (V_DETECT_L — Left Detector)
    ADCMCTL0 |= ADCSREF_0; // V(R+) = AVCC, V(R-) = AVSS

    // --- Enable ADC conversion-complete interrupt ---
    ADCIE |= ADCIE0;        // ADCIFG interrupt enable

    // --- Start first conversion ---
    ADCCTL0 |= ADCENC;     // Enable conversions
    ADCCTL0 |= ADCSC;      // Start first conversion (software trigger)
}

//==============================================================================
// ISR: ADC_ISR
// Description: Fires when an ADC conversion completes (ADCIFG set).
//              Reads the result from ADCMEM0, stores it in the appropriate
//              global variable, switches to the next channel, and starts the
//              next conversion.
//
//   Cycle:  A2 → A3 → A5 → A2 → A3 → A5 → ...
//              Left    Right  Thumb
//
// NOTE: ADCENC must be cleared before changing the channel, then re-enabled.
//==============================================================================
#pragma vector = ADC_VECTOR
__interrupt void ADC_ISR(void){

    switch(__even_in_range(ADCIV, ADCIV_ADCIFG)){

        case ADCIV_NONE:
            break;

        case ADCIV_ADCOVIFG:    // ADC memory overflow — conversion result lost
            break;

        case ADCIV_ADCTOVIFG:   // ADC conversion time overflow
            break;

        case ADCIV_ADCHIIFG:    // Window comparator: above high threshold
            break;

        case ADCIV_ADCLOIFG:    // Window comparator: below low threshold
            break;

        case ADCIV_ADCINIFG:    // Window comparator: in-range
            break;

        case ADCIV_ADCIFG:      // Conversion complete — normal path
            // Disable conversions before changing the channel register
            ADCCTL0 &= ~ADCENC;

            switch(adc_channel++){
                case ADC_CHANNEL_SEQ_LEFT:    // Just read A2 — store Left Detect
                    ADC_Left_Detect  = ADCMEM0;
                    ADCMCTL0 &= ~ADCINCH_15;  // Clear channel selection bits [3:0]
                    ADCMCTL0 |=  ADCINCH_3;   // Next conversion: A3 (Right Detector)
                    break;

                case ADC_CHANNEL_SEQ_RIGHT:   // Just read A3 — store Right Detect
                    ADC_Right_Detect = ADCMEM0;
                    ADCMCTL0 &= ~ADCINCH_15;  // Clear channel selection bits [3:0]
                    ADCMCTL0 |=  ADCINCH_5;   // Next conversion: A5 (Thumbwheel)
                    break;

                case ADC_CHANNEL_SEQ_THUMB:   // Just read A5 — store Thumbwheel
                    ADC_Thumb = ADCMEM0;
                    ADCMCTL0 &= ~ADCINCH_15;  // Clear channel selection bits [3:0]
                    ADCMCTL0 |=  ADCINCH_2;   // Next conversion: A2 (Left Detector)
                    adc_channel = 0;           // Reset the channel counter
                    break;

                default:
                    adc_channel = 0;           // Recover from unexpected state
                    break;
            }

            // Re-enable and trigger the next conversion
            ADCCTL0 |= ADCENC;    // Re-enable conversions
            ADCCTL0 |= ADCSC;     // Start next conversion
            break;

        default:
            break;
    }
}

//==============================================================================
// Function: HexToBCD
// Description: Converts a 12-bit integer value (0–4095) into four decimal
//              digit characters (ASCII) stored in the globals:
//                thousands, hundreds, tens, ones
//
//              Uses repeated subtraction to avoid division (which is slow on
//              the MSP430 and may not be available in all configurations).
//
//              After the call, build an LCD string like:
//                display_line[0][2] = thousands;
//                display_line[0][3] = hundreds;
//                display_line[0][4] = tens;
//                display_line[0][5] = ones;
//
// Parameters:
//   hex_value — 12-bit ADC reading (0 to 4095)
//==============================================================================
void HexToBCD(int hex_value){

    int value = hex_value;

    // --- Thousands digit ---
    thousands = 0;
    if(value >= 1000){
        value     -= 1000;
        thousands  = 1;
    }

    // --- Hundreds digit ---
    hundreds = 0;
    while(value >= 100){
        value    -= 100;
        hundreds++;
    }

    // --- Tens digit ---
    tens = 0;
    while(value >= 10){
        value -= 10;
        tens++;
    }

    // --- Ones digit ---
    ones = value;

    // Convert raw BCD (0–9) to ASCII ('0'–'9') by OR-ing with 0x30
    thousands |= 0x30;
    hundreds  |= 0x30;
    tens      |= 0x30;
    ones      |= 0x30;
}
