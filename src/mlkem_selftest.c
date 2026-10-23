#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mlkem_native.h"

#define CHECK(condition, message)                         \
    do {                                                  \
        if (!(condition)) {                               \
            (void)fprintf(stderr, "[FAIL] %s\n", message); \
            return 1;                                     \
        }                                                 \
    } while (0)

int main(void)
{
    uint8_t public_key[MLKEM512_PUBLICKEYBYTES];
    uint8_t secret_key[MLKEM512_SECRETKEYBYTES];
    uint8_t secret_key_modified[MLKEM512_SECRETKEYBYTES];
    uint8_t ciphertext[MLKEM512_CIPHERTEXTBYTES];
    uint8_t ciphertext_modified[MLKEM512_CIPHERTEXTBYTES];
    uint8_t shared_secret_sender[MLKEM_BYTES];
    uint8_t shared_secret_receiver[MLKEM_BYTES];
    uint8_t shared_secret_modified[MLKEM_BYTES];

    CHECK(mlkem_keypair(public_key, secret_key) == 0,
          "key generation failed");
    CHECK(mlkem_check_pk(public_key) == 0,
          "generated public key did not pass validation");
    CHECK(mlkem_check_sk(secret_key) == 0,
          "generated secret key did not pass validation");

    CHECK(mlkem_enc(ciphertext, shared_secret_sender, public_key) == 0,
          "encapsulation failed");
    CHECK(mlkem_dec(shared_secret_receiver, ciphertext, secret_key) == 0,
          "decapsulation failed");
    CHECK(memcmp(shared_secret_sender, shared_secret_receiver, MLKEM_BYTES) == 0,
          "sender and receiver produced different shared secrets");

    /* ML-KEM uses implicit rejection: a damaged ciphertext still decapsulates,
     * but must not reproduce the original shared secret. */
    (void)memcpy(ciphertext_modified, ciphertext, sizeof(ciphertext_modified));
    ciphertext_modified[sizeof(ciphertext_modified) / 2] ^= 0x01U;
    CHECK(mlkem_dec(shared_secret_modified, ciphertext_modified, secret_key) == 0,
          "implicit rejection returned an unexpected error");
    CHECK(memcmp(shared_secret_sender, shared_secret_modified, MLKEM_BYTES) != 0,
          "damaged ciphertext reproduced the original shared secret");

    /* The stored H(pk) begins 64 bytes before the end of an ML-KEM secret key. */
    (void)memcpy(secret_key_modified, secret_key, sizeof(secret_key_modified));
    secret_key_modified[sizeof(secret_key_modified) - 64] ^= 0x01U;
    CHECK(mlkem_check_sk(secret_key_modified) == MLK_ERR_INVALID_SK,
          "damaged secret key was not rejected by validation");
    CHECK(mlkem_dec(shared_secret_modified, ciphertext, secret_key_modified) ==
              MLK_ERR_INVALID_SK,
          "decapsulation accepted a damaged secret key");

    (void)printf("ML-KEM-512 public key:  %u bytes\n",
                 (unsigned int)sizeof(public_key));
    (void)printf("ML-KEM-512 secret key:  %u bytes\n",
                 (unsigned int)sizeof(secret_key));
    (void)printf("ML-KEM-512 ciphertext:  %u bytes\n",
                 (unsigned int)sizeof(ciphertext));
    (void)printf("ML-KEM shared secret:   %u bytes\n",
                 (unsigned int)sizeof(shared_secret_sender));
    (void)printf("[PASS] key generation, encapsulation, decapsulation and "
                 "negative tests passed\n");
    return 0;
}
