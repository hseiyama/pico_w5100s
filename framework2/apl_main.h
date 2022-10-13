enum request_event {
    EVENT_NONE = 0,
    EVENT_BLINK_500MS,
    EVENT_BLINK_1000MS,
    EVENT_BLINK_2000MS,
    EVENT_PWM0_LEVEL0,
    EVENT_PWM0_LEVEL1,
    EVENT_PWM0_LEVEL2,
    EVENT_PWM0_LEVEL3,
    EVENT_PWM0_LEVEL4,
    EVENT_PWM0_ADC_VALUE,
    EVENT_PWM1_LEVEL0,
    EVENT_PWM1_LEVEL1,
    EVENT_PWM1_LEVEL2,
    EVENT_PWM1_LEVEL3,
    EVENT_PWM1_LEVEL4,
    EVENT_PWM1_ADC_VALUE,
    EVENT_BTN1_INTR_DISENABLE,
    EVENT_BTN1_INTR_ENABLE,
    EVENT_BTN2_INTR_DISENABLE,
    EVENT_BTN2_INTR_ENABLE,
    EVENT_SLEEP,
    EVENT_WDOG_RESET,
    EVENT_MCORE_STOP,
    EVENT_MCORE_START,
    EVENT_NUM
};

// フラッシュ領域の保存用に型定義を移動
enum blink_state {
    BLINK_500MS = 0,
    BLINK_1000MS,
    BLINK_2000MS,
    BLINK_STATE_NUM
};

enum pwm_group {
    PWM0 = 0,
    PWM1,
    PWM_GROUP_NUM
};

struct pwm_request {
    uint16_t u16_duty;
    bool bl_state;
};

// EEPROM(I2C)に保存するデータ
// 構造体のサイズは最大値（IOD_I2C_EEP_DATA_SIZE）を超えないこと
struct eep_i2c_data {
    uint32_t u32_count;
};

// EEPROM(SPI)に保存するデータ
// 構造体のサイズは最大値（IOD_SPI_EEP_DATA_SIZE）を超えないこと
struct eep_spi_data {
    uint32_t u32_count;
};

// フラッシュ領域のデータ
// 構造体のサイズは最大値（IOD_FLASH_DATA_SIZE）を超えないこと
struct flash_data {
    uint32_t u32_count;
    enum blink_state u8_blink_state;
    struct pwm_request ast_pwm_request[PWM_GROUP_NUM];
};

// フラッシュ領域の情報
struct flash_info {
    bool bl_status;
    struct flash_data st_data;
};

extern uint8_t au8g_tx_message[IOD_UART_BUFF_SIZE];
extern struct eep_spi_data stg_eep_spi_data;
extern struct flash_info stg_flash_info;

// apl_main
extern void apl_init();
extern void apl_deinit();
extern void apl_reinit();
extern void apl_main();
extern void apl_intr_btn1_down();
extern void apl_intr_btn2_down();
extern void apl_intr_gyro_intr1();
extern void apl_intr_gyro_intr2();
// apl_request
extern void apl_request_init();
extern void apl_request_deinit();
extern void apl_request_reinit();
extern void apl_request_main();
extern void apl_read_request_event(enum request_event *);
// apl_blink
extern void apl_blink_init();
extern void apl_blink_deinit();
extern void apl_blink_reinit();
extern void apl_blink_main();
// apl_pwm
extern void apl_pwm_init();
extern void apl_pwm_deinit();
extern void apl_pwm_reinit();
extern void apl_pwm_main();
// apl_core1
extern void apl_core1_task_init();
extern void apl_core1_task_main();
