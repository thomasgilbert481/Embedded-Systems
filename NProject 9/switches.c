//------------------------------------------------------------------------------
//
//  Description: This file contains the switches config
//
//  Noah Cartwight
//  Feb 20 2026
//
//------------------------------------------------------------------------------
#include  "msp430.h"
#include  <string.h>
#include  "functions.h"
#include  "LCD.h"
#include  "ports.h"
#include "macros.h"
#include "globals.h"

unsigned int sw1_position;
unsigned int okay_to_look_at_switch1 = OKAY;
unsigned int count_debounce_SW1 = DEBOUNCE_RESTART;

unsigned int sw2_position;
unsigned int okay_to_look_at_switch2 = OKAY;
unsigned int count_debounce_SW2 = DEBOUNCE_RESTART;

void Switches_Process(void)
{
//------------------------------------------------------------------------------
// This function calls the individual Switch Functions
//------------------------------------------------------------------------------
    //Switch1_Process();
    //Switch2_Process();
}

void Switch1_Process(void)
{
//------------------------------------------------------------------------------
// Switch 1 Configurations
// Port P4 Pin 1
    //set to use SMCLK
//------------------------------------------------------------------------------
//    if (okay_to_look_at_switch1 && sw1_position)
//    {
//        if (!(P4IN & SW1))
//        {
//            sw1_position = PRESSED;
//            okay_to_look_at_switch1 = NOT_OKAY;
//            count_debounce_SW1 = DEBOUNCE_RESTART;
//// do what you want with button press
//
//
//
//        }
//    }
//    if (count_debounce_SW1 <= DEBOUNCE_TIME)
//    {
//        count_debounce_SW1++;
//    }
//    else
//    {
//        okay_to_look_at_switch1 = OKAY;
//        if (P4IN & SW1)
//        {
//            sw1_position = RELEASED;
//            P1OUT &= ~RED_LED;
//        }
//    }
}

void Switch2_Process(void)
{
//------------------------------------------------------------------------------
// Switch 2 Configurations
// Port P2 Pin 3
    //sets to use GPIO
//------------------------------------------------------------------------------
//    if (okay_to_look_at_switch2 && sw2_position)
//    {
//        if (!(P2IN & SW2))
//        {
//            sw2_position = PRESSED;
//            okay_to_look_at_switch2 = NOT_OKAY;
//            count_debounce_SW2 = DEBOUNCE_RESTART;
//// do what you want with button press
//
//
//
//
//
//        }
//    }
//    if (count_debounce_SW2 <= DEBOUNCE_TIME)
//    {
//        count_debounce_SW2++;
//    }
//    else
//    {
//        okay_to_look_at_switch2 = OKAY;
//        if (P2IN & SW2)
//        {
//            sw2_position = RELEASED;
//        }
//    }
}

