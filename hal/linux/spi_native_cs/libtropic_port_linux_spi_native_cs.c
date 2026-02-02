/**
 * @file libtropic_port_linux_spi_native_cs.c
 * @copyright Copyright (c) 2020-2025 Tropic Square s.r.o.
 * @brief Port for communication using Generic SPI UAPI with native CS handling and GPIO for interrupt handling.
 *
 * @note As this HAL controls CS using SPI driver natively,  whole buffer is transferred each time, which
 * introduces a small overhead.
 *
 * @warning This HAL is experimental. It can be modified or removed in the next release without notice.
 *
 * @license For the license see LICENSE.md in the root directory of this source tree.
 */

// SPI-related includes
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <unistd.h>

// GPIO-related includes
#if LT_USE_INT_PIN
#include <linux/gpio.h>
#include <poll.h>
#endif

// Other
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>

#include "libtropic_common.h"
#include "libtropic_logging.h"
#include "libtropic_macros.h"
#include "libtropic_port.h"
#include "libtropic_port_linux_spi_native_cs.h"

lt_ret_t lt_port_init(lt_l2_state_t *s2)
{
    lt_dev_linux_spi_native_cs_t *device = (lt_dev_linux_spi_native_cs_t *)(s2->device);
    const uint32_t request_spi_mode = SPI_MODE_0;
    lt_ret_t ret = LT_OK;

    device->frame_in_progress = 0;
    device->frame_completed = 0;

// Initialize file descriptors to -1 so lt_port_deinit() can always execute safely.
#if LT_USE_INT_PIN
    device->gpio_fd = -1;
    device->gpioreq_int.fd = -1;
#endif
    device->spi_fd = -1;

    LT_LOG_DEBUG("Initializing SPI...\n");
    LT_LOG_DEBUG("SPI speed: %d", device->spi_speed);
    LT_LOG_DEBUG("SPI device: %s", device->spi_dev);
#if LT_USE_INT_PIN
    LT_LOG_DEBUG("GPIO device: %s", device->gpio_dev);
    LT_LOG_DEBUG("GPIO interrupt pin: %d", device->gpio_int_num);
#endif

    device->spi_fd = open(device->spi_dev, O_RDWR);
    if (device->spi_fd < 0) {
        LT_LOG_ERROR("Can't open device!");
        return LT_FAIL;
    }

    // Set the SPI mode.
    if (ioctl(device->spi_fd, SPI_IOC_WR_MODE32, &request_spi_mode) < 0) {
        LT_LOG_ERROR("Can't set SPI mode!");
        ret = LT_FAIL;
        goto spi_error;
    }

    // Read what SPI mode the device actually is in.
    uint32_t read_spi_mode;
    if (ioctl(device->spi_fd, SPI_IOC_RD_MODE32, &read_spi_mode) < 0) {
        LT_LOG_ERROR("Can't get SPI mode!");
        ret = LT_FAIL;
        goto spi_error;
    }
    if (request_spi_mode != read_spi_mode) {
        LT_LOG_ERROR("Device does not support requested mode 0x%" PRIx32, request_spi_mode);
        ret = LT_FAIL;
        goto spi_error;
    }

    if (ioctl(device->spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &device->spi_speed) < 0) {
        LT_LOG_ERROR("Can't set max SPI speed.");
        ret = LT_FAIL;
        goto spi_error;
    }

#if LT_USE_INT_PIN
    device->gpio_fd = open(device->gpio_dev, O_RDWR | O_CLOEXEC);
    if (device->gpio_fd < 0) {
        LT_LOG_ERROR("Can't open GPIO device!");
        ret = LT_FAIL;
        goto spi_error;
    }

    struct gpiochip_info info;
    if (ioctl(device->gpio_fd, GPIO_GET_CHIPINFO_IOCTL, &info) < 0) {
        LT_LOG_ERROR("GPIO_GET_CHIPINFO_IOCTL error!");
        LT_LOG_ERROR("Error string: %s", strerror(errno));
        ret = LT_FAIL;
        goto gpio_error;
    }

    LT_LOG_DEBUG("GPIO chip information:");
    LT_LOG_DEBUG("- info.name  = \"%s\"", info.name);
    LT_LOG_DEBUG("- info.label = \"%s\"", info.label);
    LT_LOG_DEBUG("- info.lines = \"%u\"", info.lines);

    // Setup for INT pin (INPUT, RISING EDGE)
    device->gpioreq_int.offsets[0] = device->gpio_int_num;
    device->gpioreq_int.num_lines = 1;
    // Configure as input with rising edge detection
    device->gpioreq_int.config.flags = GPIO_V2_LINE_FLAG_INPUT | GPIO_V2_LINE_FLAG_EDGE_RISING;

    if (ioctl(device->gpio_fd, GPIO_V2_GET_LINE_IOCTL, &device->gpioreq_int) < 0) {
        LT_LOG_ERROR("GPIO_V2_GET_LINE_IOCTL (INT pin) error!");
        LT_LOG_ERROR("Error string: %s", strerror(errno));
        ret = LT_FAIL;
        goto gpio_error;
    }
#endif

    return LT_OK;

#if LT_USE_INT_PIN
gpio_error:
    close(device->gpio_fd);
    device->gpio_fd = -1;
#endif

spi_error:
    close(device->spi_fd);
    device->spi_fd = -1;

    return ret;
}

lt_ret_t lt_port_deinit(lt_l2_state_t *s2)
{
    lt_dev_linux_spi_native_cs_t *device = (lt_dev_linux_spi_native_cs_t *)(s2->device);

    close(device->spi_fd);
    device->spi_fd = -1;

#if LT_USE_INT_PIN
    close(device->gpioreq_int.fd);
    close(device->gpio_fd);
    device->gpioreq_int.fd = -1;
    device->gpio_fd = -1;
#endif

    return LT_OK;
}

lt_ret_t lt_port_spi_csn_low(lt_l2_state_t *s2)
{
    lt_dev_linux_spi_native_cs_t *device = (lt_dev_linux_spi_native_cs_t *)(s2->device);

    device->frame_in_progress = 1;
    device->frame_completed = 0;

    return LT_OK;
}

lt_ret_t lt_port_spi_csn_high(lt_l2_state_t *s2)
{
    lt_dev_linux_spi_native_cs_t *device = (lt_dev_linux_spi_native_cs_t *)(s2->device);

    device->frame_in_progress = 0;

    return LT_OK;
}

lt_ret_t lt_port_spi_transfer(lt_l2_state_t *s2, uint8_t offset, uint16_t tx_data_length, uint32_t timeout_ms)
{
    LT_UNUSED(offset);
    LT_UNUSED(tx_data_length);
    LT_UNUSED(timeout_ms);
    lt_dev_linux_spi_native_cs_t *device = (lt_dev_linux_spi_native_cs_t *)(s2->device);

    if (!device->frame_in_progress) {
        LT_LOG_ERROR("lt_port_spi_transfer: No transfer in progress (spi_transfer called before csn_low)!");
        return LT_L1_SPI_ERROR;
    }

    if (device->frame_completed) {
        return LT_OK;
    }

    int ret = 0;
    struct spi_ioc_transfer spi = {
        .tx_buf = (unsigned long)s2->buff,
        .rx_buf = (unsigned long)s2->buff,
        .len = TR01_L1_LEN_MAX,  // We always read whole buffer at once.
        .delay_usecs = 0,
    };

    ret = ioctl(device->spi_fd, SPI_IOC_MESSAGE(1), &spi);
    if (ret >= 0) {
        device->frame_completed = 1;
        return LT_OK;
    }
    return LT_L1_SPI_ERROR;
}

lt_ret_t lt_port_delay(lt_l2_state_t *s2, uint32_t ms)
{
    LT_UNUSED(s2);

    int ret = usleep(ms * 1000);
    if (ret != 0) {
        LT_LOG_ERROR("lt_port_delay: usleep() failed: %s (%d)", strerror(errno), ret);
        return LT_FAIL;
    }

    return LT_OK;
}

lt_ret_t lt_port_random_bytes(lt_l2_state_t *s2, void *buff, size_t count)
{
    LT_UNUSED(s2);

    ssize_t ret = getrandom(buff, count, 0);

    if (ret < 0) {
        LT_LOG_ERROR("lt_port_random_bytes: getrandom() failed (%s)!", strerror(errno));
        return LT_FAIL;
    }

    if ((size_t)ret != count) {
        LT_LOG_ERROR("lt_port_random_bytes: getrandom() generated %zd bytes instead of requested %zu bytes!", ret,
                     count);
        return LT_FAIL;
    }

    return LT_OK;
}

#if LT_USE_INT_PIN
lt_ret_t lt_port_delay_on_int(lt_l2_state_t *s2, uint32_t ms)
{
    lt_dev_linux_spi_native_cs_t *device = (lt_dev_linux_spi_native_cs_t *)(s2->device);
    struct pollfd pfd;
    int ret;

    // Set up the poll structure
    pfd.fd = device->gpioreq_int.fd;
    pfd.events = POLLIN | POLLPRI;  // Wait for data or priority event (GPIO edge)
    pfd.revents = 0;

    LT_LOG_DEBUG("lt_port_delay_on_int: Polling on INT pin (fd: %d) for %u ms...", pfd.fd, ms);

    // Wait for the event or timeout
    ret = poll(&pfd, 1, (int)ms);

    if (ret < 0) {
        LT_LOG_ERROR("lt_port_delay_on_int: poll() failed: %s", strerror(errno));
        return LT_FAIL;
    }

    if (ret == 0) {
        LT_LOG_WARN("lt_port_delay_on_int: Timeout waiting for INT pin.");
        return LT_L1_INT_TIMEOUT;
    }

    // Event occurred, check if it's the right type
    if (pfd.revents & (POLLIN | POLLPRI)) {
        // Event is ready. We MUST read it to consume it, otherwise poll()
        // will return immediately on the next call.
        struct gpio_v2_line_event event;
        ret = read(pfd.fd, &event, sizeof(event));

        if (ret < 0) {
            LT_LOG_ERROR("lt_port_delay_on_int: read() on INT pin failed: %s", strerror(errno));
            return LT_FAIL;
        }

        if (ret != sizeof(event)) {
            LT_LOG_ERROR("lt_port_delay_on_int: read() on INT pin returned unexpected size: %d", ret);
            return LT_FAIL;
        }

        // Since we only configured for RISING_EDGE, any event is the one we want.
        // event.id == GPIO_V2_LINE_EVENT_RISING_EDGE
        LT_LOG_DEBUG("lt_port_delay_on_int: Interrupt received!");
        return LT_OK;
    }

    LT_LOG_ERROR("lt_port_delay_on_int: Poll returned positive but no expected revents.");
    return LT_FAIL;
}
#endif

int lt_port_log(const char *format, ...)
{
    va_list args;
    int ret;

    va_start(args, format);
    ret = vfprintf(stderr, format, args);
    fflush(stderr);
    va_end(args);

    return ret;
}