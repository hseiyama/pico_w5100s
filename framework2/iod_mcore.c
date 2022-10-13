#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "iod_main.h"
#include "apl_main.h"

// キュー用
static queue_t sts_queue_core0;
static queue_t sts_queue_core1;
// クリティカルセクション用
static critical_section_t sts_critical_section;
// ミューテックス用
static mutex_t sts_mutex;
// セマフォ用
static semaphore_t sts_semaphore;

static void iod_mcore_core1_task();

// 外部公開関数
void iod_mcore_init() {
    // キューの初期化
    queue_init(&sts_queue_core0, sizeof(uint32_t), 2);
    queue_init(&sts_queue_core1, sizeof(uint32_t), 2);
    // クリティカルセクションの初期化
    critical_section_init(&sts_critical_section);
    // ミューテックスの初期化
    mutex_init(&sts_mutex);
    // セマフォの初期化
    sem_init(&sts_semaphore, 1, 1);
}

void iod_mcore_deinit() {
}

void iod_mcore_reinit() {
}

void iod_mcore_main_1ms() {
}

void iod_mcore_main_in() {
}

void iod_mcore_main_out() {
}

void iod_call_mcore_start() {
    iod_call_mcore_mutex_enter();   // ロック開始
    multicore_reset_core1();
    multicore_launch_core1(iod_mcore_core1_task);
    iod_call_mcore_mutex_exit();    // ロック解除
}

void iod_call_mcore_stop() {
    iod_call_mcore_mutex_enter();   // ロック開始
    multicore_reset_core1();
    iod_call_mcore_mutex_exit();    // ロック解除
}

bool iod_call_mcore_fifo_push(uint32_t u32a_data) {
    bool bla_rcode = multicore_fifo_wready();
    if (bla_rcode) {
        multicore_fifo_push_blocking(u32a_data);
    }

    return bla_rcode;
}

bool iod_call_mcore_fifo_pop(uint32_t *pu32a_data) {
    bool bla_rcode = multicore_fifo_rvalid();
    if (bla_rcode) {
        *pu32a_data = multicore_fifo_pop_blocking();
    }

    return bla_rcode;
}

bool iod_call_mcore_queue_add_core0(uint32_t u32a_data) {
    bool bla_rcode = queue_try_add(&sts_queue_core0, &u32a_data);
    return bla_rcode;
}

bool iod_call_mcore_queue_remove_core0(uint32_t *pu32a_data) {
    bool bla_rcode = queue_try_remove(&sts_queue_core0, pu32a_data);
    return bla_rcode;
}

bool iod_call_mcore_queue_add_core1(uint32_t u32a_data) {
    bool bla_rcode = queue_try_add(&sts_queue_core1, &u32a_data);
    return bla_rcode;
}

bool iod_call_mcore_queue_remove_core1(uint32_t *pu32a_data) {
    bool bla_rcode = queue_try_remove(&sts_queue_core1, pu32a_data);
    return bla_rcode;
}

void iod_call_mcore_lock_enter() {
    // 注意１：ロック中は割り込み禁止の状態になっている
    // 注意２：ロック中に sleep_ms()は使用不可（内部で割り込みを使用）
    critical_section_enter_blocking(&sts_critical_section);
}

void iod_call_mcore_lock_exit() {
    critical_section_exit(&sts_critical_section);
}

void iod_call_mcore_mutex_enter() {
    mutex_enter_blocking(&sts_mutex);
}

void iod_call_mcore_mutex_exit() {
    mutex_exit(&sts_mutex);
}

void iod_call_mcore_sem_acquire() {
    sem_acquire_blocking(&sts_semaphore);
}

bool iod_call_mcore_sem_release() {
    bool bla_rcode = sem_release(&sts_semaphore);
    return bla_rcode;
}

// 内部関数
static void iod_mcore_core1_task() {
    apl_core1_task_init();
    while (true) {
        apl_core1_task_main();
    }
}
