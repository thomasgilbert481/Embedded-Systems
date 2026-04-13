/*
 * ADC.c
 *
 *  Created on: Mar 8, 2026
 *      Author: noah
 */


#include  "msp430.h"
#include  <string.h>
#include  "functions.h"
#include  "LCD.h"
#include  "ports.h"
#include "macros.h"
#include "globals.h"




// Global Variables
unsigned int ADC_Thumb     = 0;
unsigned int ADC_Right_Det = 0;
unsigned int ADC_Left_Det  = 0;
unsigned char ADC_Channel  = 0;
unsigned int DAC_data = 0;


//conversions
char adc_char[4];
int i;


volatile char adc_ready = FALSE;
extern char display_line[4][11];

//------------------------------------------------------------------------------
// HEXtoBCD - Converts a hex/int value to 4 BCD ASCII digits in adc_char[]
//------------------------------------------------------------------------------
void HEXtoBCD(int hex_value){
    int value = 0;
    for(i = 0; i < 4; i++){
        adc_char[i] = '0';
    }
    while(hex_value > 999){
        hex_value = hex_value - 1000;
        value = value + 1;
        adc_char[0] = 0x30 + value;
    }
    value = 0;
    while(hex_value > 99){
        hex_value = hex_value - 100;
        value = value + 1;
        adc_char[1] = 0x30 + value;
    }
    value = 0;
    while(hex_value > 9){
        hex_value = hex_value - 10;
        value = value + 1;
        adc_char[2] = 0x30 + value;
    }
    adc_char[3] = 0x30 + hex_value;
}

//------------------------------------------------------------------------------
// adc_line - Places adc_char[] onto a display line at a given location
// char line     => display line 1-4
// char location => starting position 0-9
//------------------------------------------------------------------------------
void adc_line(char line, char location){
    int i;
    unsigned int real_line;
    real_line = line - 1;
    for(i = 0; i < 4; i++){
        display_line[real_line][i + location] = adc_char[i];
    }
}


//----------------------------------------------------------------------------
// Init_ADC - Initialize ADC for single-channel, interrupt-driven conversion
//            Starting channel: A5 (Thumb Wheel)
//----------------------------------------------------------------------------
void Init_ADC(void){
    ADCCTL0 = 0;
    ADCCTL0 |= ADCSHT_2;
    ADCCTL0 |= ADCMSC;
    ADCCTL0 |= ADCON;

    ADCCTL1 = 0;
    ADCCTL1 |= ADCSHS_0;
    ADCCTL1 |= ADCSHP;
    ADCCTL1 &= ~ADCISSH;
    ADCCTL1 |= ADCDIV_0;
    ADCCTL1 |= ADCSSEL_0;
    ADCCTL1 |= ADCCONSEQ_0;

    ADCCTL2 = 0;
    ADCCTL2 |= ADCPDIV0;
    ADCCTL2 |= ADCRES_1;
    ADCCTL2 &= ~ADCDF;
    ADCCTL2 &= ~ADCSR;

    ADCMCTL0 |= ADCSREF_0;
    ADCMCTL0 |= ADCINCH_5;

    ADCIE    |= ADCIE0;
    ADCCTL0  |= ADCENC;
    ADCCTL0  |= ADCSC;   // start first conversion — must be LAST
}

//----------------------------------------------------------------------------
// ADC ISR - Cycles through A5 -> A3 -> A2 -> A5 ...
//----------------------------------------------------------------------------
#pragma vector = ADC_VECTOR
__interrupt void ADC_ISR(void) {
    switch (__even_in_range(ADCIV, ADCIV_ADCIFG)) {
        case ADCIV_NONE:
            break;
        case ADCIV_ADCOVIFG:   // Result overwritten before read
            break;
        case ADCIV_ADCTOVIFG:  // Conversion time overflow
            break;
        case ADCIV_ADCHIIFG:   // Window comparator high
            break;
        case ADCIV_ADCLOIFG:   // Window comparator low
            break;
        case ADCIV_ADCINIFG:   // Window comparator in-range
            break;

        case ADCIV_ADCIFG:     // Conversion result ready in ADCMEM0
            ADCCTL0 &= ~ADCENC;  // Disable conversions to change channel

            switch (ADC_Channel++) {
                case 0x00:  // Just converted A5 — Thumb Wheel
                    ADC_Thumb = ADCMEM0;
                    ADCMCTL0 &= ~ADCINCH_5;  // Disable A5
                    ADCMCTL0 |=  ADCINCH_3;  // Next: A3
                    break;

                    case 0x01:  // Just converted A3 — was Right, now assign to Left
                        ADC_Left_Det = ADCMEM0;
                        ADCMCTL0 &= ~ADCINCH_3;
                        ADCMCTL0 |=  ADCINCH_2;
                        break;

                    case 0x02:  // Just converted A2 — was Left, now assign to Right
                        ADC_Right_Det = ADCMEM0;
                        ADCMCTL0 &= ~ADCINCH_2;
                        ADCMCTL0 |=  ADCINCH_5;
                        ADC_Channel = 0;
                        adc_ready = TRUE;
                        break;

                default:
                    break;
            }

            ADCCTL0 |= ADCENC;   // Re-enable conversions
            ADCCTL0 |= ADCSC;    // Start next conversion
            // NOTE: Ideally ADCSC is moved to a Timer ISR to control
            // the sampling rate and turn the IR emitter on/off.
            break;

        default:
            break;
    }
}



//DAC

void Init_DAC(void){
    SAC3DAC  = DACSREF_0;
    SAC3DAC |= DACLSEL_0;
    SAC3OA   = NMUXEN;
    SAC3OA  |= PMUXEN;
    SAC3OA  |= PSEL_1;
    SAC3OA  |= NSEL_1;
    SAC3OA  |= OAPM;
    SAC3PGA  = MSEL_1;
    SAC3OA  |= SACEN;
    SAC3OA  |= OAEN;
    DAC_data  = DAC_Begin;
    SAC3DAT   = DAC_data;
    TB0CTL   |= TBIE;        // enable overflow interrupt
    RED_LED_ON;
    SAC3DAC  |= DACEN;
}
