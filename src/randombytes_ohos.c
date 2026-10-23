#include "randombytes_ohos.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

int randombytes(uint8_t *out, size_t outlen)
{
    int fd;
    size_t offset = 0;

    if (out == NULL && outlen != 0) {
        return -1;
    }

    do {
        fd = open("/dev/urandom", O_RDONLY);
    } while (fd < 0 && errno == EINTR);

    if (fd < 0) {
        return -1;
    }

    while (offset < outlen) {
        ssize_t result = read(fd, out + offset, outlen - offset);

        if (result < 0 && errno == EINTR) {
            continue;
        }
        if (result <= 0) {
            (void)close(fd);
            return -1;
        }
        offset += (size_t)result;
    }

    if (close(fd) != 0) {
        return -1;
    }
    return 0;
}
