#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/clocks.h"
#include "wizchip_conf.h"
#include "w5x00_spi.h"
#include "w5x00_gpio_irq.h"
#include "socket.h"

#define PLL_SYS_KHZ     (133 * 1000) // Clock
#define TCP_BUF_SIZE    (64) // Buffer
#define TCP_SN          (1) // Socket(0-3)
#define TCP_PORT        (5000) // Port

struct ether_data {
    uint8_t au8_buffer[TCP_BUF_SIZE];
    int32_t s32_size;
    bool bl_flag;
};

static const wiz_NetInfo g_net_info =
    {
        .mac = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56}, // MAC address
        .ip = {192, 168, 0, 150},                     // IP address
        .sn = {255, 255, 255, 0},                    // Subnet Mask
        .gw = {192, 168, 0, 1},                     // Gateway
        .dns = {8, 8, 8, 8},                         // DNS server
        .dhcp = NETINFO_STATIC                       // DHCP enable/disable
};

static uint8_t u8s_tcp_sn;
static struct ether_data sts_tcp_rx_data;
static struct ether_data sts_tcp_tx_data;

// iod_spi_w5100s
extern void iod_spi_w5100s_init();
extern void iod_spi_w5100s_deinit();
extern void iod_spi_w5100s_reinit();
extern void iod_spi_w5100s_main_1ms();
extern void iod_spi_w5100s_main_in();
extern void iod_spi_w5100s_main_out();
extern uint32_t iod_spi_w5100s_tcp_recv(uint8_t *, uint32_t);
extern uint32_t iod_spi_w5100s_tcp_send(uint8_t *, uint32_t);

static void set_clock_khz();
static void iod_spi_w5100s_tcp_in();
static void iod_spi_w5100s_tcp_out();
static void iod_spi_w5100s_intr_gpio();

// 外部公開関数
void main() {
    uint8_t au8a_tcp_buffer[TCP_BUF_SIZE];
    uint16_t u16a_tcp_size;
    uint8_t u8a_count = 0;

//    stdio_init_all();
    iod_spi_w5100s_init();

    while (true) {
        iod_spi_w5100s_main_in();
        {
            u16a_tcp_size = iod_spi_w5100s_tcp_recv(au8a_tcp_buffer, sizeof(au8a_tcp_buffer));
            if (u16a_tcp_size > 0) {
                iod_spi_w5100s_tcp_send(au8a_tcp_buffer, u16a_tcp_size);
            } else {
                printf("passed. (%d)\n", u8a_count);
            }
        }
        iod_spi_w5100s_main_out();
        sleep_ms(1000);
        u8a_count++;
    }
}

void iod_spi_w5100s_init() {
    memset(&sts_tcp_rx_data, 0, sizeof(sts_tcp_rx_data));
    memset(&sts_tcp_tx_data, 0, sizeof(sts_tcp_tx_data));
    u8s_tcp_sn = 0;

    // Initialize
    set_clock_khz();
    wizchip_spi_initialize();
    wizchip_cris_initialize();
    wizchip_reset();
    wizchip_initialize();
    wizchip_check();
    network_initialize(g_net_info);
    // 割り込み設定
    wizchip_gpio_interrupt_initialize(TCP_SN, iod_spi_w5100s_intr_gpio);

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
    iod_spi_w5100s_tcp_in();
}

void iod_spi_w5100s_main_out() {
    iod_spi_w5100s_tcp_out();
}

uint32_t iod_spi_w5100s_tcp_recv(uint8_t *pu8a_buffer, uint32_t u32a_size) {
    uint32_t u32a_rcode = 0;
    if (sts_tcp_rx_data.bl_flag) {
        sts_tcp_rx_data.s32_size = MIN(sts_tcp_rx_data.s32_size, u32a_size);
        memcpy(pu8a_buffer, sts_tcp_rx_data.au8_buffer, sts_tcp_rx_data.s32_size);
        u32a_rcode = sts_tcp_rx_data.s32_size;
    }
    return u32a_rcode;
}

uint32_t iod_spi_w5100s_tcp_send(uint8_t *pu8a_buffer, uint32_t u32a_size) {
    sts_tcp_tx_data.s32_size = MIN(TCP_BUF_SIZE, u32a_size);
    memcpy(sts_tcp_tx_data.au8_buffer, pu8a_buffer, sts_tcp_tx_data.s32_size);
    sts_tcp_tx_data.bl_flag = true;

    return sts_tcp_tx_data.s32_size;
}

// 内部関数
static void set_clock_khz()
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

static void iod_spi_w5100s_tcp_in() {
    sts_tcp_rx_data.bl_flag = false;

    switch(getSn_SR(u8s_tcp_sn)) { // SOCKET n Status Register
    case SOCK_CLOSED:
        u8s_tcp_sn = socket(TCP_SN, Sn_MR_TCP, TCP_PORT, 0x00);
        break;
    case SOCK_INIT :
        listen(u8s_tcp_sn);
        break;
    case SOCK_LISTEN:
        break;
    case SOCK_ESTABLISHED :
        if (getSn_RX_RSR(u8s_tcp_sn) > 0) { // SOCKET n RX Received Size Register
            sts_tcp_rx_data.s32_size = recv(u8s_tcp_sn, sts_tcp_rx_data.au8_buffer, sizeof(sts_tcp_rx_data.au8_buffer));
            if (sts_tcp_rx_data.s32_size > 0) {
                sts_tcp_rx_data.bl_flag = true;
            }
        }
        break;
    case SOCK_CLOSE_WAIT :
        disconnect(u8s_tcp_sn);
        break;
    default:
        break;
    }
}

static void iod_spi_w5100s_tcp_out() {
    switch(getSn_SR(u8s_tcp_sn)) { // SOCKET n Status Register
    case SOCK_ESTABLISHED :
        if (sts_tcp_tx_data.bl_flag) {
            send(u8s_tcp_sn, sts_tcp_tx_data.au8_buffer, sts_tcp_tx_data.s32_size);
            sts_tcp_tx_data.bl_flag = false;
        }
        break;
    default:
        break;
    }
}

static void iod_spi_w5100s_intr_gpio() {
    if(getSn_IR(u8s_tcp_sn) & Sn_IR_CON) {
        setSn_IR(u8s_tcp_sn,Sn_IR_CON);
        printf("CONNECTED Interrupt.\n");
    } else if (getSn_IR(u8s_tcp_sn) & Sn_IR_RECV) {
        setSn_IR(u8s_tcp_sn,Sn_IR_RECV);
        printf("RECEIVED Interrupt.\n");
    } else if (getSn_IR(u8s_tcp_sn) & Sn_IR_DISCON) {
        setSn_IR(u8s_tcp_sn,Sn_IR_DISCON);
        printf("DISCONNECTED Interrupt.\n");
    } else {
        printf("Other Interrupt.\n");
    }
}
