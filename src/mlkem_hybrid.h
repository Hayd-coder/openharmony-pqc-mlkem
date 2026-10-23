/*
 * Educational KEM-DEM helpers shared by the N-API demo and host self-test.
 * This construction is for the graduation-project demonstration and is not
 * presented as a standardized or certified commercial DEM.
 */

#ifndef PQC_MLKEM_HYBRID_H
#define PQC_MLKEM_HYBRID_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "mlkem_native.h"

#define MAX_MESSAGE_BYTES 512U
#define NONCE_BYTES 16U
#define TAG_BYTES 32U

extern void mlkem_shake256(uint8_t *output, size_t outlen,
    const uint8_t *input, size_t inlen);
extern void mlkem_sha3_256(uint8_t output[TAG_BYTES],
    const uint8_t *input, size_t inlen);

static void DeriveKeyStream(const uint8_t secret[MLKEM_BYTES],
    const uint8_t nonce[NONCE_BYTES], uint8_t *output, size_t outputLength)
{
    uint8_t input[MLKEM_BYTES + NONCE_BYTES] = {0};
    (void)memcpy(input, secret, MLKEM_BYTES);
    (void)memcpy(input + MLKEM_BYTES, nonce, NONCE_BYTES);
    mlkem_shake256(output, outputLength, input, sizeof(input));
}

static void ComputeTag(const uint8_t secret[MLKEM_BYTES],
    const uint8_t nonce[NONCE_BYTES], const uint8_t *ciphertext,
    size_t ciphertextLength, uint8_t tag[TAG_BYTES])
{
    uint8_t input[MLKEM_BYTES + NONCE_BYTES + MAX_MESSAGE_BYTES] = {0};
    (void)memcpy(input, secret, MLKEM_BYTES);
    (void)memcpy(input + MLKEM_BYTES, nonce, NONCE_BYTES);
    (void)memcpy(input + MLKEM_BYTES + NONCE_BYTES, ciphertext,
        ciphertextLength);
    mlkem_sha3_256(tag, input,
        MLKEM_BYTES + NONCE_BYTES + ciphertextLength);
}

static bool ConstantTimeEqual(const uint8_t *left, const uint8_t *right,
    size_t length)
{
    uint8_t difference = 0;
    size_t index;
    for (index = 0; index < length; ++index) {
        difference |= left[index] ^ right[index];
    }
    return difference == 0;
}

#endif
