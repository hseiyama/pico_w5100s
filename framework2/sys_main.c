/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "hardware/sync.h"
#include "sys_main.h"
#include "iod_main.h"
#include "apl_main.h"

#define SYS_TIME_MAX (0xFFFFFFFFFFFFFFFF)

static volatile uint64_t u64s_timer_sys;
static volatile uint8_t u8s_timer_5ms;
static bool bls_sleep_request;
static uint32_t u32s_interrupts_status;

static void sys_init();
static void sys_deinit();
static void sys_reinit();
static void sys_main_1ms();
static void sys_main_5ms();
static bool sys_intr_timer_1ms(struct repeating_timer *);

// 外部公開関数
void main() {
    struct repeating_timer sta_repeat_timer;

    sys_init();
    // 1msのタイマー割り込みを設定
    // （引数 delay_ms=-1：前回の割り込み開始時から計測）
    add_repeating_timer_ms(-1, sys_intr_timer_1ms, NULL, &sta_repeat_timer);

    while (true) {
        // 5ms経過毎に条件が成立
        if (u8s_timer_5ms >= SYS_MAIN_CYCLE) {
            u8s_timer_5ms = 0;
            sys_main_5ms();
        }
        // スリープモードの移行要求がある場合
        if (bls_sleep_request) {
            bls_sleep_request = false;
            // 1msのタイマー割り込みを解除
            cancel_repeating_timer(&sta_repeat_timer);
            sys_deinit();
            // スリープモードに移行する（未解決）
            __wfi();
            sys_reinit();
            // 1msのタイマー割り込みを設定
            add_repeating_timer_ms(-1, sys_intr_timer_1ms, NULL, &sta_repeat_timer);
        }
    }
}

void sys_call_timer_start(struct sys_timer *psta_sys_timer) {
    psta_sys_timer->u64_time = u64s_timer_sys;
    psta_sys_timer->bl_state = true;
}

void sys_call_timer_stop(struct sys_timer *psta_sys_timer) {
    psta_sys_timer->u64_time = SYS_TIME_MAX;
    psta_sys_timer->bl_state = false;
}

bool sys_call_timer_check(struct sys_timer *psta_sys_timer, uint64_t u64a_wait_time) {
    uint64_t u64a_time_diff;
    bool bla_rcode;

    u64a_time_diff = sys_call_timer_diff(psta_sys_timer, u64a_wait_time);
    // 待ち時間が経過している場合は、trueを設定
    bla_rcode = (u64a_time_diff > 0) ? false : true;

    return bla_rcode;
}

uint64_t sys_call_timer_diff(struct sys_timer *psta_sys_timer, uint64_t u64a_wait_time) {
    uint64_t u64a_pass_time;
    uint64_t u64a_rtime;

    // タイマーが開始している場合
    if (psta_sys_timer->bl_state) {
        // 経過時間を算出する
        if (u64s_timer_sys >= psta_sys_timer->u64_time) {
            u64a_pass_time = u64s_timer_sys - psta_sys_timer->u64_time;
        } else {
            u64a_pass_time = SYS_TIME_MAX - (psta_sys_timer->u64_time - u64s_timer_sys);
        }
        // 待ち時間との差を算出する（経過後は、0を設定）
        if (u64a_wait_time > u64a_pass_time) {
            u64a_rtime = u64a_wait_time - u64a_pass_time;
        } else {
            u64a_rtime = 0;
        }
    // タイマーが停止している場合は、0を設定
    } else {
        u64a_rtime = 0;
    }

    return u64a_rtime;
}

bool sys_call_timer_isrun(struct sys_timer *psta_sys_timer) {
    return psta_sys_timer->bl_state;
}

void sys_call_disable_interrupts() {
    u32s_interrupts_status = save_and_disable_interrupts();
}
void sys_call_enable_interrupts() {
    restore_interrupts(u32s_interrupts_status);
}

void sys_call_sleep_request() {
    bls_sleep_request = true;
}

// 内部関数
static void sys_init() {
    u64s_timer_sys = 0;
    u8s_timer_5ms = 0;
    bls_sleep_request = false;
    u32s_interrupts_status = 0;
    iod_init();
    apl_init();
}

static void sys_deinit() {
    apl_deinit();
    iod_deinit();
}

static void sys_reinit() {
    iod_reinit();
    apl_reinit();
}

static void sys_main_1ms() {
    iod_main_1ms();
}

static void sys_main_5ms() {
    iod_main_in();
    apl_main();
    iod_main_out();
}

static bool sys_intr_timer_1ms(struct repeating_timer *psta_repeat_timer) {
    u64s_timer_sys++;
    u8s_timer_5ms++;
    sys_main_1ms();

    return true;
}
