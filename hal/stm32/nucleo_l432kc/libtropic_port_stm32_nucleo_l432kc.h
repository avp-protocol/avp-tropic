#ifndef LIBTROPIC_PORT_STM32_NUCLEO_L432KC_H
#define LIBTROPIC_PORT_STM32_NUCLEO_L432KC_H

/**
 * @file libtropic_port_stm32_nucleo_l432kc.h
 * @copyright Copyright (c) 2020-2025 Tropic Square s.r.o.
 * @brief Port for STM32 L432KC using native SPI HAL (and GPIO HAL for chip select).
 *
 * @license For the license see LICENSE.md in the root directory of this source tree.
 */

#include "libtropic_port.h"
#include "stm32l4xx_hal.h"

/**
 * @brief Device structure for STM32 L432KC port.
 *
 * @note Public members are meant to be configured by the developer before passing the handle to
 *       libtropic.
 */
typedef struct lt_dev_stm32_nucleo_l432kc_t {
    /** @brief @public Instance of STM SPI interface. Use STM32 macro (SPIX, e.g. SPI1). */
    SPI_TypeDef *spi_instance;

    /**
     * @brief @public Baudrate prescaler value, used to set SPI speed. Use STM32 macro (e.g. SPI_BAUDRATEPRESCALER_32).
     *
     * @note If set to zero, it will default to SPI_BAUDRATEPRESCALER_32.
     */
    uint16_t baudrate_prescaler;

    /** @brief @public GPIO pin used for chip select. Use STM32 macro (GPIO_PIN_XX). */
    uint16_t spi_cs_gpio_pin;
    /** @brief @public GPIO bank of the pin used for chip select. Use STM32 macro (GPIOX). */
    GPIO_TypeDef *spi_cs_gpio_bank;

    /** @brief @public Random number generator handle. */
    RNG_HandleTypeDef *rng_handle;

    /** @brief @private SPI handle. */
    SPI_HandleTypeDef spi_handle;
} lt_dev_stm32_nucleo_l432kc_t;

#endif  // LIBTROPIC_PORT_STM32_NUCLEO_L432KC_H