
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

#define UART_ID uart0
#define BAUD_RATE 100000
#define DATA_BITS 8
#define STOP_BITS 2
#define PARITY UART_PARITY_EVEN
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


//０番と1番ピンに接続
#define UART_TX_PIN 0
#define UART_RX_PIN 1

//グローバル変数の宣言
extern uint16_t Chdata[18];

//グローバル関数の宣言
void radio_init(void);
