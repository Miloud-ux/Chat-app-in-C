#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "helpers.h"

void die(const char *err_msg) {
  perror(err_msg);
  exit(EXIT_FAILURE);
}

int32_t read_full(int fd, char *buf, size_t n) {
  while (n > 0) {
    ssize_t rv = read(fd, buf, n);
    if (rv < 0) {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }
    if (rv == 0) {
      return -1;
    }
    assert((size_t)rv <= n);
    n -= (size_t)rv;
    buf += rv;
  }
  return 0;
}

int32_t write_full(int fd, char *buf, size_t n) {
  while (n > 0) {
    ssize_t wv = write(fd, buf, n);
    if (wv < 0) {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }
    if (wv == 0) {
      return -1;
    }
    assert((size_t)wv <= n);
    n -= (size_t)wv;
    buf += wv;
  }
  return 0;
}
