#include "pico/stdlib.h"
#include "iod_main.h"
#include "iod_i2c.h"

// 外部公開関数
void iod_i2c_init() {
    iod_i2c_eep_init();
    iod_i2c_gyro_init();
    iod_i2c_6axis_init();
}

void iod_i2c_deinit() {
    iod_i2c_6axis_deinit();
    iod_i2c_gyro_deinit();
    iod_i2c_eep_deinit();
}

void iod_i2c_reinit() {
    iod_i2c_eep_reinit();
    iod_i2c_gyro_reinit();
    iod_i2c_6axis_reinit();
}

void iod_i2c_main_1ms() {
    iod_i2c_eep_main_1ms();
    iod_i2c_gyro_main_1ms();
    iod_i2c_6axis_main_1ms();
}

void iod_i2c_main_in() {
    iod_i2c_eep_main_in();
    iod_i2c_gyro_main_in();
    iod_i2c_6axis_main_in();
}

void iod_i2c_main_out() {
    iod_i2c_eep_main_out();
    iod_i2c_gyro_main_out();
    iod_i2c_6axis_main_out();
}
