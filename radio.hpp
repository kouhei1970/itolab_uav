
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

#define UART_ID uart0
#define BAUD_RATE 100000
#define DATA_BITS 8
#define STOP_BITS 2
#define PARITY UART_PARITY_EVEN

//#define T6L 

#ifndef T6L
    #define CH1MAX 1696
    #define CH1MIN 352
    #define CH2MAX 1696
    #define CH2MIN 352
    #define CH3MAX 1696
    #define CH3MIN 352
    #define CH4MAX 1696
    #define CH4MIN 352
    #define CH5MAX 1696
    #define CH5MIN 352
    #define CH6MAX 1696
    #define CH6MIN 352
    #define CH_MAX 1696
    #define CH_MIN 352
    #define CH_MID 1024
#else
    #define CH1MAX 1747
    #define CH1MID 1032
    #define CH1MIN 291

    #define CH2MAX 1763
    #define CH2MID 992
    #define CH2MIN 324

    #define CH3MAX 1712
    #define CH3MID 1024
    #define CH3MIN 321

    #define CH4MAX 1745
    #define CH4MID 1024
    #define CH4MIN 289

    #define CH5MAX 1904
    #define CH5MID 1024
    #define CH5MIN 144

    #define CH6MAX 1696
    #define CH6MIN 352
    #define CH_MAX 1696
    #define CH_MIN 352
    #define CH_MID 1024
#endif

//０番と1番ピンに接続
#define UART_TX_PIN 0
#define UART_RX_PIN 1

//グローバル変数の宣言
extern uint16_t Chdata[18];

//グローバル関数の宣言
void radio_init(void);
