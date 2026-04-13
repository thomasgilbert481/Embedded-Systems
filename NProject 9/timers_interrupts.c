/*******************************************************************************
 * File:        timers_interrupts.c
 * Description: Timer B0 init and ISRs for backlight blink and switch debounce
 * Author:      Noah Cartwright
 * Date:        Feb 25, 2026
 * Class:       ECE 306
 ******************************************************************************/

#include  "msp430.h"
#include  <string.h>
#include  "functions.h"
#include  "LCD.h"
#include  "ports.h"
#include "macros.h"
#include "globals.h"

volatile unsigned int Time_Sequence    = RESET_STATE;
volatile char         one_time         = FALSE;
volatile unsigned int sw1_debounce_count  = DEBOUNCE_RESET;
volatile unsigned int sw2_debounce_count  = DEBOUNCE_RESET;
volatile char         sw1_debounce_active = DEBOUNCE_INACTIVE;
volatile char         sw2_debounce_active = DEBOUNCE_INACTIVE;
unsigned int          blink_count         = RESET_STATE;

// HW8 action flags — set by SW ISR, cleared and acted on by main loop
volatile char sw1_action_pending = 0;
volatile char sw2_action_pending = 0;

extern char                    display_line[4][11];
extern volatile unsigned char  update_display;
extern volatile unsigned char  display_changed;

extern volatile char         state;
extern volatile unsigned int ir_cnt;
extern volatile unsigned int motor_cmd_ticks;
extern volatile char         cmd_dequeue_flag;


//------------------------------------------------------------------------------
// Init_Timer_B0 - sets up TB0 for 50ms interrupts on CCR0
// CCR1 and CCR2 start disabled, enabled on switch press for debounce
//------------------------------------------------------------------------------
void Init_Timer_B0(void)
{
    TB0CTL  =  TBSSEL__SMCLK;   // SMCLK source (8MHz)
    TB0CTL  |= TBCLR;           // Clear timer
    TB0CTL  |= MC__CONTINOUS;   // Continuous mode
    TB0CTL  |= ID__2;           // Divide by 2

    TB0EX0  =  TBIDEX__8;       // Divide by 8 (total /16 = 500kHz)

    TB0CCR0  =  TB0CCR0_INTERVAL;   // CCR0 - 50ms interval
    TB0CCTL0 |= CCIE;               // Enable CCR0 interrupt

    TB0CCR1  =  TB0CCR1_INTERVAL;   // CCR1 - SW1 debounce
    TB0CCTL1 &= ~CCIE;              // Start disabled

    TB0CCR2  =  TB0CCR2_INTERVAL;   // CCR2 - SW2 debounce
    TB0CCTL2 &= ~CCIE;              // Start disabled

    TB0CTL  &= ~TBIE;           // Disable overflow interrupt
    TB0CTL  &= ~TBIFG;          // Clear overflow flag
}

//------------------------------------------------------------------------------
// Init_Timer_B3 - sets up PWM for motors and LCD backlight brightness
//------------------------------------------------------------------------------
void Init_Timer_B3(void){
    TB3CTL  = TBSSEL__SMCLK;
    TB3CTL |= MC__UP;
    TB3CTL |= TBCLR;

    PWM_PERIOD = WHEEL_PERIOD;

    TB3CCTL1 = OUTMOD_7;
    LCD_BACKLITE_BRIGHTNESS = PERCENT_80;

    TB3CCTL2 = OUTMOD_7;
    RIGHT_FORWARD_SPEED = WHEEL_OFF;

    TB3CCTL3 = OUTMOD_7;
    LEFT_FORWARD_SPEED  = WHEEL_OFF;

    TB3CCTL4 = OUTMOD_7;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;

    TB3CCTL5 = OUTMOD_7;
    LEFT_REVERSE_SPEED  = WHEEL_OFF;
}
//------------------------------------------------------------------------------
// Timer0_B0_ISR - fires every 50ms via CCR0
// toggles backlight every 200ms (2.5 blinks/sec), updates Time_Sequence
//------------------------------------------------------------------------------
#pragma vector = TIMER0_B0_VECTOR
__interrupt void Timer0_B0_ISR(void)
{
    one_time = TRUE;
    Time_Sequence++;
    update_display = TRUE;

    if (state == WHITE_SEARCH) {
        ir_cnt++;
    } else {
        ir_cnt = 0;
    }

    // IOT motor command countdown
    if (motor_cmd_ticks > 0) {
        motor_cmd_ticks--;
        if (motor_cmd_ticks == 0) {
            motors_reset();
            cmd_dequeue_flag = 1;  // signal Serial_Process to start next queued cmd
        }
    }

    TB0CCR0 += TB0CCR0_INTERVAL;
}
//------------------------------------------------------------------------------
// TIMER0_B1_ISR - handles CCR1 (SW1 debounce) and CCR2 (SW2 debounce)
// re-enables switch and backlight after debounce threshold is reached
//------------------------------------------------------------------------------
#pragma vector = TIMER0_B1_VECTOR
__interrupt void TIMER0_B1_ISR(void)
{
    switch (__even_in_range(TB0IV, TB0IV_MAX))
    {
        case TB0IV_NO:
            break;

        case TB0IV_CCR1:  // SW1 debounce
            if (sw1_debounce_active)
            {
                sw1_debounce_count++;
                if (sw1_debounce_count >= DEBOUNCE_THRESHOLD)
                {
                    sw1_debounce_active = DEBOUNCE_INACTIVE;
                    sw1_debounce_count  = DEBOUNCE_RESET;
                    P4IE     |= SW1;
                    TB0CCTL1 &= ~CCIE;

                }
            }
            TB0CCR1 += TB0CCR1_INTERVAL;
            break;

        case TB0IV_CCR2:  // SW2 debounce
            if (sw2_debounce_active)
            {
                sw2_debounce_count++;
                if (sw2_debounce_count >= DEBOUNCE_THRESHOLD)
                {
                    sw2_debounce_active = DEBOUNCE_INACTIVE;
                    sw2_debounce_count  = DEBOUNCE_RESET;
                    P2IE     |= SW2;
                    TB0CCTL2 &= ~CCIE;

                }
            }
            TB0CCR2 += TB0CCR2_INTERVAL;
            break;

        case TB0IV_OVERFLOW:
            DAC_data = DAC_data - 100;
                SAC3DAT  = DAC_data;
                if(DAC_data <= DAC_Limit){
                    DAC_data  = DAC_Adjust;
                    SAC3DAT   = DAC_data;
                    TB0CTL   &= ~TBIE;
                    RED_LED_OFF;
                }
            break;

        default:
            break;
    }
}

//------------------------------------------------------------------------------
// switch1_interrupt - Port 4 ISR, triggered on SW1 falling edge
// disables SW1, turns off backlight, starts CCR1 debounce timer
//------------------------------------------------------------------------------
#pragma vector = PORT4_VECTOR
__interrupt void switch1_interrupt(void)
{
    if (P4IFG & SW1)
    {
        P4IE  &= ~SW1;
        P4IFG &= ~SW1;

        sw1_debounce_count  = DEBOUNCE_RESET;
        sw1_debounce_active = DEBOUNCE_ACTIVE;

        TB0CCTL1 &= ~CCIFG;
        TB0CCR1   = TB0R + TB0CCR1_INTERVAL;
        TB0CCTL1 |= CCIE;

        sw1_action_pending = 1;     // HW8: signal main loop to switch to 115,200
    }
}

//------------------------------------------------------------------------------
// switch2_interrupt - Port 2 ISR, triggered on SW2 falling edge
// disables SW2, turns off backlight, starts CCR2 debounce timer
//------------------------------------------------------------------------------
#pragma vector = PORT2_VECTOR
__interrupt void switch2_interrupt(void)
{
    if (P2IFG & SW2)
    {
        P2IE  &= ~SW2;
        P2IFG &= ~SW2;

        sw2_debounce_count  = DEBOUNCE_RESET;
        sw2_debounce_active = DEBOUNCE_ACTIVE;

        TB0CCTL2 &= ~CCIFG;
        TB0CCR2   = TB0R + TB0CCR2_INTERVAL;
        TB0CCTL2 |= CCIE;

        sw2_action_pending = 1;     // HW8: signal main loop to switch to 460,800
    }
}
