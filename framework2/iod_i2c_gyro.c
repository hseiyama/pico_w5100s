#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "iod_main.h"
#include "iod_i2c.h"

#define I2C1_ID             i2c1
#define I2C1_SDA_GPIO_GP10  GPIO_GP10_I2C
#define I2C1_SCL_GPIO_GP11  GPIO_GP11_I2C
#define I2C1_ADR_GPIO_GP12  GPIO_GP12_I2C
#define I2C1_CSN_GPIO_GP13  GPIO_GP13_I2C

// L3GD20H のアドレス
// Address 7bit: SlaveAddress=0b1101010
#define GYRO_ADDRESS            (0b01101010)
// L3GD20H のレジスタアドレス
#define GYRO_REG_WHO_AM_I       (0x0F)
#define GYRO_REG_CTRL1          (0x20)
#define GYRO_REG_CTRL2          (0x21)
#define GYRO_REG_CTRL3          (0x22)
#define GYRO_REG_CTRL4          (0x23)
#define GYRO_REG_CTRL5          (0x24)
#define GYRO_REG_REFERENCE      (0x25)
#define GYRO_REG_OUT_TEMP       (0x26)
#define GYRO_REG_STATUS         (0x27)
#define GYRO_REG_OUT_X_L        (0x28)
#define GYRO_REG_OUT_X_H        (0x29)
#define GYRO_REG_OUT_Y_L        (0x2A)
#define GYRO_REG_OUT_Y_H        (0x2B)
#define GYRO_REG_OUT_Z_L        (0x2C)
#define GYRO_REG_OUT_Z_H        (0x2D)
#define GYRO_REG_FIFO_CTRL      (0x2E)
#define GYRO_REG_FIFO_SRC       (0x2F)
#define GYRO_REG_IG_CFG         (0x30)
#define GYRO_REG_IG_SRC         (0x31)
#define GYRO_REG_IG_THS_XH      (0x32)
#define GYRO_REG_IG_THS_XL      (0x33)
#define GYRO_REG_IG_THS_YH      (0x34)
#define GYRO_REG_IG_THS_YL      (0x35)
#define GYRO_REG_IG_THS_ZH      (0x36)
#define GYRO_REG_IG_THS_ZL      (0x37)
#define GYRO_REG_IG_DURATION    (0x38)
#define GYRO_REG_LOW_ODR        (0x39)

// 読み出し操作オプション（複数バイト読み出し）
// READ MSb=1 RegisterAddress=7bit
#define GYRO_READ_OPT           (0b10000000)
#define GYRO_READ_SIZE          (1)
// 書き込み操作オプション（1バイト書き込み）
// WRITE MSb=0 RegisterAddress=7bit
#define GYRO_WRITE_OPT          (0b00000000)
#define GYRO_WRITE_SIZE         (2)

enum iod_i2c_gyro_group {
    GYRO_X = 0,
    GYRO_Y,
    GYRO_Z,
    GYRO_GROUP_NUM
};

static const uint8_t acu8s_gyro_address[GYRO_GROUP_NUM] = {
    GYRO_REG_OUT_X_L,
    GYRO_REG_OUT_Y_L,
    GYRO_REG_OUT_Z_L
};

static int16_t as16s_gyro_value[GYRO_GROUP_NUM];
static uint8_t au8s_tx_buffer[32];

static void iod_i2c_gyro_read(uint8_t, uint8_t *, uint8_t);
static void iod_i2c_gyro_write(uint8_t, uint8_t);

// 外部公開関数
void iod_i2c_gyro_init() {
    memset(as16s_gyro_value, 0, sizeof(as16s_gyro_value));
    memset(au8s_tx_buffer, 0, sizeof(au8s_tx_buffer));

#if SELECT_I2C_GYRO
    // I2C1の初期設定（クロックは 400KHz）
    i2c_init(I2C1_ID, 400*1000);
    gpio_set_function(I2C1_SDA_GPIO_GP10, GPIO_FUNC_I2C);
    gpio_set_function(I2C1_SCL_GPIO_GP11, GPIO_FUNC_I2C);
    //gpio_pull_up(I2C1_SDA_GPIO_GP10);
    //gpio_pull_up(I2C1_SCL_GPIO_GP11);

    // I2C用の設定（SPI共用のポートを設定）
    gpio_init(I2C1_ADR_GPIO_GP12);
    gpio_init(I2C1_CSN_GPIO_GP13);
    gpio_set_dir(I2C1_ADR_GPIO_GP12, GPIO_OUT);
    gpio_set_dir(I2C1_CSN_GPIO_GP13, GPIO_OUT);
    gpio_put(I2C1_ADR_GPIO_GP12, false);
    gpio_put(I2C1_CSN_GPIO_GP13, true);

    // L3GD20H設定
    iod_i2c_gyro_write(GYRO_REG_CTRL4, 0x20); // FS1-FS0=1x: 2000 dps
    iod_i2c_gyro_write(GYRO_REG_CTRL3, 0xA0); // INT1_IG=1: enable on INT1 pin, H_Lactive=1: low
    iod_i2c_gyro_write(GYRO_REG_IG_CFG, 0x20); // ZHIE=1: enable interrupt request on measured angular rate value higher than preset threshold
    iod_i2c_gyro_write(GYRO_REG_IG_THS_ZH, 0x40); // THSZ14-THSZ8=100 0000: Interrupt threshold on Z axis
    iod_i2c_gyro_write(GYRO_REG_IG_THS_ZL, 0x00); // THSZ7-THSZ0=0000 0000: Interrupt threshold on Z axis
    iod_i2c_gyro_write(GYRO_REG_CTRL1, 0x0F); // PD=1: Normal Mode
#endif
}

void iod_i2c_gyro_deinit() {
}

void iod_i2c_gyro_reinit() {
}

void iod_i2c_gyro_main_1ms() {
}

void iod_i2c_gyro_main_in() {
#if SELECT_I2C_GYRO
    uint8_t u8a_index;

    for (u8a_index = 0;u8a_index < GYRO_GROUP_NUM; u8a_index++) {
        iod_i2c_gyro_read(acu8s_gyro_address[u8a_index], (uint8_t *)&as16s_gyro_value[u8a_index], sizeof(int16_t));
    }
#endif
}

void iod_i2c_gyro_main_out() {
}

#if SELECT_I2C_GYRO
void iod_read_gyro_x_value(int16_t *ps16a_value) {
    *ps16a_value = as16s_gyro_value[GYRO_X];
}

void iod_read_gyro_y_value(int16_t *ps16a_value) {
    *ps16a_value = as16s_gyro_value[GYRO_Y];
}

void iod_read_gyro_z_value(int16_t *ps16a_value) {
    *ps16a_value = as16s_gyro_value[GYRO_Z];
}
#endif

// 内部関数
#if SELECT_I2C_GYRO
static void iod_i2c_gyro_read(uint8_t u8a_address, uint8_t *pu8a_buffer, uint8_t u8a_size) {
    // 読み出し操作コマンド（複数バイト読み出し）
    au8s_tx_buffer[0] = (u8a_address & 0x7F) | GYRO_READ_OPT;
    // 読み出し操作
    i2c_write_blocking(I2C1_ID, GYRO_ADDRESS, au8s_tx_buffer, GYRO_READ_SIZE, true);
    i2c_read_blocking(I2C1_ID, GYRO_ADDRESS, pu8a_buffer, u8a_size, false);
}

static void iod_i2c_gyro_write(uint8_t u8a_address, uint8_t u8a_data) {
    // 書き込み操作コマンド（1バイト書き込み）
    au8s_tx_buffer[0] = (u8a_address & 0x7F) | GYRO_WRITE_OPT;
    au8s_tx_buffer[1] = u8a_data;
    // 書き込み操作
    i2c_write_blocking(I2C1_ID, GYRO_ADDRESS, au8s_tx_buffer, GYRO_WRITE_SIZE, false);
}
#endif
