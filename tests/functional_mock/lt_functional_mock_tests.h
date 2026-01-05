#ifndef LT_FUNCTIONAL_MOCK_TESTS_H
#define LT_FUNCTIONAL_MOCK_TESTS_H

/**
 * @file lt_functional_mock_tests.h
 * @brief Declaration of functional mock test functions.
 * @copyright Copyright (c) 2020-2025 Tropic Square s.r.o.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "libtropic_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Test for checking if TROPIC01 attributes are set correctly based on RISC-V FW version.
 *
 * Test steps:
 *  For each tested RISC-V FW version:
 *   1. Mock TROPIC01 responses on init to simulate different RISC-V FW versions.
 *   2. Initialize libtropic handle.
 *   3. Verify that attributes in the handle are set correctly according to the mocked FW version
 *   4. Deinitialize libtropic handle.
 *
 * @param h Handle for communication with TROPIC01
 */
void lt_test_mock_attrs(lt_handle_t *h);

/**
 * @brief Test for handling invalid CRC in TROPIC01 responses.
 *
 * Test steps:
 *  1. Mock a response with an invalid CRC for a dummy request (Get_Info is used).
 *  2. Send request.
 *  3. Verify that Libtropic correctly identifies the invalid CRC and returns an error.
 *
 * @param h Handle for communication with TROPIC01
 */
void lt_test_mock_invalid_in_crc(lt_handle_t *h);

/**
 * @brief Test for handling HARDWARE_FAIL return code.
 *
 * Test steps:
 * 1. Mock Secure Session initialization.
 * 2. For each of Pairing_Key_Write, Pairing_Key_Invalidate, R_Config_Write, I_Config_Write, R_Mem_Data_Write:
 *   a. Mock L3 Result with RESULT=HARDWARE_FAIL.
 *   b. Call Libtropic function corresponding to the L3 Command and verify that Libtropic returns LT_L3_HARDWARE_FAIL.
 * 3. Mock Secure Session deinitialization.
 *
 * @param h Handle for communication with TROPIC01
 */
void lt_test_mock_hardware_fail(lt_handle_t *h);

#ifdef __cplusplus
}
#endif

#endif  // LT_FUNCTIONAL_MOCK_TESTS_H