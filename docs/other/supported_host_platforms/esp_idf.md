# ESP-IDF
To support the widest range of Espressif SoCs possible, we provide a HAL for the [Espressif IoT Development Framework](https://github.com/espressif/esp-idf) (ESP-IDF). The HAL is available in the `libtropic/hal/esp-idf/` directory.

!!! info "ESP-IDF Version"
    The ESP-IDF version tested with Libtropic is 5.5.1.

Currently tested ESP32 boards are:

- [ESP32-DevKitC-V4](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32/esp32-devkitc/user_guide.html)
- [ESP32-S3-DevKitC-1](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/index.html)

## Initialization
Because the HAL uses GPIO interrupts, the [`gpio_install_isr_service`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gpio.html#_CPPv424gpio_install_isr_servicei) function with parameter `0` has to be called in your code before calling `lt_init`. This function has to be called **exactly once** in your application. See the ESP-IDF examples in the `examples/` directory for inspiration.