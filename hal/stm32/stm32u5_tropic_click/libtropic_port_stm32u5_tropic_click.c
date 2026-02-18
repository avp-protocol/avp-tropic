/**
 * @file libtropic_port_stm32u5_tropic_click.c
 * @brief Port implementation for STM32U5 with MikroE Secure Tropic Click.
 *
 * This implementation targets STM32U5 series MCUs with the MikroE Secure Tropic
 * Click board for AVP Hardware conformance.
 *
 * Key features:
 * - Hardware RNG (TRNG) for cryptographic random numbers
 * - TrustZone support for secure memory isolation
 * - Optimized SPI for TROPIC01 communication
 *
 * @copyright Copyright (c) 2026 AVP Protocol Contributors
 * @copyright Original libtropic code Copyright (c) 2020-2026 Tropic Square s.r.o.
 * @license BSD-3-Clause-Clear (see LICENSE.md)
 */

#include "libtropic_port_stm32u5_tropic_click.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "libtropic_common.h"
#include "libtropic_logging.h"
#include "libtropic_macros.h"
#include "libtropic_port.h"

#define LT_STM32U5_GPIO_CHECK_ATTEMPTS 10
#define LT_STM32U5_RESET_PULSE_MS      10
#define LT_STM32U5_RESET_DELAY_MS      50

/*=============================================================================
 * Random Number Generation
 *============================================================================*/

lt_ret_t lt_port_random_bytes(lt_l2_state_t *s2, void *buff, size_t count)
{
    lt_dev_stm32u5_tropic_click_t *device = (lt_dev_stm32u5_tropic_click_t *)(s2->device);

    if (device->rng_handle == NULL) {
        LT_LOG_ERROR("RNG handle not configured");
        return LT_FAIL;
    }

    uint8_t *buff_ptr = (uint8_t *)buff;
    size_t bytes_left = count;
    uint32_t random_data;
    HAL_StatusTypeDef ret;

    while (bytes_left > 0) {
        ret = HAL_RNG_GenerateRandomNumber(device->rng_handle, &random_data);
        if (ret != HAL_OK) {
            LT_LOG_ERROR("HAL_RNG_GenerateRandomNumber failed, ret=%d", ret);
            return LT_FAIL;
        }

        size_t copy_size = (bytes_left < sizeof(random_data)) ? bytes_left : sizeof(random_data);
        memcpy(buff_ptr, &random_data, copy_size);
        bytes_left -= copy_size;
        buff_ptr += copy_size;
    }

    /* Clear random data from stack */
    random_data = 0;

    return LT_OK;
}

/*=============================================================================
 * SPI Chip Select Control
 *============================================================================*/

lt_ret_t lt_port_spi_csn_low(lt_l2_state_t *s2)
{
    lt_dev_stm32u5_tropic_click_t *device = (lt_dev_stm32u5_tropic_click_t *)(s2->device);

    HAL_GPIO_WritePin(device->spi_cs_gpio_port, device->spi_cs_gpio_pin, GPIO_PIN_RESET);

    /* Verify CS went low */
    for (uint8_t i = 0; i < LT_STM32U5_GPIO_CHECK_ATTEMPTS; i++) {
        if (HAL_GPIO_ReadPin(device->spi_cs_gpio_port, device->spi_cs_gpio_pin) == GPIO_PIN_RESET) {
            return LT_OK;
        }
    }

    LT_LOG_ERROR("Failed to set CS low");
    return LT_L1_SPI_ERROR;
}

lt_ret_t lt_port_spi_csn_high(lt_l2_state_t *s2)
{
    lt_dev_stm32u5_tropic_click_t *device = (lt_dev_stm32u5_tropic_click_t *)(s2->device);

    HAL_GPIO_WritePin(device->spi_cs_gpio_port, device->spi_cs_gpio_pin, GPIO_PIN_SET);

    /* Verify CS went high */
    for (uint8_t i = 0; i < LT_STM32U5_GPIO_CHECK_ATTEMPTS; i++) {
        if (HAL_GPIO_ReadPin(device->spi_cs_gpio_port, device->spi_cs_gpio_pin) == GPIO_PIN_SET) {
            return LT_OK;
        }
    }

    LT_LOG_ERROR("Failed to set CS high");
    return LT_L1_SPI_ERROR;
}

/*=============================================================================
 * Port Initialization
 *============================================================================*/

lt_ret_t lt_port_init(lt_l2_state_t *s2)
{
    lt_dev_stm32u5_tropic_click_t *device = (lt_dev_stm32u5_tropic_click_t *)(s2->device);
    HAL_StatusTypeDef ret;

    /* Configure SPI */
    device->spi_handle.Instance = device->spi_instance;

    if (device->baudrate_prescaler == 0) {
        device->spi_handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    } else {
        device->spi_handle.Init.BaudRatePrescaler = device->baudrate_prescaler;
    }

    device->spi_handle.Init.Direction = SPI_DIRECTION_2LINES;
    device->spi_handle.Init.CLKPhase = SPI_PHASE_1EDGE;
    device->spi_handle.Init.CLKPolarity = SPI_POLARITY_LOW;
    device->spi_handle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    device->spi_handle.Init.DataSize = SPI_DATASIZE_8BIT;
    device->spi_handle.Init.FirstBit = SPI_FIRSTBIT_MSB;
    device->spi_handle.Init.NSS = SPI_NSS_SOFT;
    device->spi_handle.Init.TIMode = SPI_TIMODE_DISABLE;
    device->spi_handle.Init.Mode = SPI_MODE_MASTER;
    device->spi_handle.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    device->spi_handle.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;

    ret = HAL_SPI_Init(&device->spi_handle);
    if (ret != HAL_OK) {
        LT_LOG_ERROR("HAL_SPI_Init failed, ret=%d", ret);
        return LT_L1_SPI_ERROR;
    }

    /* Configure CS GPIO */
    GPIO_InitTypeDef gpio_init = {0};
    HAL_GPIO_WritePin(device->spi_cs_gpio_port, device->spi_cs_gpio_pin, GPIO_PIN_SET);
    gpio_init.Pin = device->spi_cs_gpio_pin;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(device->spi_cs_gpio_port, &gpio_init);

#if LT_USE_INT_PIN
    /* Configure INT GPIO */
    gpio_init.Pin = device->int_gpio_pin;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(device->int_gpio_port, &gpio_init);
#endif

    /* Configure RST GPIO if used */
    if (device->rst_gpio_port != NULL && device->rst_gpio_pin != 0) {
        HAL_GPIO_WritePin(device->rst_gpio_port, device->rst_gpio_pin, GPIO_PIN_SET);
        gpio_init.Pin = device->rst_gpio_pin;
        gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
        gpio_init.Pull = GPIO_PULLUP;
        gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(device->rst_gpio_port, &gpio_init);
    }

    LT_LOG_DEBUG("STM32U5 Tropic Click port initialized");
    return LT_OK;
}

lt_ret_t lt_port_deinit(lt_l2_state_t *s2)
{
    lt_dev_stm32u5_tropic_click_t *device = (lt_dev_stm32u5_tropic_click_t *)(s2->device);
    HAL_StatusTypeDef ret;

    ret = HAL_SPI_DeInit(&device->spi_handle);
    if (ret != HAL_OK) {
        LT_LOG_ERROR("HAL_SPI_DeInit failed, ret=%d", ret);
        return LT_L1_SPI_ERROR;
    }

    return LT_OK;
}

/*=============================================================================
 * SPI Transfer
 *============================================================================*/

lt_ret_t lt_port_spi_transfer(lt_l2_state_t *s2, uint8_t offset, uint16_t tx_data_length, uint32_t timeout_ms)
{
    lt_dev_stm32u5_tropic_click_t *device = (lt_dev_stm32u5_tropic_click_t *)(s2->device);

    if (offset + tx_data_length > TR01_L1_LEN_MAX) {
        LT_LOG_ERROR("Invalid data length: offset=%u, len=%u", offset, tx_data_length);
        return LT_L1_DATA_LEN_ERROR;
    }

    HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive(
        &device->spi_handle,
        s2->buff + offset,
        s2->buff + offset,
        tx_data_length,
        timeout_ms
    );

    if (ret != HAL_OK) {
        LT_LOG_ERROR("HAL_SPI_TransmitReceive failed, ret=%d", ret);
        return LT_L1_SPI_ERROR;
    }

    return LT_OK;
}

/*=============================================================================
 * Delay Functions
 *============================================================================*/

lt_ret_t lt_port_delay(lt_l2_state_t *s2, uint32_t ms)
{
    LT_UNUSED(s2);
    HAL_Delay(ms);
    return LT_OK;
}

#if LT_USE_INT_PIN
lt_ret_t lt_port_delay_on_int(lt_l2_state_t *s2, uint32_t ms)
{
    lt_dev_stm32u5_tropic_click_t *device = (lt_dev_stm32u5_tropic_click_t *)(s2->device);
    uint32_t start_tick = HAL_GetTick();

    while (HAL_GPIO_ReadPin(device->int_gpio_port, device->int_gpio_pin) == GPIO_PIN_RESET) {
        if ((HAL_GetTick() - start_tick) > ms) {
            return LT_L1_INT_TIMEOUT;
        }
    }

    return LT_OK;
}
#endif

/*=============================================================================
 * Hardware Reset (Optional)
 *============================================================================*/

lt_ret_t lt_port_hw_reset(lt_l2_state_t *s2)
{
    lt_dev_stm32u5_tropic_click_t *device = (lt_dev_stm32u5_tropic_click_t *)(s2->device);

    if (device->rst_gpio_port == NULL || device->rst_gpio_pin == 0) {
        LT_LOG_WARN("Hardware reset not configured");
        return LT_FAIL;
    }

    /* Assert reset (active low) */
    HAL_GPIO_WritePin(device->rst_gpio_port, device->rst_gpio_pin, GPIO_PIN_RESET);
    HAL_Delay(LT_STM32U5_RESET_PULSE_MS);

    /* Release reset */
    HAL_GPIO_WritePin(device->rst_gpio_port, device->rst_gpio_pin, GPIO_PIN_SET);
    HAL_Delay(LT_STM32U5_RESET_DELAY_MS);

    LT_LOG_DEBUG("Hardware reset complete");
    return LT_OK;
}

/*=============================================================================
 * Logging
 *============================================================================*/

int lt_port_log(const char *format, ...)
{
    va_list args;
    int ret;

    va_start(args, format);
    ret = vprintf(format, args);
    fflush(stdout);
    va_end(args);

    return ret;
}
