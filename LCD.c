/*
  size is 1*16
  if do not need to read busy, then you can tie R/W=ground
  ground = pin 1    Vss
  power  = pin 2    Vdd   +3.3V or +5V depending on the device
  ground = pin 3    Vlc   grounded for highest contrast
  P6.4   = pin 4    RS    (1 for data, 0 for control/status)
  ground = pin 5    R/W   (1 for read, 0 for write)
  P6.5   = pin 6    E     (enable)
  P4.4   = pin 11   DB4   (4-bit data)
  P4.5   = pin 12   DB5
  P4.6   = pin 13   DB6
  P4.7   = pin 14   DB7
16 characters are configured as 1 row of 16
addr  00 01 02 03 04 05 ... 0F
*/

#include <stdint.h>
#include "LCD.h"
#include "SysTickInts.h"
#include "msp.h"

// Marcros
#define T6us 20              // 6us
#define T40us 120            // 40us
#define T160us 480         // 160us
#define T1ms 1000          // 1ms
#define T1600us 2000        // 1.60ms
#define T5ms 7000          // 5ms
#define T15ms 10000         // 15ms

// Global Vars
uint8_t LCD_RS, LCD_E;              // LCD Enable and Register Select
char Num_Out[17];                   // Character string for outputting numbers to LCD
char Hex_Out[10];
char Fix_Out[6];
int i_Dec = 0;                      // integer for strings
int i_Hex = 0;
int Comma_Space = 0;                // used to place commas in strings
int Overflow = 0;                   // used for fixed point format output
char lcd_digit;                     // character for string input

/**************** Private Functions ****************/

void OutPort6() {
    P6OUT = (LCD_RS<<4) | (LCD_E<<5);
}

void SendPulse() {
    OutPort6();
    Delay(T6us);             // wait 6us
    LCD_E = 1;                      // E=1, R/W=0, RS=1
    OutPort6();
    Delay(T6us);             // wait 6us
    LCD_E = 0;                      // E=0, R/W=0, RS=1
    OutPort6();
}

void SendChar() {
    LCD_E = 0;
    LCD_RS = 1;                     // E=0, R/W=0, RS=1
    SendPulse();
    Delay(T1600us);          // wait 1.6ms
}

void SendCmd() {
    LCD_E = 0;
    LCD_RS = 0;                     // E=0, R/W=0, RS=0
    SendPulse();
    Delay(T40us);            // wait 40us
}

/**************** Public Functions ****************/
// Clear the LCD
// Inputs: none
// Outputs: none
void LCD_Clear() {
    LCD_OutCmd(0x01);               // Clear Display
    LCD_OutCmd(0x80);               // Move cursor back to 1st position
}

// Initialize LCD
// Inputs: none
// Outputs: none
void LCD_Init() {
    P4SEL0 &= ~0xF0;
    P4SEL1 &= ~0xF0;                // configure upper nibble of P4 as GPIO
    P4DIR |= 0xF0;                  // make upper nibble of P4 out

    P6SEL0 &= ~0x30;
    P6SEL1 &= ~0x30;                // configure P6.4 and P6.5 as GPIO
    P6DIR |= 0x30;                  // make P6.4 and P6.5 out

    //Clock_Init48MHz();                // set system clock to 48 MHz
    // SysTick_Init1();                 // Volume 1 Program 4.7, Volume 2 Program 2.12

    LCD_E = 0;
    LCD_RS = 0;                     // E=0, R/W=0, RS=0
    OutPort6();

    LCD_OutCmd(0x30);               // command 0x30 = Wake up
    Delay(T5ms);             // must wait 5ms, busy flag not available
    LCD_OutCmd(0x30);               // command 0x30 = Wake up #2
    Delay(T160us);           // must wait 160us, busy flag not available
    LCD_OutCmd(0x30);               // command 0x30 = Wake up #3
    Delay(T160us);           // must wait 160us, busy flag not available

    LCD_OutCmd(0x28);               // Function set: 4-bit/2-line
    LCD_Clear();
    LCD_OutCmd(0x10);               // Set cursor
    LCD_OutCmd(0x06);               // Entry mode set
}

// Output a character to the LCD
// Inputs: letter is ASCII character, 0 to 0x7F
// Outputs: none
void LCD_OutChar(char letter) {
    unsigned char let_low = (0x0F&letter)<<4;
    unsigned char let_high = 0xF0&letter;

    P4OUT = let_high;
    SendChar();
    P4OUT = let_low;
    SendChar();
    Delay(T1ms);             // wait 1ms
}

// Output a command to the LCD
// Inputs: 8-bit command
// Outputs: none
void LCD_OutCmd(unsigned char command) {
    unsigned char com_low = (0x0F&command)<<4;
    unsigned char com_high = 0xF0&command;
    P4OUT = com_high;
    SendCmd();
    P4OUT = com_low;
    SendCmd();
    Delay(T1ms);             // wait 1ms
}

//------------LCD_OutString------------
// Output String (NULL termination)
// Input: pointer to a NULL-terminated string to be transferred
// Output: none
void LCD_OutString(char *pt) {
    while(*pt != '\0'){
        LCD_OutChar(*pt);
        pt++;
    }
}

//-----------------------LCD_OutUDec-----------------------
// Output a 32-bit number in unsigned decimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1-10 digits with no space before or after
void LCD_OutUDec(uint32_t n) {
// This function uses recursion to convert decimal number
//   of unspecified length as an ASCII string
    if (n>9){
        lcd_digit = (n%10) + '0';
        Num_Out[i_Dec] = lcd_digit;
        i_Dec++;
        LCD_OutUDec(n/10);
    }
    else{
        lcd_digit = (n%10) + '0';
        Num_Out[i_Dec] = lcd_digit;
    }

// This section uses i to place a comma in the proper position on the LCD
    if (i_Dec == 3 || i_Dec == 6 || i_Dec == 9){
        Comma_Space = 1;
    }
    else if (i_Dec == 4 || i_Dec == 7){
        Comma_Space = 2;
    }
    else{
        Comma_Space = 3;
    }

// Output the string backwards and place a comma appropriately
    for (; i_Dec>=0; i_Dec--){
        if (Comma_Space == 0){
            LCD_OutChar(',');
            LCD_OutChar(Num_Out[i_Dec]);
            Comma_Space = 2;
        }
        else{
            LCD_OutChar(Num_Out[i_Dec]);
            Comma_Space--;
        }
    }
}

//--------------------------LCD_OutUHex----------------------------
// Output a 32-bit number in unsigned hexadecimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1 to 8 digits with no space before or after
void LCD_OutUHex(uint32_t number) {
// This function uses recursion to convert the number of
//   unspecified length as an ASCII string
    if (number>0){
        int temp = number%16;
        if (temp >= 10){
            lcd_digit = temp + '7';
            Hex_Out[i_Hex] = lcd_digit;
            i_Hex++;
            LCD_OutUHex(number/16);
        }
        else{
            lcd_digit = temp + '0';
            Hex_Out[i_Hex] = lcd_digit;
            i_Hex++;
            LCD_OutUHex(number/16);
        }
    }

// Outputs the string to the LCD and places a space where necessary
    i_Hex--;

    if (i_Hex >= 4){
        Comma_Space = i_Hex - 3;
    }
    else{
        Comma_Space = 3;
    }

    for (; i_Hex>=0; i_Hex--){
        if (Comma_Space == 0){
            LCD_OutChar(32);
            LCD_OutChar(Hex_Out[i_Hex]);
            Comma_Space = 3;
        }
        else{
            LCD_OutChar(Hex_Out[i_Hex]);
            Comma_Space--;
        }
    }
}

// -----------------------LCD_OutUFix----------------------
// Output characters to LCD display in fixed-point format
// unsigned decimal, resolution 0.001, range 0.000 to 9.999
// Inputs:  an unsigned 32-bit number
// Outputs: none
// E.g., 0,    then output "0.000 "
//       3,    then output "0.003 "
//       89,   then output "0.089 "
//       123,  then output "0.123 "
//       9999, then output "9.999 "
//       9999, then output "*.*** "
void LCD_OutUFix(uint32_t number) {
    if (number >= 10000){           // If over 10,000, output *.***
        Overflow = 1;
    }

// Input number into Fix_Out[] while placing an '0' for the '.' in output
    if (Overflow == 0){
        for (int i=0; i<=4; i++){
            if (i == 3){
                Fix_Out[i] = '0';
            }
            else{
                lcd_digit = (number%10) + '0';
                number /= 10;
                Fix_Out[i] = lcd_digit;
            }
        }
        for (int i=0; i<=4; i++){
            if (i == 1){
                LCD_OutChar('.');
            }
            else{
                LCD_OutChar(Fix_Out[(4-i)]);
            }
        }
    }
    else{
        for (int i=0; i<=4; i++){   // output *.***
            if (i == 1){
                LCD_OutChar('.');
            }
            else{
                LCD_OutChar('*');
            }
        }
    }
}

void Clear_All(void){
    i_Dec = 0;
    i_Hex = 0;
}

void Delay(uint32_t time){
    uint32_t i;
    for( i=0; i<=time; i++){

    }
}
