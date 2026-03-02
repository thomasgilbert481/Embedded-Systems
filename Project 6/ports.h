#define FALSE                  (0x00) //
#define TRUE                   (0x01) //
#define MOTOR                  (0x00) //
#define SMCLK_OFF              (0x00) //
#define SMCLK_ON               (0x01) //
#define PORTS                  (0x00) // RED LED 0
#define PWM_MODE               (0x01) // GREEN LED 1
#define WHEEL_OFF              (0x00)
#define WHEEL_PERIOD          (10000)
#define RIGHT_FORWARD_SPEED (TB3CCR2)
#define RIGHT_REVERSE_SPEED (TB3CCR3)
#define LEFT_FORWARD_SPEED  (TB3CCR4)
#define LEFT_REVERSE_SPEED  (TB3CCR5)
#define STEP                   (2000)
#define FORWARD                (0x00) // FORWARD
#define REVERSE                (0x01) // REVERSE


// Port 1 Pins
#define RED_LED     (0x01) // P1.0 - Red LED
#define A1_SEEED    (0x02) // P1.1 - A1_SEEED ADC input
#define V_DETECT_L  (0x04) // P1.2 - Left voltage detect
#define V_DETECT_R  (0x08) // P1.3 - Right voltage detect
#define A4_SEEED    (0x10) // P1.4 - A4_SEEED ADC input
#define V_THUMB     (0x20) // P1.5 - Thumb voltage
#define UCA0RXD     (0x40) // P1.6 - UART Receive
#define UCA0TXD     (0x80) // P1.7 - UART Transmit

// Port 2 Pins
#define SLOW_CLK               (0x01) // 2.0 LCD Reset
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
#define OA2N                   (0x04) // 3.2 Photodiode Circuit
#define OA2P                   (0x08) // 3.3 Photodiode Circuit
#define SMCLK                  (0x10) // 3.4 SMCLK
#define DAC_CNTL               (0x20) // 3.5 DAC_CNTL
#define IOT_LINK_CPU           (0x40) // 3.6 IOT_LINK_GRN
#define IOT_EN_CPU             (0x80) // 3.7 IOT_EN              1

// Port 4 Pins
#define RESET_LCD              (0x01) // 4.0 RESET_LCD
#define SW1                    (0x02) // 4.1 SW1
#define UCA1RXD                (0x04) // 4.2 Back Channel UCA1RXD
#define UCA1TXD                (0x08) // 4.3 Back Channel UCA1TXD
#define UCB1_CS_LCD            (0x10) // 4.4 Chip Select
#define UCB1CLK                (0x20) // 4.5UCB1SCK
#define UCB1SIMO               (0x40) // 4.6 UCB1SIMO
#define UCB1SOMI               (0x80) // 4.7 UCB1SOMI

// Port 5 Pins
#define V_BAT                  (0x01) // 5.0 V_BAT
#define V_5                    (0x02) // 5.1 V_5_0
#define V_DAC                  (0x04) // 5.2 V_DAC
#define V_3_3                  (0x08) // 5.3 V_3_3
#define IOT_BOOT_CPU           (0x10) // 5.4 IOT_BOOT           1

// Port 6 Pins
#define LCD_BACKLITE           (0x01) // 6.0 LCD_BACKLITE
#define R_FORWARD              (0x02) // 6.1 P6_0_PWM
#define L_FORWARD              (0x04) // 6.2 P6_1_PWM
#define R_REVERSE              (0x08) // 6.3 P6_2_PWM
#define L_REVERSE              (0x10) // 6.4 P6_3_PWM
#define P6_5                   (0x20) // 6.5
#define GRN_LED                (0x40) // 6.6 GREEN LED
