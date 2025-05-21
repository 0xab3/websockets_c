#ifndef __EXTRA_INT_H__
#define __EXTRA_INT_H__
#include <stdint.h>

uint32_t to_be32(uint32_t value);
uint64_t to_be64(uint64_t value);
uint32_t to_le32(uint32_t value);
uint64_t to_le64(uint64_t value);

void write_be32(uint8_t *data, uint32_t value);
void write_be64(uint8_t *data, uint64_t value);
uint32_t read_be32(const uint8_t *data);
uint32_t to_be32(uint32_t value) {
  return ((value & 0xFF000000) >> 24) | ((value & 0x00FF0000) >> 8) |
         ((value & 0x0000FF00) << 8) | ((value & 0x000000FF) << 24);
}
uint64_t to_be64(uint64_t value) {
  return ((value & 0xFF00000000000000ULL) >> 56) |
         ((value & 0x00FF000000000000ULL) >> 40) |
         ((value & 0x0000FF0000000000ULL) >> 24) |
         ((value & 0x000000FF00000000ULL) >> 8) |
         ((value & 0x00000000FF000000ULL) << 8) |
         ((value & 0x0000000000FF0000ULL) << 24) |
         ((value & 0x000000000000FF00ULL) << 40) |
         ((value & 0x00000000000000FFULL) << 56);
}
uint32_t to_le32(uint32_t value) {
  return ((value & 0x000000FFU) << 24) |
         ((value & 0x0000FF00U) << 8) |
         ((value & 0x00FF0000U) >> 8) |
         ((value & 0xFF000000U) >> 24);
}

uint64_t to_le64(uint64_t value) {
  return ((value & 0x00000000000000FFULL) << 56) |
         ((value & 0x000000000000FF00ULL) << 40) |
         ((value & 0x0000000000FF0000ULL) << 24) |
         ((value & 0x00000000FF000000ULL) << 8) |
         ((value & 0x000000FF00000000ULL) >> 8) |
         ((value & 0x0000FF0000000000ULL) >> 24) |
         ((value & 0x00FF000000000000ULL) >> 40) |
         ((value & 0xFF00000000000000ULL) >> 56);
}

uint32_t read_be32(const uint8_t *data) {
  return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
         ((uint32_t)data[2] << 8) | ((uint32_t)data[3]);
}
void write_be32(uint8_t *data, uint32_t value) {
  data[0] = (value >> 24) & 0xFF;
  data[1] = (value >> 16) & 0xFF;
  data[2] = (value >> 8) & 0xFF;
  data[3] = value & 0xFF;
  return;
}
void write_be64(uint8_t *data, uint64_t value) {
  data[0] = (value >> 56) & 0xFF;
  data[1] = (value >> 48) & 0xFF;
  data[2] = (value >> 40) & 0xFF;
  data[3] = (value >> 32) & 0xFF;
  data[4] = (value >> 24) & 0xFF;
  data[5] = (value >> 16) & 0xFF;
  data[6] = (value >> 8) & 0xFF;
  data[7] = value & 0xFF;
}

#endif
