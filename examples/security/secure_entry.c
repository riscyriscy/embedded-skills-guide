/* The secure side of a TrustZone-M design: one function that the non-secure side may
   call. The key stays in secure memory. Build with -mcpu=cortex-m33 -mcmse. */
#include <arm_cmse.h>
#include <stddef.h>
#include <stdint.h>

static const uint8_t device_key[16] = {1, 2,  3,  4,  5,  6,  7,  8,
                                       9, 10, 11, 12, 13, 14, 15, 16};

/* The entry point is reached through a secure gateway (sg) instruction and returns
   with bxns. */
__attribute__((cmse_nonsecure_entry)) int secure_sign(const uint8_t *message,
                                                      size_t length, uint8_t *tag)
{
    /* The caller is not trusted: both pointers must lie in memory that the
       non-secure side may use. */
    if (cmse_check_address_range((void *)message, length,
                                 CMSE_NONSECURE | CMSE_MPU_READ) == NULL ||
        cmse_check_address_range(tag, 16,
                                 CMSE_NONSECURE | CMSE_MPU_READWRITE) == NULL) {
        return -1;
    }
    for (size_t i = 0; i < 16; i++) { /* a stand-in for a real MAC */
        tag[i] = device_key[i] ^ (length > i ? message[i] : 0u);
    }
    return 0;
}
