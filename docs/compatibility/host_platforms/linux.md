# Linux
Libtropic support on Linux is implemented with:

- [SPI and GPIO Linux Userspace API](#spi-and-gpio-linux-userspace-api)
- [SPI and GPIO Linux Userspace API with native CS](#spi-and-gpio-linux-userspace-api-with-native-cs)

HALs for these ports are available in the `libtropic/hal/linux/` directory.

See our [Linux SPI Tutorials](../../tutorials/linux/spi/index.md) to quickly get started.

## SPI and GPIO Linux Userspace API
This port uses the [SPI](https://docs.kernel.org/spi/spidev.html) and [GPIO](https://docs.kernel.org/userspace-api/gpio/chardev.html) Linux Userspace API. It was tested on:

- [Raspberry Pi 4](https://www.raspberrypi.com/products/raspberry-pi-4-model-b/),
- [Raspberry Pi 5](https://www.raspberrypi.com/products/raspberry-pi-5/).

Examples for this port are in the `examples/linux/spi_devkit` directory.

## SPI and GPIO Linux Userspace API with native CS

!!! warning
    This port is experimental. It can be modified or removed in the next release without notice. As such, no examples are provided.

This port uses the [SPI](https://docs.kernel.org/spi/spidev.html) and [GPIO](https://docs.kernel.org/userspace-api/gpio/chardev.html) Linux Userspace API.

The main difference from the [SPI and GPIO Linux Userspace API](#spi-and-gpio-linux-userspace-api) port is that the GPIO is used only for interrupt handling, as the chip select is handled natively by the SPI driver. The main benefit is that no additional GPIO pin is required and no GPIOs are required at all if interrupts are not used. However, more data are transmitted on each transaction, as this port has no custom control over chip select, meaning it needs to transfer whole buffer on each transaction.

This port was tested on:

- [Raspberry Pi 4](https://www.raspberrypi.com/products/raspberry-pi-4-model-b/)