#ifndef HASH_H
#define HASH_H

#include <stddef.h>
#include <stdint.h>

uint32_t hash32(const void *data, size_t len, uint32_t seed);

#endif /* HASH_H */
