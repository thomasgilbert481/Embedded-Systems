//==============================================================================
// File:        serial.h
// Description: Declarations for UCA0 serial communication (Project 8).
//              UCA0 is connected to J9 (IOT connector) for AD2 communication.
//                P1.6 = UCA0RXD (receive from AD2)
//                P1.7 = UCA0TXD (transmit to AD2)
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
// Ring buffer (ISR writes, main loop reads via IOT_Process)
//==============================================================================
extern volatile char         IOT_Ring_Rx[IOT_RING_SIZE]; // Circular receive buffer
extern volatile unsigned int iot_rx_wr;                   // ISR write index (volatile)
extern          unsigned int iot_rx_rd;                   // Main-loop read index

//==============================================================================
// TX buffer (loaded by Serial_Transmit, drained by TX ISR)
//==============================================================================
extern char         iot_TX_buf[SERIAL_MSG_LENGTH + 2];   // +2 for CR/LF
extern unsigned int iot_tx;                               // Current TX index

//==============================================================================
// Received command storage
//==============================================================================
extern char                  received_command[SERIAL_MSG_LENGTH + 1]; // Assembled command
extern volatile unsigned char command_ready;   // Set to 1 when full command received

//==============================================================================
// Baud rate state
//==============================================================================
extern unsigned char baud_rate_index;          // Current baud rate (BAUD_9600 or BAUD_115200)

//==============================================================================
// Function prototypes
//==============================================================================
void Init_Serial_UCA0(unsigned char baud_index); // Initialize UCA0 at given baud rate
void IOT_Process(void);                          // Drain ring buffer, assemble commands
void Serial_Transmit(char *msg);                 // Load TX buffer and start ISR-driven TX
void Update_Baud_Display(void);                  // Write baud rate string to LCD line 3

#endif /* SERIAL_H_ */
