#include <stdint.h>
#include <string.h>

#include "main.h"
#include <wolfssl/wolfcrypt/error-crypt.h>
#include "stm32f4xx_hal.h"

int wolfcrypt_custom_seed_gen(unsigned char* output, unsigned int sz)
{
    HAL_StatusTypeDef hal_status = HAL_OK;
    uint32_t random_data;
    size_t bytes_left = sz;

    while (bytes_left) {
        hal_status = HAL_RNG_GenerateRandomNumber(&RNGHandle, &random_data);
        if (hal_status != HAL_OK) {
            return RNG_FAILURE_E;
        }

        size_t cpy_cnt = bytes_left < sizeof(random_data) ? bytes_left : sizeof(random_data);
        memcpy(output, &random_data, cpy_cnt);
        bytes_left -= cpy_cnt;
        output += cpy_cnt;
    }

    return 0;
}