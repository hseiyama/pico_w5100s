#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "iod_main.h"

#define UART0_ID            uart0
#define UART0_TX_GPIO_GP0   GPIO_GP0_UART
#define UART0_RX_GPIO_GP1   GPIO_GP1_UART

static void iod_uart_clear_rx_fifo(uart_inst_t *);

// 外部公開関数
void iod_uart_init() {
    // UART0の初期設定
    uart_init(UART0_ID, 115200);
    gpio_set_function(UART0_TX_GPIO_GP0, GPIO_FUNC_UART);
    gpio_set_function(UART0_RX_GPIO_GP1, GPIO_FUNC_UART);
    // UART0のRX_FIFOをクリア（機能の動作安定まで時間待ち：1us）
    sleep_us(1);
    iod_uart_clear_rx_fifo(UART0_ID);
}

void iod_uart_deinit() {
}

void iod_uart_reinit() {
}

void iod_uart_main_1ms() {
}

void iod_uart_main_in() {
}

void iod_uart_main_out() {
}

bool iod_call_uart_receive(uint8_t *pu8a_message) {
    uint8_t u8a_index = 0;
    bool bla_rcode = false;

    while (uart_is_readable(UART0_ID)
    && (u8a_index < IOD_UART_BUFF_SIZE - 1)) {
        bla_rcode = true;
        // UART0の受信
        pu8a_message[u8a_index] = uart_getc(UART0_ID);
        u8a_index++;
    }
    pu8a_message[u8a_index] = 0x00;

    return bla_rcode;
}

void iod_call_uart_transmit(uint8_t *pu8a_message) {
    // UART0の送信
    iod_call_mcore_mutex_enter();   // ロック開始
    uart_puts(UART0_ID, pu8a_message);
    iod_call_mcore_mutex_exit();    // ロック解除
}

// 内部関数
static void iod_uart_clear_rx_fifo(uart_inst_t *psta_uart_id) {
    while (uart_is_readable(psta_uart_id)) {
        uart_getc(psta_uart_id);
    }
}
