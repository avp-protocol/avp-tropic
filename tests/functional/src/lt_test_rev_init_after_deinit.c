/**
 * @file lt_test_rev_init_after_deinit.c
 * @brief Test calling lt_init after lt_deinit.
 * @copyright Copyright (c) 2020-2026 Tropic Square s.r.o.
 *
 * @license For the license see LICENSE.md in the root directory of this source tree.
 */

#include <inttypes.h>
#include <string.h>

#include "libtropic.h"
#include "libtropic_common.h"
#include "libtropic_functional_tests.h"
#include "libtropic_logging.h"
#include "lt_port_wrap.h"
#include "lt_test_common.h"

void lt_test_rev_init_after_deinit(lt_handle_t *h)
{
    LT_LOG_INFO("----------------------------------------------");
    LT_LOG_INFO("lt_test_rev_init_after_deinit()");
    LT_LOG_INFO("----------------------------------------------");

    uint8_t ping_msg_out[TR01_PING_LEN_MAX], ping_msg_in[TR01_PING_LEN_MAX];
    uint16_t ping_msg_len;

    for (int i = 0; i < 2; i++) {
        LT_LOG_INFO("Iteration #%d", i + 1);

        LT_LOG_INFO("Initializing handle");
        LT_TEST_ASSERT(LT_OK, lt_init(h));

        LT_LOG_INFO("Starting Secure Session with key %d", (int)TR01_PAIRING_KEY_SLOT_INDEX_0);
        LT_TEST_ASSERT(LT_OK,
                       lt_verify_chip_and_start_secure_session(h, LT_TEST_SH0_PRIV, LT_TEST_SH0_PUB,
                                                               TR01_PAIRING_KEY_SLOT_INDEX_0));

        LT_LOG_INFO("Generating random data length <= %d...", (int)TR01_PING_LEN_MAX);
        LT_TEST_ASSERT(LT_OK, lt_random_bytes(h, &ping_msg_len, sizeof(ping_msg_len)));
        ping_msg_len %= TR01_PING_LEN_MAX + 1;  // 0-4096

        LT_LOG_INFO("Generating %" PRIu16 " random bytes...", ping_msg_len);
        LT_TEST_ASSERT(LT_OK, lt_random_bytes(h, ping_msg_out, ping_msg_len));

        LT_LOG_INFO("Sending Ping command...");
        LT_TEST_ASSERT(LT_OK, lt_ping(h, ping_msg_out, ping_msg_in, ping_msg_len));

        LT_LOG_INFO("Comparing sent and received message...");
        LT_TEST_ASSERT(0, memcmp(ping_msg_out, ping_msg_in, ping_msg_len));

        LT_LOG_INFO("Aborting Secure Session...");
        LT_TEST_ASSERT(LT_OK, lt_session_abort(h));

        LT_LOG_INFO("Deinitializing handle");
        LT_TEST_ASSERT(LT_OK, lt_deinit(h));

        LT_LOG_LINE();
    }
}
