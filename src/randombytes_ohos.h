#ifndef PQC_MLKEM_RANDOMBYTES_OHOS_H
#define PQC_MLKEM_RANDOMBYTES_OHOS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Returns 0 on success and -1 if the operating-system RNG fails. */
int randombytes(uint8_t *out, size_t outlen);

#ifdef __cplusplus
}
#endif

#endif /* PQC_MLKEM_RANDOMBYTES_OHOS_H */
