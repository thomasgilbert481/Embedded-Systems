//==============================================================================
// File: adc.c
// Description: 12-bit ADC sequencer for Project 9 Part 2.  Cycles through
//              A2 (V_DETECT_L), A3 (V_DETECT_R), A5 (V_THUMB) continuously.
//              Ported from Project 7.
//==============================================================================

#include "msp430.h"
#include "macros.h"
#include "ports.h"
#include "functions.h"
#include "adc.h"

volatile unsigned int ADC_Left_Detect  = 0;
volatile unsigned int ADC_Right_Detect = 0;
volatile unsigned int ADC_Thumb        = 0;

static volatile unsigned int adc_channel = 0;  // Which channel we just read

volatile unsigned int ir_emitter_on = 0;

#define ADC_SEQ_LEFT   (0)
#define ADC_SEQ_RIGHT  (1)
#define ADC_SEQ_THUMB  (2)

void Init_ADC(void){
    ADCCTL0 = 0;
    ADCCTL0 |= ADCSHT_2;
    ADCCTL0 |= ADCMSC;
    ADCCTL0 |= ADCON;

    ADCCTL1 = 0;
    ADCCTL1 |= ADCSHS_0;
    ADCCTL1 |= ADCSHP;

    ADCCTL2 = 0;
    ADCCTL2 |= ADCRES_2;               // 12-bit

    ADCMCTL0 = 0;
    ADCMCTL0 |= ADCINCH_2;              // Start with A2
    ADCMCTL0 |= ADCSREF_0;              // AVCC / AVSS

    ADCIE |= ADCIE0;                    // Conversion-complete IRQ

    ADCCTL0 |= ADCENC;
    ADCCTL0 |= ADCSC;                   // Kick off first conversion
}

#pragma vector = ADC_VECTOR
__interrupt void ADC_ISR(void){
    switch(__even_in_range(ADCIV, ADCIV_ADCIFG)){
        case ADCIV_ADCIFG:
            ADCCTL0 &= ~ADCENC;
            switch(adc_channel++){
                case ADC_SEQ_LEFT:
                    ADC_Left_Detect = ADCMEM0;
                    ADCMCTL0 &= ~ADCINCH_15;
                    ADCMCTL0 |=  ADCINCH_3;
                    break;
                case ADC_SEQ_RIGHT:
                    ADC_Right_Detect = ADCMEM0;
                    ADCMCTL0 &= ~ADCINCH_15;
                    ADCMCTL0 |=  ADCINCH_5;
                    break;
                case ADC_SEQ_THUMB:
                    ADC_Thumb = ADCMEM0;
                    ADCMCTL0 &= ~ADCINCH_15;
                    ADCMCTL0 |=  ADCINCH_2;
                    adc_channel = 0;
                    break;
                default:
                    adc_channel = 0;
                    break;
            }
            ADCCTL0 |= ADCENC;
            ADCCTL0 |= ADCSC;
            break;
        default:
            break;
    }
}
