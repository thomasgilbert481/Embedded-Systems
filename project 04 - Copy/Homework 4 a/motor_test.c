//==============================================================================
// File Name: motor_test.c
// Description: HW4 motor test point verification - cycle with SW1
// Author: Thomas Gilbert
//==============================================================================
#include "msp430.h"
#include "functions.h"
#include "LCD.h"
#include "ports.h"
#include "macros.h"

extern char display_line[4][11];
extern volatile unsigned char display_changed;

static unsigned int test_step = 0;

// Turn ALL motors off
void Motors_Off(void){
    P6OUT &= ~R_FORWARD;
    P6OUT &= ~R_REVERSE;
    P6OUT &= ~L_FORWARD;
    P6OUT &= ~L_REVERSE;
}

// Individual motor on functions (always call Motors_Off first!)
void R_Forward_On(void){
    Motors_Off();
    P6OUT |= R_FORWARD;
}
void R_Reverse_On(void){
    Motors_Off();
    P6OUT |= R_REVERSE;
}
void L_Forward_On(void){
    Motors_Off();
    P6OUT |= L_FORWARD;
}
void L_Reverse_On(void){
    Motors_Off();
    P6OUT |= L_REVERSE;
}

// Step 12: H-bridge safety check - call in main while loop
void Motor_Safety_Check(void){
    if((P6OUT & L_FORWARD) && (P6OUT & L_REVERSE)){
        P6OUT &= ~L_FORWARD;
        P6OUT &= ~L_REVERSE;
        P1OUT |= RED_LED;
    }
    if((P6OUT & R_FORWARD) && (P6OUT & R_REVERSE)){
        P6OUT &= ~R_FORWARD;
        P6OUT &= ~R_REVERSE;
        P1OUT |= RED_LED;
    }
}

// Call this from your SW1 handler to advance the test
void Motor_Test_Advance(void){
    test_step++;
    if(test_step > 8) test_step = 0;

    switch(test_step){
        case 0: // All off - safe starting state
            Motors_Off();
            strcpy(display_line[0], " ALL OFF  ");
            strcpy(display_line[1], "  Ready   ");
            strcpy(display_line[2], "          ");
            strcpy(display_line[3], "SW1=next  ");
            break;

        case 1: // Step 8a: R_FORWARD off - check TP1,TP3 = Vbat
            Motors_Off();
            strcpy(display_line[0], " R_FWD OFF");
            strcpy(display_line[1], "TP1=Vbat  ");
            strcpy(display_line[2], "TP3=Vbat  ");
            strcpy(display_line[3], "SW1=next  ");
            break;

        case 2: // Step 8b: R_FORWARD on - TP1<=Vbat/2, TP3~GND
            R_Forward_On();
            strcpy(display_line[0], " R_FWD ON ");
            strcpy(display_line[1], "TP1<=Vb/2 ");
            strcpy(display_line[2], "TP3~GND   ");
            strcpy(display_line[3], "SW1=next  ");
            break;

        case 3: // Step 9a: R_REVERSE off - check TP2,TP4 = Vbat
            Motors_Off();
            strcpy(display_line[0], " R_REV OFF");
            strcpy(display_line[1], "TP2=Vbat  ");
            strcpy(display_line[2], "TP4=Vbat  ");
            strcpy(display_line[3], "SW1=next  ");
            break;

        case 4: // Step 9b: R_REVERSE on - TP2<=Vbat/2, TP4~GND
            R_Reverse_On();
            strcpy(display_line[0], " R_REV ON ");
            strcpy(display_line[1], "TP2<=Vb/2 ");
            strcpy(display_line[2], "TP4~GND   ");
            strcpy(display_line[3], "SW1=next  ");
            break;

        case 5: // Step 10a: L_FORWARD off - TP5,TP7 = Vbat
            Motors_Off();
            strcpy(display_line[0], " L_FWD OFF");
            strcpy(display_line[1], "TP5=Vbat  ");
            strcpy(display_line[2], "TP7=Vbat  ");
            strcpy(display_line[3], "SW1=next  ");
            break;

        case 6: // Step 10b: L_FORWARD on - TP5<=Vbat/2, TP7~GND
            L_Forward_On();
            strcpy(display_line[0], " L_FWD ON ");
            strcpy(display_line[1], "TP5<=Vb/2 ");
            strcpy(display_line[2], "TP7~GND   ");
            strcpy(display_line[3], "SW1=next  ");
            break;

        case 7: // Step 11a: L_REVERSE off - TP6,TP8 = Vbat
            Motors_Off();
            strcpy(display_line[0], " L_REV OFF");
            strcpy(display_line[1], "TP6=Vbat  ");
            strcpy(display_line[2], "TP8=Vbat  ");
            strcpy(display_line[3], "SW1=next  ");
            break;

        case 8: // Step 11b: L_REVERSE on - TP6<=Vbat/2, TP8~GND
            L_Reverse_On();
            strcpy(display_line[0], " L_REV ON ");
            strcpy(display_line[1], "TP6<=Vb/2 ");
            strcpy(display_line[2], "TP8~GND   ");
            strcpy(display_line[3], "SW1=done! ");
            break;

        default:
            Motors_Off();
            test_step = 0;
            break;
    }
    display_changed = 1;
}
