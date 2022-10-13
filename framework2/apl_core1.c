#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "sys_main.h"
#include "iod_main.h"
#include "apl_main.h"

static struct sys_timer sts_blink_timer;
static uint8_t au8s_tx_message[IOD_UART_BUFF_SIZE];
static uint8_t u8s_core1_count;

// 外部公開関数
void apl_core1_task_init() {
    memset(au8s_tx_message, 0, sizeof(au8s_tx_message));
    u8s_core1_count = 0;
    // 点滅タイマーの開始
    sys_call_timer_start(&sts_blink_timer);
}

void apl_core1_task_main() {
    static uint16_t u16a_blink_time = 1000;
    static bool bla_blink_value = true;
    uint32_t u32a_fifo_data;

    // 点滅タイマーが満了した場合
    if (sys_call_timer_check(&sts_blink_timer, u16a_blink_time)) {
        // core0から受信（FIFO / QUEUE）
//        while (iod_call_mcore_fifo_pop(&u32a_fifo_data)) {
        while (iod_call_mcore_queue_remove_core1(&u32a_fifo_data)) {
            u16a_blink_time = (uint16_t)u32a_fifo_data;
        }

        bla_blink_value = !bla_blink_value;
        snprintf(au8s_tx_message, sizeof(au8s_tx_message), "core1 task(%d)\r\n", u8s_core1_count);
        u8s_core1_count++;
        iod_call_uart_transmit(au8s_tx_message);

        // core1へ送信（FIFO / QUEUE）
//        iod_call_mcore_fifo_push(bla_blink_value);
        iod_call_mcore_queue_add_core0(bla_blink_value);

        // 点滅タイマーの再開
        sys_call_timer_start(&sts_blink_timer);
    }
}
