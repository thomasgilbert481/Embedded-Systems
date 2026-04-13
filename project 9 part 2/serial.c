//==============================================================================
// File:        serial.c
// Description: UCA0 and UCA1 UART serial communication for Project 9 Part 1.
//              Implements a transparent serial bridge (passthrough) between
//              the PC (UCA1) and ESP32 IOT module (UCA0).
//
//              Data path:
//                Termite (PC) -> UCA1 RX ISR -> UCA0 TXBUF -> ESP32
//                ESP32        -> UCA0 RX ISR -> UCA1 TXBUF -> Termite (PC)
//
//              UCA0 (IOT port -- J9):
//                P1.6 = UCA0RXD  (ESP32 TXD -> Vehicle RX)
//                P1.7 = UCA0TXD  (ESP32 RXD <- Vehicle TX)
//
//              UCA1 (PC backchannel):
//                P4.2 = UCA1RXD  (PC TX -> Vehicle RX)
//                P4.3 = UCA1TXD  (PC RX <- Vehicle TX)
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
// External globals (LCD display -- defined in LCD.obj)
//==============================================================================
extern char                  display_line[4][11]; // LCD line buffers
extern volatile unsigned char display_changed;    // Set to signal LCD refresh

//==============================================================================
// Module global definitions (declared extern in serial.h)
//==============================================================================

// IOT ring buffer -- filled by eUSCI_A0_ISR
volatile char         IOT_Ring_Rx[IOT_RING_SIZE];
volatile unsigned int iot_rx_wr = BEGINNING;

// USB ring buffer -- filled by eUSCI_A1_ISR
volatile char         USB_Ring_Rx[USB_RING_SIZE];
volatile unsigned int usb_rx_wr = BEGINNING;

// PC TX gate: blocks all FRAM->PC output until PC sends first character
volatile unsigned char pc_ok_to_tx = FALSE;

// Main-loop read index for IOT ring buffer (non-volatile)
         unsigned int iot_rx_rd = BEGINNING;

// TX buffer: loaded by Serial_Transmit(), drained by TX ISR.
// Sized for full AT commands (e.g. "AT+CIPSERVER=1,65535\r\n" + null).
char         iot_TX_buf[IOT_TX_BUF_SIZE];
unsigned int iot_tx    = BEGINNING;                // Index into iot_TX_buf

// Legacy short-command storage (kept for backward compatibility, unused in P9P2)
char                  received_command[SERIAL_MSG_LENGTH + 1];
volatile unsigned char command_ready = 0;

// Active baud rate index
unsigned char baud_rate_index = BAUD_115200;       // IOT default 115,200

// Multi-line ESP32 response buffer (filled by IOT_Process, scanned by state machine)
char         IOT_Data[IOT_DATA_LINES][IOT_DATA_COLS];
unsigned int iot_data_line  = BEGINNING;
static unsigned int iot_data_col = BEGINNING;

//------------------------------------------------------------------------------
// Init_Serial_UCA0 -- IOT serial port (J9), P1.6=RXD, P1.7=TXD
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
// Init_Serial_UCA1 -- PC backchannel, P4.2=RXD, P4.3=TXD
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
// Description: Drains IOT_Ring_Rx into IOT_Data[][] line buffer.
//              Each LF (0x0A) finishes a row; the row is null-terminated and
//              iot_data_line advances.  CR (0x0D) is ignored (not stored).
//              IOT_Data rows are scanned by the IOT state machine.
//==============================================================================
void IOT_Process(void){
    unsigned int iot_rx_wr_snap;
    char         incoming_byte;

    iot_rx_wr_snap = iot_rx_wr;

    while(iot_rx_wr_snap != iot_rx_rd){
        incoming_byte = IOT_Ring_Rx[iot_rx_rd++];
        if(iot_rx_rd >= IOT_RING_SIZE){
            iot_rx_rd = BEGINNING;
        }

        if(incoming_byte == SERIAL_CR){
            // Skip CR -- LF terminates the line
            iot_rx_wr_snap = iot_rx_wr;
            continue;
        }

        if(incoming_byte == SERIAL_LF){
            // End of line -- null-terminate and advance row
            if(iot_data_col < IOT_DATA_COLS){
                IOT_Data[iot_data_line][iot_data_col] = SERIAL_NULL;
            } else {
                IOT_Data[iot_data_line][IOT_DATA_COLS - 1] = SERIAL_NULL;
            }
            // Only advance to a new row if the line had content -- ignore blank lines
            if(iot_data_col > BEGINNING){
                iot_data_line++;
                if(iot_data_line >= IOT_DATA_LINES){
                    iot_data_line = BEGINNING;
                }
                IOT_Data[iot_data_line][0] = SERIAL_NULL;  // Clear next row
            }
            iot_data_col = BEGINNING;
        } else {
            if(iot_data_col < (IOT_DATA_COLS - 1)){
                IOT_Data[iot_data_line][iot_data_col++] = incoming_byte;
                IOT_Data[iot_data_line][iot_data_col]   = SERIAL_NULL;
            }
            // else: line too long, silently drop the rest until LF
        }

        iot_rx_wr_snap = iot_rx_wr;
    }
}

//==============================================================================
// Function: Serial_Transmit
// Description: Copies a null-terminated string into iot_TX_buf and kicks the
//              UCA0 TX ISR to drain it.  The caller is responsible for any
//              trailing CR+LF (AT command strings include them).
//==============================================================================
void Serial_Transmit(char *msg){
    unsigned int i;

    for(i = BEGINNING; (i < (IOT_TX_BUF_SIZE - 1)) && (msg[i] != SERIAL_NULL); i++){
        iot_TX_buf[i] = msg[i];
    }
    iot_TX_buf[i] = SERIAL_NULL;

    iot_tx = BEGINNING;

    if(iot_TX_buf[iot_tx] != SERIAL_NULL){
        UCA0TXBUF = iot_TX_buf[iot_tx++];
        UCA0IE   |= UCTXIE;
    }
}

//==============================================================================
// Function: USB_transmit_string
// Description: Transmit a null-terminated string to the PC via UCA1 (polled).
//              Respects the pc_ok_to_tx gate -- silently drops the string if
//              the PC has not yet sent its first character.
//==============================================================================
void USB_transmit_string(const char *str){
    unsigned int i = BEGINNING;
    if(!pc_ok_to_tx){
        return;
    }
    while(str[i] != SERIAL_NULL){
        while(!(UCA1IFG & UCTXIFG));   // Wait for TX buffer empty
        UCA1TXBUF = str[i++];
    }
}

//==============================================================================
// Function: Update_Baud_Display
// Description: P9P2 -- LCD line 3 is reserved for the IP address; no baud
//              indication on the screen.  Kept as a no-op for FRAM ^F/^S.
//==============================================================================
void Update_Baud_Display(void){
    // intentional no-op
}

//==============================================================================
// Function: Clear_Serial_Buffers
//==============================================================================
void Clear_Serial_Buffers(void){
    UCA0IE &= ~UCTXIE;

    iot_rx_wr = BEGINNING;
    iot_rx_rd = BEGINNING;

    iot_tx = BEGINNING;

    iot_data_line = BEGINNING;
    iot_data_col  = BEGINNING;
    IOT_Data[0][0] = SERIAL_NULL;

    command_ready = 0;
}

//------------------------------------------------------------------------------
// eUSCI_A0_ISR -- IOT serial port (J9)
// RX: character arrived from ESP32 -> store in ring buffer -> forward to PC
// TX: not used in passthrough mode (polling TX used instead)
//------------------------------------------------------------------------------
#pragma vector = EUSCI_A0_VECTOR
__interrupt void eUSCI_A0_ISR(void){
  char iot_receive;

  switch(__even_in_range(UCA0IV, 0x08)){
    case 0: break;                        // No interrupt

    case 2:{                              // RX -- character from ESP32
      iot_receive = UCA0RXBUF;
      P6OUT ^= GRN_LED;                  // DEBUG: toggle GRN every ESP32 byte

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

    case 4:{                              // TX -- drain iot_TX_buf to ESP32
      if(iot_TX_buf[iot_tx] != SERIAL_NULL){
        UCA0TXBUF = iot_TX_buf[iot_tx++];
      } else {
        UCA0IE &= ~UCTXIE;                // Buffer exhausted, disable TX IE
        iot_tx  = BEGINNING;
      }
    }break;

    default: break;
  }
}

//------------------------------------------------------------------------------
// eUSCI_A1_ISR -- PC backchannel
// RX: character arrived from PC.
//     - First byte unlocks pc_ok_to_tx (PC->FRAM gate).
//     - If byte is '^', start collecting a FRAM-only command (^^, ^F, ^S);
//       these are consumed by the FRAM and NEVER forwarded to the ESP32.
//     - Otherwise, passthrough to IOT (UCA0) and echo to PC.
// TX: not used in passthrough mode (polling TX used instead).
//------------------------------------------------------------------------------
// Module-scope state for FRAM '^' command assembly
static volatile unsigned char fram_cmd_active = FALSE;

#pragma vector = EUSCI_A1_VECTOR
__interrupt void eUSCI_A1_ISR(void){
  char usb_value;

  switch(__even_in_range(UCA1IV, 0x08)){
    case 0: break;                        // No interrupt

    case 2:{                              // RX -- character received from PC
      usb_value = UCA1RXBUF;

      // First character from PC unlocks bidirectional TX
      if(!pc_ok_to_tx){
        pc_ok_to_tx = TRUE;
      }

      // Store in USB ring buffer (bookkeeping, not required for passthrough)
      USB_Ring_Rx[usb_rx_wr++] = usb_value;
      if(usb_rx_wr >= USB_RING_SIZE){
        usb_rx_wr = BEGINNING;
      }

      //----------------------------------------------------------------------
      // FRAM command detection: '^' prefix.
      // ^^ -> FRAM responds "I'm here"
      // ^F -> switch UCA0 to 115,200 baud
      // ^S -> switch UCA0 to   9,600 baud
      // These bytes are consumed by the FRAM and NOT forwarded to the ESP32.
      //
      // Order matters: dispatch FIRST when already collecting, so that the
      // second '^' of "^^" lands in the dispatch (FRAM_CMD_PING) instead of
      // re-arming fram_cmd_active.
      //----------------------------------------------------------------------
      if(fram_cmd_active){
        fram_cmd_active = FALSE;
        // Echo the command character to Termite
        while(!(UCA1IFG & UCTXIFG));
        UCA1TXBUF = usb_value;

        switch(usb_value){
          case FRAM_CMD_PING:
            USB_transmit_string("\r\nI'm here\r\n");
            break;
          case FRAM_CMD_FAST:
            Clear_Serial_Buffers();
            Init_Serial_UCA0(BAUD_115200);
            baud_rate_index = BAUD_115200;
            Update_Baud_Display();
            USB_transmit_string("\r\nUCA0: 115,200\r\n");
            break;
          case FRAM_CMD_SLOW:
            Clear_Serial_Buffers();
            Init_Serial_UCA0(BAUD_9600);
            baud_rate_index = BAUD_9600;
            Update_Baud_Display();
            USB_transmit_string("\r\nUCA0: 9,600\r\n");
            break;
          default:
            // Unknown ^ command -- ignore, do NOT forward to ESP32
            break;
        }
        break;                            // FRAM command handled, do not forward
      }

      if(usb_value == SERIAL_CARET){
        fram_cmd_active = TRUE;
        // Echo the '^' so the operator sees it in Termite
        while(!(UCA1IFG & UCTXIFG));
        UCA1TXBUF = usb_value;
        break;                            // Do NOT forward '^' to ESP32
      }

      //----------------------------------------------------------------------
      // Normal passthrough: forward character to IOT (UCA0)
      //----------------------------------------------------------------------
      while(!(UCA0IFG & UCTXIFG));
      UCA0TXBUF = usb_value;

      // Echo back to PC so operator sees what they typed
      while(!(UCA1IFG & UCTXIFG));
      UCA1TXBUF = usb_value;
    }break;

    case 4: break;                        // TX -- not used in passthrough mode

    default: break;
  }
}
