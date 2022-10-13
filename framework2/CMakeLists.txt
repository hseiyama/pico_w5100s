add_executable(framework2
        sys_main.c
        apl_main.c
        apl_request.c
        apl_blink.c
        apl_pwm.c
        apl_core1.c
        iod_main.c
        iod_port.c
        iod_uart.c
        iod_adc.c
        iod_pwm.c
        iod_mcore.c
        iod_i2c.c
        iod_i2c_eep.c
        iod_i2c_gyro.c
        iod_i2c_6axis.c
        iod_spi.c
        iod_spi_eep.c
        iod_spi_gyro.c
        iod_flash.c
        iod_wdog.c
        )

# pull in common dependencies
target_link_libraries(framework2
        pico_stdlib
        hardware_adc
        hardware_pwm
        pico_multicore
        hardware_i2c
        hardware_spi
        hardware_flash
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(framework2)
