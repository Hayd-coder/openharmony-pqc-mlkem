#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mlkem_native.h"
#include "mlkem_hybrid.h"
#include "randombytes_ohos.h"

#define CHECK(condition, message)                          \
    do {                                                   \
        if (!(condition)) {                                \
            (void)fprintf(stderr, "[FAIL] %s\n", message); \
            return 1;                                      \
        }                                                  \
    } while (0)

int main(void)
{
    static const char message[] = "你好，OpenHarmony 后量子密码！";
    const size_t messageLength = strlen(message);
    uint8_t publicKey[MLKEM512_PUBLICKEYBYTES] = {0};
    uint8_t secretKey[MLKEM512_SECRETKEYBYTES] = {0};
    uint8_t kemCiphertext[MLKEM512_CIPHERTEXTBYTES] = {0};
    uint8_t senderSecret[MLKEM_BYTES] = {0};
    uint8_t receiverSecret[MLKEM_BYTES] = {0};
    uint8_t nonce[NONCE_BYTES] = {0};
    uint8_t keyStream[MAX_MESSAGE_BYTES] = {0};
    uint8_t ciphertext[MAX_MESSAGE_BYTES] = {0};
    uint8_t recovered[MAX_MESSAGE_BYTES + 1U] = {0};
    uint8_t tag[TAG_BYTES] = {0};
    uint8_t expectedTag[TAG_BYTES] = {0};
    size_t index;

    CHECK(messageLength <= MAX_MESSAGE_BYTES, "test message is too long");
    CHECK(mlkem_keypair(publicKey, secretKey) == 0,
        "key generation failed");
    CHECK(mlkem_enc(kemCiphertext, senderSecret, publicKey) == 0,
        "encapsulation failed");
    CHECK(mlkem_dec(receiverSecret, kemCiphertext, secretKey) == 0,
        "decapsulation failed");
    CHECK(ConstantTimeEqual(senderSecret, receiverSecret, MLKEM_BYTES),
        "sender and receiver shared secrets differ");
    CHECK(randombytes(nonce, sizeof(nonce)) == 0,
        "nonce generation failed");

    DeriveKeyStream(senderSecret, nonce, keyStream, messageLength);
    for (index = 0; index < messageLength; ++index) {
        ciphertext[index] = ((const uint8_t *)message)[index] ^ keyStream[index];
    }
    ComputeTag(senderSecret, nonce, ciphertext, messageLength, tag);

    ComputeTag(receiverSecret, nonce, ciphertext, messageLength, expectedTag);
    CHECK(ConstantTimeEqual(tag, expectedTag, TAG_BYTES),
        "valid message authentication failed");
    DeriveKeyStream(receiverSecret, nonce, keyStream, messageLength);
    for (index = 0; index < messageLength; ++index) {
        recovered[index] = ciphertext[index] ^ keyStream[index];
    }
    CHECK(memcmp(message, recovered, messageLength) == 0,
        "recovered UTF-8 plaintext differs from input");

    ciphertext[messageLength / 2U] ^= 0x01U;
    ComputeTag(receiverSecret, nonce, ciphertext, messageLength, expectedTag);
    CHECK(!ConstantTimeEqual(tag, expectedTag, TAG_BYTES),
        "tampered message was accepted");

    (void)printf("Plaintext: %s\n", message);
    (void)printf("Plaintext bytes: %u\n", (unsigned int)messageLength);
    (void)printf("Hybrid payload bytes: %u\n",
        (unsigned int)(MLKEM512_CIPHERTEXTBYTES + NONCE_BYTES +
        TAG_BYTES + messageLength));
    (void)printf("[PASS] UTF-8 encrypt/decrypt and tamper rejection passed\n");
    return 0;
}
