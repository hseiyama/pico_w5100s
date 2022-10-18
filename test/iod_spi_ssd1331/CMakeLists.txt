add_executable(iod_spi_ssd1331
        iod_spi_ssd1331.c
        )

# pull in common dependencies
target_link_libraries(iod_spi_ssd1331
        pico_stdlib
        hardware_spi
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(iod_spi_ssd1331)
