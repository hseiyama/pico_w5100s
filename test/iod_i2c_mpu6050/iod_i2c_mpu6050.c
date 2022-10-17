#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// iod_main
#define GPIO_GP10_I2C       (10)
#define GPIO_GP11_I2C       (11)

#define I2C1_ID             i2c1
#define I2C1_SDA_GPIO       GPIO_GP10_I2C
#define I2C1_SCL_GPIO       GPIO_GP11_I2C

// MPU-6050 のアドレス
// Address 7bit: SlaveAddress=0b1101000
#define MPU6050_ADDRESS     (0b01101000)
// MPU-6050 のレジスタアドレス（抜粋）
#define GYRO_CONFIG         (0x1B)
#define ACCEL_CONFIG        (0x1C)
#define ACCEL_XOUT_H        (0x3B)
#define ACCEL_XOUT_L        (0x3C)
#define ACCEL_YOUT_H        (0x3D)
#define ACCEL_YOUT_L        (0x3E)
#define ACCEL_ZOUT_H        (0x3F)
#define ACCEL_ZOUT_L        (0x40)
#define TEMP_OUT_H          (0x41)
#define TEMP_OUT_L          (0x42)
#define GYRO_XOUT_H         (0x43)
#define GYRO_XOUT_L         (0x44)
#define GYRO_YOUT_H         (0x45)
#define GYRO_YOUT_L         (0x46)
#define GYRO_ZOUT_H         (0x47)
#define GYRO_ZOUT_L         (0x48)
#define PWR_MGMT_1          (0x6B)
#define PWR_MGMT_2          (0x6C)
#define WHO_AM_I            (0x75)

// 読み出し操作（複数バイト読み出し）
#define MPU6050_READ_SIZE   (1)
// 書き込み操作（1バイト書き込み）
#define MPU6050_WRITE_SIZE  (2)

enum axis_group {
    ACCEL_X = 0,
    ACCEL_Y,
    ACCEL_Z,
    GYRO_X,
    GYRO_Y,
    GYRO_Z,
    AXIS_GROUP_NUM
};

static const uint8_t cau8s_axis_address[AXIS_GROUP_NUM] = {
    ACCEL_XOUT_H,
    ACCEL_YOUT_H,
    ACCEL_ZOUT_H,
    GYRO_XOUT_H,
    GYRO_YOUT_H,
    GYRO_ZOUT_H,
};

static int16_t as16s_axis_value[AXIS_GROUP_NUM];
static uint8_t au8s_tx_buffer[32];
// キャリブレーション用
static int16_t as16s_axis_offset[AXIS_GROUP_NUM];
static int16_t as16s_calibrate_sum[AXIS_GROUP_NUM];
static bool bla_calibrate_flag;
static uint8_t u8a_calibrate_count;

// iod_i2c_6axis
void iod_read_accel_x(int16_t *);
void iod_read_accel_y(int16_t *);
void iod_read_accel_z(int16_t *);
void iod_read_gyro_x(int16_t *);
void iod_read_gyro_y(int16_t *);
void iod_read_gyro_z(int16_t *);

// iod_i2c_mpu6050
extern void iod_i2c_mpu6050_init();
extern void iod_i2c_mpu6050_deinit();
extern void iod_i2c_mpu6050_reinit();
extern void iod_i2c_mpu6050_main_1ms();
extern void iod_i2c_mpu6050_main_in();
extern void iod_i2c_mpu6050_main_out();

static void iod_i2c_mpu6050_calibrate_init();
static void iod_i2c_mpu6050_calibrate_run();
static void iod_i2c_mpu6050_read(uint8_t, uint8_t *, uint8_t);
static void iod_i2c_mpu6050_write(uint8_t, uint8_t);

// 外部公開関数
void main() {
    stdio_init_all();
    iod_i2c_mpu6050_init();

    while (true) {
        iod_i2c_mpu6050_main_in();
        {
            int16_t s16a_in_accel_x;
            int16_t s16a_in_accel_y;
            int16_t s16a_in_accel_z;
            int16_t s16a_in_gyro_x;
            int16_t s16a_in_gyro_y;
            int16_t s16a_in_gyro_z;

            iod_read_accel_x(&s16a_in_accel_x);
            iod_read_accel_y(&s16a_in_accel_y);
            iod_read_accel_z(&s16a_in_accel_z);
            iod_read_gyro_x(&s16a_in_gyro_x);
            iod_read_gyro_y(&s16a_in_gyro_y);
            iod_read_gyro_z(&s16a_in_gyro_z);

            printf("accel = %d, %d, %d\n", s16a_in_accel_x, s16a_in_accel_y, s16a_in_accel_z);
            printf("gyro = %d, %d, %d\r\n", s16a_in_gyro_x, s16a_in_gyro_y, s16a_in_gyro_z);
        }
        sleep_ms(1000);
    }
}

void iod_i2c_mpu6050_init() {
    memset(as16s_axis_value, 0, sizeof(as16s_axis_value));
    memset(au8s_tx_buffer, 0, sizeof(au8s_tx_buffer));
    // キャリブレーション用
    memset(as16s_axis_offset, 0, sizeof(as16s_axis_offset));
    bla_calibrate_flag = true;
    iod_i2c_mpu6050_calibrate_init();

    // I2C1の初期設定（クロックは 400KHz）
    i2c_init(I2C1_ID, 400*1000);
    gpio_set_function(I2C1_SDA_GPIO, GPIO_FUNC_I2C);
    gpio_set_function(I2C1_SCL_GPIO, GPIO_FUNC_I2C);
    //gpio_pull_up(I2C1_SDA_GPIO);
    //gpio_pull_up(I2C1_SCL_GPIO);
    sleep_ms(30); // MPU-6050 待ち時間 30ms

    // MPU-6050 設定
    iod_i2c_mpu6050_write(GYRO_CONFIG, 0x18); // FS_SEL=11: 2000 deg/s (full scale range of gyroscopes)
    iod_i2c_mpu6050_write(ACCEL_CONFIG, 0x18); // AFS_SEL=11: 16 g (full scale range of accelerometers)
    iod_i2c_mpu6050_write(PWR_MGMT_1, 0x00); // SLEEP=0: non sleep mode
}

void iod_i2c_mpu6050_deinit() {
}

void iod_i2c_mpu6050_reinit() {
}

void iod_i2c_mpu6050_main_1ms() {
}

void iod_i2c_mpu6050_main_in() {
    uint8_t au8a_data[2];
    uint8_t u8a_index;

    if (bla_calibrate_flag) {
        iod_i2c_mpu6050_calibrate_run();
    } else {
        for (u8a_index = 0;u8a_index < AXIS_GROUP_NUM; u8a_index++) {
            iod_i2c_mpu6050_read(cau8s_axis_address[u8a_index], au8a_data, sizeof(au8a_data));
            as16s_axis_value[u8a_index] = (int16_t)((au8a_data[0] << 8) | au8a_data[1]) - as16s_axis_offset[u8a_index];
        }
    }
}

void iod_i2c_mpu6050_main_out() {
}

void iod_read_accel_x(int16_t *ps16a_value) {
    *ps16a_value = as16s_axis_value[ACCEL_X];
}

void iod_read_accel_y(int16_t *ps16a_value) {
    *ps16a_value = as16s_axis_value[ACCEL_Y];
}

void iod_read_accel_z(int16_t *ps16a_value) {
    *ps16a_value = as16s_axis_value[ACCEL_Z];
}

void iod_read_gyro_x(int16_t *ps16a_value) {
    *ps16a_value = as16s_axis_value[GYRO_X];
}

void iod_read_gyro_y(int16_t *ps16a_value) {
    *ps16a_value = as16s_axis_value[GYRO_Y];
}

void iod_read_gyro_z(int16_t *ps16a_value) {
    *ps16a_value = as16s_axis_value[GYRO_Z];
}

// 内部関数
static void iod_i2c_mpu6050_calibrate_init() {
    memset(as16s_calibrate_sum, 0, sizeof(as16s_calibrate_sum));
    u8a_calibrate_count = 0;
}

static void iod_i2c_mpu6050_calibrate_run() {
    uint8_t au8a_data[2];
    uint8_t u8a_index;

    for (u8a_index = 0;u8a_index < AXIS_GROUP_NUM; u8a_index++) {
        iod_i2c_mpu6050_read(cau8s_axis_address[u8a_index], au8a_data, sizeof(au8a_data));
        as16s_calibrate_sum[u8a_index] += (int16_t)((au8a_data[0] << 8) | au8a_data[1]);
    }
    u8a_calibrate_count++;
    // オフセット値を設定
    if (u8a_calibrate_count >= 10) {
        for (u8a_index = 0;u8a_index < AXIS_GROUP_NUM; u8a_index++) {
            if (u8a_index != ACCEL_Z) { // Z軸の加速度は除く
                as16s_axis_offset[u8a_index] = as16s_calibrate_sum[u8a_index] / u8a_calibrate_count;
            }
        }
        iod_i2c_mpu6050_calibrate_init();
        bla_calibrate_flag = false;
    }
}

static void iod_i2c_mpu6050_read(uint8_t u8a_address, uint8_t *pu8a_buffer, uint8_t u8a_size) {
    // 読み出し操作コマンド（複数バイト読み出し）
    au8s_tx_buffer[0] = u8a_address;
    // 読み出し操作
    i2c_write_blocking(I2C1_ID, MPU6050_ADDRESS, au8s_tx_buffer, MPU6050_READ_SIZE, true);
    i2c_read_blocking(I2C1_ID, MPU6050_ADDRESS, pu8a_buffer, u8a_size, false);
}

static void iod_i2c_mpu6050_write(uint8_t u8a_address, uint8_t u8a_data) {
    // 書き込み操作コマンド（1バイト書き込み）
    au8s_tx_buffer[0] = u8a_address;
    au8s_tx_buffer[1] = u8a_data;
    // 書き込み操作
    i2c_write_blocking(I2C1_ID, MPU6050_ADDRESS, au8s_tx_buffer, MPU6050_WRITE_SIZE, false);
}
