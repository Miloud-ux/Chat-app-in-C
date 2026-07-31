#ifndef HELPERS_H
#define HELPERS_H

#include <stddef.h>
#include <stdint.h>
#define MAX(x, y) ((x) > (y) ? (x) : (y))

void die(const char *err_msg);
int32_t read_full(int fd, char *buf, size_t n);
int32_t write_full(int fd, char *buf, size_t n);

#endif
