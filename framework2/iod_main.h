// 機能選択オプション
#define SELECT_I2C_GYRO     false
#define SELECT_SPI_GYRO     true
#define SELECT_I2C_6AXIS    true
/* 定義の整合チェック */
#if SELECT_I2C_GYRO && SELECT_SPI_GYRO
#error Both SELECT_I2C_GYRO and SELECT_SPI_GYRO is true.
#endif
#if SELECT_I2C_GYRO && SELECT_I2C_6AXIS
#error Both SELECT_I2C_GYRO and SELECT_I2C_6AXIS is true.
#endif

// IOD内部用
#define GPIO_GP0_UART       (0)
#define GPIO_GP1_UART       (1)
#define GPIO_GP2_PORT       (2)
#define GPIO_GP3_PORT       (3)
#define GPIO_GP4_PORT       (4)
#define GPIO_GP5_PORT       (5)
#define GPIO_GP6_PWM        (6)
#define GPIO_GP7_PWM        (7)
#define GPIO_GP8_PORT       (8)
#define GPIO_GP9_PORT       (9)
#if SELECT_I2C_GYRO
#define GPIO_GP10_I2C       (10)
#define GPIO_GP11_I2C       (11)
#define GPIO_GP12_I2C       (12)
#define GPIO_GP13_I2C       (13)
#endif
#if SELECT_I2C_6AXIS
#define GPIO_GP10_I2C       (10)
#define GPIO_GP11_I2C       (11)
#endif
#if SELECT_SPI_GYRO
#define GPIO_GP12_SPI       (12)
#define GPIO_GP13_SPI       (13)
#define GPIO_GP14_SPI       (14)
#define GPIO_GP15_SPI       (15)
#endif
#define GPIO_GP16_SPI       (16)
#define GPIO_GP17_SPI       (17)
#define GPIO_GP18_SPI       (18)
#define GPIO_GP19_SPI       (19)
#define GPIO_GP20_I2C       (20)
#define GPIO_GP21_I2C       (21)
#define GPIO_GP25_PORT      PICO_DEFAULT_LED_PIN
#define GPIO_GP26_ADC       (26)
#define GPIO_GP27_ADC       (27)
#define GPIO_GP28_ADC       (28)
/// 外部公開用
#define IOD_BTN0_VALUE_INIT     true
#define IOD_LED0_VALUE_INIT     true
#define IOD_LED1_VALUE_INIT     true
#define IOD_UART_BUFF_SIZE      (32 + 1)
#define IOD_ADC_VALUE_MAX       (0x0FFF)
#define IOD_PWM0_DUTY_MAX       (25000) // PWM3_WRAP と一致させる
#define IOD_PWM1_DUTY_MAX       (25000) // PWM3_WRAP と一致させる
#define IOD_I2C_EEP_DATA_SIZE   (64 - 4) // I2C_EEP_DATA_SIZE と一致させる
#define IOD_SPI_EEP_DATA_SIZE   (32 - 4) // SPI_EEP_DATA_SIZE と一致させる
#define IOD_FLASH_DATA_SIZE     ((1u << 8) - 4) // FLASH_DATA_SIZE と一致させる

// iod_main
extern void iod_init();
extern void iod_deinit();
extern void iod_reinit();
extern void iod_main_1ms();
extern void iod_main_in();
extern void iod_main_out();
// iod_port
extern void iod_port_init();
extern void iod_port_deinit();
extern void iod_port_reinit();
extern void iod_port_main_1ms();
extern void iod_port_main_in();
extern void iod_port_main_out();
extern void iod_read_btn0_value(bool *);
extern void iod_write_led0_value(bool);
extern void iod_write_led1_value(bool);
extern void iod_call_btn1_intr_enabled(bool);
extern void iod_call_btn2_intr_enabled(bool);
// iod_uart
extern void iod_uart_init();
extern void iod_uart_deinit();
extern void iod_uart_reinit();
extern void iod_uart_main_1ms();
extern void iod_uart_main_in();
extern void iod_uart_main_out();
extern bool iod_call_uart_receive(uint8_t *);
extern void iod_call_uart_transmit(uint8_t *);
// iod_adc
extern void iod_adc_init();
extern void iod_adc_deinit();
extern void iod_adc_reinit();
extern void iod_adc_main_1ms();
extern void iod_adc_main_in();
extern void iod_adc_main_out();
extern void iod_read_vrest_value(uint16_t *);
extern void iod_read_gyro1_value(uint16_t *);
extern void iod_read_gyro2_value(uint16_t *);
// iod_pwm
extern void iod_pwm_init();
extern void iod_pwm_deinit();
extern void iod_pwm_reinit();
extern void iod_pwm_main_1ms();
extern void iod_pwm_main_in();
extern void iod_pwm_main_out();
extern void iod_call_pwm0_set_duty(uint16_t);
extern void iod_call_pwm1_set_duty(uint16_t);
// iod_mcore
extern void iod_mcore_init();
extern void iod_mcore_deinit();
extern void iod_mcore_reinit();
extern void iod_mcore_main_1ms();
extern void iod_mcore_main_in();
extern void iod_mcore_main_out();
extern void iod_call_mcore_start();
extern void iod_call_mcore_stop();
extern bool iod_call_mcore_fifo_push(uint32_t);
extern bool iod_call_mcore_fifo_pop(uint32_t *);
extern bool iod_call_mcore_queue_add_core0(uint32_t);
extern bool iod_call_mcore_queue_remove_core0(uint32_t *);
extern bool iod_call_mcore_queue_add_core1(uint32_t);
extern bool iod_call_mcore_queue_remove_core1(uint32_t *);
extern void iod_call_mcore_lock_enter();
extern void iod_call_mcore_lock_exit();
extern void iod_call_mcore_mutex_enter();
extern void iod_call_mcore_mutex_exit();
extern void iod_call_mcore_sem_acquire();
extern bool iod_call_mcore_sem_release();
// iod_i2c
extern void iod_i2c_init();
extern void iod_i2c_deinit();
extern void iod_i2c_reinit();
extern void iod_i2c_main_1ms();
extern void iod_i2c_main_in();
extern void iod_i2c_main_out();
extern bool iod_call_i2c_eep_read(uint8_t *, uint16_t);
extern bool iod_call_i2c_eep_write(uint8_t *, uint16_t);
// iod_spi
extern void iod_spi_init();
extern void iod_spi_deinit();
extern void iod_spi_reinit();
extern void iod_spi_main_1ms();
extern void iod_spi_main_in();
extern void iod_spi_main_out();
extern bool iod_call_spi_eep_read(uint8_t *, uint16_t);
extern bool iod_call_spi_eep_write(uint8_t *, uint16_t);
// iod_i2c_gyro, iod_spi_gyro
void iod_read_gyro_x_value(int16_t *);
void iod_read_gyro_y_value(int16_t *);
void iod_read_gyro_z_value(int16_t *);
// iod_i2c_6axis
void iod_read_6axis_accel_x_value(int16_t *);
void iod_read_6axis_accel_y_value(int16_t *);
void iod_read_6axis_accel_z_value(int16_t *);
void iod_read_6axis_gyro_x_value(int16_t *);
void iod_read_6axis_gyro_y_value(int16_t *);
void iod_read_6axis_gyro_z_value(int16_t *);
// iod_flash
extern void iod_flash_init();
extern void iod_flash_deinit();
extern void iod_flash_reinit();
extern void iod_flash_main_1ms();
extern void iod_flash_main_in();
extern void iod_flash_main_out();
extern bool iod_call_flash_read(uint8_t *, uint16_t);
extern bool iod_call_flash_write(uint8_t *, uint16_t);
// iod_wdog
extern void iod_wdog_init();
extern void iod_wdog_deinit();
extern void iod_wdog_reinit();
extern void iod_wdog_main_1ms();
extern void iod_wdog_main_in();
extern void iod_wdog_main_out();
extern bool iod_call_wdog_isboot();
