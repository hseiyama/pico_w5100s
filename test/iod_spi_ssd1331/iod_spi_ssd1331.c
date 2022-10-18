#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

// iod_main
#define GPIO_GP6_SPI        (6)
#define GPIO_GP7_SPI        (7)
#define GPIO_GP12_SPI       (12)
#define GPIO_GP13_SPI       (13)
#define GPIO_GP14_SPI       (14)
#define GPIO_GP15_SPI       (15)

#define SPI1_ID             spi1
#define OLED_DC_GPIO        GPIO_GP6_SPI
#define OLED_RES_GPIO       GPIO_GP7_SPI
#define SPI1_RX_GPIO        GPIO_GP12_SPI
#define SPI1_CSN_GPIO       GPIO_GP13_SPI
#define SPI1_SCK_GPIO       GPIO_GP14_SPI
#define SPI1_TX_GPIO        GPIO_GP15_SPI

const uint8_t cau8s_command_initial[] = {
    0xAE, // Set Display Off
    0xA0, 0b00110010, // Remap & Color Depth setting, A[7:6]=00(256 color) 01(65k color format)
    0xA1, 0, // Set Display Start Line
    0xA2, 0, // Set Display Offset
    0xA4, // Set Display Mode (Normal)
    0xA8, 0b00111111, // Set Multiplex Ratio (15-63)
    0xAD, 0b10001110, // Set Master Configration, A[0]=0(Select external Vcc supply) 1(Reserved(reset))
    0xB0, 0b00000000, // Power Save Mode  0x1A Enable power save mode
    0xB1, 0x74, // Phase 1 and 2 period adjustment
    0xB3, 0xF0, // Display Clock DIV
    0x8A, 0x81, // Pre Charge A
    0x8B, 0x82, // Pre Charge B
    0x8C, 0x83, // Pre Charge C
    0xBB, 0x3A, // Set Pre-charge level
    0xBE, 0x3E, // Set VcomH
    0x87, 0x06, // Set Master Current Control
    0x15, 0, 95, // Set Column Address
    0x75, 0, 63, // Set Row Address
    0x81, 255, // Set Contrast for Color A
    0x82, 255, // Set Contrast for Color B
    0x83, 255, // Set Contrast for Color C
    0xAF // Set Display On
};

static uint8_t au8s_tx_buffer[32];

// iod_spi_ssd1331
extern void iod_spi_ssd1331_init();
extern void iod_spi_ssd1331_deinit();
extern void iod_spi_ssd1331_reinit();
extern void iod_spi_ssd1331_main_1ms();
extern void iod_spi_ssd1331_main_in();
extern void iod_spi_ssd1331_main_out();

static void iod_spi_ssd1331_brightness(uint8_t);
static void iod_spi_ssd1331_clear_window(uint8_t, uint8_t, uint8_t, uint8_t);
static void iod_spi_ssd1331_copy(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);
static void iod_spi_ssd1331_draw_pixcel_256_color(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);
static void iod_spi_ssd1331_draw_pixcel_65k_color(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);
static void iod_spi_ssd1331_set_pixcel(uint8_t, uint8_t);
static void iod_spi_ssd1331_draw_line(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);
static void iod_spi_ssd1331_draw_rectangle(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);
static void iod_spi_ssd1331_draw_rectangle_fill(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);
static void iod_spi_ssd1331_write_command(const uint8_t *, uint8_t);
static void iod_spi_ssd1331_write_data(const uint8_t *, uint8_t);

// 外部公開関数
void main() {
    uint8_t u8a_count = 0;

    stdio_init_all();
    iod_spi_ssd1331_init();

    while (true) {
        iod_spi_ssd1331_main_in();
        {
            printf("pass. (%d)\n", u8a_count);
        }
        iod_spi_ssd1331_main_out();
        sleep_ms(250);
        u8a_count++;
    }
}

void iod_spi_ssd1331_init() {
    memset(au8s_tx_buffer, 0, sizeof(au8s_tx_buffer));

    // SPI1の初期設定（クロックは 7MHz）
    spi_init(SPI1_ID, 7000*1000);
    gpio_set_function(OLED_DC_GPIO, GPIO_FUNC_SIO);
    gpio_set_function(OLED_RES_GPIO, GPIO_FUNC_SIO);
    gpio_set_function(SPI1_RX_GPIO, GPIO_FUNC_SPI);
    gpio_set_function(SPI1_CSN_GPIO, GPIO_FUNC_SIO);
    gpio_set_function(SPI1_SCK_GPIO, GPIO_FUNC_SPI);
    gpio_set_function(SPI1_TX_GPIO, GPIO_FUNC_SPI);
    spi_set_format(SPI1_ID, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST); // SPI通信モード3
    // SPI通信では、CS端子は LOWアクティブ（初期値 HI）
    gpio_put(SPI1_CSN_GPIO, true);
    gpio_set_dir(SPI1_CSN_GPIO, GPIO_OUT);
    // SSD1331 では、他に DC#端子と RES#端子の制御が必要
    gpio_put(OLED_DC_GPIO, true);
    gpio_put(OLED_RES_GPIO, true);
    gpio_set_dir(OLED_DC_GPIO, GPIO_OUT);
    gpio_set_dir(OLED_RES_GPIO, GPIO_OUT);
    // SSD1331 RES#端子の制御（リセット）
    gpio_put(OLED_RES_GPIO, false);
    sleep_ms(1); // SSD1331 待ち時間は最低 3us必要
    gpio_put(OLED_RES_GPIO, true);

    // SSD1331 初期設定
    iod_spi_ssd1331_write_command(cau8s_command_initial, sizeof(cau8s_command_initial));
    sleep_ms(100); // ディスプレイONコマンドの後は最低 100ms必要

    uint8_t R = 7, G = 0, B = 0;
    uint8_t Dot1 = (R << 5) | (G << 2) | B; // //256 color : R (0-7), G (0-7), B (0-3)
    for(uint8_t j=0; j<64; j++){
        for(uint8_t i=0; i<96; i++){
            iod_spi_ssd1331_write_data(&Dot1, 1);
        }
    }

    sleep_ms(1000);
    iod_spi_ssd1331_clear_window(30, 30, 40, 40);
    sleep_ms(1000);
    iod_spi_ssd1331_brightness(127);
    sleep_ms(1000);
    iod_spi_ssd1331_brightness(255);
    sleep_ms(1000);
    iod_spi_ssd1331_copy(25, 25, 45, 45, 50, 25);
    sleep_ms(1000);
    iod_spi_ssd1331_draw_pixcel_256_color(5, 5, 7, 7, 3);
    sleep_ms(1000);
    iod_spi_ssd1331_draw_pixcel_65k_color(10, 10, 31, 63, 31);
    sleep_ms(1000);
    iod_spi_ssd1331_draw_line(0, 63, 63, 0, 31, 63, 31);
    sleep_ms(1000);
    iod_spi_ssd1331_draw_rectangle(28, 28, 42, 42, 31, 63, 31);
    sleep_ms(1000);
    iod_spi_ssd1331_draw_rectangle_fill(48, 18, 62, 32, 31, 63, 31, 0, 0, 31);
}

void iod_spi_ssd1331_deinit() {
}

void iod_spi_ssd1331_reinit() {
}

void iod_spi_ssd1331_main_1ms() {
}

void iod_spi_ssd1331_main_in() {
}

void iod_spi_ssd1331_main_out() {
}

// 内部関数
static void iod_spi_ssd1331_brightness(uint8_t u8a_brightness) {
    // 操作コマンド
    au8s_tx_buffer[0] = 0x81; // Set Contrast for Color A
    au8s_tx_buffer[1] = u8a_brightness;
    au8s_tx_buffer[2] = 0x82; // Set Contrast for Color B
    au8s_tx_buffer[3] = u8a_brightness;
    au8s_tx_buffer[4] = 0x83; // Set Contrast for Color C
    au8s_tx_buffer[5] = u8a_brightness;
    // コマンド書き込み操作
    iod_spi_ssd1331_write_command(au8s_tx_buffer, 6);
}

static void iod_spi_ssd1331_clear_window(uint8_t u8a_x0, uint8_t u8a_y0, uint8_t u8a_x1, uint8_t u8a_y1) {
    // 操作コマンド
    au8s_tx_buffer[0] = 0x25; // Clear Window
    au8s_tx_buffer[1] = u8a_x0; // Column Address of Start
    au8s_tx_buffer[2] = u8a_y0; // Row Address of Start
    au8s_tx_buffer[3] = u8a_x1; // Column Address of End
    au8s_tx_buffer[4] = u8a_y1; // Row Address of End
    // コマンド書き込み操作
    sleep_us(400); // クリアーコマンドは 400us以上の休止期間が必要かも
    iod_spi_ssd1331_write_command(au8s_tx_buffer, 5);
    sleep_us(800); // ここの間隔は各自調節してください
}

static void iod_spi_ssd1331_copy(uint8_t u8a_x0, uint8_t u8a_y0, uint8_t u8a_x1, uint8_t u8a_y1, uint8_t u8a_x2, uint8_t u8a_y2) {
    // 操作コマンド
    au8s_tx_buffer[0] = 0x23; //Copy Command
    au8s_tx_buffer[1] = u8a_x0; // Column Address of Start
    au8s_tx_buffer[2] = u8a_y0; // Row Address of Start
    au8s_tx_buffer[3] = u8a_x1; // Column Address of End
    au8s_tx_buffer[4] = u8a_y1; // Row Address of End
    au8s_tx_buffer[5] = u8a_x2; // Column Address of New Start
    au8s_tx_buffer[6] = u8a_y2; // Row Address of New Start
    // コマンド書き込み操作
    sleep_us(500);
    iod_spi_ssd1331_write_command(au8s_tx_buffer, 7);
    sleep_us(500);
}

static void iod_spi_ssd1331_draw_pixcel_256_color(uint8_t u8a_x, uint8_t u8a_y, uint8_t u8a_color_r, uint8_t u8a_color_g, uint8_t u8a_color_b) {
    iod_spi_ssd1331_set_pixcel(u8a_x, u8a_y);
    // 操作データ
    au8s_tx_buffer[0] = ((u8a_color_r & 0x07) << 5) | ((u8a_color_g & 0x07) << 2) | (u8a_color_b & 0x03);
    // データ書き込み操作
    iod_spi_ssd1331_write_data(au8s_tx_buffer, 1);
}

static void iod_spi_ssd1331_draw_pixcel_65k_color(uint8_t u8a_x, uint8_t u8a_y, uint8_t u8a_color_r, uint8_t u8a_color_g, uint8_t u8a_color_b) {
    iod_spi_ssd1331_set_pixcel(u8a_x, u8a_y);
    // 操作データ
    au8s_tx_buffer[0] = ((u8a_color_r & 0x1F) << 3) | ((u8a_color_g & 0x38) >> 3);
    au8s_tx_buffer[0] = ((u8a_color_g & 0x07) << 5) | (u8a_color_b & 0x1F);
    // データ書き込み操作
    iod_spi_ssd1331_write_data(au8s_tx_buffer, 2);
}

static void iod_spi_ssd1331_set_pixcel(uint8_t u8a_x, uint8_t u8a_y) {
    // 操作コマンド
    au8s_tx_buffer[0] = 0x15; // Set Column Address
    au8s_tx_buffer[1] = u8a_x;
    au8s_tx_buffer[2] = u8a_x;
    au8s_tx_buffer[3] = 0x75; // Set Row Address
    au8s_tx_buffer[4] = u8a_y;
    au8s_tx_buffer[5] = u8a_y;
    // コマンド書き込み操作
    iod_spi_ssd1331_write_command(au8s_tx_buffer, 6);
}

static void iod_spi_ssd1331_draw_line(uint8_t u8a_x0, uint8_t u8a_y0, uint8_t u8a_x1, uint8_t u8a_y1, uint8_t u8a_color_r, uint8_t u8a_color_g, uint8_t u8a_color_b) {
    // 操作コマンド
    au8s_tx_buffer[0] = 0x21; // Draw Line
    au8s_tx_buffer[1] = u8a_x0;
    au8s_tx_buffer[2] = u8a_y0;
    au8s_tx_buffer[3] = u8a_x1;
    au8s_tx_buffer[4] = u8a_y1;
    au8s_tx_buffer[5] = (u8a_color_r & 0x1F) << 1; // (0-31)
    au8s_tx_buffer[6] = u8a_color_g & 0x3F; // (0-63)
    au8s_tx_buffer[7] = (u8a_color_b & 0x1F) << 1; // (0-31)
    // コマンド書き込み操作
    iod_spi_ssd1331_write_command(au8s_tx_buffer, 8);
}

static void iod_spi_ssd1331_draw_rectangle(uint8_t u8a_x0, uint8_t u8a_y0, uint8_t u8a_x1, uint8_t u8a_y1, uint8_t u8a_color_r, uint8_t u8a_color_g, uint8_t u8a_color_b) {
    // 操作コマンド
    au8s_tx_buffer[0] = 0x26; // Fill Enable or Disable
    au8s_tx_buffer[1] = 0b00000000; // A[0]=0 Fill Disable
    au8s_tx_buffer[2] = 0x22; //Drawing Rectangle
    au8s_tx_buffer[3] = u8a_x0; //Column Address of Start
    au8s_tx_buffer[4] = u8a_y0; //Row Address of Start
    au8s_tx_buffer[5] = u8a_x1; //Column Address of End
    au8s_tx_buffer[6] = u8a_y1; //Row Address of End
    au8s_tx_buffer[7] = (u8a_color_r & 0x1F) << 1; // (0-31)
    au8s_tx_buffer[8] = u8a_color_g & 0x3F; // (0-63)
    au8s_tx_buffer[9] = (u8a_color_b & 0x1F) << 1; // (0-31)
    au8s_tx_buffer[10] = 0;
    au8s_tx_buffer[11] = 0;
    au8s_tx_buffer[12] = 0;
    // コマンド書き込み操作
    iod_spi_ssd1331_write_command(au8s_tx_buffer, 13);
}

static void iod_spi_ssd1331_draw_rectangle_fill(uint8_t u8a_x0, uint8_t u8a_y0, uint8_t u8a_x1, uint8_t u8a_y1, uint8_t u8a_line_r, uint8_t u8a_line_g, uint8_t u8a_line_b, uint8_t u8a_fill_r, uint8_t u8a_fill_g, uint8_t u8a_fill_b) {
    // 操作コマンド
    au8s_tx_buffer[0] = 0x26; // Fill Enable or Disable
    au8s_tx_buffer[1] = 0b00000001; //A[0]=1 Fill Enable
    au8s_tx_buffer[2] = 0x22; //Drawing Rectangle
    au8s_tx_buffer[3] = u8a_x0; //Column Address of Start
    au8s_tx_buffer[4] = u8a_y0; //Row Address of Start
    au8s_tx_buffer[5] = u8a_x1; //Column Address of End
    au8s_tx_buffer[6] = u8a_y1; //Row Address of End
    au8s_tx_buffer[7] = (u8a_line_r & 0x1F) << 1; // (0-31)
    au8s_tx_buffer[8] = u8a_line_g & 0x3F; // (0-63)
    au8s_tx_buffer[9] = (u8a_line_b & 0x1F) << 1; // (0-31)
    au8s_tx_buffer[10] = (u8a_fill_r & 0x1F) << 1; // (0-31)
    au8s_tx_buffer[11] = u8a_fill_g & 0x3F; // (0-63)
    au8s_tx_buffer[12] = (u8a_fill_b & 0x1F) << 1; // (0-31)
    // コマンド書き込み操作
    iod_spi_ssd1331_write_command(au8s_tx_buffer, 13);
}

static void iod_spi_ssd1331_write_command(const uint8_t *pu8a_buffer, uint8_t u8a_size) {
    // 書き込み操作
    gpio_put(SPI1_CSN_GPIO, false);
    gpio_put(OLED_DC_GPIO, false);
    spi_write_blocking(SPI1_ID, pu8a_buffer, u8a_size);
    gpio_put(SPI1_CSN_GPIO, true);
}

static void iod_spi_ssd1331_write_data(const uint8_t *pu8a_buffer, uint8_t u8a_size) {
    // 書き込み操作
    gpio_put(SPI1_CSN_GPIO, false);
    gpio_put(OLED_DC_GPIO, true);
    spi_write_blocking(SPI1_ID, pu8a_buffer, u8a_size);
    gpio_put(SPI1_CSN_GPIO, true);
}
