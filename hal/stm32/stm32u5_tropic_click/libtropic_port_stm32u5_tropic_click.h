/**
 * @file libtropic_port_stm32u5_tropic_click.h
 * @brief Port for STM32U5 with MikroE Secure Tropic Click board.
 *
 * This port is designed for use with:
 * - STM32U5 series MCUs (STM32U575, STM32U585, etc.)
 * - MikroE Secure Tropic Click board (MIKROE-6559)
 * - Agent Vault Protocol (AVP) integration
 *
 * Pin mapping for MikroE Click interface (directly on TROPIC01):
 * - SPI MOSI: Pin 6 (directly mapped from Click)
 * - SPI MISO: Pin 5 (directly mapped from Click)
 * - SPI SCK:  Pin 4 (directly mapped from Click)
 * - SPI CS:   Pin 3 (directly mapped from Click)
 * - INT:      Pin 15 (directly mapped from Click, directly wired from TROPIC01)
 * - RST:      Optional reset control
 *
 * @copyright Copyright (c) 2026 AVP Protocol Contributors
 * @copyright Original libtropic code Copyright (c) 2020-2026 Tropic Square s.r.o.
 * @license BSD-3-Clause-Clear (see LICENSE.md)
 */

#ifndef LIBTROPIC_PORT_STM32U5_TROPIC_CLICK_H
#define LIBTROPIC_PORT_STM32U5_TROPIC_CLICK_H

#include "libtropic_port.h"
#include "stm32u5xx_hal.h"

/**
 * @brief Device structure for STM32U5 with Secure Tropic Click.
 *
 * @note The STM32U5 series includes hardware RNG and TrustZone support,
 *       making it ideal for AVP Hardware conformance implementations.
 */
typedef struct lt_dev_stm32u5_tropic_click_t {
    /*=== SPI Configuration ===*/

    /** @brief @public SPI instance (SPI1, SPI2, SPI3, etc.) */
    SPI_TypeDef *spi_instance;

    /**
     * @brief @public SPI baudrate prescaler.
     *
     * For STM32U5 at 160MHz:
     * - SPI_BAUDRATEPRESCALER_8  = 20 MHz (max for TROPIC01)
     * - SPI_BAUDRATEPRESCALER_16 = 10 MHz (recommended)
     * - SPI_BAUDRATEPRESCALER_32 = 5 MHz (safe default)
     *
     * @note If set to zero, defaults to SPI_BAUDRATEPRESCALER_32.
     */
    uint32_t baudrate_prescaler;

    /*=== GPIO Configuration (MikroE Click Interface) ===*/

    /** @brief @public Chip select GPIO pin (GPIO_PIN_XX) */
    uint16_t spi_cs_gpio_pin;
    /** @brief @public Chip select GPIO port (GPIOA, GPIOB, etc.) */
    GPIO_TypeDef *spi_cs_gpio_port;

#if LT_USE_INT_PIN
    /** @brief @public Interrupt GPIO pin from TROPIC01 */
    uint16_t int_gpio_pin;
    /** @brief @public Interrupt GPIO port */
    GPIO_TypeDef *int_gpio_port;
#endif

    /** @brief @public Optional reset GPIO pin (0 if not used) */
    uint16_t rst_gpio_pin;
    /** @brief @public Optional reset GPIO port (NULL if not used) */
    GPIO_TypeDef *rst_gpio_port;

    /*=== Hardware RNG ===*/

    /** @brief @public RNG handle (STM32U5 has hardware TRNG) */
    RNG_HandleTypeDef *rng_handle;

    /*=== Private Members (managed by HAL) ===*/

    /** @brief @private SPI handle - initialized by lt_port_init() */
    SPI_HandleTypeDef spi_handle;

} lt_dev_stm32u5_tropic_click_t;

/**
 * @brief Default MikroE Click pin configuration for NUCLEO-U575ZI-Q.
 *
 * MikroBUS socket on Arduino connector:
 * - SPI: SPI1 (PA5=SCK, PA6=MISO, PA7=MOSI)
 * - CS:  PD14
 * - INT: PF13
 */
#define LT_STM32U5_TROPIC_CLICK_NUCLEO_DEFAULTS { \
    .spi_instance = SPI1, \
    .baudrate_prescaler = SPI_BAUDRATEPRESCALER_16, \
    .spi_cs_gpio_pin = GPIO_PIN_14, \
    .spi_cs_gpio_port = GPIOD, \
    .int_gpio_pin = GPIO_PIN_13, \
    .int_gpio_port = GPIOF, \
    .rst_gpio_pin = 0, \
    .rst_gpio_port = NULL, \
    .rng_handle = NULL \
}

/**
 * @brief Hardware reset the TROPIC01 via RST pin (if configured).
 *
 * @param s2 Pointer to L2 state containing device handle.
 * @return LT_OK on success, LT_FAIL if RST pin not configured.
 */
lt_ret_t lt_port_hw_reset(lt_l2_state_t *s2);

#endif /* LIBTROPIC_PORT_STM32U5_TROPIC_CLICK_H */
