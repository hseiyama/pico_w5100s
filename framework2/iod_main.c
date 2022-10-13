#include "pico/stdlib.h"
#include "iod_main.h"

// 外部公開関数
void iod_init() {
    iod_port_init();
    iod_uart_init();
    iod_adc_init();
    iod_pwm_init();
    iod_mcore_init();
    iod_i2c_init();
    iod_spi_init();
    iod_flash_init();
    iod_wdog_init();
}

void iod_deinit() {
    iod_wdog_deinit();
    iod_flash_deinit();
    iod_spi_deinit();
    iod_i2c_deinit();
    iod_mcore_deinit();
    iod_pwm_deinit();
    iod_adc_deinit();
    iod_uart_deinit();
    iod_port_deinit();
}

void iod_reinit() {
    iod_port_reinit();
    iod_uart_reinit();
    iod_adc_reinit();
    iod_pwm_reinit();
    iod_mcore_reinit();
    iod_i2c_reinit();
    iod_spi_reinit();
    iod_flash_reinit();
    iod_wdog_reinit();
}

void iod_main_1ms() {
    iod_port_main_1ms();
    iod_uart_main_1ms();
    iod_adc_main_1ms();
    iod_pwm_main_1ms();
    iod_mcore_main_1ms();
    iod_i2c_main_1ms();
    iod_spi_main_1ms();
    iod_flash_main_1ms();
    iod_wdog_main_1ms();
}

void iod_main_in() {
    iod_port_main_in();
    iod_uart_main_in();
    iod_adc_main_in();
    iod_pwm_main_in();
    iod_mcore_main_in();
    iod_i2c_main_in();
    iod_spi_main_in();
    iod_flash_main_in();
    iod_wdog_main_in();
}

void iod_main_out() {
    iod_port_main_out();
    iod_uart_main_out();
    iod_adc_main_out();
    iod_pwm_main_out();
    iod_mcore_main_out();
    iod_i2c_main_out();
    iod_spi_main_out();
    iod_flash_main_out();
    iod_wdog_main_out();
}
