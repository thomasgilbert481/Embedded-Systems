#ifndef MACROS_H_
#define MACROS_H_

// NOTE: TRUE, FALSE, WHEEL_OFF, WHEEL_PERIOD, RED_LED, GRN_LED,
//       FORWARD, REVERSE are defined in ports.h — do NOT redefine here

#define ALWAYS              (1)
#define RESET_STATE         (0)
#define ON                  (0x01)
#define OFF                 (0x00)

#define RED_LED_ON          (P1OUT |=  RED_LED)
#define RED_LED_OFF         (P1OUT &= ~RED_LED)

#define USE_GPIO            (0x00)
#define USE_SMCLK           (0x01)

#define DEBOUNCE_TIME       (500)
#define DEBOUNCE_RESTART    (0)
#define RELEASED            (1)
#define NOT_OKAY            (0)
#define OKAY                (1)
#define PRESSED             (0)

#define P4PUD               (P4OUT)
#define PWM_PERIOD          (TB3CCR0)
#define LCD_BACKLITE_DIMING     (TB3CCR5)
#define LCD_BACKLITE_BRIGHTNESS (TB3CCR1)

#define TB0CCR0_INTERVAL    (25000)
#define TB0CCR1_INTERVAL    (25000)
#define TB0CCR2_INTERVAL    (25000)

#define TIMER_B0_CCR0_VECTOR            (TIMER0_B0_VECTOR)
#define TIMER_B0_CCR1_2_OV_VECTOR       (TIMER0_B1_VECTOR)
#define TIMER_B1_CCR0_VECTOR            (TIMER1_B0_VECTOR)
#define TIMER_B1_CCR1_2_OV_VECTOR       (TIMER1_B1_VECTOR)
#define TIMER_B2_CCR0_VECTOR            (TIMER2_B0_VECTOR)
#define TIMER_B2_CCR1_2_OV_VECTOR       (TIMER2_B1_VECTOR)
#define TIMER_B3_CCR0_VECTOR            (TIMER3_B0_VECTOR)
#define TIMER_B3_CCR1_2_OV_VECTOR       (TIMER3_B1_VECTOR)

#define DEBOUNCE_THRESHOLD  (20)
#define DEBOUNCE_RESET      (0)
#define DEBOUNCE_ACTIVE     (1)
#define DEBOUNCE_INACTIVE   (0)

#define RESET_REGISTER      (0x0000)
#define DIM                 (0)

#define TB0IV_MAX           (14)
#define TB0IV_NO            (0)
#define TB0IV_CCR1          (2)
#define TB0IV_CCR2          (4)
#define TB0IV_OVERFLOW      (14)

// DAC
#define DAC_Begin           (2725)
#define DAC_Limit           (850)
#define DAC_Adjust          (875)

// States
#define IDLE                ('I')
#define DELAY               ('D')
#define DRIVING             ('P')
#define LINE_FOUND          ('L')
#define TURNING             ('T')
#define CIRCLING            ('C')
#define EXITING             ('Q')
#define STOPPED             ('Z')
#define WAIT                ('W')
#define START               ('S')
#define RUN                 ('R')
#define END                 ('E')
#define DONE                ('X')
#define NONE                ('N')

// Motor speeds
#define FOLLOW_SPEED        (15000)
#define SLOW                (11000) //keep at this
#define FAST                (40000)
#define SPIN                (15000)

// Sensor thresholds
#define BLACK_THRESHOLD     (500) //above this for black
#define WHITE_THRESHOLD     (350)//below this for white
#define IR_DIFF_THRESHOLD   (100)

// Timing
#define LAPS_TO_DO          (2)
#define TURN_TICKS          (6)
#define EXIT_SPIN_TICKS     (15)
#define EXIT_DRIVE_TICKS    (25)
#define HALF_SEC            (10)
#define WAITING2START       (500)

#define WHITE_SEARCH    ('H')
#define BLACK_LINE      ('B')
#define LEFT            ('<')
#define RIGHT           ('>')
#define SEARCH_FWD      (4)    // tune this — ticks before switching spin di


#define PERCENT_80      (45000)
#define PERCENT_100     (50000)

// Baud rate selection tokens
#define BAUD_115200     (0)
#define BAUD_9600       (2)
#define BAUD_460800     (1)

// IOT_EN alias (P3.7 is named IOT_RN in ports.h — same pin)
#define IOT_EN              IOT_RN

// IOT reset pulse: hold EN low for this many 50ms ticks before releasing
// 4 ticks = 200ms (spec minimum is 100ms)
#define IOT_RESET_TICKS     (4)

// IOT blocking-wait timeouts (in 50ms ticks)
#define IOT_READY_TIMEOUT   (60)    //  60 × 50ms =  3s  (boot → "ready")
#define IOT_CMD_TIMEOUT     (20)    //  20 × 50ms =  1s  (AT command response)
#define IOT_WIFI_TIMEOUT    (400)   // 400 × 50ms = 20s  (WiFi auto-connect)

// TCP server port
#define IOT_TCP_PORT        "8080"

// Command protocol
#define IOT_CMD_PIN         "5555"   // 4-char PIN required in every command
#define IOT_CMD_START       ('^')    // command start character
#define IOT_PIN_LEN         (4)      // PIN length
// Time unit = 1 timer tick = 50ms
// Demo: F0040=2s fwd, B0020=1s rev, R0010=90deg right, L0005=45deg left

// Splash screen duration: 5 seconds at 50 ms/tick = 100 ticks
#define SPLASH_TICKS    (100)

// Project 8 — serial message length (chars, not counting CR/LF/null)
#define MSG_LEN         (10)

// Project 8 state machine values
#define P8_WAITING      (0)
#define P8_RECEIVED     (1)
#define P8_TRANSMIT     (2)


// ─── Homework 9: Menu System ──────────────────────────────────────────────────

// Top-level menu state values
#define MENU_SPLASH         (0)   // startup splash — wait for any button press
#define MENU_MAIN           (1)   // 3-item main menu (Resistors / Shapes / Song)
#define MENU_RESISTOR       (2)   // resistor colour-code sub-menu
#define MENU_SHAPES         (3)   // shapes sub-menu (lcd_BIG_mid)
#define MENU_SONG           (4)   // song scroll sub-menu (lcd_BIG_mid)

// Main-menu item indices
#define MAIN_RESISTORS      (0)
#define MAIN_SHAPES         (1)
#define MAIN_SONG           (2)
#define NUM_MAIN_ITEMS      (3)

// ADC dividers — 10-bit ADC range is 0–1023
#define ADC_MAIN_DIV        (341)   // 1024/3  → 3 equal regions for main menu
#define ADC_SUB_DIV         (102)   // 1024/10 → 10 equal regions for sub-menus

// Sub-menu item counts
#define NUM_RESISTORS       (10)
#define NUM_SHAPES          (10)

// Song-scroll constants
#define SONG_DISP_LEN       (10)  // visible character width of the big LCD line
#define LCD_LINE_BUF        (SONG_DISP_LEN + 1) // display_line column size (10 chars + null)
#define ADC_SONG_SHIFT      (7)   // right-shift maps 10-bit ADC → 8 zones (0–7)
#define SONG_CHARS_PER_ZONE (5)   // song characters advanced per CCW zone crossing
#define SONG_ALT_RESET      (0)   // initial value of the Line-1/3 alternating flag
#define SONG_START          (0)   // starting character index into song_text[]

// Sentinel value — wider than any valid item index (0–9), forces a redraw
#define INVALID_ITEM        (0xFFFFu)

// display_line[] index aliases (lcd_4line mode)
#define MENU_LINE1          (0)
#define MENU_LINE2          (1)
#define MENU_LINE3          (2)
#define MENU_LINE4          (3)

// display_line[] index aliases (lcd_BIG_mid mode)
#define BIG_TOP             (0)   // small top line
#define BIG_MID             (1)   // large centre line
#define BIG_BOT             (2)   // small bottom line

// Generic flag values (used where TRUE/FALSE would be ambiguous)
#define CLEAR_FLAG          (0)
#define SET_FLAG            (1)

#endif /* MACROS_H_ */
