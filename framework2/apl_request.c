#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "iod_main.h"
#include "apl_main.h"

enum request_state {
    REQUEST_NONE = 0,
    REQUEST_BLINK,
    REQUEST_PWM0,
    REQUEST_PWM1,
    REQUEST_BTN1_INTR,
    REQUEST_BTN2_INTR,
    REQUEST_MCORE,
    REQUEST_STATE_NUM
};

static uint8_t au8s_rx_message[IOD_UART_BUFF_SIZE];
static enum request_state u8s_request_sate;
static enum request_event u8s_request_event;

static bool request_sate(uint8_t);
static bool request_sate_none(uint8_t);
static bool request_sate_blink(uint8_t);
static bool request_sate_pwm(uint8_t, enum request_event);
static bool request_sate_btn_intr(uint8_t, enum request_event);
static bool request_sate_mcore(uint8_t);

// 外部公開関数
void apl_request_init() {
    memset(au8s_rx_message, 0, sizeof(au8s_rx_message));
    u8s_request_sate = REQUEST_NONE;
    u8s_request_event = EVENT_NONE;
}

void apl_request_deinit() {
}

void apl_request_reinit() {
}

void apl_request_main() {
    // 要求イベントの初期化
    u8s_request_event = EVENT_NONE;

    // UART受信した場合
    if (iod_call_uart_receive(au8s_rx_message)) {
        // 要求状態を更新
        bool bla_request_update = request_sate(au8s_rx_message[0]);
        // 受信メッセージをUART送信
        if (bla_request_update) {
            snprintf(au8g_tx_message, sizeof(au8g_tx_message), "request='%c'\r\n", au8s_rx_message[0]);
        } else {
            snprintf(au8g_tx_message, sizeof(au8g_tx_message), "request error '%c'\r\n", au8s_rx_message[0]);
        }
        iod_call_uart_transmit(au8g_tx_message);
    }
}

void apl_read_request_event(enum request_event *pu8a_event) {
    *pu8a_event = u8s_request_event;
}

// 内部関数
static bool request_sate(uint8_t u8a_request) {
    bool bla_rcode = false;

    // 対象要求の処理
    switch (u8s_request_sate) {
    case REQUEST_NONE:
        bla_rcode = request_sate_none(u8a_request);
        break;
    case REQUEST_BLINK:
        bla_rcode = request_sate_blink(u8a_request);
        break;
    case REQUEST_PWM0:
        bla_rcode = request_sate_pwm(u8a_request, EVENT_PWM0_LEVEL0);
        break;
    case REQUEST_PWM1:
        bla_rcode = request_sate_pwm(u8a_request, EVENT_PWM1_LEVEL0);
        break;
    case REQUEST_BTN1_INTR:
        bla_rcode = request_sate_btn_intr(u8a_request, EVENT_BTN1_INTR_DISENABLE);
        break;
    case REQUEST_BTN2_INTR:
        bla_rcode = request_sate_btn_intr(u8a_request, EVENT_BTN2_INTR_DISENABLE);
        break;
    case REQUEST_MCORE:
        bla_rcode = request_sate_mcore(u8a_request);
        break;
    default:
        // 異常時：デフォルト値の設定
        u8s_request_sate = REQUEST_NONE;
        break;
    }

    return bla_rcode;
}

static bool request_sate_none(uint8_t u8a_request) {
    bool bla_rcode = false;

    // 対象要求の処理
    switch (u8a_request) {
    case 'a':
        u8s_request_sate = REQUEST_BLINK;
        bla_rcode = true;
        break;
    case 'b':
        u8s_request_sate = REQUEST_PWM0;
        bla_rcode = true;
        break;
    case 'c':
        u8s_request_sate = REQUEST_PWM1;
        bla_rcode = true;
        break;
    case 'd':
        u8s_request_sate = REQUEST_BTN1_INTR;
        bla_rcode = true;
        break;
    case 'e':
        u8s_request_sate = REQUEST_BTN2_INTR;
        bla_rcode = true;
        break;
    case 's':
        // 要求イベント設定
        u8s_request_event = EVENT_SLEEP;
        bla_rcode = true;
        break;
    case 'w':
        // 要求イベント設定
        u8s_request_event = EVENT_WDOG_RESET;
        bla_rcode = true;
        break;
    case 'x':
        u8s_request_sate = REQUEST_MCORE;
        bla_rcode = true;
        break;
    }

    return bla_rcode;
}

static bool request_sate_blink(uint8_t u8a_request) {
    bool bla_rcode = false;

    // 対象要求の処理
    switch (u8a_request) {
    case '0':
    case '1':
    case '2':
        u8s_request_event = EVENT_BLINK_500MS + (u8a_request - '0');
        bla_rcode = true;
        break;
    }
    u8s_request_sate = REQUEST_NONE;

    return bla_rcode;
}

static bool request_sate_pwm(uint8_t u8a_request, enum request_event u8a_event_base) {
    bool bla_rcode = false;

    // 対象要求の処理
    switch (u8a_request) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
        u8s_request_event = u8a_event_base + (u8a_request - '0');
        bla_rcode = true;
        break;
    }
    u8s_request_sate = REQUEST_NONE;

    return bla_rcode;
}

static bool request_sate_btn_intr(uint8_t u8a_request, enum request_event u8a_event_base) {
    bool bla_rcode = false;
    bool bla_enabled;

    // 対象要求の処理
    switch (u8a_request) {
    case '0':
    case '1':
        u8s_request_event = u8a_event_base + (u8a_request - '0');
        bla_rcode = true;
        break;
    }
    u8s_request_sate = REQUEST_NONE;

    return bla_rcode;
}

static bool request_sate_mcore(uint8_t u8a_request) {
    bool bla_rcode = false;

    // 対象要求の処理
    switch (u8a_request) {
    case '0':
    case '1':
        u8s_request_event = EVENT_MCORE_STOP + (u8a_request - '0');
        bla_rcode = true;
        break;
    }
    u8s_request_sate = REQUEST_NONE;

    return bla_rcode;
}
