/* --COPYRIGHT--,BSD_EX
 * Copyright (c) 2014, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************
 * 
 *                       MSP430 CODE EXAMPLE DISCLAIMER
 *
 * MSP430 code examples are self-contained low-level programs that typically
 * demonstrate a single peripheral function or device feature in a highly
 * concise manner. For this the code may rely on the device's power-on default
 * register values and settings such as the clock configuration and care must
 * be taken when combining code from several examples to avoid potential side
 * effects. Also see www.ti.com/grace for a GUI- and www.ti.com/msp430ware
 * for an API functional library-approach to peripheral configuration.
 *
 * --/COPYRIGHT--*/
//******************************************************************************
//  MSP430FR2422 Demo - ADC, Sample A1, AVcc Ref, Set LED if A1 > 0.5*AVcc
//
//  Description: This example works on Single-Channel Single-Conversion Mode.
//  A single sample is made on A1 with default reference to AVcc.
//  Software sets ADCSC to start sample and conversion - ADCSC automatically
//  cleared at EOC. ADC internal oscillator times sample (16x) and conversion.
//  In Mainloop MSP430 waits in LPM0 to save power until ADC conversion complete,
//  ADC_ISR will force exit from LPM0 in Mainloop on reti.
//  If A1 > 0.5*AVcc, P1.0 set, else reset.
//  ACLK = default REFO ~32768Hz, MCLK = SMCLK = default DCODIV ~1MHz.
//
//               MSP430FR2422
//            -----------------
//        /|\|                 |
//         | |                 |
//         --|RST              |
//           |                 |
//       >---|P1.1/A1      P1.0|--> LED
//
//
//  Ling Zhu
//  Texas Instruments Inc.
//  May 2017
//  Built with IAR Embedded Workbench v6.50 & Code Composer Studio v7.1.0
//******************************************************************************
#include <msp430.h>
#include "driverlib.h"
#include <math.h>
unsigned int ADC_Result;
volatile uint16_t adc_values[4];
volatile uint16_t adc_sum[4]={0};
volatile uint8_t adc_index = 0;
volatile uint8_t adc_done = 0;
#define ADC_SAMPLE_TIMES 5
uint8_t adc_sample_count = 0;
const uint8_t adc_channels[4] = {
    ADC_INPUT_A0, // P1.0
    ADC_INPUT_A1, // P1.1
    ADC_INPUT_A2, // P1.2
    ADC_INPUT_A3  // P1.3
};
#define TABLE_LENGTH 110
//const uint32_t res_table[TABLE_LENGTH] = {
//    321140, 290679, 250886 , 217106 ,198530, 124690, 80650, 53300, 35840, 24680, 17250, 12260, 8860, 6500
//};
//const uint32_t temp_table[TABLE_LENGTH] = {
//    0, 2000, 5000, 8000,10000, 20000, 30000, 40000, 50000, 60000, 70000, 80000, 90000, 100000
//};
int16_t temp_table[TABLE_LENGTH] = {
    -50, -40, -30, -20, -10,
    0, 10, 20, 30, 40,
    50, 60, 70, 80, 90,
    100, 110, 120, 130, 140,
    150, 160, 170, 180, 190,
    200, 210, 220, 230, 240,
    250, 260, 270, 280, 290,
    300, 310, 320, 330, 340,
    350, 360, 370, 380, 390,
    400, 410, 420, 430, 440,
    450, 460, 470, 480, 490,
    500, 510, 520, 530, 540,
    550, 560, 570, 580, 590,
    600, 610, 620, 630, 640,
    650, 660, 670, 680, 690,
    700, 710, 720, 730, 740,
    750, 760, 770, 780, 790,
    800, 810, 820, 830, 840,
    850, 860, 870, 880, 890,
    900, 910, 920, 930, 940,
    950, 960, 970, 980, 990,
    1000, 1010, 1020, 1030, 1040
};
uint16_t res_table[TABLE_LENGTH] = {
    42834, 40620, 38533, 36566, 34710,
    32740, 31110, 29580, 28120, 26750,
    25450, 24220, 23060, 21960, 20920,
    19940, 19000, 18120, 17280, 16490,
    15730, 15020, 14340, 13690, 13080,
    12500, 11950, 11420, 10930, 10450,
    10000, 9570, 9160, 8770, 8400,
    8050, 7720, 7400, 7090, 6800,
    6520, 6260, 6010, 5760, 5530,
    5320, 5110, 4910, 4710, 4530,
    4360, 4190, 4030, 3880, 3730,
    3590, 3460, 3330, 3200, 3090,
    2970, 2870, 2760, 2660, 2570,
    2480, 2390, 2300, 2220, 2150,
    2070, 2000, 1930, 1870, 1800,
    1740, 1680, 1630, 1570, 1520,
    1470, 1420, 1380, 1330, 1290,
    1250, 1210, 1170, 1130, 1100,
    1060, 1030, 1000,  970,  940,
    910,  880,  850,  830,  800,
    780,  760,  740,  710,  690,
    674,  655,  636,  618, 600,
};
uint32_t temp_res;
int32_t ADCtoTemperature(uint16_t adc) {
    if (adc > 1000 || adc < 30) {
        return INT32_MIN; //
    }
    uint32_t Vout_mV = ((uint32_t)adc * 3300) / 1023;
    uint32_t temp=Vout_mV*1000;
    uint32_t Res_NTC = temp / (3300-Vout_mV)*10;
    temp_res = Res_NTC;
    if (Res_NTC > res_table[0]) {
        return temp_table[0];
    }
    if (Res_NTC < res_table[TABLE_LENGTH - 1]) {
        return temp_table[TABLE_LENGTH - 1];
    }
    uint8_t i;
    for (i = 0; i < TABLE_LENGTH-1; i++) {
        uint16_t R1 = res_table[i];
        uint16_t R2 = res_table[i + 1];
        if ((Res_NTC <= R1) && (Res_NTC >= R2)) {
            int16_t T1 = temp_table[i];
            int16_t T2 = temp_table[i + 1];
            int32_t deltaR = (int32_t)Res_NTC - R1;
            int32_t deltaT = (int32_t)T2 - T1;
            int32_t rangeR = (int32_t)R2 -(int32_t)R1;
            int32_t result = T1 + ((deltaR * deltaT) / rangeR);
//            if(result > 620){
//                result-=10;
//            }
            return result;
        }
    }
}
uint32_t ADCtoHeaterTemperature(uint16_t adc){
    uint64_t temp = (uint64_t)adc * 750ULL;
    temp = temp / 1023;
    return (uint32_t)(temp + 250);
}
uint32_t ADCtoColdTemperature(uint16_t adc){
    uint64_t temp = (uint64_t)adc * 180ULL;
    return (uint32_t)(temp / 1023);
}
int32_t res[4];
uint8_t val,val2,tem;
void BSP_configureMCU(void)
{
    PM5CTL0 &= ~LOCKLPM5;
    CSCTL1 &= ~(DCORSEL_7);
    CSCTL1 |= DCORSEL_5;
    CSCTL2 = FLLD_0 + 487;
    CSCTL3 |= SELREF__REFOCLK;
    CSCTL4 = SELMS__DCOCLKDIV | SELA__REFOCLK;
}

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;                                // Stop WDT
    BSP_configureMCU();
    __bis_SR_register(GIE);
    // Configure GPIO
    GPIO_setAsOutputPin(GPIO_PORT_P1,GPIO_PIN5);
    GPIO_setAsOutputPin(GPIO_PORT_P1,GPIO_PIN6);
    GPIO_setAsOutputPin(GPIO_PORT_P1,GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P2,GPIO_PIN2);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P2, GPIO_PIN0);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P2, GPIO_PIN1);
    //configure Timer
    Timer_A_initUpModeParam timerConfig = {
        .clockSource = TIMER_A_CLOCKSOURCE_SMCLK,
        .clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_16,
        .timerPeriod = 10000 - 1,
        .timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE,
        .captureCompareInterruptEnable_CCR0_CCIE = TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE,
        .timerClear = TIMER_A_DO_CLEAR,
        .startTimer = true
    };
    Timer_A_initUpMode(TIMER_A0_BASE, &timerConfig);


    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);
    ADC_init(ADC_BASE, ADC_SAMPLEHOLDSOURCE_SC, ADC_CLOCKSOURCE_ADCOSC,  ADC_CLOCKDIVIDER_1);
    ADC_enable(ADC_BASE);
    ADC_setupSamplingTimer(ADC_BASE, ADC_CYCLEHOLD_16_CYCLES, ADC_MULTIPLESAMPLESDISABLE);
    ADC_configureMemory(ADC_BASE,  adc_channels[adc_index], ADC_VREFPOS_AVCC, ADC_VREFNEG_AVSS);
    ADC_clearInterrupt(ADC_BASE, ADC_COMPLETED_INTERRUPT);
    ADC_enableInterrupt(ADC_BASE, ADC_COMPLETED_INTERRUPT);
    static uint8_t heater_on = 0;
    static uint8_t cooler_on = 0;
    while(1)
    {
//        __delay_cycles(5000);
//        ADC_startConversion(ADC_BASE,ADC_SINGLECHANNEL);
        __bis_SR_register(CPUOFF + GIE);
        if(adc_done){
            adc_done=0;
            val = P1IN;
            val2 = P2IN;
            uint32_t user_temp_cold  = ADCtoColdTemperature(adc_values[0]);
            uint32_t user_temp_hot   = ADCtoHeaterTemperature(adc_values[1]);
            int32_t temp_cold_ntc   = ADCtoTemperature(adc_values[2]);
            int32_t temp_hot_ntc    = ADCtoTemperature(adc_values[3]);
            if (temp_cold_ntc == INT32_MIN) {
                cooler_on = 0;
            }
            if (temp_hot_ntc == INT32_MIN) {
                heater_on = 0;
            }
            res[0]=user_temp_cold;
            res[1]=user_temp_hot;
            res[2]=temp_cold_ntc;
            res[3]=temp_hot_ntc;
            // Cold
            if(cooler_on) {
                if(temp_cold_ntc <= user_temp_cold)
                    cooler_on = 0;
            }
            else{
                if((temp_cold_ntc >= user_temp_cold + 20) && (temp_cold_ntc != INT32_MIN) )
                    cooler_on = 1;
            }
            if(cooler_on && ( !GPIO_getInputPinValue(GPIO_PORT_P2, GPIO_PIN0)) ) {
                GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN7);
                GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN2);
            }
            else{
                GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN7);
                GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN2);
            }
            // Hot
            if(heater_on) {
                if (temp_hot_ntc >= user_temp_hot)
                    heater_on = 0;
            }
            else{
                if( (temp_hot_ntc <= user_temp_hot - 20) && (temp_hot_ntc != INT32_MIN) )
                    heater_on = 1;
            }
            if(heater_on && ( !GPIO_getInputPinValue(GPIO_PORT_P2, GPIO_PIN1)) ) {
                GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN5);
                GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN6);
            }
            else{
                GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN5);
                GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN6);
            }
            __no_operation();
        }
    }
}

// ADC interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=ADC_VECTOR
__interrupt void ADC_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(ADC_VECTOR))) ADC_ISR (void)
#else
#error Compiler not supported!
#endif
{
    switch (__even_in_range(ADCIV,12)){
        case  0: break; //No interrupt
        case  2: break; //conversion result overflow
        case  4: break; //conversion time overflow
        case  6: break; //ADC10HI
        case  8: break; //ADC10LO
        case 10: break; //ADC10IN
        case 12:        //ADC10IFG0
            adc_values[adc_index] = ADC_getResults(ADC_BASE);
            adc_index++;
            if (adc_index < 4)
            {
                ADC_disableConversions(ADC_BASE, false);

                ADC_configureMemory(ADC_BASE,
                    adc_channels[adc_index],
                    ADC_VREFPOS_AVCC,
                    ADC_VREFNEG_AVSS);

                ADC_startConversion(ADC_BASE, ADC_SINGLECHANNEL);
            }
            else
            {
                ADC_disableConversions(ADC_BASE, false);
                uint8_t i;
                for(i=0;i<4;i++){
                    adc_sum[i] += adc_values[i];
                }
                adc_sample_count++;
                if (adc_sample_count >= ADC_SAMPLE_TIMES) {
                    for (i = 0; i < 4; i++) {
                        adc_values[i] = adc_sum[i] / ADC_SAMPLE_TIMES;
                        adc_sum[i] = 0;
                    }
                    adc_sample_count = 0;
                    adc_done = 1;
                }
                adc_index = 0;
//                adc_done = 1;
            }
          __bic_SR_register_on_exit(CPUOFF);
          break;
        default: break;
    }
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER0_A0_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(TIMER0_A0_VECTOR)))
#endif
void TIMERA0_ISR (void)
{
    ADC_configureMemory(ADC_BASE, adc_channels[adc_index], ADC_VREFPOS_AVCC, ADC_VREFNEG_AVSS);
    ADC_startConversion(ADC_BASE,ADC_SINGLECHANNEL);
}
