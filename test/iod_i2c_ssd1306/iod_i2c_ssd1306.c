#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// iod_main
#define GPIO_GP8_I2C        (8)
#define GPIO_GP9_I2C        (9)

#define I2C0_ID             i2c0
#define I2C0_SDA_GPIO       GPIO_GP8_I2C
#define I2C0_SCL_GPIO       GPIO_GP9_I2C

// SSD1306 のアドレス
// Address 7bit: SlaveAddress=0b0111100
#define SSD1306_ADDRESS             (0b00111100)

// 画面分割の設定
#define SSD1306_PAGE_SIZE           (8)
#define SSD1306_COLUMN_SIZE         (128)
#define DISPLAY_CHARACTER_WIDTH     (8)
#define DISPLAY_HEIGHT_BLOCK_NUM    SSD1306_PAGE_SIZE
#define DISPLAY_WIDTH_BLOCK_NUM     (SSD1306_COLUMN_SIZE / DISPLAY_CHARACTER_WIDTH)

// アドレス設定コマンドの位置指定
#define COMMAND_POS_PAGE            (1)
#define COMMAND_POS_COLUMN_START    (4)
#define COMMAND_POS_COLUMN_STOP     (5)

enum character_group {
    CHARACTER_BLACK = 0,
    CHARACTER_WHITE,
    CHARACTER_GRAY,
    CHARACTER_HALF,
    CHARACTER_SYMBOL,
    CHARACTER_GROUP_NUM
};

const uint8_t cau8s_command_initial[] = {
    // Control byte:
    // 7bit - Co(Continuation bit) = 0(continue) / 1,
    // 6bit - D/C#(Data/Comman Selection bit) = 0(command) / 1(data),
    // 5-0bit - 000000
    0b10000000, 0xAE, // Set Display Off 0xAE
    0b00000000, 0xA8, 0b00111111, // Set Multiplex Ratio 0xA8, 0x3F(64MUX)
    0b00000000, 0xD3, 0x00, // Set Display Offset 0xD3, 0x00
    0b10000000, 0x40, // Set Display Start Line 0x40
    0b10000000, 0xA1, // Set Segment re-map 0xA0/0xA1
    0b10000000, 0xC8, // Set COM Output Scan Direction 0xC0/0xC8
    0b00000000, 0xDA, 0b00010010, // Set COM Pins hardware configuration 0xDA, 0x12
    0b00000000, 0x81, 255, // Set Contrast Control 0x81, default=0x7F(0-255)
    0b10000000, 0xA4, // Disable Entire Display On
    0b10000000, 0xA6, // Set Normal Display 0xA6, Inverse display 0xA7
    0b00000000, 0xD5, 0b10000000, // Set Display Clock Divide Ratio/Oscillator Frequency 0xD5, 0x80
    0b00000000, 0x20, 0x10, // Set Memory Addressing Mode (Page addressing mode)
    0b00000000, 0x22, 0, 7, // Set Page Address (Start page set, End page set)
    0b00000000, 0x21, 0, 127, // Set Column Address (Column Start Address, Column Stop Address)
    0b00000000, 0x8D, 0x14, // Set Enable charge pump regulator 0x8D, 0x14
    0b10000000, 0xAF // Display On 0xAF
};

uint8_t cau8s_command_set_address[] = {
    0b10000000, 0xB0, // Set Page Start address(B0～B7)
    0b00000000, 0x21, 0, 127 // Set Column Address (Column Start Address(0-127), Column Stop Address(0-127))
};

const uint8_t cau8s_data_character[CHARACTER_GROUP_NUM][9] = {
    { // CHARACTER_BLACK
        0b01000000, // Control byte, Co bit = 0(continue), D/C# = 1(data)
        // Data byte
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000
    },
    { // CHARACTER_WHITE
        0b01000000, // Control byte, Co bit = 0(continue), D/C# = 1(data)
        // Data byte
        0b11111111,
        0b11111111,
        0b11111111,
        0b11111111,
        0b11111111,
        0b11111111,
        0b11111111,
        0b11111111
    },
    { // CHARACTER_GRAY
        0b01000000, // Control byte, Co bit = 0(continue), D/C# = 1(data)
        // Data byte
        0b01010101,
        0b10101010,
        0b01010101,
        0b10101010,
        0b01010101,
        0b10101010,
        0b01010101,
        0b10101010
    },
    { // CHARACTER_HALF
        0b01000000, // Control byte, Co bit = 0(continue), D/C# = 1(data)
        // Data byte
        0b11111111,
        0b01111111,
        0b00111111,
        0b00011111,
        0b00001111,
        0b00000111,
        0b00000011,
        0b00000001
    },
    { // CHARACTER_SYMBOL
        0b01000000, // Control byte, Co bit = 0(continue), D/C# = 1(data)
        // Data byte
        0b11111111,
        0b00000111,
        0b11111111,
        0b00000011,
        0b00000101,
        0b00001001,
        0b00010001,
        0b00100001
    }
};

static uint8_t au8s_tx_buffer[32];
static uint8_t u8s_row;
static uint8_t u8s_column;
enum character_group u8s_character;

// iod_i2c_ssd1306
extern void iod_i2c_ssd1306_init();
extern void iod_i2c_ssd1306_deinit();
extern void iod_i2c_ssd1306_reinit();
extern void iod_i2c_ssd1306_main_1ms();
extern void iod_i2c_ssd1306_main_in();
extern void iod_i2c_ssd1306_main_out();

static void iod_i2c_ssd1306_display_clear();
static void iod_i2c_ssd1306_display_set(uint8_t, uint8_t, enum character_group);
static void iod_i2c_ssd1306_write(const uint8_t *, uint8_t);

// 外部公開関数
void main() {
    uint8_t u8a_count = 0;

    stdio_init_all();
    iod_i2c_ssd1306_init();

    while (true) {
        iod_i2c_ssd1306_main_in();
        {
            printf("pass. (%d)\n", u8a_count);
        }
        iod_i2c_ssd1306_main_out();
        sleep_ms(250);
        u8a_count++;
    }
}

void iod_i2c_ssd1306_init() {
    memset(au8s_tx_buffer, 0, sizeof(au8s_tx_buffer));
    u8s_row = 0;
    u8s_column = 0;
    u8s_character = CHARACTER_WHITE;

    // I2C0の初期設定（クロックは 400KHz）
    i2c_init(I2C0_ID, 400*1000);
    gpio_set_function(I2C0_SDA_GPIO, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL_GPIO, GPIO_FUNC_I2C);
    //gpio_pull_up(I2C0_SDA_GPIO);
    //gpio_pull_up(I2C0_SCL_GPIO);
    sleep_ms(100); // SSD1306 待ち時間 100ms

    // SSD1306 初期設定
    iod_i2c_ssd1306_write(cau8s_command_initial, sizeof(cau8s_command_initial));
    sleep_ms(10); // SSD1306 待ち時間 10ms
    // 画面のクリア
    iod_i2c_ssd1306_display_clear();
}

void iod_i2c_ssd1306_deinit() {
}

void iod_i2c_ssd1306_reinit() {
}

void iod_i2c_ssd1306_main_1ms() {
}

void iod_i2c_ssd1306_main_in() {
}

void iod_i2c_ssd1306_main_out() {
    // 文字の描画
    iod_i2c_ssd1306_display_set(u8s_row, u8s_column, u8s_character);
    u8s_column++;
    if (u8s_column >= DISPLAY_WIDTH_BLOCK_NUM) {
        u8s_row++;
        if (u8s_row >= DISPLAY_HEIGHT_BLOCK_NUM) {
            u8s_character++;
            u8s_character = (u8s_character < CHARACTER_GROUP_NUM) ? u8s_character : CHARACTER_BLACK;
            u8s_row = 0;
        }
        u8s_column = 0;
    }
}

// 内部関数
static void iod_i2c_ssd1306_display_clear() {
    uint8_t u8a_index_row;
    uint8_t u8a_index_column;

    for (u8a_index_row = 0; u8a_index_row < DISPLAY_HEIGHT_BLOCK_NUM; u8a_index_row++) {
        // ページと列範囲のアドレス指定
        memcpy(au8s_tx_buffer, cau8s_command_set_address, sizeof(cau8s_command_set_address));
        au8s_tx_buffer[COMMAND_POS_PAGE] |= u8a_index_row;
        iod_i2c_ssd1306_write(au8s_tx_buffer, sizeof(cau8s_command_set_address));

        // 画面のクリア
        for (u8a_index_column = 0; u8a_index_column < DISPLAY_WIDTH_BLOCK_NUM; u8a_index_column++) {
            iod_i2c_ssd1306_write(cau8s_data_character[CHARACTER_BLACK], sizeof(cau8s_data_character[CHARACTER_BLACK]));
        }
    }
}

static void iod_i2c_ssd1306_display_set(uint8_t u8a_row, uint8_t u8a_column, enum character_group u8a_character) {
    // ページと列範囲のアドレス指定
    memcpy(au8s_tx_buffer, cau8s_command_set_address, sizeof(cau8s_command_set_address));
    au8s_tx_buffer[COMMAND_POS_PAGE] |= u8a_row;
    au8s_tx_buffer[COMMAND_POS_COLUMN_START] = u8a_column * DISPLAY_CHARACTER_WIDTH;
    au8s_tx_buffer[COMMAND_POS_COLUMN_STOP] = (u8a_column + 1) * DISPLAY_CHARACTER_WIDTH - 1;
    iod_i2c_ssd1306_write(au8s_tx_buffer, sizeof(cau8s_command_set_address));

    // キャラクタの描画
    iod_i2c_ssd1306_write(cau8s_data_character[u8a_character], sizeof(cau8s_data_character[u8a_character]));
}

static void iod_i2c_ssd1306_write(const uint8_t *pu8a_buffer, uint8_t u8a_size) {
    // 書き込み操作
    i2c_write_blocking(I2C0_ID, SSD1306_ADDRESS, pu8a_buffer, u8a_size, false);
}
