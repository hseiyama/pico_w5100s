#include "pico/stdlib.h"
#include "sys_main.h"
#include "iod_main.h"
#include "apl_main.h"

static const uint16_t acu16s_blink_time[BLINK_STATE_NUM] = {
    500, 1000, 2000
};

static enum blink_state u8s_blink_state;
static struct sys_timer asts_blink_timer[BLINK_STATE_NUM];
static bool abls_blink_value[BLINK_STATE_NUM];

static void blink_request();
static bool blink_update(bool, uint16_t);

// 外部公開関数
void apl_blink_init() {
    uint8_t u8a_index;

    u8s_blink_state = BLINK_1000MS;
    for (u8a_index = 0; u8a_index < BLINK_STATE_NUM; u8a_index++) {
        abls_blink_value[u8a_index] = IOD_LED0_VALUE_INIT;
        // 点滅タイマーの開始
        sys_call_timer_start(&asts_blink_timer[u8a_index]);
    }

    // フラッシュ領域のデータを復元
    if (stg_flash_info.bl_status) {
        u8s_blink_state = stg_flash_info.st_data.u8_blink_state;
    }
}

void apl_blink_deinit() {
    // フラッシュ領域にデータを退避
    stg_flash_info.st_data.u8_blink_state = u8s_blink_state;
}

void apl_blink_reinit() {
}

void apl_blink_main() {
    bool bla_in_btn0_value;
    uint16_t u16a_in_vrest_value;
    bool bla_out_led1_value;

    // 入力処理
    iod_read_btn0_value(&bla_in_btn0_value);
    iod_read_vrest_value(&u16a_in_vrest_value);

    // 要求イベントの処理
    blink_request();
    // LED点滅関連の更新
    bla_out_led1_value = blink_update(bla_in_btn0_value, u16a_in_vrest_value);

    // 出力処理
    iod_write_led0_value(abls_blink_value[BLINK_1000MS]);
    iod_write_led1_value(bla_out_led1_value);
}

// 内部関数
static void blink_request() {
    enum request_event u8a_event;

    apl_read_request_event(&u8a_event);
    // 対象イベントの処理
    switch (u8a_event) {
    case EVENT_BLINK_500MS:
    case EVENT_BLINK_1000MS:
    case EVENT_BLINK_2000MS:
        u8s_blink_state = BLINK_500MS + (u8a_event - EVENT_BLINK_500MS);
        break;
    }
}

static bool blink_update(bool bla_btn_value, uint16_t u16a_adc_value) {
    static bool bla_blink_value = true;
    uint32_t u32a_fifo_data;
    bool bla_led_value;
    uint8_t u8a_index;

    // core1から受信（FIFO / QUEUE）
//    while (iod_call_mcore_fifo_pop(&u32a_fifo_data)) {
    while (iod_call_mcore_queue_remove_core0(&u32a_fifo_data)) {
        bla_blink_value = (bool)(u32a_fifo_data & 0xFF);
    }

    // LED出力用の保持値を更新
    for (u8a_index = 0; u8a_index < BLINK_STATE_NUM; u8a_index++) {
        // 点滅タイマーが満了した場合
        if (sys_call_timer_check(&asts_blink_timer[u8a_index], acu16s_blink_time[u8a_index])) {
            // LED出力用の保持値を反転する
            abls_blink_value[u8a_index] = !abls_blink_value[u8a_index];
            // 点滅タイマーの再開
            sys_call_timer_start(&asts_blink_timer[u8a_index]);
        }
    }

    // ボタンの入力値により、LED1の出力値を決定する
    if (bla_btn_value) {
        bla_led_value = abls_blink_value[u8s_blink_state];
    } else {
        bla_led_value = bla_blink_value;
    }

    // core1へ送信（FIFO / QUEUE）
//    iod_call_mcore_fifo_push(u16a_adc_value);
    iod_call_mcore_queue_add_core1(u16a_adc_value);

    return bla_led_value;
}
