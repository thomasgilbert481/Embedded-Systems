//==============================================================================
// File: functions.h
// Description: Function prototypes for Project 8 — Serial Communication
// Author: Thomas Gilbert
// Date: Mar 2026
// Compiler: Code Composer Studio
// Target: MSP430FR2355
//==============================================================================

// Main
void main(void);

// Initialization
void Init_Conditions(void);

// Interrupts
void enable_interrupts(void);

// Timer B0 ISRs (interrupts_timers.c)
__interrupt void Timer0_B0_ISR(void);   // CCR0: 200 ms display update tick
__interrupt void TIMER0_B1_ISR(void);   // CCR1/CCR2: SW1/SW2 debounce timers

// Port ISRs (interrupt_ports.c)
__interrupt void switch1_interrupt(void); // PORT4_VECTOR: SW1 (P4.1) — transmit
__interrupt void switch2_interrupt(void); // PORT2_VECTOR: SW2 (P2.3) — baud cycle

// Debounce counters (defined in interrupts_timers.c)
extern volatile unsigned int sw1_debounce_count;
extern volatile unsigned int sw2_debounce_count;

// Clocks
void Init_Clocks(void);

// LCD (from LCD.obj — Carlson's library)
void Display_Process(void);
void Display_Update(char p_L1, char p_L2, char p_L3, char p_L4);
void enable_display_update(void);
void update_string(char *string_data, int string);
void Init_LCD(void);
void lcd_clear(void);
void lcd_putc(char c);
void lcd_puts(char *s);
void lcd_power_on(void);
void lcd_write_line1(void);
void lcd_write_line2(void);
void lcd_enter_sleep(void);
void lcd_exit_sleep(void);
void Write_LCD_Ins(char instruction);
void Write_LCD_Data(char data);
void ClrDisplay(void);
void ClrDisplay_Buffer_0(void);
void ClrDisplay_Buffer_1(void);
void ClrDisplay_Buffer_2(void);
void ClrDisplay_Buffer_3(void);
void SetPostion(char pos);
void DisplayOnOff(char data);
void lcd_BIG_mid(void);
void lcd_BIG_bot(void);
void lcd_120(void);
void lcd_4line(void);
void lcd_out(char *s, char line, char position);
void lcd_rotate(char view);
void lcd_write(unsigned char c);
void lcd_write_line3(void);
void lcd_command(char data);
void LCD_test(void);
void LCD_iot_meassage_print(int nema_index);

// Ports
void Init_Ports(void);
void Init_Port1(void);
void Init_Port2(void);
void Init_Port3(void);
void Init_Port4(void);
void Init_Port5(void);
void Init_Port6(void);

// SPI (LCD driver)
void Init_SPI_B1(void);
void SPI_B1_write(char byte);
void spi_rs_data(void);
void spi_rs_command(void);
void spi_LCD_idle(void);
void spi_LCD_active(void);
void SPI_test(void);
void WriteIns(char instruction);
void WriteData(char data);

// Switches
void Switches_Process(void);

// Timers
void Init_Timers(void);
void Init_Timer_B0(void);

// Serial communication (serial.c / serial.h)
// (prototypes also in serial.h — include either header)
void Init_Serial_UCA0(unsigned char baud_index);
void IOT_Process(void);
void Serial_Transmit(char *msg);
void Update_Baud_Display(void);
