#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#include "iod_main.h"

// 外部公開関数
void iod_wdog_init() {
    // ウォッチドッグを有効に設定（10ms）
    // 注意：watchdog_enable()を設定すると、デバッグ時の接続で失敗する確率が高くなる
//    watchdog_enable(10, true);
}

void iod_wdog_deinit() {
}

void iod_wdog_reinit() {
}

void iod_wdog_main_1ms() {
}

void iod_wdog_main_in() {
}

void iod_wdog_main_out() {
    // ウォッチドッグを更新
    watchdog_update();
}

bool iod_call_wdog_isboot() {
    return watchdog_caused_reboot();
}
