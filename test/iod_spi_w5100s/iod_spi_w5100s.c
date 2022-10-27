#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/clocks.h"
#include "wizchip_conf.h"
#include "w5x00_spi.h"
#include "socket.h"

#define PLL_SYS_KHZ             (133 * 1000) // Clock
#define ETHERNET_BUF_MAX_SIZE   (1024 * 2) // Buffer
#define SOCKET_LOOPBACK         (0) // Socket
#define PORT_LOOPBACK           (5000) // Port

#define TX_MESSAGE              "send message.\n"

static const wiz_NetInfo g_net_info =
    {
        .mac = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56}, // MAC address
        .ip = {192, 168, 0, 150},                     // IP address
        .sn = {255, 255, 255, 0},                    // Subnet Mask
        .gw = {192, 168, 0, 1},                     // Gateway
        .dns = {8, 8, 8, 8},                         // DNS server
        .dhcp = NETINFO_STATIC                       // DHCP enable/disable
};

static uint8_t u8s_sn;
static uint8_t au8s_rx_message[64];
static uint8_t au8s_tx_message[64];
int32_t s32s_rx_size;
int32_t s32s_tx_size;
bool bls_rx_flag;
bool bls_tx_flag;

// iod_spi_w5100s
extern void iod_spi_w5100s_init();
extern void iod_spi_w5100s_deinit();
extern void iod_spi_w5100s_reinit();
extern void iod_spi_w5100s_main_1ms();
extern void iod_spi_w5100s_main_in();
extern void iod_spi_w5100s_main_out();

static void set_clock_khz(void);
static void iod_spi_w5100s_state_in();
static void iod_spi_w5100s_state_out();

// 外部公開関数
void main() {
    uint8_t u8a_count = 0;

//    stdio_init_all();
    iod_spi_w5100s_init();

    while (true) {
        iod_spi_w5100s_main_in();
        {
            if (bls_rx_flag) {
                au8s_rx_message[s32s_rx_size] = 0x00;
                printf("rx_message = %s", au8s_rx_message);
                bls_tx_flag = true;
                s32s_tx_size = sizeof(TX_MESSAGE);
                memcpy(au8s_tx_message, TX_MESSAGE, s32s_tx_size);
            } else {
                printf("pass. (%d)\n", u8a_count);
            }
        }
        iod_spi_w5100s_main_out();
        sleep_ms(1000);
        u8a_count++;
    }
}

void iod_spi_w5100s_init() {
    memset(au8s_rx_message, 0, sizeof(au8s_rx_message));
    memset(au8s_tx_message, 0, sizeof(au8s_tx_message));
    u8s_sn = 0;
    s32s_rx_size = 0;
    s32s_tx_size = 0;
    bls_rx_flag = false;
    bls_tx_flag = false;

    // Initialize
    set_clock_khz();
    wizchip_spi_initialize();
    wizchip_cris_initialize();
    wizchip_reset();
    wizchip_initialize();
    wizchip_check();
    network_initialize(g_net_info);

    //【注意】stdio_init_all()はクロック設定後に実行する
    stdio_init_all();
    // Get network information
    print_network_information(g_net_info);
    printf("start iod_spi_w5100s\n");
}

void iod_spi_w5100s_deinit() {
}

void iod_spi_w5100s_reinit() {
}

void iod_spi_w5100s_main_1ms() {
}

void iod_spi_w5100s_main_in() {
    iod_spi_w5100s_state_in();
}

void iod_spi_w5100s_main_out() {
    iod_spi_w5100s_state_out();
}

// 内部関数
static void set_clock_khz(void)
{
    // set a system clock frequency in khz
    set_sys_clock_khz(PLL_SYS_KHZ, true);

    // configure the specified clock
    clock_configure(
        clk_peri,
        0,                                                // No glitchless mux
        CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, // System PLL on AUX mux
        PLL_SYS_KHZ * 1000,                               // Input frequency
        PLL_SYS_KHZ * 1000                                // Output (must be same as no divider)
    );
}

static void iod_spi_w5100s_state_in() {
    bls_rx_flag = false;

    switch(getSn_SR(u8s_sn)) { // SOCKET n Status Register
    case SOCK_CLOSED:
        socket(u8s_sn, Sn_MR_TCP, PORT_LOOPBACK, 0x00);
        break;
    case SOCK_INIT :
        listen(u8s_sn);
        break;
    case SOCK_LISTEN:
        break;
    case SOCK_ESTABLISHED :
        if (getSn_RX_RSR(u8s_sn) > 0) { // SOCKET n RX Received Size Register
            s32s_rx_size = recv(u8s_sn, au8s_rx_message, sizeof(au8s_rx_message));
            if (s32s_rx_size > 0) {
                bls_rx_flag = true;
            }
        }
        break;
    case SOCK_CLOSE_WAIT :
        disconnect(u8s_sn);
        break;
    default:
        break;
    }
}

static void iod_spi_w5100s_state_out() {
    switch(getSn_SR(u8s_sn)) { // SOCKET n Status Register
    case SOCK_ESTABLISHED :
        if (bls_tx_flag) {
            send(u8s_sn, au8s_tx_message, s32s_tx_size);
            bls_tx_flag = false;
        }
        break;
    default:
        break;
    }
}
