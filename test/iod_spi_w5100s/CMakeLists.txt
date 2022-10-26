add_executable(iod_spi_w5100s
        iod_spi_w5100s.c
        )

# pull in common dependencies
target_link_libraries(iod_spi_w5100s
        pico_stdlib
        hardware_spi
        hardware_dma
        ETHERNET_FILES
        IOLIBRARY_FILES
        )

pico_enable_stdio_usb(iod_spi_w5100s 0)
pico_enable_stdio_uart(iod_spi_w5100s 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(iod_spi_w5100s)
