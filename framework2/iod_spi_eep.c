#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "iod_main.h"
#include "iod_spi.h"

#define SPI0_ID             spi0
#define SPI0_RX_GPIO_GP16   GPIO_GP16_SPI
#define SPI0_CSN_GPIO_GP17  GPIO_GP17_SPI
#define SPI0_SCK_GPIO_GP18  GPIO_GP18_SPI
#define SPI0_TX_GPIO_GP19   GPIO_GP19_SPI

#define EEP_TARGET_SIZE     (128) // 93AA46C-I/P のROMサイズ
#define EEP_BLOCK_SIZE      (32) // 読み書き用ブロックのサイズ（任意）
#define EEP_HEADER_SIZE     (4) // EEP_BLOCK_SIZE のうちヘッダー部のサイズ
#define EEP_HEADER_VALUE_A  (0xAA55AA55)
#define EEP_HEADER_VALUE_B  (~EEP_HEADER_VALUE_A)
#define EEP_BLOCK_NUM       (EEP_TARGET_SIZE / EEP_BLOCK_SIZE)
#define SPI_EEP_DATA_SIZE   (EEP_BLOCK_SIZE - EEP_HEADER_SIZE)

/* 定義の整合チェック */
#if (SPI_EEP_DATA_SIZE != IOD_SPI_EEP_DATA_SIZE)
#error IOD_SPI_EEP_DATA_SIZE is not match SPI_EEP_DATA_SIZE.
#endif

// 読み出し操作コマンド（複数バイト読み出し）
// Microwire通信では、READ命令でダミー「0」ビットが出力される
// その対策として、命令を 1ビット前にずらしておく
// READ SB+Opcode=110, Address=7bit, dummy=1bit
#define EEP_OPERATE_READ1       (0b00000110)
#define EEP_OPERATE_READ2       (0b00000000)
#define EEP_OPERATE_READ_SIZE   (2)
// 書き込み許可コマンド
// EWEN SB+Opcode=10011, dummy=5bit
#define EEP_OPERATE_EWEN1       (0b00000010)
#define EEP_OPERATE_EWEN2       (0b01100000)
#define EEP_OPERATE_EWEN_SIZE   (2)
// 書き込み禁止コマンド
// EWDS SB+Opcode=10011, dummy=5bit
#define EEP_OPERATE_EWDS1       (0b00000010)
#define EEP_OPERATE_EWDS2       (0b00000000)
#define EEP_OPERATE_EWDS_SIZE   (2)
// 書き込み操作コマンド（1バイト書き込み）
// WRITE SB+Opcode=101, Address=7bit, DataIn=8bit
#define EEP_OPERATE_WRITE1      (0b00000010)
#define EEP_OPERATE_WRITE2      (0b10000000)
#define EEP_OPERATE_WRITE_SIZE  (3)

struct iod_spi_eep_buffer {
    uint32_t u32_header;
    uint8_t au8_data[SPI_EEP_DATA_SIZE];
};

struct iod_spi_eep_info {
    bool bl_status;
    bool bl_request;
    uint8_t u8_index;
    uint32_t u32_header;
};

static struct iod_spi_eep_buffer sts_eep_buffer;
static struct iod_spi_eep_info sts_eep_info;
static uint8_t au8s_tx_buffer[3];

static void iod_spi_eep_clear();
static void iod_spi_eep_check_data();
static void iod_spi_eep_read_data();
static void iod_spi_eep_write_data();
static void iod_spi_eep_block_read(uint8_t, uint8_t *, uint8_t);
static void iod_spi_eep_write_enable();
static void iod_spi_eep_write_disable();
static bool iod_spi_eep_block_write(uint8_t, uint8_t *, uint8_t);
static void iod_spi_eep_write(uint8_t, uint8_t);
static bool iod_spi_eep_isbusy();

// 外部公開関数
void iod_spi_eep_init() {
    // SPI0の初期設定（クロックは 2MHz）
    spi_init(SPI0_ID, 2000*1000);
    gpio_set_function(SPI0_RX_GPIO_GP16, GPIO_FUNC_SPI);
    gpio_set_function(SPI0_CSN_GPIO_GP17, GPIO_FUNC_SIO);
    gpio_set_function(SPI0_SCK_GPIO_GP18, GPIO_FUNC_SPI);
    gpio_set_function(SPI0_TX_GPIO_GP19, GPIO_FUNC_SPI);
    // Microwire通信では、CS端子は HIアクティブ（初期値 LOW）
    gpio_put(SPI0_CSN_GPIO_GP17, false);
    gpio_set_dir(SPI0_CSN_GPIO_GP17, GPIO_OUT);

    iod_spi_eep_clear();
    iod_spi_eep_check_data();
    iod_spi_eep_read_data();
}

void iod_spi_eep_deinit() {
    if (sts_eep_info.bl_request) {
        sts_eep_info.bl_request = false;
        iod_spi_eep_write_data();
    }
}

void iod_spi_eep_reinit() {
    iod_spi_eep_clear();
    iod_spi_eep_check_data();
    iod_spi_eep_read_data();
}

void iod_spi_eep_main_1ms() {
}

void iod_spi_eep_main_in() {
}

void iod_spi_eep_main_out() {
}

bool iod_call_spi_eep_read(uint8_t *pu8a_buffer, uint16_t u16a_size) {
    bool bla_rcode = false;

    if (sts_eep_info.bl_status && (u16a_size <= SPI_EEP_DATA_SIZE)) {
        memcpy(pu8a_buffer, sts_eep_buffer.au8_data, u16a_size);
        bla_rcode = true;
    }
    return bla_rcode;
}

bool iod_call_spi_eep_write(uint8_t *pu8a_buffer, uint16_t u16a_size) {
    bool bla_rcode = false;

    if (u16a_size <= SPI_EEP_DATA_SIZE) {
        sts_eep_info.bl_request = true;
        memcpy(sts_eep_buffer.au8_data, pu8a_buffer, u16a_size);
        bla_rcode = true;
    }
    return bla_rcode;
}

// 内部関数
static void iod_spi_eep_clear() {
    memset(&sts_eep_buffer, 0, sizeof(sts_eep_buffer));
    sts_eep_info.bl_status = false;
    sts_eep_info.bl_request = false;
    sts_eep_info.u8_index = 0;
    sts_eep_info.u32_header = EEP_HEADER_VALUE_A;
}

static void iod_spi_eep_check_data() {
    uint8_t u8a_address;
    uint32_t u32a_header;
    uint8_t u8a_index;

    for (u8a_index = 0; u8a_index < EEP_BLOCK_NUM; u8a_index++) {
        u8a_address = EEP_BLOCK_SIZE * u8a_index;
        iod_spi_eep_block_read(u8a_address, (uint8_t *)&u32a_header, sizeof(u32a_header));
        if (u8a_index == 0) {
            // 先頭データのヘッダー部を確認
            if (u32a_header == EEP_HEADER_VALUE_A || u32a_header == EEP_HEADER_VALUE_B) {
                sts_eep_info.u32_header = u32a_header;
                sts_eep_info.u8_index = 0;
                sts_eep_info.bl_status = true;
            } else {
                sts_eep_info.u32_header = EEP_HEADER_VALUE_A;
                sts_eep_info.u8_index = 0;
                sts_eep_info.bl_status = false;
                break;
            }
        } else {
            if (u32a_header == sts_eep_info.u32_header) {
                sts_eep_info.u8_index = u8a_index;
                sts_eep_info.bl_status = true;
            } else {
                break;
            }
        }
    }
}

static void iod_spi_eep_read_data() {
    uint8_t u8a_address;

    if (sts_eep_info.bl_status) {
        u8a_address = EEP_BLOCK_SIZE * sts_eep_info.u8_index;
        iod_spi_eep_block_read(u8a_address, (uint8_t *)&sts_eep_buffer, sizeof(sts_eep_buffer));
    }
}

static void iod_spi_eep_write_data() {
    uint8_t u8a_address;

    if (sts_eep_info.bl_status) {
        sts_eep_info.u8_index++;
        if (sts_eep_info.u8_index >= EEP_BLOCK_NUM) {
            sts_eep_info.u8_index = 0;
            sts_eep_info.u32_header = ~sts_eep_info.u32_header;
        }
    }
    u8a_address = EEP_BLOCK_SIZE * sts_eep_info.u8_index;
    sts_eep_buffer.u32_header = sts_eep_info.u32_header;
    iod_spi_eep_block_write(u8a_address, (uint8_t *)&sts_eep_buffer, sizeof(sts_eep_buffer));
}

static void iod_spi_eep_block_read(uint8_t u8a_address, uint8_t *pu8a_buffer, uint8_t u8a_size) {
    // 読み出し操作コマンド（複数バイト読み出し）
    au8s_tx_buffer[0] = EEP_OPERATE_READ1;
    au8s_tx_buffer[1] = EEP_OPERATE_READ2 | ((u8a_address & 0x7F) << 1);
    // 読み出し操作
    gpio_put(SPI0_CSN_GPIO_GP17, true);
    spi_write_blocking(SPI0_ID, au8s_tx_buffer, EEP_OPERATE_READ_SIZE);
    spi_read_blocking(SPI0_ID, 0, pu8a_buffer, u8a_size);
    gpio_put(SPI0_CSN_GPIO_GP17, false);
}

static void iod_spi_eep_write_enable() {
    // 書き込み許可コマンド
    au8s_tx_buffer[0] = EEP_OPERATE_EWEN1;
    au8s_tx_buffer[1] = EEP_OPERATE_EWEN2;
    // 書き込み許可
    gpio_put(SPI0_CSN_GPIO_GP17, true);
    spi_write_blocking(SPI0_ID, au8s_tx_buffer, EEP_OPERATE_EWEN_SIZE);
    gpio_put(SPI0_CSN_GPIO_GP17, false);
}

static void iod_spi_eep_write_disable() {
    // 書き込み禁止コマンド
    au8s_tx_buffer[0] = EEP_OPERATE_EWDS1;
    au8s_tx_buffer[1] = EEP_OPERATE_EWDS2;
    // 書き込み禁止
    gpio_put(SPI0_CSN_GPIO_GP17, true);
    spi_write_blocking(SPI0_ID, au8s_tx_buffer, EEP_OPERATE_EWDS_SIZE);
    gpio_put(SPI0_CSN_GPIO_GP17, false);
}

static bool iod_spi_eep_block_write(uint8_t u8a_address, uint8_t *pu8a_buffer, uint8_t u8a_size) {
    uint8_t u8a_index;

    iod_spi_eep_write_enable();
    for (u8a_index = 0; u8a_index < u8a_size; u8a_index++) {
        iod_spi_eep_write(u8a_address + u8a_index, pu8a_buffer[u8a_index]);
        // BUSY状態の間は待機（約3.4ms）
        while (iod_spi_eep_isbusy()) {
            tight_loop_contents();
        }
    }
    iod_spi_eep_write_disable();
}

static void iod_spi_eep_write(uint8_t u8a_address, uint8_t u8a_data) {
    // 書き込み操作コマンド（1バイト書き込み）
    au8s_tx_buffer[0] = EEP_OPERATE_WRITE1;
    au8s_tx_buffer[1] = EEP_OPERATE_WRITE2 | (u8a_address & 0x7F);
    au8s_tx_buffer[2] = u8a_data;
    // 書き込み操作
    gpio_put(SPI0_CSN_GPIO_GP17, true);
    spi_write_blocking(SPI0_ID, au8s_tx_buffer, EEP_OPERATE_WRITE_SIZE);
    gpio_put(SPI0_CSN_GPIO_GP17, false);
}

static bool iod_spi_eep_isbusy() {
    uint8_t u8a_data;
    bool bla_rcode;

    gpio_put(SPI0_CSN_GPIO_GP17, true);
    spi_read_blocking(SPI0_ID, 0, &u8a_data, 1);
    gpio_put(SPI0_CSN_GPIO_GP17, false);
    bla_rcode = (u8a_data != 0xFF) ? true : false;

    return bla_rcode;
}
