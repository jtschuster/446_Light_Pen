///* --COPYRIGHT--,BSD
// * Copyright (c) 2014, Texas Instruments Incorporated
// * All rights reserved.
// *
// * Redistribution and use in source and binary forms, with or without
// * modification, are permitted provided that the following conditions
// * are met:
// *
// * *  Redistributions of source code must retain the above copyright
// *    notice, this list of conditions and the following disclaimer.
// *
// * *  Redistributions in binary form must reproduce the above copyright
// *    notice, this list of conditions and the following disclaimer in the
// *    documentation and/or other materials provided with the distribution.
// *
// * *  Neither the name of Texas Instruments Incorporated nor the names of
// *    its contributors may be used to endorse or promote products derived
// *    from this software without specific prior written permission.
// *
// * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
// * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// * --/COPYRIGHT--*/
////****************************************************************************
////  MSP430FR59xx EnergyTrace Demo- High Energy Consumption Code
////
////   Description: This code is intentionally written inefficiently in order
////   to use an unnecessary amount of energy. The ULP Advisor and EnergyTrace
////   are used to help identify the problem areas in the code to point out
////   where changes can be made to increase efficiency.
////
////   About every second, an ADC temperature sample is taken and the degrees
////   Celsius and Fahrenheit are found using floating point calculations.
////   The results are printed and transmitted through the UART.
////   Some examples of the inefficient coding styles used are:
////      sprintf()
////      Floating point operations
////      Divide operations
////      Flag polling
////      Software delays
////      Floating port pins
////      No use of low-power modes
////      Counting up in loops
////
////   B. Finch
////   Texas Instruments Inc.
////   June 2013
////   Built with Code Composer Studio V5.5.0.00039
////
////
////   Modified for COMP_ENG 346 by Josiah Hester @ Northwestern University
////   May 2019
////
////****************************************************************************
//
//#include <stdio.h>
//#include <msp430.h>
//#include <stdint.h>
//
//#define CAL_ADC_12T30  (*((uint16_t *)0x1A1A)) // Temperature Sensor Calibration-30 C 1.2V ref
//#define CAL_ADC_12T85  (*((uint16_t *)0x1A1C)) // Temperature Sensor Calibration-85 C 1.2V ref
//
//#define BUFFER_SIZE 64
//
//unsigned short adc_ring_buffer[BUFFER_SIZE] = {0};
//
//inline short ADC_OLD_AVE (unsigned short* ring_buffer, int head) {
//    unsigned short ave = 0;
//    unsigned int i = 0;
//    for (i = ((head + BUFFER_SIZE/2) % BUFFER_SIZE); i != head; i = (i == BUFFER_SIZE-1) ? 0 : i+1) {
//        ave += ring_buffer[i];
//    }
//    return ave >> 7; // divide by 128
//}
//
//// ((i + 1) % 256)
//
//inline short ADC_CURR_AVE (unsigned short* ring_buffer, int head) {
//    unsigned short ave = 0;
//    unsigned int i = head;
//    for (i = head; i != (head + BUFFER_SIZE/2) % BUFFER_SIZE; i = (i == BUFFER_SIZE-1) ? 0 : i+1) {
//        ave += ring_buffer[i];
//    }
//    return ave >> 7; // divide by 128
//}
//
//#define EXP_AVE_SHIFT_BITS 3
//#define EXP_AVE_BUFF_SIZE (1 << 6)
//
//inline unsigned short DIFF_THRESH(short ave) {
//    return (ave < 1200) ?
//                     50 :
//                     ((ave < 2000) ?
//                                40 :
//                                35);
//};
//
//unsigned short exp_buffer[EXP_AVE_BUFF_SIZE] = {0};
//unsigned short buffered_val = 0;
//unsigned short exp_curr_ave = 0;
//unsigned short exp_last_ave = 0;
//unsigned int exp_head = 0;
//unsigned short time[EXP_AVE_BUFF_SIZE] = {0};
//
//unsigned char adc_head = 0;
//
//
//int main(void) {
//    unsigned short adc_val=0;
//    unsigned short adc_ave = 0;
//    unsigned short adc_last_ave = 0;
//    WDTCTL = WDTPW | WDTHOLD;                 // Stop watchdog timer
//
//    // Configure used port pins
//    P4SEL0 |= BIT2;
//    P4SEL1 &= ~BIT2;
//    P2SEL1 |= BIT1 | BIT0;                    // Configure UART pins
//    P2SEL0 &= ~(BIT1 | BIT0);                 // Configure UART pins
//    P1SEL0 &= ~BIT0 & ~BIT4;
//    P1SEL1 &= ~BIT0 & ~BIT4;
//    P1DIR |= BIT0 | BIT4;
//    PM5CTL0 &= ~LOCKLPM5;                     // Disable GPIO power-on default high-impedance mode
//                                              // to activate previously configured port settings
//
//    // Configure ADC12
//    ADC12CTL0 = ADC12SHT0_7 | ADC12ON;        // 16 ADC12CLKs, ADC ON
//    ADC12CTL1 = ADC12SHP | ADC12SSEL_2 | ADC12CONSEQ_0; // s/w trigger, MCLK, single ch/conv
//    ADC12CTL2 = ADC12RES__12BIT;              // 12-bit conversion results
//    ADC12MCTL0 = ADC12VRSEL_0 | ADC12INCH_10; // ADC input ch A11
//
//    // Configure internal reference
//    while(REFCTL0 & REFGENBUSY);              // If ref generator busy, WAIT
//    REFCTL0 |= REFVSEL_0 | REFGENOT | REFON;  // Select internal ref = 1.2V, reference on
//    __delay_cycles(400);                      // Delay for Ref to settle
//
//    // Configure 1sec Timer
//    TA0CTL = TASSEL__SMCLK | ID__8 | MC__UP | TACLR | TAIE; // SMCLK / 8, up mode, clear timer
//    TA0EX0 = TAIDEX_7;                        // (SMCLK / 8) / 8 ~ 15.625 kHz. Default SMCLK: 1MHz
//    TA0CCR0 = 0x3D09;                         // ~1 sec
//
//    // Configure UART
////    UCA0CTLW0 |= UCSSEL__SMCLK | UCSWRST;     // No parity, LSB first, 8-bit data, 1 stop
////    UCA0BRW = 6;                              // Baud rate register prescale. Configure 9600 baud
////    UCA0MCTLW |= 0x2081;                      // UCBRS = 0x20, UCBRF = 8; UCOS16 = 1
////    UCA0CTLW0 &= ~UCSWRST;                    // Enable eUSCI_A
//
//    __no_operation();                         // SET A BREAKPOINT HERE
//    short diff = 0;
//    unsigned int num_adcs = 0;
//
//    while(1)
//    {
//        if (TA0IV == TA0IV_TAIFG)              // Poll the timer overflow interrupt status
//        {
//            __no_operation();
//        }
//            num_adcs++;
//            ADC12CTL0 |= ADC12ENC | ADC12SC;  // Sampling and conversion start
//            while(ADC12IFGR0 & ~ADC12IFG0);  // Wait for the conversion to complete
//            while(ADC12CTL1 & ADC12BUSY);
//            adc_val = ADC12MEM0;
//
//            // Moving average
//            adc_ring_buffer[adc_head] = adc_val;
////            adc_ave = ADC_OLD_AVE(adc_ring_buffer, adc_head);
////            adc_last_ave = ADC_CURR_AVE(adc_ring_buffer, adc_head);
////
////            if (num_adcs & 16) {
////                diff = adc_last_ave - adc_ave;
////                diff = diff < 0 ? -diff : diff;
////                if (diff > 100) {
////                    P1OUT |= BIT0;
//////                    __delay_cycles(1000);
////                }
////            } else {
////                P1OUT &= ~BIT0;
////            }
//            adc_head = (adc_head == BUFFER_SIZE - 1) ? 0 : adc_head + 1;
//
//
//
//            // EXP Moving Average
//            time[exp_head] = TA0R;
//            exp_buffer[exp_head] = adc_val;
//            exp_head = (exp_head == EXP_AVE_BUFF_SIZE - 1) ? 0 : exp_head + 1;
//            exp_curr_ave = (exp_curr_ave - (exp_curr_ave >> EXP_AVE_SHIFT_BITS)) + (adc_val >> EXP_AVE_SHIFT_BITS);
//            buffered_val = exp_buffer[exp_head];
//            exp_last_ave = (exp_last_ave - (exp_last_ave >> EXP_AVE_SHIFT_BITS)) + (buffered_val >> EXP_AVE_SHIFT_BITS);
//            if (num_adcs & 16) {
//                diff = exp_curr_ave - exp_last_ave;
//                diff = diff < 0 ? -diff : diff;
//                if (diff > DIFF_THRESH(exp_curr_ave)) {
//                    P1OUT |= BIT0 | BIT4;
////                    __delay_cycles(1000);
//                } else {
//                    P1OUT &= ~BIT0 & ~BIT4;
//                }
//            }
//
//            __no_operation();                 // For debugger
//
//    }
//}
