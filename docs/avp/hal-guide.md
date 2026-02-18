# Hardware Abstraction Layer (HAL) Guide

## Overview

The AVP-Tropic HAL provides platform-specific implementations for communicating with the TROPIC01 secure element. This guide covers the STM32U5 + Secure Tropic Click HAL and how to port to other platforms.

## STM32U5 Tropic Click HAL

### Files

```
hal/stm32/stm32u5_tropic_click/
├── CMakeLists.txt
├── libtropic_port_stm32u5_tropic_click.h
└── libtropic_port_stm32u5_tropic_click.c
```

### Device Structure

```c
typedef struct lt_dev_stm32u5_tropic_click_t {
    /* SPI Configuration */
    SPI_TypeDef *spi_instance;       // SPI1, SPI2, SPI3
    uint32_t baudrate_prescaler;     // SPI clock divider

    /* GPIO - Chip Select */
    uint16_t spi_cs_gpio_pin;        // GPIO_PIN_xx
    GPIO_TypeDef *spi_cs_gpio_port;  // GPIOx

    /* GPIO - Interrupt (optional) */
    uint16_t int_gpio_pin;
    GPIO_TypeDef *int_gpio_port;

    /* GPIO - Reset (optional) */
    uint16_t rst_gpio_pin;
    GPIO_TypeDef *rst_gpio_port;

    /* Hardware RNG */
    RNG_HandleTypeDef *rng_handle;

    /* Private - managed by HAL */
    SPI_HandleTypeDef spi_handle;
} lt_dev_stm32u5_tropic_click_t;
```

### Pin Mapping

#### NUCLEO-U575ZI-Q with MikroBUS

| Function | STM32 Pin | MikroBUS Pin | Click Pin |
|----------|-----------|--------------|-----------|
| SPI SCK | PA5 | SCK | 4 |
| SPI MISO | PA6 | MISO | 5 |
| SPI MOSI | PA7 | MOSI | 6 |
| CS | PD14 | CS | 3 |
| INT | PF13 | INT | 15 |
| RST | — | RST | 16 |
| 3.3V | 3V3 | 3V3 | 7 |
| GND | GND | GND | 8 |

#### Default Configuration

```c
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
```

### SPI Configuration

TROPIC01 SPI requirements:
- Mode 0 (CPOL=0, CPHA=0)
- MSB first
- 8-bit data size
- Maximum 20 MHz clock

```c
/* SPI parameters set by HAL */
device->spi_handle.Init.Direction = SPI_DIRECTION_2LINES;
device->spi_handle.Init.CLKPhase = SPI_PHASE_1EDGE;      // CPHA=0
device->spi_handle.Init.CLKPolarity = SPI_POLARITY_LOW;  // CPOL=0
device->spi_handle.Init.DataSize = SPI_DATASIZE_8BIT;
device->spi_handle.Init.FirstBit = SPI_FIRSTBIT_MSB;
device->spi_handle.Init.Mode = SPI_MODE_MASTER;
```

Recommended clock speeds:

| Prescaler | Clock (160MHz core) | Notes |
|-----------|---------------------|-------|
| 8 | 20 MHz | Maximum for TROPIC01 |
| 16 | 10 MHz | Recommended |
| 32 | 5 MHz | Safe default |
| 64 | 2.5 MHz | For long cables |

### HAL Functions

#### lt_port_init

Initializes SPI and GPIO peripherals.

```c
lt_ret_t lt_port_init(lt_l2_state_t *s2);
```

- Configures SPI with device parameters
- Sets up CS as output (high by default)
- Configures INT as input (if enabled)
- Configures RST as output (if provided)

#### lt_port_deinit

Deinitializes peripherals.

```c
lt_ret_t lt_port_deinit(lt_l2_state_t *s2);
```

#### lt_port_spi_transfer

Full-duplex SPI transfer.

```c
lt_ret_t lt_port_spi_transfer(
    lt_l2_state_t *s2,
    uint8_t offset,
    uint16_t tx_data_length,
    uint32_t timeout_ms
);
```

- Uses `HAL_SPI_TransmitReceive()` for full-duplex
- Data is in `s2->buff` at specified offset
- Same buffer used for TX and RX

#### lt_port_spi_csn_low / lt_port_spi_csn_high

Controls chip select line.

```c
lt_ret_t lt_port_spi_csn_low(lt_l2_state_t *s2);
lt_ret_t lt_port_spi_csn_high(lt_l2_state_t *s2);
```

- Includes verification loop to confirm GPIO state
- Returns `LT_L1_SPI_ERROR` if pin doesn't respond

#### lt_port_random_bytes

Generates random bytes using hardware RNG.

```c
lt_ret_t lt_port_random_bytes(lt_l2_state_t *s2, void *buff, size_t count);
```

- Uses STM32U5 TRNG (True Random Number Generator)
- Required for session nonces and key generation

#### lt_port_delay / lt_port_delay_on_int

Timing functions.

```c
lt_ret_t lt_port_delay(lt_l2_state_t *s2, uint32_t ms);
lt_ret_t lt_port_delay_on_int(lt_l2_state_t *s2, uint32_t ms);
```

- `lt_port_delay`: Uses `HAL_Delay()`
- `lt_port_delay_on_int`: Polls INT pin, returns when high or timeout

#### lt_port_hw_reset

Hardware reset via RST pin.

```c
lt_ret_t lt_port_hw_reset(lt_l2_state_t *s2);
```

- Only works if RST pin is configured
- 10ms low pulse, 50ms recovery time

---

## Porting to Other Platforms

### Required Functions

To port AVP-Tropic to a new platform, implement these functions:

```c
/* Initialize hardware */
lt_ret_t lt_port_init(lt_l2_state_t *s2);
lt_ret_t lt_port_deinit(lt_l2_state_t *s2);

/* SPI communication */
lt_ret_t lt_port_spi_transfer(lt_l2_state_t *s2, uint8_t offset,
                              uint16_t tx_data_length, uint32_t timeout_ms);
lt_ret_t lt_port_spi_csn_low(lt_l2_state_t *s2);
lt_ret_t lt_port_spi_csn_high(lt_l2_state_t *s2);

/* Random number generation */
lt_ret_t lt_port_random_bytes(lt_l2_state_t *s2, void *buff, size_t count);

/* Timing */
lt_ret_t lt_port_delay(lt_l2_state_t *s2, uint32_t ms);

/* Optional: Interrupt-based waiting */
#if LT_USE_INT_PIN
lt_ret_t lt_port_delay_on_int(lt_l2_state_t *s2, uint32_t ms);
#endif

/* Logging */
int lt_port_log(const char *format, ...);
```

### Example: Raspberry Pi Port

```c
/* Raspberry Pi HAL using wiringPi/pigpio */

#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <time.h>

#define SPI_CHANNEL 0
#define SPI_SPEED   10000000  // 10 MHz
#define CS_PIN      8
#define INT_PIN     25

typedef struct lt_dev_rpi_t {
    int spi_channel;
    int cs_pin;
    int int_pin;
} lt_dev_rpi_t;

lt_ret_t lt_port_init(lt_l2_state_t *s2) {
    lt_dev_rpi_t *dev = (lt_dev_rpi_t *)s2->device;

    wiringPiSetupGpio();
    wiringPiSPISetup(dev->spi_channel, SPI_SPEED);

    pinMode(dev->cs_pin, OUTPUT);
    digitalWrite(dev->cs_pin, HIGH);

    pinMode(dev->int_pin, INPUT);

    return LT_OK;
}

lt_ret_t lt_port_spi_transfer(lt_l2_state_t *s2, uint8_t offset,
                              uint16_t len, uint32_t timeout_ms) {
    lt_dev_rpi_t *dev = (lt_dev_rpi_t *)s2->device;
    wiringPiSPIDataRW(dev->spi_channel, s2->buff + offset, len);
    return LT_OK;
}

lt_ret_t lt_port_spi_csn_low(lt_l2_state_t *s2) {
    lt_dev_rpi_t *dev = (lt_dev_rpi_t *)s2->device;
    digitalWrite(dev->cs_pin, LOW);
    return LT_OK;
}

lt_ret_t lt_port_spi_csn_high(lt_l2_state_t *s2) {
    lt_dev_rpi_t *dev = (lt_dev_rpi_t *)s2->device;
    digitalWrite(dev->cs_pin, HIGH);
    return LT_OK;
}

lt_ret_t lt_port_random_bytes(lt_l2_state_t *s2, void *buff, size_t count) {
    FILE *f = fopen("/dev/urandom", "r");
    if (f) {
        fread(buff, 1, count, f);
        fclose(f);
        return LT_OK;
    }
    return LT_FAIL;
}

lt_ret_t lt_port_delay(lt_l2_state_t *s2, uint32_t ms) {
    delay(ms);
    return LT_OK;
}
```

### Example: ESP32 Port

```c
/* ESP32 HAL using ESP-IDF */

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_random.h"

typedef struct lt_dev_esp32_t {
    spi_device_handle_t spi;
    gpio_num_t cs_pin;
    gpio_num_t int_pin;
} lt_dev_esp32_t;

lt_ret_t lt_port_init(lt_l2_state_t *s2) {
    lt_dev_esp32_t *dev = (lt_dev_esp32_t *)s2->device;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = GPIO_NUM_23,
        .miso_io_num = GPIO_NUM_19,
        .sclk_io_num = GPIO_NUM_18,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 10000000,
        .mode = 0,
        .spics_io_num = -1,  // Manual CS
        .queue_size = 1,
    };

    spi_bus_initialize(VSPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    spi_bus_add_device(VSPI_HOST, &dev_cfg, &dev->spi);

    gpio_set_direction(dev->cs_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(dev->cs_pin, 1);

    return LT_OK;
}

lt_ret_t lt_port_random_bytes(lt_l2_state_t *s2, void *buff, size_t count) {
    esp_fill_random(buff, count);
    return LT_OK;
}
```

---

## Debugging

### Enable Debug Logging

Set log level in your build:

```cmake
add_compile_definitions(LT_LOG_LEVEL=4)  # Debug level
```

Log levels:
- 0: None
- 1: Error
- 2: Warning
- 3: Info
- 4: Debug

### Common Issues

| Symptom | Cause | Solution |
|---------|-------|----------|
| `LT_L1_SPI_ERROR` | SPI not initialized | Check `HAL_SPI_Init()` return |
| `LT_L1_SPI_ERROR` | Wrong SPI mode | Verify CPOL=0, CPHA=0 |
| `LT_L1_SPI_ERROR` | CS pin not working | Check GPIO config |
| `LT_L1_INT_TIMEOUT` | INT pin not connected | Verify wiring |
| `LT_FAIL` | RNG not initialized | Initialize RNG peripheral |
| Communication errors | Clock too fast | Reduce SPI prescaler |

### Logic Analyzer Setup

Recommended probes:
- SPI CLK, MOSI, MISO, CS
- INT pin
- 3.3V and GND

Protocol settings:
- SPI Mode 0
- MSB first
- 8-bit words

### UART Debug Output

The HAL uses `printf()` for logging. Configure UART:

```c
/* Redirect printf to UART */
int _write(int fd, char *ptr, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}
```
