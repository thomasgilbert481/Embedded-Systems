//==============================================================================
// File: serial.c
// Description: Serial port initialization and interrupt service routines
//              for eUSCI_A0 (IOT / J9) and eUSCI_A1 (USB-PC backchannel / J14).
//
// Data flow for Configuration 3 (J9 loopback, J14 pass-thru):
//   PC (Termite) -> J14 -> UCA1 RX ISR -> UCA0 TX -> J9 loopback ->
//   UCA0 RX ISR -> UCA1 TX -> J14 -> PC (Termite) -- character echoes back
//
// Baud rate table (BRCLK = SMCLK = 8 MHz):
//   115,200: UCOS16=1, UCBRx=4,  UCFx=5,  UCSx=0x55 => MCTLW=0x5551, BRW=4
//   460,800: UCOS16=0, UCBRx=17, UCFx=0,  UCSx=0x4A => MCTLW=0x4A00, BRW=17
//
// Author: Thomas Gilbert
// Date: Mar 2026
// Compiler: Code Composer Studio
// Target: MSP430FR2355
//==============================================================================

#include "msp430.h"
#include "functions.h"
#include "macros.h"

//==============================================================================
// External globals
//==============================================================================
extern char display_line[4][11];
extern volatile unsigned char display_changed;

//==============================================================================
// Module globals
//==============================================================================

// Ring buffer: IOT (UCA0 RX) -> PC (UCA1 TX)
volatile char IOT_2_PC[SMALL_RING_SIZE];
volatile unsigned int iot_rx_wr = BEGINNING;
volatile unsigned int iot_rx_rd = BEGINNING;

// Ring buffer: PC (UCA1 RX) -> IOT (UCA0 TX)
volatile char PC_2_IOT[SMALL_RING_SIZE];
volatile unsigned int pc_rx_wr = BEGINNING;
volatile unsigned int pc_rx_rd = BEGINNING;

// UCA1 transmit buffer (for Transmit_String_UCA1)
volatile char UCA1_transmit_buf[25];
volatile unsigned int uca1_tx_index = BEGINNING;
volatile unsigned int UCA1_tx_active = BEGINNING;

// Received character display buffer (read by main loop to update LCD line 1)
volatile char received_string[11] = "          ";
volatile unsigned int received_index = BEGINNING;

//==============================================================================
// Init_Serial_UCA0 -- Configure eUSCI_A0 for UART (IOT port J9)
//   P1.6 = UCA0RXD, P1.7 = UCA0TXD (configured in ports.c)
//   Default: 115,200 baud, 8-N-1, SMCLK source
//==============================================================================
void Init_Serial_UCA0(void){
    UCA0CTLW0  = UCSWRST;             // Hold in reset during config
    UCA0CTLW0 |= UCSSEL__SMCLK;       // BRCLK = SMCLK = 8 MHz
    UCA0CTLW0 &= ~UCMSB;              // LSB first
    UCA0CTLW0 &= ~UCSPB;              // 1 stop bit
    UCA0CTLW0 &= ~UCPEN;              // No parity
    UCA0CTLW0 &= ~UCSYNC;             // Asynchronous (UART)
    UCA0CTLW0 &= ~UC7BIT;             // 8-bit data
    UCA0CTLW0 |=  UCMODE_0;           // UART mode

    // 115,200 baud: UCBRx=4, UCOS16=1, UCFx=5, UCSx=0x55
    UCA0BRW   = 4;
    UCA0MCTLW = 0x5551;

    UCA0CTLW0 &= ~UCSWRST;            // Release from reset
    UCA0TXBUF  = 0x00;                // Prime the TX pump (sets TXIFG)
    UCA0IE    |= UCRXIE;              // Enable RX interrupt
}

//==============================================================================
// Init_Serial_UCA1 -- Configure eUSCI_A1 for UART (PC backchannel J14)
//   P4.2 = UCA1RXD, P4.3 = UCA1TXD (configured in ports.c)
//   Default: 115,200 baud, 8-N-1, SMCLK source
//==============================================================================
void Init_Serial_UCA1(void){
    UCA1CTLW0  = UCSWRST;             // Hold in reset during config
    UCA1CTLW0 |= UCSSEL__SMCLK;       // BRCLK = SMCLK = 8 MHz
    UCA1CTLW0 &= ~UCMSB;              // LSB first
    UCA1CTLW0 &= ~UCSPB;              // 1 stop bit
    UCA1CTLW0 &= ~UCPEN;              // No parity
    UCA1CTLW0 &= ~UCSYNC;             // Asynchronous (UART)
    UCA1CTLW0 &= ~UC7BIT;             // 8-bit data
    UCA1CTLW0 |=  UCMODE_0;           // UART mode

    // 115,200 baud: UCBRx=4, UCOS16=1, UCFx=5, UCSx=0x55
    UCA1BRW   = 4;
    UCA1MCTLW = 0x5551;

    UCA1CTLW0 &= ~UCSWRST;            // Release from reset
    UCA1TXBUF  = 0x00;                // Prime the TX pump (sets TXIFG)
    UCA1IE    |= UCRXIE;              // Enable RX interrupt
}

//==============================================================================
// Set_Baud_Rate -- Change baud rate on both UCA0 and UCA1 simultaneously
//   baud_select: BAUD_115200 (0) or BAUD_460800 (1)
//   Both ports must be placed in reset (UCSWRST) to change baud registers.
//==============================================================================
void Set_Baud_Rate(unsigned int baud_select){
    // --- UCA0 ---
    UCA0CTLW0 |= UCSWRST;
    if(baud_select == BAUD_460800){
        // 460,800 baud: UCBRx=17, UCOS16=0, UCFx=0, UCSx=0x4A
        UCA0BRW   = 17;
        UCA0MCTLW = 0x4A00;
    } else {
        // 115,200 baud (default)
        UCA0BRW   = 4;
        UCA0MCTLW = 0x5551;
    }
    UCA0CTLW0 &= ~UCSWRST;

    // --- UCA1 ---
    UCA1CTLW0 |= UCSWRST;
    if(baud_select == BAUD_460800){
        UCA1BRW   = 17;
        UCA1MCTLW = 0x4A00;
    } else {
        UCA1BRW   = 4;
        UCA1MCTLW = 0x5551;
    }
    UCA1CTLW0 &= ~UCSWRST;
}

//==============================================================================
// Transmit_String_UCA1 -- Load a string and start transmitting via UCA1 TX ISR
//   Copies string into UCA1_transmit_buf, sets UCA1_tx_active, enables UCTXIE.
//   The TX ISR sends one character per TXIFG interrupt until null terminator.
//==============================================================================
void Transmit_String_UCA1(const char *string){
    unsigned int i = BEGINNING;
    while(string[i] != '\0' && i < 24){
        UCA1_transmit_buf[i] = string[i];
        i++;
    }
    UCA1_transmit_buf[i] = '\0';
    uca1_tx_index = BEGINNING;
    UCA1_tx_active = TRUE;
    UCA1IE |= UCTXIE;              // Enable TX interrupt to start sending
}

//==============================================================================
// eUSCI_A0 ISR -- UCA0 (IOT port / J9) RX and TX
//
//   RX (case 2): Character arrived from J9 loopback.
//     - Store in IOT_2_PC ring buffer
//     - Forward directly to UCA1 TXBUF (back to PC)
//     - Capture in received_string for LCD display
//
//   TX (case 4): Not actively used for basic echo; placeholder.
//==============================================================================
#pragma vector = EUSCI_A0_VECTOR
__interrupt void eUSCI_A0_ISR(void){
    char receive_val;
    switch(__even_in_range(UCA0IV, 0x08)){

        case 0:                                // No interrupt
            break;

        case 2:                                // RXIFG -- received from J9
            receive_val = UCA0RXBUF;

            // Store in ring buffer
            IOT_2_PC[iot_rx_wr++] = receive_val;
            if(iot_rx_wr >= SMALL_RING_SIZE){
                iot_rx_wr = BEGINNING;
            }

            // Forward to PC (UCA1 TX)
            UCA1TXBUF = receive_val;

            // Capture for LCD display (up to 10 chars)
            if(received_index < 10){
                received_string[received_index++] = receive_val;
                received_string[received_index]   = '\0';
            }
            break;

        case 4:                                // TXIFG -- not used for basic echo
            break;

        default:
            break;
    }
}

//==============================================================================
// eUSCI_A1 ISR -- UCA1 (PC backchannel / J14) RX and TX
//
//   RX (case 2): Character arrived from PC (Termite).
//     - Store in PC_2_IOT ring buffer
//     - Forward directly to UCA0 TXBUF (out to J9 / IOT)
//
//   TX (case 4): Used when Transmit_String_UCA1 sends a string.
//     - Sends one character per interrupt until null terminator.
//     - Disables UCTXIE when string transmission is complete.
//==============================================================================
#pragma vector = EUSCI_A1_VECTOR
__interrupt void eUSCI_A1_ISR(void){
    char receive_val;
    switch(__even_in_range(UCA1IV, 0x08)){

        case 0:                                // No interrupt
            break;

        case 2:                                // RXIFG -- received from PC
            receive_val = UCA1RXBUF;

            // Store in ring buffer
            PC_2_IOT[pc_rx_wr++] = receive_val;
            if(pc_rx_wr >= SMALL_RING_SIZE){
                pc_rx_wr = BEGINNING;
            }

            // Forward to IOT port (UCA0 TX -> J9 loopback)
            UCA0TXBUF = receive_val;
            break;

        case 4:                                // TXIFG -- transmit next char of string
            if(UCA1_tx_active){
                if(UCA1_transmit_buf[uca1_tx_index] != '\0'){
                    UCA1TXBUF = UCA1_transmit_buf[uca1_tx_index++];
                }
                // Check again (index advanced above)
                if(UCA1_transmit_buf[uca1_tx_index] == '\0'){
                    UCA1IE    &= ~UCTXIE;  // Done -- disable TX interrupt
                    UCA1_tx_active = FALSE;
                }
            } else {
                UCA1IE &= ~UCTXIE;         // Safety: disable if nothing pending
            }
            break;

        default:
            break;
    }
}
