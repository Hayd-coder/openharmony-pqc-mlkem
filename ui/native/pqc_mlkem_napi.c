/*
 * Copyright (c) 2026
 * Licensed under the Apache License, Version 2.0.
 */

#include <napi/native_api.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "mlkem_native.h"
#include "../../src/mlkem_hybrid.h"

#define FINGERPRINT_BYTES 8U
#define FINGERPRINT_CHARS (FINGERPRINT_BYTES * 2U)
#define KEM_PREVIEW_BYTES 32U
#define DEFAULT_BENCHMARK_ITERATIONS 500U
#define MAX_BENCHMARK_ITERATIONS 5000U
#define BENCHMARK_WARMUP_ITERATIONS 5U

extern int randombytes(uint8_t *out, size_t outlen);

/* Separately namespaced portable build for an in-process comparison. */
extern int mlkem_portable_keypair(uint8_t *publicKey, uint8_t *secretKey);
extern int mlkem_portable_enc(uint8_t *ciphertext, uint8_t *sharedSecret,
    const uint8_t *publicKey);
extern int mlkem_portable_dec(uint8_t *sharedSecret,
    const uint8_t *ciphertext, const uint8_t *secretKey);

typedef struct {
    bool ready;
    size_t messageLength;
    uint8_t secretKey[MLKEM512_SECRETKEYBYTES];
    uint8_t kemCiphertext[MLKEM512_CIPHERTEXTBYTES];
    uint8_t messageCiphertext[MAX_MESSAGE_BYTES];
    uint8_t nonce[NONCE_BYTES];
    uint8_t tag[TAG_BYTES];
    uint8_t senderSecret[MLKEM_BYTES];
} HybridSession;

typedef int (*KeypairFunction)(uint8_t *, uint8_t *);
typedef int (*EncapsFunction)(uint8_t *, uint8_t *, const uint8_t *);
typedef int (*DecapsFunction)(uint8_t *, const uint8_t *, const uint8_t *);

typedef struct {
    double keypairUs;
    double encapsUs;
    double decapsUs;
} BenchmarkResult;

static HybridSession g_session = {0};

static uint64_t MonotonicTimeNs(void)
{
    struct timespec current = {0};
    if (clock_gettime(CLOCK_MONOTONIC, &current) != 0) {
        return 0;
    }
    return (uint64_t)current.tv_sec * UINT64_C(1000000000) +
        (uint64_t)current.tv_nsec;
}

static void HexEncode(const uint8_t *data, size_t size, char *result)
{
    static const char hex[] = "0123456789abcdef";
    size_t index;
    for (index = 0; index < size; ++index) {
        result[index * 2U] = hex[data[index] >> 4U];
        result[index * 2U + 1U] = hex[data[index] & 0x0fU];
    }
    result[size * 2U] = '\0';
}

static void Fingerprint(const uint8_t *data,
    char result[FINGERPRINT_CHARS + 1U])
{
    HexEncode(data, FINGERPRINT_BYTES, result);
}

static void SetBoolean(napi_env env, napi_value object, const char *name,
    bool value)
{
    napi_value property = NULL;
    (void)napi_get_boolean(env, value, &property);
    (void)napi_set_named_property(env, object, name, property);
}

static void SetUint32(napi_env env, napi_value object, const char *name,
    uint32_t value)
{
    napi_value property = NULL;
    (void)napi_create_uint32(env, value, &property);
    (void)napi_set_named_property(env, object, name, property);
}

static void SetDouble(napi_env env, napi_value object, const char *name,
    double value)
{
    napi_value property = NULL;
    (void)napi_create_double(env, value, &property);
    (void)napi_set_named_property(env, object, name, property);
}

static void SetStringWithLength(napi_env env, napi_value object,
    const char *name, const char *value, size_t length)
{
    napi_value property = NULL;
    (void)napi_create_string_utf8(env, value, length, &property);
    (void)napi_set_named_property(env, object, name, property);
}

static void SetString(napi_env env, napi_value object, const char *name,
    const char *value)
{
    SetStringWithLength(env, object, name, value, strlen(value));
}

static napi_value CreateFailure(napi_env env, const char *message)
{
    napi_value result = NULL;
    (void)napi_create_object(env, &result);
    SetBoolean(env, result, "success", false);
    SetString(env, result, "message", message);
    return result;
}

static napi_value EncryptMessage(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = {NULL};
    napi_valuetype valueType = napi_undefined;
    size_t requestedLength = 0;
    size_t messageLength = 0;
    char plaintext[MAX_MESSAGE_BYTES + 1U] = {0};
    uint8_t publicKey[MLKEM512_PUBLICKEYBYTES] = {0};
    uint8_t senderSecret[MLKEM_BYTES] = {0};
    uint8_t keyStream[MAX_MESSAGE_BYTES] = {0};
    char ciphertextHex[MAX_MESSAGE_BYTES * 2U + 1U] = {0};
    char nonceHex[NONCE_BYTES * 2U + 1U] = {0};
    char tagHex[TAG_BYTES * 2U + 1U] = {0};
    char kemPreview[KEM_PREVIEW_BYTES * 2U + 1U] = {0};
    char fingerprint[FINGERPRINT_CHARS + 1U] = {0};
    uint64_t totalStart;
    uint64_t operationStart;
    uint64_t operationEnd;
    double keypairUs;
    double encapsUs;
    double dataEncryptUs;
    size_t index;
    napi_value result = NULL;

    (void)napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    if (argc != 1 || napi_typeof(env, argv[0], &valueType) != napi_ok ||
        valueType != napi_string) {
        return CreateFailure(env, "请输入要加密的文本");
    }
    if (napi_get_value_string_utf8(env, argv[0], NULL, 0,
        &requestedLength) != napi_ok || requestedLength == 0) {
        return CreateFailure(env, "消息不能为空");
    }
    if (requestedLength > MAX_MESSAGE_BYTES) {
        return CreateFailure(env, "消息过长，最多允许 512 个 UTF-8 字节");
    }
    if (napi_get_value_string_utf8(env, argv[0], plaintext,
        sizeof(plaintext), &messageLength) != napi_ok || messageLength == 0) {
        return CreateFailure(env, "读取输入消息失败");
    }

    g_session.ready = false;
    totalStart = MonotonicTimeNs();
    if (totalStart == 0) {
        return CreateFailure(env, "无法读取单调时钟");
    }

    operationStart = MonotonicTimeNs();
    if (mlkem_keypair(publicKey, g_session.secretKey) != 0) {
        return CreateFailure(env, "ML-KEM 密钥生成失败");
    }
    operationEnd = MonotonicTimeNs();
    keypairUs = (double)(operationEnd - operationStart) / 1000.0;

    operationStart = MonotonicTimeNs();
    if (mlkem_enc(g_session.kemCiphertext, senderSecret, publicKey) != 0) {
        return CreateFailure(env, "ML-KEM 密钥封装失败");
    }
    operationEnd = MonotonicTimeNs();
    encapsUs = (double)(operationEnd - operationStart) / 1000.0;

    operationStart = MonotonicTimeNs();
    if (randombytes(g_session.nonce, sizeof(g_session.nonce)) != 0) {
        return CreateFailure(env, "生成消息随机数失败");
    }
    DeriveKeyStream(senderSecret, g_session.nonce, keyStream, messageLength);
    for (index = 0; index < messageLength; ++index) {
        g_session.messageCiphertext[index] =
            ((const uint8_t *)plaintext)[index] ^ keyStream[index];
    }
    ComputeTag(senderSecret, g_session.nonce, g_session.messageCiphertext,
        messageLength, g_session.tag);
    operationEnd = MonotonicTimeNs();
    dataEncryptUs = (double)(operationEnd - operationStart) / 1000.0;

    g_session.messageLength = messageLength;
    (void)memcpy(g_session.senderSecret, senderSecret, MLKEM_BYTES);
    g_session.ready = true;

    HexEncode(g_session.messageCiphertext, messageLength, ciphertextHex);
    HexEncode(g_session.nonce, NONCE_BYTES, nonceHex);
    HexEncode(g_session.tag, TAG_BYTES, tagHex);
    HexEncode(g_session.kemCiphertext, KEM_PREVIEW_BYTES, kemPreview);
    Fingerprint(senderSecret, fingerprint);

    (void)napi_create_object(env, &result);
    SetBoolean(env, result, "success", true);
    SetString(env, result, "message", "消息加密成功，可以执行解密");
    SetString(env, result, "ciphertextHex", ciphertextHex);
    SetString(env, result, "nonceHex", nonceHex);
    SetString(env, result, "authTagHex", tagHex);
    SetString(env, result, "kemCiphertextPreview", kemPreview);
    SetString(env, result, "senderFingerprint", fingerprint);
    SetUint32(env, result, "messageBytes", (uint32_t)messageLength);
    SetUint32(env, result, "kemCiphertextBytes", MLKEM512_CIPHERTEXTBYTES);
    SetUint32(env, result, "hybridCiphertextBytes",
        MLKEM512_CIPHERTEXTBYTES + NONCE_BYTES + TAG_BYTES +
        (uint32_t)messageLength);
    SetDouble(env, result, "keypairUs", keypairUs);
    SetDouble(env, result, "encapsUs", encapsUs);
    SetDouble(env, result, "dataEncryptUs", dataEncryptUs);
    SetDouble(env, result, "totalEncryptUs",
        (double)(operationEnd - totalStart) / 1000.0);
    return result;
}

static napi_value DecryptMessage(napi_env env, napi_callback_info info)
{
    uint8_t receiverSecret[MLKEM_BYTES] = {0};
    uint8_t expectedTag[TAG_BYTES] = {0};
    uint8_t keyStream[MAX_MESSAGE_BYTES] = {0};
    char plaintext[MAX_MESSAGE_BYTES + 1U] = {0};
    char fingerprint[FINGERPRINT_CHARS + 1U] = {0};
    uint64_t totalStart;
    uint64_t operationStart;
    uint64_t operationEnd;
    double decapsUs;
    double dataDecryptUs;
    bool secretMatch;
    bool authValid;
    size_t index;
    napi_value result = NULL;
    (void)info;

    if (!g_session.ready) {
        return CreateFailure(env, "请先加密一条消息");
    }

    totalStart = MonotonicTimeNs();
    operationStart = totalStart;
    if (mlkem_dec(receiverSecret, g_session.kemCiphertext,
        g_session.secretKey) != 0) {
        return CreateFailure(env, "ML-KEM 密钥解封装失败");
    }
    operationEnd = MonotonicTimeNs();
    decapsUs = (double)(operationEnd - operationStart) / 1000.0;
    secretMatch = ConstantTimeEqual(receiverSecret, g_session.senderSecret,
        MLKEM_BYTES);

    operationStart = MonotonicTimeNs();
    ComputeTag(receiverSecret, g_session.nonce, g_session.messageCiphertext,
        g_session.messageLength, expectedTag);
    authValid = ConstantTimeEqual(expectedTag, g_session.tag, TAG_BYTES);
    if (authValid) {
        DeriveKeyStream(receiverSecret, g_session.nonce, keyStream,
            g_session.messageLength);
        for (index = 0; index < g_session.messageLength; ++index) {
            plaintext[index] = (char)(g_session.messageCiphertext[index] ^
                keyStream[index]);
        }
    }
    operationEnd = MonotonicTimeNs();
    dataDecryptUs = (double)(operationEnd - operationStart) / 1000.0;

    (void)napi_create_object(env, &result);
    SetBoolean(env, result, "success", authValid && secretMatch);
    SetString(env, result, "message", authValid && secretMatch ?
        "解密成功，完整性校验通过" : "解密失败：密文或认证标签已被修改");
    SetBoolean(env, result, "authValid", authValid);
    SetBoolean(env, result, "secretMatch", secretMatch);
    SetStringWithLength(env, result, "plaintext", plaintext,
        authValid ? g_session.messageLength : 0U);
    Fingerprint(receiverSecret, fingerprint);
    SetString(env, result, "receiverFingerprint", fingerprint);
    SetDouble(env, result, "decapsUs", decapsUs);
    SetDouble(env, result, "dataDecryptUs", dataDecryptUs);
    SetDouble(env, result, "totalDecryptUs",
        (double)(operationEnd - totalStart) / 1000.0);
    return result;
}

static int RunBenchmark(uint32_t iterations, KeypairFunction keypair,
    EncapsFunction encaps, DecapsFunction decaps, BenchmarkResult *result)
{
    uint8_t publicKey[MLKEM512_PUBLICKEYBYTES] = {0};
    uint8_t secretKey[MLKEM512_SECRETKEYBYTES] = {0};
    uint8_t ciphertext[MLKEM512_CIPHERTEXTBYTES] = {0};
    uint8_t sharedSecret[MLKEM_BYTES] = {0};
    volatile uint8_t sink = 0;
    uint64_t start;
    uint64_t end;
    uint32_t index;

    for (index = 0; index < BENCHMARK_WARMUP_ITERATIONS; ++index) {
        if (keypair(publicKey, secretKey) != 0 ||
            encaps(ciphertext, sharedSecret, publicKey) != 0 ||
            decaps(sharedSecret, ciphertext, secretKey) != 0) {
            return -1;
        }
    }

    start = MonotonicTimeNs();
    for (index = 0; index < iterations; ++index) {
        if (keypair(publicKey, secretKey) != 0) {
            return -1;
        }
        sink ^= publicKey[0];
    }
    end = MonotonicTimeNs();
    result->keypairUs = (double)(end - start) /
        ((double)iterations * 1000.0);

    start = MonotonicTimeNs();
    for (index = 0; index < iterations; ++index) {
        if (encaps(ciphertext, sharedSecret, publicKey) != 0) {
            return -1;
        }
        sink ^= ciphertext[0];
    }
    end = MonotonicTimeNs();
    result->encapsUs = (double)(end - start) /
        ((double)iterations * 1000.0);

    start = MonotonicTimeNs();
    for (index = 0; index < iterations; ++index) {
        if (decaps(sharedSecret, ciphertext, secretKey) != 0) {
            return -1;
        }
        sink ^= sharedSecret[0];
    }
    end = MonotonicTimeNs();
    result->decapsUs = (double)(end - start) /
        ((double)iterations * 1000.0);
    (void)sink;
    return 0;
}

static double Improvement(double baseline, double optimized)
{
    return baseline > 0.0 ? (baseline - optimized) * 100.0 / baseline : 0.0;
}

static napi_value CompareMethods(napi_env env, napi_callback_info info)
{
    uint32_t iterations = DEFAULT_BENCHMARK_ITERATIONS;
    size_t argc = 1;
    napi_value argv[1] = {NULL};
    BenchmarkResult portable = {0};
    BenchmarkResult neon = {0};
    napi_value result = NULL;

    (void)napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    if (argc == 1) {
        uint32_t requested = 0;
        if (napi_get_value_uint32(env, argv[0], &requested) == napi_ok &&
            requested > 0 && requested <= MAX_BENCHMARK_ITERATIONS) {
            iterations = requested;
        }
    }

    if (RunBenchmark(iterations, mlkem_portable_keypair,
        mlkem_portable_enc, mlkem_portable_dec, &portable) != 0) {
        return CreateFailure(env, "可移植 C 版本性能测试失败");
    }
    if (RunBenchmark(iterations, mlkem_keypair, mlkem_enc, mlkem_dec,
        &neon) != 0) {
        return CreateFailure(env, "NEON 版本性能测试失败");
    }

    (void)napi_create_object(env, &result);
    SetBoolean(env, result, "success", true);
    SetString(env, result, "message", "两种实现的现场性能对比完成");
    SetUint32(env, result, "iterations", iterations);
    SetDouble(env, result, "portableKeypairUs", portable.keypairUs);
    SetDouble(env, result, "portableEncapsUs", portable.encapsUs);
    SetDouble(env, result, "portableDecapsUs", portable.decapsUs);
    SetDouble(env, result, "neonKeypairUs", neon.keypairUs);
    SetDouble(env, result, "neonEncapsUs", neon.encapsUs);
    SetDouble(env, result, "neonDecapsUs", neon.decapsUs);
    SetDouble(env, result, "keypairImprovement",
        Improvement(portable.keypairUs, neon.keypairUs));
    SetDouble(env, result, "encapsImprovement",
        Improvement(portable.encapsUs, neon.encapsUs));
    SetDouble(env, result, "decapsImprovement",
        Improvement(portable.decapsUs, neon.decapsUs));
    return result;
}

static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor descriptors[] = {
        {"encryptMessage", NULL, EncryptMessage, NULL, NULL, NULL,
            napi_default, NULL},
        {"decryptMessage", NULL, DecryptMessage, NULL, NULL, NULL,
            napi_default, NULL},
        {"compareMethods", NULL, CompareMethods, NULL, NULL, NULL,
            napi_default, NULL},
    };
    (void)napi_define_properties(env, exports,
        sizeof(descriptors) / sizeof(descriptors[0]), descriptors);
    return exports;
}

static napi_module g_pqcMlkemModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = NULL,
    .nm_register_func = Init,
    .nm_modname = "pqc_mlkem_napi",
    .nm_priv = NULL,
    .reserved = {NULL},
};

__attribute__((constructor)) static void RegisterPqcMlkemModule(void)
{
    napi_module_register(&g_pqcMlkemModule);
}
