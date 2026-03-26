//==============================================================================
// File:        serial.c
// Description: UCA0 UART serial communication for Project 8.
//              Handles initialization, ISR, ring buffer processing,
//              and interrupt-driven transmission.
//
//              Connected to J9 (IOT connector) for AD2 communication:
//                P1.6 = UCA0RXD  (AD2 TX  → Vehicle RX)
//                P1.7 = UCA0TXD  (AD2 RX  ← Vehicle TX)
//
//              Architecture:
//                RX: ISR deposits bytes into IOT_Ring_Rx ring buffer.
//                    Main loop calls IOT_Process() to drain the buffer
//                    and assemble complete commands.
//                TX: Serial_Transmit() loads iot_TX_buf, writes the first
//                    byte to UCA0TXBUF, then enables UCTXIE.
//                    The TX ISR sends one byte per TXIFG interrupt until
//                    the null-terminator sentinel is reached.
//
// Baud rate register values (BRCLK = SMCLK = 8 MHz):
//   9,600:   UCOS16=1, UCBRx=52, UCFx=1,  UCSx=0x49 => BRW=52,  MCTLW=0x4911
//  115,200:  UCOS16=1, UCBRx=4,  UCFx=5,  UCSx=0x55 => BRW=4,   MCTLW=0x5551
//
// Author: Thomas Gilbert
// Date: Mar 2026
// Compiler: Code Composer Studio
// Target: MSP430FR2355
//==============================================================================

#include "msp430.h"
#include <string.h>
#include "macros.h"
#include "serial.h"

//==============================================================================
// External globals (LCD display — defined in LCD.obj)
//==============================================================================
extern char                  display_line[4][11]; // LCD line buffers
extern volatile unsigned char display_changed;    // Set to signal LCD refresh

//==============================================================================
// Module global definitions (declared extern in serial.h)
//==============================================================================

// Circular receive buffer and indices
volatile char         IOT_Ring_Rx[IOT_RING_SIZE]; // Bytes deposited by RX ISR
volatile unsigned int iot_rx_wr = BEGINNING;       // ISR write index (volatile!)
         unsigned int iot_rx_rd = BEGINNING;       // Main-loop read index (non-volatile)

// TX buffer: loaded by Serial_Transmit(), drained by TX ISR
char         iot_TX_buf[SERIAL_MSG_LENGTH + 2];   // +2 for CR + LF terminator
unsigned int iot_tx    = BEGINNING;               // Index into iot_TX_buf

// Assembled received command (null-terminated, SERIAL_MSG_LENGTH chars max)
char                  received_command[SERIAL_MSG_LENGTH + 1];
volatile unsigned char command_ready = 0;         // 1 when full command is assembled

// Active baud rate index — determines which register values Init_Serial_UCA0 loads
unsigned char baud_rate_index = BAUD_9600;        // Start at 9,600 baud

// Character count used while assembling a command from individual ring-buffer bytes.
// Module-level (not function-local) so Clear_Serial_Buffers() can reset it.
static unsigned int rx_char_count = BEGINNING;

//==============================================================================
// Function: Init_Serial_UCA0
// Description: Configure eUSCI_A0 for UART mode at the selected baud rate.
//              Must be called AFTER Init_Clocks() (requires stable 8 MHz SMCLK).
//              Safe to call again to change baud rate — UCSWRST briefly halts UCA0.
//
// Parameters:  baud_index — BAUD_9600 or BAUD_115200 (from macros.h)
//==============================================================================
void Init_Serial_UCA0(unsigned char baud_index){

    // Place eUSCI into reset before touching any control registers
    UCA0CTLW0  = UCSWRST;

    // Clock source: SMCLK (8 MHz)
    UCA0CTLW0 |= UCSSEL__SMCLK;

    // Frame format: LSB first, 8 data bits, no parity, 1 stop bit, async UART
    UCA0CTLW0 &= ~UCMSB;      // LSB first (standard UART convention)
    UCA0CTLW0 &= ~UC7BIT;     // 8-bit data (not 7-bit)
    UCA0CTLW0 &= ~UCPEN;      // No parity
    UCA0CTLW0 &= ~UCSPB;      // 1 stop bit (not 2)
    UCA0CTLW0 &= ~UCSYNC;     // Asynchronous (UART) mode
    UCA0CTLW0 |=  UCMODE_0;   // UART mode

    // Load baud rate registers from the selected entry in the table
    switch(baud_index){
        case BAUD_9600:
            UCA0BRW   = BAUD_9600_BRW;    // 52
            UCA0MCTLW = BAUD_9600_MCTLW;  // 0x4911: UCOS16=1, UCFx=1, UCSx=0x49
            break;
        case BAUD_115200:
            UCA0BRW   = BAUD_115200_BRW;   // 4
            UCA0MCTLW = BAUD_115200_MCTLW; // 0x5551: UCOS16=1, UCFx=5, UCSx=0x55
            break;
        default:
            // Fall back to 9,600 for any unknown index
            UCA0BRW   = BAUD_9600_BRW;
            UCA0MCTLW = BAUD_9600_MCTLW;
            break;
    }

    // Release eUSCI from reset — peripheral is now active
    UCA0CTLW0 &= ~UCSWRST;

    // Prime the TX buffer to clear any spurious first-byte issue
    // Writing here ensures TXIFG is set and the TX path is ready
    UCA0TXBUF = SERIAL_NULL;

    // Enable RX interrupt only; TX interrupt is enabled on-demand in Serial_Transmit
    UCA0IE |= UCRXIE;
}

//==============================================================================
// Function: IOT_Process
// Description: Reads one byte per call from IOT_Ring_Rx and assembles it into
//              received_command[]. Called from the main while() loop — NOT
//              from an ISR. Sets command_ready when a full command is assembled.
//
//              End-of-message detection:
//                - CR (0x0D) or LF (0x0A) → treat as terminator
//                - SERIAL_MSG_LENGTH characters received → treat as full command
//
//              CR and LF bytes are discarded (not stored in received_command).
//==============================================================================
void IOT_Process(void){
    unsigned int  iot_rx_wr_snap; // Snapshot of volatile write index (avoids tearing)
    char          incoming_byte;

    iot_rx_wr_snap = iot_rx_wr;   // Read volatile index once

    if(iot_rx_wr_snap != iot_rx_rd){
        // At least one byte is available — read one
        incoming_byte = IOT_Ring_Rx[iot_rx_rd++];

        // Wrap read index at end of ring buffer
        if(iot_rx_rd >= IOT_RING_SIZE){
            iot_rx_rd = BEGINNING;
        }

        // CR or LF signals end of message — do not store, just finalize
        if((incoming_byte == SERIAL_CR) || (incoming_byte == SERIAL_LF)){
            if(rx_char_count > BEGINNING){
                received_command[rx_char_count] = SERIAL_NULL; // Null-terminate
                rx_char_count = BEGINNING;                     // Reset for next message
                command_ready = 1;                             // Signal main loop
            }
            return; // Done for this byte
        }

        // Store printable character into the command assembly buffer
        if(rx_char_count < SERIAL_MSG_LENGTH){
            received_command[rx_char_count++] = incoming_byte;
        }

        // Exactly SERIAL_MSG_LENGTH characters received → command is complete
        if(rx_char_count >= SERIAL_MSG_LENGTH){
            received_command[SERIAL_MSG_LENGTH] = SERIAL_NULL;
            rx_char_count = BEGINNING;
            command_ready = 1;
        }
    }
}

//==============================================================================
// Function: Serial_Transmit
// Description: Copies msg into iot_TX_buf (appending CR+LF), then kicks off
//              interrupt-driven transmission by writing the first byte to
//              UCA0TXBUF and enabling UCTXIE. The TX ISR (case 4) sends the
//              remaining bytes one per TXIFG interrupt until it hits SERIAL_NULL.
//
// Parameters:  msg — null-terminated string, max SERIAL_MSG_LENGTH characters
//==============================================================================
void Serial_Transmit(char *msg){
    unsigned int i;

    // Copy message into TX buffer and append CR+LF so the AD2 sees a full line
    for(i = BEGINNING; (i < SERIAL_MSG_LENGTH) && (msg[i] != SERIAL_NULL); i++){
        iot_TX_buf[i] = msg[i];
    }
    iot_TX_buf[i++] = SERIAL_CR;    // Carriage return
    iot_TX_buf[i++] = SERIAL_LF;    // Line feed
    iot_TX_buf[i]   = SERIAL_NULL;  // Sentinel so TX ISR knows when to stop

    // Reset TX index to start of buffer
    iot_tx = BEGINNING;

    // Write first byte manually — this clears TXIFG and starts the hardware shift
    // The TX ISR picks up at iot_tx = 1 when TXIFG fires after the first byte
    UCA0TXBUF = iot_TX_buf[iot_tx++];

    // Enable TX interrupt; ISR sends remaining bytes
    UCA0IE |= UCTXIE;
}

//==============================================================================
// Function: Update_Baud_Display
// Description: Writes the current baud rate label to LCD line 3 (display_line[2]).
//              Uses the LCD_LINE3_BAUD index from macros.h.
//              Called after baud_rate_index changes (SW2 press).
//==============================================================================
void Update_Baud_Display(void){
    switch(baud_rate_index){
        case BAUD_9600:
            strcpy(display_line[LCD_LINE3_BAUD], "  9,600   ");
            break;
        case BAUD_115200:
            strcpy(display_line[LCD_LINE3_BAUD], " 115,200  ");
            break;
        default:
            strcpy(display_line[LCD_LINE3_BAUD], "  ??????  ");
            break;
    }
    display_changed = 1;
}

//==============================================================================
// Function: Clear_Serial_Buffers
// Description: Resets all RX and TX buffer state. Call this before switching
//              baud rates so no bytes received at the old rate get processed
//              or retransmitted at the new rate.
//
//              - Disables UCTXIE to abort any in-progress transmission
//              - Resets ring buffer indices (discards unread RX bytes)
//              - Resets TX index and command assembly state
//              - Clears command_ready so a stale command is not acted on
//==============================================================================
void Clear_Serial_Buffers(void){
    // Abort any TX in progress
    UCA0IE &= ~UCTXIE;

    // Reset ring buffer indices — bytes written at the old baud rate are discarded
    iot_rx_wr = BEGINNING;
    iot_rx_rd = BEGINNING;

    // Reset TX buffer index
    iot_tx = BEGINNING;

    // Reset command assembly state
    rx_char_count = BEGINNING;
    command_ready = 0;
}

//==============================================================================
// ISR: eUSCI_A0_ISR
// Description: Handles UCA0 RX and TX interrupts.
//
//   Case 2 (RXIFG): A byte arrived from the AD2.
//     Deposit it into IOT_Ring_Rx at iot_rx_wr, then advance and wrap the
//     write index. Main loop drains the buffer via IOT_Process().
//
//   Case 4 (TXIFG): TX register empty — hardware is ready for the next byte.
//     If iot_TX_buf[iot_tx] is not the null sentinel, load it into UCA0TXBUF
//     and advance iot_tx. If it IS the sentinel, all bytes have been sent:
//     disable UCTXIE and reset iot_tx for the next call to Serial_Transmit().
//==============================================================================
#pragma vector = EUSCI_A0_VECTOR
__interrupt void eUSCI_A0_ISR(void){

    switch(__even_in_range(UCA0IV, 0x08)){

        case 0:  // Vector 0 — no interrupt pending
            break;

        case 2:  // Vector 2 — RXIFG: byte received from AD2
            // Deposit directly into ring buffer; main loop reads via IOT_Process
            IOT_Ring_Rx[iot_rx_wr++] = UCA0RXBUF;
            if(iot_rx_wr >= IOT_RING_SIZE){
                iot_rx_wr = BEGINNING;  // Wrap write index
            }
            break;

        case 4:  // Vector 4 — TXIFG: TX register empty, ready for next byte
            if(iot_TX_buf[iot_tx] != SERIAL_NULL){
                // More bytes remain — load the next one
                UCA0TXBUF = iot_TX_buf[iot_tx++];
            } else {
                // Null sentinel reached — entire message has been sent
                UCA0IE &= ~UCTXIE;    // Disable TX interrupt until next transmit
                iot_tx = BEGINNING;   // Reset index for next Serial_Transmit call
            }
            break;

        default:
            break;
    }
}
