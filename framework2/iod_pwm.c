#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "iod_main.h"

#define PWM3_A_GPIO_GP6     GPIO_GP6_PWM
#define PWM3_B_GPIO_GP7     GPIO_GP7_PWM
#define PWM3_WRAP           (25000) // PWMの分解能：周期 125[Mhz]/25000=5[Khz]->0.2[ms]
//#define PWM3_CLKDIV         (100.0) // 分周比：周期 5[Khz]/100.0=50[hz]->0.02[s]
#define PWM3_A_DUTY_INIT    IOD_PWM0_DUTY_MAX
#define PWM3_B_DUTY_INIT    IOD_PWM1_DUTY_MAX

/* 定義の整合チェック */
#if (PWM3_WRAP != IOD_PWM0_DUTY_MAX) || (PWM3_WRAP != IOD_PWM1_DUTY_MAX)
#error IOD_PWMx_DUTY_MAX is not match PWM3_WRAP.
#endif

// 外部公開関数
void iod_pwm_init() {
    // PWM5(A,B)の初期設定
    gpio_set_function(PWM3_A_GPIO_GP6, GPIO_FUNC_PWM);
    gpio_set_function(PWM3_B_GPIO_GP7, GPIO_FUNC_PWM);
    // スライス番号を取得
    uint u32a_slice_num = pwm_gpio_to_slice_num(PWM3_A_GPIO_GP6);
    // PWMの分解能を設定
    pwm_set_wrap(u32a_slice_num, PWM3_WRAP - 1);
    // 分周比を設定（0～256未満の実数値で設定）
//    pwm_set_clkdiv(u32a_slice_num, PWM3_CLKDIV);
    // duty(High期間)の設定
    pwm_set_chan_level(u32a_slice_num, PWM_CHAN_A, PWM3_A_DUTY_INIT);
    pwm_set_chan_level(u32a_slice_num, PWM_CHAN_B, PWM3_B_DUTY_INIT);
    // PWM出力の有効/無効を設定
    pwm_set_enabled(u32a_slice_num, true);
}

void iod_pwm_deinit() {
}

void iod_pwm_reinit() {
}

void iod_pwm_main_1ms() {
}

void iod_pwm_main_in() {
}

void iod_pwm_main_out() {
}

void iod_call_pwm0_set_duty(uint16_t u16a_level) {
    // PWM5(A)のduty設定
    uint u32a_slice_num = pwm_gpio_to_slice_num(PWM3_A_GPIO_GP6);
    pwm_set_chan_level(u32a_slice_num, PWM_CHAN_A, u16a_level);
}

void iod_call_pwm1_set_duty(uint16_t u16a_level) {
    // PWM5(B)のduty設定
    uint u32a_slice_num = pwm_gpio_to_slice_num(PWM3_B_GPIO_GP7);
    pwm_set_chan_level(u32a_slice_num, PWM_CHAN_B, u16a_level);
}
