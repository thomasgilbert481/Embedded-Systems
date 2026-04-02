//==============================================================================
// File:        serial.h
// Description: Declarations for UCA0 and UCA1 serial communication
//              (Project 9 Part 1 -- IOT Passthrough).
//              UCA0 is connected to J9 (IOT connector) for ESP32 communication.
//                P1.6 = UCA0RXD (receive from ESP32)
//                P1.7 = UCA0TXD (transmit to ESP32)
//              UCA1 is connected to PC backchannel.
//                P4.2 = UCA1RXD (receive from PC)
//                P4.3 = UCA1TXD (transmit to PC)
//
// Author: Thomas Gilbert
// Date: Mar 2026
// Compiler: Code Composer Studio
// Target: MSP430FR2355
//==============================================================================

#ifndef SERIAL_H_
#define SERIAL_H_

#include "msp430.h"
#include "macros.h"

//==============================================================================
// IOT ring buffer (UCA0 receive path -- ESP32 -> FRAM)
//==============================================================================
extern volatile char         IOT_Ring_Rx[IOT_RING_SIZE];
extern volatile unsigned int iot_rx_wr;

//==============================================================================
// USB ring buffer (UCA1 receive path -- PC -> FRAM)
//==============================================================================
extern volatile char         USB_Ring_Rx[USB_RING_SIZE];
extern volatile unsigned int usb_rx_wr;

//==============================================================================
// PC TX gate -- FALSE until PC sends first character
//==============================================================================
extern volatile unsigned char pc_ok_to_tx;

//==============================================================================
// TX buffer (loaded by Serial_Transmit, drained by TX ISR)
//==============================================================================
extern char         iot_TX_buf[SERIAL_MSG_LENGTH + 2];
extern unsigned int iot_tx;

//==============================================================================
// Received command storage
//==============================================================================
extern char                  received_command[SERIAL_MSG_LENGTH + 1];
extern volatile unsigned char command_ready;

//==============================================================================
// Baud rate state
//==============================================================================
extern unsigned char baud_rate_index;

//==============================================================================
// Function prototypes
//==============================================================================
void Init_Serial_UCA0(char speed);
void Init_Serial_UCA1(char speed);
void IOT_Process(void);
void Serial_Transmit(char *msg);
void Update_Baud_Display(void);
void Clear_Serial_Buffers(void);

#endif /* SERIAL_H_ */
