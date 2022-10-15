add_executable(iod_i2c_ssd1306
        iod_i2c_ssd1306.c
        )

# pull in common dependencies
target_link_libraries(iod_i2c_ssd1306
        pico_stdlib
        hardware_i2c
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(iod_i2c_ssd1306)
