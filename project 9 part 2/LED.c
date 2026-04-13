//==============================================================================
// File Name: led.c
// Description: LED control functions
// Author: Thomas Gilbert
//==============================================================================
#include "msp430.h"
#include "functions.h"
#include "LCD.h"
#include "ports.h"
#include "macros.h"

void Init_LEDs(void){
//------------------------------------------------------------------------------
// LED Configurations
//------------------------------------------------------------------------------
// Turns on both LEDs
  P1OUT |= RED_LED;
  P6OUT |= GRN_LED;
//------------------------------------------------------------------------------
}
