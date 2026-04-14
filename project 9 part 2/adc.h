//==============================================================================
// File: adc.h  -- 12-bit ADC for the IR line detectors + thumbwheel.
// Ported from Project 7.
//==============================================================================

#ifndef ADC_H_
#define ADC_H_

extern volatile unsigned int ADC_Left_Detect;   // Channel A2 (P1.2)
extern volatile unsigned int ADC_Right_Detect;  // Channel A3 (P1.3)
extern volatile unsigned int ADC_Thumb;         // Channel A5 (P1.5)

extern volatile unsigned int ir_emitter_on;     // 1 = IR LED ON

void Init_ADC(void);

#endif /* ADC_H_ */
