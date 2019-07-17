#include <stdint.h>
#include "SysTickInts.h"
#include "LCD.h"
#include "ADC14.h"
#include "msp432p401r.h"

uint32_t ADC_to_Int(uint32_t ADC);

void main() {

    uint32_t period = 75075; //40Hz or 25ms
    char Units[] = "cm";
    char EGCP[] = " EGCP 450 Lab 7";
    uint32_t Annie = 0;                 //Temp var for output
    uint32_t Addy = 0;                  //Var to test mailbox status

    SysTick_Init(period);               //Initializations
    LCD_Init();
    ADC0_InitSWTriggerCh0();

    LCD_OutString(EGCP);                //Set Banner on LCD
    LCD_OutCmd(0xC0);
    LCD_OutUFix(0);
    LCD_OutString(Units);

    while(1){
        Addy = SysTick_ADCStatus();
        if(Addy == 1){
            Annie = SysTick_Mailbox();
            Annie = ADC_to_Int(Annie);
            for(int i=0; i<7; i++)
                LCD_OutCmd(0x10);
            LCD_OutUFix(Annie);
            LCD_OutString(Units);
            Clear_All();
        }
    }
}

uint32_t ADC_to_Int(uint32_t ADC){
// Slide Potentiometer is 7.303cm.
// 16383 is highest output for ADC.
    if (ADC <= 30){
        return 0;
    }
    else{
        ADC *= 100;
        ADC /= 224;
        return ADC;
    }
}

