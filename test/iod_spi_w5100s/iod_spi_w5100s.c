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
#define BUFFER_SIZE     (64) // Buffer
#define TCP_SN          (0) // Socket(0-3)
#define TCP_PORT        (5000) // Port
#define UDP_SN          (1) // Socket(0-3)
#define UDP_PORT        (5000) // Port

struct ether_data {
    uint8_t au8_buffer[BUFFER_SIZE];
    int32_t s32_size;
    bool bl_flag;
};

static const wiz_NetInfo csts_net_info =
    {
        .mac = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56}, // MAC address
        .ip = {192, 168, 0, 150},                     // IP address
        .sn = {255, 255, 255, 0},                    // Subnet Mask
        .gw = {192, 168, 0, 1},                     // Gateway
        .dns = {8, 8, 8, 8},                         // DNS server
        .dhcp = NETINFO_STATIC                       // DHCP enable/disable
};

static struct ether_data sts_tcp_rx_data;
static struct ether_data sts_tcp_tx_data;
static struct ether_data sts_udp_rx_data;
static struct ether_data sts_udp_tx_data;
static uint8_t au8s_udp_ip[4];
static uint16_t u16s_udp_port;

// iod_spi_w5100s
extern void iod_spi_w5100s_init();
extern void iod_spi_w5100s_deinit();
extern void iod_spi_w5100s_reinit();
extern void iod_spi_w5100s_main_1ms();
extern void iod_spi_w5100s_main_in();
extern void iod_spi_w5100s_main_out();
extern uint32_t iod_spi_w5100s_tcp_recv(uint8_t *, uint32_t);
extern uint32_t iod_spi_w5100s_tcp_send(uint8_t *, uint32_t);
extern uint32_t iod_spi_w5100s_udp_recv(uint8_t *, uint32_t);
extern uint32_t iod_spi_w5100s_udp_send(uint8_t *, uint32_t);

static void iod_spi_w5100s_set_clock();
static void iod_spi_w5100s_tcp_in(uint8_t);
static void iod_spi_w5100s_tcp_out(uint8_t);
static void iod_spi_w5100s_udp_in(uint8_t);
static void iod_spi_w5100s_udp_out(uint8_t);
static uint32_t iod_spi_w5100s_recv(uint8_t *, uint32_t, struct ether_data *);
static uint32_t iod_spi_w5100s_send(uint8_t *, uint32_t, struct ether_data *);
static void iod_spi_w5100s_intr_init();
static void iod_spi_w5100s_intr_gpio();

// 外部公開関数
void main() {
    uint8_t au8a_buffer[BUFFER_SIZE];
    uint16_t u16a_size;
    uint8_t u8a_count = 0;

//    stdio_init_all();
    iod_spi_w5100s_init();

    while (true) {
        iod_spi_w5100s_main_in();
        {
            u16a_size = iod_spi_w5100s_tcp_recv(au8a_buffer, sizeof(au8a_buffer));
            if (u16a_size > 0) {
                iod_spi_w5100s_tcp_send(au8a_buffer, u16a_size);
            }
            u16a_size = iod_spi_w5100s_udp_recv(au8a_buffer, sizeof(au8a_buffer));
            if (u16a_size > 0) {
                iod_spi_w5100s_udp_send(au8a_buffer, u16a_size);
            }
            printf("passed. (%d)\n", u8a_count);
        }
        iod_spi_w5100s_main_out();
        sleep_ms(1000);
        u8a_count++;
    }
}

void iod_spi_w5100s_init() {
    memset(&sts_tcp_rx_data, 0, sizeof(sts_tcp_rx_data));
    memset(&sts_tcp_tx_data, 0, sizeof(sts_tcp_tx_data));
    memset(&sts_udp_rx_data, 0, sizeof(sts_udp_rx_data));
    memset(&sts_udp_tx_data, 0, sizeof(sts_udp_tx_data));
    memset(&au8s_udp_ip, 0, sizeof(au8s_udp_ip));
    u16s_udp_port = 0;

    // Initialize
    iod_spi_w5100s_set_clock();
    wizchip_spi_initialize();
    wizchip_cris_initialize();
    wizchip_reset();
    wizchip_initialize();
    wizchip_check();
    network_initialize(csts_net_info);
    // 割り込み設定
    //【注意】wizchip_gpio_interrupt_initialize() の仕様により、
    //        IMR (Interrupt Mask Register)の設定は後勝ちとなる
//    wizchip_gpio_interrupt_initialize(TCP_SN, iod_spi_w5100s_intr_gpio);
//    wizchip_gpio_interrupt_initialize(UDP_SN, iod_spi_w5100s_intr_gpio);
    iod_spi_w5100s_intr_init();

    //【注意】stdio_init_all()はクロック設定後に実行する
    stdio_init_all();
    // Get network information
    print_network_information(csts_net_info);
    printf("start iod_spi_w5100s\n");
}

void iod_spi_w5100s_deinit() {
}

void iod_spi_w5100s_reinit() {
}

void iod_spi_w5100s_main_1ms() {
}

void iod_spi_w5100s_main_in() {
    iod_spi_w5100s_tcp_in(TCP_SN);
    iod_spi_w5100s_udp_in(UDP_SN);
}

void iod_spi_w5100s_main_out() {
    iod_spi_w5100s_tcp_out(TCP_SN);
    iod_spi_w5100s_udp_out(UDP_SN);
}

uint32_t iod_spi_w5100s_tcp_recv(uint8_t *pu8a_buffer, uint32_t u32a_size) {
    return iod_spi_w5100s_recv(pu8a_buffer, u32a_size, &sts_tcp_rx_data);
}

uint32_t iod_spi_w5100s_tcp_send(uint8_t *pu8a_buffer, uint32_t u32a_size) {
    return iod_spi_w5100s_send(pu8a_buffer, u32a_size, &sts_tcp_tx_data);
}

uint32_t iod_spi_w5100s_udp_recv(uint8_t *pu8a_buffer, uint32_t u32a_size) {
    return iod_spi_w5100s_recv(pu8a_buffer, u32a_size, &sts_udp_rx_data);
}

uint32_t iod_spi_w5100s_udp_send(uint8_t *pu8a_buffer, uint32_t u32a_size) {
    return iod_spi_w5100s_send(pu8a_buffer, u32a_size, &sts_udp_tx_data);
}

// 内部関数
static void iod_spi_w5100s_set_clock()
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

static void iod_spi_w5100s_tcp_in(uint8_t u8a_sn) {
    sts_tcp_rx_data.bl_flag = false;

    switch(getSn_SR(u8a_sn)) { // SOCKET n Status Register
    case SOCK_CLOSED:
        socket(u8a_sn, Sn_MR_TCP, TCP_PORT, 0x00);
        break;
    case SOCK_INIT :
        listen(u8a_sn);
        break;
    case SOCK_LISTEN:
        break;
    case SOCK_ESTABLISHED :
        if (getSn_RX_RSR(u8a_sn) > 0) { // SOCKET n RX Received Size Register
            sts_tcp_rx_data.s32_size = recv(u8a_sn, sts_tcp_rx_data.au8_buffer, sizeof(sts_tcp_rx_data.au8_buffer));
            if (sts_tcp_rx_data.s32_size > 0) {
                sts_tcp_rx_data.bl_flag = true;
            }
        }
        break;
    case SOCK_CLOSE_WAIT :
        disconnect(u8a_sn);
        break;
    default:
        break;
    }
}

static void iod_spi_w5100s_tcp_out(uint8_t u8a_sn) {
    switch(getSn_SR(u8a_sn)) { // SOCKET n Status Register
    case SOCK_ESTABLISHED :
        if (sts_tcp_tx_data.bl_flag) {
            send(u8a_sn, sts_tcp_tx_data.au8_buffer, sts_tcp_tx_data.s32_size);
            sts_tcp_tx_data.bl_flag = false;
        }
        break;
    default:
        break;
    }
}

static void iod_spi_w5100s_udp_in(uint8_t u8a_sn) {
    sts_udp_rx_data.bl_flag = false;

    switch(getSn_SR(u8a_sn)) { // SOCKET n Status Register
    case SOCK_CLOSED:
        socket(u8a_sn, Sn_MR_UDP, UDP_PORT, 0x00);
        break;
    case SOCK_UDP :
        if (getSn_RX_RSR(u8a_sn) > 0) { // SOCKET n RX Received Size Register
            sts_udp_rx_data.s32_size = recvfrom(u8a_sn, sts_udp_rx_data.au8_buffer, sizeof(sts_udp_rx_data.au8_buffer), au8s_udp_ip, &u16s_udp_port);
            if (sts_udp_rx_data.s32_size > 0) {
                sts_udp_rx_data.bl_flag = true;
            }
        }
        break;
    default:
        break;
    }
}

static void iod_spi_w5100s_udp_out(uint8_t u8a_sn) {
    switch(getSn_SR(u8a_sn)) { // SOCKET n Status Register
    case SOCK_UDP :
        if (sts_udp_tx_data.bl_flag) {
            sendto(u8a_sn, sts_udp_tx_data.au8_buffer, sts_udp_tx_data.s32_size, au8s_udp_ip, u16s_udp_port);
            sts_udp_tx_data.bl_flag = false;
        }
        break;
    default:
        break;
    }
}

static uint32_t iod_spi_w5100s_recv(uint8_t *pu8a_buffer, uint32_t u32a_size, struct ether_data *psta_data) {
    uint32_t u32a_rcode = 0;
    if (psta_data->bl_flag) {
        psta_data->s32_size = MIN(psta_data->s32_size, u32a_size);
        memcpy(pu8a_buffer, psta_data->au8_buffer, psta_data->s32_size);
        u32a_rcode = psta_data->s32_size;
    }
    return u32a_rcode;
}

static uint32_t iod_spi_w5100s_send(uint8_t *pu8a_buffer, uint32_t u32a_size, struct ether_data *psta_data) {
    psta_data->s32_size = MIN(BUFFER_SIZE, u32a_size);
    memcpy(psta_data->au8_buffer, pu8a_buffer, psta_data->s32_size);
    psta_data->bl_flag = true;

    return psta_data->s32_size;
}

static void iod_spi_w5100s_intr_init() {
    uint16_t u16a_reg_val;

    //【注意】SIK_SENT を含めると送受信が正常に動作しなくなる（原因不明）
    //u16a_reg_val = SIK_ALL; ///< all interrupt
    u16a_reg_val = SIK_CONNECTED | SIK_DISCONNECTED | SIK_RECEIVED | SIK_TIMEOUT; // except SendOK
    ctlsocket(TCP_SN, CS_SET_INTMASK, (void *)&u16a_reg_val);
    ctlsocket(UDP_SN, CS_SET_INTMASK, (void *)&u16a_reg_val);
    u16a_reg_val = (1 << TCP_SN) | (1 << UDP_SN);
    ctlwizchip(CW_SET_INTRMASK, (void *)&u16a_reg_val);

    gpio_set_irq_enabled_with_callback(PIN_INT, GPIO_IRQ_EDGE_FALL, true, iod_spi_w5100s_intr_gpio);
}

static void iod_spi_w5100s_intr_gpio() {
    if(getSn_IR(TCP_SN) & Sn_IR_CON) {
        setSn_IR(TCP_SN,Sn_IR_CON);
        printf("CONNECTED (tcp) Interrupt.\n");
    } else if (getSn_IR(TCP_SN) & Sn_IR_RECV) {
        setSn_IR(TCP_SN,Sn_IR_RECV);
        printf("RECEIVED (tcp) Interrupt.\n");
//    } else if (getSn_IR(TCP_SN) & Sn_IR_SENDOK) {
//        setSn_IR(TCP_SN,Sn_IR_SENDOK);
//        printf("SEND OK (tcp) Interrupt.\n");
    } else if (getSn_IR(TCP_SN) & Sn_IR_DISCON) {
        setSn_IR(TCP_SN,Sn_IR_DISCON);
        printf("DISCONNECTED (tcp) Interrupt.\n");
    } else if (getSn_IR(UDP_SN) & Sn_IR_RECV) {
        setSn_IR(UDP_SN,Sn_IR_RECV);
        printf("RECEIVED (udp) Interrupt.\n");
//    } else if (getSn_IR(UDP_SN) & Sn_IR_SENDOK) {
//        setSn_IR(UDP_SN,Sn_IR_SENDOK);
//        printf("SEND OK (udp) Interrupt.\n");
    } else {
        printf("Other Interrupt.\n");
    }
}
