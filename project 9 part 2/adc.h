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

// Set by the ADC ISR whenever a full L/R/Thumb sweep completes.  The PD
// line-follow loop reads this flag to gate its update so d_error is always
// computed on a fresh sample pair rather than a re-read of stale values.
// Consumer clears it after reading.
extern volatile unsigned char ADC_sample_ready;

void Init_ADC(void);

#endif /* ADC_H_ */
