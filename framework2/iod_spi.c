#include "pico/stdlib.h"
#include "iod_main.h"
#include "iod_spi.h"

// 外部公開関数
void iod_spi_init() {
    iod_spi_eep_init();
    iod_spi_gyro_init();
}

void iod_spi_deinit() {
    iod_spi_gyro_deinit();
    iod_spi_eep_deinit();
}

void iod_spi_reinit() {
    iod_spi_eep_reinit();
    iod_spi_gyro_reinit();
}

void iod_spi_main_1ms() {
    iod_spi_eep_main_1ms();
    iod_spi_gyro_main_1ms();
}

void iod_spi_main_in() {
    iod_spi_eep_main_in();
    iod_spi_gyro_main_in();
}

void iod_spi_main_out() {
    iod_spi_eep_main_out();
    iod_spi_gyro_main_out();
}
