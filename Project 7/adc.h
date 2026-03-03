//==============================================================================
// File Name: adc.h
// Description: ADC function prototypes and extern global variable declarations
//              for Project 6 -- Black Line Detection & ADC Integration.
//
//              Provides access to:
//                - ADC result variables (Left/Right detectors, Thumbwheel)
//                - BCD digit output from HexToBCD()
//                - IR emitter state
//
// Author: Thomas Gilbert
// Date: Mar 2026
// Compiler: Code Composer Studio
//==============================================================================

#ifndef ADC_H_
#define ADC_H_

//==============================================================================
// ADC Result Globals (defined in adc.c, extern'd here for use in other modules)
//==============================================================================
extern volatile unsigned int ADC_Left_Detect;   // Channel A2 -- Left IR detector
extern volatile unsigned int ADC_Right_Detect;  // Channel A3 -- Right IR detector
extern volatile unsigned int ADC_Thumb;          // Channel A5 -- Thumbwheel potentiometer

//==============================================================================
// BCD Digit Output (populated by HexToBCD(), used to build LCD display strings)
//==============================================================================
extern char thousands;  // Thousands digit (ASCII)
extern char hundreds;   // Hundreds digit  (ASCII)
extern char tens;       // Tens digit      (ASCII)
extern char ones;       // Ones digit      (ASCII)

//==============================================================================
// IR Emitter State (0 = OFF, 1 = ON)
//==============================================================================
extern unsigned int ir_emitter_on;

//==============================================================================
// Function Prototypes
//==============================================================================

// Initialize the 12-bit ADC (ADCCTL0/1/2, ADCMCTL0, interrupt enable)
void Init_ADC(void);

// Convert a 12-bit integer (0-4095) to four ASCII BCD digits stored in
// the globals: thousands, hundreds, tens, ones
void HexToBCD(int hex_value);

#endif /* ADC_H_ */
