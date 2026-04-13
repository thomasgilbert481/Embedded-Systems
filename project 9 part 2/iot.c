//==============================================================================
// File:        iot.c
// Description: Project 9 Part 2 -- IOT AT command state machine + +IPD parser.
//
//  After the FRAM releases IOT_EN, the ESP32 boots and prints "ready".  This
//  state machine then:
//    1) verifies AT round-trip works
//    2) waits for "WIFI GOT IP" (autoconnect already configured by hand)
//    3) issues  AT+CIPMUX=1
//    4) issues  AT+CIPSERVER=1,<IOT_TCP_PORT>
//    5) issues  AT+CIFSR  and parses STAIP into car_ip[]
//    6) shows SSID + IP on the LCD
//    7) loops watching IOT_Data[][] for "+IPD,..."  lines from the TCP client
//       and decodes the protocol  ^<PIN><dir><time-units>  (e.g. ^1234F0010).
//
//  The motor command runs for time_units * CMD_TIME_UNIT_MS milliseconds,
//  then Vehicle_Cmd_Tick() (called from the 200 ms Timer B0 ISR) auto-stops
//  the wheels.
//
// Author: Thomas Gilbert
// Date: Mar 2026
// Compiler: Code Composer Studio
// Target: MSP430FR2355
//==============================================================================

#include "msp430.h"
#include <string.h>
#include "macros.h"
#include "ports.h"
#include "functions.h"
#include "serial.h"
#include "iot.h"

//==============================================================================
// External LCD globals
//==============================================================================
extern char                  display_line[4][11];
extern volatile unsigned char display_changed;

//==============================================================================
// Module globals
//==============================================================================
char car_ssid[12] = "ncsu      ";   // 10-char left-justified placeholder
char car_ip[20]   = "          ";   // Filled in after AT+CIFSR

volatile unsigned int  cmd_remaining_ms = BEGINNING;
volatile char          cmd_active_dir   = SERIAL_NULL;
volatile unsigned int  cmd_active_time  = BEGINNING;

//==============================================================================
// Internal state-machine state
//==============================================================================
#define IOT_STATE_WAIT_READY    (0)
#define IOT_STATE_SEND_AT       (1)
#define IOT_STATE_WAIT_AT_OK    (2)
#define IOT_STATE_WAIT_WIFI     (3)
#define IOT_STATE_SEND_CIPMUX   (4)
#define IOT_STATE_SEND_SERVER   (5)
#define IOT_STATE_SEND_CIFSR    (6)
#define IOT_STATE_WAIT_IP       (7)
#define IOT_STATE_RUNNING       (8)

static unsigned int  iot_state    = IOT_STATE_WAIT_READY;
static unsigned long iot_wait_cnt = BEGINNING;

//==============================================================================
// AT command strings (CR+LF appended -- caller uses Serial_Transmit as-is)
//==============================================================================
static char AT_CHECK[]   = "AT\r\n";
static char AT_CIPMUX[]  = "AT+CIPMUX=1\r\n";
static char AT_CIFSR[]   = "AT+CIFSR\r\n";
// AT+CIPSERVER=1,<port>\r\n  -- built at runtime so IOT_TCP_PORT is the only knob
static char AT_CIPSERVER[24];

//==============================================================================
// Helper: clear the IOT_Data parse buffer
//==============================================================================
static void clear_iot_data(void){
    unsigned int i;
    for(i = BEGINNING; i < IOT_DATA_LINES; i++){
        IOT_Data[i][0] = SERIAL_NULL;
    }
    iot_data_line = BEGINNING;
}

//==============================================================================
// Helper: scan all IOT_Data rows for a substring (returns row index or -1)
//==============================================================================
static int find_in_iot_data(const char *needle){
    int i;
    for(i = 0; i < IOT_DATA_LINES; i++){
        if(IOT_Data[i][0] != SERIAL_NULL && strstr(IOT_Data[i], needle)){
            return i;
        }
    }
    return -1;
}

//==============================================================================
// Helper: build "AT+CIPSERVER=1,<IOT_TCP_PORT>\r\n" into AT_CIPSERVER[]
//==============================================================================
static void build_cipserver_string(void){
    unsigned int port = IOT_TCP_PORT;
    char digits[6];
    int  d = 0;
    int  i = 0;
    const char prefix[] = "AT+CIPSERVER=1,";

    // Copy prefix
    while(prefix[i] != SERIAL_NULL){
        AT_CIPSERVER[i] = prefix[i];
        i++;
    }
    // Convert port to ASCII digits (reversed)
    if(port == 0){
        digits[d++] = '0';
    }
    while(port > 0){
        digits[d++] = (char)('0' + (port % 10));
        port /= 10;
    }
    // Append digits in correct order
    while(d > 0){
        AT_CIPSERVER[i++] = digits[--d];
    }
    AT_CIPSERVER[i++] = SERIAL_CR;
    AT_CIPSERVER[i++] = SERIAL_LF;
    AT_CIPSERVER[i]   = SERIAL_NULL;
}

//==============================================================================
// IOT_State_Machine -- call from main loop every iteration.
//==============================================================================
void IOT_State_Machine(void){

    switch(iot_state){

        case IOT_STATE_WAIT_READY:
            // ESP32 prints "ready" after IOT_EN releases; if not seen, just
            // proceed to AT after a long timeout (already-running module).
            if(find_in_iot_data("ready") >= 0){
                clear_iot_data();
                iot_wait_cnt = BEGINNING;
                iot_state = IOT_STATE_SEND_AT;
                break;
            }
            if(++iot_wait_cnt > IOT_TIMEOUT_READY){
                iot_wait_cnt = BEGINNING;
                iot_state = IOT_STATE_SEND_AT;
            }
            break;

        case IOT_STATE_SEND_AT:
            Serial_Transmit(AT_CHECK);
            iot_wait_cnt = BEGINNING;
            iot_state = IOT_STATE_WAIT_AT_OK;
            break;

        case IOT_STATE_WAIT_AT_OK:
            if(find_in_iot_data("OK") >= 0){
                clear_iot_data();
                iot_wait_cnt = BEGINNING;
                iot_state = IOT_STATE_WAIT_WIFI;
                break;
            }
            if(++iot_wait_cnt > IOT_TIMEOUT_AT){
                iot_wait_cnt = BEGINNING;
                iot_state = IOT_STATE_SEND_AT;     // Retry AT
            }
            break;

        case IOT_STATE_WAIT_WIFI:
            // Autoconnect was configured manually; just wait for GOT IP.
            // If already connected, the ESP32 won't say it again -- so a
            // generous timeout falls through to CIPMUX setup anyway.
            if(find_in_iot_data("GOT IP") >= 0){
                clear_iot_data();
                iot_wait_cnt = BEGINNING;
                iot_state = IOT_STATE_SEND_CIPMUX;
                break;
            }
            if(++iot_wait_cnt > IOT_TIMEOUT_WIFI){
                iot_wait_cnt = BEGINNING;
                iot_state = IOT_STATE_SEND_CIPMUX;  // Assume already connected
            }
            break;

        case IOT_STATE_SEND_CIPMUX:
            Serial_Transmit(AT_CIPMUX);
            iot_wait_cnt = BEGINNING;
            iot_state = IOT_STATE_SEND_SERVER;
            break;

        case IOT_STATE_SEND_SERVER:
            // Build CIPSERVER once, then send.
            if(AT_CIPSERVER[0] == SERIAL_NULL){
                build_cipserver_string();
            }
            // Wait for the previous CIPMUX response to settle, then send.
            if(++iot_wait_cnt > IOT_TIMEOUT_GENERIC){
                Serial_Transmit(AT_CIPSERVER);
                iot_wait_cnt = BEGINNING;
                iot_state = IOT_STATE_SEND_CIFSR;
            }
            break;

        case IOT_STATE_SEND_CIFSR:
            if(++iot_wait_cnt > IOT_TIMEOUT_GENERIC){
                clear_iot_data();
                Serial_Transmit(AT_CIFSR);
                iot_wait_cnt = BEGINNING;
                iot_state = IOT_STATE_WAIT_IP;
            }
            break;

        case IOT_STATE_WAIT_IP: {
            int row = find_in_iot_data("STAIP");
            if(row >= 0){
                // Response row looks like:  +CIFSR:STAIP,"10.152.15.74"
                char *q1 = strchr(IOT_Data[row], '"');
                char *q2 = (q1 != NULL) ? strchr(q1 + 1, '"') : NULL;
                if(q1 && q2 && q2 > q1){
                    unsigned int len = (unsigned int)(q2 - q1 - 1);
                    if(len >= sizeof(car_ip)){
                        len = sizeof(car_ip) - 1;
                    }
                    memcpy(car_ip, q1 + 1, len);
                    car_ip[len] = SERIAL_NULL;
                }
                clear_iot_data();
                Display_Network_Info();
                iot_state = IOT_STATE_RUNNING;
                break;
            }
            if(++iot_wait_cnt > IOT_TIMEOUT_GENERIC){
                iot_wait_cnt = BEGINNING;
                iot_state = IOT_STATE_SEND_CIFSR;   // Retry
            }
        } break;

        case IOT_STATE_RUNNING: {
            // Scan for "+IPD" -- the TCP client sent us a payload.
            int i;
            for(i = 0; i < IOT_DATA_LINES; i++){
                if(IOT_Data[i][0] != SERIAL_NULL && strstr(IOT_Data[i], "+IPD")){
                    Parse_IPD_Command(IOT_Data[i]);
                    IOT_Data[i][0] = SERIAL_NULL;
                }
            }
        } break;

        default:
            iot_state = IOT_STATE_WAIT_READY;
            break;
    }
}

//==============================================================================
// Parse_IPD_Command -- parse "+IPD,<conn>,<len>:<payload>" line.
//   payload format:   ^<PIN0><PIN1><PIN2><PIN3><dir><t3><t2><t1><t0>
//   e.g.              ^1234F0010   -> Forward, 10 * 100ms = 1.0 s
//==============================================================================
void Parse_IPD_Command(char *line){
    char         *payload;
    char          dir;
    unsigned int  time_units;
    unsigned int  i;

    // Locate ':' that separates header from payload
    payload = strchr(line, ':');
    if(payload == NULL){
        USB_transmit_string("ERR: +IPD no ':'\r\n");
        return;
    }
    payload++;   // first byte of payload

    // Must start with the CARET prefix
    if(payload[0] != SERIAL_CARET){
        USB_transmit_string("ERR: missing ^\r\n");
        return;
    }

    // Verify PIN
    if(payload[1] != CMD_PIN_0 || payload[2] != CMD_PIN_1 ||
       payload[3] != CMD_PIN_2 || payload[4] != CMD_PIN_3){
        USB_transmit_string("ERR: bad PIN\r\n");
        return;
    }

    // Direction byte
    dir = payload[CMD_DIR_OFFSET];

    // Time units -- 4 ASCII digits
    time_units = BEGINNING;
    for(i = BEGINNING; i < CMD_TIME_DIGITS; i++){
        char c = payload[CMD_TIME_OFFSET + i];
        if(c < '0' || c > '9'){
            USB_transmit_string("ERR: bad time\r\n");
            return;
        }
        time_units = (time_units * 10) + (unsigned int)(c - '0');
    }

    // Stop any in-flight motion before starting a new command (SAFETY)
    Wheels_All_Off();
    cmd_remaining_ms = BEGINNING;
    cmd_active_dir   = SERIAL_NULL;

    switch(dir){
        case CMD_DIR_FORWARD:
            Forward_On();
            USB_transmit_string("CMD: F\r\n");
            break;
        case CMD_DIR_BACKWARD:
            Reverse_On();
            USB_transmit_string("CMD: B\r\n");
            break;
        case CMD_DIR_RIGHT:
            Spin_CW_On();
            USB_transmit_string("CMD: R\r\n");
            break;
        case CMD_DIR_LEFT:
            Spin_CCW_On();
            USB_transmit_string("CMD: L\r\n");
            break;
        default:
            USB_transmit_string("ERR: bad dir\r\n");
            return;
    }

    // Arm the auto-stop countdown
    cmd_active_dir   = dir;
    cmd_active_time  = time_units;
    cmd_remaining_ms = time_units * CMD_TIME_UNIT_MS;
}

//==============================================================================
// Vehicle_Cmd_Tick -- called from Timer B0 CCR0 ISR every TB0_TICK_MS (200ms).
// Decrements the active-command countdown and stops motors when it reaches 0.
//==============================================================================
void Vehicle_Cmd_Tick(void){
    if(cmd_remaining_ms == BEGINNING){
        return;
    }
    if(cmd_remaining_ms <= TB0_TICK_MS){
        cmd_remaining_ms = BEGINNING;
        cmd_active_dir   = SERIAL_NULL;
        Wheels_All_Off();
    } else {
        cmd_remaining_ms -= TB0_TICK_MS;
    }
}

//==============================================================================
// Display_Network_Info -- show SSID + IP on the LCD.
//   Line 1: SSID (10 chars)
//   Line 2: "IP address"
//   Line 3: first two octets   (e.g. "10.152    ")
//   Line 4: last two octets    (e.g. "15.74     ")
//==============================================================================
void Display_Network_Info(void){
    unsigned int i;
    char dot_count;
    int  split_idx = -1;
    char hi[11] = "          ";
    char lo[11] = "          ";

    // SSID line
    for(i = 0; i < 10; i++){
        display_line[LCD_LINE1_SSID][i] =
            (car_ssid[i] != SERIAL_NULL) ? car_ssid[i] : ' ';
    }
    display_line[LCD_LINE1_SSID][10] = SERIAL_NULL;

    // Static label
    strcpy(display_line[LCD_LINE2_LABEL], "IP address");

    // Split car_ip on the second '.'  -> hi = "AAA.BBB", lo = "CCC.DDD"
    dot_count = 0;
    for(i = 0; car_ip[i] != SERIAL_NULL && i < sizeof(car_ip); i++){
        if(car_ip[i] == '.'){
            dot_count++;
            if(dot_count == 2){
                split_idx = (int)i;
                break;
            }
        }
    }

    if(split_idx >= 0){
        // hi = car_ip[0 .. split_idx-1]
        for(i = 0; i < (unsigned int)split_idx && i < 10; i++){
            hi[i] = car_ip[i];
        }
        // lo = car_ip[split_idx+1 .. ]
        {
            unsigned int j = 0;
            unsigned int k = (unsigned int)split_idx + 1;
            while(car_ip[k] != SERIAL_NULL && j < 10){
                lo[j++] = car_ip[k++];
            }
        }
    } else {
        // Fallback: dump whole IP into line 3
        for(i = 0; car_ip[i] != SERIAL_NULL && i < 10; i++){
            hi[i] = car_ip[i];
        }
    }

    strcpy(display_line[LCD_LINE3_IP_HI], hi);
    strcpy(display_line[LCD_LINE4_IP_LO], lo);

    display_changed = TRUE;
}
