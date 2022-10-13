#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "iod_main.h"
#include "iod_i2c.h"

#define I2C0_ID             i2c0
#define I2C0_SDA_GPIO_GP20  GPIO_GP20_I2C
#define I2C0_SCL_GPIO_GP21  GPIO_GP21_I2C

#define EEP_TARGET_SIZE     (8192) // 24FC64-I/P のROMサイズ
#define EEP_BLOCK_SIZE      (64) // 読み書き用ブロックのサイズ（任意）
#define EEP_PAGE_SIZE       (32) // 24FC64-I/P のページサイズ
#define EEP_HEADER_SIZE     (4) // EEP_BLOCK_SIZE のうちヘッダー部のサイズ
#define EEP_HEADER_VALUE_A  (0xAA55AA55)
#define EEP_HEADER_VALUE_B  (~EEP_HEADER_VALUE_A)
#define EEP_BLOCK_NUM       (EEP_TARGET_SIZE / EEP_BLOCK_SIZE)
#define I2C_EEP_DATA_SIZE   (EEP_BLOCK_SIZE - EEP_HEADER_SIZE)

/* 定義の整合チェック */
#if (I2C_EEP_DATA_SIZE != IOD_I2C_EEP_DATA_SIZE)
#error IOD_I2C_EEP_DATA_SIZE is not match I2C_EEP_DATA_SIZE.
#endif

// 24FC64-I/P のアドレス
// Address 7bit: ControlCode=0b1010xxx, ChipSelectBits=0bxxxx100
#define EEP_ADDRESS         (0b01010100)

// 読み出し操作（ページ読み出し）
#define EEP_READ_SIZE       (2)
// 書き込み操作（ページ書き込み）
#define EEP_WRITE_SIZE      (2)

struct iod_i2c_eep_buffer {
    uint32_t u32_header;
    uint8_t au8_data[I2C_EEP_DATA_SIZE];
};

struct iod_i2c_eep_info {
    bool bl_status;
    bool bl_request;
    uint16_t u16_index;
    uint32_t u32_header;
};

static struct iod_i2c_eep_buffer sts_eep_buffer;
static struct iod_i2c_eep_info sts_eep_info;
static uint8_t au8s_tx_buffer[EEP_PAGE_SIZE + EEP_WRITE_SIZE];

static void iod_i2c_eep_clear();
static void iod_i2c_eep_check_data();
static void iod_i2c_eep_read_data();
static void iod_i2c_eep_write_data();
static void iod_i2c_eep_block_read(uint16_t, uint8_t *, uint16_t);
static void iod_i2c_eep_read(uint16_t, uint8_t *, uint16_t);
static bool iod_i2c_eep_block_write(uint16_t, uint8_t *, uint16_t);
static void iod_i2c_eep_write(uint16_t, uint8_t *, uint16_t);
static bool iod_i2c_eep_isbusy();

// 外部公開関数
void iod_i2c_eep_init() {
    // I2C0の初期設定（クロックは 1MHz）
    i2c_init(I2C0_ID, 1000*1000);
    gpio_set_function(I2C0_SDA_GPIO_GP20, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL_GPIO_GP21, GPIO_FUNC_I2C);
    //gpio_pull_up(I2C0_SDA_GPIO_GP20);
    //gpio_pull_up(I2C0_SCL_GPIO_GP21);

    iod_i2c_eep_clear();
    iod_i2c_eep_check_data();
    iod_i2c_eep_read_data();
}

void iod_i2c_eep_deinit() {
    if (sts_eep_info.bl_request) {
        sts_eep_info.bl_request = false;
        iod_i2c_eep_write_data();
    }
}

void iod_i2c_eep_reinit() {
    iod_i2c_eep_clear();
    iod_i2c_eep_check_data();
    iod_i2c_eep_read_data();
}

void iod_i2c_eep_main_1ms() {
}

void iod_i2c_eep_main_in() {
}

void iod_i2c_eep_main_out() {
}

bool iod_call_i2c_eep_read(uint8_t *pu8a_buffer, uint16_t u16a_size) {
    bool bla_rcode = false;

    if (sts_eep_info.bl_status && (u16a_size <= I2C_EEP_DATA_SIZE)) {
        memcpy(pu8a_buffer, sts_eep_buffer.au8_data, u16a_size);
        bla_rcode = true;
    }
    return bla_rcode;
}

bool iod_call_i2c_eep_write(uint8_t *pu8a_buffer, uint16_t u16a_size) {
    bool bla_rcode = false;

    if (u16a_size <= I2C_EEP_DATA_SIZE) {
        sts_eep_info.bl_request = true;
        memcpy(sts_eep_buffer.au8_data, pu8a_buffer, u16a_size);
        bla_rcode = true;
    }
    return bla_rcode;
}

// 内部関数
static void iod_i2c_eep_clear() {
    memset(&sts_eep_buffer, 0, sizeof(sts_eep_buffer));
    sts_eep_info.bl_status = false;
    sts_eep_info.bl_request = false;
    sts_eep_info.u16_index = 0;
    sts_eep_info.u32_header = EEP_HEADER_VALUE_A;
}

static void iod_i2c_eep_check_data() {
    uint16_t u16a_address;
    uint32_t u32a_header;
    uint16_t u16a_index;

    for (u16a_index = 0; u16a_index < EEP_BLOCK_NUM; u16a_index++) {
        u16a_address = EEP_BLOCK_SIZE * u16a_index;
        iod_i2c_eep_read(u16a_address, (uint8_t *)&u32a_header, sizeof(u32a_header));
        if (u16a_index == 0) {
            // 先頭データのヘッダー部を確認
            if (u32a_header == EEP_HEADER_VALUE_A || u32a_header == EEP_HEADER_VALUE_B) {
                sts_eep_info.u32_header = u32a_header;
                sts_eep_info.u16_index = 0;
                sts_eep_info.bl_status = true;
            } else {
                sts_eep_info.u32_header = EEP_HEADER_VALUE_A;
                sts_eep_info.u16_index = 0;
                sts_eep_info.bl_status = false;
                break;
            }
        } else {
            if (u32a_header == sts_eep_info.u32_header) {
                sts_eep_info.u16_index = u16a_index;
                sts_eep_info.bl_status = true;
            } else {
                break;
            }
        }
    }
}

static void iod_i2c_eep_read_data() {
    uint16_t u16a_address;

    if (sts_eep_info.bl_status) {
        u16a_address = EEP_BLOCK_SIZE * sts_eep_info.u16_index;
        iod_i2c_eep_block_read(u16a_address, (uint8_t *)&sts_eep_buffer, sizeof(sts_eep_buffer));
    }
}

static void iod_i2c_eep_write_data() {
    uint16_t u16a_address;

    if (sts_eep_info.bl_status) {
        sts_eep_info.u16_index++;
        if (sts_eep_info.u16_index >= EEP_BLOCK_NUM) {
            sts_eep_info.u16_index = 0;
            sts_eep_info.u32_header = ~sts_eep_info.u32_header;
        }
    }
    u16a_address = EEP_BLOCK_SIZE * sts_eep_info.u16_index;
    sts_eep_buffer.u32_header = sts_eep_info.u32_header;
    iod_i2c_eep_block_write(u16a_address, (uint8_t *)&sts_eep_buffer, sizeof(sts_eep_buffer));
}

static void iod_i2c_eep_block_read(uint16_t u16a_address, uint8_t *pu8a_buffer, uint16_t u16a_size) {
    const uint16_t u16a_page_num = u16a_size / EEP_PAGE_SIZE;
    uint16_t u16a_address_page;
    uint8_t *pu8a_buffer_page;
    uint16_t u16a_index;

    for (u16a_index = 0; u16a_index < u16a_page_num; u16a_index++) {
        u16a_address_page = u16a_address + EEP_PAGE_SIZE * u16a_index;
        pu8a_buffer_page = pu8a_buffer + EEP_PAGE_SIZE * u16a_index;
        iod_i2c_eep_read(u16a_address_page, pu8a_buffer_page, EEP_PAGE_SIZE);
    }
}

static void iod_i2c_eep_read(uint16_t u16a_address, uint8_t *pu8a_buffer, uint16_t u16a_size) {
    // 読み出し操作コマンド（ページ読み出し）
    au8s_tx_buffer[0] = (u16a_address >> 8) & 0xFF;
    au8s_tx_buffer[1] = u16a_address & 0xFF;
    // 読み出し操作
    i2c_write_blocking(I2C0_ID, EEP_ADDRESS, au8s_tx_buffer, EEP_READ_SIZE, true);
    i2c_read_blocking(I2C0_ID, EEP_ADDRESS, pu8a_buffer, u16a_size, false);
}

static bool iod_i2c_eep_block_write(uint16_t u16a_address, uint8_t *pu8a_buffer, uint16_t u16a_size) {
    const uint16_t u16a_page_num = u16a_size / EEP_PAGE_SIZE;
    uint16_t u16a_address_page;
    uint8_t *pu8a_buffer_page;
    uint16_t u16a_index;

    for (u16a_index = 0; u16a_index < u16a_page_num; u16a_index++) {
        u16a_address_page = u16a_address + EEP_PAGE_SIZE * u16a_index;
        pu8a_buffer_page = pu8a_buffer + EEP_PAGE_SIZE * u16a_index;
        iod_i2c_eep_write(u16a_address_page, pu8a_buffer_page, EEP_PAGE_SIZE);
        // BUSY状態の間は待機（約3.5ms）
        while (iod_i2c_eep_isbusy()) {
            tight_loop_contents();
        }
    }
}

static void iod_i2c_eep_write(uint16_t u16a_address, uint8_t *pu8a_buffer, uint16_t u16a_size) {
    // 書き込み操作コマンド（ページ書き込み）
    au8s_tx_buffer[0] = (u16a_address >> 8) & 0xFF;
    au8s_tx_buffer[1] = u16a_address & 0xFF;
    memcpy(&au8s_tx_buffer[EEP_WRITE_SIZE], pu8a_buffer, u16a_size);
    // 書き込み操作
    i2c_write_blocking(I2C0_ID, EEP_ADDRESS, au8s_tx_buffer, u16a_size + EEP_WRITE_SIZE, false);
}

static bool iod_i2c_eep_isbusy() {
    uint32_t u32a_rcode;
    uint8_t u8a_data;
    bool bla_rcode;

    u32a_rcode = i2c_read_blocking(I2C0_ID, EEP_ADDRESS, &u8a_data, sizeof(u8a_data), false);
    bla_rcode = (u32a_rcode == PICO_ERROR_GENERIC) ? true : false;

    return bla_rcode;
}
