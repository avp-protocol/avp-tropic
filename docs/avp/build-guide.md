# Build Guide

## Prerequisites

### Software

| Tool | Version | Purpose |
|------|---------|---------|
| CMake | 3.16+ | Build system |
| ARM GCC | 10+ | Cross compiler |
| STM32CubeMX | 6.x | Code generation (optional) |
| OpenOCD | 0.11+ | Flashing/debugging |
| Make or Ninja | — | Build tool |

### Installation (macOS)

```bash
# Install ARM toolchain
brew install --cask gcc-arm-embedded

# Install build tools
brew install cmake ninja openocd

# Verify installation
arm-none-eabi-gcc --version
cmake --version
```

### Installation (Ubuntu/Debian)

```bash
# Install ARM toolchain
sudo apt install gcc-arm-none-eabi

# Install build tools
sudo apt install cmake ninja-build openocd

# Verify installation
arm-none-eabi-gcc --version
```

### Installation (Windows)

1. Download [ARM GCC](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm)
2. Install [CMake](https://cmake.org/download/)
3. Install [Ninja](https://github.com/ninja-build/ninja/releases)
4. Add all to PATH

---

## Project Structure

```
avp-tropic/
├── avp/                          # AVP protocol layer
│   ├── avp_tropic.h
│   └── avp_tropic.c
├── hal/
│   └── stm32/
│       └── stm32u5_tropic_click/
│           ├── CMakeLists.txt
│           ├── libtropic_port_stm32u5_tropic_click.h
│           └── libtropic_port_stm32u5_tropic_click.c
├── examples/
│   └── stm32/
│       └── stm32u5_tropic_click/
│           └── avp_vault/        # Example application
├── include/                      # libtropic headers
├── src/                          # libtropic source
├── cmake/
│   └── arm-none-eabi.cmake       # Toolchain file
└── CMakeLists.txt
```

---

## Building

### Step 1: Clone Repository

```bash
git clone https://github.com/avp-protocol/avp-tropic.git
cd avp-tropic
```

### Step 2: Configure Build

```bash
mkdir build && cd build

cmake -G Ninja \
      -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi.cmake \
      -DCMAKE_BUILD_TYPE=Release \
      -DHAL=stm32u5_tropic_click \
      -DTARGET_MCU=STM32U575xx \
      ..
```

**Build Options:**

| Option | Values | Description |
|--------|--------|-------------|
| `HAL` | `stm32u5_tropic_click` | Hardware abstraction layer |
| `TARGET_MCU` | `STM32U575xx`, `STM32U585xx` | Target MCU |
| `CMAKE_BUILD_TYPE` | `Debug`, `Release` | Build type |
| `LT_LOG_LEVEL` | 0-4 | Logging verbosity |
| `LT_USE_INT_PIN` | ON/OFF | Use interrupt pin |

### Step 3: Build

```bash
ninja
# or
make -j$(nproc)
```

**Build Outputs:**

```
build/
├── libtropic.a               # libtropic static library
├── libtropic_hal.a           # HAL static library
├── libavp_tropic.a           # AVP protocol library
└── examples/
    └── avp_vault.elf         # Example firmware
    └── avp_vault.bin         # Binary for flashing
    └── avp_vault.hex         # Intel HEX format
```

---

## Flashing

### Using OpenOCD

```bash
# Flash binary
openocd -f interface/stlink.cfg \
        -f target/stm32u5x.cfg \
        -c "program build/examples/avp_vault.bin 0x08000000 verify reset exit"
```

### Using STM32CubeProgrammer

```bash
STM32_Programmer_CLI -c port=SWD \
                     -w build/examples/avp_vault.bin 0x08000000 \
                     -v -rst
```

### Using st-flash

```bash
st-flash write build/examples/avp_vault.bin 0x08000000
```

---

## Debugging

### GDB with OpenOCD

Terminal 1 - Start OpenOCD:
```bash
openocd -f interface/stlink.cfg -f target/stm32u5x.cfg
```

Terminal 2 - Start GDB:
```bash
arm-none-eabi-gdb build/examples/avp_vault.elf

(gdb) target remote localhost:3333
(gdb) monitor reset halt
(gdb) load
(gdb) break main
(gdb) continue
```

### VS Code Configuration

`.vscode/launch.json`:
```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug AVP-Tropic",
            "type": "cortex-debug",
            "request": "launch",
            "servertype": "openocd",
            "cwd": "${workspaceFolder}",
            "executable": "${workspaceFolder}/build/examples/avp_vault.elf",
            "configFiles": [
                "interface/stlink.cfg",
                "target/stm32u5x.cfg"
            ],
            "svdFile": "${workspaceFolder}/STM32U575.svd",
            "runToMain": true
        }
    ]
}
```

---

## Example Application

### Creating a New Application

1. Create directory:
```bash
mkdir -p examples/stm32/stm32u5_tropic_click/my_app
```

2. Create `CMakeLists.txt`:
```cmake
cmake_minimum_required(VERSION 3.16)
project(my_app C ASM)

# Add executable
add_executable(my_app
    main.c
    startup_stm32u575xx.s
    system_stm32u5xx.c
)

# Link libraries
target_link_libraries(my_app
    avp_tropic
    libtropic
    libtropic_hal_stm32u5_tropic_click
)

# Generate binary
add_custom_command(TARGET my_app POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:my_app> my_app.bin
)
```

3. Create `main.c`:
```c
#include "avp_tropic.h"
#include "libtropic_port_stm32u5_tropic_click.h"
#include "stm32u5xx_hal.h"

/* Peripheral handles */
SPI_HandleTypeDef hspi1;
RNG_HandleTypeDef hrng;
UART_HandleTypeDef huart2;

void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_SPI1_Init(void);
void MX_RNG_Init(void);
void MX_USART2_UART_Init(void);

int main(void) {
    /* Initialize HAL */
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_RNG_Init();
    MX_USART2_UART_Init();

    printf("AVP-Tropic Example\r\n");

    /* Configure device */
    lt_dev_stm32u5_tropic_click_t device = {
        .spi_instance = SPI1,
        .baudrate_prescaler = SPI_BAUDRATEPRESCALER_16,
        .spi_cs_gpio_pin = GPIO_PIN_14,
        .spi_cs_gpio_port = GPIOD,
        .int_gpio_pin = GPIO_PIN_13,
        .int_gpio_port = GPIOF,
        .rng_handle = &hrng,
    };

    /* Initialize AVP */
    avp_vault_t vault;
    if (avp_init(&vault, &device) != AVP_OK) {
        printf("AVP init failed\r\n");
        while (1);
    }

    /* Authenticate */
    if (avp_authenticate(&vault, NULL, "123456", 300) != AVP_OK) {
        printf("Auth failed\r\n");
        while (1);
    }

    printf("Authenticated! Session: %s\r\n", vault.session_id);

    /* Your application code here */

    while (1) {
        /* Main loop */
    }
}

/* Printf redirect to UART */
int _write(int fd, char *ptr, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}
```

---

## Troubleshooting

### Build Errors

| Error | Solution |
|-------|----------|
| `arm-none-eabi-gcc not found` | Add toolchain to PATH |
| `CMake error: could not find CMAKE_C_COMPILER` | Set `CMAKE_TOOLCHAIN_FILE` |
| `undefined reference to HAL_*` | Link STM32 HAL library |
| `multiple definition of main` | Only one main() in project |

### Link Errors

| Error | Solution |
|-------|----------|
| `undefined reference to lt_*` | Link `libtropic` |
| `undefined reference to avp_*` | Link `avp_tropic` |
| `.text section exceeds region` | Optimize for size (`-Os`) |

### Runtime Issues

| Issue | Solution |
|-------|----------|
| No output on UART | Check baud rate (115200) |
| SPI communication fails | Verify pin connections |
| Device not responding | Check power supply (3.3V) |
| Authentication fails | Verify PIN, check lockout |

---

## Memory Optimization

### Reduce Flash Usage

```cmake
# Enable LTO (Link Time Optimization)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")

# Optimize for size
set(CMAKE_C_FLAGS_RELEASE "-Os")

# Remove unused code
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
```

### Reduce RAM Usage

```cmake
# Use smaller buffers
add_compile_definitions(LT_L1_LEN_MAX=512)  # Default: 1024

# Disable features
add_compile_definitions(LT_USE_INT_PIN=0)   # Saves ~20 bytes
```

### Typical Memory Usage

| Component | Flash | RAM |
|-----------|-------|-----|
| AVP Protocol | ~4 KB | ~1 KB |
| libtropic | ~20 KB | ~2 KB |
| HAL | ~2 KB | ~100 B |
| STM32 HAL | ~10 KB | ~500 B |
| **Total** | **~36 KB** | **~3.6 KB** |

STM32U575 has 2MB Flash and 786KB RAM — plenty of headroom.
