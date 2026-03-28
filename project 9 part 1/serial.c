//==============================================================================
// File:        serial.c
// Description: UCA0 and UCA1 UART serial communication for Project 9 Part 1.
//              Implements a transparent serial bridge (passthrough) between
//              the PC (UCA1) and ESP32 IOT module (UCA0).
//
//              Data path:
//                Termite (PC) → UCA1 RX ISR → UCA0 TXBUF → ESP32
//                ESP32        → UCA0 RX ISR → UCA1 TXBUF → Termite (PC)
//
//              UCA0 (IOT port — J9):
//                P1.6 = UCA0RXD  (ESP32 TXD → Vehicle RX)
//                P1.7 = UCA0TXD  (ESP32 RXD ← Vehicle TX)
//
//              UCA1 (PC backchannel):
//                P4.2 = UCA1RXD  (PC TX → Vehicle RX)
//                P4.3 = UCA1TXD  (PC RX ← Vehicle TX)
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

// IOT ring buffer — filled by eUSCI_A0_ISR
volatile char         IOT_Ring_Rx[IOT_RING_SIZE];
volatile unsigned int iot_rx_wr = BEGINNING;

// USB ring buffer — filled by eUSCI_A1_ISR
volatile char         USB_Ring_Rx[USB_RING_SIZE];
volatile unsigned int usb_rx_wr = BEGINNING;

// PC TX gate: blocks all FRAM→PC output until PC sends first character
volatile unsigned char pc_ok_to_tx = FALSE;

// Main-loop read index for IOT ring buffer (non-volatile)
         unsigned int iot_rx_rd = BEGINNING;

// TX buffer: loaded by Serial_Transmit(), drained by TX ISR
char         iot_TX_buf[SERIAL_MSG_LENGTH + 2];   // +2 for CR + LF terminator
unsigned int iot_tx    = BEGINNING;                // Index into iot_TX_buf

// Assembled received command (null-terminated, SERIAL_MSG_LENGTH chars max)
char                  received_command[SERIAL_MSG_LENGTH + 1];
volatile unsigned char command_ready = 0;          // 1 when full command is assembled

// Active baud rate index
unsigned char baud_rate_index = BAUD_115200;       // Start at 115,200 baud for IOT

// Character count used while assembling a command from individual ring-buffer bytes.
static unsigned int rx_char_count = BEGINNING;

//------------------------------------------------------------------------------
// Init_Serial_UCA0 — IOT serial port (J9), P1.6=RXD, P1.7=TXD
// speed: BAUD_115200 (0) or BAUD_9600 (1)
//
// Baud rate register values (SMCLK = 8 MHz):
//   115200: BRW=4,  MCTLW=0x5551
//   9600:   BRW=52, MCTLW=0x4911
//------------------------------------------------------------------------------
void Init_Serial_UCA0(char speed){
  UCA0CTLW0  =  UCSWRST;           // Hold in reset during config
  UCA0CTLW0 |=  UCSSEL__SMCLK;     // Clock = SMCLK (8 MHz)
  UCA0CTLW0 &= ~UCMSB;             // LSB first
  UCA0CTLW0 &= ~UCSPB;             // 1 stop bit
  UCA0CTLW0 &= ~UCPEN;             // No parity
  UCA0CTLW0 &= ~UCSYNC;            // Async (UART) mode
  UCA0CTLW0 &= ~UC7BIT;            // 8 data bits
  UCA0CTLW0 |=  UCMODE_0;          // UART mode

  if(speed == BAUD_9600){
    UCA0BRW   = 52;
    UCA0MCTLW = 0x4911;
  } else {                          // Default: 115200
    UCA0BRW   = 4;
    UCA0MCTLW = 0x5551;
  }

  UCA0CTLW0 &= ~UCSWRST;           // Release from reset
  UCA0TXBUF  =  0x00;              // Prime the pump
  UCA0IE    |=  UCRXIE;            // Enable RX interrupt
}

//------------------------------------------------------------------------------
// Init_Serial_UCA1 — PC backchannel, P4.2=RXD, P4.3=TXD
// speed: BAUD_115200 (0) or BAUD_9600 (1)
//------------------------------------------------------------------------------
void Init_Serial_UCA1(char speed){
  pc_ok_to_tx = FALSE;              // Block TX until PC sends first char

  UCA1CTLW0  =  UCSWRST;           // Hold in reset during config
  UCA1CTLW0 |=  UCSSEL__SMCLK;     // Clock = SMCLK (8 MHz)
  UCA1CTLW0 &= ~UCMSB;             // LSB first
  UCA1CTLW0 &= ~UCSPB;             // 1 stop bit
  UCA1CTLW0 &= ~UCPEN;             // No parity
  UCA1CTLW0 &= ~UCSYNC;            // Async (UART) mode
  UCA1CTLW0 &= ~UC7BIT;            // 8 data bits
  UCA1CTLW0 |=  UCMODE_0;          // UART mode

  if(speed == BAUD_9600){
    UCA1BRW   = 52;
    UCA1MCTLW = 0x4911;
  } else {                          // Default: 115200
    UCA1BRW   = 4;
    UCA1MCTLW = 0x5551;
  }

  UCA1CTLW0 &= ~UCSWRST;           // Release from reset
  UCA1TXBUF  =  0x00;              // Prime the pump
  UCA1IE    |=  UCRXIE;            // Enable RX interrupt
}

//==============================================================================
// Function: IOT_Process
// Description: Reads one byte per call from IOT_Ring_Rx and assembles it into
//              received_command[]. Called from the main while() loop.
//==============================================================================
void IOT_Process(void){
    unsigned int  iot_rx_wr_snap;
    char          incoming_byte;

    iot_rx_wr_snap = iot_rx_wr;

    if(iot_rx_wr_snap != iot_rx_rd){
        incoming_byte = IOT_Ring_Rx[iot_rx_rd++];

        if(iot_rx_rd >= IOT_RING_SIZE){
            iot_rx_rd = BEGINNING;
        }

        if((incoming_byte == SERIAL_CR) || (incoming_byte == SERIAL_LF)){
            if(rx_char_count > BEGINNING){
                received_command[rx_char_count] = SERIAL_NULL;
                rx_char_count = BEGINNING;
                command_ready = 1;
            }
            return;
        }

        if(rx_char_count < SERIAL_MSG_LENGTH){
            received_command[rx_char_count++] = incoming_byte;
        }

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
//              interrupt-driven transmission on UCA0.
//==============================================================================
void Serial_Transmit(char *msg){
    unsigned int i;

    for(i = BEGINNING; (i < SERIAL_MSG_LENGTH) && (msg[i] != SERIAL_NULL); i++){
        iot_TX_buf[i] = msg[i];
    }
    iot_TX_buf[i++] = SERIAL_CR;
    iot_TX_buf[i++] = SERIAL_LF;
    iot_TX_buf[i]   = SERIAL_NULL;

    iot_tx = BEGINNING;

    UCA0TXBUF = iot_TX_buf[iot_tx++];

    UCA0IE |= UCTXIE;
}

//==============================================================================
// Function: Update_Baud_Display
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
//==============================================================================
void Clear_Serial_Buffers(void){
    UCA0IE &= ~UCTXIE;

    iot_rx_wr = BEGINNING;
    iot_rx_rd = BEGINNING;

    iot_tx = BEGINNING;

    rx_char_count = BEGINNING;
    command_ready = 0;
}

//------------------------------------------------------------------------------
// eUSCI_A0_ISR — IOT serial port (J9)
// RX: character arrived from ESP32 → store in ring buffer → forward to PC
// TX: not used in passthrough mode (polling TX used instead)
//------------------------------------------------------------------------------
#pragma vector = EUSCI_A0_VECTOR
__interrupt void eUSCI_A0_ISR(void){
  char iot_receive;

  switch(__even_in_range(UCA0IV, 0x08)){
    case 0: break;                        // No interrupt

    case 2:{                              // RX — character from ESP32
      iot_receive = UCA0RXBUF;

      // Store in IOT ring buffer
      IOT_Ring_Rx[iot_rx_wr++] = iot_receive;
      if(iot_rx_wr >= IOT_RING_SIZE){
        iot_rx_wr = BEGINNING;
      }

      // Forward to PC only if PC gate is open
      if(pc_ok_to_tx){
        while(!(UCA1IFG & UCTXIFG));
        UCA1TXBUF = iot_receive;
      }
    }break;

    case 4: break;                        // TX — not used in passthrough mode

    default: break;
  }
}

//------------------------------------------------------------------------------
// eUSCI_A1_ISR — PC backchannel
// RX: character arrived from PC → passthrough to IOT (UCA0) → echo to PC
//------------------------------------------------------------------------------
#pragma vector = EUSCI_A1_VECTOR
__interrupt void eUSCI_A1_ISR(void){
  char usb_value;

  switch(__even_in_range(UCA1IV, 0x08)){
    case 0: break;                        // No interrupt

    case 2:{                              // RX — character received from PC
      usb_value = UCA1RXBUF;

      // First character from PC unlocks bidirectional TX
      if(!pc_ok_to_tx){
        pc_ok_to_tx = TRUE;
      }

      // Store in USB ring buffer
      USB_Ring_Rx[usb_rx_wr++] = usb_value;
      if(usb_rx_wr >= USB_RING_SIZE){
        usb_rx_wr = BEGINNING;
      }

      // Passthrough: forward character to IOT (UCA0)
      // Poll — character must go out before ISR exits
      while(!(UCA0IFG & UCTXIFG));
      UCA0TXBUF = usb_value;

      // Echo back to PC so operator sees what they typed
      // Only echo if UCA1 TX buffer is ready
      if(pc_ok_to_tx){
        while(!(UCA1IFG & UCTXIFG));
        UCA1TXBUF = usb_value;
      }
    }break;

    case 4:{                              // TX — not used in passthrough mode
    }break;

    default: break;
  }
}
