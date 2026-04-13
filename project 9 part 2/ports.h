#define FALSE                  (0x00)
#define TRUE                   (0x01)
#define WHEEL_OFF              (0x00) // Motor stopped (0% PWM duty)

//------------------------------------------------------------------------------
// Hardware PWM macros -- Timer B3 CCR registers mapped to Port 6 motor pins.
//
// EMPIRICAL MAPPING for this car (verified via F/B/R/L TCP tests):
//   P6.1 = NOT ROUTED to any H-bridge input (TB3CCR1 ignored by PCB)
//   P6.2 = LEFT  FORWARD  --> TB3CCR2
//   P6.3 = RIGHT FORWARD  --> TB3CCR3   (NOT right reverse as labeled)
//   P6.4 = LEFT  REVERSE  --> TB3CCR4
//   P6.5 = RIGHT REVERSE  --> TB3CCR5   (the only remaining TB3 PWM pin)
//
// The R_FORWARD / R_REVERSE Port-6-bit names in this file still reflect the
// schematic labels; only the speed macros below are remapped so that calling
// Forward_On()/Reverse_On()/Spin_*_On() drive the correct directions on
// THIS car.
//------------------------------------------------------------------------------
#define RIGHT_FORWARD_SPEED (TB3CCR3)  // P6.3 (empirically right forward)
#define LEFT_FORWARD_SPEED  (TB3CCR2)  // P6.2 L_FORWARD
#define RIGHT_REVERSE_SPEED (TB3CCR5)  // P6.5 (TB3.5 drives right reverse)
#define LEFT_REVERSE_SPEED  (TB3CCR4)  // P6.4 L_REVERSE

// Port 1 Pins
#define RED_LED     (0x01) // P1.0 - Red LED
#define A1_SEEED    (0x02) // P1.1 - A1_SEEED ADC input
#define V_DETECT_L  (0x04) // P1.2 - Left voltage detect (ADC A2)
#define V_DETECT_R  (0x08) // P1.3 - Right voltage detect (ADC A3)
#define A4_SEEED    (0x10) // P1.4 - A4_SEEED ADC input
#define V_THUMB     (0x20) // P1.5 - Thumb voltage (ADC A5)
#define UCA0RXD     (0x40) // P1.6 - eUSCI_A0 UART RX (IOT)
#define UCA0TXD     (0x80) // P1.7 - eUSCI_A0 UART TX (IOT)

// Port 2 Pins
#define SLOW_CLK               (0x01) // 2.0
#define CHECK_BAT              (0x02) // 2.1 CHECK_BAT
#define IR_LED                 (0x04) // 2.2 IR LED
#define SW2                    (0x08) // 2.3 SW2
#define IOT_RUN_RED            (0x10) // 2.4 IOT_RUN_CPU
#define DAC_ENB                (0x20) // 2.5 DAC_ENB
#define LFXOUT                 (0x40) // 2.6 XOUTR
#define LFXIN                  (0x80) // 2.7 XINR

// Port 3 Pins
#define TEST_PROBE             (0x01) // 3.0 TEST PROBE
#define OA2O                   (0x02) // 3.1 OA2O
#define OA2N                   (0x04) // 3.2
#define OA2P                   (0x08) // 3.3
#define SMCLK                  (0x10) // 3.4 SMCLK
#define DAC_CNTL               (0x20) // 3.5 DAC_CNTL
#define IOT_LINK_CPU           (0x40) // 3.6
#define IOT_EN_CPU             (0x80) // 3.7

// Port 4 Pins
#define RESET_LCD              (0x01) // 4.0 RESET_LCD
#define SW1                    (0x02) // 4.1 SW1
#define UCA1RXD                (0x04) // 4.2 Back Channel UCA1RXD
#define UCA1TXD                (0x08) // 4.3 Back Channel UCA1TXD
#define UCB1_CS_LCD            (0x10) // 4.4 Chip Select
#define UCB1CLK                (0x20) // 4.5 UCB1SCK
#define UCB1SIMO               (0x40) // 4.6 UCB1SIMO
#define UCB1SOMI               (0x80) // 4.7 UCB1SOMI

// Port 5 Pins
#define V_BAT                  (0x01) // 5.0 V_BAT
#define V_5                    (0x02) // 5.1 V_5_0
#define V_DAC                  (0x04) // 5.2 V_DAC
#define V_3_3                  (0x08) // 5.3 V_3_3
#define IOT_BOOT_CPU           (0x10) // 5.4 IOT_BOOT

// Port 6 Pins
#define LCD_BACKLITE           (0x01) // 6.0 LCD_BACKLITE
#define R_FORWARD              (0x02) // 6.1 Right Forward
#define L_FORWARD              (0x04) // 6.2 Left Forward
#define R_REVERSE              (0x08) // 6.3 Right Reverse
#define L_REVERSE              (0x10) // 6.4 Left Reverse
#define P6_5                   (0x20) // 6.5 unused
#define GRN_LED                (0x40) // 6.6 GREEN LED
