/*******************************************************************************
 * File:        serial.c
 * Description: UCA0 (J9, AD2 side) and UCA1 (J14, PC side) UART config + ISRs
 *
 * Project 8:  UCA0 is the primary comm port to the Analog Discovery 2.
 *              AD2 TX -> J9 RX (P1.6 / UCA0RXD)
 *              AD2 RX <- J9 TX (P1.7 / UCA0TXD)
 *
 * Baud rate register values for SMCLK = 8 MHz, oversampling mode (UCOS16 = 1):
 *
 *   115,200 baud:  N = 69.444
 *     UCBRx  = INT(N/16)              = 4
 *     UCBRFx = INT(frac(N/16) * 16)  = 5
 *     UCBRSx = table[frac(N)=0.444]  = 0x55   (Table 18-4)
 *     MCTLW  = 0x5551
 *
 *   460,800 baud:  N = 17.361  (> 16, oversampling required)
 *     UCBRx  = INT(N/16)              = 1
 *     UCBRFx = INT(frac(N/16) * 16)  = 1
 *     UCBRSx = table[frac(N)=0.361]  = 0x4A   (Table 18-4)
 *     MCTLW  = 0x4A11
 *
 * Author:  Noah Cartwright
 * Date:    Mar 2026
 * Class:   ECE 306
 ******************************************************************************/

#include "msp430.h"
#include <string.h>
#include "functions.h"
#include "LCD.h"
#include "ports.h"
#include "macros.h"
#include "globals.h"

//------------------------------------------------------------------------------
// Baud rate register values (SMCLK = 8 MHz, UCOS16 = 1)
//
//   115,200: N=69.44  UCBRx=4,  UCBRFx=5,  UCBRSx=0x55  MCTLW=0x5551
//     9,600: N=833.3  UCBRx=52, UCBRFx=1,  UCBRSx=0x49  MCTLW=0x4911
//   460,800: N=17.36  UCBRx=1,  UCBRFx=1,  UCBRSx=0x4A  MCTLW=0x4A11
//------------------------------------------------------------------------------
#define UCBRx_115200    (4)
#define MCTLW_115200    (0x5551)

#define UCBRx_9600      (52)
#define MCTLW_9600      (0x4911)

#define UCBRx_460800    (1)
#define MCTLW_460800    (0x4A11)

//------------------------------------------------------------------------------
// Baud rate display strings (10 chars, centered on 10-char LCD)
//------------------------------------------------------------------------------
const char str_115200[] = "  115,200 ";
const char str_460800[] = "  460,800 ";
const char str_baud[]   = "   Baud   ";

//------------------------------------------------------------------------------
// Current baud selection
//------------------------------------------------------------------------------
volatile char current_baud = BAUD_460800;

//------------------------------------------------------------------------------
// UCA0 receive buffer — accumulates 10-char messages from AD2 via J9
//------------------------------------------------------------------------------
volatile char         uca0_rx_buf[MSG_LEN + 1];
volatile unsigned int uca0_rx_idx  = 0;
volatile char         uca0_rx_done = 0;

//------------------------------------------------------------------------------
// UCA0 transmit buffer — sends message back to AD2, plus CR+LF terminator
//------------------------------------------------------------------------------
volatile char         uca0_tx_buf[MSG_LEN + 3];  // message + CR + LF + null
volatile unsigned int uca0_tx_idx  = 0;
volatile unsigned int uca0_tx_len  = 0;
volatile char         uca0_tx_busy = 0;

//------------------------------------------------------------------------------
// UCA1 TX state — interrupt-driven string transmit
//------------------------------------------------------------------------------
volatile char         tx1_busy  = 0;
volatile unsigned int tx1_idx   = 0;
volatile unsigned int tx1_len   = 0;
volatile char         uca1_tx_buf[32];  // string transmit buffer (max 31 chars + null)

//------------------------------------------------------------------------------
// UCA1 RX / PC-ready gate
// pc_ready: OFF until the first character is received from the PC.
// The ISR sets it ON; transmit functions check it before sending.
//------------------------------------------------------------------------------
volatile char         pc_ready  = OFF;

volatile char         rx1_buf[11];
volatile unsigned int rx1_idx   = 0;
volatile char         rx1_update = 0;

//------------------------------------------------------------------------------
extern volatile unsigned int display_timer;
extern volatile unsigned int Time_Sequence;  // 50ms tick counter (timers_interrupts.c)
extern char display_line[4][11];
extern volatile unsigned char display_changed;
extern volatile unsigned char update_display;

// Auto-transmit: string sent to PC every second once pc_ready is ON
static volatile unsigned int last_tick    = 0;
static volatile unsigned int serial_ticks = 0;
#define SERIAL_AUTO_INTERVAL    (ONE_SEC)     // 20 ticks × 50ms = 1 second
static const char auto_msg[] = "ECE306 P9\r\n";

//------------------------------------------------------------------------------
// UCA0 (IOT) TX string buffer — interrupt-driven, mirrors UCA1 TX design
//------------------------------------------------------------------------------
#define UCA0_TX_BUF_LEN         (64)
volatile char         uca0_str_tx_buf[UCA0_TX_BUF_LEN];
volatile unsigned int uca0_str_tx_idx  = 0;
volatile unsigned int uca0_str_tx_len  = 0;
volatile char         uca0_str_tx_busy = 0;

//------------------------------------------------------------------------------
// UCA0 (IOT) RX line buffer
// ISR accumulates chars; sets iot_line_ready when '\n' received.
// Main context reads iot_line_buf and clears iot_line_ready.
//------------------------------------------------------------------------------
#define IOT_LINE_BUF_LEN        (128)
volatile char         iot_line_buf[IOT_LINE_BUF_LEN];
volatile unsigned int iot_line_idx   = 0;
volatile char         iot_line_ready = 0;
char                  iot_matched_line[IOT_LINE_BUF_LEN]; // copy of matched line

// Parsed IOT network info (displayed on LCD)
char iot_ssid[11];    // SSID max 10 chars + null
char iot_ip[16];      // IP address string max 15 chars + null

// Motor command timer — set by IOT_Execute_Command, decremented by Timer0_B0_ISR
volatile unsigned int motor_cmd_ticks  = 0;
volatile char         cmd_dequeue_flag = 0;  // set by ISR when cmd finishes

// Command queue — holds up to 4 pending motor commands
#define CMD_QUEUE_SIZE  (4)
typedef struct { char dir; unsigned int ticks; } motor_cmd_t;
static motor_cmd_t    cmd_queue[CMD_QUEUE_SIZE];
static unsigned int   cmd_queue_head  = 0;
static unsigned int   cmd_queue_tail  = 0;
static unsigned int   cmd_queue_count = 0;

//------------------------------------------------------------------------------
// FRAM command parser state
// A command starts with '^' and ends with CR (0x0D).
// While cmd_active, chars are buffered — NOT forwarded to IOT.
// When CR is received, cmd_ready is set for Serial_Process to act on.
//------------------------------------------------------------------------------
#define CMD_BUF_LEN             (16)
#define CMD_START_CHAR          ('^')

volatile char         cmd_buf[CMD_BUF_LEN]; // chars received after '^', before CR
volatile unsigned int cmd_idx    = 0;
volatile char         cmd_active = OFF;     // set when '^' received
volatile char         cmd_ready  = OFF;     // set when CR received

// Response strings
static const char resp_here[]    = "I'm here\r\n";
static const char resp_fast[]    = "115,200\r\n";
static const char resp_slow[]    = "9,600\r\n";
static const char str_9600[]     = "   9,600  ";  // LCD display string

//------------------------------------------------------------------------------
// Init_Serial_UCA0
// Configure UCA0 for UART at the selected baud rate.
// P1.6 (RXD) and P1.7 (TXD) assigned to UART function in Init_Port1().
//------------------------------------------------------------------------------
void Init_Serial_UCA0(char baud_sel)
{
    UCA0IE    &= ~UCTXIE;              // Disable TX interrupt before reset
    UCA0CTLW0  =  UCSWRST;            // 1. Hold in reset
    UCA0CTLW0 |=  UCSSEL__SMCLK;      // 2. Clock = SMCLK (8 MHz)
    UCA0CTLW0 &= ~UCMSB;              //    LSB first
    UCA0CTLW0 &= ~UCSPB;              //    1 stop bit
    UCA0CTLW0 &= ~UCPEN;              //    No parity
    UCA0CTLW0 &= ~UCSYNC;             //    Async (UART) mode
    UCA0CTLW0 &= ~UC7BIT;             //    8 data bits
    UCA0CTLW0 |=  UCMODE_0;           //    UART mode

    if (baud_sel == BAUD_9600) {
        UCA0BRW   = UCBRx_9600;
        UCA0MCTLW = MCTLW_9600;
    } else if (baud_sel == BAUD_460800) {
        UCA0BRW   = UCBRx_460800;
        UCA0MCTLW = MCTLW_460800;
    } else {
        UCA0BRW   = UCBRx_115200;
        UCA0MCTLW = MCTLW_115200;
    }

    // 3. Ports already configured in Init_Port1()
    UCA0CTLW0 &= ~UCSWRST;            // 4. Release reset
    UCA0IE    |=  UCRXIE;             // 5. Enable RX interrupt only
    uca0_tx_busy = 0;                 //    Clear any pending TX state
}

//------------------------------------------------------------------------------
// Init_Serial_UCA1
// Configure UCA1 for UART at the selected baud rate.
// P4.2 (RXD) and P4.3 (TXD) assigned to UART function in Init_Port4().
//------------------------------------------------------------------------------
void Init_Serial_UCA1(char baud_sel)
{
    UCA1IE    &= ~UCTXIE;              // Disable TX interrupt before reset
    UCA1CTLW0  =  UCSWRST;            // 1. Hold in reset
    UCA1CTLW0 |=  UCSSEL__SMCLK;      // 2. Clock = SMCLK (8 MHz)
    UCA1CTLW0 &= ~UCMSB;              //    LSB first
    UCA1CTLW0 &= ~UCSPB;              //    1 stop bit
    UCA1CTLW0 &= ~UCPEN;              //    No parity
    UCA1CTLW0 &= ~UCSYNC;             //    Async (UART) mode
    UCA1CTLW0 &= ~UC7BIT;             //    8 data bits
    UCA1CTLW0 |=  UCMODE_0;           //    UART mode

    // UCA1 (PC back-channel) is always 115,200 — baud_sel ignored
    UCA1BRW   = UCBRx_115200;
    UCA1MCTLW = MCTLW_115200;

    // 3. Ports already configured in Init_Port4()
    UCA1CTLW0 &= ~UCSWRST;            // 4. Release reset
    UCA1IE    |=  UCRXIE;             // 5. Enable RX interrupt only
    tx1_busy   =  0;
    pc_ready   =  ON;                 // Open immediately — don't drop IOT boot messages
}

//------------------------------------------------------------------------------
// Change_Baud_Rate — re-initialize UCA0 (IOT port) only.
// UCA1 (PC back-channel) is always 115,200 and is never changed.
//------------------------------------------------------------------------------
void Change_Baud_Rate(char baud_sel)
{
    current_baud = baud_sel;
    Init_Serial_UCA0(baud_sel);
}

//------------------------------------------------------------------------------
// Transmit_UCA0_String — send null-terminated string to IOT module (UCA0)
// Must end with "\r\n" for AT commands.
//------------------------------------------------------------------------------
void Transmit_UCA0_String(const char *s)
{
    unsigned int i;
    if (uca0_str_tx_busy) return;

    for (i = 0; i < UCA0_TX_BUF_LEN - 1 && s[i] != '\0'; i++) {
        uca0_str_tx_buf[i] = s[i];
    }
    uca0_str_tx_len  = i;
    uca0_str_tx_idx  = 1;
    uca0_str_tx_busy = 1;

    UCA0TXBUF = uca0_str_tx_buf[0];
    UCA0IE   |= UCTXIE;
}

//------------------------------------------------------------------------------
// IOT_Wait_For — blocking wait for a line containing 'expected' from UCA0.
// Polls iot_line_ready (set by ISR on '\n'). Times out after timeout_ticks×50ms.
// On match, copies the line into iot_matched_line and returns 1.
// Returns 0 on timeout.
//------------------------------------------------------------------------------
char IOT_Wait_For(const char *expected, unsigned int timeout_ticks)
{
    unsigned int start = Time_Sequence;
    iot_line_ready = 0;
    iot_line_idx   = 0;

    while ((unsigned int)(Time_Sequence - start) < timeout_ticks) {
        if (iot_line_ready) {
            iot_line_ready = 0;
            if (strstr((char *)iot_line_buf, expected)) {
                strcpy(iot_matched_line, (char *)iot_line_buf);
                return 1;
            }
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
// Helper — parse SSID from +CWJAP:"ssid","..."
//------------------------------------------------------------------------------
static void parse_ssid(const char *line, char *out)
{
    const char *p = strchr(line, '"');
    unsigned int i = 0;
    if (!p) { out[0] = '\0'; return; }
    p++;
    while (*p && *p != '"' && i < 10) out[i++] = *p++;
    out[i] = '\0';
}

//------------------------------------------------------------------------------
// Helper — parse IP from +CIFSR:STAIP,"x.x.x.x"
//------------------------------------------------------------------------------
static void parse_ip(const char *line, char *out)
{
    const char *p = strchr(line, '"');
    unsigned int i = 0;
    if (!p) { out[0] = '\0'; return; }
    p++;
    while (*p && *p != '"' && i < 15) out[i++] = *p++;
    out[i] = '\0';
}

//------------------------------------------------------------------------------
// Helper — center src in a 10-char LCD field, write to dst (must be 11 bytes)
//------------------------------------------------------------------------------
static void center_str(const char *src, char *dst)
{
    unsigned int len = strlen(src);
    unsigned int pad, i;
    if (len > 10) len = 10;
    pad = (10 - len) / 2;
    for (i = 0; i < 10; i++) dst[i] = ' ';
    for (i = 0; i < len; i++) dst[pad + i] = src[i];
    dst[10] = '\0';
}

//------------------------------------------------------------------------------
// Helper — split IP "a.b.c.d" into "a.b" → first and "c.d" → second
//------------------------------------------------------------------------------
static void split_ip(const char *ip, char *first, char *second)
{
    unsigned int dot_count = 0, i = 0;
    while (ip[i] && dot_count < 2) {
        if (ip[i] == '.') dot_count++;
        i++;
    }
    strncpy(first, ip, i - 1);
    first[i - 1] = '\0';
    strcpy(second, ip + i);
}

//------------------------------------------------------------------------------
// IOT_Display_SSID_IP — step 53-54: show network info on 4-line LCD
//   Line 1: SSID centered (max 10 chars)
//   Line 2: "IP address"
//   Line 3: first 2 IP groups centered  (e.g. "  10.154  ")
//   Line 4: last  2 IP groups centered  (e.g. "  41.102  ")
//------------------------------------------------------------------------------
void IOT_Display_SSID_IP(void)
{
    char ip_top[16], ip_bot[16], tmp[11];

    lcd_4line();

    center_str(iot_ssid, tmp);
    strcpy(display_line[0], tmp);

    strcpy(display_line[1], "IP address");

    split_ip(iot_ip, ip_top, ip_bot);
    center_str(ip_top, tmp);
    strcpy(display_line[2], tmp);
    center_str(ip_bot, tmp);
    strcpy(display_line[3], tmp);

    Display_Update(0, 0, 0, 0);
}

//------------------------------------------------------------------------------
// IOT_Init
// Performs the hardware reset sequence for the ESP32-WROOM IOT module.
//
// IOT_EN (P3.7) was held LOW since port init (IOT in reset from power-on).
// This function waits IOT_RESET_TICKS × 50ms (200ms), then releases EN HIGH
// so the module boots into normal run mode.
//
// IOT_BOOT (P5.4) is already HIGH (set in Init_Port5), so the module boots
// into normal firmware — NOT download mode.
//
// LCD feedback during the sequence:
//   Line 1: "IOT Reset"  (during hold)
//   Line 1: "IOT Ready"  (after release)
//------------------------------------------------------------------------------
void IOT_Init(void)
{
    unsigned int start;

    // ── 1. Hardware reset pulse ───────────────────────────────────────────────
    lcd_4line();
    strcpy(display_line[0], "IOT Reset ");
    strcpy(display_line[1], "          ");
    strcpy(display_line[2], "          ");
    strcpy(display_line[3], "          ");
    Display_Update(0, 0, 0, 0);

    P3OUT &= ~IOT_EN;
    start = Time_Sequence;
    while ((unsigned int)(Time_Sequence - start) < IOT_RESET_TICKS);
    P3OUT |= IOT_EN;

    // ── 2. Wait for "ready" from AT firmware ─────────────────────────────────
    strcpy(display_line[0], "IOT Boot  ");
    Display_Update(0, 0, 0, 0);

    if (!IOT_Wait_For("ready", IOT_READY_TIMEOUT)) {
        strcpy(display_line[0], "No Ready! ");
        Display_Update(0, 0, 0, 0);
        return;
    }

    // ── 3. Configure TCP server (required every power-on) ────────────────────
    strcpy(display_line[0], "IOT Ready ");
    Display_Update(0, 0, 0, 0);

    Transmit_UCA0_String("AT+CIPMUX=1\r\n");
    IOT_Wait_For("OK", IOT_CMD_TIMEOUT);

    Transmit_UCA0_String("AT+CIPSERVER=1," IOT_TCP_PORT "\r\n");
    IOT_Wait_For("OK", IOT_CMD_TIMEOUT);

    // ── 4. Wait for WiFi auto-connect ────────────────────────────────────────
    strcpy(display_line[0], "WiFi Wait ");
    Display_Update(0, 0, 0, 0);
    {
        unsigned int tries;
        for (tries = 0; tries < IOT_WIFI_TIMEOUT / IOT_CMD_TIMEOUT; tries++) {
            Transmit_UCA0_String("AT+CWSTATE?\r\n");
            if (IOT_Wait_For("+CWSTATE:2", IOT_CMD_TIMEOUT)) break;
        }
    }

    // ── 5. Query SSID ─────────────────────────────────────────────────────────
    Transmit_UCA0_String("AT+CWJAP?\r\n");
    if (IOT_Wait_For("+CWJAP:", IOT_READY_TIMEOUT)) {
        parse_ssid(iot_matched_line, iot_ssid);
    } else {
        strcpy(iot_ssid, "No WiFi");
    }

    // ── 6. Query IP address ───────────────────────────────────────────────────
    Transmit_UCA0_String("AT+CIFSR\r\n");
    if (IOT_Wait_For("STAIP", IOT_CMD_TIMEOUT)) {
        parse_ip(iot_matched_line, iot_ip);
    } else {
        strcpy(iot_ip, "0.0.0.0");
    }

    // ── 7. Display SSID and IP on LCD (steps 53-54) ──────────────────────────
    IOT_Display_SSID_IP();
}

//------------------------------------------------------------------------------
// IOT_Display_Command — step 58: show direction on enlarged middle LCD line
//------------------------------------------------------------------------------
static void IOT_Display_Command(char dir)
{
    lcd_BIG_mid();
    strcpy(display_line[BIG_TOP], "  CMD rcv ");
    switch (dir) {
        case 'F': strcpy(display_line[BIG_MID], " FORWARD  "); break;
        case 'B': strcpy(display_line[BIG_MID], " BACKWARD "); break;
        case 'R': strcpy(display_line[BIG_MID], "  RIGHT   "); break;
        case 'L': strcpy(display_line[BIG_MID], "  LEFT    "); break;
        default:  strcpy(display_line[BIG_MID], " UNKNOWN  "); break;
    }
    strcpy(display_line[BIG_BOT], "          ");
    display_changed = TRUE;
    update_display  = TRUE;
}

//------------------------------------------------------------------------------
// IOT_Execute_Now — immediately start motors and set countdown timer
//------------------------------------------------------------------------------
static void IOT_Execute_Now(char dir, unsigned int ticks)
{
    motors_reset();
    switch (dir) {
        case 'F': motors_forward();      break;
        case 'B': motors_reverse();      break;
        case 'R': pivot_right_pwm(SPIN); break;
        case 'L': pivot_left_pwm(SPIN);  break;
        default:  return;
    }
    IOT_Display_Command(dir);
    motor_cmd_ticks = ticks;
}

//------------------------------------------------------------------------------
// IOT_Execute_Command — queue command if motors busy, execute immediately if not
//------------------------------------------------------------------------------
static void IOT_Execute_Command(char dir, unsigned int ticks)
{
    if (motor_cmd_ticks == 0 && cmd_queue_count == 0) {
        IOT_Execute_Now(dir, ticks);
    } else if (cmd_queue_count < CMD_QUEUE_SIZE) {
        cmd_queue[cmd_queue_tail].dir   = dir;
        cmd_queue[cmd_queue_tail].ticks = ticks;
        cmd_queue_tail = (cmd_queue_tail + 1) % CMD_QUEUE_SIZE;
        cmd_queue_count++;
    }
}

//------------------------------------------------------------------------------
// IOT_Parse_Command — parse ALL ^PIN+DIR+TIME commands in a TCP payload.
// Scans the full payload so two commands sent together are both queued.
//
// Format per command:  ^5555F0040
//   [0]     = '^'
//   [1..4]  = PIN (IOT_CMD_PIN = "5555")
//   [5]     = direction  F/B/R/L
//   [6..]   = time units (1 unit = 50ms)
//------------------------------------------------------------------------------
static void IOT_Parse_Command(const char *payload)
{
    const char *p = payload;
    char dir;
    unsigned int ticks, i;

    while (*p) {
        if (*p != IOT_CMD_START)           { p++; continue; }
        if (strncmp(p + 1, IOT_CMD_PIN, IOT_PIN_LEN) != 0) { p++; continue; }

        dir   = p[1 + IOT_PIN_LEN];
        ticks = 0;
        i     = 2 + IOT_PIN_LEN;
        while (p[i] >= '0' && p[i] <= '9') {
            ticks = ticks * 10 + (p[i] - '0');
            i++;
        }
        IOT_Execute_Command(dir, ticks);
        p += i;
    }
}

//------------------------------------------------------------------------------
// IOT_IPD_Process — called from Serial_Process each main-loop tick.
// When UCA0 RX ISR sets iot_line_ready, check if it is an +IPD packet,
// extract the payload after ':', and pass it to the command parser.
//------------------------------------------------------------------------------
void IOT_IPD_Process(void)
{
    char *colon;

    if (!iot_line_ready) return;
    iot_line_ready = 0;

    if (strncmp((char *)iot_line_buf, "+IPD", 4) != 0) return;

    colon = strchr((char *)iot_line_buf, ':');
    if (!colon) return;

    IOT_Parse_Command(colon + 1);
}

//------------------------------------------------------------------------------
// Serial_Process
// Called every main-loop iteration.
// Counts 50ms ticks from Time_Sequence; every ONE_SEC ticks (1 second) it
// sends auto_msg to the PC via UCA1 — but only after pc_ready is ON.
// Also the right place to add any other background serial work later.
//------------------------------------------------------------------------------
void Serial_Process(void)
{
    // Step 15: auto-transmit disabled — was cluttering IOT responses
    // (kept for reference — re-enable by uncommenting below)
    /*
    if (Time_Sequence != last_tick) {
        last_tick = Time_Sequence;
        serial_ticks++;
        if (serial_ticks >= SERIAL_AUTO_INTERVAL) {
            serial_ticks = 0;
            Transmit_UCA1_String(auto_msg);
        }
    }
    */

    // Dequeue next motor command when current one finishes
    if (cmd_dequeue_flag) {
        cmd_dequeue_flag = 0;
        if (cmd_queue_count > 0) {
            motor_cmd_t next = cmd_queue[cmd_queue_head];
            cmd_queue_head  = (cmd_queue_head + 1) % CMD_QUEUE_SIZE;
            cmd_queue_count--;
            IOT_Execute_Now(next.dir, next.ticks);
        }
    }

    // Steps 19-23: act on completed '^' commands from PC
    IOT_Command_Process();

    // Steps 56-58: handle incoming TCP commands from Java client
    IOT_IPD_Process();
}

//------------------------------------------------------------------------------
// Transmit_UCA0_Message
// Send the contents of uca0_rx_buf back to the AD2 via UCA0 (J9).
// Appends CR+LF so the AD2 WaveForms terminal recognizes end-of-message.
// TX interrupt handles all chars after the first.
//------------------------------------------------------------------------------
void Transmit_UCA0_Message(void)
{
    unsigned int i;
    if (uca0_tx_busy) return;

    // Copy receive buffer, pad to MSG_LEN with spaces
    for (i = 0; i < MSG_LEN; i++) {
        if (uca0_rx_buf[i] != '\0') {
            uca0_tx_buf[i] = uca0_rx_buf[i];
        } else {
            uca0_tx_buf[i] = ' ';
        }
    }
    uca0_tx_buf[i++] = '\r';           // Carriage return
    uca0_tx_buf[i++] = '\n';           // Line feed
    uca0_tx_buf[i]   = '\0';
    uca0_tx_len  = i;                  // Total chars to send (MSG_LEN + 2)

    uca0_tx_idx  = 1;                  // ISR sends from index 1 onward
    uca0_tx_busy = 1;

    UCA0TXBUF = uca0_tx_buf[0];        // Kick off first char
    UCA0IE   |= UCTXIE;                // Enable TX interrupt for remaining chars
}

//------------------------------------------------------------------------------
// Update_Baud_Display
// Write the current baud rate string to LCD line 3 (display_line[2]).
//------------------------------------------------------------------------------
void Update_Baud_Display(char baud_sel)
{
    if (baud_sel == BAUD_9600) {
        strcpy(display_line[2], str_9600);
    } else if (baud_sel == BAUD_460800) {
        strcpy(display_line[2], str_460800);
    } else {
        strcpy(display_line[2], str_115200);
    }
    display_changed = TRUE;
    update_display  = TRUE;
}

//------------------------------------------------------------------------------
// IOT_Command_Process
// Called from Serial_Process every main loop tick.
// Acts on completed '^' commands (cmd_ready flag set by UCA1 RX ISR).
//
// Commands (everything between '^' and CR):
//   ^       (empty / ^^) → respond "I'm here"
//   F       (^F)         → set UCA0 to 115,200; respond "115,200"
//   S       (^S)         → set UCA0 to   9,600; respond "9,600"
//------------------------------------------------------------------------------
void IOT_Command_Process(void)
{
    if (!cmd_ready) return;
    cmd_ready = OFF;

    if (cmd_buf[0] == CMD_START_CHAR || cmd_buf[0] == '\0') {
        // ^^ command
        Transmit_UCA1_String(resp_here);

    } else if (cmd_buf[0] == 'F') {
        // ^F — fast baud rate on IOT port
        Change_Baud_Rate(BAUD_115200);
        Transmit_UCA1_String(resp_fast);
        Update_Baud_Display(BAUD_115200);

    } else if (cmd_buf[0] == 'S') {
        // ^S — slow baud rate on IOT port
        Change_Baud_Rate(BAUD_9600);
        Transmit_UCA1_String(resp_slow);
        Update_Baud_Display(BAUD_9600);
    }
}

//------------------------------------------------------------------------------
// eUSCI_A0_ISR — primary comm port (AD2 via J9)
//
// RX: accumulate chars into uca0_rx_buf.
//     Message ends on CR ('\r'), LF ('\n'), or after MSG_LEN chars.
//     Sets uca0_rx_done when complete message received.
//
// TX: send next char from uca0_tx_buf; disable interrupt when done.
//------------------------------------------------------------------------------
#pragma vector = EUSCI_A0_VECTOR
__interrupt void eUSCI_A0_ISR(void)
{
    char c;
    switch (__even_in_range(UCA0IV, 0x08))
    {
        case 0x00: break;

        case 0x02:                              // RX — char from IOT
            c = UCA0RXBUF;
            // Forward to PC
            if (pc_ready && !tx1_busy && (UCA1IFG & UCTXIFG)) {
                UCA1TXBUF = c;
            }
            // Buffer line for response parsing (IOT_Wait_For)
            if (c == '\n') {
                iot_line_buf[iot_line_idx] = '\0';
                iot_line_ready = 1;
                iot_line_idx   = 0;
            } else if (c != '\r' && iot_line_idx < IOT_LINE_BUF_LEN - 1) {
                iot_line_buf[iot_line_idx++] = c;
            }
            break;

        case 0x04:                              // TX — send next string char to IOT
            if (uca0_str_tx_busy && uca0_str_tx_idx < uca0_str_tx_len) {
                UCA0TXBUF = uca0_str_tx_buf[uca0_str_tx_idx++];
            } else {
                uca0_str_tx_busy = 0;
                UCA0IE &= ~UCTXIE;
            }
            break;

        default: break;
    }
}

//------------------------------------------------------------------------------
// Transmit_UCA1_String
// Sends a null-terminated string out the PC back-channel (UCA1).
// Only transmits if pc_ready is ON (i.e., PC has sent at least one char).
// Interrupt-driven: first char is kicked here; ISR handles the rest.
//------------------------------------------------------------------------------
void Transmit_UCA1_String(const char *s)
{
    unsigned int i;
    if (!pc_ready || tx1_busy) return;

    for (i = 0; i < 31 && s[i] != '\0'; i++) {
        uca1_tx_buf[i] = s[i];
    }
    uca1_tx_buf[i] = '\0';
    tx1_len  = i;
    tx1_idx  = 1;
    tx1_busy = 1;

    UCA1TXBUF = uca1_tx_buf[0];        // Kick off first char
    UCA1IE   |= UCTXIE;                // Enable TX interrupt for remaining chars
}

//------------------------------------------------------------------------------
// eUSCI_A1_ISR — PC back-channel (J14, UCA1)
//
// RX (step 9-10): Read into temp; echo back to PC.
//                 First received char sets pc_ready ON (step 13-14).
//                 Echo is suppressed while a string TX is in progress
//                 to avoid corrupting the TX buffer.
//
// TX (step 15+):  Send next char from uca1_tx_buf; disable interrupt when done.
//------------------------------------------------------------------------------
#pragma vector = EUSCI_A1_VECTOR
__interrupt void eUSCI_A1_ISR(void)
{
    char temp;
    switch (__even_in_range(UCA1IV, 0x08))
    {
        case 0x00: break;

        case 0x02:                              // RX — char from PC
            temp = UCA1RXBUF;
            pc_ready = ON;                      // Step 14: gate now open

            if (temp == CMD_START_CHAR && !cmd_active) {
                // Step 23: '^' detected — start FRAM command, do NOT forward
                cmd_active = ON;
                cmd_idx    = 0;
            } else if (cmd_active) {
                if (temp == '\r') {
                    // CR = end of command
                    cmd_buf[cmd_idx] = '\0';
                    cmd_ready  = ON;
                    cmd_active = OFF;
                } else if (cmd_idx < CMD_BUF_LEN - 1) {
                    // Accumulate command chars — do NOT forward to IOT
                    cmd_buf[cmd_idx++] = temp;
                }
            } else {
                // Step 18: normal char — passthrough to IOT (UCA0)
                if (UCA0IFG & UCTXIFG) {
                    UCA0TXBUF = temp;
                }
            }
            break;

        case 0x04:                              // TX — send next string char
            if (tx1_busy && tx1_idx < tx1_len) {
                UCA1TXBUF = uca1_tx_buf[tx1_idx++];
            } else {
                tx1_busy = 0;
                UCA1IE  &= ~UCTXIE;
            }
            break;

        default: break;
    }
}
